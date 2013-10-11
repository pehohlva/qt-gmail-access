/*
cmlserver.cpp: Implementation of CmlServer class

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

#include <QtNetwork>
#include <QtCore>
#include <QXmlQuery>

#include "cmlserver.h"

CmlServer::DebugOptions CmlServer::debugOptions(unsigned int dbgValue)
{
    CmlServer::DebugOptions debug = CmlServer::DebugNone;
    if (dbgValue & CmlServer::ShowClientOps) debug |= CmlServer::ShowClientOps;
    if (dbgValue & CmlServer::ShowHTTPHeaders) debug |= CmlServer::ShowHTTPHeaders;
    if (dbgValue & CmlServer::ShowHTTPState) debug |= CmlServer::ShowHTTPState;
    if (dbgValue & CmlServer::ShowRawSocketData) debug |= CmlServer::ShowRawSocketData;

    return debug;
}

QString CmlServer::debugString(CmlServer::DebugOptions debug)
{
    QString str("");
    if (debug.testFlag(CmlServer::DebugNone)) str += "CmlServer::DebugNone | ";
    if (debug.testFlag(CmlServer::ShowClientOps)) str += "CmlServer::ShowClientOps | ";
    if (debug.testFlag(CmlServer::ShowHTTPHeaders)) str += "CmlServer::ShowHTTPHeaders | ";
    if (debug.testFlag(CmlServer::ShowHTTPState)) str += "CmlServer::ShowHTTPState | ";
    if (debug.testFlag(CmlServer::ShowRawSocketData)) str += "CmlServer::ShowRawSocketData | ";

    if (! str.isEmpty()) {
        str = str.left(str.size()-3);
    }
    return str;
}

QDebug operator<<(QDebug dbg, CmlServer::DebugOptions & dbgOptions)
{
    dbg.nospace() << CmlServer::debugString(dbgOptions);
    return dbg.space();
}

CmlServer::CmlServer(QObject *parent)
        : QTcpServer(parent)
{
    const char *fnName = "CmlServer::Server:";

    _omapdConfig = OmapdConfig::getInstance();

    if (_omapdConfig->valueFor("cml_ssl_configuration").toBool()) {
        // Set server cert, private key, CRLs, etc.
        QString ssl_proto = "AnyProtocol";
        if (_omapdConfig->isSet("cml_ssl_protocol")) {
            ssl_proto = _omapdConfig->valueFor("cml_ssl_protocol").toString();
        }
        // for NoSslV2 we kill the connection later if it is SslV2
        if ( ( ssl_proto == "AnyProtocol") || ( ssl_proto == "NoSslV2") )
            _desiredSSLprotocol = QSsl::AnyProtocol;
        else if (ssl_proto == "SslV2")
            _desiredSSLprotocol = QSsl::SslV2;
        else if (ssl_proto == "SslV3")
            _desiredSSLprotocol = QSsl::SslV3;
        else if (ssl_proto == "TlsV1")
            _desiredSSLprotocol = QSsl::TlsV1;
        else
        { // If this else is reached - an invalid protocol was in the xml file
          qDebug() << "cml_ssl_protocol -- type invalid -- trying to continue "
                   << "using AnyProtocol";
          this->_desiredSSLprotocol = QSsl::AnyProtocol;
        }

        QString keyFileName = "server.key";
        QByteArray keyPassword = "";
        if (_omapdConfig->isSet("cml_private_key_file")) {
            keyFileName = _omapdConfig->valueFor("cml_private_key_file").toString();
            if (_omapdConfig->isSet("cml_private_key_password")) {
                keyPassword = _omapdConfig->valueFor("cml_private_key_password").toByteArray();
            }
        }
        QFile keyFile(keyFileName);
        // TODO: Add QSsl::Der format support from _omapdConfig
        // TODO: Add QSsl::Dsa support from _omapdConfig
        if (!keyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << fnName << "No private key file:" << keyFile.fileName();
        } else {
            _serverKey = QSslKey(&keyFile, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, keyPassword);
            qDebug() << fnName << "Loaded private key";
        }

        QString certFileName = "server.cert";
        // TODO: Add QSsl::Der format support from _omapdConfig
        if (_omapdConfig->isSet("cml_certificate_file")) {
            certFileName = _omapdConfig->valueFor("cml_certificate_file").toString();
        }
        QFile certFile(certFileName);
        if (!certFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << fnName << "No certificate file:" << certFile.fileName();
        } else {
            // Try PEM format fail over to DER; since they are the only 2
            // supported by the QSsl Certificate classes
            _serverCert = QSslCertificate(&certFile, QSsl::Pem);
            if ( _serverCert.isNull() )
                _serverCert = QSslCertificate(&certFile, QSsl::Der);

            qDebug() << fnName << "Loaded certificate with CN:" << _serverCert.subjectInfo(QSslCertificate::CommonName);
        }

        // Load server CAs
        if (_omapdConfig->isSet("cml_ca_certificates_file")) {
            QFile caFile(_omapdConfig->valueFor("cml_ca_certificates_file").toString());
            if (!caFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qDebug() << fnName << "No CA certificates file" << caFile.fileName();
            } else {
                _caCerts = QSslCertificate::fromDevice(&caFile, QSsl::Pem);
                qDebug() << fnName << "Loaded num CA certs:" << _caCerts.size();
            }
        }
    }

    QHostAddress listenOn;
    if (listenOn.setAddress(_omapdConfig->valueFor("cml_address").toString())) {
        unsigned int port = _omapdConfig->valueFor("cml_port").toUInt();

        if (listen(listenOn, port)) {
            this->setMaxPendingConnections(30); // 30 is QTcpServer default

            connect(this, SIGNAL(headerReceived(QTcpSocket*,QNetworkRequest)),
                    this, SLOT(processHeader(QTcpSocket*,QNetworkRequest)));
            connect(this, SIGNAL(getReqReceived(QTcpSocket*,QString)),
                    this, SLOT(processGetReq(QTcpSocket*,QString)));
            connect(this, SIGNAL(putReqReceived(QTcpSocket*,QString)),
                    this, SLOT(processPutReq(QTcpSocket*,QString)));
            connect(this, SIGNAL(postReqReceived(QTcpSocket*,QString)),
                    this, SLOT(processPostReq(QTcpSocket*,QString)));
            connect(this, SIGNAL(delReqReceived(QTcpSocket*,QString)),
                    this, SLOT(processDelReq(QTcpSocket*,QString)));
        } else {
            qDebug() << fnName << "Error with listen on:" << listenOn.toString()
                    << ":" << port;
        }
    } else {
        qDebug() << fnName << "Error setting server address";
    }
}

void CmlServer::incomingConnection(int socketDescriptor)
{
    const char *fnName = "CmlServer::incomingConnection:";
    if (_omapdConfig->valueFor("cml_ssl_configuration").toBool()) {
        QSslSocket *sslSocket = new QSslSocket(this);
        if (sslSocket->setSocketDescriptor(socketDescriptor)) {

            sslSocket->setCiphers(QSslSocket::supportedCiphers());
            // SSL protocol type is now user set'able from the config file
            // the default value if not present is  QSsl::AnyProtocol
            sslSocket->setProtocol(_desiredSSLprotocol);

            // TODO: Have an option to set QSslSocket::setPeerVerifyDepth

            if (_omapdConfig->valueFor("cml_require_client_certificates").toBool()) {
                sslSocket->setPeerVerifyMode(QSslSocket::VerifyPeer);
            } else {
                // QueryPeer just asks for the client cert, but does not verify it
                sslSocket->setPeerVerifyMode(QSslSocket::QueryPeer);
            }

            // Connect SSL error signals to local slots
            connect(sslSocket, SIGNAL(peerVerifyError(QSslError)),
                    this, SLOT(clientSSLVerifyError(QSslError)));
            connect(sslSocket, SIGNAL(sslErrors(QList<QSslError>)),
                    this, SLOT(clientSSLErrors(QList<QSslError>)));

            sslSocket->setPrivateKey(_serverKey);
            sslSocket->setLocalCertificate(_serverCert);
            if (! _caCerts.isEmpty()) {
                sslSocket->setCaCertificates(_caCerts);
            }

            connect(sslSocket, SIGNAL(encrypted()), this, SLOT(socketReady()));

            sslSocket->startServerEncryption();
        } else {
            qDebug() << fnName << "Error setting SSL socket descriptor on QSslSocket";
            delete sslSocket;
        }
    } else {
        QTcpSocket *socket = new QTcpSocket(this);
        if (socket->setSocketDescriptor(socketDescriptor)) {
            connect(socket, SIGNAL(readyRead()), this, SLOT(readClient()));
            connect(socket, SIGNAL(disconnected()), this, SLOT(discardClient()));
            connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                    this, SLOT(clientConnState(QAbstractSocket::SocketState)));
        } else {
            qDebug() << fnName << "Error setting socket descriptor on QTcpSocket";
            delete socket;
        }
    }
}

void CmlServer::clientSSLVerifyError(const QSslError &error)
{
    const char *fnName = "CmlServer::clientSSLVerifyError:";
    //QSslSocket *sslSocket = (QSslSocket *)sender();

    qDebug() << fnName << error.errorString();
}

void CmlServer::clientSSLErrors(const QList<QSslError> &errors)
{
    const char *fnName = "CmlServer::clientSSLErrors:";
    QSslSocket *sslSocket = (QSslSocket *)sender();

    foreach (const QSslError &error, errors) {
        qDebug() << fnName << error.errorString();
    }

    qDebug() << fnName << "Calling ignoreSslErrors";
    sslSocket->ignoreSslErrors();
}

void CmlServer::socketReady()
{
    const char *fnName = "CmlServer::socketReady:";
    QSslSocket *sslSocket = (QSslSocket *)sender();
    /// Do SSLV2 Checks
    if ( sslSocket->protocol() == QSsl::SslV2 ) {
        /// if we've got SSLV2 kill it if NoSslV2 was requested
        if ( _omapdConfig->isSet("cml_ssl_protocol") &&
         _omapdConfig->valueFor("cml_ssl_protocol").toString() == "NoSslV2") {
            /// Not explicity Requested - so shut it down
            qDebug() << fnName << "Disconnecting client - client is using SslV2 - NoSslV2 was requested in config ";
            sslSocket->disconnectFromHost();
            sslSocket->deleteLater();
            return;
        }
    }
    qDebug() << fnName << "Successful SSL handshake with peer:" << sslSocket->peerAddress().toString();

    bool clientAuthorized = false;

    if (_omapdConfig->valueFor("cml_require_client_certificates").toBool()) {
        clientAuthorized = authorizeClient(sslSocket);
    } else {
        qDebug() << fnName << "Client authorized because cml_require_client_certificates is false, for peer:"
                 << sslSocket->peerAddress().toString();
        clientAuthorized = true;
    }

    if (clientAuthorized) {
        connect(sslSocket, SIGNAL(readyRead()), this, SLOT(readClient()));
        connect(sslSocket, SIGNAL(disconnected()), this, SLOT(discardClient()));
        connect(sslSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                this, SLOT(clientConnState(QAbstractSocket::SocketState)));
    } else {
        if (_omapdConfig->valueFor("cml_debug_level").value<CmlServer::DebugOptions>().testFlag(CmlServer::ShowClientOps))
            qDebug() << fnName << "Disconnecting unauthorized client at:" << sslSocket->peerAddress().toString();
        sslSocket->disconnectFromHost();
        sslSocket->deleteLater();
    }
}

bool CmlServer::authorizeClient(QSslSocket *sslSocket)
{
    const char *fnName = "CmlServer::authorizeClient:";

    QList<QSslCertificate> clientCerts = sslSocket->peerCertificateChain();
    qDebug() << fnName << "Cert chain for client at:" << sslSocket->peerAddress().toString();
    for (int i=0; i<clientCerts.size(); i++) {
        qDebug() << fnName << "-- CN:" << clientCerts.at(i).subjectInfo(QSslCertificate::CommonName);
    }

    // TODO: add authorization and policy layer
    return true;
}

void CmlServer::newClientConnection()
{
    while (this->hasPendingConnections()) {
        QTcpSocket *socket = this->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), this, SLOT(readClient()));
        connect(socket, SIGNAL(disconnected()), this, SLOT(discardClient()));
        connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                this, SLOT(clientConnState(QAbstractSocket::SocketState)));
    }
}

void CmlServer::clientConnState(QAbstractSocket::SocketState sState)
{
    const char *fnName = "CmlServer::clientConnState:";

    QTcpSocket* socket = (QTcpSocket*)sender();

    if (_omapdConfig->valueFor("cml_debug_level").value<CmlServer::DebugOptions>().testFlag(CmlServer::ShowHTTPState))
        qDebug() << fnName << "socket state for socket:" << socket
                 << "------------->:" << sState;

}

void CmlServer::readClient()
{
    const char *fnName = "CmlServer::readClient:";
    QTcpSocket* socket = (QTcpSocket*)sender();

    bool readError = false;
    qint64 nBytesAvailable = socket->bytesAvailable();
    QByteArray requestByteArr;

    while (nBytesAvailable && !readError) {
        // No header received yet
        if (readHeader(socket)) {
        } else {
            // Error - invalid header
            readError = true;
        }

        // Have received http header
        // TODO: Set a max on the size of the requestByteArr
        if (nBytesAvailable > 0) {
            QByteArray arr;
            arr.resize(nBytesAvailable);
            qint64 read = socket->read(arr.data(), nBytesAvailable);
            arr.resize(read);
            if (arr.size() > 0) {
                requestByteArr.append(arr);
            }
        }

        nBytesAvailable = socket->bytesAvailable();
    }

    if (_omapdConfig->valueFor("cml_debug_level").value<CmlServer::DebugOptions>().testFlag(CmlServer::ShowRawSocketData))
        qDebug() << fnName << "Raw Socket Data:" << endl << requestByteArr;
}

int CmlServer::readHeader(QTcpSocket *socket)
{
    const char *fnName = "CmlServer::readHeader:";
    QNetworkRequest requestWithHdr;
    bool end = false;
    QString tmp;
    QString headerStr = QLatin1String("");
    QString getReq, putReq, postReq, delReq;

    while (!end && socket->canReadLine()) {
        tmp = QString::fromUtf8(socket->readLine());
        if (tmp == QLatin1String("\r\n") || tmp == QLatin1String("\n") || tmp.isEmpty()) {
            end = true;
        } else {
            int hdrSepIndex = tmp.indexOf(":");
            if (hdrSepIndex != -1) {
                QString hdrName = tmp.left(hdrSepIndex);
                QString hdrValue = tmp.mid(hdrSepIndex+1).trimmed();
                requestWithHdr.setRawHeader(hdrName.toUtf8(), hdrValue.toUtf8());
                //qDebug() << fnName << "Got header:" << hdrName << "--->" << hdrValue;
            } else {
                if (tmp.contains("GET", Qt::CaseInsensitive)) {
                    int lIndex = tmp.indexOf("GET ");
                    int rIndex = tmp.indexOf(" HTTP");
                    getReq = tmp.mid(lIndex, rIndex - lIndex);
                    qDebug() << fnName << "Recieved GET request:" << getReq;
                } else if (tmp.contains("PUT", Qt::CaseInsensitive)) {
                    int lIndex = tmp.indexOf("PUT ");
                    int rIndex = tmp.indexOf(" HTTP");
                    putReq = tmp.mid(lIndex, rIndex - lIndex);
                    qDebug() << fnName << "Recieved PUT request:" << putReq;
                } else if (tmp.contains("POST", Qt::CaseInsensitive)) {
                    int lIndex = tmp.indexOf("POST ");
                    int rIndex = tmp.indexOf(" HTTP");
                    postReq = tmp.mid(lIndex, rIndex - lIndex);
                    qDebug() << fnName << "Recieved POST request:" << postReq;
                } else if (tmp.contains("DELETE", Qt::CaseInsensitive)) {
                    int lIndex = tmp.indexOf("DELETE ");
                    int rIndex = tmp.indexOf(" HTTP");
                    delReq = tmp.mid(lIndex, rIndex - lIndex);
                    qDebug() << fnName << "Recieved DELETE request:" << delReq;
                }
            }
            headerStr += tmp;
        }
    }

    if (end) {
        emit headerReceived(socket, requestWithHdr);
        if (! getReq.isEmpty()) emit getReqReceived(socket, getReq);
        if (! putReq.isEmpty()) emit putReqReceived(socket, putReq);
        if (! postReq.isEmpty()) emit postReqReceived(socket, postReq);
        if (! delReq.isEmpty()) emit delReqReceived(socket, delReq);
    }

    if (_omapdConfig->valueFor("cml_debug_level").value<CmlServer::DebugOptions>().testFlag(CmlServer::ShowHTTPHeaders))
        qDebug() << fnName << "headerStr:" << endl << headerStr;

    return headerStr.length();
}

void CmlServer::processHeader(QTcpSocket *socket, QNetworkRequest requestHdrs)
{
    const char *fnName = "CmlServer::processHeader:";

    // TODO: Improve http protocol support

    // Get CML Commands
    if (requestHdrs.hasRawHeader(QByteArray("Expect"))) {
        if (_omapdConfig->valueFor("cml_debug_level").value<CmlServer::DebugOptions>().testFlag(CmlServer::ShowHTTPHeaders))
            qDebug() << fnName << "Got Expect header";
        QByteArray expectValue = requestHdrs.rawHeader(QByteArray("Expect"));
        if (! expectValue.isEmpty() && expectValue.contains(QByteArray("100-continue"))) {
            if (_omapdConfig->valueFor("cml_debug_level").value<CmlServer::DebugOptions>().testFlag(CmlServer::ShowHTTPHeaders)) {
                qDebug() << fnName << "Got 100-continue Expect Header";
            }
            sendHttpResponse(socket, 100, "Continue");
        }
    }

    if (requestHdrs.hasRawHeader(QByteArray("Authorization"))) {
        if (_omapdConfig->valueFor("cml_debug_level").value<CmlServer::DebugOptions>().testFlag(CmlServer::ShowHTTPHeaders))
            qDebug() << fnName << "Got Authorization header";
        QByteArray basicAuthValue = requestHdrs.rawHeader(QByteArray("Authorization"));
        if (! basicAuthValue.isEmpty() && basicAuthValue.contains(QByteArray("Basic"))) {
            basicAuthValue = basicAuthValue.mid(6);
            if (_omapdConfig->valueFor("cml_debug_level").value<CmlServer::DebugOptions>().testFlag(CmlServer::ShowHTTPHeaders)) {
                qDebug() << fnName << "Got Basic Auth value:" << basicAuthValue;
            }
        }
    }
}

void CmlServer::sendHttpResponse(QTcpSocket *socket, int hdrNumber, QString hdrText)
{
    const char *fnName = "CmlServer::sendHttpResponse:";

    if (_omapdConfig->valueFor("cml_debug_level").value<CmlServer::DebugOptions>().testFlag(CmlServer::ShowHTTPHeaders)) {
        qDebug() << fnName << "Sending Http Response:" << hdrNumber << hdrText;
    }

    QHttpResponseHeader header(hdrNumber, hdrText);
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(header.toString().toUtf8() );
    }
}

void CmlServer::sendResponse(QTcpSocket *socket, const QByteArray &respArr)
{
    const char *fnName = "CmlServer::sendResponse:";

    QHttpResponseHeader header(200,"OK");
    header.setContentType("text/xml");
    //header.setValue("Content-Encoding","UTF-8");
    header.setContentLength( respArr.size() );
    header.setValue("Server","omapd/cml");

    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(header.toString().toUtf8() );
        socket->write( respArr );

        if (_omapdConfig->valueFor("cml_debug_level").value<CmlServer::DebugOptions>().testFlag(CmlServer::ShowHTTPHeaders))
            qDebug() << fnName << "Sent reply headers to client:" << endl << header.toString();

        if (_omapdConfig->valueFor("cml_debug_level").value<CmlServer::DebugOptions>().testFlag(CmlServer::ShowRawSocketData))
            qDebug() << fnName << "Sent reply to client:" << endl << respArr << endl;
    } else {
        qDebug() << fnName << "Socket is not connected!  Not sending reply to client";
    }
}

void CmlServer::discardClient()
{
    QTcpSocket *socket = (QTcpSocket *)sender();
    socket->deleteLater();
}

void CmlServer::processGetReq(QTcpSocket *socket, QString getReq)
{
    const char *fnName = "CmlServer::processGetReq:";

    if (getReq.compare("GET /config") == 0) {

    } else if (getReq.compare("GET /config/server") == 0) {

    } else if (getReq.compare("GET /config/server/port") == 0) {
        QString pStr;
        pStr.setNum(_omapdConfig->valueFor("ifmap_port").toUInt());
        qDebug() << fnName << "Got server port:" << pStr;
        sendResponse(socket, QByteArray(pStr.toUtf8()));
    }
}

void CmlServer::processPutReq(QTcpSocket *socket, QString putReq)
{
    const char *fnName = "CmlServer::processPutReq:";
    qDebug() << fnName << "Got PUT cmd:" << putReq << "on socket:" << socket;

    if (putReq.compare("PUT /config/server/port")) {
        qDebug() << fnName << "Don't know how to get port out of request payload";
        unsigned int port = 8082;
        _omapdConfig->addConfigItem("ifmap_port", port);
    }
}

void CmlServer::processPostReq(QTcpSocket *socket, QString postReq)
{
    const char *fnName = "CmlServer::processPostReq:";
    qDebug() << fnName << "Got POST cmd:" << postReq << "on socket:" << socket;
}

void CmlServer::processDelReq(QTcpSocket *socket, QString delReq)
{
    const char *fnName = "CmlServer::processDelReq:";
    qDebug() << fnName << "Got DELETE cmd:" << delReq << "on socket:" << socket;
}
