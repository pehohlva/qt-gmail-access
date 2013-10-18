//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#include "client_session.h"



MailSession* MailSession::st_ = 0;

MailSession* MailSession::instance() {
    if (st_ == 0) {
        st_ = new MailSession();
    }
    return st_;
}

bool MailSession::register_header(QString block, const int uid, bool rec) {
    bool validmail = false;
    QString Md5 = Utils::_partmd5(block.toUtf8(), uid);
    //// to_file = _READMAILTMPDIR_ + QString("uid-%1.eml.gz").arg(uid);
    QString _from = Filter::lineonPos(QString("From:"), block);
    QString _to = Filter::lineonPos(QString("To:"), block);
    QString _retp = Filter::lineonPos(QString("Return-Path:"), block);
    QString _cco = Filter::lineonPos(QString("Cc:"), block);
    c_subject = Filter::lineonPos(QString("Subject:"), block);
    register_mail(_from);
    register_mail(_to);
    register_mail(_cco);
    register_mail(_retp);
    QString sender = Utils::token(_from, 60, 62); // <> indide caret text
    QString returnsend = Utils::token(_retp, 60, 62);
    /////qDebug() << returnsend << " | " <<   sender;
    ////qDebug() << c_subject;
    if (sender == returnsend) {
        validmail = true;
        if (rec) {
            mailref.insert(uid, block);
        }
        c_from = sender;
    }
    return validmail;
}

void MailSession::register_mail(QString block) {
    if (!block.isEmpty()) {
        QStringList items = block.split(",");
        for (int i = 0; i < items.size(); ++i) {
            const QString mail = QString(items.at(i).simplified());
            QString imail = Utils::token(mail, 60, 62); /// <leonardo.spagnoli@rsi.ch>
            ///// qDebug() << mail << " | " <<   imail;
            MailList_A << imail;
        }
    }
    MailList_A.removeDuplicates();
}

QString MailSession::File_Fromuid(const int uid ) const {
    QString to_file;
    if (_COMPRESSCACHEFILE_ == 1) {
        to_file = _READMAILTMPDIR_ + QString("uid-%1.eml.gz").arg(uid);
    } else {
        to_file = _READMAILTMPDIR_ + QString("uid-%1.eml").arg(uid);
    }
    return to_file;
}

MailSession::MailSession() {
    MailList_A.clear();
}

MailSession::~MailSession() {
    MailList_A.clear();
    delete this;
    st_ = 0;
}

