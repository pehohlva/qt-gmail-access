//
// C++ Implementation: sample to send mail on smtp from gmail
//
// Description:
// SMTP Encrypted sending mail attachment & mime chunk Editmail->document()
// ( QTextDocument ) fill from image e html tag & style color full mime mail.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution
//  on mac make Makefile by
// qmake -spec /usr/local/Qt4.8/mkspecs/macx-g++ -o Makefile gmailsend.pro
// using qtcreator to make code & netbeanide C++ to format

#include "mwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setOrganizationName("Mail sender smtp Gmail");
    a.setOrganizationDomain("QGmail");
    a.setApplicationName("coremailersender");
    MWindow w;
    w.show();

    return a.exec();
}
