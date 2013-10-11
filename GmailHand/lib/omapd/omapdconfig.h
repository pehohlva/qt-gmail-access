/*
omapdconfig.h: Declaration of OmapdConfig class

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

#ifndef OMAPDCONFIG_H
#define OMAPDCONFIG_H

#include <QtCore>

class ClientConfiguration;

typedef QPair<QString, QString> VSM; // elementName, elementNamespace

class OmapdConfig : public QObject
{
    Q_OBJECT
public:
    enum IfmapDebug {
                DebugNone = 0x000,
                ShowClientOps = 0x0001,
                ShowXML = 0x0002,
                ShowHTTPHeaders = 0x0004,
                ShowHTTPState = 0x0008,
                ShowXMLParsing = 0x0010,
                ShowXMLFilterResults = 0x0020,
                ShowXMLFilterStatements = 0x0040,
                ShowMAPGraphAfterChange = 0x0080,
                ShowRawSocketData = 0x0100,
                ShowSearchAlgorithm = 0x0200,
                ShowPluginOperations = 0x0400,
                ShowManagementRequests = 0x0800
               };
    Q_DECLARE_FLAGS(IfmapDebugOptions, IfmapDebug);
    static IfmapDebugOptions debugOptions(unsigned int dbgValue);
    static QString debugString(OmapdConfig::IfmapDebugOptions debug);

    enum MapVersionSupport {
               SupportNone = 0x00,
               SupportIfmapV10 = 0x01,
               SupportIfmapV11 = 0x02,
               SupportIfmapV20 = 0x04
                           };
    Q_DECLARE_FLAGS(MapVersionSupportOptions, MapVersionSupport);
    static MapVersionSupportOptions mapVersionSupportOptions(unsigned int value);
    static QString mapVersionSupportString(OmapdConfig::MapVersionSupportOptions debug);

    enum Authz {
        DenyAll = 0x00,
        AllowPublish = 0x01,
        AllowSearch = 0x02,
        AllowSubscribe = 0x04,
        AllowPoll = 0x08,
        AllowPurgeSelf = 0x10,
        AllowPurgeOthers = 0x20,
        AllowAll = 0x3F // Convenience enum "or" of all allow options
    };
    Q_DECLARE_FLAGS(AuthzOptions, Authz);

    static void destroy();

    static AuthzOptions authzOptions(unsigned int authzValue);
    static QString authzOptionsString(OmapdConfig::AuthzOptions option);

    static OmapdConfig* getInstance();

    bool isSet(const QString& key) { return _omapdConfig.contains(key); }
    const QVariant& valueFor(const QString& key);
    void showConfigValues();

    const QList<ClientConfiguration *>& clientConfigurations() { return _clientConfigurations; }
    const QMultiHash<QString, VSM>& metadataPolicies() { return _metadataPolicies; }

    int readConfigFile(const QString& configFileName = "omapd.conf");

    void addConfigItem(const QString& key, const QVariant& value);
private:
    OmapdConfig(QObject * parent = 0);
    ~OmapdConfig();

    bool readConfigXML(QIODevice *device);

private:
    static OmapdConfig *_instance;

    QMap<QString,QVariant> _omapdConfig;
    QList<ClientConfiguration *> _clientConfigurations;
    QMultiHash<QString,VSM> _metadataPolicies; // policyName, (metaName, metaNS)
};
Q_DECLARE_OPERATORS_FOR_FLAGS(OmapdConfig::IfmapDebugOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(OmapdConfig::MapVersionSupportOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(OmapdConfig::AuthzOptions)
Q_DECLARE_METATYPE(OmapdConfig::IfmapDebugOptions)
Q_DECLARE_METATYPE(OmapdConfig::MapVersionSupportOptions)
Q_DECLARE_METATYPE(OmapdConfig::AuthzOptions)

QDebug operator<<(QDebug dbg, OmapdConfig::IfmapDebugOptions & dbgOptions);
QDebug operator<<(QDebug dbg, OmapdConfig::MapVersionSupportOptions & dbgOptions);
QDebug operator<<(QDebug dbg, OmapdConfig::AuthzOptions & authzOptions);

#endif // OMAPDCONFIG_H
