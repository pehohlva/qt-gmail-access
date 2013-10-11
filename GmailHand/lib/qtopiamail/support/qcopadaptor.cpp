/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcopadaptor.h"
#include "qcopchannel.h"
#include <QtCore/qmap.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qatomic.h>
#include <QtCore/qdebug.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qvarlengtharray.h>

/* ! - documentation comments in this file are diasabled:
    \class QCopAdaptor
    \inpublicgroup QtBaseModule

    \ingroup ipc
    \brief The QCopAdaptor class provides an interface to messages on a QCop IPC channel which simplifies remote signal and slot invocations.

    Using this class, it is very easy to convert a signal emission into an IPC
    message on a channel, and to convert an IPC message on a channel into a
    slot invocation.  In the following example, when the signal \c{valueChanged(int)}
    is emitted from the object \c source, the IPC message \c{changeValue(int)} will
    be sent on the channel \c{QPE/Foo}:

    \code
    QCopAdaptor *adaptor = new QCopAdaptor("QPE/Foo");
    QCopAdaptor::connect
        (source, SIGNAL(valueChanged(int)), adaptor, MESSAGE(changeValue(int)));
    \endcode

    Note that we use QCopAdaptor::connect() to connect the signal to the
    IPC message, not QObject::connect().  A common error is to use \c{connect()}
    without qualifying it with \c{QCopAdaptor::} and picking up
    QObject::connect() by mistake.

    On the server side of an IPC protocol, the \c{changeValue(int)} message can
    be connected to the slot \c{setValue()} on \c dest:

    \code
    QCopAdaptor *adaptor = new QCopAdaptor("QPE/Foo");
    QCopAdaptor::connect
        (adaptor, MESSAGE(changeValue(int)), dest, SLOT(setValue(int)));
    \endcode

    Now, whenever the client emits the \c{valueChanged(int)} signal, the
    \c{setValue(int)} slot will be automatically invoked on the server side,
    with the \c int parameter passed as its argument.

    Only certain parameter types can be passed across an IPC boundary in this fashion.
    The type must be visible to QVariant as a meta-type.  Many simple built-in
    types are already visible; for user-defined types, use Q_DECLARE_METATYPE()
    and qRegisterMetaTypeStreamOperators().

    \sa QCopChannel
*/

class QCopAdaptorChannel : public QCopChannel
{
    Q_OBJECT
public:
    QCopAdaptorChannel(const QString& channel, QCopAdaptor *adapt)
        : QCopChannel(channel, adapt), adaptor(adapt) {}
    ~QCopAdaptorChannel() {}

    void receive(const QString& msg, const QByteArray &data);

private:
    QCopAdaptor *adaptor;
};

void QCopAdaptorChannel::receive(const QString& msg, const QByteArray &data)
{
    adaptor->received(msg, data);
}

class QCopAdaptorSignalInfo
{
public:
    QObject *sender;
    int signalIndex;
    int destroyIndex;
    QString message;
    int *types;
    int numArgs;
};

class QCopAdaptorSlotInfo
{
public:
    ~QCopAdaptorSlotInfo()
    {
        qFree(types);
    }

    QObject *receiver;
    int memberIndex;
    bool destroyed;
    int returnType;
    int *types;
    int numArgs;
};

class QCopAdaptorPrivate : public QObject
{
    // Do not put Q_OBJECT here.
public:
    QCopAdaptorPrivate(QCopAdaptor *obj, const QString& chan);
    ~QCopAdaptorPrivate();

    QAtomicInt ref;
    QCopAdaptor *parent;
    QString channelName;
    bool connected;
    const QMetaObject *publishedTo;
    QMultiMap<QString, QCopAdaptorSlotInfo *> invokers;
    QList<QCopAdaptorSignalInfo *> signalList;
    int slotIndex;

    static const int QVariantId = -243;
    static int *connectionTypes(const QByteArray& member, int& nargs);
    static int typeFromName(const QByteArray& name);

protected:
    int qt_metacall(QMetaObject::Call c, int id, void **a);
};

QCopAdaptorPrivate::QCopAdaptorPrivate(QCopAdaptor *obj, const QString& chan)
    : ref(1), channelName(chan)
{
    parent = obj;
    connected = false;
    publishedTo = 0;

    // Fake slots start at this index in the QMetaObject.
    slotIndex = staticMetaObject.methodCount();
}

QCopAdaptorPrivate::~QCopAdaptorPrivate()
{
    qDeleteAll(invokers);

    // Disconnect all of the signals associated with this adaptor.
    int index = slotIndex;
    foreach (QCopAdaptorSignalInfo *info, signalList) {
        if (info->signalIndex >= 0) {
            QMetaObject::disconnect(info->sender, info->signalIndex,
                                    this, index);
        }
        if (info->destroyIndex >= 0) {
            QMetaObject::disconnect(info->sender, info->destroyIndex,
                                    this, index + 1);
        }
        qFree(info->types);
        delete info;
        index += 2;
    }
}

// Get the QVariant type number for a type name.
int QCopAdaptorPrivate::typeFromName( const QByteArray& type )
{
    int id;
    if (type.endsWith('*'))
        return QMetaType::VoidStar;
    else if ( type.size() == 0 || type == "void" )
        return QMetaType::Void;
    else if ( type == "QVariant" )
        return QCopAdaptorPrivate::QVariantId;
    id = QMetaType::type( type.constData() );
    if ( id != (int)QMetaType::Void )
        return id;
    return QVariant::nameToType(type);
}

// Returns the connection types associated with a signal or slot member.
int *QCopAdaptorPrivate::connectionTypes( const QByteArray& member, int& nargs )
{
    // Based on Qt's internal queuedConnectionTypes function.
    nargs = 0;
    int *types = 0;
    const char *s = member.constData();
    while (*s != '\0' && *s != '(') { ++s; }
    if ( *s == '\0' )
        return 0;
    ++s;
    const char *e = s;
    while (*e != ')') {
        ++e;
        if (*e == ')' || *e == ',')
            ++nargs;
    }

    types = (int *) qMalloc((nargs+1)*sizeof(int));
    types[nargs] = 0;
    for (int n = 0; n < nargs; ++n) {
        e = s;
        while (*s != ',' && *s != ')')
            ++s;
        QByteArray type(e, s-e);
        ++s;

        types[n] = typeFromName(type);
        if (!types[n]) {
            qWarning("QCopAdaptorPrivate::connectionTypes: Cannot marshal arguments of type '%s'", type.constData());
            qFree(types);
            return 0;
        }
    }
    return types;
}

int QCopAdaptorPrivate::qt_metacall(QMetaObject::Call c, int id, void **a)
{
    id = QObject::qt_metacall(c, id, a);
    if (id < 0)
        return id;
    if (c == QMetaObject::InvokeMetaMethod) {
        // Each signal that we have intercepted has two fake slots
        // associated with it.  The first is the activation slot.
        // The second is the destroyed() slot for the signal's object.
        if (id < signalList.size() * 2) {
            QCopAdaptorSignalInfo *info = signalList[id / 2];
            if ((id % 2) == 0) {
                // The signal we are interested in has been activated.
                if (info->types) {
                    QList<QVariant> args;
                    for (int i = 0; i < info->numArgs; ++i) {
                        if (info->types[i] != QCopAdaptorPrivate::QVariantId) {
                            QVariant arg(info->types[i], a[i + 1]);
                            args.append(arg);
                        } else {
                            args.append(*((const QVariant *)(a[i + 1])));
                        }
                    }
                    parent->sendMessage(info->message, args);
                }
            } else {
                // The sender has been destroyed.  Clear the signal indices
                // so that we don't try to do a manual disconnect when our
                // own destructor is called.
                info->signalIndex = -1;
                info->destroyIndex = -1;
            }
        }
        id -= signalList.size() * 2;
    }
    return id;
}

// Special variant class that can perform QDataStream operations
// without the QVariant header information.
class QCopAdaptorVariant : public QVariant
{
public:
    QCopAdaptorVariant() : QVariant() {}
    explicit QCopAdaptorVariant(const QVariant& value)
        : QVariant(value) {}

    void load(QDataStream& stream, int typeOrMetaType)
    {
        clear();
        create(typeOrMetaType, 0);
        d.is_null = false;
        QMetaType::load(stream, d.type, const_cast<void *>(constData()));
    }

    void save(QDataStream& stream) const
    {
        QMetaType::save(stream, d.type, constData());
    }
};

/* !
    Construct a Qt Extended IPC message object for \a channel and attach it to \a parent.
    If \a channel is empty, then messages are taken from the application's
    \c{appMessage} channel.
*/
QCopAdaptor::QCopAdaptor(const QString& channel, QObject *parent)
    : QObject(parent)
{
    d = new QCopAdaptorPrivate(this, channel);
}

/* !
    Destroy this Qt Extended IPC messaging object.
*/
QCopAdaptor::~QCopAdaptor()
{
    if (!d->ref.deref())
        delete d;
    d = 0;
}

/* !
    Returns the name of the channel that this adaptor is associated with.
*/
QString QCopAdaptor::channel() const
{
    return d->channelName;
}

/* !
    Connects \a signal on \a sender to \a member on \a receiver.  Returns true
    if the connection was possible; otherwise returns false.

    If either \a sender or \a receiver are instances of
    QCopAdaptor, this function will arrange for the signal
    to be delivered over a Qt Extended IPC channel.  If both \a sender and
    \a receiver are local, this function is identical
    to QObject::connect().

    If the same signal is connected to same slot multiple times,
    then signal delivery will happen that many times.
*/
bool QCopAdaptor::connect(QObject *sender, const QByteArray& signal,
                          QObject *receiver, const QByteArray& member)
{
    QCopAdaptor *senderProxy;
    QCopAdaptor *receiverProxy;

    // Bail out if the parameters are invalid.
    if (!sender || signal.isEmpty() || !receiver || member.isEmpty())
        return false;

    // Resolve the objects to find the remote proxies.
    senderProxy = qobject_cast<QCopAdaptor *>(sender);
    receiverProxy = qobject_cast<QCopAdaptor *>(receiver);

    // Remove proxies if the signal or member is not tagged with MESSAGE().
    if (!member.startsWith(QMESSAGE_CODE + '0'))
        receiverProxy = 0;
    if (!signal.startsWith(QMESSAGE_CODE + '0'))
        senderProxy = 0;

    // If neither has a proxy, then use a local connect.
    if (!senderProxy && !receiverProxy)
        return QObject::connect(sender, signal, receiver, member);

    // If both are still remote proxies, then fail the request.
    if (senderProxy && receiverProxy) {
        qWarning("QCopAdaptor::connect: cannot connect MESSAGE() to MESSAGE()");
        return false;
    }

    // Determine which direction the connect needs to happen in.
    if (receiverProxy) {
        // Connecting a local signal to a remote slot.
        return receiverProxy->connectLocalToRemote(sender, signal, member);
    } else {
        // Connecting a remote signal to a local slot.
        return senderProxy->connectRemoteToLocal(signal, receiver, member);
    }
}

/* !
    Publishes the signal or slot called \a member on this object on
    the Qt Extended IPC channel represented by this QCopAdaptor.

    If \a member is a slot, then whenever an application sends a
    message to the channel with that name, the system will arrange
    for the slot to be invoked.

    If \a member is a signal, then whenever this object emits that
    signal, the system will arrange for a message with that name to
    be sent on the channel.

    Returns false if \a member does not refer to a valid signal or slot.

    \sa publishAll()
*/
bool QCopAdaptor::publish(const QByteArray& member)
{
    // '1' is QSLOT_CODE in Qt 4.4 and below,
    // '5' is QSLOT_CODE in Qt 4.5 and higher.
    if (member.size() >= 1 && (member[0] == '1' || member[0] == '5')) {
        // Exporting a slot.
        return connectRemoteToLocal("3" + member.mid(1), this, member);
    } else {
        // Exporting a signal.
        return connectLocalToRemote(this, member, member);
    }
}

/* !
    \enum QCopAdaptor::PublishType
    Type of members to publish via QCopAdaptor.

    \value Signals Publish only signals.
    \value Slots Publish only public slots.
    \value SignalsAndSlots Publish both signals and public slots.
*/

/* !
    Publishes all signals or public slots on this object within subclasses of
    QCopAdaptor.  This is typically called from a subclass constructor.
    The \a type indicates if all signals, all public slots, or both, should
    be published.  Private and protected slots will never be published.

    \sa publish()
*/
void QCopAdaptor::publishAll(QCopAdaptor::PublishType type)
{
    const QMetaObject *meta = metaObject();
    if (meta != d->publishedTo) {
        int count = meta->methodCount();
        int index;
        if (d->publishedTo)
            index = d->publishedTo->methodCount();
        else
            index = QCopAdaptor::staticMetaObject.methodCount();
        for (; index < count; ++index) {

            QMetaMethod method = meta->method(index);
            if (method.methodType() == QMetaMethod::Slot &&
                 method.access() == QMetaMethod::Public &&
                 (type == Slots || type == SignalsAndSlots)) {
                QByteArray name = method.signature();
                connectRemoteToLocal("3" + name, this, "1" + name);
            } else if (method.methodType() == QMetaMethod::Signal &&
                        (type == Signals || type == SignalsAndSlots)) {
                QByteArray name = method.signature();
                connectLocalToRemote(this, "2" + name, "3" + name);
            }
        }
        d->publishedTo = meta;
    }
}

/* !
    Sends a message on the Qt Extended IPC channel which will cause the invocation
    of \a member on receiving objects.  The return value can be used
    to add arguments to the message before transmission.
*/
QCopAdaptorEnvelope QCopAdaptor::send(const QByteArray& member)
{
    return QCopAdaptorEnvelope
        (sendChannels(d->channelName), memberToMessage(member));
}

/* !
    Sends a message on the Qt Extended IPC channel which will cause the invocation
    of the single-argument \a member on receiving objects, with the
    argument \a arg1.
*/
void QCopAdaptor::send(const QByteArray& member, const QVariant &arg1)
{
    QList<QVariant> args;
    args.append(arg1);
    sendMessage(memberToMessage(member), args);
}

/* !
    Sends a message on the Qt Extended IPC channel which will cause the invocation
    of the double-argument \a member on receiving objects, with the
    arguments \a arg1 and \a arg2.
*/
void QCopAdaptor::send(const QByteArray& member, const QVariant &arg1, const QVariant &arg2)
{
    QList<QVariant> args;
    args.append(arg1);
    args.append(arg2);
    sendMessage(memberToMessage(member), args);
}

/* !
    Sends a message on the Qt Extended IPC channel which will cause the invocation
    of the triple-argument \a member on receiving objects, with the
    arguments \a arg1, \a arg2, and \a arg3.
*/
void QCopAdaptor::send(const QByteArray& member, const QVariant &arg1,
                       const QVariant &arg2, const QVariant &arg3)
{
    QList<QVariant> args;
    args.append(arg1);
    args.append(arg2);
    args.append(arg3);
    sendMessage(memberToMessage(member), args);
}

/* !
    Sends a message on the Qt Extended IPC channel which will cause the invocation
    of the multi-argument \a member on receiving objects, with the
    argument list \a args.
*/
void QCopAdaptor::send(const QByteArray& member, const QList<QVariant>& args)
{
    sendMessage(memberToMessage(member), args);
}

/* !
    Returns true if the message on the Qt Extended IPC channel corresponding to \a signal
    has been connected to a local slot; otherwise returns false.
*/
bool QCopAdaptor::isConnected(const QByteArray& signal)
{
    return d->invokers.contains(memberToMessage(signal));
}

/* !
    Converts a signal or slot \a member name into a Qt Extended IPC message name.
    The default implementation strips the signal or slot prefix number
    from \a member and then normalizes the name to convert types
    such as \c{const QString&} into QString.

    \sa QMetaObject::normalizedSignature()
*/
QString QCopAdaptor::memberToMessage(const QByteArray& member)
{
    if (member.size() >= 1 && member[0] >= '0' && member[0] <= '9') {
        return QString::fromLatin1
            (QMetaObject::normalizedSignature(member.constData() + 1));
    } else {
        return QString::fromLatin1(member.data(), member.size());
    }
}

/* !
    Converts \a channel into a list of names to use for sending messages.
    The default implementation returns a list containing just \a channel.

    \sa receiveChannel()
*/
QStringList QCopAdaptor::sendChannels(const QString& channel)
{
    QStringList list;
    list << channel;
    return list;
}

/* !
    Converts \a channel into a new name to use for receiving messages.
    The default implementation returns \a channel.

    \sa sendChannels()
*/
QString QCopAdaptor::receiveChannel(const QString& channel)
{
    return channel;
}

void QCopAdaptor::received(const QString& msg, const QByteArray& data)
{
    // Increase the reference count on the private data just
    // in case the QCopAdaptor is deleted by one of the slots.
    QCopAdaptorPrivate *priv = d;
    priv->ref.ref();

    // Iterate through the slots for the message and invoke them.
    QMultiMap<QString, QCopAdaptorSlotInfo *>::ConstIterator iter;
    for (iter = priv->invokers.find(msg);
         iter != priv->invokers.end() && iter.key() == msg; ++iter) {
        QCopAdaptorSlotInfo *info = iter.value();
        if (info->destroyed)
            continue;

        // Convert "data" into a set of arguments suitable for qt_metacall.
        QDataStream stream(data);
        QList<QVariant> args;
        QVariant returnValue;
        QVarLengthArray<void *, 32> a(info->numArgs + 1);
        if (info->returnType != (int)QVariant::Invalid) {
            returnValue = QVariant(info->returnType, (const void *)0);
            a[0] = returnValue.data();
        } else {
            a[0] = 0;
        }
        for (int arg = 0; arg < info->numArgs; ++arg) {
            if (info->types[arg] != QCopAdaptorPrivate::QVariantId) {
                QCopAdaptorVariant temp;
                temp.load(stream, info->types[arg]);
                args.append(temp);
                a[arg + 1] = (void *)(args[arg].data());
            } else {
                // We need to handle QVariant specially because we actually
                // need the type header in this case.
                QVariant temp;
                stream >> temp;
                args.append(temp);
                a[arg + 1] = (void *)&(args[arg]);
            }
        }

        // Invoke the specified slot.
    #if !defined(QT_NO_EXCEPTIONS)
        try {
    #endif
            info->receiver->qt_metacall
                (QMetaObject::InvokeMetaMethod, info->memberIndex, a.data());
    #if !defined(QT_NO_EXCEPTIONS)
        } catch (...) {
        }
    #endif
    }

    // Decrease the reference count and delete if necessary.
    if (!priv->ref.deref())
        delete priv;
}

void QCopAdaptor::receiverDestroyed()
{
    // Mark all slot information blocks that match the receiver
    // as destroyed so that we don't try to invoke them again.
    QObject *obj = sender();
    QMultiMap<QString, QCopAdaptorSlotInfo *>::Iterator it;
    for (it = d->invokers.begin(); it != d->invokers.end(); ++it) {
        if (it.value()->receiver == obj)
            it.value()->destroyed = true;
    }
}

bool QCopAdaptor::connectLocalToRemote
    (QObject *sender, const QByteArray& signal, const QByteArray& member)
{
    QCopAdaptorSignalInfo *info = new QCopAdaptorSignalInfo();
    info->sender = sender;
    info->message = memberToMessage(member);

    // Resolve the signal name on the sender object.
    if (signal.size() > 0) {
        if (signal[0] != (QSIGNAL_CODE + '0')) {
            qWarning("QCopAdaptor: `%s' is not a valid signal "
                     "specification", signal.constData());
            delete info;
            return false;
        }
        QByteArray signalName =
            QMetaObject::normalizedSignature(signal.constData() + 1);
        info->signalIndex
            = sender->metaObject()->indexOfSignal(signalName.constData());
        if (info->signalIndex < 0) {
            qWarning("QCopAdaptor: no such signal: %s::%s",
                     sender->metaObject()->className(), signalName.constData());
            delete info;
            return false;
        }
        info->destroyIndex
            = sender->metaObject()->indexOfSignal("destroyed()");
        info->types = QCopAdaptorPrivate::connectionTypes
            (signalName, info->numArgs);
    } else {
        delete info;
        return false;
    }

    // Connect up the signals.
    int index = d->slotIndex + d->signalList.size() * 2;
    QMetaObject::connect(sender, info->signalIndex, d, index,
                         Qt::DirectConnection, 0);
    if (info->destroyIndex >= 0) {
        QMetaObject::connect(sender, info->destroyIndex, d, index + 1,
                             Qt::DirectConnection, 0);
    }

    // Add the signal information to the active list.
    d->signalList.append(info);
    return true;
}

bool QCopAdaptor::connectRemoteToLocal
    (const QByteArray& signal, QObject *receiver, const QByteArray& member)
{
    // Make sure that we are actively monitoring the channel for messages.
    if (!d->connected) {
        QString chan = receiveChannel(d->channelName);
        if (chan.isEmpty()) {
            // Qt Extended uses an empty channel name to indicate the
            // "application channel".  Messages on this special channel
            // are made available via the appMessage() signal on the
            // application object.
            QObject::connect
                (qApp, SIGNAL(appMessage(QString,QByteArray)),
                 this, SLOT(received(QString,QByteArray)));
        } else {
            // Short-cut the signal emits in QCopChannel for greater
            // performance when dispatching incoming messages.
            new QCopAdaptorChannel(chan, this);
        }
        d->connected = true;
    }

    // Create a slot invoker to handle executing the member when necessary.
    QCopAdaptorSlotInfo *info = new QCopAdaptorSlotInfo();
    QByteArray name;
    if (member.size() > 0 && member[0] >= '0' && member[0] <= '9') {
        // Strip off the member type code.
        name = QMetaObject::normalizedSignature(member.constData() + 1);
    } else {
        name = QMetaObject::normalizedSignature(member.constData());
    }
    info->receiver = receiver;
    info->destroyed = false;
    info->returnType = 0;
    info->types = 0;
    info->numArgs = 0;
    if (receiver && name.size() > 0) {
        info->memberIndex
            = receiver->metaObject()->indexOfMethod(name.constData());
        if (info->memberIndex != -1) {
            connect(receiver, SIGNAL(destroyed()), this, SLOT(receiverDestroyed()));
            QMetaMethod method = receiver->metaObject()->method(info->memberIndex);
            info->returnType = QCopAdaptorPrivate::typeFromName(method.typeName());
            info->types = QCopAdaptorPrivate::connectionTypes(name, info->numArgs);
            if (!(info->types))
                info->destroyed = true;
        } else {
            qWarning("QCopAdaptor: no such member: %s::%s",
                     receiver->metaObject()->className(), name.constData());
        }
    } else {
        info->memberIndex = -1;
    }
    if (info->memberIndex == -1) {
        delete info;
        return false;
    }
    d->invokers.insert(memberToMessage(signal), info);
    return true;
}

void QCopAdaptor::sendMessage(const QString& msg, const QList<QVariant>& args)
{
    send(sendChannels(d->channelName), msg, args);
}

void QCopAdaptor::send
        (const QStringList& channels,
         const QString& msg, const QList<QVariant>& args)
{
    QByteArray array;
    {
        QDataStream stream
            (&array, QIODevice::WriteOnly | QIODevice::Append);
        QList<QVariant>::ConstIterator iter;
        if (!msg.contains(QLatin1String("QVariant"))) {
            for (iter = args.begin(); iter != args.end(); ++iter) {
                QCopAdaptorVariant copy(*iter);
                copy.save(stream);
            }
        } else {
            QByteArray name = msg.toLatin1();
            name = QMetaObject::normalizedSignature(name.constData());
            int numParams = 0;
            int *params = QCopAdaptorPrivate::connectionTypes
                    (name, numParams);
            int index = 0;
            for (iter = args.begin(); iter != args.end(); ++iter, ++index) {
                if (index < numParams &&
                     params[index] == QCopAdaptorPrivate::QVariantId) {
                    // We need to handle QVariant specially because we actually
                    // need the type header in this case.
                    stream << *iter;
                } else {
                    QCopAdaptorVariant copy(*iter);
                    copy.save(stream);
                }
            }
            if (params)
                qFree(params);
        }
        // Stream is flushed and closed at this point.
    }
    QStringList::ConstIterator iter;
    for (iter = channels.begin(); iter != channels.end(); ++iter)
        QCopChannel::send(*iter, msg, array);
}

/* !
    \class QCopAdaptorEnvelope
    \inpublicgroup QtBaseModule

    \ingroup ipc
    \brief The QCopAdaptorEnvelope class provides a mechanism to send Qt Extended IPC messages with an argument number of arguments.


    The most common way to use this class is to call QCopAdaptor::send(),
    as demonstrated in the following example:

    \code
    QCopAdaptor *channel = ...;
    QCopAdaptorEnvelope env = channel->send(MESSAGE(foo(QString)));
    env << "Hello";
    \endcode

    \sa QCopAdaptor
*/

class QCopAdaptorEnvelopePrivate
{
public:
    QStringList channels;
    QString message;
    bool shouldBeSent;
    QList<QVariant> arguments;
};

QCopAdaptorEnvelope::QCopAdaptorEnvelope
        (const QStringList& channels, const QString& message)
{
    d = new QCopAdaptorEnvelopePrivate();
    d->channels = channels;
    d->message = message;
    d->shouldBeSent = true;
}

/* !
    Construct an empty QCopAdaptorEnvelope.
*/
QCopAdaptorEnvelope::QCopAdaptorEnvelope()
{
    d = new QCopAdaptorEnvelopePrivate();
    d->shouldBeSent = false;
}

/* !
    Construct a copy of \a value.
*/
QCopAdaptorEnvelope::QCopAdaptorEnvelope(const QCopAdaptorEnvelope& value)
{
    d = new QCopAdaptorEnvelopePrivate();
    d->channels = value.d->channels;
    d->message = value.d->message;
    d->arguments = value.d->arguments;
    d->shouldBeSent = true;

    // If we make a copy of another object, that other object
    // must not be transmitted.  This typically happens when
    // we do the following:
    //
    //  QCopAdaptorEnvelope env = channel->send(MESSAGE(foo(QString)));
    //  env << "Hello";
    //
    // The intermediate copies of the envelope, prior to the arguments
    // being added, must not be transmitted.  Only the final version is.
    value.d->shouldBeSent = false;
}

/* !
    Destroy this envelope object and send the message.
*/
QCopAdaptorEnvelope::~QCopAdaptorEnvelope()
{
    if (d->shouldBeSent)
        QCopAdaptor::send(d->channels, d->message, d->arguments);
    delete d;
}

/* !
    Copy \a value into this object.
*/
QCopAdaptorEnvelope& QCopAdaptorEnvelope::operator=(const QCopAdaptorEnvelope& value)
{
    if (&value == this)
        return *this;

    d->channels = value.d->channels;
    d->message = value.d->message;
    d->arguments = value.d->arguments;

    // Don't transmit the original copy.  See above for details.
    d->shouldBeSent = true;
    value.d->shouldBeSent = false;

    return *this;
}

/* !
    \fn QCopAdaptorEnvelope& QCopAdaptorEnvelope::operator<<(const char *value)

    \overload
    Add \a value to the arguments for this Qt Extended IPC message.
*/

/* !
    \fn QCopAdaptorEnvelope& QCopAdaptorEnvelope::operator<<(const T &value)
    Add \a value to the arguments for this Qt Extended IPC message.
 */

void QCopAdaptorEnvelope::addArgument(const QVariant& value)
{
    d->arguments.append(value);
}

#include "qcopadaptor.moc"
