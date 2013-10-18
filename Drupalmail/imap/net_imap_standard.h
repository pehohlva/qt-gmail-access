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

typedef QMap<int, quint16> MailUIDResult;

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

