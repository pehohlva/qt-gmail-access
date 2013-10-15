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
#ifndef MAILFORMAT_H
#define MAILFORMAT_H


#include <QtCore/QString>
#include <QtCore/QtDebug>
#include <QtCore/QDebug>
#include <QApplication>
#include <QtCore/QStringList>

#include <QtCore/QTime>
#include <QtGui/QTextDocument>
#include <QtCore/QCryptographicHash>
#include <QtGui/QTextCursor>
#include <QImage>
#include <QtCore/QUrl>
#include <QtGui/QTextFrame>
#include <QtGui/QTextImageFormat>
#include <QtCore/QStringList>
#include <QtCore/QBuffer>
#include <QtGui/QImageWriter>
#include <QTextCodec>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QTextDocumentWriter>

#include "mime_type.h"

#define __TESTWRITEMAIL__ \
             QString("%1/Downloads/").arg(QDir::homePath())

#define _ODTFORMAT_ \
             QString("%1mail_text_sender.odt").arg(__TESTWRITEMAIL__)

namespace Utils {

    QString _partmd5(const QByteArray xml, int position);

}

inline bool fwriteutf8(QString fullFileName, QString xml) {
    if (fullFileName.contains("/", Qt::CaseInsensitive)) {
        QString ultimacartellaaperta = fullFileName.left(fullFileName.lastIndexOf("/")) + "/";
        QDir dira(ultimacartellaaperta);
        if (dira.mkpath(ultimacartellaaperta)) {
        } else {
            return false;
        }
    }

    QTextCodec *codecx;
    codecx = QTextCodec::codecForMib(106);
    QFile f(fullFileName);
    if (f.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream sw(&f);
        sw.setCodec(codecx);
        sw << xml;
        f.close();
        return true;
    }
    return false;
}

inline QString InfoHumanSize_human(qint64 x) {
    float num = x;
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit("bytes");

    while (num >= 1024.0 && i.hasNext()) {
        unit = i.next();
        num /= 1024.0;
    }
    return QString().setNum(num, 'f', 2) + " " + unit;
}

class MailFormat {
public:
    MailFormat();

    const QString From() {
        return a_frommail;
    }

    const QString To() {
        return a_tomail;
    }

    void SetFrom(QString f) {
        a_frommail = f;
    }

    void SetTo(QString t) {
        a_tomail = t;
    }

    void SetFromTo(QString f, QString tomail) {
        a_frommail = f;
        a_tomail = tomail;
    }

    /* return header body attachment ecc ..  */
    const QString FullChunkMail() {
        qDebug() << "FullChunkMail size:" << InfoHumanSize_human(Rawmail.size());
        return Rawmail;
    }
    /// make null all attachment & rawmail inside!!

    void Clear() {
        Rawmail.clear();
    }
    //// call AppendAttachment bevore SetMessage 
    bool AppendAttachment(QFileInfo filepath);
    void SetMessage(const QString Subject, const QTextDocument *doc);

    bool Valid() {
        return true;
    }




private:

    QString _chunkAttachment(const QString appendfile);
    QString _inlineImage(QImage image, const QString endname);
    QString ComposeHeader(const QString Subject, QString CC = QString("")); /// 0
    QString ComposeTxtPlain(QString txt); /// 1
    QString ComposeHtml(QString html); /// 2
    QString Place_Inline_Image(const QTextDocument *doc);

    QString Format_String(QString s);
    /// attachment
    /// QString encodeQP(QString s); // all base64 encodet
    QString a_frommail;
    QString a_tomail;
    QString now;
    QString Rawmail;
    QString Attachmail;
    QString UniqueKeyInlineImage;
    QString UniqueKeyTexttPlainHtml;
    QString UniqueKeyAttachment;
    qint64 unixtime;
    QStringList imagelist;
    QStringList attachmentlist;
    QString sendhost;
};



#endif // MAILFORMAT_H
