/*
mapclient.h: Declaration of MapClient Class

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

#ifndef MAPCLIENT_H
#define MAPCLIENT_H

#include "maprequest.h"
#include "subscription.h"

class MapClient
{
public:
    // Don't use, but needs to be here for QHash's methods
    // that return a default constructed value
    MapClient();

    MapClient(const QString& authToken,
              MapRequest::AuthenticationType authType,
              OmapdConfig::AuthzOptions authz,
              const QString& pubId,
              const QString& metadataPolicy);

    ~MapClient();

    const QString& pubId()  const { return _pubId; }
    const QString& authToken() const { return _authToken; }
    MapRequest::AuthenticationType authType() const { return _authType; }
    OmapdConfig::AuthzOptions authz() const { return _authz; }
    const QString& metadataPolicy() const { return _metadataPolicy; }

    const QString& sessId() const { return _sessId; }
    bool hasActiveSSRC() const { return _hasActiveSSRC; }
    bool hasActiveARC() const { return _hasActiveARC; }
    bool hasActivePoll() const { return _hasActivePoll; }
    const QList<Subscription*>& subscriptionList() const { return _subscriptionList; }
    QList<Subscription*>& subscriptionList() { return _subscriptionList; }

    void setSessId(const QString& sessId) { _sessId = sessId; }
    void clearSessId() { _sessId.clear(); }
    void setHasActiveSSRC(bool hasActiveSSRC) { _hasActiveSSRC = hasActiveSSRC; }
    void setHasActiveARC(bool hasActiveARC) { _hasActiveARC = hasActiveARC; }
    void setHasActivePoll(bool hasActivePoll) { _hasActivePoll = hasActivePoll; }

    // void setSubscriptionList(const QList<Subscription>& subList) { _subscriptionList = subList; }
    void insertSubscription(Subscription* sub);
    void removeSubscription(const QString& name);
    void clearSubscriptionList();

private:
    QString _pubId;
    QString _authToken;
    MapRequest::AuthenticationType _authType;
    OmapdConfig::AuthzOptions _authz;
    QString _metadataPolicy;

    QString _sessId;
    bool _hasActiveSSRC;
    bool _hasActiveARC;
    bool _hasActivePoll;
    QList<Subscription*> _subscriptionList;
};

#endif // MAPCLIENT_H
