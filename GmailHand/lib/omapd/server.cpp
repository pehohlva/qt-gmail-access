/*
server.cpp: Implementation of Server class

Copyright (C) 2010  Sarab D. Mattes <mattes@nixnux.org>

This file is part of omapd.

omapd is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

omapd is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with omapd.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "server.h"
#include "clienthandler.h"
#include "mapresponse.h"
#include "mapsessions.h"

Server::Server(MapGraphInterface *mapGraph, QObject *parent)
        : QTcpServer(parent), _mapGraph(mapGraph)
{
    _omapdConfig = OmapdConfig::getInstance();
    _metadataTimeout = _omapdConfig->valueFor("session_metadata_timeout").toUInt() * 1000;
}

bool Server::startListening()
{
    bool rc = false;
    QHostAddress listenOn;
    if (listenOn.setAddress(_omapdConfig->valueFor("address").toString())) {
        unsigned int port = _omapdConfig->valueFor("port").toUInt();

        if (listen(listenOn, port)) {
            rc = true;
            this->setMaxPendingConnections(30); // 30 is QTcpServer default
        } else {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Error with listen on:" << listenOn.toString()
                    << ":" << port;
        }
    } else {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "Error setting server address";
    }

    // Load up MapSessions, which will create configured clients
    MapSessions::getInstance();

    return rc;
}

void Server::incomingConnection(int socketDescriptor)
{
    ClientHandler *client = new ClientHandler(_mapGraph, this);
    client->setSocketDescriptor(socketDescriptor);

    if (_omapdConfig->valueFor("send_tcp_keepalives").toBool()) {
        client->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    }

    client->startServerEncryption();

    QObject::connect(client, SIGNAL(disconnected()),
                     this, SLOT(discardConnection()));

    connect(client,
            SIGNAL(receivedNewSession(QString)),
            this,
            SLOT(addClientToTimeout(QString)));
    connect(client,
            SIGNAL(receivedRenewSession(QString)),
            this,
            SLOT(addClientToTimeout(QString)));
    connect(client,
            SIGNAL(receivedEndSession()),
            this,
            SLOT(removeClientFromTimeout()));
    connect(client,
            SIGNAL(receivedMigratedSession(ClientHandler*)),
            this,
            SLOT(removeClientConnectionFromTimeout(ClientHandler*)));

    connect(client,
            SIGNAL(needToSendPollResponse(ClientHandler*,QByteArray,MapRequest::RequestVersion)),
            this,
            SLOT(sendPollResponseToClient(ClientHandler*,QByteArray,MapRequest::RequestVersion)));

}

void Server::discardConnection()
{
    ClientHandler *client = (ClientHandler*)sender();
    if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowClientOps))
        qDebug() << __PRETTY_FUNCTION__ << ":" << "client:" << client;

    QString authToken=_clientAuthTokens.take(client);
    if (!authToken.isEmpty()) {
        removeTimerForClient(authToken);
        if (_metadataTimeout > 0) {
            QTimer *timer = new QTimer(this);
            connect(timer, SIGNAL(timeout()), this, SLOT(deleteClientMetadata()));
            timer->start(_metadataTimeout);

            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowClientOps))
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Created session metadata timeout for authToken:" << authToken;

            _clientTimeouts.insert(authToken, timer);
        }
    }

    MapSessions::getInstance()->removeClientConnections(client);
    client->deleteLater();
}

void Server::addClientToTimeout(const QString& authToken)
{
    ClientHandler *client = (ClientHandler*)sender();
    _clientAuthTokens.insert(client, authToken);
    removeTimerForClient(authToken);
}

void Server::removeClientFromTimeout()
{
    ClientHandler *client = (ClientHandler*)sender();
    _clientAuthTokens.remove(client);
}

void Server::removeClientConnectionFromTimeout(ClientHandler *client)
{
    _clientAuthTokens.remove(client);
}

void Server::removeTimerForClient(const QString& authToken)
{
    if (_clientTimeouts.contains(authToken)) {
        QTimer *timer = _clientTimeouts.take(authToken);
        timer->stop();
        delete timer;
    }
}

void Server::deleteClientMetadata()
{
    QTimer *timer = (QTimer*)sender();
    timer->stop();

    QString authToken = _clientTimeouts.key(timer);

    if (!authToken.isEmpty()) {
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowClientOps))
             qDebug() << __PRETTY_FUNCTION__ << ":" << "Time to remove session metadata for client with authToken:" << authToken;
        ClientHandler client(_mapGraph, authToken, this);
        connect(&client,
                SIGNAL(needToSendPollResponse(ClientHandler*, const QByteArray&,MapRequest::RequestVersion)),
                this,
                SLOT(sendPollResponseToClient(ClientHandler*,const QByteArray&,MapRequest::RequestVersion)));
        client.sessionMetadataTimeout();

        removeTimerForClient(authToken);
    } else {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "Error: Missing authToken for metadata expiration timer";
    }

}

void Server::sendPollResponseToClient(ClientHandler *client, const QByteArray& response, MapRequest::RequestVersion reqVersion)
{
    if (client) {
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowClientOps))
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Client:" << client;

        client->sendPollResponse(response, reqVersion);
    } else {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "ERROR: Client is null!";
    }

}
