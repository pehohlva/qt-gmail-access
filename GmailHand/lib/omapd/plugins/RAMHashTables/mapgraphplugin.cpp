/*
mapgraphplugin.cpp: Implementation of MapGraphPlugin class

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

#include "mapgraphplugin.h"

void MapGraphPlugin::addMeta(Link link, bool isLink, QList<Meta> publisherMeta, QString publisherId)
{
    const char *fnName = "MapGraphPlugin::addMeta:";

    if (_debug)
        qDebug() << fnName << "number of metadata objects:" << publisherMeta.size();

    if (isLink) {
        if (_debug)
            qDebug() << fnName << "link:" << link;
    } else  {
        if (_debug)
            qDebug() << fnName << "id:" << link.first;
    }

    while (! publisherMeta.isEmpty()) {
        // All Metadata currently on identifier/link
        QList<Meta> existingMetaList;
        if (isLink) {
             existingMetaList = _linkMeta.take(link);
        } else {
             existingMetaList = _idMeta.take(link.first);
        }

        Meta newMeta = publisherMeta.takeFirst();
        // This matches metadata with same element name and element namespace
        int existingMetaIndex = existingMetaList.indexOf(newMeta);
        if (existingMetaIndex != -1) {
            if (newMeta.cardinality() == Meta::SingleValue) {
                // replace
                if (_debug)
                    qDebug() << fnName << "Replacing singleValue meta:" << newMeta.elementName();
                existingMetaList.replace(existingMetaIndex, newMeta);
            } else {
                // add to list
                if (_debug)
                    qDebug() << fnName << "Appending multiValue meta:" << newMeta.elementName();
                existingMetaList << newMeta;
            }
        } else {
            // no existing metadata of this type so add to list regardless of cardinality
            if (_debug)
                qDebug() << fnName << "Adding meta:" << newMeta.elementName();
            existingMetaList << newMeta;
        }

        if (isLink) {
            // Place updated metadata back on link
            _linkMeta.insert(link, existingMetaList);
        } else {
            // Place updated metadata back on identifier
            _idMeta.insert(link.first, existingMetaList);
        }
    }

    if (isLink) {
        // Update lists of identifier linkages
        if (! _linksTo.contains(link.first, link.second)) {
            _linksTo.insert(link.first, link.second);
            _linksTo.insert(link.second, link.first);
        }

        // Track links published by this publisherId
        if (! _publisherLinks.contains(publisherId, link)) {
            _publisherLinks.insert(publisherId, link);
        }

        if (_debug)
            qDebug() << fnName << "_linkMeta has size:" << _linkMeta.size();
    } else {
        // Track identifiers published by this publisherId
        if (! _publisherIds.contains(publisherId, link.first)) {
            _publisherIds.insert(publisherId, link.first);
        }

        if (_debug)
            qDebug() << fnName << "_idMeta has size:" << _idMeta.size();
    }
}

void MapGraphPlugin::replaceMeta(Link link, bool isLink, QList<Meta> newMetaList)
{
    //const char *fnName = "MapGraphPlugin::replaceMeta:";

    /*
    TODO: A clear performance increase can be made by improving on the scorched
    earth procedures in this method: removing all the metadata, link pointers,
    publisher-id pointers, and then adding the keepers back on.
    */

    if (isLink) {
        // Remove metadata on link
        _linkMeta.remove(link);

        // Remove entries from _linksTo
        _linksTo.remove(link.first, link.second);
        _linksTo.remove(link.second, link.first);

        // Remove all publisherIds that published on this link
        QList<QString> pubIdsOnLink = _publisherLinks.keys(link);
        QListIterator<QString> pubIt(pubIdsOnLink);
        while (pubIt.hasNext()) {
            QString pubId = pubIt.next();
            _publisherLinks.remove(pubId, link);
        }

    } else {
        // Remove metadata on identifier
        _idMeta.remove(link.first);

        // Remove all publisherIds that published on this identifier
        QList<QString> pubIdsOnId = _publisherIds.keys(link.first);
        QListIterator<QString> pubIt(pubIdsOnId);
        while (pubIt.hasNext()) {
            QString pubId = pubIt.next();
            _publisherIds.remove(pubId, link.first);
        }
    }

    if (! newMetaList.isEmpty()) {
        if (isLink) {
            // Place updated metadata back on link
            _linkMeta.insert(link, newMetaList);

            // Update lists of identifier linkages
            if (! _linksTo.contains(link.first, link.second)) {
                _linksTo.insert(link.first, link.second);
                _linksTo.insert(link.second, link.first);
            }

            QListIterator<Meta> metaIt(newMetaList);
            while (metaIt.hasNext()) {
                QString pubId = metaIt.next().publisherId();
                // Track links published by this publisherId
                if (! _publisherLinks.contains(pubId, link)) _publisherLinks.insert(pubId, link);
            }
        } else {
            // Place updated metadata back on identifier
            _idMeta.insert(link.first, newMetaList);

            QListIterator<Meta> metaIt(newMetaList);
            while (metaIt.hasNext()) {
                QString pubId = metaIt.next().publisherId();
                // Track identifiers published by this publisherId
                if (! _publisherIds.contains(pubId, link.first)) _publisherIds.insert(pubId, link.first);
            }
        }
    }
}

bool MapGraphPlugin::deleteMetaWithPublisherId(QString pubId, QHash<Id, QList<Meta> > *idMetaDeleted, QHash<Link, QList<Meta> > *linkMetaDeleted, bool sessionMetaOnly)
{
    const char *fnName = "MapGraphPlugin::deleteMetaWithPublisherId:";
    bool somethingDeleted = false;

    // Delete publisher's metadata on identifiers
    QList<Id> idsWithPub = _publisherIds.values(pubId);
    if (_debug)
        qDebug() << fnName << "have publisherId on num ids:" << idsWithPub.size();
    QListIterator<Id> idIt(idsWithPub);
    while (idIt.hasNext()) {
        Id idPub = idIt.next();
        QList<Meta> deletedMetaList;
        bool publisherHasMetaOnId = false;

        // Remove metadata on this identifier -- by definition this list in non-empty
        QList<Meta> metaOnId = _idMeta.take(idPub);
        if (_debug)
            qDebug() << fnName << "Examining metadata (" << metaOnId.size() << ") on id:" << idPub;
        for (int metaIndex = metaOnId.size()-1; metaIndex >= 0; metaIndex--) {
            QString testPubId = metaOnId.at(metaIndex).publisherId();

            if (pubId.compare(testPubId) == 0) {
                if (sessionMetaOnly) {
                    // Only delete session-level metadata
                    if (metaOnId.at(metaIndex).lifetime() == Meta::LifetimeSession) {
                        if (_debug)
                            qDebug() << fnName << "Removed session identifier Meta:" << metaOnId.at(metaIndex).elementName();
                        deletedMetaList << metaOnId.takeAt(metaIndex);
                    } else {
                        // Not removing metadata for this publisher with lifetime=forever
                        publisherHasMetaOnId = true;
                    }
                } else {
                    // Delete metadata regardless of lifetime
                    if (_debug)
                        qDebug() << fnName << "Removed identifier Meta:" << metaOnId.at(metaIndex).elementName();
                    deletedMetaList << metaOnId.takeAt(metaIndex);
                }
            }
        }

        // Keep track of deleted metadata
        if (! deletedMetaList.isEmpty()) {
            idMetaDeleted->insert(idPub, deletedMetaList);
            somethingDeleted = true;
        }

        // Replace remaining metadata on table of identifiers <--> metadata
        if (! metaOnId.isEmpty()) {
            _idMeta.insert(idPub, metaOnId);
        }

        // Update table of publisher <--> identifiers
        if (! publisherHasMetaOnId) {
            _publisherIds.remove(pubId, idPub);
        }
    }

    // Delete publisher's metadata on links
    QList<Link> linksWithPub = _publisherLinks.values(pubId);
    if (_debug)
        qDebug() << fnName << "have publisherId on num links:" << linksWithPub.size();
    bool linkDeleted = false;
    QListIterator<Link> linkIt(linksWithPub);
    while (linkIt.hasNext() && !linkDeleted) {
        Link linkPub = linkIt.next();
        QList<Meta> deletedMetaList;
        bool publisherHasMetaOnLink = false;

        // Remove metadata on this link -- by definition this list is non-empty
        QList<Meta> metaOnLink = _linkMeta.take(linkPub);
        if (_debug)
            qDebug() << fnName << "Examining publisher metadata (" << metaOnLink.size() << ") on link:" << linkPub;
        for (int metaIndex = metaOnLink.size()-1; metaIndex >= 0; metaIndex--) {
            QString testPubId = metaOnLink.at(metaIndex).publisherId();

            if (pubId.compare(testPubId) == 0) {
                if (sessionMetaOnly) {
                    // Only delete session-level metadata
                    if (metaOnLink.at(metaIndex).lifetime() == Meta::LifetimeSession) {
                        if (_debug)
                            qDebug() << fnName << "Removed session link Meta:" << metaOnLink.at(metaIndex).elementName();
                        deletedMetaList << metaOnLink.takeAt(metaIndex);
                    } else {
                        // Not removing metadata for this publisher with lifetime=forever
                        publisherHasMetaOnLink = true;
                    }
                } else {
                    // Delete metadata regardless of lifetime
                    if (_debug)
                        qDebug() << fnName << "Removed link Meta:" << metaOnLink.at(metaIndex).elementName();
                    deletedMetaList << metaOnLink.takeAt(metaIndex);
                }
            }
        }

        // Keep track of deleted metadata
        if (! deletedMetaList.isEmpty()) {
            linkMetaDeleted->insert(linkPub, deletedMetaList);
            somethingDeleted = true;
            linkDeleted = true;
        }

        if (metaOnLink.isEmpty()) {
            // Update list of identifier links
            _linksTo.remove(linkPub.first, linkPub.second);
            _linksTo.remove(linkPub.second, linkPub.first);
        } else {
            // Replace remaining metadata table of links <--> metadata
            _linkMeta.insert(linkPub, metaOnLink);
        }

        // Update table of publisher <--> links
        if (! publisherHasMetaOnLink) {
            _publisherLinks.remove(pubId, linkPub);
        }
    }

    return somethingDeleted;
}

void MapGraphPlugin::dumpMap()
{
    const char *fnName = "MapGraphPlugin::dumpMap:";
    qDebug() << fnName << "---------------------- start dump -------------------" << endl;

    QString idString;
    QTextStream idStream(&idString);

    qDebug() << fnName << "_idMeta has metadata on num identifiers:" << _idMeta.size();
    QHash<Id, QList<Meta> >::const_iterator i;
    for (i = _idMeta.constBegin(); i != _idMeta.constEnd(); ++i) {
        Id id = i.key();
        QList<Meta> metaList = i.value();
        qDebug() << fnName << "Identifier metadata of length " << metaList.size() << endl << id;
        QListIterator<Meta> it(metaList);
        while (it.hasNext()) {
            Meta meta = it.next();
            idStream << meta.metaXML();
            QString metaXML = idStream.readAll();
            qDebug() << meta.lifetimeString() << "--->" <<  metaXML << endl;
        }
    }

    qDebug() << fnName << "_linkMeta has metadata on num links:" << _linkMeta.size();
    QHash<Link, QList<Meta> >::const_iterator lm;
    for (lm = _linkMeta.constBegin(); lm != _linkMeta.constEnd(); ++lm) {
        Link link = lm.key();
        QList<Meta> metaList = lm.value();
        qDebug() << fnName << "Link metadata of length" << metaList.size() << endl << link;
        QListIterator<Meta> it(metaList);
        while (it.hasNext()) {
            Meta meta = it.next();
            idStream << meta.metaXML();
            QString metaXML = idStream.readAll();
            qDebug() << meta.lifetimeString() << "--->" <<  metaXML << endl;
        }
    }

    qDebug() << fnName << "---------------------- end dump -------------------" << endl;
}

void MapGraphPlugin::clearMap()
{
    const char *fnName = "MapGraphPlugin::clearMap:";
    qDebug() << fnName << "WARNING: clearing entire MAP contents!";
    _idMeta.clear();
    _linkMeta.clear();
    _linksTo.clear();
    _publisherIds.clear();
    _publisherLinks.clear();
}

QList<Id> MapGraphPlugin::linksTo(Id targetId) {
    return _linksTo.values(targetId);
}

QList<Meta> MapGraphPlugin::metaForLink(Link link) {
    return _linkMeta.value(link);
}

QList<Meta> MapGraphPlugin::metaForId(Id id) {
    return _idMeta.value(id);
}

Q_EXPORT_PLUGIN2(RAMHashTables, MapGraphPlugin)
