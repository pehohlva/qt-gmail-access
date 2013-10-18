//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#include "net_starterimap.h"
#include "parser_utils.h"

/// network
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>




/// /Users/pro/project/GmailHand/

Net_StarterImap::Net_StarterImap(QObject *parent) :
QObject(parent), _cmd(new Imap_Cmd()) {
    sendcmd = 0;
    socket = NULL;
    Talk = NO_ACTIVE;
    q_Maildirlist.clear();
    query = QString("comunicato stampa");
    before = -2;
    Current_Letter = "A";
    SERVERLETTER = "";
    SENDCOMAND = "";
    TotalMail = 0;
    readchunk = false;
    CursorPos = 0;
    CursorUID_Get = 0;
    maction = MAILNONE;
    in_socket = new ReadMail::StreamMail();
    QTextStream out(stdout);
    QString str("*");
    CursorGetPoint = 0;

    QDir dira(_APPSCACHE_);
    if (dira.mkpath(_APPSCACHE_)) {
        out << str.fill('*', _IMAIL_WIDTH_) << "\n";
        out << "Make cache dir  on:" << _APPSCACHE_ << " \n";
        out << str.fill('*', _IMAIL_WIDTH_) << "\n";
        out.flush();
    }
    create_steps(UNKNOWSTATUS, QVariant("UNKNOWSTATUS"));
    _cmd->SetComand(Imap_Cmd::IMAP_Unconnected);
}

void Net_StarterImap::Connect(const QString user, const QString word) {
    username = user;
    password = word;
    create_steps(CONNECT_0, QVariant("CONNECT_0"));
    socket = new QSslSocket(0);
    socket->setObjectName("SSL_on_Gmail");
    /// socket->setSslConfiguration(QSsl::SslOptionDisableEmptyFragments|QSsl::SslOptionDisableLegacyRenegotiation|QSsl::SslOptionDisableCompression);
    connect(socket, SIGNAL(encrypted()), this, SLOT(Ready_encrypted()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(Incomming_data()));
    socket->connectToHostEncrypted("imap.googlemail.com", 993);
    _cmd->SetComand(Imap_Cmd::IMAP_Init);
}

void Net_StarterImap::ReConnect() {
    /// send to other contenents QTextStream if like.
    QTextStream out(stdout);
    QString str("*");

    if (socket) {
        socket->deleteLater();
    }
    if (!password.isEmpty() && !password.isEmpty()) {
        socket = new QSslSocket(0);
        create_steps(CONNECT_0, QVariant("CONNECT_0"));
        connect(socket, SIGNAL(encrypted()), this, SLOT(Ready_encrypted()));
        connect(socket, SIGNAL(readyRead()), this, SLOT(Incomming_data()));
        //// connect(socket, SIGNAL(encryptedBytesWritten(qint64 written)), this, SLOT(SockBytesWritten(qint64 written)));
        socket->connectToHostEncrypted("imap.googlemail.com", 993);
        _cmd->SetComand(Imap_Cmd::IMAP_Init);
    } else {
        out << str.fill('*', _IMAIL_WIDTH_) << "\n";
        out << "Warning: User & Password is Null or not found! \n";
        out << str.fill('*', _IMAIL_WIDTH_) << "\n";
        out.flush();
        Resume(__LINE__);
    }
}

void Net_StarterImap::SendToServer(const QByteArray msg, bool cmd_prepend, bool newline) {

    QTextStream out(stdout);
    QString str("*");
    //// QHostAddress ipnamer = socket->localAddress();
    ////const QString localhost = ipnamer.toString();
    sendcmd++;
    QVariant nr(sendcmd);
    QByteArray anr(nr.toByteArray());
    QByteArray Acmd(Current_Letter);
    Acmd.append(anr.rightJustified(3, '0'));
    SENDCOMAND = QString(Acmd);
    QByteArray prepare(msg.simplified());
    if (cmd_prepend) {
        Acmd.append(" ");
        prepare.prepend(Acmd);
    }
    prepare.append("\r\n");
    QString log_client_send(QString("Client:%1").arg(prepare.simplified().constData()));
    Log_Handshake += log_client_send;
    ////out << str.fill('*', _IMAIL_WIDTH_) << "\n";
    ////out << log_client_send << "\n";
    ////out.flush();
    socket->write(prepare);
}
/// not constand to append rn carriage return

/*
 handler from error and wake up to search errors
 */

bool Net_StarterImap::Error_Handler_Stream_WakeUp(const QString rline) {
    QTextStream out(stdout);
    QString str("*");
    //// const QString rline = QString(line.constData());
    Log_Handshake += QString("Server:%1").arg(rline);
    QStringList error_codes;
    error_codes << "Invalid" << "AUTHENTICATIONFAILED" << "Failure" << "not parse";
    for (int i = 0; i < error_codes.size(); ++i) {
        QString error_words = QString(error_codes.at(i).toLocal8Bit());
        if (rline.contains(error_words, Qt::CaseInsensitive)) {
            _cmd->SetComand(Imap_Cmd::IMAP_Close);
            out << str.fill('*', _IMAIL_WIDTH_) << "\n";
            out << "Warning: " << error_words.toLatin1() << "\n";
            out << "Mistake line incomming: " << Format_st76(QString(rline));
            out << "Insert your user and pass on console main\nor take the input console script.\n";
            out << str.fill('*', _IMAIL_WIDTH_) << "\n";
            out.flush();
            Resume(__LINE__);
            socket->deleteLater();
            emit Message_Display(rline);
            emit Exit_Close();
            return false;
        }
    }
    return true;
}

/*
 Handle incomming streams from server and talk with him:
 only function Incomming_debug  Incomming_data
 */

void Net_StarterImap::Incomming_data() {
    QTextStream out(stdout);
    QString str = "*";
    quint16 in_status = 0;
    quint16 ch_status = 0;
    if (Current_Letter == "T" && readchunk) {
        return Incomming_mailstream(); /// incomming data to save as mail and chunk
    }
    while (socket->canReadLine()) {
        QByteArray biteline = socket->readLine();
        ///// const qint64 bitein = socket->encryptedBytesAvailable();
        ////qDebug() << "Serviersize IN:" << bitein << " -> " << __FUNCTION__;
        if (biteline.size() > 0) {
            int cpos = 0;
            const QString line = QString(biteline.constData()).simplified();
            if (line.size() > 0) {
                cpos = line.size();
            } else {
                return;
            }
            if (!Error_Handler_Stream_WakeUp(biteline)) {
                return;
            }
            /// capture the first clean char from line and handle
            QChar first = line.at(0);
            if (first.isLetter()) {
                QString Incomming_Letter(first);
                SERVERLETTER = Incomming_Letter.toLatin1();
                in_status = first.unicode();
                ch_status = 0;
            } else {
                in_status = 0;
                ch_status = first.unicode();
                SERVERLETTER = ""; //  QString();
            }

            out << str.fill('*', _IMAIL_WIDTH_) << "\n"; //// (CURRENTHANDLE) property(STEPS)
            out << Format_st76(line) << "\n";
            out << "STEP:" << steps() << "\n";
            out << "LETTER=(" << SERVERLETTER << ") ch_status=" << ch_status << " in_status=" << in_status << "\n";
            out << str.fill('*', _IMAIL_WIDTH_) << "\n";
            out.flush();


            if (ch_status != 42 && !SERVERLETTER.isEmpty()) {
                switch (STEPS) {
                    case CAPABILITY_WAIT:
                        if (SERVERLETTER == "C") {
                            create_steps(AUTHENTICATION_SEND_WAIT, QVariant("AUTHENTICATION_SEND_WAIT"));
                            _auth_login();
                        }
                        break;
                    case AUTHENTICATION_SEND_WAIT:
                        create_steps(AUTHENTICATION_SUCCESS, QVariant("AUTHENTICATION_SUCCESS"));
                        //// clean jobs 
                        q_Maildirlist.clear();
                        sendcmd = 0;
                        Current_Letter = "L";
                        _cmd->SetComand(Imap_Cmd::IMAP_Login_Ok);
                        emit Message_Display(line);
                        create_steps(LISTCOMAND_SEND_WAIT, QVariant("LISTCOMAND_SEND_WAIT"));
                        SendToServer("LIST \"\" \"*\"", true);
                        break;
                    case LISTCOMAND_SEND_WAIT:
                        Current_Letter = "G"; /// count mail ...... 
                        create_steps(INBOX_COUNT_SEND_WAIT, QVariant("INBOX_COUNT_SEND_WAIT"));
                        SendToServer("SELECT INBOX", true);
                        break;
                    case INBOX_COUNT_SEND_WAIT:
                        /// after count mail 
                        if (!query.isEmpty() && _cmd->Exists() > 0) { /// && 
                            Current_Letter = "S";
                            sendcmd = 100;
                            CursorGetPoint = 0;
                            SetQuery(query); //// SEARCH_RESULT_SEND_WAIT 
                        } else {
                            emit Next_Standby();
                        }
                        break;
                    case SEARCH_RESULT_SEND_WAIT:
                        //// next query 
                        if (_cmd->getUidSearch().size() > 0) {
                            //// qDebug() << "getUidSearch():" <<  _cmd->getUidSearch();
                            /// parse search result 
                            Current_Letter = "T";
                            create_steps(WAITFETCHMAIL_LONGCHUNK, QVariant("WAITFETCHMAIL_LONGCHUNK"));
                            GetMailUI(_cmd->getUidSearch().value(1)); /// go to the first item from search 
                        } else {
                            /// end or next search !!!
                            emit Next_Standby();
                        }
                        break;
                    default:
                        continue;
                }
            }
            //// here only if server response on start first letter ******* 
            if (ch_status == 42 && SERVERLETTER.isEmpty()) { /// sign * unicode 42 
                /// 1 CONNECT_0  CONNECT_0 
                /// switsch STEPS  CURRENTHANDLE STEPS;
                switch (STEPS) {
                    case CONNECT_0:
                        //// back from connect 
                        Current_Letter = "C";
                        create_steps(CAPABILITY_WAIT, QVariant("CAPABILITY_WAIT"));
                        SendToServer("CAPABILITY", true);
                        break;
                    default:
                        /// incomming results list search CAPABILITY ecc...
                        Untagged_Line(line);
                        continue;

                }

            }









        }

    }
}

void Net_StarterImap::ReadMail_UID_Body(const QString chunk) {

    QTextStream out(stdout);
    QString str('*');
    /// bool send_deflate = false;


    if (chunk.contains(_CHUNK_UIDMAIL_STOP_, Qt::CaseSensitive)) {
        out << str.fill('*', _IMAIL_WIDTH_) << "\n";
        if (_cmd->isCAPABILITY("DEFLATE")) {
            out << "SERVER CAN DEFLATE CHUNK!\n";
        }

        out << str.fill('*', _IMAIL_WIDTH_) << "\n";
        out.flush();
        ///imap_microsleep(100000);


        Resume(__LINE__);
    } else {
        if (maction == MAILGETHEADER) {
            ///CursorMail.append(chunk);
        } else if (maction == MAILGETHEADER) {
            ////CursorMail.append(chunk);
        }
    }
}


/// line start in * .......
///  not action here onnly read tag responser 

void Net_StarterImap::Untagged_Line(const QString line) {

    bool result;

    if (line.indexOf("EXISTS", 0) != -1) {
        int start = 0;
        //// total mail on box 
        /// QString token(QString text, int unicode); 
        QString temp = Utils::token(line, QChar(' ').unicode(), 0).simplified();
        quint32 exists = temp.toUInt(&result);
        //// qDebug() << "exists:" <<  exists;
        if (!result)
            exists = 0;
        _cmd->setExists(exists);

    } else if (line.indexOf("* LIST", 0) != -1) {
        q_Maildirlist.append(line);
    } else if (line.indexOf("RECENT", 0) != -1) {
        int start = 0;
        const int letterspace = QChar(' ').unicode();
        QString temp = Utils::token(line, letterspace, 0).simplified();
        quint32 recent = temp.toUInt(&result);
        if (!result)
            recent = 0;
        _cmd->setRecent(recent);
        qDebug() << "recent:" << recent;
    } else if (line.startsWith("* FLAGS")) {
        int start = 0;
        QString flags = Utils::token(line, 40, 41).simplified(); /// ())
        _cmd->setFlags(flags);
    } else if (line.indexOf("UIDVALIDITY", 0) != -1) {
        int start = 0;
        QString temp = Utils::token(line, 91, 93).simplified(); //// []
    } else if (line.indexOf("UIDNEXT", 0) != -1) {
        int start = 0;
        QString temp = Utils::token(line, 91, 93).simplified(); //// []
        QString nextStr = temp.mid(8);
        quint32 next = nextStr.toUInt(&result);
        if (!result)
            next = 0;
        _cmd->setUidNext(next);
    } else if (line.startsWith("* SEARCH") && step() == SEARCH_RESULT_SEND_WAIT) {
        //// S101 OK SEARCH completed (Success)
        QStringList search_results = line.split(QRegExp(" "), QString::SkipEmptyParts);
        if (search_results.size() > 2) {
            search_results.removeAt(0);
            search_results.removeAt(0);
        } else {
            search_results.clear();
        }
        _cmd->setSearchResult(search_results);
        qDebug() << "search_results:" << search_results;
    } else if (line.startsWith("* CAPABILITY", Qt::CaseSensitive)) {
        _cmd->setCAPABILITY(line);
    }
}

void Net_StarterImap::Incomming_mailstream() {
    bool next_steep = false;
    //// only  maction MAILGETBODY_BY_UID 
    QTextStream out(stdout);
    QString str('*');
    const int GetPointCorrect = CursorGetPoint + 1; //// start and go to  _cmd->getUidSearch().size()
    int rec = 0;
    while (socket->canReadLine()) {
        const QByteArray chunk = socket->readLine();
        if (chunk.startsWith("T500 OK Success")) {
            //// QString to_fileheader = _READMAILTMPDIR_ + QString("uid-%1.txt").arg(CursorUID_Get);
            QString HeaderClean = in_socket->HeaderMail();
            MailSession *mini = MailSession::instance();
            bool is_valid = mini->register_header(HeaderClean, CursorUID_Get);
            //// Utils::_write_file(to_fileheader,HeaderClean.toUtf8(),"utf-8");
            if (is_valid) {
                QString s_title = "No subject ";
                if (!mini->subject().isEmpty()) {
                    s_title = mini->subject();
                }
                if (!in_socket->WriteOnFile(mini->File_Fromuid(CursorUID_Get))) {
                    qFatal("NOt Possibel to save file..... !");
                }
                in_socket->clear();
                in_socket->start();
                out << str.fill('*', _IMAIL_WIDTH_) << "\n";
                out << mini->from() << "\n";
                out << s_title << " /size():" << HeaderClean.size() << "\n";
                out << "Sum:" << GetPointCorrect << ":" << _cmd->getUidSearch().size() << "\n";
                out.flush();
                emit Progress(CursorUID_Get);
            } else {
                out << str.fill('*', _IMAIL_WIDTH_) << "\n";
                out << "Not valid!: from:" << mini->from() << "\n";
            }
            //// flush chunk in qbuffer!!!
            NextMailUI();
        } else {
            //// register strema in 
            rec++;
            in_socket->device()->write(chunk);
            QTextStream xc(stdout);
            xc << "   Line:" << rec << "\r";
            xc.flush();

        }
    }
}

void Net_StarterImap::NextMailUI() {

    CursorGetPoint++;
    bool found = false;
    quint16 nextcursor = 0;
    int get = 0;
    /////qDebug() << "NextMailUI position:" << get << " from:" << _cmd->getUidSearch().size();
    if (CursorUID_Get > 0) {
        const quint16 last = CursorUID_Get;

        MailUIDResult finder_uid = _cmd->getUidSearch();
        QMapIterator<int, quint16> i(finder_uid);
        while (i.hasNext()) {
            i.next();
            ///// qDebug() << "NextMailUI:" << i.key() << ":" << i.value();
            if (i.value() == last) {
                get = i.key();
                get++;
            } else if (get == i.key()) {
                nextcursor = i.value();
                found = true;
            }
        }
    }

    /////qDebug() << "NextMailUI position:" << get << " from:" << _cmd->getUidSearch().size();

    /////

    if (nextcursor == CursorUID_Get) {
        qDebug() << "NextMailUI nextcursor :" << nextcursor << ":" << CursorUID_Get;
        qFatal("call same UID from cursor!");
    }
    if (nextcursor > 0 && found) {
        GetMailUI(nextcursor, MAILGETBODY_BY_UID);
    } else {
        GetMailUI(0, MAILNONE);
    }


}

void Net_StarterImap::GetMailUI(const quint32 uid, GETACTIONCURSOR cursornow) {
    maction = cursornow; /// get header or text
    CursorUID_Get = uid;
    create_steps(WAITFETCHMAIL_LONGCHUNK, QVariant("WAITFETCHMAIL_LONGCHUNK"));
    ///" (BODY[])"   (RFC822.HEADER) (body.peek[text])
    /// buono BODY.PEEK[HEADER.FIELDS (Subject)] 
    readchunk = true;
    sendcmd = 499;
    Current_Letter = "T";
    if (uid == 0) {
        readchunk = false;
        /// end or next search !!!
        emit Next_Standby();
        return;
    }
    QByteArray next_cmd("UID FETCH "); /// UID FETCH  (BODY[]) conform
    QString nummero = QString("%1").arg(uid);
    next_cmd.append(nummero.toLatin1());
    next_cmd.append(" (BODY[])");
    SendToServer(next_cmd, true);
}


/// like a reset to clear action

void Net_StarterImap::PreparetoClose() {
    Resume(__LINE__);
}

void Net_StarterImap::Resume(const int at) {

    QTextStream out(stdout);
    QString str('*');
    /// in_socket stay open in case of reconnect!!!
    in_socket->updateStatus();


    out << str.fill('*', _IMAIL_WIDTH_) << "\n";
    if (_cmd->isCAPABILITY("DEFLATE")) {
        out << "SERVER CAN DEFLATE CHUNK!\n";
    } else {
        out << "SERVER !NOT DEFLATE CHUNK!\n";
    }

    out << "Resume from this one job.\n" << _READMAILTMPDIR_ << "\n";
    for (int j = 0; j < ResumeMail.length(); j++) {
        out << ResumeMail[j] << "\n";
    }
    out << "Resume comand from __LINE__:" << at << "\n";
    out << str.fill('*', _IMAIL_WIDTH_) << "\n";
    out.flush();
    //// create_steps(CONNECTION_STOP_ERROR,QVariant("CONNECTION_STOP_ERROR"));


    sendcmd = 0;
    maction = MAILNONE;
    readchunk = false;
    if (socket) {
        socket->deleteLater();
    }
    _cmd->setSearchResult(QStringList());
    _cmd->SetComand(Imap_Cmd::IMAP_Unconnected);
    create_steps(CONNECTION_STOP_ERROR, QVariant("CONNECTION_STOP_ERROR"));
    emit Exit_Close();



}

void Net_StarterImap::SetQuery(const QString word) {

    QLocale::setDefault(QLocale::English);
    const QByteArray stringtosearch = QByteArray(word.toUtf8());
    if (before > 0) {
        before = -1;
    }
    //// date search format 20-Sep-2013 
    QDateTime now = QDateTime::currentDateTime();
    QDateTime lastday = now.addDays(before);
    qint64 unixtime = lastday.toTime_t();
    const QString sinceday = Utils::date_imap_format(unixtime);
    //////qDebug() << sinceday;
    ////// qFatal("call same UID from cursor!");
    /// correct MMM is  QLocale::setDefault(QLocale::English);
    ///Jan" / "Feb" / "Mar" / "Apr" / "May" / "Jun" /
    //// "Jul" / "Aug" / "Sep" / "Oct" / "Nov" / "Dec"
    Current_Letter = "S";
    sendcmd = 100;
    CursorGetPoint = 0;

    ////QDateTime now = QDateTime::currentDateTime();
    ////QDateTime lastday = now.addDays(before);
    ////const QString searchsince = lastday.toString("dd-MMM-yyyy");
    //// CHARSET utf-8 ///HEADER Subject
    /////QByteArray searchw("UID SEARCH HEADER SUBJECT "); ok
    QByteArray searchw("UID SEARCH SUBJECT \"");
    searchw.append(stringtosearch);
    searchw.append("\" SINCE ");
    searchw.append(sinceday.toUtf8()); //// 20-Sep-2013"
    /// correct QByteArray searchw("UID SEARCH HEADER SUBJECT ");
    //// QByteArray searchw("UID SEARCH CHARSET utf-8 SUBJECT ");
    ////QString nummero = QString("\"%1\"").arg(word);
    ////searchw.append(nummero.toUtf8());
    ////searchw.append(" SINCE "); ///" (BODY[])"
    /////searchw.append(searchsince);
    //// correct and run:
    ///// UID SEARCH SUBJECT \"foto\" SINCE 20-Sep-2013

    ///// QByteArray searchw("UID SEARCH SUBJECT \"foto\" SINCE 20-Sep-2013"); /// run ok
    ////searchw.append(" SINCE "); ///" (BODY[])"
    /////searchw.append(searchsince);
    create_steps(SEARCH_RESULT_SEND_WAIT, QVariant("SEARCH_RESULT_SEND_WAIT"));

    SendToServer(searchw, true);
}

void Net_StarterImap::Ready_encrypted() {

}

void Net_StarterImap::OnDisconnected() {
    _cmd->SetComand(Imap_Cmd::IMAP_Unconnected);
    qDebug() << "SERVER QUIT:" << __FUNCTION__;
}

void Net_StarterImap::SockBytesWritten(qint64 written) {
    qDebug() << "Net_StarterImap-clock:" << __FUNCTION__ << " byte:" << written;
}

void Net_StarterImap::SomeErrorincomming(const QSslError & error) {
    qDebug() << "Net_StarterImap-clock:" << __FUNCTION__ << " error: " << error.errorString();
}

void Net_StarterImap::SearchWord(QString w, const int day) {
    //// qDebug() << "SearchWord:" << w;
    if (w.size() > 3) {
        query = w;
        before = day;
    }
}

void Net_StarterImap::_auth_login() {
    create_steps(AUTHENTICATION_SEND_WAIT, QVariant("AUTHENTICATION_SEND_WAIT"));
    Current_Letter = "A";
    QByteArray ba;
    ba.append('\0');
    ba.append(username.toUtf8());
    ba.append('\0');
    ba.append(password.toUtf8());
    QByteArray encoded = ba.toBase64();
    encoded.prepend("AUTHENTICATE PLAIN ");
    _cmd->SetComand(Imap_Cmd::IMAP_SendLogin);
    SendToServer(encoded, true);
}


//// SearchWord(word,2);


