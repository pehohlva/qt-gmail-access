/*
clientparser.h: Declaration of ClientParser class

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

#ifndef CLIENTPARSER_H
#define CLIENTPARSER_H

#include <QXmlStreamReader>
#include <QStringList>
#include <QNetworkRequest>
#include <zlib.h>

#include "omapdconfig.h"
#include "maprequest.h"
#include "identifier.h"

static QStringList ID_NAMES = (QStringList()
                               << "access-request"
                               << "identity"
                               << "device"
                               << "ip-address"
                               << "mac-address");

class ClientParser : public QObject
{
    Q_OBJECT
public:
    ClientParser(QIODevice *parent = 0);
    ~ClientParser();
    const QString& clientXML() { return _clientRequestXml; }
    QString errorString() const { return _xml.errorString(); }
    QXmlStreamReader::Error error() const { return _xml.error(); }

    const QVariant& request() const { return _mapRequest; }
    const MapRequest::RequestError& requestError() const { return _requestError; }
    const  MapRequest::RequestVersion& requestVersion() const { return _requestVersion; }
    MapRequest::RequestType requestType() const { return _requestType; }
    const QString& sessionId() const { return _sessionId; }


signals:
    void parsingComplete();
    void headerReceived(QNetworkRequest requestHdrs);

public slots:
    void readData();

private:
    void parse();
    void parseDocument();
    void parseEnvelope();
    void parseHeader();
    void parseBody();
    void parseNewSession();
    void parseAttachSession();
    void parseEndSession();
    void parseRenewSession();
    void parsePurgePublisher();
    void parseSearch();
    SearchType parseSearchDetails(MapRequest &request);
    void parseSubscribe();
    void parsePoll();
    void parsePublish();
    void parsePublishUpdateOrNotify(PublishOperation::PublishType publishType, PublishRequest &pubReq);
    void parsePublishDelete(PublishRequest &pubReq);
    Id parseIdentifier(MapRequest &request);
    QList<Meta> parseMetadata(PublishRequest &pubReq, Meta::Lifetime lifetime);
    void registerMetadataNamespaces();
    void setSessionId(MapRequest &request);

    int readHeader();
    bool gzipCheckHeader(QByteArray &content, int &pos);
    int gunzipBodyPartially(QByteArray &compressed, QByteArray &inflated);
private:
    OmapdConfig* _omapdConfig;

    QNetworkRequest _requestHeaders;
    bool _haveAllHeaders;

    QXmlStreamReader _xmlSocketReader;
    QXmlStreamReader _xml;
    QXmlStreamWriter *_writer;
    QString _clientRequestXml;

    bool _setDeviceForCompression;
    bool _initInflate;
    bool _streamEnd;
    z_stream _inflateStrm;

    // mapping of prefix --> namespace for metadata types
    QMap<QString,QString> _namespaces;

    MapRequest::RequestError _requestError;
    MapRequest::RequestVersion _requestVersion;
    MapRequest::RequestType _requestType;

    QString _sessionId;
    bool _clientSetSessionId;

    QVariant _mapRequest;
};

#endif // CLIENTPARSER_H
