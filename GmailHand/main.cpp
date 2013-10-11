//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#include <QCoreApplication>
#include "net_starterimap.h"
#include "parser_eml.h"


/// cd /Users/pro/Desktop/Qt_project/GmailHand/
/// cd /Users/pro/project/version_sv/GmailHand/
//  cd /Users/pro/.GMaildir/

const int liwi = 44;

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv); //renamed the a to app
    QStringList localip;
    QTextStream out(stdout);
    
    QFileInfo feml("/Users/pro/project/version_sv/GmailHand/zzz_sample_mail.eml");
    
    QString str("*");
    out << str.fill('A', liwi) << "\n";
    if (feml.exists()) {  
       out << "Parse File:" << feml.absoluteFilePath() << "\n";  
    } else {  
        out << "Warming file not found:" << feml.absoluteFilePath() << "\n";
    }
    out << str.fill('B', liwi) << "\n";
    out.flush();
 
    //// qDebug() << "unicode: " << newline_br.unicode() << "\n";
    
    /////qFatal("SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN SCAN");
    
    ReadMail::Parser read_mail(feml.absoluteFilePath());
    QTimer::singleShot(500, &app, SLOT(quit()));
    return app.exec();
}

