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
/// root /Users/pro/project/github/Gmailsend/

#include "mwindow.h"
#include <QApplication>

///// info to build a dmg bundle on mac
//// old 2007 qt http://qxhtml-edit.googlecode.com/svn/trunk/htmledit/mac_bundle/ 
#if defined(_USE_STATIC_BUILDS_)
#include <QtPlugin>
#if defined(_USE_qjpeg)
Q_IMPORT_PLUGIN(qjpeg)
#endif
#if defined(_USE_qgif)
Q_IMPORT_PLUGIN(qgif)
#endif
#if defined(_USE_qmng)
Q_IMPORT_PLUGIN(qmng)
#endif
#if defined(_USE_qtiff)
Q_IMPORT_PLUGIN(qtiff)
#endif
#endif




int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

#if defined Q_WS_MAC
    QStringList path;
    path.append(QApplication::applicationDirPath());
    QDir dir(QApplication::applicationDirPath());
    dir.cdUp();
    path.append(dir.absolutePath());
    dir.cd("plugins");
    path.append(dir.absolutePath());
    dir.cdUp();
    path.append(dir.absolutePath());
    QApplication::setLibraryPaths(path);
    QDir::setCurrent(dir.absolutePath()); /* here down -> Frameworks */
#endif





    a.setOrganizationName("Mail sender smtp Gmail");
    a.setOrganizationDomain("QGmail");
    a.setApplicationName("coremailersender");
    MWindow w;
    w.show();

    return a.exec();
}
