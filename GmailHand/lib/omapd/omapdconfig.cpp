/*
omapdconfig.cpp: Implementation of OmapdConfig class

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

#include "omapdconfig.h"
#include "server.h"
#include "mapclient.h"
#include "clientconfiguration.h"

OmapdConfig* OmapdConfig::_instance = 0;

OmapdConfig* OmapdConfig::getInstance()
{
    if (_instance == 0) {
        _instance = new OmapdConfig();
    }
    return _instance;
}

void OmapdConfig::destroy()
{
    if (_instance != 0) {
        delete _instance;
        _instance = NULL;
    }
}

OmapdConfig::IfmapDebugOptions OmapdConfig::debugOptions(unsigned int dbgValue)
{
    OmapdConfig::IfmapDebugOptions debug = OmapdConfig::DebugNone;
    if (dbgValue & OmapdConfig::ShowClientOps) debug |= OmapdConfig::ShowClientOps;
    if (dbgValue & OmapdConfig::ShowXML) debug |= OmapdConfig::ShowXML;
    if (dbgValue & OmapdConfig::ShowHTTPHeaders) debug |= OmapdConfig::ShowHTTPHeaders;
    if (dbgValue & OmapdConfig::ShowHTTPState) debug |= OmapdConfig::ShowHTTPState;
    if (dbgValue & OmapdConfig::ShowXMLParsing) debug |= OmapdConfig::ShowXMLParsing;
    if (dbgValue & OmapdConfig::ShowXMLFilterResults) debug |= OmapdConfig::ShowXMLFilterResults;
    if (dbgValue & OmapdConfig::ShowXMLFilterStatements) debug |= OmapdConfig::ShowXMLFilterStatements;
    if (dbgValue & OmapdConfig::ShowMAPGraphAfterChange) debug |= OmapdConfig::ShowMAPGraphAfterChange;
    if (dbgValue & OmapdConfig::ShowRawSocketData) debug |= OmapdConfig::ShowRawSocketData;
    if (dbgValue & OmapdConfig::ShowSearchAlgorithm) debug |= OmapdConfig::ShowSearchAlgorithm;
    if (dbgValue & OmapdConfig::ShowPluginOperations) debug |= OmapdConfig::ShowPluginOperations;
    if (dbgValue & OmapdConfig::ShowManagementRequests) debug |= OmapdConfig::ShowManagementRequests;

    return debug;
}

QString OmapdConfig::debugString(OmapdConfig::IfmapDebugOptions debug)
{
    QString str("");
    if (debug.testFlag(OmapdConfig::DebugNone)) str += "DebugNone | ";
    if (debug.testFlag(OmapdConfig::ShowClientOps)) str += "ShowClientOps | ";
    if (debug.testFlag(OmapdConfig::ShowXML)) str += "ShowXML | ";
    if (debug.testFlag(OmapdConfig::ShowHTTPHeaders)) str += "ShowHTTPHeaders | ";
    if (debug.testFlag(OmapdConfig::ShowHTTPState)) str += "ShowHTTPState | ";
    if (debug.testFlag(OmapdConfig::ShowXMLParsing)) str += "ShowXMLParsing | ";
    if (debug.testFlag(OmapdConfig::ShowXMLFilterResults)) str += "ShowXMLFilterResults | ";
    if (debug.testFlag(OmapdConfig::ShowXMLFilterStatements)) str += "ShowXMLFilterStatements | ";
    if (debug.testFlag(OmapdConfig::ShowMAPGraphAfterChange)) str += "ShowMAPGraphAfterChange | ";
    if (debug.testFlag(OmapdConfig::ShowRawSocketData)) str += "ShowRawSocketData | ";
    if (debug.testFlag(OmapdConfig::ShowSearchAlgorithm)) str += "ShowSearchAlgorithm | ";
    if (debug.testFlag(OmapdConfig::ShowPluginOperations)) str += "ShowPluginOperations | ";
    if (debug.testFlag(OmapdConfig::ShowManagementRequests)) str += "ShowManagementRequests | ";

    if (! str.isEmpty()) {
        str = str.left(str.size()-3);
    }
    return str;
}

QDebug operator<<(QDebug dbg, OmapdConfig::IfmapDebugOptions & dbgOptions)
{
    dbg.nospace() << OmapdConfig::debugString(dbgOptions);
    return dbg.space();
}

OmapdConfig::MapVersionSupportOptions OmapdConfig::mapVersionSupportOptions(unsigned int value)
{
    OmapdConfig::MapVersionSupportOptions support = OmapdConfig::SupportNone;
    if (value & OmapdConfig::SupportIfmapV10) support |= OmapdConfig::SupportIfmapV10;
    if (value & OmapdConfig::SupportIfmapV11) support |= OmapdConfig::SupportIfmapV11;
    if (value & OmapdConfig::SupportIfmapV20) support |= OmapdConfig::SupportIfmapV20;

    return support;
}

QString OmapdConfig::mapVersionSupportString(OmapdConfig::MapVersionSupportOptions debug)
{
    QString str("");
    if (debug.testFlag(OmapdConfig::SupportNone)) str += "SupportNone | ";
    if (debug.testFlag(OmapdConfig::SupportIfmapV10)) str += "SupportIfmapV10 | ";
    if (debug.testFlag(OmapdConfig::SupportIfmapV11)) str += "SupportIfmapV11 | ";
    if (debug.testFlag(OmapdConfig::SupportIfmapV20)) str += "SupportIfmapV20 | ";

    if (! str.isEmpty()) {
        str = str.left(str.size()-3);
    }
    return str;
}

QDebug operator<<(QDebug dbg, OmapdConfig::MapVersionSupportOptions & dbgOptions)
{
    dbg.nospace() << OmapdConfig::mapVersionSupportString(dbgOptions);
    return dbg.space();
}

OmapdConfig::AuthzOptions OmapdConfig::authzOptions(unsigned int authzValue)
{
    OmapdConfig::AuthzOptions value = OmapdConfig::DenyAll;

    if (authzValue & OmapdConfig::AllowPublish) value |= OmapdConfig::AllowPublish;
    if (authzValue & OmapdConfig::AllowSearch) value |= OmapdConfig::AllowSearch;
    if (authzValue & OmapdConfig::AllowSubscribe) value |= OmapdConfig::AllowSubscribe;
    if (authzValue & OmapdConfig::AllowPoll) value |= OmapdConfig::AllowPoll;
    if (authzValue & OmapdConfig::AllowPurgeSelf) value |= OmapdConfig::AllowPurgeSelf;
    if (authzValue & OmapdConfig::AllowPurgeOthers) value |= OmapdConfig::AllowPurgeOthers;

    return value;
}

QString OmapdConfig::authzOptionsString(OmapdConfig::AuthzOptions option)
{
    QString str("");
    if (option.testFlag(OmapdConfig::DenyAll)) str += "DenyAll | ";
    if (option.testFlag(OmapdConfig::AllowPublish)) str += "AllowPublish | ";
    if (option.testFlag(OmapdConfig::AllowSearch)) str += "AllowSearch | ";
    if (option.testFlag(OmapdConfig::AllowSubscribe)) str += "AllowSubscribe | ";
    if (option.testFlag(OmapdConfig::AllowPoll)) str += "AllowPoll | ";
    if (option.testFlag(OmapdConfig::AllowPurgeSelf)) str += "AllowPurgeSelf | ";
    if (option.testFlag(OmapdConfig::AllowPurgeOthers)) str += "AllowPurgeOthers | ";

    // Note str replace here if we get AllowAll
    if (option.testFlag(OmapdConfig::AllowAll)) str = "AllowAll | ";

    if (! str.isEmpty()) {
        str = str.left(str.size()-3);
    }
    return str;
}

QDebug operator<<(QDebug dbg, OmapdConfig::AuthzOptions & dbgOptions)
{
    dbg.nospace() << OmapdConfig::authzOptionsString(dbgOptions);
    return dbg.space();
}

OmapdConfig::OmapdConfig(QObject *parent)
    : QObject(parent)
{
    QVariant var;
    // Defaults
    _omapdConfig.insert("log_stderr", true);

    // Default MAP Version support is v1.0, v1.1, v2.0
    var.setValue(OmapdConfig::mapVersionSupportOptions(7));
    _omapdConfig.insert("version_support", var);

    var.setValue(OmapdConfig::debugOptions(0));

    _omapdConfig.insert("debug_level", var);
    _omapdConfig.insert("address", "0.0.0.0");
    _omapdConfig.insert("port", 8096);
    _omapdConfig.insert("ssl_configuration", false);
    _omapdConfig.insert("create_client_configurations", true);
    _omapdConfig.insert("allow_basic_auth_clients", true);
    _omapdConfig.insert("allow_invalid_session_id", false);
    _omapdConfig.insert("allow_unauthenticated_clients", false);
    _omapdConfig.insert("allow_arc_on_ssrc", false);
    _omapdConfig.insert("session_metadata_timeout", 180);
    _omapdConfig.insert("send_tcp_keepalives", false);
    _omapdConfig.insert("management_configuration", true);
    _omapdConfig.insert("mgmt_address", "127.0.0.1");
    _omapdConfig.insert("mgmt_port", 8097);

    // Default authorization is DenyAll
    var.setValue(OmapdConfig::authzOptions(0));
    _omapdConfig.insert("default_authorization", var);
}

OmapdConfig::~OmapdConfig()
{
    const char *fnName = "OmapdConfig::~OmapdConfig():";
    qDebug() << fnName;

    QListIterator<ClientConfiguration *> it(_clientConfigurations);
    while(it.hasNext())
    {
        delete it.next();
    }
    _clientConfigurations.clear();
}

void OmapdConfig::addConfigItem(const QString& key, const QVariant& value)
{
    _omapdConfig.insert(key,value);
}

void OmapdConfig::showConfigValues()
{
    const char *fnName = "showConfigValues:";

    QMapIterator<QString,QVariant> configIt(_omapdConfig);
    while (configIt.hasNext()) {
        configIt.next();
        QVariant var = configIt.value();
        if (var.type() == QVariant::UserType) {
            QString value;
            if (configIt.key() == "debug_level")
                value = OmapdConfig::debugString(var.value<OmapdConfig::IfmapDebugOptions>());
            else if (configIt.key() == "version_support")
                value = OmapdConfig::mapVersionSupportString(var.value<OmapdConfig::MapVersionSupportOptions>());
            else if (configIt.key() == "default_authorization")
                value = OmapdConfig::authzOptionsString(var.value<OmapdConfig::AuthzOptions>());

            qDebug() << fnName << configIt.key() << "-->" << value;
        } else {
            qDebug() << fnName << configIt.key() << "-->" << var;
        }
    }

    qDebug() << fnName << "Num client configurations loaded:" << _clientConfigurations.count();

    qDebug() << fnName << "Number of metadata policy definitions:" << _metadataPolicies.uniqueKeys().size();
    QHashIterator<QString, VSM> i(_metadataPolicies);
    while (i.hasNext()) {
        i.next();
        QString policyName = i.key();
        VSM metadataPolicy = i.value();
        qDebug() << fnName << "    policy:" << policyName
                 << "metadata:" << metadataPolicy.first
                 << "namespace:" << metadataPolicy.second;
    }
}

bool OmapdConfig::readConfigXML(QIODevice *device)
{
    const char *fnName = "ConfigFile::readConfigXML:";
    QXmlStreamReader xmlReader(device);

    if (xmlReader.readNextStartElement()) {
        if (xmlReader.name() == "omapd_configuration" &&
            xmlReader.attributes().value("version") == "2.0") {

            xmlReader.readNext();

            while (xmlReader.readNextStartElement()) {
                if (xmlReader.name() == "log_file_location") {
                    bool append = true;
                    if (xmlReader.attributes().value("append") == "no")
                        append = false;
                    addConfigItem("log_file_append", append);

                    addConfigItem(xmlReader.name().toString(), xmlReader.readElementText());

                } else if (xmlReader.name() == "log_stderr") {
                    bool enable = true;
                    if (xmlReader.attributes().value("enable") == "no")
                        enable = false;
                    addConfigItem(xmlReader.name().toString(), enable);

                } else if (xmlReader.name() == "debug_level") {
                    QString value = xmlReader.readElementText();
                    bool ok;
                    unsigned int dbgVal = value.toUInt(&ok, 16);
                    QVariant dbgVar(0);
                    if (ok) {
                        dbgVar.setValue(OmapdConfig::debugOptions(dbgVal));
                    }
                    addConfigItem(xmlReader.name().toString(), dbgVar);

                } else if (xmlReader.name() == "map_graph_plugin_path") {
                    QString pluginFile = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                    if (! QFile::exists(pluginFile))
                        xmlReader.raiseError(QObject::tr("map_graph_plugin_path file not found"));
                    addConfigItem(xmlReader.name().toString(), pluginFile);

                } else if (xmlReader.name() == "ifmap_metadata_v11_schema_path") {
                    QString ifmap11SchemaFile = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                    if (! QFile::exists(ifmap11SchemaFile))
                        xmlReader.raiseError(QObject::tr("ifmap_metadata_v11_schema_path file not found"));
                    addConfigItem(xmlReader.name().toString(), ifmap11SchemaFile);

                } else if (xmlReader.name() == "ifmap_metadata_v20_schema_path") {
                    QString ifmap20SchemaFile = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                    if (! QFile::exists(ifmap20SchemaFile))
                        xmlReader.raiseError(QObject::tr("ifmap_metadata_v20_schema_path file not found"));
                    addConfigItem(xmlReader.name().toString(), ifmap20SchemaFile);

                } else if (xmlReader.name() == "management_configuration") {
                    bool enable = false;
                    if (xmlReader.attributes().value("enable") == "yes")
                        enable = true;
                    addConfigItem(xmlReader.name().toString(), enable);

                    while (xmlReader.readNextStartElement()) {
                        if (xmlReader.name() == "address") {
                            addConfigItem("mgmt_" + xmlReader.name().toString(), xmlReader.readElementText());

                        } else if (xmlReader.name() == "port") {
                            QString value = xmlReader.readElementText();
                            addConfigItem("mgmt_" + xmlReader.name().toString(), QVariant(value.toUInt()));

                        } else {
                            xmlReader.skipCurrentElement();
                        }
                        xmlReader.readNext();
                    }

                } else if (xmlReader.name() == "service_configuration") {

                    while (xmlReader.readNextStartElement()) {
                        if (xmlReader.name() == "version_support") {
                            QString value = xmlReader.readElementText();
                            bool ok;
                            unsigned int supportVal = value.toUInt(&ok, 16);
                            QVariant supportVar;
                            supportVar.setValue(OmapdConfig::mapVersionSupportOptions(3));
                            if (ok) {
                                supportVar.setValue(OmapdConfig::mapVersionSupportOptions(supportVal));
                            }
                            addConfigItem(xmlReader.name().toString(), supportVar);

                        } else if (xmlReader.name() == "address") {
                            addConfigItem(xmlReader.name().toString(), xmlReader.readElementText());

                        } else if (xmlReader.name() == "port") {
                            QString value = xmlReader.readElementText();
                            addConfigItem(xmlReader.name().toString(), QVariant(value.toUInt()));

                        } else if (xmlReader.name() == "create_client_configurations") {
                            bool enable = false;
                            if (xmlReader.attributes().value("enable") == "yes")
                                enable = true;
                            addConfigItem(xmlReader.name().toString(), enable);

                        } else if (xmlReader.name() == "allow_basic_auth_clients") {
                            bool allow = false;
                            if (xmlReader.attributes().value("allow") == "yes")
                                allow = true;
                            addConfigItem(xmlReader.name().toString(), allow);

                        } else if (xmlReader.name() == "allow_unauthenticated_clients") {
                            bool allow = false;
                            if (xmlReader.attributes().value("allow") == "yes")
                                allow = true;
                            addConfigItem(xmlReader.name().toString(), allow);

                        } else if (xmlReader.name() == "allow_invalid_session_id") {
                            bool allow = false;
                            if (xmlReader.attributes().value("allow") == "yes")
                                allow = true;
                            addConfigItem(xmlReader.name().toString(), allow);

                        } else if (xmlReader.name() == "allow_arc_on_ssrc") {
                            bool allow = false;
                            if (xmlReader.attributes().value("allow") == "yes")
                                allow = true;
                            addConfigItem(xmlReader.name().toString(), allow);

                        } else if (xmlReader.name() == "session_metadata_timeout") {
                            QString value = xmlReader.readElementText();
                            addConfigItem(xmlReader.name().toString(), QVariant(value.toUInt()));

                        } else if (xmlReader.name() == "send_tcp_keepalives") {
                            bool enable = false;
                            if (xmlReader.attributes().value("enable") == "yes")
                                enable = true;
                            addConfigItem(xmlReader.name().toString(), enable);

                        } else if (xmlReader.name() == "ssl_configuration") {
                            bool enable = false;
                            if (xmlReader.attributes().value("enable") == "yes")
                                enable = true;
                            addConfigItem(xmlReader.name().toString(), enable);

                            while (xmlReader.readNextStartElement()) {
                                if (xmlReader.name() == "certificate_file") {
                                    QString certFile = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                                    if (! QFile::exists(certFile))
                                        xmlReader.raiseError(QObject::tr("certificate_file not found"));
                                    addConfigItem(xmlReader.name().toString(), certFile);

                                } else if (xmlReader.name() == "private_key_file") {
                                    QString keyFile = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                                    if (! QFile::exists(keyFile))
                                        xmlReader.raiseError(QObject::tr("private_key_file not found"));
                                    addConfigItem(xmlReader.name().toString(), keyFile);

                                    if (xmlReader.attributes().hasAttribute("password")) {
                                        addConfigItem("private_key_password", xmlReader.attributes().value("password").toString());
                                    }

                                } else {
                                    xmlReader.skipCurrentElement();
                                }
                                xmlReader.readNext();
                            }  // ssl_configuration
                        } else {
                            xmlReader.skipCurrentElement();
                        }
                        xmlReader.readNext();
                    }  // service_configuration
                } else if (xmlReader.name() == "metadata_policies") {
                    bool policiesDone = false;
                    while (!xmlReader.atEnd() && !policiesDone) {
                        xmlReader.readNext();

                        if (xmlReader.isStartElement() && xmlReader.name() == "metadata_policy") {
                            QString policyName;
                            if (xmlReader.attributes().hasAttribute("name")) {
                                policyName = xmlReader.attributes().value("name").toString();
                            } else {
                                xmlReader.raiseError(QObject::tr("name attribute not specified for policy"));
                            }
                            if (policyName.isEmpty()) {
                                xmlReader.raiseError(QObject::tr("invalid policy name attribute"));
                            }

                            bool metadataDone = false;
                            while (!xmlReader.atEnd() && !metadataDone) {
                                xmlReader.readNext();

                                if (xmlReader.isStartElement() && xmlReader.name() == "metadata") {
                                    QString metadataName;
                                    if (xmlReader.attributes().hasAttribute("name")) {
                                        metadataName = xmlReader.attributes().value("name").toString();
                                    } else {
                                        xmlReader.raiseError(QObject::tr("name attribute not specified for metadata"));
                                    }
                                    QString metadataNamespace;
                                    if (xmlReader.attributes().hasAttribute("metaNS")) {
                                        metadataNamespace = xmlReader.attributes().value("metaNS").toString();
                                    } else {
                                        xmlReader.raiseError(QObject::tr("namespace attribute not specified for metadata"));
                                    }

                                    if (metadataName.isEmpty() || metadataNamespace.isEmpty()) {
                                        xmlReader.raiseError(QObject::tr("metadata name or namespace attribute not specified"));
                                    }
                                    VSM mpol;
                                    mpol.first = metadataName;
                                    mpol.second = metadataNamespace;
                                    _metadataPolicies.insert(policyName, mpol);
                                }

                                if (xmlReader.tokenType() == QXmlStreamReader::EndElement &&
                                        xmlReader.name() == "metadata_policy") {
                                    metadataDone = true;
                                }
                            }
                            // End while (metadata)
                        }

                        if (xmlReader.tokenType() == QXmlStreamReader::EndElement &&
                            xmlReader.name() == "metadata_policies") {
                            policiesDone = true;
                        }
                    }
                    // End while (metadata_policies

                } else if (xmlReader.name() == "client_configuration") {
                    QVariant defaultAuthzVar;
                    if (xmlReader.attributes().hasAttribute("default-authorization")) {
                        bool ok;
                        unsigned int authzVal = xmlReader.attributes().value("default-authorization").toString().toUInt(&ok, 16);
                        if (ok) {
                            defaultAuthzVar.setValue(authzVal);
                            addConfigItem("default_authorization", defaultAuthzVar);
                        }
                    }

                    bool clientsDone = false;
                    while (!xmlReader.atEnd() && !clientsDone) {
                        xmlReader.readNext();

                        if (xmlReader.isStartElement() && xmlReader.name() == "client") {
                            unsigned int clientAuthz = _omapdConfig.value("default_authorization").toUInt();
                            QString clientName;
                            QString authType;
                            QString metadataPolicy;
                            if (xmlReader.attributes().hasAttribute("name")) {
                                clientName = xmlReader.attributes().value("name").toString();
                            } else {
                                xmlReader.raiseError(QObject::tr("name attribute not specified for client"));
                            }
                            if (xmlReader.attributes().hasAttribute("authorization")) {
                                bool ok;
                                unsigned int authzAttrVal = xmlReader.attributes().value("authorization").toString().toUInt(&ok, 16);
                                if (ok) {
                                    clientAuthz = authzAttrVal;
                                }
                            }
                            if (xmlReader.attributes().hasAttribute("authentication")) {
                                authType = xmlReader.attributes().value("authentication").toString();
                            } else {
                                xmlReader.raiseError(QObject::tr("authentication attribute not specified for client"));
                            }
                            if (xmlReader.attributes().hasAttribute("metadata_policy")) {
                                metadataPolicy = xmlReader.attributes().value("metadata_policy").toString();
                                if (!_metadataPolicies.contains(metadataPolicy)) {
                                    xmlReader.raiseError(QObject::tr("metadata_policy not found for client"));
                                }
                            }
                            xmlReader.readNextStartElement();

                            if (authType == "basic") {
                                QString username, password;
                                bool haveUsername = false, havePassword = false;
                                for (int i=0; i<2; i++) {
                                    if (xmlReader.name() == "username") {
                                        username = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                                        haveUsername = true;
                                    } else if (xmlReader.name() == "password") {
                                        password = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                                        havePassword = true;
                                    } else {
                                        xmlReader.raiseError(QObject::tr("invalid basic auth element"));
                                    }
                                    xmlReader.readNextStartElement();
                                }
                                if (haveUsername && havePassword) {
                                    // Create client
                                    ClientConfiguration *clientConfig = new ClientConfiguration();
                                    clientConfig->createBasicAuthClient(clientName,
                                                                        username,
                                                                        password,
                                                                        OmapdConfig::authzOptions(clientAuthz),
                                                                        metadataPolicy);
                                    _clientConfigurations.append(clientConfig);
                                }
                            } else if (authType == "certificate") {
                                bool haveClientCertFile = false, haveCACertFile = false;
                                QString certFileName;
                                QString caCertFileName;

                                for (int i=0; i<2; i++) {
                                    if (xmlReader.name() == "certificate_file") {
                                        // TODO: Deal with certificate type {pem|der}
                                        certFileName = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                                        haveClientCertFile = QFile::exists(certFileName);
                                    } else if (xmlReader.name() == "ca_certificates_file") {
                                        // TODO: Deal with certificate type {pem|der}
                                        caCertFileName = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                                        haveCACertFile = QFile::exists(caCertFileName);
                                    } else {
                                        xmlReader.raiseError(QObject::tr("invalid cert auth element"));
                                    }
                                    xmlReader.readNextStartElement();
                                }
                                if (haveCACertFile && haveClientCertFile) {
                                    // Create client
                                    ClientConfiguration *clientConfig = new ClientConfiguration();
                                    clientConfig->createCertAuthClient(clientName,
                                                                       certFileName,
                                                                       caCertFileName,
                                                                       OmapdConfig::authzOptions(clientAuthz),
                                                                       metadataPolicy);
                                    _clientConfigurations.append(clientConfig);
                                }

                            } else if (authType == "ca-certificate") {
                                bool haveIssuingCACertFile = false, haveCACertFile = false;
                                QString issuingCaCertFileName;
                                QString caCertFileName;
                                QString blacklistDirectory;

                                for (int i=0; i<2; i++) {
                                    if (xmlReader.name() == "issuing_ca_certificate_file") {
                                        // TODO: Deal with certificate type {pem|der}
                                        issuingCaCertFileName = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                                        haveIssuingCACertFile = QFile::exists(issuingCaCertFileName);
                                    } else if (xmlReader.name() == "ca_certificates_file") {
                                        // TODO: Deal with certificate type {pem|der}
                                        caCertFileName = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                                        haveCACertFile = QFile::exists(caCertFileName);
                                    } else if (xmlReader.name() == "blacklist_directory") {
                                        blacklistDirectory = xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                                    } else {
                                        xmlReader.raiseError(QObject::tr("invalid ca-certificate auth element"));
                                    }
                                    xmlReader.readNextStartElement();
                                }
                                if (haveCACertFile && haveIssuingCACertFile) {
                                    // Create client
                                    ClientConfiguration *clientConfig = new ClientConfiguration();
                                    clientConfig->createCAAuthClient(clientName,
                                                                     issuingCaCertFileName,
                                                                     caCertFileName,
                                                                     OmapdConfig::authzOptions(clientAuthz),
                                                                     metadataPolicy,
                                                                     blacklistDirectory);
                                    _clientConfigurations.append(clientConfig);
                                }

                            } else {
                                xmlReader.raiseError(QObject::tr("invalid authentication type specified for client"));
                            }
                        }

                        if (xmlReader.tokenType() == QXmlStreamReader::EndElement &&
                            xmlReader.name() == "client_configuration") {
                            clientsDone = true;
                        }
                    }
                } else {
                    xmlReader.skipCurrentElement();
                }
                xmlReader.readNext();
            }  // omapd_configuration
        } else {
            xmlReader.raiseError(QObject::tr("The file is not an omapd Config file version 2.0"));
        }

    }

    if (xmlReader.error()) {
        qDebug() << fnName << "XML Error on line" << xmlReader.lineNumber();
        qDebug() << fnName << "-->" << xmlReader.errorString();
    }

    return !xmlReader.error();
}

int OmapdConfig::readConfigFile(const QString& configFileName)
{
    const char *fnName = "ConfigFile::readConfigFile:";
    int rc = 0;

    QFile cfile(configFileName);
    if (!cfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << fnName << "Error opening omapd Config File:" << cfile.fileName();
        qDebug() << fnName << "|-->" << cfile.errorString();
        return -1;
    }

    if (!readConfigXML(&cfile)) {
        qDebug() << fnName << "Error reading XML in IPM Config File:" << cfile.fileName();
        rc = -1;
    } else {
        rc = _omapdConfig.count();
    }

    cfile.close();

    return rc;
}

const QVariant& OmapdConfig::valueFor(const QString& key)
{
    return _omapdConfig[key];
}
