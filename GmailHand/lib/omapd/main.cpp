/*
main.cpp: Entry point of omapd

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

#include <QtCore/QCoreApplication>
#include <QtNetwork>
#include <stdio.h>

#include "server.h"
#include "omapdconfig.h"
#include "mapgraphinterface.h"
#include "managementserver.h"

#if defined(Q_OS_WIN)
#define _MAPGRAPH_PLUGIN_FILENAME "RAMHashTables.dll"
#else
#define _MAPGRAPH_PLUGIN_FILENAME "libRAMHashTables.so"
#endif

QFile logFile;
QFile logStderr;

void myMessageOutput(QtMsgType type, const char *msg)
{
    QByteArray bmsg(msg, qstrlen(msg));
    bmsg.prepend(": ");
    bmsg.prepend(QDateTime::currentDateTime().toUTC().toString("yyyy-MM-ddThh:mm:ssZ").toAscii());
    bmsg.append("\n");
    switch (type) {
    case QtDebugMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
        if (logFile.isOpen()) {
            logFile.write(bmsg);
            logFile.flush();
        }
        if (logStderr.isOpen()) {
            logStderr.write(bmsg);
            logStderr.flush();
        }
        break;
    case QtFatalMsg:
        if (logFile.isOpen()) {
            logFile.write(bmsg);
            logFile.flush();
        }
        if (logStderr.isOpen()) {
            logStderr.write(bmsg);
            logStderr.flush();
        }
        abort();
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString confFile("omapd.conf");
    if (argc == 2) {
        qDebug() << "main: will look for omapd configuration in:" << argv[1];
        confFile = QString(argv[1]);
    }

    OmapdConfig *omapdConfig = OmapdConfig::getInstance();
    if (omapdConfig->readConfigFile(confFile) < 0) {
        exit(-1);
    }

    if (omapdConfig->isSet("log_file_location")) {
        logFile.setFileName(omapdConfig->valueFor("log_file_location").toString().toAscii());
        QIODevice::OpenMode openMode = QIODevice::WriteOnly | QIODevice::Text;
        if (omapdConfig->valueFor("log_file_append").toBool())
            openMode |= QIODevice::Append;
        if (! logFile.open(openMode)) {
            qDebug() << "main: Error opening omapd log file:" << logFile.fileName();
            qDebug() << "main: |-->" << logFile.errorString();
        }
    }

    if (omapdConfig->valueFor("log_stderr").toBool()) {
        if (! logStderr.open(stderr, QIODevice::WriteOnly)) {
            qDebug() << "main: Error opening stderr for logging:" << logStderr.errorString();
        }
    }
    qInstallMsgHandler(myMessageOutput);

    qDebug() << "main: starting omapd on:" << QDateTime::currentDateTime().toString();

    omapdConfig->showConfigValues();

    //TODO: Threadpool the server objects and synchronize access to the MAP Graph

    QString pluginPath;
    if (omapdConfig->isSet("map_graph_plugin_path")) {
        pluginPath = omapdConfig->valueFor("map_graph_plugin_path").toString();
    } else {
        QDir pluginsDir(qApp->applicationDirPath());
#if defined(Q_OS_WIN)
        if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
            pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
        if (pluginsDir.dirName() == "MacOS") {
            pluginsDir.cdUp();
            pluginsDir.cdUp();
            pluginsDir.cdUp();
        }
#endif
        pluginsDir.cd("plugins");
        pluginPath = pluginsDir.absoluteFilePath(_MAPGRAPH_PLUGIN_FILENAME);
    }

    qDebug() << "Will load plugin from:" << pluginPath;
    MapGraphInterface *mapGraph = 0;
    QPluginLoader pluginLoader(pluginPath);
    QObject *plugin = pluginLoader.instance();
    if (plugin) {
        mapGraph = qobject_cast<MapGraphInterface *>(plugin);
        if (!mapGraph) {
            qDebug() << "main: could not load MapGraph Plugin";
            exit(1);
        }
        if (omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowPluginOperations))
            mapGraph->setDebug(true);
        else
            mapGraph->setDebug(false);

    } else {
        qDebug() << "main: could not get plugin instance";
        exit(1);
    }

    // Set default SSL Configuration
    QSslConfiguration defaultConfig = QSslConfiguration::defaultConfiguration();

    // Don't allow SSLv2
    defaultConfig.setProtocol(QSsl::SecureProtocols);

    // TODO: Have an option to set QSslSocket::setPeerVerifyDepth
    defaultConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);

    QString keyFileName = "server.key";
    QByteArray keyPassword = "";
    if (omapdConfig->isSet("private_key_file")) {
        keyFileName = omapdConfig->valueFor("private_key_file").toString();
        if (omapdConfig->isSet("private_key_password")) {
            keyPassword = omapdConfig->valueFor("private_key_password").toByteArray();
        }
    }
    QFile keyFile(keyFileName);
    // TODO: Add QSsl::Der format support from omapdConfig
    // TODO: Add QSsl::Dsa support from omapdConfig
    if (!keyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "No private key file:" << keyFile.fileName();
    } else {
        QSslKey serverKey(&keyFile, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, keyPassword);
        if (omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowClientOps))
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Loaded omapd server private key";
        defaultConfig.setPrivateKey(serverKey);
    }

    QSslCertificate serverCert;
    QString certFileName = "server.pem";
    // TODO: Add QSsl::Der format support from omapdConfig
    if (omapdConfig->isSet("certificate_file")) {
        certFileName = omapdConfig->valueFor("certificate_file").toString();
    }
    QFile certFile(certFileName);
    if (!certFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "No certificate file:" << certFile.fileName();
    } else {
        // Try PEM format fail over to DER; since they are the only 2
        // supported by the QSsl Certificate classes
        serverCert = QSslCertificate(&certFile, QSsl::Pem);
        if ( serverCert.isNull() )
            serverCert = QSslCertificate(&certFile, QSsl::Der);

        defaultConfig.setLocalCertificate(serverCert);
        if (omapdConfig->valueFor("debug_level").value<OmapdConfig::IfmapDebugOptions>().testFlag(OmapdConfig::ShowClientOps))
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Loaded omapd server certificate with CN:" << serverCert.subjectInfo(QSslCertificate::CommonName);
    }

    // Clear CA Cert List
    QList<QSslCertificate> emptyCACertList;
    emptyCACertList.append(serverCert);
    defaultConfig.setCaCertificates(emptyCACertList);

    QSslConfiguration::setDefaultConfiguration(defaultConfig);

    // Start a server with this MAP graph
    Server *server = new Server(mapGraph);
    if (!server->startListening()) {
        qDebug() << __PRETTY_FUNCTION__ << ":" << "Could not start server";
        exit(2);
    }
    qDebug() << "Started MAP server";

    if (omapdConfig->valueFor("management_configuration").toBool()) {
        ManagementServer *mgmtServer = new ManagementServer(mapGraph);
        if (!mgmtServer->startListening()) {
            qDebug() << __PRETTY_FUNCTION__ << ":" << "Could not start management server";
            exit(3);
        }
        qDebug() << "Started management server";
    }

    int exitCode = a.exec();

    OmapdConfig::destroy();

    return exitCode;
}
