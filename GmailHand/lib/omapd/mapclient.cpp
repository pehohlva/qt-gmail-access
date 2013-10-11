/*
mapclient.cpp: Implementation of MapClient class

Copyright (C) 2011  Sarab D. Mattes <mattes@nixnux.org>

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

#include "mapclient.h"

MapClient::MapClient()
{
    _authType = MapRequest::AuthNone;
    _authz = OmapdConfig::DenyAll;
    _hasActiveSSRC = false;
    _hasActiveARC = false;
    _hasActivePoll = false;
}

MapClient::MapClient(const QString& authToken, MapRequest::AuthenticationType authType, OmapdConfig::AuthzOptions authz, const QString& pubId, const QString& metadataPolicy)
{
    _hasActiveSSRC = false;
    _hasActiveARC = false;
    _hasActivePoll = false;
    _authToken = authToken;
    _authType = authType;
    _authz = authz;
    _pubId = pubId;
    _metadataPolicy = metadataPolicy;
}

MapClient::~MapClient()
{
    clearSubscriptionList();
}

void MapClient::clearSubscriptionList()
{
    QListIterator<Subscription*> it(_subscriptionList);
    while(it.hasNext())
    {
        delete it.next();
    }
    _subscriptionList.clear();
}

void MapClient::insertSubscription(Subscription* sub)
{
    // we need to check if a subscription with this name
    // exists already and replace it with the new one if found
    QMutableListIterator<Subscription*> it(_subscriptionList);
    while(it.hasNext())
    {
        Subscription* s = it.next();
        if(*s == *sub)
        {
            delete s;
            it.setValue(sub);
            return;
        }
    }

    // otherwise just add the new one:
    _subscriptionList << sub;

}

void MapClient::removeSubscription(const QString& name)
{
    // we need to check if a subscription with this name
    // exists already and replace it with the new one if found
    QMutableListIterator<Subscription*> it(_subscriptionList);
    while(it.hasNext())
    {
        Subscription* s = it.next();
        if(s->_name == name)
        {
            delete s;
            it.remove();
            return;
        }
    }
}
