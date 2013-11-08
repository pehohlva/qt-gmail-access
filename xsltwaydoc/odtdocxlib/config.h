/* 
 * File:   config.h
 * Author: pro
 *
 * Created on 8. November 2013, 09:45
 */

#ifndef CONFIG_H
#define	CONFIG_H

#include <QtCore/qfile.h>
#include <QtCore/qstring.h>
#include <QtCore/QMap>
#include <QtCore/QDebug>
#include <QtCore/QBuffer>
#include <QtCore/qiodevice.h>
#include <QtCore/qbytearray.h>
#include <qstringlist.h>
#include <QtXml/QDomDocument>
#include <QtGui/QTextDocument>
#include <QCryptographicHash>
#include <QDate>
#include <QDateTime>
#include <QStringList>
#include <QDomElement>
#include <QBuffer>
#include <QTextCodec>
#include <QFile>
#include <QString>
#include <QDebug>
#include <QDir>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QDomDocument>
#include <QXmlQuery>
#include <QImage>



// swap 0  on debug  lib connect on __DOCTMPDIR__ 
#define __DOCTMPDIR0__ \
             QString("%1/.DocumentCache/").arg(QDir::homePath())

#define __DOCTMPDIR__ \
              QString("/Users/pro/code/minisvn/doc/Xsltqt5/cache/")


#define __DEBUGFILEHTML__ \
              QString("/Users/pro/code/minisvn/doc/Xsltqt5/index.html")

#define STYLELOCALDIR \
              QString("/Users/pro/code/minisvn/doc/Xsltqt5/style/")


const int iowi = 75;

#if 1 //// 1 or 0 
#define BASICDOCDEBUG qDebug
#else
#define BASICDOCDEBUG if (0) qDebug
#endif

#define i18n QObject::tr



#define __DOCVERSION__ \
              QString("Ve.1.2.5")

#define __APPNAME__ \
              QString("Office DocConverter")





#endif	/* CONFIG_H */

