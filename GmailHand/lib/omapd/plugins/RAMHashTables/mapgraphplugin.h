/*
mapgraphplugin.h: Declaration of MapGraphPlugin class

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

#ifndef MAPGRAPHPLUGIN_H
#define MAPGRAPHPLUGIN_H

#include <QtCore>

#include "mapgraphinterface.h"

class MapGraphPlugin : public QObject, MapGraphInterface {
    Q_OBJECT
    Q_INTERFACES(MapGraphInterface)

public:
    void dumpMap();
    void clearMap();

    void addMeta(Link key, bool isLink, QList<Meta> publisherMeta, QString publisherId);
    bool deleteMetaWithPublisherId(QString pubId, QHash<Id, QList<Meta> > *idMetaDeleted, QHash<Link, QList<Meta> > *linkMetaDeleted, bool sessionMetaOnly = false);
    void replaceMeta(Link link, bool isLink, QList<Meta> newMetaList = QList<Meta>());

    // List of all identifiers that targetId is on a link with
    QList<Id> linksTo(Id targetId);

    QList<Meta> metaForLink(Link link);
    QList<Meta> metaForId(Id id);

    void setDebug(bool debug) { _debug = debug; }

private:
    QHash<Id, QList<Meta> > _idMeta; // Id --> all metadata on Id
    QHash<Link, QList<Meta> > _linkMeta;  // Link --> all metadata on Link
    QMultiHash<Id, Id> _linksTo;  // id1 --> id2 and id2 --> id1
    QMultiHash<QString, Id> _publisherIds;  // publisherId --> Id (useful for purgePublisher)
    QMultiHash<QString, Link> _publisherLinks;  // publisherId --> Link (useful for purgePublisher)

    bool _debug;
};

#endif // MAPGRAPHPLUGIN_H
