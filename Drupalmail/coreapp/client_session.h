//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#ifndef CLIENT_SESSION_H
#define	CLIENT_SESSION_H
#include <QObject>
#include <QtCore>
#include "parser_utils.h"
#include "net_starterimap.h"
#include "net_imap_standard.h"
#include "parser_config.h"




    class MailSession{

        public :
        static MailSession * instance();
        QString subject() {
            return c_subject;  /// current stream!!!
        }
         QString from() {
            return c_from;  /// current stream!!!
        }
        bool register_header(QString block , const int uid , bool rec = true );
        QString File_Fromuid(const int uid ) const;
        ~MailSession();

private:
        QHash<int, QString> mailref;
        MailSession();
        void register_mail(QString block);
        static MailSession* st_;
        QStringList MailList_A;
        QString c_subject;
        QString c_from;
        
    };



#endif	/* CLIENT_SESSION_H */

