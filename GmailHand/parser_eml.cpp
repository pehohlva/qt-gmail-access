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

namespace ReadMail {

    QString _format_string(QString s) {
        ///
        QString repair = "";
        qint64 o = 0;
        for (int i = 0; i < s.size(); ++i) {
            o++;
            repair.append(s.at(i));
            if (o == 76) {
                o = 0;
                repair.append(QString("\n"));
            }
        }

        return repair;
    }

    inline static int _addstrPos(const int pos, QString key) {
        const int x = key.length();
        return ( pos + x + 1);
    }
    //// remove unicode 39 && 34 not allowed in boundary key 

    static QString _stripcore(QString x) {
        //// qDebug() << vox.unicode() << ":" << vox << "_";
        const int less = x.size() - 1;
        QString newstr;
        for (int i = 0; i < x.size(); ++i) {
            QChar vox(x.at(i));
            int letter = vox.unicode();

            if (i == 0 || i == less) {
                if (letter == 39 ||
                        letter == 34 ||
                        letter == 59 ||
                        letter == 58) {
                    /// error at end or first!!
                } else {
                    newstr.append(vox);
                }

            } else {
                newstr.append(vox);
            }
        }
        return newstr;
    }

    inline static QStringList _multipart_resolver(QString x) {
        QStringList keylist;
        QRegExp expression("multipart/(.*)[;\\s]", Qt::CaseInsensitive);
        expression.setMinimal(true);
        int iPosition = 0;
        //// search only on Mail Header part...
        while ((iPosition = expression.indexIn(x, iPosition)) != -1) {
            keylist.append(expression.cap(1));
            iPosition += expression.matchedLength();
        }
        return keylist;
    }

    inline static QStringList _multipart_resolver(QByteArray x) {
        QStringList keylist;
        QRegExp expression("multipart/(.*)[;\\s]", Qt::CaseInsensitive);
        expression.setMinimal(true);
        int iPosition = 0;
        //// search only on Mail Header part...
        while ((iPosition = expression.indexIn(x, iPosition)) != -1) {
            keylist.append(expression.cap(1));
            iPosition += expression.matchedLength();
        }
        return keylist;
    }

    inline static QString _contenttype_resolver(QString x) {

        QString found;
        QRegExp ContentT("Content-Type: +(.*)"); /// end on ;!!!
        if (ContentT.indexIn(x.simplified()) != -1) {
            QString start = ContentT.cap(1).simplified();
            QStringList words = start.split(" ", QString::SkipEmptyParts);
            if (words.size() > 0) {
                found = words.at(0);
                if (found.lastIndexOf(QChar(';')) != -1) {
                    found = found.mid(0, found.size() - 1);
                }
            }
        }
        return found;
    }

    inline static QString _onekey_resolver(QByteArray x) {

        QString found;
        QRegExp Onesinglekey("boundary[\\S'](.*)[\\s]"); /// end on ;!!!
        if (Onesinglekey.indexIn(x.simplified()) != -1) {
            found = _stripcore(Onesinglekey.cap(1).simplified());

        }
        return found;
    }

    inline static QString partmd5(const QByteArray xml, int position) {
        QCryptographicHash formats(QCryptographicHash::Md5);
        QString found = QString("FilePosition=%1").arg(position);
        formats.addData(xml);
        formats.addData(xml);
        return QString(formats.result().toHex().constData());
    }

    //// one word search result 

    inline static QByteArray _RX_resolver(const QRegExp rx, QVariant x) {
        QByteArray text;
        QString result;
        if (x.canConvert(QVariant::ByteArray)) {
            text = x.toByteArray();
        }
        if (rx.indexIn(text) != -1) {
            QString start(rx.cap(1).simplified());
            QStringList words = start.split(" ", QString::SkipEmptyParts);
            if (words.size() > 0) {
                result = _stripcore(words.at(0));
                text = QByteArray(result.toAscii());
                if (text.length() > 10) {
                    text.clear();
                }
            }
        }
        return text;
    }

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
        QByteArray onlyMETAHEADER = QByteArray();
        QByteArray afterMETAHEADER = QByteArray();
        QByteArray realyFullMail = QByteArray();
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
        qmail.charcodec = QString(_RX_resolver(_CHARTSETMM_,
                QVariant(Mheader)).constData());
        // _COTRANSFERENCODING_  Content-Transfer-Encoding: 7bit ***
        qmail.encodingsend = QString(_RX_resolver(_COTRANSFERENCODING_,
                QVariant(Mheader)).constData());

        if (qmail.useragent.isEmpty()) {
            qmail.useragent = QString(_RX_resolver(_XMAILERSENDER_,
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
        } else {
            current_bound = GetMailKey();
        }

        const int chunkmiles = Mfull_A.length();
        /// end from file having _FILECLOSER_ chars must having!!!
        const int LASTPOSITIONFROMMAIL = Mfull_A.indexOf(_FILECLOSER_, 0);
        if (LASTPOSITIONFROMMAIL == -1) {
            out << "App Error _FILECLOSER_ Not found!\n";
            out.flush();
            return -1;
        }

        //// marker this point init trouble ...
        QString msg_e, keya, keyb;
        const int modelistcurrent = current_bound.size();
        if (modelistcurrent == 1) {
            keya = QString(current_bound.at(0));
            msg_e = QString("Working on key.");
        } else if (modelistcurrent == 2) {
            keya = QString(current_bound.at(0));
            keyb = QString(current_bound.at(0));
            msg_e = QString("Working two key.");
        }
        out << "KeyA:" << keya << " KeyB:" << keyb << "\n";

        int maxPosi, minPosi, Position, Lenght, resultb, resulta, resultbn, resultan = 0;
        //// /Users/pro/project/version_sv/GmailHand/
        /*
         const QString tkey = QString("--%1").arg(keyb);
         const QString zkey = QString("--%1--").arg(keya);
         * first check the end of document and cut out after search start!!!
         */
        Position = 0;
        int lastStart = 0;
        bool go = true;
        out << "Slice init:" << Position << " distance:" << chunkmiles << "  \n";
        do {
            Position = Position + (76 * 2);
            const int ScanPos = Position;
            out << "Slice at:" << Position << " distance:" << chunkmiles << "  \n";
            if (!keya.isEmpty()) {
                resulta = Mfull_A.indexOf(QString("--%1").arg(keya), ScanPos);
                if (resulta != -1) {
                    lastStart = resulta;
                    go = true;
                    out << "Found start:" << resulta << " search KeyA start:" << keya << "  \n";
                }
            }
            if (!keya.isEmpty() && go) {
                resultan = Mfull_A.indexOf(QString("--%1--").arg(keya), lastStart + 76); // + 76 next line from result
                if (resultan != -1) {
                    //// stop no loop more if end doc 
                    if ((resultan + 100) > chunkmiles) {
                        Position = chunkmiles + 500;
                        out << "Found end:" << resultan << " search KeyA end:" << keya << "  \n";
                    }
                }
            }
            out.flush();

        } while (Position < chunkmiles);
        
        int down = qMin(resulta, resultan );
        int atend = qMax(resulta, resultan );
        out << "First start:" << down << " KeyA end:" << atend << "  \n";
        out.flush();


        out << str.fill('.', _IMAIL_MAXL_) << "\n";
        out.flush();
        /*   ----------------------------------------  */
        //// QByteArray *xml = new QByteArray();


        qFatal("SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN");
        return -1;
    }
    //// best solution but must send 2 boundary to grep each part from multipart mixed

    DocSlice Parser::SliceBodyPart(const int start, const int stop, ICmail& qmail) const {

        /// alternative like say text only  | related composed mixed document
        Q_UNUSED(start);
        Q_UNUSED(stop);


    }

    bool Parser::Header_Code_Grep(const QString header_line) const {
        /*
         Content-Type: text/plain; charset=UTF-8
         Content-Transfer-Encoding: quoted-printable
         * Content-Type: image/png; x-mac-type="0"; x-mac-creator="0";
        name="asf-logo-nt.png"
        Content-Transfer-Encoding: base64
        Content-ID: <part1.00030704.04090905@yahoo.com.br>
        Content-Disposition: inline;
        filename="asf-logo-nt.png"
         * Content-Type: text/html; charset=ISO-8859-15
        Content-Transfer-Encoding: 7bit
         * 
         * Content-Type: application/zip;
        name="useragent_mobi.log.zip"
        Content-Transfer-Encoding: base64
        Content-Disposition: attachment;
        filename="useragent_mobi.log.zip"
         * User-Agent:
         * Subject: il coso inline test
        Content-Type: multipart/mixed;
        boundary="------------030608050104060100030904"

        This is a multi-part message in MIME format.
        --------------030608050104060100030904
        Content-Type: multipart/alternative;
        boundary="------------030801020001090808060804"
         */
        QStringList error_codes;
        error_codes << "Content-Type" << "charset" << "quoted-printable"
                << "Content-Transfer-Encoding" << "UTF-8" << "text/" << "<part1."
                << "quoted-printable" << "base64" << "us-ascii" << "7bit";
        for (int i = 0; i < error_codes.size(); ++i) {
            QString error_words = QString(error_codes.at(i).toLocal8Bit());
            if (header_line.contains(error_words, Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    }

    /*
     Mount QByteArray header on QString to grep decoded base64 subject or other
     
     */
    QString Parser::Makedecoder(const QByteArray charset, QByteArray chunk, int preaction) const {
        const QByteArray name = QByteArray(charset);
        QTextCodec *codec = QTextCodec::codecForName(name);
        QString mount;
        QTextStream in(&mount);
        in.setCodec(codec);
        if (preaction == 0) {
            in << _translateFromQuotedPrintable(chunk); //// mail_handler.cpp 
        } else if (preaction == 1) {
            in << QByteArray::fromBase64(chunk.simplified());
        } else {
            in << chunk;
        }
        in.flush();
        /* void QTextStream::flush ()
        Flushes any buffered data waiting to be written to the device.
        If QTextStream operates on a string, this function does nothing. 
         Ha ha ;-)
         */
        return mount;
    }

    Parser::~Parser() {

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
                QString marker = _stripcore(expression.cap(1));
                keylist.append(marker);
                iPosition += expression.matchedLength();
            }
            return keylist;
        } else {
            ///// find boundary= on mail header l chunk to parse down in the deep see from no standard mail
            while ((iPosition = expression.indexIn(Mheader, iPosition)) != -1) {
                QString marker = _stripcore(expression.cap(1));
                keylist.append(marker);
                iPosition += expression.matchedLength();
            }
            return keylist;
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