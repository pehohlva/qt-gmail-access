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


/// latest revision on 16 okt 2013
/// all contenent base64 encoding 
/// running stable and many mail client can read 



namespace Utils {

    QString _partmd5(const QByteArray xml, int position) {
        QCryptographicHash formats(QCryptographicHash::Md5);
        QString found = QString("FilePosition=%1").arg(position);
        formats.addData(xml);
        formats.addData(xml);
        return QString(formats.result().toHex().constData());
    }

}

static inline QString _format_string76(QString s) {
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

MailFormat::MailFormat() {
    now = QString(QTime::currentTime().toString("h:mm:ss "));
    a_frommail = "frommail@example.org";
    a_tomail = "sendermail@example.org";
    QDateTime now = QDateTime::currentDateTime();
    unixtime = now.toMSecsSinceEpoch();
    //// qDebug() << "boundarykey:" << unixtime;
    imagelist.clear();
    Rawmail = "";
    QString str("*");
    sendhost = QString("gmail.com");
    
    /// check if download dir exist __TESTWRITEMAIL__ 
    
    const QString path = __TESTWRITEMAIL__;
        QDir dir;
        if (!dir.exists(path))
            dir.mkpath(path);
        
    UniqueKeyAttachment = QString("_003_%1").arg(unixtime);
    UniqueKeyInlineImage = QString("_002_%1").arg(unixtime);
    UniqueKeyTexttPlainHtml = QString("_000_%1").arg(unixtime);

    UniqueKeyTexttPlainHtml.append(str.fill(QChar('T'), 55 - UniqueKeyTexttPlainHtml.size()));
    UniqueKeyAttachment.append(str.fill(QChar('A'), 55 - UniqueKeyAttachment.size()));
    UniqueKeyInlineImage.append(str.fill(QChar('I'), 55 - UniqueKeyInlineImage.size()));
}

QString MailFormat::_chunkAttachment(const QString fi) {
    QFileInfo localfile(fi);
    QByteArray blob;
    QFile local_file(fi);
    const QString extension = localfile.suffix().toLower();
    QString xchunk;
    if (local_file.open(QIODevice::ReadOnly)) {
        blob = local_file.readAll();
        QString reformat_blob = QString(blob.toBase64().constData());
        xchunk.append(Format_String(reformat_blob));
        xchunk.append("\r\n");
    } else {
        blob = QByteArray("Error unable to open file!!");
        QString blob0 = QString(blob.toBase64().constData());
        xchunk.append(Format_String(blob0));
        xchunk.append("\r\n");
    }
    
    int maxsizefile = qMax(xchunk.size(),blob.size());
    
    MimeTypes question;
    QString mimcurrent = question.value(extension);
    if ( mimcurrent.size() < 3   ) {
        mimcurrent = QString("application/%1").arg(extension);
    }
    QString parts = ""; ///  2 line break before
    parts.append(QString("--%1\r\n").arg(UniqueKeyAttachment));
    parts.append(QString("Content-type: %1;\n").arg(mimcurrent));
    parts.append(QString("\tname=\"%1\"\r\n").arg(localfile.fileName()));
     parts.append(QString("Content-Description: %1\r\n").arg(localfile.fileName()));
    parts.append("Content-disposition: attachment;");
    parts.append(QString("\tfilename=\"%1\";  size=%2;\r\n").arg(localfile.fileName()).arg(QString::number(maxsizefile)));
    parts.append("Content-Transfer-Encoding: base64\r\n");
    parts.append("\r\n");
    parts.append(xchunk);
    
    
    
    /* Content-Type: application/pdf;
	name="COMUNICATO STAMPA POMERIGGIO CORSI Mendrisio.pdf"
Content-Description: COMUNICATO STAMPA POMERIGGIO CORSI Mendrisio.pdf
Content-Disposition: attachment;
	filename="COMUNICATO STAMPA POMERIGGIO CORSI Mendrisio.pdf"; size=243370;
	creation-date="Tue, 15 Oct 2013 07:17:38 GMT";
	modification-date="Tue, 15 Oct 2013 07:11:50 GMT"
Content-Transfer-Encoding: base64 */
    
    
    

    
    local_file.close();
    return parts;
}

bool MailFormat::AppendAttachment(QFileInfo filepath) {

    ////qDebug() << "AppendAttachment:" << filepath.absoluteFilePath();

    if (filepath.exists()) {
        attachmentlist += filepath.absoluteFilePath();
        return true;
    }
    return false;
}

QString MailFormat::ComposeTxtPlain(QString txt) {
    /// qDebug() << "boundarykey:" << UniqueKeyTexttPlainHtml;
    QByteArray chunk = QByteArray(txt.toLocal8Bit()   ).toBase64();
    QString base64data = QString(chunk.constData());

    QString parts = "\r\n"; ///  2 line break before
    parts.append(QString("--%1\r\n").arg(UniqueKeyTexttPlainHtml));
    parts.append("Content-type: text/plain; charset=utf-8\r\n");
    parts.append("Content-Transfer-Encoding: base64\r\n");
    parts.append("\r\n");
    parts.append(Format_String(base64data));
    parts.append("\r\n");
    return parts;
    ////
    /*if (imagelist.size() > 0) {
        parts.append(QString("--%1\r\n").arg(unixtime));
        parts.append(QString("Content-Type: multipart/related;\n    boundary=\"%1\"\r\n").arg(UniqueKeyInlineImage));
    }*/
}

QString MailFormat::ComposeHtml(QString html) {

    QByteArray chunk = QByteArray(html.toLocal8Bit()).toBase64();
    QString base64data = QString(chunk.constData());

    QString parts = "\r\n"; ///  2 line break before
    parts.append(QString("--%1\r\n").arg(UniqueKeyTexttPlainHtml));
    parts.append("Content-type: text/html; charset=utf-8\r\n");
    parts.append("Content-Transfer-Encoding: base64\r\n");
    parts.append("\r\n");
    parts.append(Format_String(base64data));
    parts.append("\r\n");
    parts.append(QString("--%1--\r\n").arg(UniqueKeyTexttPlainHtml));
    return parts;
    ////
}

QString MailFormat::ComposeHeader(const QString Subject, QString CC) {
    QString header = "";
    QString agent = QString("User-Agent: QTMacMail (Macintosh; Intel Mac OS X 10.8; rv:17.0)\r\n");

    //// QString strRestored(QByteArray::fromBase64(asSaved));
    QString cores;
    cores.append(Subject);
    header.append(QString("From:\"%1\" <%1>\r\n").arg(a_frommail));
    /* line 2 */
    header.append(QString("Reply-To:\"%1\" <%1>\r\n").arg(a_frommail));
    /* line 3 subject  */
    header.append(QString("Subject: =?utf-8?B?%1?=\r\n").arg(cores.toUtf8().toBase64().constData())); /// encoding text ????
    header.append("X-Powered-BY: Peter Hohl autor\r\n");
    header.append("X-Mailer: Freeroad Libs (0.0.1)\r\n");
    header.append(QString("To:\"%1\" <%1>\r\n").arg(a_tomail));
    if (!CC.isEmpty()) {
        /* line 5 */
        header.append(QString("Cc:\"%1\" <%1>\r\n").arg(CC));
    }
    header.append("X-MS-Has-Attach: yes\r\n");
    header.append("X-MS-TNEF-Correlator:\r\n");
    //// header.append(QString("Cc:\"%1\" <%1>\r\n").arg(marebello));
    /* line 6 */
    

    /*
    Content-Type: multipart/mixed;
        boundary="_009_9356FEBDF6AD6C4B893B13E2BFF2173614B0C14Emscsbegia0022me_"
MIME-Version: 1.0

--_009_9356FEBDF6AD6C4B893B13E2BFF2173614B0C14Emscsbegia0022me_
Content-Type: multipart/related;
        boundary="_008_9356FEBDF6AD6C4B893B13E2BFF2173614B0C14Emscsbegia0022me_";
        type="multipart/alternative"

--_008_9356FEBDF6AD6C4B893B13E2BFF2173614B0C14Emscsbegia0022me_
Content-Type: multipart/alternative;
        boundary="_000_9356FEBDF6AD6C4B893B13E2BFF2173614B0C14Emscsbegia0022me_" */

    /* UniqueKeyAttachment = QString("006_%1").arg(unixtime);
    UniqueKeyInlineImage = QString("005_%1").arg(unixtime);
    UniqueKeyTexttPlainHtml = QString("000_%1").arg(unixtime);
     */



    /* line 7 */
    QDateTime now = QDateTime::currentDateTime();
    QString date_nz = now.toString("dd.MM.yyyy hh:ss");
    /// quello che chiude tutto
    header.append(QString("Content-Type: multipart/mixed;\r\n\tboundary=\"%1\";\r\n").arg(UniqueKeyAttachment));
    ////header.append(QString("Note:%1\r\n").arg(date_nz));
    ///header.append("This is a MIME encoded message. Unix Time on boundary. \r\n");
    header.append("MIME-Version: 1.0\r\n");
    header.append("\r\n");
    ///// header.append(QString("-\"%1\"-\r\n").arg( marebello  ));
    header.append(QString("--%1\r\n").arg(UniqueKeyAttachment));
    header.append(QString("Content-Type: multipart/related;\r\n\tboundary=\"%1\";\r\n").arg(UniqueKeyInlineImage));
    header.append(QString("type=\"multipart/alternative\"\r\n"));
    header.append("\r\n");
    header.append(QString("--%1\r\n").arg(UniqueKeyInlineImage));
    header.append(QString("Content-Type: multipart/alternative;\r\n\tboundary=\"%1\";\r\n").arg(UniqueKeyTexttPlainHtml));
    /// text now 
    return header;
}
/// public

void MailFormat::SetMessage(const QString Subject, const QTextDocument *doc) {
    /// target to make Rawmail!!!
    //// qDebug() << "boundarykey:" << unixtime;
    bool insert = false;
    QString txt = doc->toPlainText();
    QDateTime now = QDateTime::currentDateTime();
    qint64  istime = now.toMSecsSinceEpoch();
    const QString messageasdoc = __TESTWRITEMAIL__ + QString("MailMessage_%1.odt")
            .arg( QString::number(istime));
    
    
    
    QTextDocumentWriter writer(messageasdoc);
    if (writer.write(doc)) {
        // append attachment 
        insert = true;
        ////// qDebug() << "ODT written ok:" << _ODTFORMAT_;
    }


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
            ////qDebug() << "### childFrame ok ";
        } else if (para.isValid()) {
            //// qDebug() << "### para.isValid() ok ";
            QTextBlockFormat ParentBl = para.blockFormat();
            QTextBlock::iterator de;
            for (de = para.begin(); !(de.atEnd()); ++de) {
                QTextFragment fr = de.fragment();
                if (fr.isValid()) {
                    //////  qDebug() << "### fr.fragment() ok ";
                    QTextCharFormat TXTCh = fr.charFormat();
                    QTextImageFormat Pics = TXTCh.toImageFormat();
                    ////QTextTableFormat Tabl = TXTCh.toTableFormat();
                    ////QTextListFormat Uls = TXTCh.toListFormat();
                    if (Pics.isValid() && !Pics.name().isEmpty()) {
                        QString name = Pics.name();
                        imagelist += name; //// search after on html code
                       /////  qDebug() << "### Pics.isValid  ok " << name;
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
        const QString endname = Utils::_partmd5(name.toLocal8Bit(), i) + QString(".png");
        QString srcinsert = QString("src=\"cid:%1\"  alt=\"Image pos %2\"").arg(endname).arg(i);
        ///// qDebug() << "### on list" << name;
        /////  qDebug() << "### on src" << srcinsert;
        xhtml.replace(QString("src=\"%1\"").arg(name), srcinsert);
    }

    Rawmail = "";
    Rawmail.append(ComposeHeader(Subject));
    Rawmail.append(ComposeTxtPlain(txt));
    Rawmail.append(ComposeHtml(xhtml));
    Rawmail.append("\r\n");
    // append inline image from html! here 
    Rawmail.append(Place_Inline_Image(doc)); /// Place_Inline_Image
    Rawmail.append(QString("--%1--\r\n").arg(UniqueKeyInlineImage));
    Rawmail.append("\r\n");
    //// loop attachment file if having!!!
    /* QStringList imagelist;
    QStringList attachmentlist; */

    //// end mail here 
    QString basetextfile = _chunkAttachment(messageasdoc);
    Rawmail.append(basetextfile);
    /// attachment 
    for (int x = 0; x < attachmentlist.size(); ++x) {
        QString namefileabsolute = QString(attachmentlist.at(x).toLocal8Bit().constData());
        QString base64attach = _chunkAttachment(namefileabsolute);
        Rawmail.append(base64attach);
    }
    ///// last line from mail 
    Rawmail.append(QString("--%1--\r\n").arg(UniqueKeyAttachment));
    //// Rawmail.append("\r\n");
}

QString MailFormat::Place_Inline_Image(const QTextDocument *doc) {

    QString code = "";
    if (imagelist.size() < 1) {
        QImage imageg;
        //// load a dummy placeholder to close tag mixed
        QString nameg = QString(":/images/image-x-generic.png");
        const QString endnameg = Utils::_partmd5(nameg.toLocal8Bit(), 999) + QString(".png");
        QFile file(nameg);
        if (file.open(QFile::ReadOnly)) {
            imageg.loadFromData(file.readAll());
            if (!imageg.isNull()) {
                return _inlineImage(imageg, endnameg);
            }
        }
        return code;
    }
    // iterate QStringList imagelist;

    for (int i = 0; i < imagelist.size(); ++i) {
        QImage image;
        QString name = QString(imagelist.at(i).toLocal8Bit().constData());
        const QString endname = Utils::_partmd5(name.toLocal8Bit(), i) + QString(".png");
        int biteloadetmodus = 0;

        if (biteloadetmodus == 0) {
            // try to load direct as normal file
            QFile file(name);
            if (file.open(QFile::ReadOnly)) {
                image.loadFromData(file.readAll());
                if (!image.isNull()) {
                    biteloadetmodus = 1;
                }
            }
        }
        //// qDebug() << "###  biteloadetmodus 1   " << biteloadetmodus;
        if (biteloadetmodus != 1) {
            QUrl url0 = QUrl::fromEncoded(name.toUtf8());
            ////qDebug() << "###  load qurl  " << url0;
            const QVariant data1 = doc->resource(QTextDocument::ImageResource, url0);
            if (data1.type() == QVariant::Image) {
                image = qvariant_cast<QImage>(data1);
            } else if (data1.type() == QVariant::ByteArray) {
                image.loadFromData(data1.toByteArray());
            }
        }
        if (!image.isNull()) {
            biteloadetmodus = 1;
        }
        ////qDebug() << "###  biteloadetmodus 2   " << biteloadetmodus;
        // vvv  Copy pasted mostly from Qt =================
        // http://qtcocoon.googlecode.com/svn/trunk/Tools_OpenDocument_Editor/patch_qt4.5_OO/qtextodfwriter.cpp


        if (biteloadetmodus != 1) {
            if (name.startsWith(QLatin1String(":/"))) {
                name.prepend(QLatin1String("qrc"));
            }
            QUrl url = QUrl::fromEncoded(name.toUtf8());
            ////qDebug() << "###  load qurl  " << url;
            const QVariant data = doc->resource(QTextDocument::ImageResource, url);
            if (data.type() == QVariant::Image) {
                image = qvariant_cast<QImage>(data);
            } else if (data.type() == QVariant::ByteArray) {
                image.loadFromData(data.toByteArray());
            }
        }

        if (!image.isNull()) {
            //// make all png from name!!!!
            ////qDebug() << "### QImage load ok :-) not isNull  " << name;
            const QString newoneimage = _inlineImage(image, endname);
            code.append(newoneimage);
        } else {
            /////qDebug() << "### QImage load  isNull bad!  bad! bad! bad! bad! bad! bad! bad! :-( " << name;
            ////qDebug() << "### QImage load  isNull bad!  bad! bad! bad! bad! bad! bad! bad! :-( " << endname;
        }

    }
    //// qDebug() << "### inline code " << code;
    return code;

}

QString MailFormat::_inlineImage(QImage image, const QString endname) {

    QString code;
    if (!image.isNull()) {
        QBuffer imageBytes;
        QImageWriter imageWriter(&imageBytes, "png");
        imageWriter.setFormat("png");
        imageWriter.setQuality(90);
        imageWriter.setText("Author", "QT Mailchunker");
        imageWriter.write(image);
        // load as chunk base 64
        QString reformat_img = QString(imageBytes.data().toBase64().constData());
        code.append(QString("--%1\r\n").arg(UniqueKeyInlineImage));
        code.append(QString("Content-type: image/png; x-mac-type=\"0\"; x-mac-creator=\"0\"; \n"));
        code.append(QString("\tname=\"%1\"\r\n").arg(endname));
        code.append("Content-Transfer-Encoding: base64\r\n");
        code.append(QString("Content-ID: <%1> \r\n").arg(endname));
        code.append(QString("Content-Disposition: inline;\n\tfilename=\"%1\" \r\n").arg(endname));
        code.append("\r\n");
        code.append(Format_String(reformat_img));
        code.append("\r\n");
    }
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

/* 
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
 */

