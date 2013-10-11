//
// C++ Implementation: sample to send mail on smtp from gmail
//
// Description:
// SMTP Encrypted sending mail attachment & mime chunk Editmail->document()
// ( QTextDocument ) fill from image e html tag & style color full mime mail.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution
//
//

#include "mailformat.h"

MailFormat::MailFormat() {
    now = QString(QTime::currentTime().toString("h:mm:ss "));
    a_frommail = "frommail@example.org";
    a_tomail = "sendermail@example.org";
    QDateTime now = QDateTime::currentDateTime();
    unixtime = now.toMSecsSinceEpoch();
    qDebug() << "boundarykey:" << unixtime;
    Rawmail = "";
    sendhost = QString("gmail.com");
    imagelist.clear();
    UniqueKeyInlineImage = QString("pic-%1-pic").arg(unixtime);
    UniqueKeyAttachment = QString("att-%1-att").arg(unixtime);
}

bool MailFormat::AppendAttachment(QFileInfo filepath) {
    QFile local_file(filepath.absoluteFilePath());
    const QString extension = filepath.suffix().toLower();
    MimeTypes question;
    QString mimeactual = question.value(extension);
    QString parts = ""; ///  2 line break before
    if (local_file.open(QIODevice::ReadOnly)) {
        QByteArray blob = local_file.readAll();
        QString reformat_blob = QString(blob.toBase64().constData());
        attachmentlist += filepath.absoluteFilePath();
        parts.append(QString("--%1\r\n").arg(UniqueKeyAttachment));
        parts.append(QString("Content-type: %1;").arg(mimeactual));
        parts.append(QString("  name=\"%1\"\r\n").arg(filepath.fileName()));
        parts.append("Content-disposition: attachment; \n");
        parts.append(QString("  filename=\"%1\"\r\n").arg(filepath.fileName()));
        parts.append("Content-Transfer-Encoding: base64\r\n");
        parts.append("\r\n");
        parts.append(Format_String(reformat_blob));
        parts.append("\r\n");
        Attachmail.append(parts);
        local_file.close();
        return true;
    }
    return false;
    /*
     * Content-type: image/png; name="Schermata 2013-08-27 a 10.45.59.png"
       Content-disposition: attachment;
        filename="Schermata 2013-08-27 a 10.45.59.png"
        Content-transfer-encoding: base64
     */

    ////target append on Attachmail;
}

QString MailFormat::ComposeTxtPlain(QString txt) {
    qDebug() << "boundarykey:" << unixtime;
    QString parts = ""; ///  2 line break before
    parts.append(QString("--%1\r\n").arg(unixtime));
    parts.append("Content-type: text/plain; charset=utf-8\r\n");
    parts.append("Content-Transfer-Encoding: quoted-printable\r\n");
    parts.append("\r\n");
    parts.append(encodeQP(txt));
    parts.append("\r\n");
    /// if next html having inline image
    if (imagelist.size() > 0) {
        parts.append(QString("--%1\r\n").arg(unixtime));
        parts.append(QString("Content-Type: multipart/related;\n    boundary=\"%1\"\r\n").arg(UniqueKeyInlineImage));
    }
    return parts;
    ////
}

QString MailFormat::ComposeHtml(QString html) {
    //// <img src="myimage" />
    QString parts = ""; ///  2 line break before
    if (imagelist.size() > 0) {
        parts.append(QString("--%1\r\n").arg(UniqueKeyInlineImage));
    } else {
        parts.append(QString("--%1\r\n").arg(unixtime));
    }
    parts.append("Content-type: text/html; charset=utf-8\r\n");
    parts.append("Content-Transfer-Encoding: quoted-printable\r\n");
    parts.append("\r\n");
    parts.append(encodeQP(html));
    parts.append("\r\n");
    return parts;
    ////
}

QString MailFormat::ComposeHeader(const QString Subject, QString CC) {
    QString header = "";
    qDebug() << "boundarykey:" << unixtime;
    QString agent = QString("User-Agent: QTMacMail (Macintosh; Intel Mac OS X 10.8; rv:17.0)\r\n");

    //// QString strRestored(QByteArray::fromBase64(asSaved));
    QString cores;
    cores.append(Subject);

    ////const QString marebello = QString(QString::number(unixtime));
    ///// qDebug() << "marebellomarebellomarebellomarebellomarebello:" << marebello;

    /* line 1 */
    header.append(QString("From:\"%1\" <%1>\r\n").arg(a_frommail));
    /* line 2 */
    header.append(QString("Reply-To:\"%1\" <%1>\r\n").arg(a_frommail));
    /* line 3 subject  */
    header.append(QString("Subject: =?utf-8?B?%1?=\r\n").arg(cores.toLatin1().toBase64().constData())); /// encoding text ????
    /* line 4 */
    header.append("X-Powered-BY: Peter Hohl autor\r\n");
    header.append("X-Mailer: Freeroad Libs (0.0.1)\r\n");
    /////X-Powered-BY: OTRS - Open Ticket Request System (http://otrs.org/)
    ////

    header.append(QString("To:\"%1\" <%1>\r\n").arg(a_frommail));
    if (!CC.isEmpty()) {
        /* line 5 */
        header.append(QString("Cc:\"%1\" <%1>\r\n").arg(CC));
    }
    //// header.append(QString("Cc:\"%1\" <%1>\r\n").arg(marebello));
    /* line 6 */
    header.append("MIME-Version: 1.0\r\n");

    /*
    Content-Type: multipart/mixed;
        boundary="------------030605040005010602040201"
X-OriginalArrivalTime: 16 Sep 2013 15:24:43.0315 (UTC) FILETIME=[DE618030:01CEB2F0]
     * */

    if (attachmentlist.size() > 0) {
        header.append(QString("Content-Type: multipart/mixed;\n    boundary=\"%1\"\r\n").arg(UniqueKeyAttachment));
        header.append(QString("\n--%1\r\n").arg(UniqueKeyAttachment));
    }


    /* line 7 */
    QDateTime now = QDateTime::currentDateTime();
    QString date_nz = now.toString("dd.MM.yyyy hh:ss");
    header.append(QString("Content-Type: multipart/alternative; boundary=\"%1\"\r\n").arg(unixtime));
    header.append(QString("Note:%1\r\n").arg(date_nz));
    header.append("This is a MIME encoded message. Note boundary is a unix time stamp to check date sending composing. \r\n");
    ///// header.append(QString("-\"%1\"-\r\n").arg( marebello  ));
    header.append("\r\n");
    header.append("\r\n");
    //////header.append("--%1\r\n").arg(boundarykey);
    /*
     Date: Sun, 15 Sep 2013 07:28:50 -0700 (PDT)
From: Filippo Giani <filippo@giani.ch>
Reply-To: Filippo Giani <filippo@giani.ch>
Subject: punto situazione
To: Peter Hohl <pehohlva@gmail.com>
Cc: "marco.bazzi@liberatv.ch" <marco.bazzi@liberatv.ch>
MIME-Version: 1.0
Content-Type: multipart/alternative; boundary="1837502048-1903744228-1379263064=:76005"
     */

    return header;
}
/// public

void MailFormat::SetMessage(const QString Subject, const QTextDocument *doc) {
    /// target to make Rawmail!!!
    qDebug() << "boundarykey:" << unixtime;
    QString txt = doc->toPlainText();
    txt.replace("\n", "\r\n");
    txt.replace("\n.", "\n=2E");
    /// extract resource image from html and convert cid:name
    /// append image attachmenent inline
    QString xhtml = doc->toHtml("utf-8");
    /// search  //// <img src="myimage" />
    QTextFrame *Tframe = doc->rootFrame();

    QTextFrame::iterator it;
    for (it = Tframe->begin(); !(it.atEnd()); ++it) {
        QTextFrame *childFrame = it.currentFrame();
        QTextBlock para = it.currentBlock();
        if (childFrame) {
            qDebug() << "### childFrame ok ";
        } else if (para.isValid()) {
            qDebug() << "### para.isValid() ok ";
            QTextBlockFormat ParentBl = para.blockFormat();
            QTextBlock::iterator de;
            for (de = para.begin(); !(de.atEnd()); ++de) {
                QTextFragment fr = de.fragment();
                if (fr.isValid()) {
                    qDebug() << "### fr.fragment() ok ";
                    QTextCharFormat TXTCh = fr.charFormat();
                    QTextImageFormat Pics = TXTCh.toImageFormat();
                    ////QTextTableFormat Tabl = TXTCh.toTableFormat();
                    ////QTextListFormat Uls = TXTCh.toListFormat();
                    if (Pics.isValid() && !Pics.name().isEmpty()) {
                        QString name = Pics.name();
                        imagelist += name; //// search after on html code
                        qDebug() << "### Pics.isValid  ok " << name;
                        int w = 0;
                        int h = 0;
                        if (Pics.height() > 0) {
                            h = Pics.height();
                        }
                        if (Pics.width() > 0) {
                            w = Pics.width();
                        }
                    }

                }

            }
        }

    }
    /// fix inline image ///
    for (int i = 0; i < imagelist.size(); ++i) {
        QString name = QString(imagelist.at(i).toLocal8Bit().constData());
        QString qt_name = name;
        qt_name.prepend(QString("part_%1").arg(i));
        qt_name.append(QString("@%1").arg(sendhost));
        xhtml.replace(QString("src=\"%1\"").arg(name), QString("src=\"cid:%1\"  alt=\"Image pos %1\"").arg(qt_name));
    }
    xhtml.replace("\n", "\r\n");
    xhtml.replace("\n.", "\n=2E");

    Rawmail = "";
    Rawmail.append(ComposeHeader(Subject));





    Rawmail.append(ComposeTxtPlain(txt));
    Rawmail.append(ComposeHtml(xhtml));

    // append inline image from html!
    Rawmail.append(Place_Inline_Image(doc));

    Rawmail.append(QString("--%1--\r\n").arg(unixtime));
    if (attachmentlist.size() > 0) {
        Rawmail.append(Attachmail);
        Rawmail.append(QString("--%1--\r\n").arg(UniqueKeyAttachment));
    }
    /// close two format


}

QString MailFormat::Place_Inline_Image(const QTextDocument *doc) {

    QString code = "";
    if (imagelist.size() < 1) {
        return code;
    }
    // iterate QStringList imagelist;

    for (int i = 0; i < imagelist.size(); ++i) {
        QString name = QString(imagelist.at(i).toLocal8Bit().constData());
        qDebug() << "### QImage code inline append  load ok :-)  " << name;
        // vvv  Copy pasted mostly from Qt =================
        // http://qtcocoon.googlecode.com/svn/trunk/Tools_OpenDocument_Editor/patch_qt4.5_OO/qtextodfwriter.cpp
        QImage image;
        if (name.startsWith(QLatin1String(":/"))) // auto-detect resources
            name.prepend(QLatin1String("qrc"));
        QUrl url = QUrl::fromEncoded(name.toUtf8());
        const QVariant data = doc->resource(QTextDocument::ImageResource, url);
        if (data.type() == QVariant::Image) {
            image = qvariant_cast<QImage>(data);
        } else if (data.type() == QVariant::ByteArray) {
            image.loadFromData(data.toByteArray());
        }
        if (!image.isNull()) {
            //// make all png from name!!!!
            qDebug() << "### QImage load ok :-)  " << name;
            QBuffer imageBytes;
            QImageWriter imageWriter(&imageBytes, "jpg");
            imageWriter.setFormat("jpg");
            imageWriter.setQuality(90);
            imageWriter.setText("Author", "QT Mailchunker");
            imageWriter.write(image);
            QString qt_name = name;
            qt_name.prepend(QString("part_%1").arg(i));
            qt_name.append(QString("@%1").arg(sendhost));
            QString human_name_image = name;
            human_name_image.append(QString("_%1.jpeg").arg(unixtime));
            /*
             Content-Type: image/jpeg; x-mac-type="0"; x-mac-creator="0";
 name="Naret23.jpeg"
Content-Transfer-Encoding: base64
Content-ID: <part1.09020004.01010006@gmail.com>
Content-Disposition: inline;
 filename="Naret23.jpeg"
             */

            QString reformat_img = QString(imageBytes.data().toBase64().constData());
            code.append(QString("--%1\r\n").arg(UniqueKeyInlineImage));
            code.append(QString("Content-type: image/jpeg; x-mac-type=\"0\"; x-mac-creator=\"0\"; \n"));
            code.append(QString("    name=\"%1\"\r\n").arg(human_name_image));
            code.append("Content-Transfer-Encoding: base64\r\n");
            code.append(QString("Content-ID: <%1> \r\n").arg(qt_name));
            code.append(QString("Content-Disposition: inline;\n  filename=\"%1\" \r\n").arg(human_name_image));
            code.append("\r\n");
            code.append(Format_String(reformat_img));
            code.append("\r\n");

        }

    }
    /// close section
    code.append(QString("--%1--\r\n").arg(UniqueKeyInlineImage));

    return code;

}

QString MailFormat::Format_String(QString s) {
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

QString MailFormat::encodeQP(QString s) {
    for (int i = 0; i < s.size(); i++) {
        int c = s.at(i).unicode();
        if (c == 9 || c == 10 || c == 13) continue;
        if ((c < 32 || s.at(i) == 61 || c >= 126)) {
            //the char has to be replaced (only "=" and non-ascii characters)
            QString hex("=");
            if (QString::number(c, 16).size() == 1) hex.append(QString::number(0));
            hex.append(QString::number(c, 16).toUpper());

            s.replace(i, 1, "=" + QString::number(c, 16).toUpper());
            i += QString::number(c, 16).size(); //jump over inserted hex-code
        }
    }
    return s;
}


