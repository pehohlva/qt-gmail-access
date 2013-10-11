/*
metadata.cpp: Implementation of Meta Class

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

#include "metadata.h"

Meta::Meta(Meta::Cardinality cardinality, Meta::Lifetime lifetime)
        : _cardinality(cardinality), _lifetime(lifetime)
{
}

QString Meta::lifetimeString() const
{
    QString str("");
    switch (_lifetime) {
    case Meta::LifetimeSession:
        str = "session";
        break;
    case Meta::LifetimeForever:
        str = "forever";
        break;
    }

    return str;
}

// Two Meta objects are equal iff their elementName and namespace members are the same
bool Meta::operator ==(const Meta &other) const
{
    if (this->_elementName == other._elementName &&
        this->_elementNS == other._elementNS)
        return true;
    else
        return false;
}

QDebug operator<<(QDebug dbg, const Meta & meta)
{
    dbg.nospace() << "Metadata XML:" << meta.metaXML();
    return dbg.space();
}
