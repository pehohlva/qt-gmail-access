/*
managementserver.cpp: Implementation of ManagementServer class

Copyright (C) 2013  Sarab D. Mattes <mattes@nixnux.org>

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

#include <QTcpSocket>
#include <QVariantMap>

#include "managementserver.h"
#include "json.h"
#include "clientconfiguration.h"
#include "mapsessions.h"

ManagementServer::ManagementServer(MapGraphInterface *mapGraph, QObject *parent) :
    QTcpServer(parent), _mapGraph(mapGraph)
{
    _omapdConfig = OmapdConfig::getInstance();
}

bool ManagementServer::startListening()
{
    bool rc = false;
    QHostAddress listenOn;
    if (listenOn.setAddress(_omapdConfig->valueFor("mgmt_address").toString())) {
        unsigned int port = _omapdConfig->valueFor("mgmt_port").toUInt();

        if (listenOn != QHostAddress::LocalHost && listenOn != QHostAddress::LocalHostIPv6) {
            qDebug() << __PRETTY_FUNCTION__ << ":"
                     << "WARNING: Management interface configured for non-localhost interface:"
                     << listenOn.toString();
        }

        if (listen(listenOn, port)) {
            rc = true;
            this->setMaxPendingConnections(30); // 30 is QTcpServer default

            connect(this, SIGNAL(newConnection()), this, SLOT(handleMgmtConnection()));
        } else {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Error with listen on:" << listenOn.toString()
                    << ":" << port;
        }
    } else {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "Error setting server address";
    }

    return rc;
}

void ManagementServer::handleMgmtConnection()
{
    QTcpSocket *clientConnection = this->nextPendingConnection();

    if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowManagementRequests)) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "Received management request from:"
                 << clientConnection->peerAddress().toString();
    }

    connect(clientConnection, SIGNAL(disconnected()),
            clientConnection, SLOT(deleteLater()));

    connect(clientConnection, SIGNAL(readyRead()),
            this, SLOT(readMgmtRequest()));
}

void ManagementServer::readMgmtRequest()
{
    QTcpSocket *clientConnection = (QTcpSocket *)sender();

    QByteArray requestData = clientConnection->readLine(MANAGEMENT_MSG_MAX_LENGTH).trimmed();
    if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowManagementRequests)) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "Received management request:"
                 << requestData;
    }

    bool parseOk;
    //json is a QString containing the JSON data
    QVariantMap parsedRequest = QtJson::parse(requestData, parseOk).toMap();

    if(!parseOk) {
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowManagementRequests)) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Error parsing JSON in management request";
        }
        return;
    }

    QString command = parsedRequest["cmd"].toString();

    if (command == "mapdump") {
        _mapGraph->dumpMap();
    } else if (command == "addCertClient") {
        // Note: Clients created through mgmt interface will not persist across omapd restarts

        /*
           { "cmd":"addCertClient",
             "type":"single",
             "certPath":"/path/to/cert",
             "caPath":"/path/to/clientCAChain",
             "name":"clientName",
             "authorization":"authzValue",
             "metadataPolicy":"policyName" }
        */

        if (parsedRequest.contains("name") &&
                parsedRequest.contains("type") &&
                parsedRequest.contains("certPath") &&
                parsedRequest.contains("caPath") &&
                QFile::exists(parsedRequest["certPath"].toString()) &&
                QFile::exists(parsedRequest["caPath"].toString())) {

            QString clientType = parsedRequest["type"].toString();
            QString clientName = parsedRequest["name"].toString();
            QString certPath = parsedRequest["certPath"].toString();
            QString certCAPath = parsedRequest["caPath"].toString();

            unsigned int defaultAuthz = _omapdConfig->valueFor("default_authorization").toUInt();
            if (parsedRequest.contains("authorization")) {
                bool ok = false;
                unsigned int clientAuthz = parsedRequest["authorization"].toString().toUInt(&ok, 16);
                if (ok) defaultAuthz = clientAuthz;
            } else {
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowManagementRequests)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "Will apply default authorization for client:" << clientName;
                }
            }

            bool metadataPolicyOk = false;
            QString metadataPolicy;
            if (parsedRequest.contains("metadataPolicy") &&
                    !parsedRequest["metadataPolicy"].toString().isEmpty()) {
                metadataPolicy = parsedRequest["metadataPolicy"].toString();
                if (_omapdConfig->metadataPolicies().contains(metadataPolicy)) {
                    metadataPolicyOk = true;
                }
            } else {
                metadataPolicyOk = true;
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowManagementRequests)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "No metadata policy specified for client:" << clientName;
                }
            }

            // Create client
            if (metadataPolicyOk) {
                ClientConfiguration clientConfig;
                if (clientType == "single") {
                    clientConfig.createCertAuthClient(clientName,
                                                      certPath,
                                                      certCAPath,
                                                      OmapdConfig::authzOptions(defaultAuthz),
                                                      metadataPolicy);
                    MapSessions::getInstance()->loadClientConfiguration(&clientConfig);
                } else if (clientType == "CA") {
                    clientConfig.createCAAuthClient(clientName,
                                                    certPath,
                                                    certCAPath,
                                                    OmapdConfig::authzOptions(defaultAuthz),
                                                    metadataPolicy);
                    MapSessions::getInstance()->loadClientConfiguration(&clientConfig);
                } else {
                    if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowManagementRequests)) {
                        qDebug() << __PRETTY_FUNCTION__ << ":" << "Invalid clientType for client:" << clientName;
                    }
                }
            } else {
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowManagementRequests)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "Invalid metadataPolicy for client:" << clientName;
                }
            }
        } else {
            if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowManagementRequests)) {
                qDebug() << __PRETTY_FUNCTION__ << ":" << "Invalid parameters to addCertClient command";
            }
        }

    } else if (command == "removeCertClient") {
        // Note: Clients removed through mgmt interface but still defined in omapd.conf
        // will reappear when omapd restarts.

        /*
           { "cmd":"removeCertClient",
             "type":"single",
             "certPath":"/path/to/cert",
             "caPath":"/path/to/clientCAChain" }
        */

        if (parsedRequest.contains("type") &&
                parsedRequest.contains("certPath") &&
                parsedRequest.contains("caPath") &&
                QFile::exists(parsedRequest["certPath"].toString()) &&
                QFile::exists(parsedRequest["caPath"].toString())) {

            QString clientType = parsedRequest["type"].toString();
            QString certPath = parsedRequest["certPath"].toString();
            QString certCAPath = parsedRequest["caPath"].toString();

            ClientConfiguration clientConfig;
            if (clientType == "single") {
                clientConfig.createCertAuthClient("", certPath, certCAPath, 0, "");
                MapSessions::getInstance()->removeClientConfiguration(&clientConfig);
            } else if (clientType == "CA") {
                clientConfig.createCAAuthClient("", certPath, certCAPath, 0, "");
                MapSessions::getInstance()->removeClientConfiguration(&clientConfig);
            } else {
                if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowManagementRequests)) {
                    qDebug() << __PRETTY_FUNCTION__ << ":" << "Invalid clientType for removing client";
                }
            }

        }

    } else if (command == "addCertClientToBlacklist") {
        // Note: Clients blacklisted through mgmt interface but still defined in omapd.conf
        // will be re-enabled when omapd restarts.
        /*
        { "cmd":"addCertClientToBlacklist",
          "certPath":"/path/to/clientCert",
          "caPath":"/path/to/clientCACert" }
        */
        if (parsedRequest.contains("certPath") &&
                parsedRequest.contains("caPath") &&
                QFile::exists(parsedRequest["certPath"].toString()) &&
                QFile::exists(parsedRequest["caPath"].toString())) {

            QString certPath = parsedRequest["certPath"].toString();
            QString certCAPath = parsedRequest["caPath"].toString();

            ClientConfiguration clientConfig;
            clientConfig.createCertAuthClient("", certPath, certCAPath, 0, "");
            MapSessions::getInstance()->addBlacklistClientConfiguration(&clientConfig);
        }

    } else if (command == "removeCertClientFromBlacklist") {
        // Note: Clients removed from the blacklist through mgmt interface but still defined
        // in omapd.conf will be re-added to the blacklist when omapd restarts.
        /*
        { "cmd":"removeCertClientFromBlacklist",
          "certPath":"/path/to/clientCert",
          "caPath":"/path/to/clientCACert" }
        */
        if (parsedRequest.contains("certPath") &&
                parsedRequest.contains("caPath") &&
                QFile::exists(parsedRequest["certPath"].toString()) &&
                QFile::exists(parsedRequest["caPath"].toString())) {

            QString certPath = parsedRequest["certPath"].toString();
            QString certCAPath = parsedRequest["caPath"].toString();

            ClientConfiguration clientConfig;
            clientConfig.createCertAuthClient("", certPath, certCAPath, 0, "");
            MapSessions::getInstance()->removeBlacklistClientConfiguration(&clientConfig);
        }

    } else {
        if (_omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowManagementRequests)) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Received unrecognized management command:"
                     << command;
        }
    }
}
