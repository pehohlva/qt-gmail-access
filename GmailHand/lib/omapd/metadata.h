/*
metadata.h: Declaration of Meta Class

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

#ifndef METADATA_H
#define METADATA_H

#include <QtCore>
#include "omapdconfig.h"

class Meta
{
public:
    enum Cardinality {
                SingleValue = 1,
                MultiValue
            };

    enum Lifetime {
                LifetimeSession = 1,
                LifetimeForever
    };

    enum PublishOperationType {
                PublishUpdate = 1,
                PublishNotify,
                PublishDelete
    };

    Meta(Meta::Cardinality cardinality = Meta::SingleValue,
         Meta::Lifetime lifetime = Meta::LifetimeSession);

    Meta::Cardinality cardinality() const { return _cardinality; }
    Meta::Lifetime lifetime() const { return _lifetime; }
    QString lifetimeString() const;

    const QString& publisherId() const { return _publisherId; }
    const QString& elementName() const { return _elementName; }
    const QString& elementNS() const { return _elementNS; }
    const QString& metaXML() const { return _metaXML; }

    void setLifetime(Meta::Lifetime lifetime) { _lifetime = lifetime; }
    void setPublisherId(const QString& pubId) { _publisherId = pubId; }
    void setElementName(const QString& elementName) { _elementName = elementName; }
    void setElementNS(const QString& elementNS) { _elementNS = elementNS; }
    void setMetaXML(const QString& metaXML) {_metaXML = metaXML; }

    // Two Meta objects are equal iff their elementName and namespace members are the same
    bool operator==(const Meta &other) const;

private:
    Meta::Cardinality _cardinality;
    Meta::Lifetime _lifetime;
    QString _publisherId;
    QString _elementName;
    QString _elementNS;
    QString _metaXML;
};

QDebug operator<<(QDebug dbg, const Meta & meta);

#endif // METADATA_H
