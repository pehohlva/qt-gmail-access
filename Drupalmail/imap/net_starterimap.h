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
#include "client_session.h"

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
    Q_ENUMS(CURRENTHANDLE)
    Q_PROPERTY(CURRENTHANDLE steps READ CURRENTHANDLE WRITE set_steps)

public:
    explicit Net_StarterImap(QObject *parent = 0);
    void SearchWord(QString w, const int day = -1);
    void SetQuery( const QString word );
    void PreparetoClose();
    void Connect(const QString user, const QString password);
    void ReConnect();
    ///// QByteArray compress_byte(const QByteArray& uncompressed );

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
        CONNECTION_LIVE = 99,
        AUTHENTICATION_SUCCESS = 2, //  AUTHENTICATION
        AUTHENTICATION_SEND_WAIT = 1,
        CAPABILITY_WAIT = 5,
        LISTCOMAND_SEND_WAIT = 55,
        DEFLATE_SEND_WAIT =  404,
        WAITFETCHMAIL_LONGCHUNK = 500,
        INBOX_COUNT_SEND_WAIT = 6,
        SEARCH_RESULT_SEND_WAIT = 20,
        UNKNOWSTATUS = 100,
        CONNECT_0 =0,
        CONNECTION_STOP_ERROR = 101
    };
    CURRENTHANDLE step() const { return STEPS; } 
    QByteArray steps() const { return STEP.toByteArray(); }
    void set_steps(CURRENTHANDLE value) { STEPS = value; }
    void create_steps(CURRENTHANDLE value , QVariant v) {
        set_steps(value);
        STEP = v;
        //// create_steps(CURRENTHANDLE,QVariant())
    }


signals:
    void Progress(int);
    void Exit_Close();
    void Next_Standby();
    void Message_Display(const QString msg);

public slots:
    void Ready_encrypted();
    void SockBytesWritten(qint64 written);
    void SomeErrorincomming(const QSslError & error);
    void Incomming_data();
    void Incomming_mailstream();
    void OnDisconnected();
    
    
protected:
    void _auth_login();
    
    
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
    QVariant STEP;
    quint16 CursorUID_Get;
    int CursorGetPoint;
    Compressor Talk;
    ReadMail::StreamMail *in_socket;
    int before; /// back day ?? to search 

};


#endif // NET_STARTERIMAP_H
