/*
clienthandler.h: Declaration of ClientHandler class

Copyright (C) 2011  Sarab D. Mattes <mattes@nixnux.org>

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

#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QSslSocket>
#include <QNetworkRequest>

#include "mapgraphinterface.h"
#include "mapresponse.h"

class ClientParser;
class MapSessions;

class ClientHandler : public QSslSocket
{
    Q_OBJECT
public:
    enum CertInfoTarget {
        Subject = 1,
        Issuer
    };

    explicit ClientHandler(MapGraphInterface *mapGraph, QObject *parent = 0);
    explicit ClientHandler(MapGraphInterface *mapGraph, const QString& authToken, QObject *parent = 0);
    ~ClientHandler();

    static QString buildDN(const QSslCertificate& cert, const ClientHandler::CertInfoTarget& target);

    void sessionMetadataTimeout();
    void sendPollResponse(const QByteArray& response, const MapRequest::RequestVersion& reqVersion);
    const QString& authToken() { return _authToken; }
    bool useCompression() { return _useCompression; }

signals:
    void needToSendPollResponse(ClientHandler *client, const QByteArray& response, MapRequest::RequestVersion reqVersion);
    void receivedNewSession(const QString& authToken);
    void receivedRenewSession(const QString& authToken);
    void receivedEndSession();
    void receivedMigratedSession(ClientHandler *client);

public slots:
    void handleParseComplete();

private slots:
    void clientSocketError(const QAbstractSocket::SocketError& socketError);
    void processReadyRead();
    void socketReady();
    void clientSSLErrors(const QList<QSslError> & errors);
    void clientConnState(QAbstractSocket::SocketState sState);
    void processHeader(const QNetworkRequest& requestHdrs);

private:
    void registerCert();
    void compressResponse(const QByteArray& uncompressed, QByteArray& deflated);
    void sendHttpResponse(int hdrNumber, const QString& hdrText);
    void sendMapResponse(const MapResponse &mapResponse);
    void sendResponse(const QByteArray& response, const MapRequest::RequestVersion& reqVersion);
    void processClientRequest();
    void sendResultsOnActivePolls();

    QString filteredMetadata(const QList<Meta>& metaList, const MetadataFilter& filter, const QMap<QString, QString>& searchNamespaces, MapRequest::RequestError &error);
    QString filteredMetadata(const Meta& meta, const MetadataFilter& filter, const QMap<QString, QString>& searchNamespaces, MapRequest::RequestError &error);

    void collectSearchGraphMetadata(Subscription &sub, SearchResult::ResultType resultType, MapRequest::RequestError &operationError);
    void addUpdateAndDeleteMetadata(Subscription &sub, SearchResult::ResultType resultType, const QSet<Id>& idList, const QSet<Link>& linkList, MapRequest::RequestError &operationError);
    void buildSearchGraph(Subscription &sub, const Id& startId, int currentDepth);
    void addIdentifierResult(Subscription &sub, const Identifier& id, const QList<Meta>& metaList, SearchResult::ResultType resultType, MapRequest::RequestError &operationError);
    void addLinkResult(Subscription &sub, const Link& link, const QList<Meta>& metaList, SearchResult::ResultType resultType, MapRequest::RequestError &operationError);

    void updateSubscriptionsWithNotify(const Link& link, bool isLink, const QList<Meta>& metaChanges);
    void updateSubscriptions(const QHash<Id, QList<Meta> >& idMetaDeleted, const QHash<Link, QList<Meta> >& linkMetaDeleted);
    void updateSubscriptions(const Link link, bool isLink, const QList<Meta>& metaChanges, Meta::PublishOperationType publishType);

    void processNewSession(const QVariant& clientRequest);
    void processRenewSession(const QVariant& clientRequest);
    void processEndSession(const QVariant& clientRequest);
    void processAttachSession(const QVariant& clientRequest);
    void processPublish(const QVariant& clientRequest);
    void processSubscribe(const QVariant& clientRequest);
    void processSearch(const QVariant& clientRequest);
    void processPurgePublisher(const QVariant& clientRequest);
    void processPoll(const QVariant& clientRequest);

    void checkPublishAtomicity(PublishRequest &pubReq, MapRequest::RequestError &requestError);
    QPair< QList<Meta>, QList<Meta> > applyDeleteFilterToMeta(const QList<Meta>& existingMetaList, const PublishOperation& pubOper, MapRequest::RequestError &requestError, bool *metadataDeleted = 0);

    bool terminateSession(const QString& sessionId, MapRequest::RequestVersion requestVersion);
    bool terminateARCSession(const QString& sessionId, MapRequest::RequestVersion requestVersion);
    bool purgePublisher(const QString& publisherId, bool sessionMetadataOnly);

private:
    OmapdConfig* _omapdConfig;
    MapGraphInterface* _mapGraph;
    MapSessions* _mapSessions;

    ClientParser* _parser;

    MapRequest::AuthenticationType _authType;
    QString _authToken;

    bool _useCompression;
};

#endif // CLIENTHANDLER_H
