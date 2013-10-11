/*
subscription.cpp: Implementation of SearchResult and Subscribtion Classes

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

#include "subscription.h"
#include "mapsessions.h"
#include <QRegExp>

SearchResult::ResultType SearchResult::resultTypeForPublishType(Meta::PublishOperationType publishType)
{
    SearchResult::ResultType resultType;
    switch (publishType) {
    case Meta::PublishUpdate:
        resultType = SearchResult::UpdateResultType;
        break;
    case Meta::PublishDelete:
        resultType = SearchResult::DeleteResultType;
        break;
    case Meta::PublishNotify:
        resultType = SearchResult::UpdateResultType;
        break;
    }
    return resultType;
}

SearchResult::SearchResult(SearchResult::ResultType type, SearchResult::ResultScope scope)
    : _resultType(type), _resultScope(scope)
{
}

SearchResult::~SearchResult()
{
}

Subscription::Subscription(MapRequest::RequestVersion requestVersion)
    : _requestVersion(requestVersion)
{
    _sentFirstResult = false;
    _curSize = 0;
    _subscriptionError = MapRequest::ErrorNone;
    _indexed = true;
}

Subscription::~Subscription()
{
    // TODO: Do I need to clearSearchResults() to avoid leaking memory?

    // (LFu) You bet:
    clearSearchResults();

    // also remove this Subscription from the index
    if(_indexed)
    {
        MapSessions* ms = MapSessions::getInstance();
        QMapIterator<Id, int> mapIt(_ids);
        while(mapIt.hasNext())
        {
            ms->removeFromIndex(mapIt.next().key(), this);
        }
    }
}

QSet<Id> Subscription::identifiers() const
{
    QSet<Id> resultSet;

    QMapIterator<Id, int> mapIt(_ids);
    while(mapIt.hasNext())
    {
        mapIt.next();
        resultSet << mapIt.key();
    }

    return resultSet;
}

QSet<Id> Subscription::identifiersWithout(const QMap<Id, int>& others) const
{
    QSet<Id> resultSet;
    QMapIterator<Id, int> mapIt(_ids);
    while(mapIt.hasNext())
    {
        mapIt.next();
        if(!others.contains(mapIt.key()))
            resultSet << mapIt.key();
    }

    return resultSet;
}

QSet<Id> Subscription::subtractFrom(const QMap<Id, int>& others) const
{
    QSet<Id> resultSet;
    QMapIterator<Id, int> mapIt(others);
    while(mapIt.hasNext())
    {
        mapIt.next();
        if(!_ids.contains(mapIt.key()))
            resultSet << mapIt.key();
    }

    return resultSet;
}

unsigned int Subscription::linksContaining(const Id& id) const
{
    unsigned int result = 0;
    QSetIterator<Link> it(_linkList);
    while(it.hasNext())
    {
        const Link& link = it.next();
        if(link.first == id || link.second == id)
            result++;
    }

    return result;
}

void Subscription::clearSearchResults()
{
    while (! _searchResults.isEmpty()) {
        delete _searchResults.takeFirst();
    }
    while (! _deltaResults.isEmpty()) {
        delete _deltaResults.takeFirst();
    }

    _curSize = 0;
}

bool Subscription::operator==(const Subscription &other) const
{
    // Two SearchGraphs are equal iff their names are equal
    if (this->_name == other._name)
        return true;
    else
        return false;
}

// (LFu) this code is now in MetadataFilter::setFilter()
//QString Subscription::translateFilter(const QString& ifmapFilter, SimplifiedFilter& simFilter, bool& canSimplifyMatching)
//{
//    //const char *fnName = "SearchGraph::translateFilter:";

//    /* non-predicate expressions joined by "or" need to be translated
//       into a parenthesized expression separated by "|".

//       Examples:
//       meta:ip-mac or scada:node
//       --> (meta:ip-mac | scada:node)

//       meta:role[@publisher-id = "myPubId" or name="myRole"] or meta:ip-mac
//       --> (meta:role[@publisher-id = "myPubId" or name="myRole"] | meta:ip-mac)

//       standard XPath does not support predicate expressions that begin with [; need to add *
//    */

//    // (LFu) - determine if the filter looks like "ns-1:elem-1 or ... or ns-n:elem-n"
//    // and provide simplified metadata matching in this case:
//    QRegExp rx("((\\w|-)+:(\\w|-)+)(\\s+(or|OR)\\s+((\\w|-)+:(\\w|-)+))*");
//    if(rx.exactMatch(ifmapFilter))
//    {
//        canSimplifyMatching = true;
//        qDebug() << "~~regex matches filter: " << ifmapFilter;
//        int pos = 0;
//        QRegExp rx2("((\\w|-)+):((\\w|-)+)(\\s+(or|OR))?");
//        while ((pos = rx2.indexIn(ifmapFilter, pos)) != -1) {
//            simFilter.append(QPair<QString, QString>(rx2.cap(1), rx2.cap(3)));
//            qDebug() << "~~\tns:" << rx2.cap(1) << " elem:" << rx2.cap(3);
//            pos += rx2.matchedLength();
//        }
//    }
//    else
//        qDebug() << "~~regex does not match filter: " << ifmapFilter;


//    // TODO: Do this with QRegExp
//    QString qtFilter = ifmapFilter;
//    if (ifmapFilter.contains(" or ", Qt::CaseInsensitive)) {
//        //qDebug() << fnName << "WARNING! filter translation is woefully incomplete!";
//        //qDebug() << fnName << "filter before translation:" << ifmapFilter;
//        qtFilter.replace(" or "," | ");
//        qtFilter.prepend("(");
//        qtFilter.append(")");
//        //qDebug() << fnName << "filter after translation:" << qtFilter;
//    }

//    return qtFilter;
//}

// (LFu) unused:
//QString Subscription::intersectFilter(const QString& matchLinksFilter, const QString& resultFilter)
//{
//    /* This method creates an intersect filter for XPath
//       as the logical AND combination of the match-links
//       filter and the result-filter.

//       The passed-in filters MUST first individually be translated
//       using SearchGraph::translateFilter().
//    */
//    QString qtFilter = "(";
//    qtFilter += matchLinksFilter;
//    qtFilter += " intersect ";
//    qtFilter += resultFilter;
//    qtFilter += ")";

//    return qtFilter;
//}

// (LFu) unused:
//QStringList Subscription::filterPrefixes(const QString& filter)
//{
//    // TODO: Improve RegExp to not include colons inside quotes
//    // For example: vend:ike-policy[@gateway=1.2.3.4 and meta:phase1/@identity=name:joe]
//    // should not capture the `name' prefix.  However, it's only a slight performance hit
//    // to capture these false prefixes, because they won't map to a declared namespace
//    // in the document, or if they do, that's ok too.
//    // Here's a possibility for a RegExp that excludes colons inside quotes, but not quite
//    //QRegExp rx("([^\"{0}]\\b\\w+:)");

//    // Look for a word boundary followed by 1 or more word characters up to a colon
//    QRegExp rx("(\\b\\w+:)");
//    QStringList prefixes;

//    int pos = 0;
//    while ((pos = rx.indexIn(filter, pos)) != -1) {
//        QString prefix = rx.cap(1);
//        prefixes << prefix.left(prefix.length()-1);
//        pos += rx.matchedLength();
//    }
//    //qDebug() << "prefixes:" << prefixes.join("|");

//    return prefixes;
//}
