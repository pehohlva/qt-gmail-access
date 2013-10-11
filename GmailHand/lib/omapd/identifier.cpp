/*
identifier.cpp: Implementation of Identifier class

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

#include "identifier.h"

QString Identifier::idStringForType(Identifier::IdType idType)
{
    QString str("");
    switch (idType) {
        case Identifier::IdNone:
            break;
        case Identifier::AccessRequest:
            str = "AccessRequest";
            break;
        case Identifier::DeviceAikName:
            str = "DeviceAikName";
            break;
        case Identifier::DeviceName:
            str = "DeviceName";
            break;
        case Identifier::IpAddressIPv4:
            str = "IpAddressIPv4";
            break;
        case Identifier::IpAddressIPv6:
            str = "IpAddressIPv6";
            break;
        case Identifier::MacAddress:
            str = "MacAddress";
            break;
        case Identifier::IdentityAikName:
            str = "IdentityAikName";
            break;
        case Identifier::IdentityDistinguishedName:
            str = "IdentityDistinguishedName";
            break;
        case Identifier::IdentityDnsName:
            str = "IdentityDnsName";
            break;
        case Identifier::IdentityEmailAddress:
            str = "IdentityEmailAddress";
            break;
        case Identifier::IdentityKerberosPrincipal:
            str = "IdentityKerberosPrincipal";
            break;
        case Identifier::IdentityTrustedPlatformModule:
            str = "IdentityTrustedPlatformModule";
            break;
        case Identifier::IdentityUsername:
            str = "IdentityUsername";
            break;
        case Identifier::IdentitySipUri:
            str = "IdentitySipUri";
            break;
        case Identifier::IdentityHipHit:
            str = "IdentityHipHit";
            break;
        case Identifier::IdentityTelUri:
            str = "IdentityTelUri";
            break;
        case Identifier::IdentityOther:
            str = "IdentityOther";
            break;
    }

    return str;
}

QString Identifier::idBaseStringForType(Identifier::IdType idType)
{
    QString str("");
    switch (idType) {
        case Identifier::IdNone:
            break;
        case Identifier::AccessRequest:
            str = "access-request";
            break;
        case Identifier::DeviceAikName:
        case Identifier::DeviceName:
            str = "device";
            break;
        case Identifier::IpAddressIPv4:
        case Identifier::IpAddressIPv6:
            str = "ip-address";
            break;
        case Identifier::MacAddress:
            str = "mac-address";
            break;
        case Identifier::IdentityAikName:
        case Identifier::IdentityDistinguishedName:
        case Identifier::IdentityDnsName:
        case Identifier::IdentityEmailAddress:
        case Identifier::IdentityKerberosPrincipal:
        case Identifier::IdentityTrustedPlatformModule:
        case Identifier::IdentityUsername:
        case Identifier::IdentitySipUri:
        case Identifier::IdentityHipHit:
        case Identifier::IdentityTelUri:
        case Identifier::IdentityOther:
            str = "identity";
            break;
    }

    return str;

}

Link Identifier::makeLinkFromIds(const Id& id1, const Id& id2)
{
    Link link;
    // Make identifier ordering independent of received order
    uint hash1 = qHash(id1.hashString());
    uint hash2 = qHash(id2.hashString());
    if (hash1 < hash2) {
        link.first = id1;
        link.second = id2;
    } else {
        link.first = id2;
        link.second = id1;
    }
    return link;
}

Id Identifier::otherIdForLink(const Link& link, const Id& targetId)
{
    if (link.first == targetId)
        return link.second;
    else
        return link.first;
}

Identifier::Identifier(Identifier::IdType type)
        : _type(type)
{
}

Identifier::Identifier(Identifier::IdType type, const QString& value, const QString& ad, const QString& other)
        : _type(type), _value(value), _ad(ad), _other(other)
{
}

QDataStream & operator<< ( QDataStream & stream, const Identifier & id )
{
    stream << Identifier::idStringForType(id.type()) << id.ad() << id.value();
    return stream;
}

bool Identifier::operator==(const Identifier &other) const
{
    if (this->type() == other.type() &&
        this->ad() == other.ad() &&
        this->value() == other.value() &&
        this->other() == other.other() )
        return true;
    else
        return false;
}

QString Identifier::hashString() const
{
    if (this->type() == Identifier::IdNone) return QString();

    QString hashStr = Identifier::idStringForType(this->type());
    hashStr += this->ad();
    hashStr += this->other();
    hashStr += this->value();

    return hashStr;
}

QDebug operator<<(QDebug dbg, const Identifier & id)
{
    dbg.nospace() << "Identifier(" << Identifier::idStringForType(id.type()) << ':'
                  << id.ad() << ":" << id.value() << ')';
    return dbg.space();
}

uint qHash ( const Identifier & key )
{
    return qHash(key.hashString());
}
