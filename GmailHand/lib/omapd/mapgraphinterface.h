#ifndef MAPGRAPHINTERFACE_H
#define MAPGRAPHINTERFACE_H

#include "identifier.h"
#include "metadata.h"
#include "maprequest.h"

class MapGraphInterface
{
public:
    virtual ~MapGraphInterface() {}

    virtual void dumpMap() = 0;
    virtual void clearMap() = 0;

    virtual void addMeta(Link key, bool isLink, QList<Meta> publisherMeta, QString publisherId) = 0;
    virtual bool deleteMetaWithPublisherId(QString pubId, QHash<Id, QList<Meta> > *idMetaDeleted, QHash<Link, QList<Meta> > *linkMetaDeleted, bool sessionMetaOnly = false) = 0;
    virtual void replaceMeta(Link link, bool isLink, QList<Meta> newMetaList = QList<Meta>()) = 0;

    // List of all identifiers that targetId is on a link with
    virtual QList<Id> linksTo(Id targetId) = 0;

    virtual QList<Meta> metaForLink(Link link) = 0;
    virtual QList<Meta> metaForId(Id id) = 0;

    virtual void setDebug(bool debug) = 0;
};

Q_DECLARE_INTERFACE(MapGraphInterface, "org.omapd.Plugin.MapGraphInterface/1.0");

#endif // MAPGRAPHINTERFACE_H

