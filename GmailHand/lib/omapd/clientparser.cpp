/*
clientparser.cpp: Implementation of ClientParser class

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

#include <QDebug>
#include <QNetworkRequest>
#include <QHostAddress>

#include "clientparser.h"
#include "mapsessions.h"

// From qtsdk-2010.05/qt/src/network/access/qhttpnetworkreply_p.h
static const unsigned char gz_magic[2] = {0x1f, 0x8b}; // gzip magic header
// gzip flag byte
#define HEAD_CRC     0x02 // bit 1 set: header CRC present
#define EXTRA_FIELD  0x04 // bit 2 set: extra field present
#define ORIG_NAME    0x08 // bit 3 set: original file name present
#define COMMENT      0x10 // bit 4 set: file comment present
#define RESERVED     0xE0 // bits 5..7: reserved
#define CHUNK 16384

ClientParser::ClientParser(QIODevice *parent) :
        QObject(parent)
{
    _omapdConfig = OmapdConfig::getInstance();

    _xmlSocketReader.setNamespaceProcessing(true);
    _xmlSocketReader.setDevice(parent);

    _writer = new QXmlStreamWriter(&_clientRequestXml);
    _writer->setAutoFormatting(true);

    _xml.setNamespaceProcessing(true);

    _requestError = MapRequest::ErrorNone;
    _requestVersion = MapRequest::VersionNone;
    _requestType = MapRequest::RequestNone;

    _clientSetSessionId = false;
    _sessionId = "";
    _haveAllHeaders = false;

    _setDeviceForCompression = false;
    _initInflate = false;
    _streamEnd = false;

    // Some clients (e.g. libifmap) don't send all the required namespaces
    // for Filters
    _namespaces.insert("hirsch", "http://www.trustedcomputinggroup.org/2006/IFMAP-HIRSCH/1");
    _namespaces.insert("trpz", "http://www.trustedcomputinggroup.org/2006/IFMAP-TRAPEZE/1");
    _namespaces.insert("scada", "http://www.trustedcomputinggroup.org/2006/IFMAP-SCADANET-METADATA/1");
    _namespaces.insert("meta", IFMAP_META_NS_1);
}

ClientParser::~ClientParser()
{
    delete _writer;
    _mapRequest.clear();
}

// From qtsdk-2010.05/qt/src/network/access/qhttpnetworkreply.cpp
bool ClientParser::gzipCheckHeader(QByteArray &content, int &pos)
{
    int method = 0; // method byte
    int flags = 0;  // flags byte
    bool ret = false;

    // Assure two bytes in the buffer so we can peek ahead -- handle case
    // where first byte of header is at the end of the buffer after the last
    // gzip segment
    pos = -1;
    QByteArray &body = content;
    int maxPos = body.size()-1;
    if (maxPos < 1) {
        return ret;
    }

    // Peek ahead to check the gzip magic header
    if (body[0] != char(gz_magic[0]) ||
        body[1] != char(gz_magic[1])) {
        return ret;
    }
    pos += 2;
    // Check the rest of the gzip header
    if (++pos <= maxPos)
        method = body[pos];
    if (pos++ <= maxPos)
        flags = body[pos];
    if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
        return ret;
    }

    // Discard time, xflags and OS code:
    pos += 6;
    if (pos > maxPos)
        return ret;
    if ((flags & EXTRA_FIELD) && ((pos+2) <= maxPos)) { // skip the extra field
        unsigned len =  (unsigned)body[++pos];
        len += ((unsigned)body[++pos])<<8;
        pos += len;
        if (pos > maxPos)
            return ret;
    }
    if ((flags & ORIG_NAME) != 0) { // skip the original file name
        while(++pos <= maxPos && body[pos]) {}
    }
    if ((flags & COMMENT) != 0) {   // skip the .gz file comment
        while(++pos <= maxPos && body[pos]) {}
    }
    if ((flags & HEAD_CRC) != 0) {  // skip the header crc
        pos += 2;
        if (pos > maxPos)
            return ret;
    }
    ret = (pos < maxPos); // return failed, if no more bytes left
    return ret;
}

// From qtsdk-2010.05/qt/src/network/access/qhttpnetworkreply.cpp
int ClientParser::gunzipBodyPartially(QByteArray &compressed, QByteArray &inflated)
{
    int ret = Z_DATA_ERROR;
    unsigned have;
    unsigned char out[CHUNK];
    int pos = -1;

    if (!_initInflate) {
        // check the header
        if (!gzipCheckHeader(compressed, pos))
            return ret;
        // allocate inflate state
        _inflateStrm.zalloc = Z_NULL;
        _inflateStrm.zfree = Z_NULL;
        _inflateStrm.opaque = Z_NULL;
        _inflateStrm.avail_in = 0;
        _inflateStrm.next_in = Z_NULL;
        ret = inflateInit2(&_inflateStrm, -MAX_WBITS);
        if (ret != Z_OK)
            return ret;
        _initInflate = true;
        _streamEnd = false;
    }

    //remove the header.
    compressed.remove(0, pos+1);

    // expand until deflate stream ends
    _inflateStrm.next_in = (unsigned char *)compressed.data();
    _inflateStrm.avail_in = compressed.size();
    do {
        _inflateStrm.avail_out = sizeof(out);
        _inflateStrm.next_out = out;
        ret = inflate(&_inflateStrm, Z_NO_FLUSH);
        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;
            // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(&_inflateStrm);
            _initInflate = false;
            return ret;
        }
        have = sizeof(out) - _inflateStrm.avail_out;
        inflated.append(QByteArray((const char *)out, have));
     } while (_inflateStrm.avail_out == 0);
    // clean up and return
    if (ret <= Z_ERRNO || ret == Z_STREAM_END) {
        inflateEnd(&_inflateStrm);
        _initInflate = false;
    }
    _streamEnd = (ret == Z_STREAM_END);

    return ret;
}

int ClientParser::readHeader()
{
    QString tmp;
    QString headerStr = QLatin1String("");

    QIODevice *socket = (QIODevice*)this->parent();
    while (!_haveAllHeaders && socket->canReadLine()) {
        tmp = QString::fromUtf8(socket->readLine());
        if (tmp == QLatin1String("\r\n") || tmp == QLatin1String("\n") || tmp.isEmpty()) {
            _haveAllHeaders = true;
        } else {
            int hdrSepIndex = tmp.indexOf(":");
            if (hdrSepIndex != -1) {
                QString hdrName = tmp.left(hdrSepIndex);
                QString hdrValue = tmp.mid(hdrSepIndex+1).trimmed();
                _requestHeaders.setRawHeader(hdrName.toUtf8(), hdrValue.toUtf8());
                //qDebug() << __PRETTY_FUNCTION__ << ":" << "Got header:" << hdrName << "--->" << hdrValue;
            }
            headerStr += tmp;
        }
    }

    if (_haveAllHeaders) {
        // If we get the Content-Length header, then the 0 above will get updated
        emit headerReceived(_requestHeaders);
    }

    if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowHTTPHeaders))
        qDebug() << __PRETTY_FUNCTION__ << ":" << "headerStr:" << endl << headerStr;

    return headerStr.length();
}

void ClientParser::readData()
{
    bool didGetCompleteRequest = false;
    if (!_haveAllHeaders) readHeader();

    QByteArray tempData;
    bool isCompressed = ((ClientHandler*)this->parent())->useCompression();
    if (isCompressed) {
        QByteArray compData = ((ClientHandler*)this->parent())->readAll();
        gunzipBodyPartially(compData, tempData);

        if (!_setDeviceForCompression) {
            _xmlSocketReader.clear();
            _setDeviceForCompression = true;
        }
        _xmlSocketReader.addData(tempData);
    }

    if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowRawSocketData)) {
        if (!_setDeviceForCompression) {
            tempData = ((ClientHandler*)this->parent())->readAll();
            _xmlSocketReader.clear();
            _xmlSocketReader.addData(tempData);
        }
        qDebug() << __PRETTY_FUNCTION__ << ": Raw Socket Data" << endl
                 << tempData
                 << endl;
    }

    if (_haveAllHeaders) {
        while (!_xmlSocketReader.atEnd()) {
            _xmlSocketReader.readNext();
            if (!_xmlSocketReader.isWhitespace() && _xmlSocketReader.tokenType() != QXmlStreamReader::Invalid) {
                _writer->writeCurrentToken(_xmlSocketReader);
            }

            if (_xmlSocketReader.tokenType() == QXmlStreamReader::EndDocument) {
                // We wait to parse the client XML until we've received a complete SOAP Document
                didGetCompleteRequest = true;
            }
        }

        if (didGetCompleteRequest && !_xmlSocketReader.error() && !_clientRequestXml.isEmpty()) {
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXML)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << endl
                        << "------ start client request XML ---------" << endl
                        << _clientRequestXml << endl
                        << "------ end client request XML ---------" << endl;
            }
            _xml.addData(_clientRequestXml);

            parseDocument();
            emit parsingComplete();
        } else if (_xmlSocketReader.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
            _requestError = MapRequest::IfmapClientSoapFault;
            _xml.raiseError(_xmlSocketReader.errorString());
            emit parsingComplete();
        }
    }
}

void ClientParser::registerMetadataNamespaces()
{
    QXmlStreamNamespaceDeclarations nsVector = _xml.namespaceDeclarations();
    for (int i=0; i<nsVector.size(); i++) {
        _namespaces.insert(nsVector.at(i).prefix().toString(), nsVector.at(i).namespaceUri().toString());
    }

}

void ClientParser::setSessionId(MapRequest &request)
{
    if (_requestVersion == MapRequest::IFMAPv11 && _clientSetSessionId) {
        request.setSessionId(_sessionId);
        request.setClientSetSessionId(true);
    } else if (_requestVersion == MapRequest::IFMAPv20 &&
               _xml.attributes().hasAttribute("session-id")) {
        request.setSessionId(_xml.attributes().value("session-id").toString());
        request.setClientSetSessionId(true);

        _sessionId = _xml.attributes().value("session-id").toString();
        _clientSetSessionId = true;
    }
}

void ClientParser::parseDocument()
{
    if (_xml.readNextStartElement()) {
        if (_xml.name().compare("Envelope", Qt::CaseSensitive) == 0) {
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Got SOAP Envelope"
                        << "in namespace:" << _xml.namespaceUri();
            }
            registerMetadataNamespaces();
            parseEnvelope();
        } else {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Error: Did not get a SOAP Envelope";
            _xml.raiseError("Did not get a SOAP Envelope");
            _requestError = MapRequest::IfmapClientSoapFault;
        }
    }
}

void ClientParser::parseEnvelope()
{
    bool done = false;
    while (!_xml.atEnd() && !done) {
        _xml.readNext();

        if (_xml.isStartElement() && _xml.name() == "Header") {
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Got SOAP Header"
                        << "in namespace:" << _xml.namespaceUri();
            }
            registerMetadataNamespaces();
            parseHeader();
        } else if (_xml.isStartElement() && _xml.name() == "Body") {
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Got SOAP Body"
                        << "in namespace:" << _xml.namespaceUri();
            }
            registerMetadataNamespaces();
            parseBody();
        } else if (_xml.isStartElement()) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Error reading SOAP Header or Body:" << _xml.name() << _xml.tokenString();
            _xml.raiseError("Error reading SOAP Header or Body");
            _requestError = MapRequest::IfmapClientSoapFault;
        }

        if (_xml.tokenType() == QXmlStreamReader::EndElement &&
            _xml.name().compare("Envelope", Qt::CaseSensitive) == 0) {
            done = true;
        }
    }
}

void ClientParser::parseHeader()
{
    _xml.readNextStartElement();
    if (_xml.name() == "new-session" && _xml.namespaceUri() == IFMAP_NS_1 &&
        _omapdConfig->valueFor("version_support").value<OmapdConfig::MapVersionSupportOptions>().testFlag(OmapdConfig::SupportIfmapV10)) {
        // Support for IF-MAP 1.0 client new-session, but this is still IF-MAP 1.1 operations
        _requestVersion = MapRequest::IFMAPv11;
        parseNewSession();
    } else if (_xml.name() == "attach-session" && _xml.namespaceUri() == IFMAP_NS_1 &&
               _omapdConfig->valueFor("version_support").value<OmapdConfig::MapVersionSupportOptions>().testFlag(OmapdConfig::SupportIfmapV10)) {
        // Support for IF-MAP 1.0 client attach-session, but this is still IF-MAP 1.1 operations
        _requestVersion = MapRequest::IFMAPv11;
        parseAttachSession();
    } else if (_xml.name() == "session-id" && _xml.namespaceUri() == IFMAP_NS_1 &&
               _omapdConfig->valueFor("version_support").value<OmapdConfig::MapVersionSupportOptions>().testFlag(OmapdConfig::SupportIfmapV11)) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "reading session-id";
        _sessionId = _xml.readElementText();
        _clientSetSessionId = true;
    } else {
        //_xml.skipCurrentElement();
    }
}

void ClientParser::parseNewSession()
{
    NewSessionRequest nsReq;
    nsReq.setRequestVersion(_requestVersion);
    _requestType = MapRequest::NewSession;

    /* IFMAP20: 4.3: max-poll-result-size is an optional attribute that indicates to the
       MAP Server the amount of buffer space the client would like to have allocated to
       hold poll results. A MAP Server MUST support buffer sizes of at least 5,000,000
       bytes, and MAY support larger sizes.
    */
    if (_requestVersion == MapRequest::IFMAPv20) {
        QXmlStreamAttributes nsAttrs = _xml.attributes();
        if (nsAttrs.hasAttribute("max-poll-result-size")) {
            bool ok = false;
            uint mprs = nsAttrs.value("max-poll-result-size").toString().toUInt(&ok);
            if (ok) {
                nsReq.setMaxPollResultSize(mprs);
                nsReq.setClientSetMaxPollResultSize(true);
            } else {
                // Client fault
                nsReq.setRequestError(MapRequest::IfmapClientSoapFault);
                _xml.raiseError("Error reading new session request");
                _requestError = MapRequest::IfmapClientSoapFault;
            }
        }
    }
    _mapRequest.setValue(nsReq);

}

void ClientParser::parseAttachSession()
{
    AttachSessionRequest asReq;
    asReq.setRequestVersion(_requestVersion);
    _requestType = MapRequest::AttachSession;

    QString sessionId = _xml.readElementText();
    if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "Got session-id in request:" << sessionId;
    }

    if (! sessionId.isEmpty()) {
        asReq.setSessionId(sessionId);
        asReq.setClientSetSessionId(true);

        _sessionId = sessionId;
        _clientSetSessionId = true;
    }

    _mapRequest.setValue(asReq);
}

void ClientParser::parseBody()
{
    while (_xml.readNextStartElement() && _mapRequest.isNull()) { // Make sure we only read the first request
        QString method = _xml.name().toString();
        QString methodNS = _xml.namespaceUri().toString();
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Got IF-MAP client request:" << method
                    << "in namespace:" << methodNS;
        }

        if (methodNS == IFMAP_NS_1 &&
            _omapdConfig->valueFor("version_support").value<OmapdConfig::MapVersionSupportOptions>().testFlag(OmapdConfig::SupportIfmapV11)) {
            _requestVersion = MapRequest::IFMAPv11;
        } else if (methodNS == IFMAP_NS_2 &&
                   _omapdConfig->valueFor("version_support").value<OmapdConfig::MapVersionSupportOptions>().testFlag(OmapdConfig::SupportIfmapV20)) {
            _requestVersion = MapRequest::IFMAPv20;
        } else {
            // ERROR!!!
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Error: Incorrect IF-MAP Namespace:" << methodNS;
            _requestError = MapRequest::IfmapClientSoapFault;
            _xml.raiseError("Did not get a valid IF-MAP Namespace");
        }

        if (method == "new-session" && methodNS == IFMAP_NS_1) {
            parseNewSession();
        } else if (method == "attach-session" && methodNS == IFMAP_NS_1) {
            parseAttachSession();
        } else if (method == "newSession" && methodNS == IFMAP_NS_2) {
            parseNewSession();
        } else if (method == "endSession" && methodNS == IFMAP_NS_2) {
            parseEndSession();
        } else if (method == "renewSession" && methodNS == IFMAP_NS_2) {
            parseRenewSession();
        } else if (method == "publish") {
            registerMetadataNamespaces();
            parsePublish();
        } else if (method == "subscribe") {
            registerMetadataNamespaces();
            parseSubscribe();
        } else if (method == "search") {
            registerMetadataNamespaces();
            parseSearch();
        } else if (method == "purgePublisher") {
            parsePurgePublisher();
        } else if (method == "poll") {
            parsePoll();
        } else {
            // ERROR!!!
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Error reading element:" << _xml.name();
            _requestError = MapRequest::IfmapClientSoapFault;
            _xml.raiseError("Did not get a valid IF-MAP Request");
        }
    }
}

void ClientParser::parseRenewSession()
{
    RenewSessionRequest rnReq;
    rnReq.setRequestVersion(_requestVersion);
    _requestType = MapRequest::RenewSession;

    setSessionId(rnReq);

    _mapRequest.setValue(rnReq);
}

void ClientParser::parseEndSession()
{
    EndSessionRequest esReq;
    esReq.setRequestVersion(_requestVersion);
    _requestType = MapRequest::EndSession;

    setSessionId(esReq);

    _mapRequest.setValue(esReq);
}

void ClientParser::parsePurgePublisher()
{
    PurgePublisherRequest ppReq;
    ppReq.setRequestVersion(_requestVersion);
    _requestType = MapRequest::PurgePublisher;

    setSessionId(ppReq);

    QString pubIdAttrName;
    if (_requestVersion == MapRequest::IFMAPv11) {
        pubIdAttrName = "publisher-id";
    } else if (_requestVersion == MapRequest::IFMAPv20) {
        pubIdAttrName = "ifmap-publisher-id";
    }

    QString reqPubId;

    QXmlStreamAttributes attrs = _xml.attributes();
    if (attrs.hasAttribute(pubIdAttrName)) {
        reqPubId = attrs.value(pubIdAttrName).toString();
        ppReq.setPublisherId(reqPubId);
        ppReq.setClientSetPublisherId(true);
    } else {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "Error reading publisher-id in purgePublisher request";
        _xml.raiseError("Error reading publisher-id in purgePublisher request");
        _requestError = MapRequest::IfmapAccessDenied;
        ppReq.setRequestError(MapRequest::IfmapAccessDenied);
    }

    if (_requestError == MapRequest::ErrorNone) {
        const QString& authToken = ((ClientHandler*)this->parent())->authToken();
        QString clientPubId = MapSessions::getInstance()->pubIdForAuthToken(authToken);
        OmapdConfig::AuthzOptions authz = MapSessions::getInstance()->authzForAuthToken(authToken);

        if (clientPubId.compare(reqPubId, Qt::CaseSensitive) == 0) {
            if (!authz.testFlag(OmapdConfig::AllowPurgeSelf)) {
                _xml.raiseError("Client not authorized to purgeSelf");
                _requestError = MapRequest::IfmapAccessDenied;
            }
        } else {
            if (!authz.testFlag(OmapdConfig::AllowPurgeOthers)) {
                _xml.raiseError("Client not authorized to purgeOthers");
                _requestError = MapRequest::IfmapAccessDenied;
            }
        }
    }

    _mapRequest.setValue(ppReq);
}

void ClientParser::parsePublish()
{
    PublishRequest pubReq;
    pubReq.setRequestVersion(_requestVersion);
    _requestType = MapRequest::Publish;
    setSessionId(pubReq);
    const QString& authToken = ((ClientHandler*)this->parent())->authToken();
    pubReq.setPublisherId(MapSessions::getInstance()->pubIdForAuthToken(authToken));

    OmapdConfig::AuthzOptions authz = MapSessions::getInstance()->authzForAuthToken(authToken);
    if (!authz.testFlag(OmapdConfig::AllowPublish)) {
        _xml.raiseError("Client not authorized to publish");
        _requestError = MapRequest::IfmapAccessDenied;
    }

    while (_xml.readNextStartElement() && !pubReq.requestError()) {
        if (_xml.name().compare("update", Qt::CaseSensitive) == 0) {
            parsePublishUpdateOrNotify(PublishOperation::Update, pubReq);
        } else if (_xml.name().compare("delete", Qt::CaseSensitive) == 0) {
            parsePublishDelete(pubReq);
        } else if (_xml.name().compare("notify", Qt::CaseSensitive) == 0) {
            parsePublishUpdateOrNotify(PublishOperation::Notify, pubReq);
        } else {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Error reading publish operation in request";
            _xml.raiseError("Error reading publish operation in request");
            _requestError = MapRequest::IfmapClientSoapFault;
            pubReq.setRequestError(MapRequest::IfmapClientSoapFault);
        }
    }
    _mapRequest.setValue(pubReq);
}

void ClientParser::parsePublishUpdateOrNotify(PublishOperation::PublishType publishType, PublishRequest &pubReq)
{
    PublishOperation pubOperation;
    QXmlStreamAttributes attrs = _xml.attributes();

    pubOperation._publishType = publishType;

    if (publishType == PublishOperation::Update) {
        if (pubReq.requestVersion() == MapRequest::IFMAPv11) {
            pubOperation._lifetime = Meta::LifetimeForever;
            pubOperation._clientSetLifetime = false;
        } else if (pubReq.requestVersion() == MapRequest::IFMAPv20) {
            if (attrs.hasAttribute("lifetime")) {
                if (attrs.value("lifetime") == "session") {
                    pubOperation._lifetime = Meta::LifetimeSession;
                    pubOperation._clientSetLifetime = true;
                } else if (attrs.value("lifetime") == "forever") {
                    pubOperation._lifetime = Meta::LifetimeForever;
                    pubOperation._clientSetLifetime = true;
                } else {
                    _xml.raiseError("Error reading publish lifetime");
                    _requestError = MapRequest::IfmapClientSoapFault;
                    pubReq.setRequestError(MapRequest::IfmapClientSoapFault);
                }
            } else {
                // IFMAP20: 3.7.1: LifetimeSession is default
                pubOperation._lifetime = Meta::LifetimeSession;
                pubOperation._clientSetLifetime = false;
            }
        }
    } else {
        pubOperation._lifetime = Meta::LifetimeSession;
        pubOperation._clientSetLifetime = false;
    }

    bool done = false;
    int numIds = 0;
    Id id1;
    Id id2;
    bool haveMetadata = false;

    while (!_xml.atEnd() && !done) {
        _xml.readNext();

        if (_xml.isStartElement() && ID_NAMES.contains(_xml.name().toString(), Qt::CaseSensitive)) {
            if (numIds == 0) {
                id1 = parseIdentifier(pubReq);
            } else if (numIds == 1) {
                id2 = parseIdentifier(pubReq);
            }
            numIds++;
        } else if (_xml.isStartElement() && _xml.name().compare("metadata", Qt::CaseSensitive) == 0) {
            pubOperation._metadata = parseMetadata(pubReq, pubOperation._lifetime);
            haveMetadata = true;
        }

        if (_xml.tokenType() == QXmlStreamReader::EndElement &&
            _xml.name().compare((publishType == PublishOperation::Update ? "update" : "notify"), Qt::CaseSensitive) == 0) {
            done = true;
        }
    }

    if (numIds < 1 || numIds > 2 || !haveMetadata) {
        _xml.raiseError("Invalid publish operation");
        _requestError = MapRequest::IfmapClientSoapFault;
        pubReq.setRequestError(MapRequest::IfmapClientSoapFault);
    } else {
        pubOperation._isLink = (numIds == 2 ? true : false);
        if (numIds == 2) {
            pubOperation._link = Identifier::makeLinkFromIds(id1, id2);
        } else {
            pubOperation._link.first = id1;
        }
        pubOperation._filterNamespaceDefinitions = _namespaces;
        pubOperation._operationNumber = pubReq.publishOperations().size() + 1;
        pubReq.addPublishOperation(pubOperation);
    }

    if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "Have publish"
                << (publishType == PublishOperation::Update ? "update" : "notify")
                << "on"
                << (numIds == 2 ? "link" : "identifier");
    }
}

void ClientParser::parsePublishDelete(PublishRequest &pubReq)
{
    PublishOperation pubOperation;
    pubOperation._publishType = PublishOperation::Delete;

    QXmlStreamAttributes attrs = _xml.attributes();
    if (attrs.hasAttribute("filter")) {
        pubOperation._deleteFilter = attrs.value("filter").toString();
        pubOperation._clientSetDeleteFilter = true;

        // TODO: make sure the application of the delete filter
        // does not result in a system error (from say a XMLQuery error)
        // that would render the entire publish operation invalid.
    }

    bool done = false;
    int numIds = 0;
    Id id1;
    Id id2;

    while (!_xml.atEnd() && !done) {
        _xml.readNext();

        if (_xml.isStartElement() && ID_NAMES.contains(_xml.name().toString(), Qt::CaseSensitive)) {
            if (numIds == 0) {
                id1 = parseIdentifier(pubReq);
            } else if (numIds == 1) {
                id2 = parseIdentifier(pubReq);
            }
            numIds++;
        }

        if (_xml.tokenType() == QXmlStreamReader::EndElement && _xml.name().compare("delete") == 0) {
            done = true;
        }
    }

    if (numIds < 1 || numIds > 2) {
        _xml.raiseError("Invalid publish operation");
        _requestError = MapRequest::IfmapClientSoapFault;
        pubReq.setRequestError(MapRequest::IfmapClientSoapFault);
    } else {
        pubOperation._isLink = (numIds == 2 ? true : false);
        if (numIds == 2) {
            pubOperation._link = Identifier::makeLinkFromIds(id1, id2);
        } else {
            pubOperation._link.first = id1;
        }
        pubOperation._filterNamespaceDefinitions = _namespaces;
        pubOperation._operationNumber = pubReq.publishOperations().size() + 1;
        pubReq.addPublishOperation(pubOperation);
    }

    if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "Have publish delete on"
                << (numIds == 2 ? "link" : "identifier");
    }
}

void ClientParser::parseSearch()
{
    SearchRequest searchReq;
    searchReq.setRequestVersion(_requestVersion);
    _requestType = MapRequest::Search;
    setSessionId(searchReq);

    searchReq.setSearch(parseSearchDetails(searchReq));
    _mapRequest.setValue(searchReq);;
}

SearchType ClientParser::parseSearchDetails(MapRequest &request)
{
    SearchType search;

    QXmlStreamAttributes attrs = _xml.attributes();

    /* IFMAP20: 3.7.2.8: If a MAP Client does not specify max-depth,
       the MAP Server MUST process the search with a max-depth of zero.
       If a MAP Client specifies a max-depth less than zero, the MAP
       Server MAY process the search with an unbounded max-depth.
    */
    int maxDepth = 0;
    if (attrs.hasAttribute("max-depth")) {
        QString md = attrs.value("max-depth").toString();
        bool ok;
        maxDepth = md.toInt(&ok);
        if (ok) {
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Got search parameter max-depth:" << maxDepth;
            }
            if (maxDepth < 0 && request.requestVersion() == MapRequest::IFMAPv11) {
                maxDepth = IFMAP_MAX_DEPTH_MAX;
            } else if (maxDepth < 0 && request.requestVersion() == MapRequest::IFMAPv20) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Got invalid search parameter max-depth:" << maxDepth;
                _xml.raiseError("Error with search attribute max-depth");
                _requestError = MapRequest::IfmapClientSoapFault;
                request.setRequestError(MapRequest::IfmapClientSoapFault);
            }
        } else {
            maxDepth = 0;
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Got invalid search parameter max-depth:" << md;
            _xml.raiseError("Error converting search attribute max-depth");
            _requestError = MapRequest::IfmapClientSoapFault;
            request.setRequestError(MapRequest::IfmapClientSoapFault);
        }
    } else {
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Using default search parameter max-depth:" << maxDepth;
        }
    }
    search.setMaxDepth(maxDepth);

    /* IFMAP20: 3.7.2.8: MAP Servers MUST support size constraints up to
       and including 100KB 1 . If a MAP Client does not specify max-size,
       the MAP Server MUST process the search with a max-size of 100KB.
       If a MAP Client specifies a max-size of -1, the MAP Server MAY
       process the search with an unbounded max-size. If a MAP Client
       specifies a max-size that exceeds what the MAP Server can support,
       the MAP Server MUST enforce its own maximum size constraints.
    */
    long long maxSize = IFMAP_MAX_SIZE;
    if (attrs.hasAttribute("max-size")) {
        QString ms = attrs.value("max-size").toString();
        bool ok;
        maxSize = ms.toLongLong(&ok);
        if (ok) {
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Got search parameter max-size:" << maxSize;
            }
            if (maxSize < 0 && request.requestVersion() == MapRequest::IFMAPv11) {
                maxSize = IFMAP_MAX_SIZE;
            } else if (maxSize < 0 && request.requestVersion() == MapRequest::IFMAPv20) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Got invalid search parameter max-size:" << maxSize;
                _xml.raiseError("Error with search attribute max-size");
                _requestError = MapRequest::IfmapClientSoapFault;
                request.setRequestError(MapRequest::IfmapClientSoapFault);
            }
        } else {
            maxSize = 0;
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Got invalid search parameter max-size:" << ms;
            _xml.raiseError("Error converting search attribute max-size");
            _requestError = MapRequest::IfmapClientSoapFault;
            request.setRequestError(MapRequest::IfmapClientSoapFault);
        }
    } else {
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Using default search parameter max-size:" << maxSize;
        }
    }
    search.setMaxSize(maxSize);

    if (attrs.hasAttribute("match-links")) {
        QString matchLinks = attrs.value("match-links").toString();
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Got search parameter match-links:" << matchLinks;
        }
        search.setMatchLinks(matchLinks);
    } else {
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Using default search parameter match-links:" << search.matchLinks().str();
        }
    }

    if (attrs.hasAttribute("result-filter")) {
        QString resultFilter = attrs.value("result-filter").toString();
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Got search parameter result-filter:" << resultFilter;
        }
        search.setResultFilter(resultFilter);
    } else {
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Using default search parameter result-filter:" << search.resultFilter().str();
        }
    }

    if (attrs.hasAttribute("terminal-identifier-type") && request.requestVersion() == MapRequest::IFMAPv20) {
        QString terminalId = attrs.value("terminal-identifier-type").toString();
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Got search parameter terminal-identifier-type:" << terminalId;
        }
        search.setTerminalId(terminalId);
    } else {
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Using default search parameter terminal-identifier-type:" << search.terminalId();
        }
    }

    if (!_xml.hasError()) {
        _xml.readNextStartElement();
        if (_xml.name() == "identifier" && request.requestVersion() == MapRequest::IFMAPv11) {
            _xml.readNextStartElement();
        }
        Id startId = parseIdentifier(request);
        if (request.requestError() == MapRequest::ErrorNone) {
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Setting starting identifier:" << startId;
            }
            search.setStartId(startId);
        }
    }

    // Finally set filterNamespaceDefinitions pulled out of SOAP Message
    search.setFilterNamespaceDefinitions(_namespaces);

    return search;
}

void ClientParser::parseSubscribe()
{
    SubscribeRequest subReq;
    subReq.setRequestVersion(_requestVersion);
    _requestType = MapRequest::Subscribe;
    setSessionId(subReq);


    bool done = false;
    while (!_xml.atEnd() && !done && !subReq.requestError()) {
        _xml.readNext();

        SubscribeOperation subOperation;

        if (_xml.tokenType() == QXmlStreamReader::StartElement) {
            QXmlStreamAttributes attrs = _xml.attributes();
            if (attrs.hasAttribute("name")) {
                subOperation.setName(attrs.value("name").toString());
            } else {
                _xml.raiseError("Error reading subscription name");
                _requestError = MapRequest::IfmapClientSoapFault;
                subReq.setRequestError(MapRequest::IfmapClientSoapFault);
            }

            if (_xml.tokenType() == QXmlStreamReader::StartElement && _xml.name().compare("update", Qt::CaseSensitive) == 0) {
                subOperation.setSubscribeType(SubscribeOperation::Update);
                subOperation.setSearch(parseSearchDetails(subReq));
                subReq.addSubscribeOperation(subOperation);
            } else if (_xml.tokenType() == QXmlStreamReader::StartElement && _xml.name().compare("delete", Qt::CaseSensitive) == 0) {
                subOperation.setSubscribeType(SubscribeOperation::Delete);
                subReq.addSubscribeOperation(subOperation);
            } else {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Error reading subscribe operation in request";
                _xml.raiseError("Error reading subscribe operation in request");
                _requestError = MapRequest::IfmapClientSoapFault;
                subReq.setRequestError(MapRequest::IfmapClientSoapFault);
            }
        }

        if (_xml.tokenType() == QXmlStreamReader::EndElement && _xml.name().compare("subscribe") ==0) {
            done = true;
        }
    }
    _mapRequest.setValue(subReq);
}

void ClientParser::parsePoll()
{
    PollRequest pollReq;
    pollReq.setRequestVersion(_requestVersion);
    _requestType = MapRequest::Poll;
    setSessionId(pollReq);

    _mapRequest.setValue(pollReq);
}

Id ClientParser::parseIdentifier(MapRequest &request)
{
    bool parseError = false;

    QXmlStreamAttributes attrs = _xml.attributes();
    QString ad = attrs.hasAttribute("administrative-domain") ?
                 attrs.value("administrative-domain").toString() :
                 QString();

    Identifier::IdType idType = Identifier::IdNone;
    QString value;
    QString other; // This is only for type Identifier::IdentityOther


    if (_xml.name().compare("access-request", Qt::CaseSensitive) == 0) {
        idType = Identifier::AccessRequest;
        if (attrs.hasAttribute("name")) {
            idType = Identifier::AccessRequest;
            value = attrs.value("name").toString();
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Got access-request name:" << value;
            }
        } else {
            // Error - did not specify access-request name
            parseError = true;
            request.setRequestError(MapRequest::IfmapInvalidIdentifier);
        }
    } else if (_xml.name().compare("device", Qt::CaseSensitive) == 0) {
        if (_xml.readNextStartElement()) {
            if (_xml.name().compare("name", Qt::CaseSensitive) == 0) {
                idType = Identifier::DeviceName;
                value = _xml.readElementText();
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "Got device name:" << value;
                }
            } else if (_xml.name().compare("aik-name", Qt::CaseSensitive) == 0 && _requestVersion == MapRequest::IFMAPv11) {
                idType = Identifier::DeviceAikName;
                value = _xml.readElementText();
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "Got device aik-name:" << value;
                }
            } else {
                // Error - unknown device type
                parseError = true;
                request.setRequestError(MapRequest::IfmapInvalidIdentifier);
            }
        }

    } else if (_xml.name().compare("identity", Qt::CaseSensitive) == 0) {
        QString type;
        if (attrs.hasAttribute("type")) {
            type = attrs.value("type").toString();
            if (type.compare("aik-name") == 0) {
                idType = Identifier::IdentityAikName;
            } else if (type.compare("distinguished-name") == 0) {
                idType = Identifier::IdentityDistinguishedName;
            } else if (type.compare("dns-name") == 0) {
                idType = Identifier::IdentityDnsName;
            } else if (type.compare("email-address") == 0) {
                idType = Identifier::IdentityEmailAddress;
            } else if (type.compare("kerberos-principal") == 0) {
                idType = Identifier::IdentityKerberosPrincipal;
            } else if (type.compare("trusted-platform-module") == 0
                       && request.requestVersion() == MapRequest::IFMAPv11) {
                idType = Identifier::IdentityTrustedPlatformModule;
            } else if (type.compare("username") == 0) {
                idType = Identifier::IdentityUsername;
            } else if (type.compare("sip-uri") == 0) {
                idType = Identifier::IdentitySipUri;
            } else if (type.compare("hip-hit") == 0 &&
                request.requestVersion() == MapRequest::IFMAPv20) {
                idType = Identifier::IdentityHipHit;
            } else if (type.compare("tel-uri") == 0) {
                idType = Identifier::IdentityTelUri;
            } else if (type.compare("other") == 0) {
                idType = Identifier::IdentityOther;
            } else {
                // Error - unknown identity type
                parseError = true;
                request.setRequestError(MapRequest::IfmapInvalidIdentifierType);
            }
        } else {
            // Error - did not specify identity type
            parseError = true;
            request.setRequestError(MapRequest::IfmapInvalidIdentifier);
        }

        if (attrs.hasAttribute("name")) {
            value = attrs.value("name").toString();
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Got identity name:" << value;
            }
        } else {
            // Error - did not specify identity name attribute
            parseError = true;
            request.setRequestError(MapRequest::IfmapInvalidIdentifier);
        }

        if (idType == Identifier::IdentityOther) {
            if (attrs.hasAttribute("other-type-definition")) {
                // Append other-type-definition to value
                other = attrs.value("other-type-definition").toString();
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "Got identity other-type-def:" << other;
                }
            } else {
                // Error - MUST have other-type-definition if idType is IdentityOther
                parseError = true;
                request.setRequestError(MapRequest::IfmapInvalidIdentifier);
            }
        }

        // Validation
        // Attempt to validate HIT
        QHostAddress test;
        if (idType == Identifier::IdentityHipHit && !parseError ) {
            if (!test.setAddress(value)) {
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "Got invalid hip-hit address conversion:" << value;
                }
                parseError = true;
                request.setRequestError(MapRequest::IfmapInvalidIdentifier);
            } else if (test == QHostAddress::AnyIPv6) {
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "Got IPv6 special address as HIT:" << value;
                }
            } else if (test.toString().toLower().compare(value, Qt::CaseSensitive) != 0) {
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "Got different hip-hit address back to string:" << test.toString();
                }
                parseError = true;
                request.setRequestError(MapRequest::IfmapInvalidIdentifier);
            }
        }

    } else if (_xml.name().compare("ip-address", Qt::CaseSensitive) == 0) {
        QString type;
        if (attrs.hasAttribute("type")) {
            type = attrs.value("type").toString();
            if (type.compare("IPv4") == 0) {
                idType = Identifier::IpAddressIPv4;
            } else if (type.compare("IPv6") == 0) {
                idType = Identifier::IpAddressIPv6;
            } else {
                // Error - did not correctly specify type
                parseError = true;
                request.setRequestError(MapRequest::IfmapInvalidIdentifier);
            }
        } else {
            idType = Identifier::IpAddressIPv4;
        }

        if (attrs.hasAttribute("value")) {
            value = attrs.value("value").toString();
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Got ip-address:" << value;
            }
            // Attempt to validate IP Address
            QHostAddress test;
            if (!test.setAddress(value)) {
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "Got invalid ip-address:" << value;
                }
                parseError = true;
                request.setRequestError(MapRequest::IfmapInvalidIdentifier);
            } else if (test.toString().toLower().compare(value, Qt::CaseSensitive) != 0 ||
                       test.toString().contains("::")) {
                // Allow non-RFC5952 formatting per TCG Compliance test
                if (!(value.compare("0:0:0:0:0:0:0:0") == 0 &&
                      test.toString().compare("::") == 0)) {
                    if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                        qDebug() << __PRETTY_FUNCTION__ << ":" << "Got different ip-address address back to string:" << test.toString();
                    }
                    parseError = true;
                    request.setRequestError(MapRequest::IfmapInvalidIdentifier);
                }
            }
        } else {
            // Error - did not specify ip-address value attribute
            parseError = true;
            request.setRequestError(MapRequest::IfmapInvalidIdentifier);
        }

    } else if (_xml.name().compare("mac-address", Qt::CaseSensitive) == 0) {
        idType = Identifier::MacAddress;

        if (attrs.hasAttribute("value")) {
            value = attrs.value("value").toString();
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Got mac-address:" << value;
            }
            // Attempt to validate MAC
            QRegExp re;
            re.setPattern("([0-9a-f][0-9a-f]:){5}[0-9a-f][0-9a-f]");
            re.setCaseSensitivity(Qt::CaseSensitive);
            if (!re.exactMatch(value)) {
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "Got invalid mac-address:" << value;
                }
                parseError = true;
                request.setRequestError(MapRequest::IfmapInvalidIdentifier);
            }
        } else {
            // Error - did not specify mac-address value attribute
            parseError = true;
            request.setRequestError(MapRequest::IfmapInvalidIdentifier);
        }

    } else {
        // Error - unknown identifier name
        parseError = true;
        request.setRequestError(MapRequest::IfmapInvalidIdentifierType);
    }

    Id id;
    if (!parseError) {
        id.setType(idType);
        id.setAd(ad);
        id.setValue(value);
        id.setOther(other);
    } else {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "Error parsing identifier";
        _requestError = request.requestError();
    }
    return id;
}

QList<Meta> ClientParser::parseMetadata(PublishRequest &pubReq, Meta::Lifetime lifetime)
{
    QList<Meta> metaList;

    QString cardinalityAttrName = "cardinality", pubIdAttrName = "publisher-id", timestampAttrName = "timestamp";
    if (_requestVersion == MapRequest::IFMAPv20) {
        cardinalityAttrName = "ifmap-cardinality";
        pubIdAttrName = "ifmap-publisher-id";
        timestampAttrName = "ifmap-timestamp";
    }

    QString allMetadata;

    while (_xml.readNextStartElement()) {
        QString metaNS = _xml.namespaceUri().toString();
        QString metaName = _xml.name().toString();
        QString metaQName = _xml.qualifiedName().toString();
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowClientOps))
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Got metadata element:" << metaQName << "in ns:" << metaNS;

        if (metaNS.isEmpty()) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Error: metadata element has no associated namespace:" << metaName;
        }

        // Local QXmlStreamWriter to add operational attributes
        QString metaString;
        QXmlStreamWriter xmlWriter(&metaString);

        // Check for attributes to apply
        QXmlStreamAttributes elementAttrs = _xml.attributes();

        if (_requestVersion == MapRequest::IFMAPv20) {
            // Remove any ifmap-* invalid attributes; only allowed client-supplied attribute is ifmap-cardinality
            for (int i=0; i<elementAttrs.size(); i++) {
                QString attrStr = elementAttrs.at(i).name().toString();
                if (attrStr.startsWith("ifmap-")) {
                    if (attrStr.compare(cardinalityAttrName) != 0) {
                        // Have invalid ifmap- attribute
                        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                            qDebug() << __PRETTY_FUNCTION__ << ":" << "Got invalid ifmap- reserved attribute:" << attrStr;
                        }
                        elementAttrs.remove(i);
                    }
                }
            }
        }

        xmlWriter.writeStartElement(metaNS, metaName);
        xmlWriter.writeAttributes(elementAttrs);

        Meta::Cardinality cardinalityValue;
        // Make sure we have the cardinality attribute and default it to multiValue
        if (elementAttrs.hasAttribute(cardinalityAttrName)) {
            cardinalityValue = (elementAttrs.value(cardinalityAttrName) == "singleValue")
                               ? Meta::SingleValue : Meta::MultiValue;
        } else if (_requestVersion == MapRequest::IFMAPv20) {
            /* IFMAP20: 3.3.3: A MAP client MUST define the ifmap-cardinality attribute of
               any metadata as singleValue or mulltiValue.
            */
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Error: metadata element does not specify ifmap-cardinality:" << metaName;
            _requestError = MapRequest::IfmapInvalidMetadata;
            pubReq.setRequestError(MapRequest::IfmapInvalidMetadata);
        } else {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Notice: assigning metadata element multiValue cardinality:" << metaName;
            cardinalityValue = Meta::MultiValue;
            xmlWriter.writeAttribute(cardinalityAttrName, "multiValue");
        }

        // Set timestamp operational attribute
        QString ts = QDateTime::currentDateTime().toUTC().toString("yyyy-MM-ddThh:mm:ssZ");
        xmlWriter.writeAttribute(timestampAttrName, ts);
        // Set publisher-id operational attribute
        xmlWriter.writeAttribute(pubIdAttrName, pubReq.publisherId());

        // While loop to recursively descend this metaName element, stopping when we get
        // to the closing metaName element (EndElement tokenType) or if we get an error
        while (!_xml.hasError() &&
               !(_xml.tokenType() == QXmlStreamReader::EndElement &&
                  _xml.name().compare(metaName, Qt::CaseSensitive) == 0 &&
                  _xml.namespaceUri().compare(metaNS, Qt::CaseSensitive) == 0)) {
            _xml.readNext();
            if (!_xml.isWhitespace()) {  // Don't include XML Whitespace
                xmlWriter.writeCurrentToken(_xml);
            }
        }
        xmlWriter.writeCurrentToken(_xml);

        if (_xml.hasError()) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Got an error:" << _xml.errorString();
            pubReq.setRequestError(MapRequest::IfmapClientSoapFault);
            _requestError = MapRequest::IfmapClientSoapFault;
        }

        Meta aMeta(cardinalityValue, lifetime);
        aMeta.setElementName(metaName);
        aMeta.setElementNS(metaNS);
        aMeta.setPublisherId(pubReq.publisherId());
        aMeta.setMetaXML(metaString);

        if (MapSessions::getInstance()->validateMetadata(aMeta)) {
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Setting xml for:" << metaName << "metaXML:" << aMeta.metaXML();
            }
            metaList << aMeta;

            allMetadata += metaString;
        } else {
            pubReq.setRequestError(MapRequest::IfmapInvalidMetadata);
            _requestError = MapRequest::IfmapInvalidMetadata;
            _xml.raiseError("Metadata did not pass validation test");
        }

        const QString& authToken = ((ClientHandler*)this->parent())->authToken();
        bool metaPolicy = MapSessions::getInstance()->metadataAuthorizationForAuthToken(authToken, metaName, metaNS);
        if (!metaPolicy) {
            pubReq.setRequestError(MapRequest::IfmapAccessDenied);
            _requestError = MapRequest::IfmapAccessDenied;
            _xml.raiseError("Client not authorized to publish metadata in request");
        }

    }

    // Can check metadata length here too
    if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowXMLParsing)) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "All metadata in request:" << endl << allMetadata;
    }

    return metaList;
}
