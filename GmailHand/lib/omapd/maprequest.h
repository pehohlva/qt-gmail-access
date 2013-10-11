/*
maprequest.h: Declaration of IF-MAP Request classes

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

#ifndef MAPREQUEST_H
#define MAPREQUEST_H

#include <QList>
#include <QString>
#include "identifier.h"
#include "metadata.h"
#include "omapdconfig.h"

static QString IFMAP_NS_2 = "http://www.trustedcomputinggroup.org/2010/IFMAP/2";
static QString IFMAP_META_NS_2 = "http://www.trustedcomputinggroup.org/2010/IFMAP-METADATA/2";
static QString IFMAP_NS_1 = "http://www.trustedcomputinggroup.org/2006/IFMAP/1";
static QString IFMAP_META_NS_1 = "http://www.trustedcomputinggroup.org/2006/IFMAP-METADATA/1";

#define IFMAP_MAX_SIZE 100000;
#define IFMAP_MAX_DEPTH_MAX 10000;

#define MAXPOLLRESULTSIZEDEFAULT    5000000

class MapRequest
{
public:
    enum RequestVersion {
        VersionNone = 0,
        IFMAPv11,
        IFMAPv20
    };

    enum RequestType {
        RequestNone = 0,
        NewSession,
        AttachSession,
        EndSession,
        RenewSession,
        Publish,
        Subscribe,
        Search,
        PurgePublisher,
        Poll
    };

    enum ValidationType {
        ValidationNone = 0,
        ValidationBaseOnly,
        ValidationMetadataOnly,
        ValidationAll
    };

    enum RequestError {
        ErrorNone = 0,
        IfmapClientSoapFault,
        IfmapAccessDenied,
        IfmapFailure, // Unspecified failure
        IfmapInvalidIdentifier,
        IfmapInvalidIdentifierType,
        IfmapIdentifierTooLong,
        IfmapInvalidMetadata,
        IfmapInvalidMetadataListType,
        IfmapInvalidSchemaVersion,
        IfmapInvalidSessionID,
        IfmapMetadataTooLong,
        IfmapSearchResultsTooBig,
        IfmapPollResultsTooBig,
        IfmapSystemError // Server error
    };

    enum AuthenticationType {
        AuthNone = 0,
        AuthAllowNone,
        AuthBasic,
        AuthCert,
        AuthCACert
    };
    
    MapRequest(MapRequest::RequestType requestType = MapRequest::RequestNone);
    MapRequest(const MapRequest&);
    ~MapRequest() {;}

    static QString requestTypeString(MapRequest::RequestType reqType);
    static QString requestVersionString(MapRequest::RequestVersion version);
    static QString requestVersionNamespace(MapRequest::RequestVersion version);
    static QString requestErrorString(MapRequest::RequestError error);

    MapRequest::RequestError requestError() const { return _requestError; }
    MapRequest::RequestVersion requestVersion() const { return _requestVersion; }
    MapRequest::RequestType requestType() const { return _requestType; }
    const QString& sessionId() const { return _sessionId; }
    bool clientSetSessionId() const { return _clientSetSessionId; }
    MapRequest::AuthenticationType authType() const { return _authType; }
    const QString& authToken() const { return _authValue; }

    void setRequestError(MapRequest::RequestError requestError) { _requestError = requestError; }
    void setRequestType(MapRequest::RequestType requestType) { _requestType = requestType; }
    void setRequestVersion(MapRequest::RequestVersion requestVersion) { _requestVersion = requestVersion; }
    void setSessionId(const QString& sessionId) { _sessionId = sessionId; }
    void setClientSetSessionId(bool set) { _clientSetSessionId = set; }
    void setAuthType(MapRequest::AuthenticationType type) { _authType = type; }
    void setAuthValue(const QString& value) { _authValue = value; }

protected:
    MapRequest::RequestError _requestError;
    MapRequest::RequestVersion _requestVersion;
    MapRequest::RequestType _requestType;
    QString _sessionId;
    bool _clientSetSessionId;
    MapRequest::AuthenticationType _authType;
    QString _authValue;
};

class NewSessionRequest : public MapRequest
{
public:
    NewSessionRequest(int maxPollResultSize = MAXPOLLRESULTSIZEDEFAULT);
    NewSessionRequest(const NewSessionRequest&);
    ~NewSessionRequest() {;}

    unsigned int maxPollResultSize() const { return _maxPollResultSize; }
    bool clientSetMaxPollResultSize() const { return _clientSetMaxPollResultSize; }
    void setMaxPollResultSize(unsigned int mprs) { _maxPollResultSize = mprs; }
    void setClientSetMaxPollResultSize(bool set) { _clientSetMaxPollResultSize = set; }
private:
    unsigned int _maxPollResultSize;
    bool _clientSetMaxPollResultSize;
};
Q_DECLARE_METATYPE(NewSessionRequest)

class AttachSessionRequest : public MapRequest
{
public:
    AttachSessionRequest();
    AttachSessionRequest(const AttachSessionRequest&);
    ~AttachSessionRequest() {;}
};
Q_DECLARE_METATYPE(AttachSessionRequest)

class EndSessionRequest : public MapRequest
{
public:
    EndSessionRequest();
    EndSessionRequest(const EndSessionRequest&);
    ~EndSessionRequest() {;}
    const EndSessionRequest& operator= (const EndSessionRequest& rhs);
};
Q_DECLARE_METATYPE(EndSessionRequest)

class RenewSessionRequest : public MapRequest
{
public:
    RenewSessionRequest();
    RenewSessionRequest(const RenewSessionRequest&);
    ~RenewSessionRequest() {;}
};
Q_DECLARE_METATYPE(RenewSessionRequest)

class PurgePublisherRequest : public MapRequest
{
public:
    PurgePublisherRequest();
    PurgePublisherRequest(const PurgePublisherRequest&);
    ~PurgePublisherRequest() {;}
    // FIXME LFu: this seems wrong (const return value)
    const PurgePublisherRequest& operator= (const PurgePublisherRequest& rhs);

    const QString& publisherId() const { return _publisherId; }
    bool clientSetPublisherId() const { return _clientSetPublisherId; }
    void setPublisherId(const QString& pubId) { _publisherId = pubId; }
    void setClientSetPublisherId(bool set) { _clientSetPublisherId = set; }
private:
    QString _publisherId;
    bool _clientSetPublisherId;
};
Q_DECLARE_METATYPE(PurgePublisherRequest)

class PublishOperation
{
public:
    enum PublishType {
        None = 0,
        Update,
        Notify,
        Delete
    };

    PublishOperation();
    ~PublishOperation();

    PublishOperation::PublishType _publishType;
    Link _link;
    bool _isLink;
    QList<Meta> _metadata;

    Meta::Lifetime _lifetime;
    bool _clientSetLifetime;

    QString _deleteFilter;
    bool _clientSetDeleteFilter;
    QMap<QString, QString> _filterNamespaceDefinitions;

    int _operationNumber;
};

class PublishRequest : public MapRequest
{
public:
    PublishRequest();
    PublishRequest(const PublishRequest&);
    ~PublishRequest() { _publishOperations.clear();}

    const QString& publisherId() const { return _publisherId; }
    void setPublisherId(const QString& pubId) { _publisherId = pubId; }
    void setPublishOperations(const QList<PublishOperation>& operations) { _publishOperations = operations; }
    const QList<PublishOperation>& publishOperations() const { return _publishOperations; }
    void addPublishOperation(const PublishOperation& pubOper) { _publishOperations.append(pubOper); }
private:
    QList<PublishOperation> _publishOperations;
    QString _publisherId;
};
Q_DECLARE_METATYPE(PublishRequest)

typedef QList<QPair<QString, QString> > SimplifiedFilter;

class MetadataFilter
{
public:
    MetadataFilter(const QString& filterStr);
    MetadataFilter(const MetadataFilter& other);
    ~MetadataFilter();

    MetadataFilter& operator=(const MetadataFilter& other);
    bool operator==(const MetadataFilter& other) const;
    bool operator==(const QString& filterStr) const;

    bool isEmpty() const { return _filter.isEmpty(); }
    bool isSimple() const { return _simFilter != NULL; }
    const QString& str() const { return _filter; }

    const SimplifiedFilter* getSimplifiedFilter() const { return _simFilter; }
    void setFilter(const QString& ifmapFilter);


private:
    QString _filter;
    SimplifiedFilter* _simFilter;
};

class SearchType
{
public:
    SearchType();
    int maxDepth() const { return _maxDepth; }
    int maxSize() const { return _maxSize; }
    const MetadataFilter& resultFilter() const { return _resultFilter; }
    const MetadataFilter& matchLinks() const { return _matchLinks; }
    const QString& terminalId() const { return _terminalId; }
    const Id& startId() const { return _startId; }
    const QMap<QString, QString>& filterNamespaceDefinitions() const { return _filterNamespaceDefinitions; }
    bool clientSetMaxDepth() const { return _clientSetMaxDepth; }
    bool clientSetMaxSize() const { return _clientSetMaxSize; }
    bool clientSetResultFilter() const { return _clientSetResultFilter; }
    bool clientSetMatchLinks() const { return _clientSetMatchLinks; }
    bool clientSetTerminalId() const { return _clientSetTerminalId; }

    void setMaxDepth(int maxDepth) { _maxDepth = maxDepth; _clientSetMaxDepth = true; }
    void setMaxSize(int maxSize) { _maxSize = maxSize; _clientSetMaxSize = true; }
    void setResultFilter(const QString& resultFilter) { _resultFilter.setFilter(resultFilter); _clientSetResultFilter = true; }
    void setMatchLinks(const QString& matchLinks) { _matchLinks.setFilter(matchLinks); _clientSetMatchLinks = true; }
    void setTerminalId(const QString& terminalId) { _terminalId = terminalId; _clientSetTerminalId = true; }
    void setStartId(Id id) { _startId = id; }
    void setFilterNamespaceDefinitions(const QMap<QString,QString>& nsDefs) {_filterNamespaceDefinitions = nsDefs; }
protected:
    int _maxDepth;
    bool _clientSetMaxDepth;
    int _maxSize;
    bool _clientSetMaxSize;
    MetadataFilter _resultFilter;
    bool _clientSetResultFilter;
    MetadataFilter _matchLinks;
    bool _clientSetMatchLinks;
    QString _terminalId;
    bool _clientSetTerminalId;
    Id _startId;
    QMap<QString, QString> _filterNamespaceDefinitions;
};

class SubscribeOperation
{
public:
    enum SubscribeType {
        Update,
        Delete
    };

    SubscribeOperation();

    void setName(const QString& name) { _name = name; }
    const QString& name() const { return _name; }

    SubscribeOperation::SubscribeType subscribeType() const { return _subscribeType; }
    void setSubscribeType(SubscribeOperation::SubscribeType subType) { _subscribeType = subType; }

    const SearchType& search() const { return _search; }
    void setSearch(const SearchType& search) { _search = search; }
private:
    QString _name;
    SubscribeOperation::SubscribeType _subscribeType;
    SearchType _search;
};

class SearchRequest : public MapRequest
{
public:
    SearchRequest();
    SearchRequest(const SearchRequest&);
    ~SearchRequest() {;}

    const SearchType& search() const { return _search; }
    void setSearch(const SearchType& search) { _search = search; }
private:
    SearchType _search;
};
Q_DECLARE_METATYPE(SearchRequest)

class SubscribeRequest : public MapRequest
{
public:
    SubscribeRequest();
    SubscribeRequest(const SubscribeRequest&);
    ~SubscribeRequest() { _subscribeOperations.clear();}

    QList<SubscribeOperation> subscribeOperations() const { return _subscribeOperations; }
    void addSubscribeOperation(const SubscribeOperation& subOper) { _subscribeOperations.append(subOper); }
private:
    QList<SubscribeOperation> _subscribeOperations;
};
Q_DECLARE_METATYPE(SubscribeRequest)

class PollRequest : public MapRequest
{
public:
    PollRequest();
    PollRequest(const PollRequest&);
    ~PollRequest() {;}
};
Q_DECLARE_METATYPE(PollRequest)

#endif // MAPREQUEST_H
