//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#include "net_imap_standard.h"

bool IMail::Setchunk(const QString chunk) {
    //// // tmpqDebug() << "run:" << __FUNCTION__ << ":" << __LINE__;
    fullbody = chunk;
    QDir dira(_TMPMAILDIR_);
    if (dira.mkpath(_TMPMAILDIR_)) {
        // tmpqDebug() << "\n\n";
    }
    imailemlfile = QString("%1new%2.eml").arg(_TMPMAILDIR_).arg(uid.trimmed());
    QFileInfo eml(imailemlfile);
    if (eml.exists()) {
        // dont rewrite chunk!!!
        return true;
    }
    return write_file(imailemlfile, fullbody);
}

bool IMail::exist() {
    if (isValid()) {
        QFileInfo eml(QString("%1new%2.eml").arg(_TMPMAILDIR_).arg(uid.trimmed()));
        //// // tmpqDebug() << "run:" << __FUNCTION__ << ":" << __LINE__ << "param:" << eml.exists();
        return eml.exists();
    } else {
        /// // tmpqDebug() << "run:" << __FUNCTION__ << ":" << __LINE__ << "param:false"; 
    }
    return false;
}

bool IMail::isValid() {

    bool validate = false;
    //// subject, from, date, uid, mime, to, mailer; 
    if (!uid.isEmpty() && !from.isEmpty() && unixtime > 0) {
        validate = true;
    } else {
        //// // tmpqDebug() << "run:" << __FUNCTION__ << ":" << __LINE__ << "param:" << validate; 
    }
    return validate;
}

void IMail::Setheader(QString h, QString id) {
    unixtime = 0;
    uid = id;
    QRegExp messageID("Message-ID: +(.*)");
    QRegExp messageuid2("Message-Id: +(.*)");
    QRegExp messageuid3("Message-id: +(.*)");
    QRegExp subjectMatch("Subject: +(.*)");
    QRegExp fromMatch("From: +(.*)");
    QRegExp dateMatch("Date: +(.*)");
    QRegExp mimeMatch("MIME-Version: +(.*)");
    QRegExp toMatch("Delivered-To: +(.*)");

    h.chop(5);
    QStringList liner = h.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
    infoheader = liner[0];
    liner.removeFirst();
    header = liner.join(QString("\r\n"));
    header.prepend(QString("IMail-Property-Head: %1\n").arg(infoheader));
    ////// header.append("\n");

    //// qFatal("work place stop :-)  hi");

    for (int j = 0; j < liner.length(); j++) {
        QString lineh = liner[j];


        if (subjectMatch.indexIn(lineh) != -1) {
            /////subject = check_Subject(subjectMatch.cap(1));
            subject = subjectMatch.cap(1);
        } else if (dateMatch.indexIn(lineh) != -1) {
            date = dateMatch.cap(1).trimmed();
            ////// tmpqDebug() << "date in :" << date;
            /// // tmpqDebug() << "date in :" << date.mid(5,date.size() - 11);
            /// make localtime
            QDateTime found = QDateTime::fromString(date.mid(5, date.size() - 11), "dd MMM yyyy hh:mm:ss");
            found.setTimeSpec(Qt::UTC);
            unixtime = found.toTime_t();
        } else if (toMatch.indexIn(lineh) != -1) {
            to = toMatch.cap(1).trimmed();;
        } else if (fromMatch.indexIn(lineh) != -1) {
            from = fromMatch.cap(1).trimmed();
        } else if (messageID.indexIn(lineh) != -1) {
            serverid = messageID.cap(1).trimmed();
        } else if (messageuid2.indexIn(lineh) != -1) {
            serverid = messageuid2.cap(1).trimmed();
        } else if (messageuid3.indexIn(lineh) != -1) {
            serverid = messageuid3.cap(1).trimmed();
        } else if (mimeMatch.indexIn(lineh) != -1) {
            mime = mimeMatch.cap(1).trimmed();
        }

    }

    if (!uid.isEmpty()) {
        uid.replace("<", "");
        uid.replace(">", "");
    }


    if (uid.isEmpty()) {
        // tmpqDebug() << "Line header:" << liner << "";
        qFatal("NOOOOO Message-ID: on header !");
    }
    if (mime.isEmpty()) {
        // tmpqDebug() << "NOOOOO mime: on header !";
    }

    if (from.isEmpty()) {
        // tmpqDebug() << "NOOOOO from: on header !";
    }
    /// qFatal
    if (to.isEmpty()) {
        // tmpqDebug() << "NOOOOO to Delivered-To : on header !";
    }

    if (subject.isEmpty()) {
        // tmpqDebug() << "NOOOOO subject: on header !";
    }





}

const QString IMail::Filepointer() {
    if (!uid.isEmpty() && isValid()) {
        QFileInfo eml(QString("%1new%2.eml").arg(_TMPMAILDIR_).arg(uid.trimmed()));
        return eml.absoluteFilePath();
    } else {
        return QString();
    }
}

QString IMail::PrintInfo() {
    if (!uid.isEmpty() && isValid()) {
        QString t;
        QTextStream sw(&t);


        QFileInfo eml(QString("%1new%2.eml").arg(_TMPMAILDIR_).arg(uid.trimmed()));
        /// error to many
        /*  
        if (!infobody.isEmpty()) {
            if (eml.exists()) {
                QFile local_file(eml.absoluteFilePath());
                if (local_file.open(QIODevice::ReadOnly)) {
                    QString reformat_blob = QString(local_file.readAll().constData());
                    QStringList liner = reformat_blob.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
                    QStringList result;
                    result = liner.filter("IMail-Property-Body:");
                    liner.clear();
                    QRegExp bodyInfoMatch("IMail-Property-Body: +(.*)");
                    if (result.size() > 0) {
                        for (int j = 0; j < result.length(); j++) {
                            if (bodyInfoMatch.indexIn(liner[j]) != -1) {
                                infobody = bodyInfoMatch.cap(1);
                            }
                        }
                    }
                    result.clear();
                    Setchunk(reformat_blob);
                }
            }
        }
         * */
        // tmpqDebug() << "H.Message-ID:" << uid;
        // tmpqDebug() << "InfoHeader:" << infoheader;
        // tmpqDebug() << "InfoBody:" << infobody;
        // tmpqDebug() << "Header Mail:" << subject << " date:" << date << "from:" << from;

        sw << "/Header Mail:" << subject << "/date:" << date << "/from:" << from;
        ////if (eml.exists()) {
        sw << "/File:" << eml.absoluteFilePath();
        sw << "/Mailsize:" << bytesToSize(eml.size());

        return t;

    } else {

        return QString("Mailbroken NOT VALID! Unknow format!");
    }
}

/* 
void IMail::Setbody(QString b) {

    b.chop(5);
    QStringList liner = b.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
    infobody = liner[0];
    liner.removeFirst();
    ////// 
    body = liner.join(QString("\r\n"));
    liner.clear();
    body.append(QString("\r\n"));

    bool completed = false;
    if (isValid()) {
        QString chunk = header;
        chunk.prepend(QString("\nIMail-Property-Body: %1\n").arg(infobody));
        chunk.append(body);
        chunk.append(QString("\r\n"));
        completed = Setchunk(chunk);
        QFileInfo eml(imailemlfile);
        if (eml.exists()) {
            completed = true;
        }
    }
    if (!completed) {
        // tmpqDebug() << "incomming Imail NOT VALID:" << body.size() << " uid:" << uid;
        qFatal("mail not valid !");
    }

}
 */

Imap_Cmd::Imap_Cmd() {

    exists = 0; /// totalmessage found inbox / search results  /
    recent = 0;
    uidnext = 0;

}

void Imap_Cmd::setFlags(QString f) {
    flags = f.split(" ", QString::SkipEmptyParts);
}

bool Imap_Cmd::isFlag(const QString f) {
    return flags.contains(f);
}

void Imap_Cmd::setCAPABILITY(QString c) {

    //// send comand -> COMPRESS DELFATE  *****
    if (cmd_CAPABILITY.size() > 0) {
        QString old = cmd_CAPABILITY.join(" ");
        old.append(" ");
        old.append(c);
        cmd_CAPABILITY = old.split(" ", QString::SkipEmptyParts);
    } else {
        cmd_CAPABILITY = c.split(" ", QString::SkipEmptyParts);
    }
    qDebug() << "SERVER:" << cmd_CAPABILITY << " -> " << __FUNCTION__;
}

bool Imap_Cmd::isCAPABILITY(const QString name) {

    QStringList result;
    result = cmd_CAPABILITY.filter(name);
    if (result.size() > 0) {
        return true;
    }
    return false; /// cmd_CAPABILITY.contains(name,Qt::CaseInsensitive);
}

void Imap_Cmd::setSearchResult(const QStringList items) {
    int o = 0;
    rec_uid.clear();
    for (int i = 0; i < items.size(); ++i) {
        QString uid_mail = QString(items.at(i).toLocal8Bit()).trimmed();
        QVariant xnumber(uid_mail);
        if (xnumber.canConvert<int>()) {
            o++;
            rec_uid.insert(o, xnumber.toInt());
        }
    }
}

Imap_Cmd::~Imap_Cmd() {
}

