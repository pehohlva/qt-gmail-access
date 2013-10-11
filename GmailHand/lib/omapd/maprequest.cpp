/*
maprequest.cpp: Implementation of IF-MAP Request classes

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

#include "maprequest.h"

QString MapRequest::requestTypeString(MapRequest::RequestType reqType)
{
    QString str = "";

    switch(reqType) {
    case MapRequest::RequestNone:
        str = "RequestNone";
        break;
    case MapRequest::NewSession:
        str = "NewSession";
        break;
    case MapRequest::AttachSession:
        str = "AttachSession";
        break;
    case MapRequest::EndSession:
        str = "EndSession";
        break;
    case MapRequest::RenewSession:
        str = "RenewSession";
        break;
    case MapRequest::Publish:
        str = "Publish";
        break;
    case MapRequest::Subscribe:
        str = "Subscribe";
        break;
    case MapRequest::Search:
        str = "Search";
        break;
    case MapRequest::PurgePublisher:
        str = "PurgePublisher";
        break;
    case MapRequest::Poll:
        str = "Poll";
        break;
    }

    return str;
}

QString MapRequest::requestVersionString(MapRequest::RequestVersion version)
{
    QString str = "";

    switch (version) {
    case MapRequest::VersionNone:
        str = "VersionNone";
        break;
    case MapRequest::IFMAPv11:
        str = "IFMAPv11";
        break;
    case MapRequest::IFMAPv20:
        str = "IFMAPv20";
        break;
    }

    return str;
}

QString MapRequest::requestVersionNamespace(MapRequest::RequestVersion version)
{
    QString str = "";

    switch (version) {
    case MapRequest::VersionNone:
        str = "VersionNone";
        break;
    case MapRequest::IFMAPv11:
        str = IFMAP_NS_1;
        break;
    case MapRequest::IFMAPv20:
        str = IFMAP_NS_2;
        break;
    }

    return str;
}

QString MapRequest::requestErrorString(MapRequest::RequestError error)
{
    QString str("");

    switch (error) {
        case MapRequest::ErrorNone:
            break;
        case MapRequest::IfmapClientSoapFault:
            str = "Client SOAP Fault";
            break;
        case MapRequest::IfmapAccessDenied:
            str = "AccessDenied";
            break;
        case MapRequest::IfmapFailure:
            str = "Failure";
            break;
        case MapRequest::IfmapInvalidIdentifier:
            str = "InvalidIdentifier";
            break;
        case MapRequest::IfmapInvalidIdentifierType:
            str = "InvalidIdentifierType";
            break;
        case MapRequest::IfmapIdentifierTooLong:
            str = "IdentifierTooLong";
            break;
        case MapRequest::IfmapInvalidMetadata:
            str = "InvalidMetadata";
            break;
        case MapRequest::IfmapInvalidMetadataListType:
            str = "InvalidMetadataListType";
            break;
        case MapRequest::IfmapInvalidSchemaVersion:
            str = "InvalidSchemaVersion";
            break;
        case MapRequest::IfmapInvalidSessionID:
            str = "InvalidSessionID";
            break;
        case MapRequest::IfmapMetadataTooLong:
            str = "MetadataTooLong";
            break;
        case MapRequest::IfmapSearchResultsTooBig:
            str = "SearchResultsTooBig";
            break;
        case MapRequest::IfmapPollResultsTooBig:
            str = "PollResultsTooBig";
            break;
        case MapRequest::IfmapSystemError:
            str = "SystemError";
            break;
    }

    return str;
}

MapRequest::MapRequest(MapRequest::RequestType requestType)
    : _requestType(requestType)
{
    _requestError = MapRequest::ErrorNone;
    _requestVersion = MapRequest::VersionNone;
    _clientSetSessionId = false;
}

MapRequest::MapRequest(const MapRequest &other)
{
    this->_clientSetSessionId = other._clientSetSessionId;
    this->_requestError = other._requestError;
    this->_requestType = other._requestType;
    this->_requestVersion = other._requestVersion;
    this->_sessionId = other._sessionId;
}

NewSessionRequest::NewSessionRequest(int maxPollResultSize)
    : MapRequest(MapRequest::NewSession), _maxPollResultSize(maxPollResultSize)
{
    _clientSetMaxPollResultSize = false;
}

NewSessionRequest::NewSessionRequest(const NewSessionRequest &other)
    : MapRequest(other)
{
    this->_clientSetMaxPollResultSize = other._clientSetMaxPollResultSize;
    this->_maxPollResultSize = other._maxPollResultSize;
}

EndSessionRequest::EndSessionRequest()
    : MapRequest(MapRequest::EndSession)
{
}

EndSessionRequest::EndSessionRequest(const EndSessionRequest &other)
    : MapRequest(other)
{
}

RenewSessionRequest::RenewSessionRequest()
    : MapRequest(MapRequest::RenewSession)
{
}

RenewSessionRequest::RenewSessionRequest(const RenewSessionRequest &other)
    : MapRequest(other)
{
}

AttachSessionRequest::AttachSessionRequest()
    : MapRequest(MapRequest::AttachSession)
{
}

AttachSessionRequest::AttachSessionRequest(const AttachSessionRequest &other)
    : MapRequest(other)
{
}

PurgePublisherRequest::PurgePublisherRequest()
    : MapRequest(MapRequest::PurgePublisher)
{
}

PurgePublisherRequest::PurgePublisherRequest(const PurgePublisherRequest &other)
    : MapRequest(other)
{
    this->_publisherId = other._publisherId;
    this->_clientSetPublisherId = other._clientSetPublisherId;
}

PublishOperation::PublishOperation()
{
    _publishType = PublishOperation::None;
    _clientSetDeleteFilter = false;
    _clientSetLifetime = false;
    _operationNumber = 0;
}

PublishOperation::~PublishOperation()
{
    _filterNamespaceDefinitions.clear();
    _metadata.clear();
}

PublishRequest::PublishRequest()
    : MapRequest(MapRequest::Publish)
{
}

PublishRequest::PublishRequest(const PublishRequest &other)
    : MapRequest(other)
{
    this->_publishOperations = other._publishOperations;
}

MetadataFilter::MetadataFilter(const QString& filterStr)
    : _filter(""), _simFilter(NULL)
{
    setFilter(filterStr);
}

MetadataFilter::MetadataFilter(const MetadataFilter& other)
{
    _simFilter = NULL;
    *this = other;
}

MetadataFilter::~MetadataFilter()
{
    if(_simFilter != NULL) delete _simFilter;
}

MetadataFilter& MetadataFilter::operator=(const MetadataFilter& other)
{
    _filter = other._filter;
    if(_simFilter != NULL) delete(_simFilter);

    if(other._simFilter != NULL)
    {
        _simFilter = new SimplifiedFilter(*other._simFilter);
    }
    else
    {
        _simFilter = NULL;
    }

    return *this;
}

bool MetadataFilter::operator==(const MetadataFilter& other) const
{
    return _filter == other._filter &&
        (_simFilter == NULL
            ? other._simFilter == NULL
            : other._simFilter != NULL && *_simFilter == *other._simFilter);
}

bool MetadataFilter::operator==(const QString& filterStr) const
{
    return _filter == filterStr;
}

// (LFu) this code used be the translateFilter() method in class Subscription
void MetadataFilter::setFilter(const QString& ifmapFilter)
{
    //const char *fnName = "MetadataFilter::setFilter:";

    /* non-predicate expressions joined by "or" need to be translated
       into a parenthesized expression separated by "|".

       Examples:
       meta:ip-mac or scada:node
       --> (meta:ip-mac | scada:node)

       meta:role[@publisher-id = "myPubId" or name="myRole"] or meta:ip-mac
       --> (meta:role[@publisher-id = "myPubId" or name="myRole"] | meta:ip-mac)

       standard XPath does not support predicate expressions that begin with [; need to add *
    */

    if(!ifmapFilter.isEmpty() && ifmapFilter != "*")
    {
        // (LFu) - determine if the filter looks like "ns-1:elem-1 or ... or ns-n:elem-n"
        // and provide simplified metadata matching in this case:
        QRegExp rx("((\\w|-)+:(\\w|-)+)(\\s+(or|OR)\\s+((\\w|-)+:(\\w|-)+))*");
        if(rx.exactMatch(ifmapFilter))
        {
            _simFilter = new SimplifiedFilter();
            // qDebug() << "~~regex matches filter: " << ifmapFilter;
            int pos = 0;
            QRegExp rx2("((\\w|-)+):((\\w|-)+)(\\s+(or|OR))?");
            while ((pos = rx2.indexIn(ifmapFilter, pos)) != -1) {
                _simFilter->append(QPair<QString, QString>(rx2.cap(1), rx2.cap(3)));
                // qDebug() << "~~\tns:" << rx2.cap(1) << " elem:" << rx2.cap(3);
                pos += rx2.matchedLength();
            }
        }
    }

    // TODO: Do this with QRegExp
    _filter = ifmapFilter;
    if (ifmapFilter.contains(" or ", Qt::CaseInsensitive)) {
        //qDebug() << fnName << "WARNING! filter translation is woefully incomplete!";
        //qDebug() << fnName << "filter before translation:" << ifmapFilter;
        _filter.replace(" or "," | ");
        _filter.prepend("(");
        _filter.append(")");
        //qDebug() << fnName << "filter after translation:" << qtFilter;
    }
}


SearchType::SearchType()    
    : _resultFilter("*"), _matchLinks("*") // Intepretation of the spec is that no match-links attribute matches all links
{
    _clientSetMaxDepth = false;
    _clientSetMaxSize = false;
    _clientSetResultFilter = false;
    _clientSetMatchLinks = false;
    _clientSetTerminalId = false;
    _terminalId = "";
}

SubscribeOperation::SubscribeOperation()
{
}

SearchRequest::SearchRequest()
    : MapRequest(MapRequest::Search)
{
}

SearchRequest::SearchRequest(const SearchRequest &other)
    : MapRequest(other)
{
    this->_search = other._search;
}

SubscribeRequest::SubscribeRequest()
    : MapRequest(MapRequest::Subscribe)
{
}

SubscribeRequest::SubscribeRequest(const SubscribeRequest &other)
    : MapRequest(other)
{
    this->_subscribeOperations = other._subscribeOperations;
}

PollRequest::PollRequest()
    : MapRequest(MapRequest::Poll)
{
}

PollRequest::PollRequest(const PollRequest &other)
    : MapRequest(other)
{
}
