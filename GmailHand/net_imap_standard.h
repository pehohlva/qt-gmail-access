//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#include <QtDebug>
#include <QQueue>
#include <QVariant>
#include <QStringList>
#include <iostream>
#include <QtCore>
#include <QCoreApplication>
#include <QMap>
#include <QList>
#include <QStringList>
#include <QObject>
#include <QDateTime>
#include <QDate>
#include <QSettings>
#include <QDebug>
#include <QObject>
#include <QString>
#include "mail_handler.h"
#include <QNetworkInterface>
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#ifndef NET_IMAP_STANDARD_H
#define	NET_IMAP_STANDARD_H

typedef QMap<int, quint16> MailUIDResult;
#define _TMPMAILDIR_ \
             QString("%1/.GMaildir/").arg(QDir::homePath())

class IMail {
public:

    IMail() {
        fullbody = "nulmail";
    }
    static const char CarriageReturn = '\015';
    static const char LineFeed = '\012';
    static const int maxcharline = 76;

    const QChar asrerix() {
        return QChar(42);
    }
    bool exist();
    QString PrintInfo();
    //// first insert header and after check exist()
    /// if file is on cache load from this!
    void Setheader(QString h, QString id);
    /// void Setbody(QString b);

    virtual ~IMail() {
    }
    bool isValid();

    const QString Filepointer();

    void flush() {
        fullbody.clear();
        header.clear();
        body.clear();
        uid.clear();
        subject.clear();
        from.clear();
        imailemlfile.clear();
        infoheader.clear();
        infobody.clear();
        date.clear();
        mailer.clear();
        to.clear();
        serverid.clear();
    }

    QString Subject() {
        if (subject.isEmpty()) {
            return QString("No Subject mail /%1/").arg(unixtime)+date;
        }
        return QString("%1/%2/").arg(subject).arg(unixtime)+date;
    }

private:
    bool Setchunk(const QString chunk);
    QString imailemlfile; // cache this data 
    quint64 unixtime;
    QString fullbody;
    QString header;
    QString body;
    QString subject, from, date, date_original, uid, mime, to, mailer, serverid;
    QString infoheader, infobody;
};

int inline static getnumber(QString text, int xat = 0) {
    QRegExp rx("(\\d+)");
    QStringList nlist;
    int pos = 0;
    int vol = xat + 2;
    while ((pos = rx.indexIn(text, pos)) != -1) {
        nlist << rx.cap(1);
        pos += rx.matchedLength();
    }
    if (nlist.size() > 0 && nlist.size() < vol) {
        QVariant xnumber(nlist.at(xat).toLocal8Bit());
        if (xnumber.canConvert<int>()) {
            return xnumber.toInt();
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

static inline void imapselect_krass_microsleep(const quint64 callertimer) {
    /// timespec delay = {nsec / 1000000000, nsec % 1000000000};
    /////timespec esimap = 2000000;
    ///// pselect(0, NULL, NULL, NULL, &esimap, NULL);
    QTextStream out(stdout);
    out << callertimer << ".08";
    out.flush();
}
/// found on http://cs.uef.fi/paikka/karol/mopsi20130812_useless/mobile/tools/src/util/formatutil.cpp

static inline QString bytesToSize(qint64 size) {
    if (size < 0)
        return QString();
    if (size < 1024)
        return QObject::tr("%1 B").arg(QString::number(((double) size), 'f', 0));
    if ((size >= 1024) && (size < 1048576))
        return QObject::tr("%1 KB").arg(QString::number(((double) size) / 1024, 'f', 0));
    if ((size >= 1048576) && (size < 1073741824))
        return QObject::tr("%1 MB").arg(QString::number(((double) size) / 1048576, 'f', 2));
    if (size >= 1073741824)
        return QObject::tr("%1 GB").arg(QString::number(((double) size) / 1073741824, 'f', 2));
    return QString();
}

static inline void imaplist_microsleep(unsigned long nsec) {
    timespec delay = {nsec / 1000000000, nsec % 1000000000};
    pselect(0, NULL, NULL, NULL, &delay, NULL);
}

static inline QString Format_st76(QString s) {
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


/// open file as text 

static inline const QString get_contenent_file(const QString fullFileName) {
    QFile file(fullFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();

    QTextCodec *codecx;
    codecx = QTextCodec::codecForMib(106);

    QTextStream in(&file);
    in.setCodec(codecx);
    QString text;
    text = in.readAll();
    file.close();
    return text;
}

/* write a file to utf-8 format */
static inline bool write_file(const QString fullFileName, const QString chunk) {

    QTextCodec *codecx;
    codecx = QTextCodec::codecForMib(106);
    QFile f(fullFileName);
    if (f.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream sw(&f);
        sw.setCodec(codecx);
        sw << chunk;
        f.close();
        return true;
    }
    return false;
}

static QString decodeQP(QString s) {
    s.replace("=\r\n", "");
    s.replace("=\n", "");
    for (int i = 0; i < s.size(); i++) {
        if (s.at(i) == QChar('=') && s.at(i + 1) != QChar('"')) {
            QString str = s.mid(i + 1, 2);
            char **c;
            str = QChar((int) strtol(str.toAscii().data(), c, 16));
            s.replace(i, 3, str);
        }
    };
    return s;
}

static inline QString encodeQP(QString s) {
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

static inline QString check_Subject(QString su, bool de = false) {
    const QString sub = su.trimmed();
    QString news = su;
    if (su.isEmpty()) {
        return QString("No Subject");
    }
    if (su.startsWith("=?utf-8?B?")) {
        /// checked is perfect
        news = QString::fromUtf8(QByteArray::fromBase64(su.mid(10, su.length() - 2).toAscii()).constData());
    } else if (su.startsWith("=?utf-8?Q?")) {
        news = QString::fromUtf8(decodeQP(su.mid(10, su.length() - 2)).toAscii());
    } else if (su.startsWith("=?ISO-8859-1?Q?")) {
        news = QString::fromLatin1(decodeQP(su.mid(15, su.length() - 2)).toAscii());
    } else if (su.startsWith("=?ISO-8859-1?B?")) {
        news = QString::fromLatin1(QByteArray::fromBase64(su.mid(15, su.length() - 2).toAscii()).constData());
    }
    if (de) {
        news.append(" /original/");
        news.append(su);
    }
    return news;
}

static inline QDateTime extractDate(const QString& field) {
    ////QRegExp dateFormat("INTERNALDATE *\"([^\"]*)\"");
    ////if (dateFormat.indexIn(field) != -1) {
    ////QString date(dateFormat.cap(1));
    QDateTime found = QDateTime::fromString(field, "ddd, dd MMM yyyy hh:mm:ss");
    QDateTime utc = QDateTime::currentDateTimeUtc();
    //// Thu, 26 Sep 2013 09:00:43 +0000
    QRegExp format("(\\d+)-(\\w{3})-(\\d{4}) (\\d{2}):(\\d{2}):(\\d{2}) ([+-])(\\d{2})(\\d{2})");
    if (format.indexIn(field) != -1) {
        static const QString Months("janfebmaraprmayjunjulaugsepoctnovdec");
        int month = (Months.indexOf(format.cap(2).toLower()) + 3) / 3;

        QDate dateComponent(format.cap(3).toInt(), month, format.cap(1).toInt());
        QTime timeComponent(format.cap(4).toInt(), format.cap(5).toInt(), format.cap(6).toInt());
        int offset = (format.cap(8).toInt() * 3600) + (format.cap(9).toInt() * 60);

        QDateTime timeStamp(dateComponent, timeComponent, Qt::UTC);
        timeStamp = timeStamp.addSecs(offset * (format.cap(7)[0] == '-' ? 1 : -1));
        return timeStamp;
    }
    ////}

    return found;
}

static inline QString token(QString str, QChar c1, QChar c2, int *index) {
    int start, stop;

    // The strings we're tokenizing use CRLF as the line delimiters - assume that the
    // caller considers the sequence to be atomic.
    if (c1 == IMail::CarriageReturn)
        c1 = IMail::LineFeed;
    start = str.indexOf(c1, *index, Qt::CaseInsensitive);
    if (start == -1)
        return QString();

    if (c2 == IMail::CarriageReturn)
        c2 = IMail::LineFeed;
    stop = str.indexOf(c2, ++start, Qt::CaseInsensitive);
    if (stop == -1)
        return QString();

    // Exclude the CR if necessary
    if (stop && (str[stop - 1] == IMail::CarriageReturn))
        --stop;

    // Bypass the LF if necessary
    *index = stop + 1;
    return str.mid(start, stop - start);
}

class Imap_Cmd {
public:
    Imap_Cmd();

    enum State {
        IMAP_Unconnected = 0,
        IMAP_Init,
        IMAP_Capability,
        IMAP_Idle_Continuation,
        IMAP_StartTLS,
        IMAP_Login_Ok,
        IMAP_SendLogin,
        IMAP_Logout,
        IMAP_List,
        IMAP_Select,
        IMAP_Examine,
        IMAP_Search,
        IMAP_Append,
        IMAP_UIDSearch,
        IMAP_UIDFetch,
        IMAP_UIDStore,
        IMAP_UIDCopy,
        IMAP_Expunge,
        IMAP_GenUrlAuth,
        IMAP_Close,
        IMAP_Full,
        IMAP_Idle,
        IMAP_Cunnk_Incomming
    };

    void setExists(quint32 e) {
        exists = e;
    }

    quint32 Exists() const {
        return exists;
    }

    void setRecent(quint32 e) {
        recent = e;
    }

    quint32 Recent() const {
        return recent;
    }

    void setFlags(QString f);
    
    bool isFlag(const QString f);

    void setCAPABILITY(QString c);
    
    bool isCAPABILITY(const QString name);

    void SetComand(State newstate) {
        s = newstate;
    }

    void setUidNext(quint32 e) {
        uidnext = e;
    }

    quint32 UidNext() const {
        return uidnext;
    }

    State CurrentComand() {
        return s;
    }

    void setSearchResult(const QStringList items);

    MailUIDResult getUidSearch() {
        return rec_uid;
    }

    void i_debug() {
        // tmpqDebug() << "Imap debug start..";
        // tmpqDebug() << "search size:" << getUidSearch().size();
        // tmpqDebug() << "exists:" << exists;
        // tmpqDebug() << "state:" << s;
        // tmpqDebug() << "uidnext:" << uidnext;
        // tmpqDebug() << "recent:" << recent;
        // tmpqDebug() << "flags:" << flags;
        // tmpqDebug() << "Imap debug end..";
    }

    ///   _cmd->SetComand(Imap_Cmd::IMAP_Unconnected);
    virtual ~Imap_Cmd();
private:
    State s;
    quint32 exists;
    quint32 uidnext;
    quint32 recent;
    QStringList flags;
    QStringList cmd_CAPABILITY;
    MailUIDResult rec_uid;
};

#endif	/* NET_IMAP_STANDARD_H */

