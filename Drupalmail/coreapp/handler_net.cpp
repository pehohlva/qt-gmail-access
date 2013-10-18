//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution
#include "handler_net.h"
#include "net_starterimap.h"
#include "client_session.h"

//// session static to register data pass name ecc.
//// http://fop-miniscribus.googlecode.com/svn/trunk/fop_miniscribus.2.0.0/src/SessionManager.h

Handler_Net::Handler_Net(QObject *parent) :
QObject(parent) {

    bool get = true;
    if (get) {
        imap = new Net_StarterImap(this);
        imap->SearchWord(QString("lugano"), -1);
        imap->Connect(QString("hjkhhkhhkhkh@gmail.com"), QString("dftvgbhnjbhgvfrdecfv"));
        connect(imap, SIGNAL(Exit_Close()), this, SLOT(Close_Imap()));
        connect(imap, SIGNAL(Next_Standby()), this, SLOT(NextComand()));
        connect(imap, SIGNAL(Progress(int)), this, SLOT(Init_Parse(int)));
    }

    /////const QString fileheder = QString("/Users/pro/.GMaildir/sample.txt");
    ///QString header = Utils::utf8loadfromfile(fileheder);
    //// QString _from = Filter::lineonPos(QString("From:"), header);

    






    //// emit Full_Quit();



    /// /Users/pro/project/github/Drupalmail/
}


void Handler_Net::Init_Parse( const int uid ) {
    
    MailSession *mini = MailSession::instance();
    
    QTextStream out(stdout);
    QString str('*');
    out << str.fill('.', _IMAIL_WIDTH_) << "\n";
    out << "Init handle parse:" << uid << "\n";
    out << str.fill('.', _IMAIL_WIDTH_) << "\n";
    out.flush();
    
}

void Handler_Net::Close_Imap() {
    emit Full_Quit();
}

//// a list from search word... on subject

void Handler_Net::NextComand() {
    if (imap) {
        imap->PreparetoClose();
        //// void SetQuery( const QString word );
    }
    emit Full_Quit();
}
