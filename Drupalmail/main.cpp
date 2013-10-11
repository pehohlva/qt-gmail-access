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

// qmake -spec /usr/local/Qt4.8/mkspecs/macx-g++ -o Makefile Drupalmail.pro
// /Users/pro/project/github/Drupalmail/
//  cd /Users/pro/.GMaildir/

const int liwi = 44;

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv); //renamed the a to app
    QStringList localip;
    QTextCodec *codecx;
    codecx = QTextCodec::codecForMib(106);
    QTextStream out(stdout);
    out.setCodec(codecx);
    /// get the sample change file eml
    QFileInfo feml("sample.eml");
    QByteArray xml;
    QString str("*");
    out << str.fill('A', liwi) << "\n";

     if (feml.exists()) {
        out << "Parse File:" << feml.absoluteFilePath() << "\n";
    } else {
        out << "Warming file not found:" << feml.absoluteFilePath() << "\n";
    }
    out << str.fill('B', liwi) << "\n";
    out.flush();
    //// only parse mail this 
    ReadMail::Parser read_mail(feml.absoluteFilePath());
    //// only parse mail this 
    QTimer::singleShot(500, &app, SLOT(quit()));
    return app.exec();
}

