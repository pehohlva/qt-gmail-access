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

#include "qcopserver.h"
#include "qcopchannel.h"
#include "qcopchannel_p.h"
#include "qcopchannelmonitor.h"
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>

QCopServerPrivate::QCopServerPrivate()
    : QCopLocalServer()
{
    bool ok;
#ifndef QT_NO_QCOP_LOCAL_SOCKET
    QString path = QCopThreadData::socketPath();
    ok = listen(path);
#ifdef Q_OS_UNIX
    if (!ok) {
        // There may be another qcop server running, or the path
        // was simply left in the filesystem after a server crash.
        // QLocalServer does not clean up such paths.  We try to
        // connect to the existing qcop server, and if that fails
        // we remove the path and try again.
        QLocalSocket *socket = new QLocalSocket();
        socket->connectToServer(path);
        if (!socket->waitForConnected()) {
            delete socket;
            QFile::remove(QDir::tempPath() + QChar('/') + path);
            ok = listen(path);
        } else {
            delete socket;
        }
    }
#endif
#else
    ok = listen(QHostAddress::LocalHost, QCopThreadData::listenPort());
    QString path = QString::number(QCopThreadData::listenPort());
#endif
    if (!ok)
        qWarning() << "Could not listen for qcop connections on"
                   << path << "; another qcop server may already be running.";
}

QCopServerPrivate::~QCopServerPrivate()
{
    qDeleteAll(applications);
}

#ifndef QT_NO_QCOP_LOCAL_SOCKET
void QCopServerPrivate::incomingConnection(quintptr socketDescriptor)
#else
void QCopServerPrivate::incomingConnection(int socketDescriptor)
#endif
{
    QCopLocalSocket * sock = new QCopLocalSocket;
    sock->setSocketDescriptor(socketDescriptor);

    QCopClient *client;
    client = new QCopClient(sock, sock);
    sock->setParent(client);
}

/* ! - documentation comments in this file are disabled:
    \class QCopServer
    \inpublicgroup QtBaseModule
    \ingroup qws
    \brief The QCopServer class provides the server-side implementation of QCopChannel.

    QCopServer is used internally by Qt Extended to implement the server-side
    counterpart to QCopChannel.

    The first QCopServer instance that is created will initialize the
    server socket, start listening for connections, and set instance().

    The QCop server will be shut down when the first QCopServer instance
    that was created by the application is destroyed.

    Only one process should create an instance of QCopServer; the process
    that has been selected to act as the QCop server.  All other processes
    should use QCopChannel to connect in client mode.

    \sa QCopChannel
*/

/* !
    Construct the QCop server and attach it to \a parent.
*/
QCopServer::QCopServer(QObject *parent)
    : QObject(parent)
{
    QCopThreadData *td = QCopThreadData::instance();
    if (!td->server) {
        d = new QCopServerPrivate();
        td->server = this;

        // Create the in-memory loopback client connection.
        if (!td->conn) {
            QCopLoopbackDevice *end1 = new QCopLoopbackDevice();
            end1->open(QIODevice::ReadWrite);
            QCopLoopbackDevice *end2 = new QCopLoopbackDevice(end1);
            end2->open(QIODevice::ReadWrite);
            QCopClient *client1 = new QCopClient(end1, true);
            QCopClient *client2 = new QCopClient(end2, false);
            end1->setParent(client1);
            end2->setParent(client2);
            client1->setParent(this);
            client2->setParent(this);
            td->conn = client2;
        }

        // Now perform the rest of the server initialization.
        d->init();
    } else {
        qWarning() << "Multiple QCopServer instances should not be created";
        d = 0;
    }
}

/* !
    Destruct the QCop server.
*/
QCopServer::~QCopServer()
{
    if (d) {
        QCopThreadData *td = QCopThreadData::instance();
        delete d;
        td->server = 0;
        td->conn = 0;
    }
}

class QCopServerSavedMessage
{
public:
    QString message;
    QByteArray data;
};

class QCopServerAppInfo
{
public:
    bool pidChannelAvailable;
    qint64 pid;
    QString pidChannel;
    QList<QCopServerSavedMessage> queue;
    QCopChannelMonitor *monitor;

    ~QCopServerAppInfo()
    {
        delete monitor;
    }
};

/* !
    Requests that an application called \a name should be activated
    because a QCop message arrived on \c{QPE/Application/<name>} and
    that application is not currently running.

    Returns the process identifier of the application if it has
    been started, or -1 if the application could not be started.
    The default implementation returns -1.

    Messages will be queued up and forwarded to the application-specific
    channel \c{QPE/Pid/<pid>} once that channel becomes available
    in the system.  If the application could not be started, then any
    queued messages will be discarded.

    \sa applicationExited()
*/
qint64 QCopServer::activateApplication(const QString& name)
{
    Q_UNUSED(name);
    return -1;
}

/* !
    Notifies the QCop server that an application with process ID \a pid
    that was previously started in response to a call to
    activateApplication() has exited or crashed.

    The next time a QCop message arrives on \c{QPE/Application/<name>},
    activateApplication() will be called to start the application again.

    \sa activateApplication()
*/
void QCopServer::applicationExited(qint64 pid)
{
    Q_UNUSED(pid);
    // TODO
}

    void startupComplete();
void QCopClient::handleAck(const QString& ch)
{
    QCopThreadData *td = QCopThreadData::instance();
    QMap<QString, QCopServerAppInfo *>::ConstIterator it;
    it = td->server->d->pidChannels.find(ch);
    if (it != td->server->d->pidChannels.constEnd()) {
        QCopServerAppInfo *info = it.value();
        if (!info->queue.isEmpty())
            info->queue.removeFirst();
    }
}

void QCopServerPrivate::init()
{
    QCopChannel *appChannel =
        new QCopChannel(QLatin1String("QPE/Application/*"), this);
    connect(appChannel, SIGNAL(forwarded(QString,QByteArray,QString)),
            this, SLOT(forwarded(QString,QByteArray,QString)));
}

// Handle messages that were forwarded on QPE/Application/* channels.
void QCopServerPrivate::forwarded
        (const QString& msg, const QByteArray &data, const QString& channel)
{
    QCopThreadData *td = QCopThreadData::instance();
    QCopServerAppInfo *info;

    // Do we already know about this application?
    QString appName = channel.mid(16);
    QMap<QString, QCopServerAppInfo *>::ConstIterator it;
    it = applications.find(appName);
    if (it != applications.constEnd()) {
        info = it.value();
    } else {
        // We haven't seen this application before, so try to start it.
        qint64 pid = td->server->activateApplication(appName);
        if (pid == -1)
            return;
        info = new QCopServerAppInfo();
        info->pidChannelAvailable = false;
        info->pid = pid;
        info->pidChannel = QLatin1String("QPE/Pid/") + QString::number(pid);
        info->monitor = new QCopChannelMonitor(info->pidChannel);
        connect(info->monitor, SIGNAL(registered()), this, SLOT(registered()));
        connect(info->monitor, SIGNAL(unregistered()), this, SLOT(unregistered()));
        applications.insert(appName, info);
        pidChannels.insert(info->pidChannel, info);
    }

    // Add the message to the application's saved message queue.
    QCopServerSavedMessage saved;
    saved.message = msg;
    saved.data = data;
    info->queue.append(saved);

    // If the application is already running, then pass it on.
    if (info->pidChannelAvailable) {
        // XXX - not right, should use answer()
        td->clientConnection()->send
            (info->pidChannel, msg, data, QCopCmd_SendRequestAck);
    }
}

void QCopServerPrivate::registered()
{
    QCopChannelMonitor *monitor = qobject_cast<QCopChannelMonitor *>(sender());
    if (monitor) {
        QMap<QString, QCopServerAppInfo *>::ConstIterator it;
        it = pidChannels.find(monitor->channel());
        if (it != pidChannels.constEnd()) {
            QCopServerAppInfo *info = it.value();
            if (!info->pidChannelAvailable)
                applicationRegistered(info);
        }
    }
}

void QCopServerPrivate::unregistered()
{
    QCopChannelMonitor *monitor = qobject_cast<QCopChannelMonitor *>(sender());
    if (monitor) {
        QMap<QString, QCopServerAppInfo *>::ConstIterator it;
        it = pidChannels.find(monitor->channel());
        if (it != pidChannels.constEnd()) {
            QCopServerAppInfo *info = it.value();
            if (info->pidChannelAvailable)
                applicationUnregistered(info);
        }
    }
}

void QCopServerPrivate::applicationRegistered(QCopServerAppInfo *info)
{
    Q_UNUSED(info);
    // TODO
}

void QCopServerPrivate::applicationUnregistered(QCopServerAppInfo *info)
{
    Q_UNUSED(info);
    // TODO
}
