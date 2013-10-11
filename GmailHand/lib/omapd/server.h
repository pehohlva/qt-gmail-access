/*
server.h: Declaration of Server class

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

#ifndef SERVER_H
#define SERVER_H

#include <QtCore>
#include <QTcpServer>
#include "mapgraphinterface.h"

#if defined(Q_CC_MSVC)
    #define __PRETTY_FUNCTION__  __FUNCSIG__
#endif

class ClientHandler;
class MapResponse;

class Server : public QTcpServer
{
    Q_OBJECT
public:
    Server(MapGraphInterface *mapGraph, QObject *parent = 0);
    bool startListening();

signals:

public slots:
    void sendPollResponseToClient(ClientHandler *client, const QByteArray& response, MapRequest::RequestVersion reqVersion);
    void addClientToTimeout(const QString& authToken);
    void removeClientFromTimeout();
    void removeClientConnectionFromTimeout(ClientHandler *client);

private slots:
    void discardConnection();
    void deleteClientMetadata();

private:
    void incomingConnection(int socketDescriptor);
    void removeTimerForClient(const QString& authToken);

private:
    OmapdConfig* _omapdConfig;
    unsigned int _metadataTimeout;
    MapGraphInterface* _mapGraph;
    QHash<QString, QTimer*> _clientTimeouts;
    QHash<ClientHandler*, QString> _clientAuthTokens;
};

#endif // SERVER_H
