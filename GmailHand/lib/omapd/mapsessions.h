/*
mapsessions.h: Declaration of MapSessions Class

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

#ifndef MAPSESSIONS_H
#define MAPSESSIONS_H

#include <QObject>
#include <QtCore>
#include <QtNetwork>
#include <QtXmlPatterns>

#include "omapdconfig.h"
#include "subscription.h"
#include "server.h"
#include "clienthandler.h"

class MapClient;

class MapSessions : public QObject
{
    Q_OBJECT
public:
    enum BlacklistAction {
        AddToBlacklist = 1,
        RemoveFromBlacklist
    };

    static MapSessions* getInstance();

    QString registerMapClient(ClientHandler *socket, MapRequest::AuthenticationType authType, const QString& authToken);
    QString sessIdForClient(const QString& authToken);
    QString addActiveSSRCForClient(ClientHandler *clientHandler, const QString& authToken);

    bool haveActiveSSRCForClient(const QString& authToken);
    void removeActiveSSRCForClient(const QString& authToken);

    bool haveActivePollForClient(const QString& authToken);
    void setActivePollForClient(const QString& authToken, ClientHandler *pollClientHandler);
    void removeActivePollForClient(const QString& authToken);
    ClientHandler* pollConnectionForClient(const QString& authToken);
    ClientHandler* ssrcForClient(const QString& authToken);
    void migrateSSRCForClient(const QString& authToken, ClientHandler *newSSRCClientHandler);

    void setActiveARCForClient(const QString& authToken, ClientHandler *arcClientHandler);
    bool haveActiveARCForClient(const QString& authToken);
    void removeActiveARCForClient(const QString& authToken);
    ClientHandler* arcForClient(const QString& authToken);

    void removeClientConnections(ClientHandler *clientHandler);
    bool validateSessionId(const QString& sessId, const QString& authToken);

    QString pubIdForAuthToken(const QString& authToken);
    QString pubIdForSessId(const QString& sessId);
    OmapdConfig::AuthzOptions authzForAuthToken(const QString& authToken);
    bool metadataAuthorizationForAuthToken(const QString& authToken, const QString& metaName, const QString& metaNamespace);

    QList<Subscription*>& subscriptionListForClient(const QString& authToken);
    void insertSubscriptionForClient(Subscription* sub, const QString&_authToken);
    void removeSubscriptionForClient(const QString& name, const QString&_authToken);
    int removeSubscriptionListForClient(const QString& authToken);
    // void setSubscriptionListForClient(const QString& authToken, const QList<Subscription>& subList);
    QHash<QString, QList<Subscription*> > subscriptionLists(); // authToken --> subscriptionList for authToken

    bool validateMetadata(const Meta& aMeta);

    QString generateSessionId();

    bool loadClientConfiguration(const ClientConfiguration *client);
    bool removeClientConfiguration(ClientConfiguration *client);

    void loadBlacklistedClients(QString blacklistDirectory, QString caCertFile);
    bool manageClientBlacklist(ClientConfiguration *client, MapSessions::BlacklistAction action);
    bool addBlacklistClientConfiguration(ClientConfiguration *client);
    bool removeBlacklistClientConfiguration(ClientConfiguration *client);

    void removeMapClient(const QString& authToken);
    void removeMapClientCA(const QString& authToken);

    QSet<Subscription*> getSubscriptionsForIdentifier(const Id& id); // return list of all subscriptions that
                                                                     // include id in their search graph
    void addToIndex(const Id& id, Subscription* sub);
    void removeFromIndex(const Id& id, Subscription* sub);


private:
    MapSessions(QObject *parent = 0);
    ~MapSessions();
    void loadClientConfigurations();

    static MapSessions* _instance;

    OmapdConfig *_omapdConfig;

    QHash<QString, MapClient*> _mapClients; // authToken --> MapClient
    QHash<QString, MapClient*> _mapClientCAs; // CA AuthToken --> MapClient
    QHash<Id, QSet<Subscription*> > _subs; // Identifier --> list of referenced subscriptions
    QSet<QString> _blacklistedClients; // blacklisted client authToken
    // Registry for published vendor specific metadata cardinalities
    QHash<VSM, Meta::Cardinality> _vsmRegistry;

    // Standard IF-MAP schemas
    QXmlSchema _ifmapBase11;
    QXmlSchema _ifmapMeta11;
    QXmlSchema _ifmapBase20;
    QXmlSchema _ifmapMeta20;
    QXmlSchemaValidator _ifmapMeta11Validator;
    QXmlSchemaValidator _ifmapMeta20Validator;

    unsigned int _pubIdIndex;

    QHash<QString, ClientHandler*> _ssrcConnections; // authToken --> ClientHandler
    QHash<QString, ClientHandler*> _arcConnections; // authToken --> ClientHandler
    QHash<QString, ClientHandler*> _activePollConnections; // authToken --> ClientHandler
};

#endif // MAPSESSIONS_H
