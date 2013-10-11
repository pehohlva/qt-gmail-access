/*
clientconfiguration.h: Declaration of ClientConfiguration class

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

#ifndef CLIENTCONFIGURATION_H
#define CLIENTCONFIGURATION_H

#include "omapdconfig.h"
#include "maprequest.h"

class ClientConfiguration
{
public:
    ClientConfiguration();
    void createBasicAuthClient(const QString& clientName, const QString& username, const QString& password, OmapdConfig::AuthzOptions authz, const QString& metadataPolicy);
    void createCertAuthClient(const QString& clientName, const QString& certFile, const QString& caCertFile, OmapdConfig::AuthzOptions authz, const QString& metadataPolicy);
    void createCAAuthClient(const QString& clientPrefix, const QString& issuingCACertFile, const QString& caCertFile, OmapdConfig::AuthzOptions authz, const QString& metadataPolicy, const QString& blacklistDir = "");

    const QString& metadataPolicy() const { return _metadataPolicy; }
    const QString& name() const { return _name; }
    const QString& username() const { return _username; }
    const QString& password() const { return _password; }
    const QString& certFileName() const { return _certFileName; }
    const QString& caCertFileName() const { return _caCertFileName; }
    const QString& blacklistDirectory() const { return _blacklistDirectory; }
    bool haveClientCert() const { return _haveClientCert; }
    MapRequest::AuthenticationType authType() const { return _authType; }
    OmapdConfig::AuthzOptions authz() const { return _authz; }

private:
    QString _metadataPolicy;
    QString _username;
    QString _password;
    QString _certFileName;
    QString _caCertFileName;
    QString _blacklistDirectory;
    bool _haveClientCert;
    MapRequest::AuthenticationType _authType;
    OmapdConfig::AuthzOptions _authz;
    QString _name;
};

#endif // CLIENTCONFIGURATION_H
