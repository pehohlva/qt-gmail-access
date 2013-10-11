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
#include "deflate_chunk_incomming.h"

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
    CMail = new IMail();
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
    STEPS = UNKNOWSTATUS;
    _cmd->SetComand(Imap_Cmd::IMAP_Unconnected);
}

void Net_StarterImap::Connect(const QString user, const QString word) {
    username = user;
    password = word;
    socket = new QSslSocket(0);
    connect(socket, SIGNAL(encrypted()), this, SLOT(Ready_encrypted()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(Incomming_data()));
    socket->connectToHostEncrypted("imap.googlemail.com", 993);
    //// socket->connectToHost("imap.googlemail.com", 993);
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

    sendcmd++;
    QVariant nr(sendcmd);
    QByteArray anr(nr.toByteArray());
    QByteArray Acmd(Current_Letter);
    Acmd.append(anr.rightJustified(3, '0'));
    SENDCOMAND = QString(Acmd);
    QByteArray prepare(msg);
    
    if (cmd_prepend) {
        Acmd.append(" ");
        prepare.prepend(Acmd);
    }
    
    if (newline && Talk != DEFLATE_TALK ) {
        /// not append carriage return!!!
        prepare.append("\r\n");
    }
    
    Log_Handshake += QString("Client:%1").arg(prepare.constData());
    out << str.fill('*', _IMAIL_WIDTH_) << "\n";
    out << "Client send:\n";
    if (Talk != DEFLATE_TALK) {
    out << Format_st76(QString(msg.constData()));
    } else {
      out << Format_st76(QString(compress_byte(prepare).constData()));  
    }
    out << "\n";
    out << str.fill('*', _IMAIL_WIDTH_) << "\n";
    out.flush();
    if (Talk != DEFLATE_TALK) {
        socket->write(prepare);
    } else {
        /// compress
        QByteArray compress = compress_byte(prepare);
        compress.append("\r\n");
        socket->write(compress);
    }
    
}
/// not constand to append rn carriage return
QByteArray Net_StarterImap::compress_byte(const QByteArray& uncompressed )
{
    QByteArray deflated = qCompress(uncompressed);
    // eliminate qCompress size on first 4 bytes and 2 byte header
    deflated = deflated.right(deflated.size() - 6);
    // remove qCompress 4 byte footer
    deflated = deflated.left(deflated.size() - 4);

    QByteArray header;
    header.resize(10);
    header[0] = 0x1f; // gzip-magic[0]
    header[1] = 0x8b; // gzip-magic[1]
    header[2] = 0x08; // Compression method = DEFLATE
    header[3] = 0x00; // Flags
    header[4] = 0x00; // 4-7 is mtime
    header[5] = 0x00;
    header[6] = 0x00;
    header[7] = 0x00;
    header[8] = 0x00; // XFL
    header[9] = 0x03; // OS=Unix
    
    deflated.prepend(header);

    QByteArray footer;
    quint32 crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const uchar*)uncompressed.data(), uncompressed.size());
    footer.resize(8);
    footer[3] = (crc & 0xff000000) >> 24;
    footer[2] = (crc & 0x00ff0000) >> 16;
    footer[1] = (crc & 0x0000ff00) >> 8;
    footer[0] = (crc & 0x000000ff);

    quint32 isize = uncompressed.size();
    footer[7] = (isize & 0xff000000) >> 24;
    footer[6] = (isize & 0x00ff0000) >> 16;
    footer[5] = (isize & 0x0000ff00) >> 8;
    footer[4] = (isize & 0x000000ff);
    deflated.append(footer);
    
    return deflated;
}

/*
 handler from error and wake up to search errors
 */

bool Net_StarterImap::Error_Handler_Stream_WakeUp(const QString rline) {
    QTextStream out(stdout);
    QString str("*");
    //// const QString rline = QString(line.constData());
    Log_Handshake += QString("Server:%1").arg(rline);
    QStringList error_codes;
    error_codes << "Invalid" << "AUTHENTICATIONFAILED" << "Failure";
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
        if (biteline.size() > 0) {
            int cpos = 7;
            /// check if compressed? to decompress data stream
            bool is_compressed = ChunkDeflate::gzipQByte(biteline, cpos);
            if (is_compressed) {
                qDebug() << "SERVER go on GZIP deflate:" << biteline << " -> " << __FUNCTION__;
            }



            const QString line = QString(biteline.constData());
            qDebug() << "SERVER:" << biteline << " -> " << __FUNCTION__;
            if (!Error_Handler_Stream_WakeUp(biteline)) {
                return;
            }

            /// capture the first clean char from line and handle
            if (line.size() > 0) {
                QChar first = line.at(0);
                if (first.isLetter()) {
                    QString Incomming_Letter(first);
                    SERVERLETTER = Incomming_Letter.toLatin1();
                    in_status = first.unicode();
                } else {
                    ch_status = first.unicode();
                }
            } else {
                return;
            }
            ///// uncomment here to see all incoming data wo start by "*"!
            if (ch_status != 42) {
                out << str.fill('*', _IMAIL_WIDTH_) << "\n"; //// (CURRENTHANDLE)
                out << "*Clean Line IN STEPS:'" << STEPS << "' LETTER='" << SERVERLETTER << "' *\n";
                out << "On {CH_STATUS:'" << ch_status << "' IN_STATUS='" << in_status << "'} Server response:\n";
                out << Format_st76(line);
                out << "\n";
                out << str.fill('*', _IMAIL_WIDTH_) << "\n";
                out.flush();
            }

            //// Server ready talk to him;
            /// first line incomming after connect or untagged line beginning "*"
            /// EXAMPLE "* OK Gimap ready for requests"  "* LIST" ecc.
            if (ch_status == 42) {
                STEPS = CONNECTION_LIVE;
                Untagged_Line(biteline);
                if (line.contains("* OK Gimap ready for requests", Qt::CaseInsensitive)) {
                    Current_Letter = "C";
                    STEPS = CAPABILITY_WAIT;
                    SendToServer("CAPABILITY", true);
                }
            }

            ///// important read next line !!!!!

            /// Letter C after capacity send //// C001 OK Thats all she wrote! r1if4988682eeo.216
            if (in_status == 67 && line.contains("OK", Qt::CaseInsensitive)) {
                STEPS = AUTHENTICATION_SEND_WAIT;
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

            }                /// from A
            else if (in_status == 111111111165 && line.contains("OK", Qt::CaseInsensitive) &&
                    line.contains(username, Qt::CaseInsensitive)) {
                ////compress deflate
                if (_cmd->isCAPABILITY("DEFLATE")) {
                    Current_Letter = "Z"; // 90
                    sendcmd = 0;
                    STEPS = DEFLATE_SEND_WAIT;
                    //// Talk = NO_ACTIVE wait the ok!
                    /////  From this point on, everything is compressed before being
                        ///// encrypted.
                    SendToServer("COMPRESS DEFLATE", true);
                } else {
                    //// exit tmp
                    qFatal("Capacity deflate to incomming ");
                }

            }  /// deflate success or auth 
            else if (in_status == 242334324290 && line.contains("OK", Qt::CaseInsensitive) && STEPS == DEFLATE_SEND_WAIT ) {
                Talk = DEFLATE_TALK;
                //// send compressed list!
                sendcmd = 0;
                Current_Letter = "L";
                SendToServer("LIST \"\" \"*\"", true);
            }
                /// Letter A after capacity send 
            else if (in_status == 65 && line.contains("OK", Qt::CaseInsensitive)) {
                /// back from AUTHENTICATION_SEND_WAIT 
                STEPS = AUTHENTICATION__SUCCESS; /// pass & user is ok
                /// save user e pass here if like
                q_Maildirlist.clear();
                sendcmd = 0;
                Current_Letter = "L";
                _cmd->SetComand(Imap_Cmd::IMAP_Login_Ok);
                emit Message_Display(line);
                /// send a list comand to know message total on all maildir && maildir name 
                STEPS = LISTCOMAND_SEND_WAIT;
                SendToServer("LIST \"\" \"*\"", true);
            }//// letter L
            else if (in_status == 76 && line.contains("OK", Qt::CaseInsensitive)) {
                Current_Letter = "G"; /// M SELECT INBOX
                /// UID SEARCH RECENT  / SEARCH X-GM-EXT-1 SUBJECT \"FW\"
                /////SendToServer("SEARCH ANSWERED", true);
                STEPS = INBOX_COUNT_SEND_WAIT;
                SendToServer("SELECT INBOX", true);
            }/// letter G  begin to search if having pending search comand 
            else if (in_status == 71 && line.contains("OK", Qt::CaseInsensitive)) {
                Current_Letter = "S";
                //// ui UID SEARCH * SEARCH 11766 11767 11770 11772 11774 
                ///  11777 11779 11781 11784 11795 11797 11799 11805 11806 11807 11810
                QDateTime now = QDateTime::currentDateTime();
                QDateTime lastday = now.addDays(before);
                const QString searchsince = lastday.toString("dd-MMM-yyyy");
                sendcmd = 100;
                QByteArray searchw("UID SEARCH SUBJECT ");
                QString nummero = QString("\"%1\"").arg(query);
                searchw.append(nummero.toLatin1());
                searchw.append(" SINCE "); ///" (BODY[])"
                searchw.append(searchsince);
                ///// searchw.append(" ON INBOX");
                //// UID SEARCH SUBJECT \"foto\" SINCE 20-Sep-2013
                STEPS = SEARCH_RESULT_SEND_WAIT;
                CursorGetPoint = 0;
                SendToServer(searchw, true);
            }///  letter S  back from search mail 
            else if (in_status == 83 && line.startsWith("S101 OK SEARCH completed")) {
                MailUIDResult finder_uid = _cmd->getUidSearch();
                out << str.fill('*', _IMAIL_WIDTH_) << "\n";
                out << "Search Results Total:" << finder_uid.size() << "\n";
                out << str.fill('*', _IMAIL_WIDTH_) << "\n";
                out.flush();
                /// imap_microsleep(100000);
                if (finder_uid.size() > 0) {
                    GetMailUI(finder_uid.value(1)); /// go to the first item from search 
                } else {
                    Resume(__LINE__); /// no search results go to await next comand 
                }
            }/// letter M  select from inbox code 
            else if (in_status == 77 && line.contains("OK", Qt::CaseInsensitive) &&
                    line.contains("(Success)", Qt::CaseInsensitive)) {
                qDebug() << "-------not implemented grep from mail inbox!!!  " << TotalMail;
                if (TotalMail > 0) {
                    sendcmd = 500;
                    Current_Letter = "U";
                    readchunk = true;
                    //// NextBody(1);
                    //// Resume(__LINE__);
                } else {
                    ///  Resume(__LINE__);
                }
                Resume(__LINE__); /// wait this action 
            }/// letter U  select from inbox finisch code 
            else if (in_status == 85 && line.contains("OK", Qt::CaseInsensitive) &&
                    line.contains("Success", Qt::CaseInsensitive)) {
                qDebug() << "-------end mail get..";
                Resume(__LINE__); /// wait this action 
            } else {
                /* 
                out << str.fill('*', _IMAIL_WIDTH_) << "\n";
                out << "Wait your comand.....\n";
                out << str.fill('*', _IMAIL_WIDTH_) << "\n";
                out.flush();
                 */

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


        out << "All search results mail ist locate on:\n" << _TMPMAILDIR_ << "\n";
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
        QString temp = token(line, ' ', ' ', &start);
        quint32 exists = temp.toUInt(&result);
        if (!result)
            exists = 0;
        _cmd->setExists(exists);
    } else if (line.indexOf("* LIST", 0) != -1) {
        q_Maildirlist.append(line);
    } else if (line.indexOf("RECENT", 0) != -1) {
        int start = 0;
        QString temp = token(line, ' ', ' ', &start);
        quint32 recent = temp.toUInt(&result);
        if (!result)
            recent = 0;
        _cmd->setRecent(recent);
    } else if (line.startsWith("* FLAGS")) {
        int start = 0;
        QString flags = token(line, '(', ')', &start);
        _cmd->setFlags(flags);
    } else if (line.indexOf("UIDVALIDITY", 0) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        /// _cmd->setUidValidity(temp.mid(12).trimmed());
    } else if (line.indexOf("UIDNEXT", 0) != -1) {
        int start = 0;
        QString temp = token(line, '[', ']', &start);
        QString nextStr = temp.mid(8);
        quint32 next = nextStr.toUInt(&result);
        if (!result)
            next = 0;
        _cmd->setUidNext(next);
    } else if (line.startsWith("* SEARCH") && Current_Letter == "S") {
        //// S101 OK SEARCH completed (Success)
        QStringList search_results = line.split(QRegExp(" "), QString::SkipEmptyParts);
        if (search_results.size() > 2) {
            search_results.removeAt(0);
            search_results.removeAt(0);
        } else {
            search_results.clear();
        }
        _cmd->setSearchResult(search_results);
    } else if (line.startsWith("* CAPABILITY", Qt::CaseSensitive)) {
        _cmd->setCAPABILITY(line);
    }
}

void Net_StarterImap::Incomming_mailstream() {
    bool next_steep = false;

    QTextStream out(stdout);
    QString str('*');
    const int GetPointCorrect = CursorGetPoint + 1;
    int rec = 0;
    while (socket->canReadLine()) {
        const QByteArray chunk = socket->readLine();
        if (chunk.startsWith("T500 OK Success")) {
            out << str.fill('*', _IMAIL_WIDTH_) << "\n";
            out << Format_st76(chunk.constData());
            out << "\n";
            out << "Sum:" << GetPointCorrect << ":" << _cmd->getUidSearch().size() << "\n";
            out.flush();
            out << "\n";
            out.flush();
            if (maction == MAILGETHEADER) {
                out << "END Mail-Header UID:" << CursorUID_Get << "\n";
            } else if (maction == MAILGETBODY) {
                bool is_onfile = false;
                out << "END Mail-Body UID:" << CursorUID_Get << "\n";
                const QString tmpeml = CMail->Filepointer();
                if (!tmpeml.isEmpty()) {
                    if (in_socket->PutOnEml(tmpeml)) {
                        is_onfile = true;
                    }
                }
                if (is_onfile) {
                    out << "Mail is saved on file:\n" << tmpeml << "\n";
                } else {
                    out << "Error NOT! on file:\n" << tmpeml << "\n";
                    out.flush();
                    qFatal("File unable to write on disk!");
                }
                CMail->flush();
                out << str.fill('*', _IMAIL_WIDTH_) << "\n";
                out.flush();
                ///// qFatal("work place stop :-)  hi");
            }
            out.flush();
            next_steep = true;
        } else {
            rec++;
            in_socket->device()->write(chunk);
            QTextStream xc(stdout);
            xc << "   Line:" << rec << "\r";
            xc.flush();
            // revision svn r7 not sleep here
            /// memory for big file running perfect..
            /// incomming mail of 5-7 MB is ok and running ok.
            /// QBuffer from mail_handler.h is a great job..
            /// large file is better from stream to strem
            ////imaplist_microsleep(1000000);
        }
    }

    if (next_steep) {
        ////qDebug() << "pos:" << __FUNCTION__ << ":" << __LINE__;
        if (maction == MAILGETHEADER) {
            CMail = NULL;
            CMail = new IMail();
            CMail->Setheader(in_socket->stream(), QString("uid-%1-user-%2-").arg(CursorUID_Get).arg(username));
            out << "Subject:\n";
            out << Format_st76(CMail->Subject());
            out << "\n";
            out.flush();

            ResumeMail.append(QString("/%1/").arg(CursorUID_Get) + CMail->Subject());
            int sum_out = ResumeMail.removeDuplicates();
            if (sum_out > 0) {
                qDebug() << "sum_out:" << sum_out;
                qFatal("Duplicate subject!!");
            }

            if (CMail->exist()) {
                out << Format_st76(CMail->PrintInfo());
                out << "\n";
                out << str.fill('*', _IMAIL_WIDTH_) << "\n";
                out.flush();
                in_socket->clear();
                NextMailUI();
                return;
                //// qDebug() << "pos:" << __FUNCTION__ << ":" << __LINE__;
            } else {
                /// chunk is saved on file 
                in_socket->clear();
                GetMailUI(CursorUID_Get, MAILGETBODY);
                return;
            }
            //// qDebug() << "pos:" << __FUNCTION__ << ":" << __LINE__;
        } else if (maction == MAILGETBODY) {
            in_socket->clear();
            NextMailUI();
            return;
        }

    }

    //// qDebug() << "pos:" << __FUNCTION__ << ":" << __LINE__;
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
        GetMailUI(nextcursor, MAILGETHEADER);
    } else {
        GetMailUI(0, MAILNONE);
    }


}

void Net_StarterImap::GetMailUI(const quint32 uid, GETACTIONCURSOR cursornow) {
    maction = cursornow; /// get header or text
    CursorUID_Get = uid;
    ///" (BODY[])"   (RFC822.HEADER) (body.peek[text])
    readchunk = true;
    sendcmd = 499;
    Current_Letter = "T";

    if (uid == 0) {
        readchunk = false;
        Resume(__LINE__);
        return;
    }
    QByteArray next_cmd("UID FETCH ");
#ifdef DEFLATE_PLAY_YES
    /// prepend deflate cmd for all big mail body!!!
    //// send comand -> COMPRESS DELFATE  *****
    if (_cmd->isCAPABILITY("DEFLATE")) {
        //// next_cmd.prepend("DEFLATE  ");
    }
#endif


    QString nummero = QString("%1").arg(uid);
    next_cmd.append(nummero.toLatin1());
    if (cursornow == MAILGETHEADER) {
        /////qDebug() << "run MAILGETHEADER:" << __FUNCTION__ << ":" << __LINE__ << "param:" << uid;
        next_cmd.append(" (body.peek[header])");
        CMail->flush();
        SendToServer(next_cmd, true);
    } else if (cursornow == MAILGETBODY) {
        ////qDebug() << "run MAILGETBODY:" << __FUNCTION__ << ":" << __LINE__ << "param:" << uid;
        next_cmd.append(" (BODY[])");
        SendToServer(next_cmd, true);
    }
}


/// like a reset to clear action

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

    out << "Resume from this one job.\n" << _TMPMAILDIR_ << "\n";
    for (int j = 0; j < ResumeMail.length(); j++) {
        out << ResumeMail[j] << "\n";
    }
    out << "Resume comand from __LINE__:" << at << "\n";
    out << str.fill('*', _IMAIL_WIDTH_) << "\n";
    out.flush();



    sendcmd = 0;
    maction = MAILNONE;
    readchunk = false;
    _cmd->i_debug();
    if (socket) {
        socket->deleteLater();
    }
    _cmd->setSearchResult(QStringList());
    _cmd->SetComand(Imap_Cmd::IMAP_Unconnected);
    emit Exit_Close();



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


//// SearchWord(word,2);


