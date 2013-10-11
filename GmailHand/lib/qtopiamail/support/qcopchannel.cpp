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

#include "qcopchannel.h"
#include "qcopserver.h"
#include "qcopchannel_p.h"
#include "qcopapplicationchannel.h"
#include "qlist.h"
#include "qmap.h"
#include "qdatastream.h"
#include "qregexp.h"
#include "qcoreapplication.h"
#include "qeventloop.h"
#include "qdebug.h"
#include "qtimer.h"
#include "qshareddata.h"
#include "qthreadstorage.h"
#include "qthread.h"
#include "qcoreevent.h"

QCopServerRegexp::QCopServerRegexp(const QString& ch, QCopClient *cl)
    : channel(ch), client(cl)
{
    if (ch.endsWith(QChar('*')) && ch.count(QChar('*')) == 1) {
        prefixMatch = ch.length() - 1;
    } else {
        prefixMatch = 0;
        regexp = QRegExp(ch, Qt::CaseSensitive, QRegExp::Wildcard);
    }
}

bool QCopServerRegexp::match(const QString& ch) const
{
    if (prefixMatch) {
        if (ch.length() >= prefixMatch)
            return !memcmp(ch.constData(), channel.constData(), prefixMatch * sizeof(ushort));
        else
            return false;
    } else {
        return regexp.exactMatch(ch);
    }
}

QCopServerRegexpList::~QCopServerRegexpList()
{
    QCopServerRegexp *current = first;
    QCopServerRegexp *next = 0;
    while (current != 0) {
        next = current->next;
        delete current;
        current = next;
    }
}

// Determine if a channel name contains wildcard characters.
static inline bool containsWildcards(const QString& channel)
{
    return channel.contains(QChar('*'));
}

#ifndef QT_NO_THREAD

static QThreadStorage<QCopThreadData *> qcopThreadStorage;

static inline QCopThreadData *qcopThreadData()
{
    QCopThreadData *data = qcopThreadStorage.localData();
    if (data)
        return data;
    data = new QCopThreadData();
    qcopThreadStorage.setLocalData(data);
    return data;
}

#else

Q_GLOBAL_STATIC(QCopThreadData, qcopThreadData);

#endif

QCopThreadData *QCopThreadData::instance()
{
    return qcopThreadData();
}

/* ! - documentation comments in this file are disabled:
    \class QCopChannel
    \inpublicgroup QtBaseModule
    \ingroup qws

    \brief The QCopChannel class provides communication capabilities
    between clients.

    QCOP is a many-to-many communication protocol for transferring
    messages on various channels. A channel is identified by a name,
    and anyone who wants to can listen to it. The QCOP protocol allows
    clients to communicate both within the same address space and
    between different processes, but it is currently only available
    for \l {Qt for Embedded Linux} (on X11 and Windows we are exploring the use
    of existing standards such as DCOP and COM).

    Typically, QCopChannel is either used to send messages to a
    channel using the provided static functions, or to listen to the
    traffic on a channel by deriving from the class to take advantage
    of the provided functionality for receiving messages.

    QCopChannel provides a couple of static functions which are usable
    without an object: The send() function, which sends the given
    message and data on the specified channel, and the isRegistered()
    function which queries the server for the existence of the given
    channel.

    In addition, the QCopChannel class provides the channel() function
    which returns the name of the object's channel, the virtual
    receive() function which allows subclasses to process data
    received from their channel, and the received() signal which is
    emitted with the given message and data when a QCopChannel
    subclass receives a message from its channel.

    \sa QCopServer, {Running Qt for Embedded Linux Applications}
*/

/* !
    Constructs a QCop channel with the given \a parent, and registers it
    with the server using the given \a channel name.

    \sa isRegistered(), channel()
*/

QCopChannel::QCopChannel(const QString& channel, QObject *parent)
    : QObject(parent)
{
    d = new QCopChannelPrivate(this, channel);
    d->ref.ref();

    if (!qApp) {
        qFatal("QCopChannel: Must construct a QApplication "
               "before QCopChannel");
        return;
    }

    QCopThreadData *td = qcopThreadData();

    // do we need a new channel list ?
    QCopClientMap::Iterator it = td->clientMap.find(channel);
    if (it != td->clientMap.end()) {
        it.value().append(QCopChannelPrivatePointer(d));
        return;
    }

    it = td->clientMap.insert(channel, QList<QCopChannelPrivatePointer>());
    it.value().append(QCopChannelPrivatePointer(d));

    // Inform the server about this channel
    td->clientConnection()->registerChannel(channel);
}

/* !
  \internal

  Resend all channel registrations.
  */
void QCopChannel::reregisterAll()
{
    QCopThreadData *td = qcopThreadData();

    foreach (const QString &channel, td->clientMap.keys()) {
        td->clientConnection()->registerChannel(channel);
    }
}

/* !
    Destroys the client's end of the channel and notifies the server
    that the client has closed its connection. The server will keep
    the channel open until the last registered client detaches.

    \sa QCopChannel()
*/

QCopChannel::~QCopChannel()
{
    QCopThreadData *td = qcopThreadData();

    QCopClientMap::Iterator it = td->clientMap.find(d->channel);
    Q_ASSERT(it != td->clientMap.end());
    it.value().removeAll(QCopChannelPrivatePointer(d));
    // still any clients connected locally ?
    if (it.value().isEmpty()) {
        if (td->hasClientConnection())
            td->clientConnection()->detachChannel(d->channel);
        td->clientMap.remove(d->channel);
    }

    // Dereference the private data structure.  It may stay around
    // for a little while longer if it is in use by sendLocally().
    d->object = 0;
    if (!d->ref.deref())
        delete d;
}

/* !
    Returns the name of the channel.

    \sa QCopChannel()
*/

QString QCopChannel::channel() const
{
    return d->channel;
}

/* !
    \fn void QCopChannel::receive(const QString& message, const QByteArray &data)

    This virtual function allows subclasses of QCopChannel to process
    the given \a message and \a data received from their channel. The default
    implementation emits the received() signal.

    Note that the format of the given \a data has to be well defined
    in order to extract the information it contains. In addition, it
    is recommended to use the DCOP convention. This is not a
    requirement, but you must ensure that the sender and receiver
    agree on the argument types.

    Example:

    \code
        void MyClass::receive(const QString &message, const QByteArray &data)
        {
            QDataStream in(data);
            if (message == "execute(QString,QString)") {
                QString cmd;
                QString arg;
                in >> cmd >> arg;
                ...
            } else if (message == "delete(QString)") {
                QString fileName;
                in >> fileName;
                ...
            } else {
                ...
            }
        }
    \endcode

    This example assumes that the \c message is a DCOP-style function
    signature and the \c data contains the function's arguments.

    \sa send()
 */
void QCopChannel::receive(const QString& msg, const QByteArray &data)
{
    emit received(msg, data);
}

/* !
    \fn void QCopChannel::received(const QString& message, const QByteArray &data)

    This signal is emitted with the given \a message and \a data whenever the
    receive() function gets incoming data.

    \sa receive()
*/

/* !
    \fn void QCopChannel::forwarded(const QString& msg, const QByteArray &data, const QString& channel)

    This signal is emitted if the QCopChannel is listening on a wildcard
    channel name (e.g. \c{QPE/Prefix*}).  The \a msg and \a data parameters
    contain the usual message information.  The \a channel parameter
    specifies the name of the actual channel the message was sent to
    before being forwarded to the wildcard channel.

    If this signal is not connected, forwarded messages will be delivered
    via the receive() method and the received() signal with a message name of
    \c{forwardedMessage(QString,QString,QByteArray)}.  The three data
    parameters will be the actual channel name, message, and data payload.

    The forwarded() signal is more efficient than using received() for
    forwarded messages.

    \sa received()
*/

/* !
    \internal
*/
void QCopChannel::connectNotify(const char *signal)
{
    if (QLatin1String(signal) == SIGNAL(forwarded(QString,QByteArray,QString)))
        d->useForwardedSignal = true;
    QObject::connectNotify(signal);
}

/* !
    Queries the server for the existence of the given \a channel. Returns true
    if the channel is registered; otherwise returns false.

    This function will block while the registration status is determined.
    The QCopChannelMonitor class provides a better mechanism for monitoring
    the registration state of a channel.

    \sa channel(), QCopChannel(), QCopChannelMonitor
*/

bool QCopChannel::isRegistered(const QString&  channel)
{
    QCopClient *client = qcopThreadData()->clientConnection();
    client->requestRegistered(channel);
    return client->waitForIsRegistered();
}

/* !
    \fn bool QCopChannel::send(const QString& channel, const QString& message)
    \overload

    Sends the given \a message on the specified \a channel.  The
    message will be distributed to all clients subscribed to the
    channel.
*/

bool QCopChannel::send(const QString& channel, const QString& msg)
{
    QByteArray data;
    return send(channel, msg, data);
}

/* !
    \fn bool QCopChannel::send(const QString& channel, const QString& message,
                       const QByteArray &data)

    Sends the given \a message on the specified \a channel with the
    given \a data.  The message will be distributed to all clients
    subscribed to the channel. Returns true if the message is sent
    successfully; otherwise returns false.

    It is recommended to use the DCOP convention. This is not a
    requirement, but you must ensure that the sender and receiver
    agree on the argument types.

    Note that QDataStream provides a convenient way to fill the byte
    array with auxiliary data.

    Example:

    \code
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out << QString("cat") << QString("file.txt");
        QCopChannel::send("System/Shell", "execute(QString,QString)", data);
    \endcode

    Here the channel is \c "System/Shell". The \c message is an
    arbitrary string, but in the example we've used the DCOP
    convention of passing a function signature. Such a signature is
    formatted as \c "functionname(types)" where \c types is a list of
    zero or more comma-separated type names, with no whitespace, no
    consts and no pointer or reference marks, i.e. no "*" or "&".

    \sa receive()
*/

bool QCopChannel::send(const QString& channel, const QString& msg,
                       const QByteArray &data)
{
    if (!qApp) {
        qFatal("QCopChannel::send: Must construct a QApplication "
               "before using QCopChannel");
        return false;
    }

    qcopThreadData()->clientConnection()->send(channel, msg, data);

    return true;
}

/* !
    \since 4.2

    Flushes any pending messages queued through QCopChannel::send() to any subscribed clients.
    Returns false if no QApplication has been constructed, otherwise returns true.
    When using QCopChannel::send(), messages are queued and actually sent when Qt re-enters the event loop.
    By using this function, an application can immediately flush these queued messages,
    and therefore reliably know that any subscribed clients will receive them.
*/
bool QCopChannel::flush()
{
    if (!qApp) {
        qFatal("QCopChannel::flush: Must construct a QApplication "
               "before using QCopChannel");
        return false;
    }

    QCopThreadData *td = qcopThreadData();
    if (td->hasClientConnection())
        td->clientConnection()->flush();

    return true;
}

void QCopClient::handleRegisterChannel(const QString& ch)
{
    QCopThreadData *td = qcopThreadData();

    // do we need a new channel list ?
    QCopServerMap::Iterator it = td->serverMap.find(ch);
    if (it == td->serverMap.end())
        it = td->serverMap.insert(ch, QList<QCopClient*>());

    // If the channel name contains wildcard characters, then we also
    // register it on the server regexp matching list.
    if (containsWildcards(ch)) {
	QCopServerRegexp *item = new QCopServerRegexp(ch, this);
	td->serverRegexpList.append(item);
    }

    // If this is the first client in the channel, announce the
    // channel as being created and inform any monitors on the channel.
    if (it.value().count() == 0) {
        QCopServerMap::ConstIterator itmon = td->serverMonitorMap.find(ch);
        if (itmon != td->serverMonitorMap.constEnd()) {
            QList<QCopClient *> clients = itmon.value();
            foreach (QCopClient *cl, clients)
                cl->sendChannelCommand(QCopCmd_MonitorRegistered, ch);
        }
    }

    it.value().append(this);
    ++channelCount;
}

void QCopClient::handleRequestRegistered(const QString& ch)
{
    QCopThreadData *td = qcopThreadData();
    bool known = td->serverMap.contains(ch) && !td->serverMap[ch].isEmpty();
    isRegisteredReply(ch, known);
}

void QCopClient::detachAll()
{
    // If all channels were detached cleanly, then there is nothing to do.
    if (!channelCount)
        return;

    QCopThreadData *td = qcopThreadData();

    QCopServerMap::Iterator it = td->serverMap.begin();
    while (it != td->serverMap.end()) {
        if (it.value().contains(this)) {
            it.value().removeAll(this);
            // If this was the last client in the channel, announce the channel as dead.
            if (it.value().isEmpty()) {
                // If there are monitors on this channel, then notify them.
                QCopServerMap::ConstIterator itmon = td->serverMonitorMap.find(it.key());
                if (itmon != td->serverMonitorMap.constEnd()) {
                    QList<QCopClient *> clients = itmon.value();
                    foreach (QCopClient *cl, clients) {
                        if (cl != this)
                            cl->sendChannelCommand(QCopCmd_MonitorUnregistered, it.key());
                    }
                }
                it = td->serverMap.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }

    // Remove the client from the monitor map.
    it = td->serverMonitorMap.begin();
    while (it != td->serverMonitorMap.end()) {
        it.value().removeAll(this);
        if (it.value().isEmpty())
            it = td->serverMonitorMap.erase(it);
        else
            ++it;
    }

    if (td->serverRegexpList.isEmpty())
	return;

    QCopServerRegexpList::Iterator it2;
    td->serverRegexpList.begin(it2);
    while (!it2.atEnd()) {
	if (it2.current->client == this)
	    td->serverRegexpList.erase(it2);
	else
	    it2.advance();
    }

    channelCount = 0;
}

void QCopClient::detach(const QString& ch)
{
    QCopThreadData *td = qcopThreadData();

    QCopServerMap::Iterator it = td->serverMap.find(ch);
    if (it != td->serverMap.end()) {
        --channelCount;
        it.value().removeAll(this);
        if (it.value().isEmpty()) {
            // If this was the last client in the channel, announce the channel as dead
            td->serverMap.erase(it);

            // If there are monitors on this channel, then notify them.
            QCopServerMap::ConstIterator itmon = td->serverMonitorMap.find(ch);
            if (itmon != td->serverMonitorMap.constEnd()) {
                QList<QCopClient *> clients = itmon.value();
                foreach (QCopClient *cl, clients)
                    cl->sendChannelCommand(QCopCmd_MonitorUnregistered, ch);
            }
        }
    }
    if (!td->serverRegexpList.isEmpty() && containsWildcards(ch)) {
        // Remove references to a wildcarded channel.
        QCopServerRegexpList::Iterator it;
        td->serverRegexpList.begin(it);
        while (!it.atEnd()) {
            if (it.current->client == this && it.current->channel == ch) {
                td->serverRegexpList.erase(it);
            } else {
                it.advance();
            }
        }
    }
}

void QCopClient::answer(const QString& ch, const char *packet, int packetLen)
{
    QCopThreadData *td = qcopThreadData();

    QList<QCopClient*> clist = td->serverMap.value(ch);
    for (int i=0; i < clist.size(); ++i) {
        QCopClient *c = clist.at(i);
        c->write(packet, packetLen);
    }

    if(!td->serverRegexpList.isEmpty() && !containsWildcards(ch)) {
	// Search for wildcard matches and forward the message on.
	QCopServerRegexpList::Iterator it;
        td->serverRegexpList.begin(it);
	while (!it.atEnd()) {
	    if (it.current->match(ch))
		it.current->client->forward(packet, it.current->channel);
            it.advance();
	}
    }
}

/* !
    \internal
    Client side: distribute received event to the QCop instance managing the
    channel.
*/
void QCopChannel::sendLocally(const QString& ch, const QString& msg,
                                const QByteArray &data)
{
    // filter out internal events
    if (ch.isEmpty())
        return;

    // feed local clients with received data
    QCopThreadData *td = qcopThreadData();
    QList<QCopChannelPrivatePointer> clients = td->clientMap[ch];
    for (int i = 0; i < clients.size(); ++i) {
	QCopChannelPrivate *channel = clients.at(i).data();
	if (channel->object)
	    channel->object->receive(msg, data);
    }
}

void QCopClient::handleStartupComplete(const QString& ch)
{
    QCopThreadData *td = qcopThreadData();
    isStartupComplete = true;
    QList<QCopChannelPrivatePointer> clients = td->clientMap[ch];
    for (int i = 0; i < clients.size(); ++i) {
	QCopChannelPrivate *channel = clients.at(i).data();
	if (channel->object) {
            QCopApplicationChannel *appchan =
                qobject_cast<QCopApplicationChannel *>(channel->object);
            if (appchan)
	        emit appchan->startupComplete();
        }
    }
}

void QCopClient::forwardLocally
    (const QString& forwardTo, const QString& ch, const QString& msg,
     const QByteArray& data)
{
    QCopThreadData *td = qcopThreadData();
    QList<QCopChannelPrivatePointer> clients = td->clientMap[forwardTo];
    QByteArray newData;
    for (int i = 0; i < clients.size(); ++i) {
	QCopChannelPrivate *channel = clients.at(i).data();
	if (channel->object) {
            if (channel->useForwardedSignal) {
	        emit channel->object->forwarded(msg, data, ch);
            } else {
                // Use the old-style forwarding mechanism for
                // backwards compatibility.
                if (newData.isEmpty()) {
                    QDataStream stream
                        (&newData, QIODevice::WriteOnly | QIODevice::Append);
                    stream << ch;
                    stream << msg;
                    stream << data;
                    // Stream is flushed and closed at this point.
                }
	        channel->object->receive
                    (QLatin1String("forwardedMessage(QString,QString,QByteArray)"),
                     newData);
            }
        }
    }
}

struct QCopPacketHeader
{
    int totalLength;
    int command;
    int chLength;
    int msgLength;
    int dataLength;
    int forwardToLength;
};

QCopClient::QCopClient(QIODevice *device, QCopLocalSocket *socket)
    : QObject()
{
    this->device = device;
    this->socket = socket;
    server = true;
    disconnectHandler = 0;
    init();
}

QCopClient::QCopClient(QIODevice *device, bool isServer)
    : QObject()
{
    this->device = device;
    this->socket = 0;
    server = isServer;
    disconnectHandler = 0;
    init();
}

void QCopClient::init()
{
    if (server || !socket)
        connectSignals();

    isStartupComplete = false;

    inBufferUsed = 0;
    inBufferExpected = minPacketSize;
    inBufferPtr = inBuffer;

    isRegisteredResponse = false;
    isRegisteredWaiter = 0;

    retryCount = 0;
    connecting = false;
    reconnecting = false;

    channelCount = 0;

    finished = false;

    loopback = qobject_cast<QCopLoopbackDevice *>(device);
}

QCopClient::~QCopClient()
{
    if (disconnectHandler) {
        delete disconnectHandler;
        disconnectHandler = 0;
    }
    if (socket) {
        delete socket;
        socket = 0;
    }
}

void QCopClient::registerChannel(const QString& ch)
{
    sendChannelCommand(QCopCmd_RegisterChannel, ch);
}

void QCopClient::detachChannel(const QString& ch)
{
    sendChannelCommand(QCopCmd_DetachChannel, ch);
}

void QCopClient::sendChannelCommand(int cmd, const QString& ch)
{
    int len = ch.length() * 2 + sizeof(QCopPacketHeader);
    int writelen;
    char *buf;
    bool freeBuf = false;
    if (loopback) {
        // Write directly into the buffer at the other end of the loopback.
        if (len < minPacketSize)
            writelen = minPacketSize;
        else
            writelen = len;
        buf = loopback->reserve(writelen);
        if (len < minPacketSize)
	    memset(buf + len, 0, minPacketSize - len);
    } else if (len <= minPacketSize) {
	buf = outBuffer;
	memset(buf + len, 0, minPacketSize - len);
        writelen = minPacketSize;
    } else {
	buf = new char [len];
        writelen = len;
        freeBuf = true;
    }
    QCopPacketHeader *header = (QCopPacketHeader *)buf;
    header->command = cmd;
    header->totalLength = len;
    header->chLength = ch.length();
    header->msgLength = 0;
    header->forwardToLength = 0;
    header->dataLength = 0;
    char *ptr = buf + sizeof(QCopPacketHeader);
    memcpy(ptr, ch.constData(), ch.length() * 2);
    if (!loopback)
        write(buf, writelen);
    if (freeBuf)
	delete[] buf;
}

void QCopClient::send
    (const QString& ch, const QString& msg, const QByteArray& data, int cmd)
{
    int len = ch.length() * 2 + msg.length() * 2 + data.length();
    len += sizeof(QCopPacketHeader);
    int writelen;
    char *buf;
    bool freeBuf = false;
    if (loopback) {
        // Write directly into the buffer at the other end of the loopback.
        if (len < minPacketSize)
            writelen = minPacketSize;
        else
            writelen = len;
        buf = loopback->reserve(writelen);
        if (len < minPacketSize)
	    memset(buf + len, 0, minPacketSize - len);
    } else if (len <= minPacketSize) {
	buf = outBuffer;
	memset(buf + len, 0, minPacketSize - len);
        writelen = minPacketSize;
    } else {
	buf = new char [len];
        writelen = len;
        freeBuf = true;
    }
    QCopPacketHeader *header = (QCopPacketHeader *)buf;
    header->command = cmd;
    header->totalLength = len;
    header->chLength = ch.length();
    header->msgLength = msg.length();
    header->forwardToLength = 0;
    header->dataLength = data.length();
    char *ptr = buf + sizeof(QCopPacketHeader);
    memcpy(ptr, ch.constData(), ch.length() * 2);
    ptr += ch.length() * 2;
    memcpy(ptr, msg.constData(), msg.length() * 2);
    ptr += msg.length() * 2;
    memcpy(ptr, data.constData(), data.length());
    if (!loopback)
        write(buf, writelen);
    if (freeBuf)
	delete[] buf;
}

void QCopClient::forward(const char *packet, const QString& forwardTo)
{
    // Copy the original QCopCmd_Send packet, append the "forwardTo"
    // value to the end of it, and modify the header accordingly.
    int totalLength = ((QCopPacketHeader *)packet)->totalLength;
    int len = totalLength + forwardTo.length() * 2;
    int dataLength = ((QCopPacketHeader *)packet)->dataLength;
    if ((dataLength % 2) == 1)
        ++len;  // Pad the data so that forwardTo is aligned properly.
    int writelen;
    char *buf;
    bool freeBuf = false;
    if (loopback) {
        // Write directly into the buffer at the other end of the loopback.
        if (len < minPacketSize)
            writelen = minPacketSize;
        else
            writelen = len;
        buf = loopback->reserve(writelen);
        if (len < minPacketSize)
	    memset(buf + len, 0, minPacketSize - len);
    } else if (len <= minPacketSize) {
	buf = outBuffer;
	memset(buf + len, 0, minPacketSize - len);
        writelen = minPacketSize;
    } else {
	buf = new char [len];
        writelen = len;
        freeBuf = true;
    }
    memcpy(buf, packet, totalLength);
    QCopPacketHeader *header = (QCopPacketHeader *)buf;
    header->command = QCopCmd_Forward;
    header->totalLength = len;
    header->forwardToLength = forwardTo.length();
    char *ptr = buf + sizeof(QCopPacketHeader);
    ptr += header->chLength * 2;
    ptr += header->msgLength * 2;
    ptr += dataLength;
    if ((dataLength % 2) == 1)
        *ptr++ = 0;
    memcpy(ptr, forwardTo.constData(), forwardTo.length() * 2);
    if (!loopback)
        write(buf, writelen);
    if (freeBuf)
	delete[] buf;
}

void QCopClient::isRegisteredReply(const QString& ch, bool known)
{
    if (known)
        sendChannelCommand(QCopCmd_IsRegistered, ch);
    else
        sendChannelCommand(QCopCmd_IsNotRegistered, ch);
}

void QCopClient::requestRegistered(const QString& ch)
{
    sendChannelCommand(QCopCmd_RequestRegistered, ch);
}

void QCopClient::flush()
{
    if (socket)
        socket->flush();
}

bool QCopClient::waitForIsRegistered()
{
    if (isRegisteredWaiter)
        return false;       // Recursive re-entry!
    isRegisteredWaiter = new QEventLoop(this);
    isRegisteredWaiter->exec();
    delete isRegisteredWaiter;
    isRegisteredWaiter = 0;
    return isRegisteredResponse;
}

void QCopClient::readyRead()
{
    qint64 len;
    while (device->bytesAvailable() > 0) {
        if (inBufferUsed < inBufferExpected) {
            len = device->read
                (inBufferPtr + inBufferUsed, inBufferExpected - inBufferUsed);
            if (len <= 0)
                break;
            inBufferUsed += (int)len;
        }
        if (inBufferUsed >= minPacketSize) {
            // We have the full packet header and minimal payload.
            QCopPacketHeader *header = (QCopPacketHeader *)inBufferPtr;
            if (header->totalLength > inBufferExpected) {
                // Expand the buffer and continue reading.
                inBufferExpected = header->totalLength;
                inBufferPtr = new char [header->totalLength];
                memcpy(inBufferPtr, inBuffer, minPacketSize);
                continue;
            }
        }
        if (inBufferUsed >= inBufferExpected) {
            // We have a full packet to be processed.  Parse the command
            // and the channel name, but nothing else just yet.
            QCopPacketHeader *header = (QCopPacketHeader *)inBufferPtr;
            int command = header->command;
            QString channel;
            char *ptr = inBufferPtr + sizeof(QCopPacketHeader);
            if (header->chLength > 0) {
                channel = QString::fromUtf16
                    ((const ushort *)ptr, header->chLength);
                ptr += header->chLength * 2;
            }

            // Dispatch the command that we received.
            if (server) {
                // Processing command on server side.
                if (command == QCopCmd_Send) {
                    // Pass the whole packet, including padding, to answer()
                    // which can then write it directly to the destination
                    // sockets without needing to parse it further.
                    answer(channel, inBufferPtr, inBufferUsed);
                } else if (command == QCopCmd_RegisterChannel) {
                    handleRegisterChannel(channel);
                } else if (command == QCopCmd_DetachChannel) {
                    detach(channel);
                } else if (command == QCopCmd_RegisterMonitor) {
                    handleRegisterMonitor(channel);
                } else if (command == QCopCmd_DetachMonitor) {
                    handleDetachMonitor(channel);
                } else if (command == QCopCmd_RequestRegistered) {
                    handleRequestRegistered(channel);
                } else if (command == QCopCmd_Ack) {
                    handleAck(channel);
                }
            } else {
                // Parse the rest of the packet now that we know we need it.
                QString msg, forwardTo;
                QByteArray data;
                if (header->msgLength > 0) {
                    msg = QString::fromUtf16
                        ((const ushort *)ptr, header->msgLength);
                    ptr += header->msgLength * 2;
                }
                if (header->dataLength > 0) {
                    data = QByteArray ((const char *)ptr, header->dataLength);
                    ptr += header->dataLength;
                }
                if (header->forwardToLength > 0) {
                    if ((header->dataLength % 2) == 1)
                        ++ptr;
                    forwardTo = QString::fromUtf16
                        ((const ushort *)ptr, header->forwardToLength);
                }

                // Processing command on client side.
                if (command == QCopCmd_Send) {
                    QCopChannel::sendLocally(channel, msg, data);
                } else if (command == QCopCmd_Forward) {
                    forwardLocally(forwardTo, channel, msg, data);
                } else if (command == QCopCmd_SendRequestAck) {
                    // Send an Ack and try to make sure that it is in the
                    // kernel's socket buffer before processing the message.
                    // If message processing causes the client to crash,
                    // the server will at least know that this message was
                    // handled and won't try to restart the client and send
                    // it to the client again.
                    sendChannelCommand(QCopCmd_Ack, channel);
                    flush();
                    QCopChannel::sendLocally(channel, msg, data);
                } else if (command == QCopCmd_IsRegistered ||
                            command == QCopCmd_IsNotRegistered) {
                    if (isRegisteredWaiter) {
                        isRegisteredResponse =
                            (command == QCopCmd_IsRegistered);
                        isRegisteredWaiter->quit();
                    }
                } else if (command == QCopCmd_MonitorRegistered) {
                    handleRegistered(channel);
                } else if (command == QCopCmd_MonitorUnregistered) {
                    handleUnregistered(channel);
                } else if (command == QCopCmd_StartupComplete) {
                    handleStartupComplete(channel);
                }
            }

            // Discard the input buffer contents.
            if (inBufferPtr != inBuffer)
                delete[] inBufferPtr;
            inBufferPtr = inBuffer;
            inBufferUsed = 0;
            inBufferExpected = minPacketSize;
        }
    }
}

void QCopClient::disconnected()
{
    if (connecting)
        return;
    if (!finished) {
        finished = true;
        if (server) {
            detachAll();
            deleteLater();
        } else if (disconnectHandler) {
            (*disconnectHandler)();
        }
    }
}

void QCopClient::reconnect()
{
    // Attempt to reconnect after a pause
    reconnecting = true;
    QTimer::singleShot(1000, this, SLOT(connectToServer()));
}

#ifndef QT_NO_QCOP_LOCAL_SOCKET

QString QCopThreadData::socketPath()
{
    //return (Qtopia::inline tempDir() + "qcop-server").toUtf8();
    return "qcop-server-0";
}

#else

int QCopThreadData::listenPort()
{
    return 45328;
}

#endif

void QCopClient::connectToServer()
{
    if (!socket)  {
        // We are retrying the socket connection.
        socket = new QCopLocalSocket(this);
        device = socket;
    }

#ifndef QT_NO_QCOP_LOCAL_SOCKET
    socket->connectToServer(QCopThreadData::socketPath());
#else
    socket->connectToHost(QHostAddress::LocalHost, QCopThreadData::listenPort());
#endif
    if (socket->waitForConnected()) {
        if (reconnecting) {
            reconnecting = false;
            foreach (const QString &channel, qcopThreadData()->clientMap.keys()) {
                registerChannel(channel);
            }
        }
        connecting = false;
        retryCount = 0;
        device = socket;
        connectSignals();
        if (pendingData.size() > 0) {
            device->write(pendingData.constData(), pendingData.size());
            pendingData = QByteArray();
        }
    } else {
        connecting = false;
        delete socket;
        socket = 0;
        device = 0;

        if ((++retryCount % 30) == 0) {
            if (reconnecting) {
                qWarning() << "Cannot connect to QCop server; retrying...";
            } else {
                qWarning() << "Could not connect to QCop server; probably not running.";
                return;
            }
        }

        QTimer::singleShot(retryCount <= 30 ? 200 : 1000, this, SLOT(connectToServer()));
    }
}

void QCopClient::connectSignals()
{
    connect(device, SIGNAL(readyRead()), this, SLOT(readyRead()));
#ifndef QT_NO_QCOP_LOCAL_SOCKET
    if (socket)
        connect(socket, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)), this, SLOT(disconnected()));
#else
    if (socket) {
        connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(disconnected()));
    }
#endif
}

void QCopClient::write(const char *buf, int len)
{
    // If the socket is open, then send it immediately.
    if (device) {
        device->write(buf, len);
        return;
    }

    // Queue up the data for when the socket is open.
    pendingData += QByteArray(buf, len);
}

QCopLoopbackDevice::QCopLoopbackDevice(QObject *parent)
    : QIODevice(parent)
{
    otherEnd = 0;
}

QCopLoopbackDevice::QCopLoopbackDevice
        (QCopLoopbackDevice *_otherEnd, QObject *parent)
    : QIODevice(parent)
{
    otherEnd = _otherEnd;
    otherEnd->otherEnd = this;
}

bool QCopLoopbackDevice::open(OpenMode mode)
{
    // We don't want QIODevice to buffer data as we will be buffering
    // it ourselves in the other end of the connection.
    setOpenMode(mode | Unbuffered);
    return true;
}

void QCopLoopbackDevice::close()
{
    setOpenMode(NotOpen);
}

qint64 QCopLoopbackDevice::bytesAvailable() const
{
    return buffer.size();
}

bool QCopLoopbackDevice::isSequential() const
{
    return true;
}

// Reserve space in the other end's ring buffer for zero-copy semantics.
// Also arrange for readyRead() to be emitted at the next event loop.
char *QCopLoopbackDevice::reserve(int len)
{
    char *buf = otherEnd->buffer.reserve(len);
    if (otherEnd->buffer.size() == len)
        QCoreApplication::postEvent(otherEnd, new QEvent(QEvent::User));
    return buf;
}

qint64 QCopLoopbackDevice::readData(char *data, qint64 maxlen)
{
    return buffer.read(data, int(maxlen));
}

qint64 QCopLoopbackDevice::writeData(const char *data, qint64 len)
{
    if (otherEnd) {
        memcpy(otherEnd->buffer.reserve(int(len)), data, int(len));
        if (otherEnd->buffer.size() == len)
            QCoreApplication::postEvent(otherEnd, new QEvent(QEvent::User));
    }
    return len;
}

bool QCopLoopbackDevice::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
        emit readyRead();
        return true;
    } else {
        return QIODevice::event(e);
    }
}
