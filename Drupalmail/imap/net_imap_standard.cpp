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

