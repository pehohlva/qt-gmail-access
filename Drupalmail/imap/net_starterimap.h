//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#ifndef NET_STARTERIMAP_H
#define NET_STARTERIMAP_H

#include <QtNetwork/QSslSocket>
#include "net_imap_standard.h"
/// network
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>
 #include <QtNetwork/QSsl>
#include <QSslConfiguration>


#define _IMAIL_WIDTH_ 76

#define _CHUNK_MAIL_STOP_ \
             QString("U501 OK Success")

#define _CHUNK_UIDMAIL_STOP_ \
             QString("T500 OK Success")

#define _APPSCACHE_ \
             QString("%1/.GMaildir/").arg(QDir::homePath())

typedef QMap<int, QString> Qmail_List;

class Net_StarterImap : public QObject {
    Q_OBJECT

public:
    explicit Net_StarterImap(QObject *parent = 0);
    void SearchWord(QString w, const int day = -1);
    void Connect(const QString user, const QString password);
    void ReConnect();
    QByteArray compress_byte(const QByteArray& uncompressed );

    enum GETACTIONCURSOR {
        MAILNONE = 0,
        MAILGETINIT,
        MAILGETHEADER,
        MAILGETBODY_BY_UID,
        MAILGETFULL_CHUNK,
        MAILGET_RFC822_HEADER,
        IMAP_Cunnk_Incomming
    };
    
        enum Compressor {
        NO_ACTIVE = 0,
        DEFLATE_TALK
    };

    enum CURRENTHANDLE {
        CONNECTION_LIVE = 0,
        AUTHENTICATION__SUCCESS, //  AUTHENTICATION
        AUTHENTICATION_SEND_WAIT,
        CAPABILITY_WAIT,
        LISTCOMAND_SEND_WAIT,
        DEFLATE_SEND_WAIT,
        INBOX_COUNT_SEND_WAIT,
        SEARCH_RESULT_SEND_WAIT,
        UNKNOWSTATUS,
        YCSDHFIFISDFAIS
    };



signals:
    void Progress(int);
    void Exit_Close();
    void Message_Display(const QString msg);

public slots:
    void Ready_encrypted();
    void SockBytesWritten(qint64 written);
    void SomeErrorincomming(const QSslError & error);
    void Incomming_data();
    void Incomming_mailstream();
    void OnDisconnected();
private:


    void SendToServer(const QByteArray msg, bool cmd_prepend = false, bool newline = true);
    void NextMailUI(); /// next from search results 
    void ReadMail_UID_Body(const QString chunk); //// full mail header + body from UID nr pending 
    void Resume(const int at = 0); // debug info having 
    void Untagged_Line(const QString line);
    bool Error_Handler_Stream_WakeUp(const QString line);
    void GetMailUI(const quint32 uid, GETACTIONCURSOR = MAILGETHEADER);
    ///State s;
    QSslSocket *socket;
    quint16 sendcmd;
    bool readchunk;
    quint16 TotalMail;
    Qmail_List MailContainer;
    int CursorPos;
    ////  QString CursorMail;
    QString username;

    QByteArray Current_Letter;
    QByteArray SERVERLETTER;
    QString password;
    QString SENDCOMAND;
    QString query;
    QStringList q_Maildirlist;
    QStringList ResumeMail;
    QStringList encoded__Maildirlist;
    QStringList Log_Handshake; // imap talk log
    Imap_Cmd *_cmd;
    GETACTIONCURSOR maction;
    CURRENTHANDLE STEPS;
    quint16 CursorUID_Get;
    int CursorGetPoint;
    Compressor Talk;
    IMail *CMail;
    ReadMail::StreamMail *in_socket;
    int before; /// back day ?? to search 

};


#endif // NET_STARTERIMAP_H
