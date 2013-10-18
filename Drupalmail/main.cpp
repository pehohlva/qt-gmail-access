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
#include "coreapp/handler_net.h"

// qmake -spec /usr/local/Qt4.8/mkspecs/macx-g++ -o Makefile Drupalmail.pro
// /Users/pro/project/github/Drupalmail/
//  cd /Users/pro/.GMaildir/

const int liwi = 44;

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv); //renamed the a to app
    QLocale::setDefault(QLocale::English); /// date format mail this is only console or lib! app
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QStringList localip;
    QTextCodec *codecx;
    codecx = QTextCodec::codecForMib(106);
    QTextStream out(stdout);
    out.setCodec(codecx);
    Handler_Net *netapp = new Handler_Net();
    QObject::connect(netapp, SIGNAL(Full_Quit()),&app, SLOT(quit()));

    //// only parse mail this 
    /////ReadMail::Parser *read_mail = new ReadMail::Parser(0,feml.absoluteFilePath());
    //// only parse mail this 
    //// QTimer::singleShot(100000000, &app, SLOT(quit()));
    return app.exec();
}

