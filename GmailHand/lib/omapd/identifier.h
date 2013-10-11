/*
identifier.h: Definition of Identifier class

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

#ifndef IDENTIFIER_H
#define IDENTIFIER_H

#include <QtCore>

#include "omapdconfig.h"

class Identifier;
typedef Identifier Id;
typedef QPair<Identifier, Identifier> Link;

class Identifier
{
public:
    enum IdType {
        IdNone = 0,
        AccessRequest,
        DeviceAikName,
        DeviceName,
        IdentityAikName,
        IdentityDistinguishedName,
        IdentityDnsName,
        IdentityEmailAddress,
        IdentityKerberosPrincipal,
        IdentityTrustedPlatformModule,
        IdentityUsername,
        IdentitySipUri,
        IdentityHipHit,
        IdentityTelUri,
        IdentityOther,
        IpAddressIPv4,
        IpAddressIPv6,
        MacAddress
    };

    Identifier(Identifier::IdType type = Identifier::IdNone);
    Identifier(Identifier::IdType type, const QString& value, const QString& ad = QString(), const QString& other = QString());

    static QString idStringForType(Identifier::IdType idType);
    static QString idBaseStringForType(Identifier::IdType idType);
    static Link makeLinkFromIds(const Id& id1, const Id& id2);
    static Id otherIdForLink(const Link& link, const Id& targetId);

    // Two Identifier objects are equal iff their type, namespace, value, and other members are the same
    bool operator==(const Identifier &other) const;
    bool operator< (const Id& other) const { return hashString() < other.hashString(); }
    //QDataStream & operator<< ( QDataStream & stream, const Identifier & id );

    Identifier::IdType type() const { return _type; }
    const QString& value() const { return _value; }
    const QString& ad() const { return _ad; }
    const QString& other() const { return _other; }

    QString hashString() const;

    void setType(Identifier::IdType type) { _type = type; }
    void setValue(const QString& value) { _value = value; }
    void setAd(const QString& ad) { _ad = ad; }
    void setOther(const QString& other) { _other = other; }

private:
    Identifier::IdType _type;
    QString _value;
    QString _ad;
    QString _other;
};

QDebug operator<<(QDebug dbg, const Identifier & id);
uint qHash ( const Identifier & key );

#endif // IDENTIFIER_H
