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

#ifndef QCOPCHANNEL_P_H
#define QCOPCHANNEL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#ifndef QT_NO_QCOP_LOCAL_SOCKET
#include <QtNetwork/qlocalsocket.h>
#include <QtNetwork/qlocalserver.h>
typedef QLocalSocket QCopLocalSocket;
typedef QLocalServer QCopLocalServer;
#else
#include <QtNetwork/qtcpsocket.h>
#include <QtNetwork/qtcpserver.h>
typedef QTcpSocket QCopLocalSocket;
typedef QTcpServer QCopLocalServer;
#endif
#include <QtCore/qshareddata.h>
#include <QtCore/qregexp.h>
#include <qringbuffer_p.h>

class QEventLoop;
class QCopChannel;
class QCopChannelMonitor;
class QCopServer;
class QCopLoopbackDevice;

#define	QCopCmd_RegisterChannel	    1
#define	QCopCmd_Send		    2
#define	QCopCmd_IsRegistered        3
#define	QCopCmd_IsNotRegistered     4
#define	QCopCmd_RequestRegistered   5
#define	QCopCmd_DetachChannel       6
#define	QCopCmd_Forward             7
#define QCopCmd_RegisterMonitor     8
#define QCopCmd_DetachMonitor       9
#define QCopCmd_MonitorRegistered   10
#define QCopCmd_MonitorUnregistered 11
#define QCopCmd_SendRequestAck      12
#define QCopCmd_Ack                 13
#define QCopCmd_StartupComplete     14

class QCopClient : public QObject
{
    Q_OBJECT

    struct MemberInvokerBase
    {
        virtual ~MemberInvokerBase() {}

        virtual void operator()() = 0;
    };

    template<typename T>
    struct MemberInvoker : public MemberInvokerBase
    {
        T* instance;
        void (T::*function)();

        MemberInvoker(T* inst, void (T::*func)())
            : instance(inst), function(func) {}

        virtual void operator()() { (instance->*function)(); }
    };

public:
    template<typename T>
    QCopClient(bool connectImmediately, T *instance = 0, void (T::*func)() = 0)
        : QObject(),
          server(false),
          socket(new QCopLocalSocket(this)),
          device(socket),
          disconnectHandler(instance ? new MemberInvoker<T>(instance, func) : 0)
    {
        init();

        if (connectImmediately) {
            connectToServer();
        }
    }

    QCopClient(QIODevice *device, QCopLocalSocket *socket);
    QCopClient(QIODevice *device, bool isServer);
    ~QCopClient();

    void registerChannel(const QString& ch);
    void detachChannel(const QString& ch);
    void sendChannelCommand(int cmd, const QString& ch);
    void send(const QString& ch, const QString& msg, const QByteArray& data,
              int cmd = QCopCmd_Send);
    void forward(const char *packet, const QString& forwardTo);
    void isRegisteredReply(const QString& ch, bool known);
    void requestRegistered(const QString& ch);
    void flush();
    bool waitForIsRegistered();

    bool isClient() const { return !server; }
    bool isServer() const { return server; }

    void write(const char *buf, int len);

    static const int minPacketSize = 256;

    bool isStartupComplete;

    void reconnect();

signals:
    void startupComplete();

private slots:
    void readyRead();
    void disconnected();
    void connectToServer();
    void connectSignals();

private:
    bool server;
    bool finished;
    QCopLoopbackDevice *loopback;
    QCopLocalSocket *socket;
    QIODevice *device;
    MemberInvokerBase *disconnectHandler;

    void init();

    char outBuffer[minPacketSize];
    char inBuffer[minPacketSize];
    char *inBufferPtr;
    int inBufferUsed;
    int inBufferExpected;
    bool isRegisteredResponse;
    QEventLoop *isRegisteredWaiter;
    QByteArray pendingData;
    int retryCount;
    bool connecting;
    bool reconnecting;
    int channelCount;

    void detachAll();
    void detach(const QString& ch);
    static void answer(const QString& ch, const char *packet, int packetLen);
    void handleRegisterChannel(const QString& ch);
    void handleRequestRegistered(const QString& ch);
    static void forwardLocally(const QString& forwardTo, const QString& ch, const QString& msg, const QByteArray& data);
    void handleRegisterMonitor(const QString& ch);
    void handleDetachMonitor(const QString& ch);
    static void handleRegistered(const QString& ch);
    static void handleUnregistered(const QString& ch);
    static void handleAck(const QString& ch);
    void handleStartupComplete(const QString& ch);
};

// Simple QIODevice that loops data back into the same process
// between two instances of this class.  This is used for the client
// connection socket when the client is in the same process as the server.
class QCopLoopbackDevice : public QIODevice
{
    Q_OBJECT
public:
    QCopLoopbackDevice(QObject *parent = 0);
    explicit QCopLoopbackDevice(QCopLoopbackDevice *otherEnd, QObject *parent = 0);

    bool open(OpenMode mode);
    void close();
    qint64 bytesAvailable() const;
    bool isSequential() const;

    char *reserve(int len);

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    bool event(QEvent *e);

private:
    QCopLoopbackDevice *otherEnd;
    QRingBuffer buffer;
};

class QCopServerAppInfo;

class QCopServerPrivate : public QCopLocalServer
{
    Q_OBJECT
public:
    QCopServerPrivate();
    ~QCopServerPrivate();

    void init();

protected:
#ifndef QT_NO_QCOP_LOCAL_SOCKET
    void incomingConnection(quintptr socketDescriptor);
#else
    void incomingConnection(int socketDescriptor);
#endif

public:
    QMap<QString, QCopServerAppInfo *> applications;
    QMap<QString, QCopServerAppInfo *> pidChannels;

private slots:
    void forwarded(const QString& msg, const QByteArray &data, const QString& channel);
    void registered();
    void unregistered();

private:
    void applicationRegistered(QCopServerAppInfo *info);
    void applicationUnregistered(QCopServerAppInfo *info);
};

class QCopServerRegexp
{
public:
    QCopServerRegexp(const QString& ch, QCopClient *cl);

    QString channel;
    QCopClient *client;
    QRegExp regexp;
    int prefixMatch;
    QCopServerRegexp *next;

    bool match(const QString& ch) const;
};

// Simple linked list of QCopServerRegexp objects.
class QCopServerRegexpList
{
public:
    inline QCopServerRegexpList() : first(0), last(0) {}
    ~QCopServerRegexpList();

    inline bool isEmpty() const { return first == 0; }

    inline void append(QCopServerRegexp *node)
    {
        if (last)
            last->next = node;
        else
            first = node;
        node->next = 0;
        last = node;
    }

    struct Iterator
    {
        QCopServerRegexp *current;
        QCopServerRegexp *prev;

        inline bool atEnd() const { return current == 0; }

        inline void advance()
        {
            prev = current;
            current = current->next;
        }
    };

    inline void begin(Iterator& it) const
    {
        it.current = first;
        it.prev = 0;
    }

    inline void erase(Iterator& it)
    {
        QCopServerRegexp *current = it.current;
        QCopServerRegexp *next = current->next;
        if (it.prev)
            it.prev->next = next;
        else
            first = next;
        if (!next)
            last = it.prev;
        it.current = next;
        delete current;
    }

private:
    QCopServerRegexp *first;
    QCopServerRegexp *last;
};

class QCopChannelPrivate : public QSharedData
{
public:
    QCopChannelPrivate(QCopChannel *obj, const QString& chan)
        : object(obj), channel(chan), useForwardedSignal(false) {}

    QCopChannel *object;
    QString channel;
    bool useForwardedSignal;
};

class QCopChannelMonitorPrivate : public QSharedData
{
public:
    QCopChannelMonitorPrivate(QCopChannelMonitor *obj, const QString& ch)
        : object(obj), channel(ch), state(0) {}

    QCopChannelMonitor *object;
    QString channel;
    int state;
};

typedef QExplicitlySharedDataPointer<QCopChannelPrivate> QCopChannelPrivatePointer;
typedef QExplicitlySharedDataPointer<QCopChannelMonitorPrivate> QCopChannelMonitorPrivatePointer;
typedef QMap<QString, QList<QCopChannelPrivatePointer> > QCopClientMap;
typedef QMap<QString, QList<QCopChannelMonitorPrivatePointer> > QCopClientMonitorMap;
typedef QMap<QString, QList<QCopClient*> > QCopServerMap;

// Thread-specific data for QCop client and server implementations.
class QCopThreadData
{
public:
    QCopThreadData()
    {
        conn = 0;
        server = 0;
    }

    ~QCopThreadData()
    {
        delete conn;
    }

    static QCopThreadData *instance();
#ifndef QT_NO_QCOP_LOCAL_SOCKET
    static QString socketPath();
#else
    static int listenPort();
#endif

    // Get the client connection object for this thread.
    inline QCopClient *clientConnection()
    {
        if (!conn) {
            conn = new QCopClient(true, this, &QCopThreadData::disconnected);
        }
        return conn;
    }

    // Determine if we have a client connection object for this thread.
    inline bool hasClientConnection() const
    {
        return (conn != 0);
    }

    // Map client-side channel names to lists of QCopChannel objects.
    QCopClientMap clientMap;

    // Map client-side channel names to the associated monitor objects.
    QCopClientMonitorMap clientMonitorMap;

    // Map server-side channel names to the clients that registered them.
    QCopServerMap serverMap;

    // Map server-side channel names to the clients that are monitoring them.
    QCopServerMap serverMonitorMap;

    // List of regular expression channel mappings in the server.
    QCopServerRegexpList serverRegexpList;

    // Pointer to the QCopServer instance if this thread is the server.
    QCopServer *server;

    QCopClient *conn;

private:
    void disconnected()
    {
        if (conn) {
            conn->deleteLater();

            conn = new QCopClient(false, this, &QCopThreadData::disconnected);
            conn->reconnect();
        }
    }
};

#endif
