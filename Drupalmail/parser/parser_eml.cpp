//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution


#include "parser_eml.h"
#include "parser_config.h"
#include "mime_standard.h"
#include "kcodecs.h"

namespace ReadMail {

    Parser::Parser(const QString file)
    : wi_line(76) {

        _d = new StreamMail();
        MAGIC_POSITION = 1;
        if (_d->LoadFile(file)) {
            qFatal("Unable to open current file!");
        }
        init_Header();
    }

    QString Parser::HeaderField(const QString name) const {
        if (name.isEmpty()) {
            return QString("Empty Value.");
        }
        /// is saved all lovercase make firstupper if like
        const QString search = QString(name).toLower();
        QMap<QString, QString>::const_iterator pos;
        for (pos = field_h.constBegin(); pos != field_h.constEnd(); ++pos) {
            if (search == pos.key()) {
                return QString(pos.value());
            }
        }
        /// not find action to handle correct
        if (name == "Content-Type") {
            return PHPMAILERFROMSERVER;
        }

        return QString();
        ////return QString("404 Name:{%1}").arg(name); ///  + QString("Unknow Mail Format Name: {%1}\n").arg(name);
        /*
         * sample iterator from tag
         QMap<QString, QString>::const_iterator pos;
        for (pos = field_h.constBegin(); pos != field_h.constEnd(); ++pos) {
            out << pos.key() << " - " << pos.value() << "\n";
            out << str.fill('-', wi_line) << "\n";
        }
         */
    }

    void Parser::init_Header() {
        _d->start(); //// seek zero
        int cursor = 0;
        ///QTextCodec *codecx;
        // codecx = QTextCodec::codecForMib(106);
        QTextStream out(stdout);
        QString str("-");
        /// out.setCodec(codecx);
        QByteArray onlyMETAHEADER, afterMETAHEADER, realyFullMail = QByteArray(_FILESTARTER_);
        QString last_chunk;
        QString boundary_back_up; /// util to find only plain text mail!!! 
        /////  if is null size here after header old format NO boundary.
        const int must_having = 15; /// not stop header from null line on first xx line!!
        int limit_cursor = 50; /// header section 
        /// if on 50 line grep true a boundary $limit_cursor append $must_having
        // to find the second boundary if having!!
        ///canread_body = false;
        int nullcursor_at = 12; //// count null line to discovery plain text mail
        int boundary_counter = 0;
        QRegExp boundaryMatch("boundary=[\\S\"\'](.*)[\"\'\\s]");
        bool START_READ_BODY = false;
        while (_d->device()->canReadLine()) {
            QByteArray chunk = _d->device()->readLine();
            realyFullMail.append(chunk); //// read all to query position here...
            if (START_READ_BODY) {
                afterMETAHEADER.append(chunk); //// read only after meta header
                /// to have plain text clean mail after the first null line on meta mail
            }
            const QByteArray searchnull = chunk.simplified(); /// search null Line / no data
            int _size_line = searchnull.size(); // not more as 76 ???
            /// count line 
            cursor++;
            if (_size_line != 0) {
                //// line + data
                if (cursor < limit_cursor && !START_READ_BODY) {
                    bool X_continue = true;
                    if (cursor > limit_cursor) {
                        X_continue = false;
                    }
                    if (X_continue) {
                        bool key_doc_found = false;
                        last_chunk = QString(chunk.constData()); /// last data line IN:
                        onlyMETAHEADER.append(chunk);
                        if (boundaryMatch.indexIn(last_chunk) != -1) {
                            limit_cursor = cursor + 7;
                            key_doc_found = true;
                            ////out << "Match xx C." << limit_cursor << ":" << cursor << "\n";
                        } else if (last_chunk.indexOf("boundary=") != -1) {
                            limit_cursor = cursor + 7;
                            key_doc_found = true;
                            /// out << "Match index C." << limit_cursor << ":" << cursor << "\n";
                        }
                        if (key_doc_found) {
                            boundary_back_up.append(last_chunk);
                            boundary_back_up.append("\n");
                            boundary_counter++;
                            //// debug ctr out << "HEADER boundary_counter >" << boundary_counter << " on line> " << cursor << "  -\n";
                        }
                        if (boundary_counter > 1) {
                            limit_cursor = 10; /// stop
                            START_READ_BODY = true;
                            afterMETAHEADER.append("Meta-------\n");
                            //// stop to read header info!!!
                            //// body can start to read if like
                        }

                        last_chunk.clear();
                    }
                }
            } else {

                nullcursor_at = qMax(cursor, nullcursor_at);
                if (cursor > 18 && !START_READ_BODY) {
                    /// count null line ??
                    if (cursor < limit_cursor) {
                        ////// limit_cursor = nullcursor_at + must_having;
                        //// debug ctr out << "HEADER null byte status  read>" << cursor << "-------------\n";
                    } else {
                        ///// out << "HEADER status NOT read header ready :-)---on" << cursor << "-------------\n";
                    }
                }
                if (!START_READ_BODY) {
                    if (cursor > 20 && cursor < limit_cursor) {
                        /// check to read body 
                        if (nullcursor_at > 20) {
                            //// debug ctr out << "CHECK to read body >" << cursor << "-------------\n";
                            if (boundary_back_up.size() < 1) {
                                //// debug ctr out << "FAST READ body now  >" << cursor << "-------------\n";
                                START_READ_BODY = true;
                                //// meta header for plain text mail message
                                //// first line this text haha..
                                afterMETAHEADER.append("--Text Message--\n\n");
                            }
                        }
                    }
                }
            }
            /////out << "NULL byte LINE --------on" << cursor << "--------------------------------\n";
            ///// this PLAY only after NULL byte LINE! or space NULL
        }

        realyFullMail.append(_FILECLOSER_);
        onlyMETAHEADER.append(_FILECLOSER_); ////to search index of tag better
        ////out << str.fill('-', wi_line) << "\n";
        ////out << afterMETAHEADER << "\n";
        //////out << str.fill('-', wi_line) << "\n";
        //// out.flush();
        //// /Users/pro/project/version_sv/GmailHand/
        //// 6 okt. 2013 save on svn all running file to take out better the meta mail header from mixed format here 
        ///qDebug() << "key_stop  chunk body(" << Mbody << ")";
        ///// qFatal("SCAN SCAN SCAN SCAN SCAN");
        ///// swap here to search META MAIL  ////
        Mfull = afterMETAHEADER;
        Mheader = onlyMETAHEADER;
        Mfull_A = realyFullMail;
        Utils::_writebin_tofile("zcurrent.txt", realyFullMail);
        ///// swap here to search META MAIL  ////
        QRegExp messageID("Message-ID: +(.*)");
        QRegExp messageID1("Message-Id: +(.*)");
        QRegExp messageID2("Message-id: +(.*)");
        QRegExp subjectMatch("Subject: +(.*)");
        QRegExp fromMatch("From: +(.*)");
        QRegExp cccMatch("Cc: +(.*)");
        QRegExp dateMatch("Date: +(.*)");
        QRegExp mimeMatch("MIME-Version: +(.*)");
        QRegExp ContentT("Content-Type: +(.*)");
        QRegExp Delivto("Delivered-To: +(.*)");
        QRegExp Senderm("Sender: +(.*)");
        QRegExp Tomailm("To: +(.*)");
        QRegExp Returnmail("Return-Path: +(.*)");
        QRegExp Useragentm("User-Agent: +(.*)");
        /// Content-Transfer-Encoding:
        //// Content-Transfer-Encoding  User-Agent:
        QRegExp transferencodingMatch("Content-Transfer-Encoding: +(.*)");
        QRegExp SenderHostMatch("Received: +(.*)");
        ////  /Users/pro/project/version_sv/GmailHand/
        ///// decode header from utf8 subject or iso eccc...
        const QString clean_header = ReadMail::decodeWordSequence(onlyMETAHEADER) + QString("\n\rLOCK***LAST***");
        QString Subject, body_decoded, from, date, msgid, mime, Content_Type, real_line = "";
        // out << "Header size:" << clean_header.size() << "\n";
        //// out << "Header size:" << clean_header << "\n";
        quint16 lastposition = 0;
        //// if one from the first line is null not read!!!
        /// to find body init or start!!!
        /// recompose header on new line to read each field 
        QString line = "";
        QStringList recivedBy;
        //// QString::KeepEmptyParts
        QStringList h_lines = clean_header.split(QRegExp("(\\r\\n)|(\\n\\r)|\\r|\\n"), QString::SkipEmptyParts);
        for (int i = 0; i < h_lines.size(); ++i) {
            const QString const_line = QString(h_lines.at(i).toAscii().constData());
            QChar fchar(const_line.at(0));

            if (fchar.isUpper()) {
                bool insert_ok = true;
                line = "";
                if (const_line.startsWith(QLatin1String("LOCK***LAST**"))) {
                    /// last line from header here!!
                    insert_ok = false;
                }

                ////out << "\n!!U!!" << line << "!!!!!!\n";
                if (insert_ok) {
                    body_decoded.append(const_line);
                    line = const_line.simplified();
                    if (real_line.simplified().size() > 0) {
                        body_decoded.insert(lastposition, real_line);
                        line.append(real_line);
                        /////out << "\n!!insert->!!" << real_line << "!!\n";
                    }

                }


                if (!insert_ok) {
                    if (real_line.simplified().size() > 0) {
                        body_decoded.insert(lastposition, real_line);
                        line.append(real_line);
                    }
                    body_decoded.append("\n");
                }
                /////out << "!" << i << "!" << line << "!!\n";

                //// query here start 
                if (subjectMatch.indexIn(line) != -1) {
                    Subject = subjectMatch.cap(1).simplified(); /// Subject
                    field_h.insert(QString("Subject").toLower(), Subject);
                }
                if (dateMatch.indexIn(line) != -1) {
                    field_h.insert(QString("Date").toLower(), dateMatch.cap(1).simplified());
                }
                if (Useragentm.indexIn(line) != -1) {
                    field_h.insert(QString("User-Agent").toLower(), Useragentm.cap(1).simplified());
                }
                if (transferencodingMatch.indexIn(line) != -1) {
                    field_h.insert(QString("Content-Transfer-Encoding").toLower(), transferencodingMatch.cap(1).simplified());
                }
                /// 
                if (messageID.indexIn(line) != -1) {
                    field_h.insert(QString("Message-ID").toLower(), messageID.cap(1).simplified());
                }
                if (messageID1.indexIn(line) != -1) {
                    field_h.insert(QString("Message-ID").toLower(), messageID1.cap(1).simplified());
                }
                if (messageID2.indexIn(line) != -1) {
                    field_h.insert(QString("Message-ID").toLower(), messageID2.cap(1).simplified());
                }
                if (fromMatch.indexIn(line) != -1) {
                    field_h.insert(QString("From").toLower(), fromMatch.cap(1).simplified());
                }
                if (mimeMatch.indexIn(line) != -1) {
                    field_h.insert(QString("MIME-Version").toLower(), mimeMatch.cap(1).simplified());
                }

                if (Delivto.indexIn(line) != -1) {
                    field_h.insert(QString("Delivered-To").toLower(), Delivto.cap(1).simplified());
                }
                if (Senderm.indexIn(line) != -1) {
                    field_h.insert(QString("Sender").toLower(), Senderm.cap(1).simplified());
                }
                if (Tomailm.indexIn(line) != -1) {
                    field_h.insert(QString("To").toLower(), Tomailm.cap(1).simplified());
                }

                if (Returnmail.indexIn(line) != -1) {
                    field_h.insert(QString("Return-Path").toLower(), Returnmail.cap(1).simplified());
                }

                if (cccMatch.indexIn(line) != -1) {
                    field_h.insert(QString("Cc").toLower(), cccMatch.cap(1).simplified());
                }

                if (SenderHostMatch.indexIn(line) != -1) {
                    recivedBy.append(SenderHostMatch.cap(1).simplified());
                }
                real_line = "";
                //// query here stop
                lastposition = (body_decoded.length() - 0);
                body_decoded.append("\n");
            }


            if (fchar.unicode() == 32 || fchar.unicode() == 9) {
                real_line.append(QString(" "));
                real_line.append(const_line.simplified());
                ////out << "\n!!32!!" << line.simplified() << "!!!!!!\n";
            }
        }


        if (ContentT.indexIn(body_decoded) != -1) {
            field_h.insert(QString("Content-Type").toLower(), ContentT.cap(1).simplified());
        }
        if (recivedBy.size() > 0) {
            QString allsender = recivedBy.join(" ");
            field_h.insert(QString("Received").toLower(), allsender);
        }

        Mheader = body_decoded.toAscii();
        ICmail mail;

        //// mail.transferencoding = HeaderField("Content-Transfer-Encoding");
        mail.msgid = HeaderField("Message-ID");
        mail.md5 = fastmd5(Mfull);
        mail.sender = HeaderField("Sender");
        mail.from = HeaderField("From");
        mail.to = HeaderField("To");
        mail.subject = HeaderField("Subject");
        mail.useragent = HeaderField("User-Agent");


        mail.language = "it";
        mail.date = HeaderField("Date");
        mail.master = ""; /// HeaderField("Master");
        mail.boundary_list = GetMailKey();
        //// qDebug() << "in " << mail.boundary_list;
        int TxtBinary = Get_MultipartMime(mail);
        qDebug() << "END PARSER doc Type=" << TxtBinary;
        ////return TxtBinary;

    }

    int Parser::Get_MultipartMime(ICmail& qmail) const {
        /*
         QByteArray Mheader;
        QByteArray Mfull;
        QByteArray Mfull_A;
         */
        bool is_valid = true;
        int Tiper = -1;
        qDebug() << "mailrootdirective: " << Mfull_A.length() << "\n";
        QString HeaderTipe_a = HeaderField("Content-Type");
        if (HeaderTipe_a == PHPMAILERFROMSERVER) {
            HeaderTipe_a = QString("text/plain");
        }

        qmail.root_cmd = HeaderTipe_a;
        qmail.charcodec = QString(Utils::_RX_resolver(_CHARTSETMM_,
                QVariant(Mheader)).constData());
        // _COTRANSFERENCODING_  Content-Transfer-Encoding: 7bit ***
        qmail.encodingsend = QString(Utils::_RX_resolver(_COTRANSFERENCODING_,
                QVariant(Mheader)).constData());

        if (qmail.useragent.isEmpty()) {
            qmail.useragent = QString(Utils::_RX_resolver(_XMAILERSENDER_,
                    QVariant(Mheader)).constData());
        }

        QTextStream out(stdout);
        QString str('*');
        out << str.fill('*', _IMAIL_MAXL_) << "\n";
        /////out << QString("charcodec>>") << qmail.charcodec << "<<" << "\n";
        out << QString("subject>>") << qmail.subject << "<<" << "\n";
        out << "Inizio a decidere di cosa si tratta in root\n";
        out << str.fill('*', _IMAIL_MAXL_) << "\n";
        out.flush();
        QString multi_a, multi_b = QString(QChar('*')); /// unicode 62
        qmail.keyb = __NULLDATA__;
        qmail.keya = __NULLDATA__;
        const int DocumentType = GetMailKey(true).size();
        if (DocumentType == 2) {
            //// multipart & alternative mixed ecc the complexed doc
            out << "Document type = 100\n";
            out << str.fill('*', _IMAIL_MAXL_) << "\n";
            out.flush();
            return CaptureText(qmail, 100);
        } else if (DocumentType == 1) {
            //// multipart 
            out << "Document type = 100\n";
            out << str.fill('*', _IMAIL_MAXL_) << "\n";
            out.flush();
            return CaptureText(qmail, 100);
        } else {
            out << "Document type = 0 plain text\n";
            out << str.fill('*', _IMAIL_MAXL_) << "\n";
            out.flush();
            return CaptureText(qmail, 0);
        }
        return Tiper;
    }

    /// 6 okt. 2013 save on svn  super method function to get the first boundary an his part 

    int Parser::CaptureText(ICmail& qmail, const int mode) const {
        ///  const int recpos = pos - range;
        const int sizeMfull = Mfull_A.size();
        const int sizeHheader = Mheader.size();
        const int beginontop = Mfull_A.indexOf("multipart/", 0);
        /////take_slice(beginontop,76,"beginontop found multipart....");



        QTextStream out(stdout);
        QString str('*');

        QStringList keyheaderlist = GetMailKey();
        QStringList full_list = GetMailKey(true);
        QStringList current_bound;
        if (mode != 100) {
            return -1;
        }
        bool swapdocmode = false;
        const int modelist = keyheaderlist.size();
        const int modelistfull = full_list.size();
        int checksize = qMax(modelistfull, modelist);
        if (checksize == 0) {
            QString hdemsgerrorep = QString("Unable to find key boundary!");
            out << str.fill('.', _IMAIL_MAXL_) << "\n";
            out << hdemsgerrorep << "  \n";
            out << str.fill('.', _IMAIL_MAXL_) << "\n";
            out.flush();
            return -1;
        }

        if (modelist != modelistfull) {
            swapdocmode = true;
            //// possible error 
            if (full_list.size() > 0) {
                const QString testkey = QString("--%1").arg(QString(full_list.at(0)));
                int checkerpos = Mfull_A.indexOf(testkey, 0);
                if (checkerpos == -1) {
                    QString hdemsgerrorep1 = QString("Unable to find key boundary = {%1} ! Check yourself from mail.").arg(testkey);
                    out << str.fill('.', _IMAIL_MAXL_) << "\n";
                    out << hdemsgerrorep1 << "  \n";
                    out << str.fill('.', _IMAIL_MAXL_) << "\n";
                    out.flush();
                    return -1;
                }

            }
            current_bound = GetMailKey(true);
            out << "Search on FULLDOC.... different result as header only...\n";
        } else {
            out << "Search on AFTER HEADER.... on header same result as full .... \n";
            current_bound = GetMailKey();
        }

        /*
         QStringList keyheaderlist = GetMailKey();
        QStringList full_list = GetMailKey(true);
         */


        const int chunkmiles = sizeMfull;
        /// end from file having _FILECLOSER_ chars must having!!!
        const int LASTPOSITIONFROMMAIL = Mfull_A.indexOf(_FILECLOSER_, 0);
        if (LASTPOSITIONFROMMAIL == -1) {
            out << "App Error _FILECLOSER_ Not found!\n";
            out.flush();
            return -1;
        } else {
            out << str.fill('|', _IMAIL_MAXL_) << "\n";
            out << "First init position on doc:" << beginontop << "\n";
            out << "Last position on doc:" << LASTPOSITIONFROMMAIL << "\n";
            out << "Max position on doc:" << chunkmiles << "\n";
            out << str.fill('|', _IMAIL_MAXL_) << "\n"; /// sizeMfull


        }

        int maxPosi, minPosi, Position, Lenght = 0;
        int Posstartsa, Posstartsb = 0;
        int EndPositionDoc_a, EndPositionDoc_b = -10;
        int Lastpresentkey = 1;
        //// /Users/pro/project/version_sv/GmailHand/
        //// marker this point init trouble ...
        QString msg_e, keya, keyb;
        bool is_singlemode = true;
        const int modelistcurrent = current_bound.size();
        if (modelistcurrent == 1) {
            is_singlemode = true;
            keya = QString(current_bound.at(0));
            msg_e = QString("Working on key A.");
            EndPositionDoc_a = Mfull_A.indexOf(QString("--%1--").arg(keya), beginontop);
            Lastpresentkey = qMax(EndPositionDoc_a, beginontop);
        } else if (modelistcurrent == 2) {
            is_singlemode = false;
            keya = QString(current_bound.at(0));
            keyb = QString(current_bound.at(1));
            msg_e = QString("Working on key A-B.");
            EndPositionDoc_a = Mfull_A.indexOf(QString("--%1--").arg(keya), beginontop);
            EndPositionDoc_b = Mfull_A.indexOf(QString("--%1--").arg(keyb), beginontop);
            Lastpresentkey = qMax(EndPositionDoc_a, EndPositionDoc_b);
        }
        ////take_slice(Lastpresentkey,22,"last key ALL");
        /// qFatal(qPrintable(QString("Work progress STOP at:%1").arg(__LINE__)));

        if (keya.isEmpty()) {
            out << "App Error on key Not found!\n";
            out.flush();
            return -1;
        }
        out << "KeyA:" << keya << " stop on:" << EndPositionDoc_a << "\n";
        out << "KeyB:" << keyb << " stop on:" << EndPositionDoc_b << "\n";
        out << "Max deep down stop on:" << Lastpresentkey << "\n";
        out << msg_e << " init scan on document mail....\n";
        out << str.fill('-', _IMAIL_MAXL_) << "\n";
        out.flush();
        QList<int> List_a;
        QList<int> List_b;
        /////qFatal(qPrintable(QString("Work progress STOP at:%1").arg(__LINE__)));
        Position = 0;

        ////  slice A quick
        do {
            Position = Position + 2;
            const int ScanPos = Position;
            Posstartsa = Mfull_A.indexOf(QString("--%1").arg(keya), ScanPos);
            if (Posstartsa != -1) {
                List_a.append(Posstartsa);
                const int diff = QString("--%1").arg(keya).length();
                take_slice(Posstartsa, diff, QString("register as A>%1 search.... ").arg(keya).toAscii());
                Position = Posstartsa + 2;
                ///// out << "<<<<<Position register:" << Position << " KeyA start:" << keya << "  >>>>>\n";
            } else {
                Position = -1;
            }
            out.flush();
        } while (Position != -1);
        //// close job here!
        if (EndPositionDoc_a != -1) {
            List_a.append(Lastpresentkey);
            List_a.append(Mfull_A.size());
        }




        if (!keyb.isEmpty()) {
            Position = 0;
            ////  slice B quick
            do {
                Position = Position + 2;
                const int ScanPos = Position;
                Posstartsb = Mfull_A.indexOf(QString("--%1").arg(keyb), ScanPos);
                if (Posstartsb != -1) {
                    List_b.append(Posstartsb);
                    const int diff = QString("--%1").arg(keyb).length();
                    take_slice(Posstartsb, diff, QString("register as B>%1 search.... ").arg(keyb).toAscii());
                    Position = Posstartsb + 2;
                    ////const int diff = QString("--%1").arg(keyb).length();
                    ////take_slice(Posstartsb, diff, "register as B search ");
                    ////out << "<<<<<Position register:" << Position << " KeyB start:" << keyb << "  >>>>>\n";
                } else {
                    Position = -1;
                }
                out.flush();
            } while (Position != -1);
            /// close job
            if (EndPositionDoc_b != -1) {
                List_b.append(Lastpresentkey);
                List_b.append(Mfull_A.size());
                //// out << "<<<<<Position register end:" << EndPositionDoc_b << " KeyB end:" << keyb << "  >>>>>\n";
            }

        }
        int result =-1;
        out << str.fill('.', _IMAIL_MAXL_) << "\n";
        out.flush();
        for (int i = 0; i < (List_a.size() / 2); ++i) {
            /// _diff_record 
            int start = i;
            int stop = i + 1;
            int start_x = List_a.at(start);
            int end_x = List_a.at(stop);
            if (start_x != end_x) {
                qmail.tmp = QVariant(keya);
                ////out << "SliceBodyPart A." << i << " s. {" << start_x << ") e.(" << end_x << ") status:" << stop << "\n";
                result = SliceBodyPart(start_x, end_x, qmail);
                if (result == -1) {
                   out << "Error on SliceBodyPart  {" << start_x << ") e.(" << end_x << ") status:" << result << " key:" <<  keya <<  "\n"; 
                }
            }

        }
        out << str.fill('.', _IMAIL_MAXL_) << "\n";
        for (int i = 0; i < (List_b.size() / 2); ++i) {
            /// _diff_record 
            int start = i;
            int stop = i + 1;
            int start_x = List_b.at(start);
            int end_x = List_b.at(stop);
            if (start_x != end_x) {
                qmail.tmp = QVariant(keyb);
                ////out << "SliceBodyPart B." << i << " s. {" << start_x << ") e.(" << end_x << ") status:" << stop << "\n";
                result = SliceBodyPart(start_x, end_x, qmail);
                if (result == -1) {
                   out << "Error on SliceBodyPart  {" << start_x << ") e.(" << end_x << ") status:" << result << " key:" <<  keyb <<  "\n"; 
                }
            }

        }
        out << str.fill('.', _IMAIL_MAXL_) << "\n";
        out.flush();


        ////qDebug() << "List A " << List_a << "\n";
        ///qDebug() << "List B " << List_b << "\n";

        qFatal(qPrintable(QString("Work progress STOP at:%1").arg(__LINE__)));
        /////qFatal("SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN");
        return -1;
    }

    /*
     Mount QByteArray header on QString to grep decoded base64 subject or other
     preaction = 1 only to text or html 
     */
    QString Parser::Makedecoder(const QByteArray charset, QByteArray chunk, int preaction) const {
        const QByteArray name = QByteArray(charset);
        QTextCodec *codec = QTextCodec::codecForName(name);
        if (!codec) {
            ///// possibel base64 .. haha..convert here
            return QString(chunk.constData());
        }
        QString mount;
        QTextStream in(&mount);
        in.setCodec(codec);
        if (preaction == 0) {
            //// parser/3rdparty/kcodecs.h     
            //// //// MCodecs::quotedPrintableDecode
            in << MCodecs::quotedPrintableDecode(chunk);
        } else if (preaction == 1) {
            in << QByteArray::fromBase64(chunk.simplified());
        } else {
            /// base64 data donttouch!!
            QString  data(chunk.constData() );
            return data; 
        }
        in.flush();
        /* void QTextStream::flush ()
        Flushes any buffered data waiting to be written to the device.
        If QTextStream operates on a string, this function does nothing. 
         Ha ha ;-)
         */
        return mount;
    }



    /// Get a list of all boundary= key from mail 
    /// not parse  after Meta header --- if mail attachment having
    /// a ***.eml message/rfc822 file format. plain tex take .. 
    /// maximum boundary= is 2x on multipart/mixed && multipart/alternative
    //// if search on full body or only header having diff ....problem deep
    /// capture  boundary='------------000501030500080101060708' ok
    ///  capture boundary="------------000501030500080101060708"  ok
    ///  capture boundary=------------000501030500080101060708  oK

    QStringList Parser::GetMailKey(bool full) const {
        QStringList keylist;
        /// QRegExp expression("boundary=(.*)[\"\'\\s\\n\\r]", Qt::CaseInsensitive);
        QRegExp expression("boundary[\\S'](.*)[\\s]", Qt::CaseInsensitive);
        expression.setMinimal(true);
        int iPosition = 0;
        //// search only on Mail Header part...
        /// the second boundary can stay at bottom of mail crazy!!!!
        if (full) {
            ///// find boundary= on complete mail chunk
            while ((iPosition = expression.indexIn(Mfull_A, iPosition)) != -1) {
                //// _stripcore remove ' " : ; at end or begin key
                QString marker = Utils::_stripcore(expression.cap(1));
                keylist.append(marker);
                iPosition += expression.matchedLength();
            }
            return keylist;
        } else {
            ///// find boundary= on mail header l chunk to parse down in the deep see from no standard mail
            while ((iPosition = expression.indexIn(Mheader, iPosition)) != -1) {
                QString marker = Utils::_stripcore(expression.cap(1));
                keylist.append(marker);
                iPosition += expression.matchedLength();
            }
            return keylist;
        }

    }

    Parser::~Parser() {

    }

    void Parser::take_slice(const int start, const int diff, QByteArray name) const {
        QByteArray showchunk;
        showchunk = Mfull_A.mid(start, diff);
        QTextStream out(stdout);
        QString str('*');
        if (showchunk.size() > 0) {


            out << str.fill('.', _IMAIL_MAXL_) << "\n";
            out << "Take {" << name << "} a slice on:" << start << " size:" << showchunk.size() << "\n";
            out << str.fill('=', _IMAIL_MAXL_) << "\n";
            out << showchunk << "\n";
            out << str.fill('=', _IMAIL_MAXL_) << "\n";

        } else {
            out << "Warning No Data slice  on:" << start << "!!!!\n";
        }
        out.flush();
    }


    //// best solution but must send 2 boundary to grep each part from multipart mixed

    int Parser::SliceBodyPart(const int start, const int stop, ICmail& qmail) const {
        int golenght = Utils::_distance_position(start, stop);
        QVariant curren_c(qmail.tmp);
        const QString work_key(curren_c.toString());
        const QString endkey = QString("--%1--").arg(work_key);
        QByteArray lc, chunk, meta, sizemeta;
        QByteArray tmp = Mfull_A.mid(start + work_key.size() + 2, 320);
        tmp.prepend("\n\r");
        const QByteArray dtype = Utils::search_byline(  tmp , QByteArray("Content-Type:") );
        ///// const QByteArray dtype = QByteArray(QString(Utils::_RX_resolver(_CONTENTYPE_,QVariant(tmp)).constData()).toAscii()).simplified().toLower();
        QByteArray scharset = QByteArray(QString(Utils::_RX_resolver(_CHARTSETMM_,
                QVariant(tmp)).constData()).toAscii()).simplified().toLower();
        QByteArray encoding, sinline, sname, sX_Attachment_Id;
        QByteArray okattname = "-";
        if (tmp.indexOf("multipart/",0) > 0) {
            return 0;
        }
        if (dtype.size() < 3) {
            qDebug() << "error Content-Type: {" << dtype << "}\n";
            return -1;
        }
        /// find this word..
        QByteArray encword("Content-Transfer-Encoding:");
        int exenc = tmp.indexOf(encword, 0);
        if (exenc != -1) {
            encoding = tmp.mid(exenc + encword.size() + 1, 6).simplified().toLower();
        } else {
            encoding = "-"; /// quoted
        }


        sinline, sname = "no";
        sX_Attachment_Id = "-";

        if (dtype.startsWith("text/")) {
            /// attachment xml !!!!  encoding = "-";  /// quoted
            if (encoding.isEmpty()) {
                encoding = "quoted";
            }
            sinline = "-";

        } else {
            /// only image pdf doc attachment
            if (tmp.indexOf("inline", 0) != -1) {
                sinline = "inline";
            } else {
                sinline = "-";
            }
            int xname = tmp.indexOf("ame=", 0);
            if (xname != -1) {
                sname = Utils::token(tmp.mid(xname + 2, 100).simplified().toLower(),
                        (int) QChar('"').unicode());
            }
            scharset = "-";
        }

        int id_fileatt = tmp.indexOf("Content-ID:", 0);
        if (id_fileatt != -1) {
            sX_Attachment_Id = Utils::token(tmp.mid(id_fileatt + 2, 100).simplified(),
                    (int) QChar('<').unicode(), (int) QChar('>').unicode());
        }
        QByteArray tmpa = Mfull_A.mid(start, stop);
        QTextStream out(stdout);
        QString str('*');
        out << "Slice miles:" << golenght << "\n";
        out << "Prepare Meta:type:" <<  dtype << "|chartset" << scharset << "|encoding:" << encoding << "|inline:" << sinline << "|filename:" << sname << "|id:" << sX_Attachment_Id << "\n";
        ///// out << sX_Attachment_Id << "|" << encoding << "|" << sinline << "|" << sname << "\n";
        ////out << str.fill('.', _IMAIL_MAXL_) << "\n";
        out.flush();
        //// validate!!!  76 one line
        if (golenght == 0 || golenght < 76 || tmpa.size() < 11) {
            return -1;
        } else {
            /// alternative like say text only  | related composed mixed document
            /// out << tmpa;
            int linr = -1;
            bool having_key = false;
            bool gobuild = false;
            int is_BASE64 = 0;
            if (encoding == "base64") {
                is_BASE64 = 1;
            } else {
                is_BASE64 = 0;
            }

            Q_FOREACH(QByteArray line, tmpa.split('\n')) {
                linr++;
                lc = line.simplified();
                //// check if having the current key
                if (linr == 0) {
                    if (lc.indexOf(work_key, 0) != -1) {
                        having_key = true;
                        continue;
                    }
                    continue;
                }
                /// first line from chunk after meta header info
                if (lc.isEmpty() && !gobuild) {
                    gobuild = true;
                    sizemeta.append(line);
                    continue;
                }
                /// take meta header info to know type
                if (having_key && !gobuild) {
                    sizemeta.append(lc);
                    sizemeta.append("\n\r");
                    continue;
                }
                if (lc.startsWith(endkey.toAscii())) {
                    gobuild = false;
                    break;
                }
                
                if (gobuild) {
                    if (is_BASE64 == 1) {
                        chunk.append(lc);
                    } else {
                        chunk.append(line);
                        chunk.append("\n\r");
                    }
                    continue;
                }
            }
            //// const int meta_miles = sizemeta.length();
            //// QByteArray mbody = Mfull_A.mid(start + meta_miles , golenght - meta_miles);
            QString smeta(meta.constData());
            //// const QString dtype = _contenttype_resolver(smeta);
            /// validate here dtype on mime type class 
            //// find info from meta header...
            /// charset Content-Transfer-Encoding
            /// charset="UTF-8"
            if ( dtype.startsWith("image/") ) { 
                //// qDebug() << chunk; 
            }
            

            /// search cid: on html doc!!!

            /// encoding  also contain quoted
            QString ready;
            if (is_BASE64 == 0 && dtype == "text/html" ||
                    is_BASE64 == 0 && dtype == "text/plain") {
                ready = Makedecoder(scharset, chunk, 0);
            } else if (is_BASE64 == 1 && dtype == "text/html" ||
                    is_BASE64 == 1 && dtype == "text/plain") {
                ready = Makedecoder(scharset, chunk.simplified(), 4);
            } else {
                ready = Makedecoder(scharset, chunk.simplified(), 3);
            }
            /// for better find image inline if having
            if (is_BASE64 != 1 && dtype == "text/html") {
                const QByteArray html(ready.toAscii());
                ready = QString(html.simplified().data());
            }
            
            /// convert is not!
            if (is_BASE64 != 1) {
                is_BASE64 = 1;
                ready = QString(ready.toAscii().toBase64().constData());
            }
            
            if ( dtype == "text/html") {
                //// save html on root mail.
                qmail.tmp = QVariant(ready);
            }
            if ( dtype == "text/plain" ) {
                //// save text on root mail.
                qmail.txt = QVariant(ready);
            }
            
            
            const int bilancia = ready.size();
            QString name, nameid, inlinepic;
            Qmailf *attachment = new Qmailf(start);
            if (is_BASE64 == 1) {
                //// save action 
                attachment->SetMeta(smeta, QString(dtype.constData()));
                if (sname.size() > 3) {
                    attachment->SetFile(QString(sname.constData()));
                }
                if (sinline.size() > 3) {
                    attachment->SetInline(true, QString(sX_Attachment_Id.constData()));
                } else {
                    attachment->SetInline(false, "");
                }
                attachment->SetChunk(ready.toAscii());
                attachment->Info();
                /////out << "Prepare Meta:type:" << dtype << "|chartset" << scharset << "|encoding:" << encoding << "|inline:" << sinline << "|filename:" << sname << "|id:" << sX_Attachment_Id << "\n";
                QMap<int, QVariant> att_files;
                QVariant v = VPtr<Qmailf>::asQVariant(attachment);
                qmail.alist.insert(attachment->Uid(), v);
            } else {
                return -1;
            }
            out << str.fill('.', _IMAIL_MAXL_) << "\n";
            out.flush();
            ///Q_UNUSED(start);
            ///Q_UNUSED(stop);
            return attachment->Uid();
        }

    }


}


//// memo
///grep  -nRHI "QRegExp" *
// static QRegExp quotemarks("^>[>\\s]*");
/// static const QRegExp linkRe("("
//"https?://" // scheme prefix
// "[;/?:@=&$\\-_.+!',0-9a-zA-Z%#~\\[\\]\\(\\)*]+" // allowed characters
// "[/@=&$\\-_+'0-9a-zA-Z%#~]" // termination
//")");
//// filename.replace(QRegExp(QLatin1String("[/\\\\:\"|<>*?]")), QLatin1String("_"));
////if (ReadMail::insensitindex("boundary", Mfull,0,last_line,0) != -1) {