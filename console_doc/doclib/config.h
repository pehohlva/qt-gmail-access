/* 
 * File:   config.h
 * Author: pro
 *
 * Created on 8.  November 2013, 09:45
 */

#ifndef CONFIG_H
#define CONFIG_H

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
#include <stdio.h>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QStack>
#include <QTextCursor>



#define i18n QObject::tr


const int DEBUGMODUS = 0;
const qint64 LIMITSIZE = 1355776; /// 1.3 mb or so..

const int iowi = 75;
const int pointo = 76;

#define APPCACHEDOC \
             QString("%1/.icache/").arg(QDir::homePath())

#define __localPDFSTYLE__ \
              QString("%1pdf2html.xsl").arg(APPCACHEDOC)
#define __localDOCXSTYLE__ \
              QString("%1docx2html.xsl").arg(APPCACHEDOC)
//// /Users/pro/www/console_doc/src/odt.xsl 
#define __localODTSTYLE__ \
              QString("%1odt2html.xsl").arg(APPCACHEDOC)

#define __TMPNONEDEBUGSETlocalODTSTYLE__ \
              QString("/Users/pro/www/console_doc/src/odt.xsl")

#define __localDTPDT__ \
              QString("%1pdf2xml.dtd").arg(APPCACHEDOC)

#define __netPDFSTYLE__ \
              QString("https://raw.github.com/pehohlva/qt-gmail-access/master/console_doc/pdf2html.xsl")

#define __netPDFDTD__ \
              QString("https://raw.github.com/pehohlva/qt-gmail-access/master/console_doc/pdf2xml.dtd")

#define __netDOCXSTYLE__ \
              QString("https://raw.github.com/pehohlva/qt-gmail-access/master/console_doc/docx2html.xsl")
#define __netODTSTYLE__ \
              QString("https://raw.github.com/pehohlva/qt-gmail-access/master/console_doc/odt2html.xsl")

// by error image insert pix image 
#define __ONEPIXELGIF__ \
              QString("data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==")

#define __DOCVERSION__ \
              QString("Ve.1.2.5")

#define __APPNAME__ \
              QString("Office console Converter")

 //// which xmlpatterns or bundle on installer or mac bundledir..
#define QTXSLT2 \
              QString("/usr/bin/xmlpatterns") 

/*  
 
 xmlpatterns -help

   xmlpatterns -- A tool for running XQuery queries.

  -                          When appearing, any following options are not
                             interpreted as switches.
  -help                      Displays this help.
  -initial-template <string> The name of the initial template to call as a
                             Clark Name.
  -is-uri                    If specified, all filenames on the command line
                             are interpreted as URIs instead of a local
                             filenames.
  -no-format                 By default output is formatted for readability.
                             When specified, strict serialization is
                             performed.
  -output <local file>       A local file to which the output should be
                             written. The file is overwritten, or if not
                             exist, created. If absent, stdout is used.
  -param <name=value>        Binds an external variable. The value is
                             directly available using the variable
                             reference: $name.
  -version                   Displays version information.
  focus <string>             The document to use as focus. Mandatory in case
                             a stylesheet is used. This option is also
                             affected by the is-uris option.
  query/stylesheet <string>  A local filename pointing to the query to run.
                             If the name ends with .xsl it's assumed to be
                             an XSL-T stylesheet. If it ends with .xq, it's
                             assumed to be an XQuery query. (In other cases
                             it's also assumed to be an XQuery query, but
                             that interpretation may change in a future
                             release of Qt.)
 
 
 */



#if 1 //// 1 or 0 
#define BASICDOCDEBUG qDebug
#else
#define BASICDOCDEBUG if (0) qDebug
#endif

#define i18n QObject::tr

/// recalc e convert point cm mm pica etc..

#define POINT_TO_CM(cm) ((cm)/28.3465058)
#define POINT_TO_MM(mm) ((mm)/2.83465058)     ////////  0.352777778
#define POINT_TO_DM(dm) ((dm)/283.465058)
#define POINT_TO_INCH(inch) ((inch)/72.0)
#define POINT_TO_PI(pi) ((pi)/12)
#define POINT_TO_DD(dd) ((dd)/154.08124)
#define POINT_TO_CC(cc) ((cc)/12.840103)

#define MM_TO_POINT(mm) ((mm)*2.83465058)
#define CM_TO_POINT(cm) ((cm)*28.3465058)     ///// 28.346456693
#define DM_TO_POINT(dm) ((dm)*283.465058)
#define INCH_TO_POINT(inch) ((inch)*72.0)
#define PI_TO_POINT(pi) ((pi)*12)
#define DD_TO_POINT(dd) ((dd)*154.08124)
#define CC_TO_POINT(cc) ((cc)*12.840103)


////  10cm  to pixel html converter 

static inline qreal unitfromodt(const QString datain) {
    const QString data = datain.simplified();
    qreal points = 0;
    int error = -1;

    if (data.size() < 1) {
        return -1;
    }

    if (data.endsWith("%")) {
        return 100;
    }

    if (datain == "0") {
        return points;
    }
    if (data.endsWith("pt") || data.endsWith("px")) {
        points = data.left(data.length() - 2).toDouble();
        return points;
    } else if (data.endsWith("cm")) {
        double value = data.left(data.length() - 2).toDouble();
        points = CM_TO_POINT(value);
    } else if (data.endsWith("em")) {
        points = data.left(data.length() - 2).toDouble();
    } else if (data.endsWith("mm")) {
        double value = data.left(data.length() - 2).toDouble();
        points = MM_TO_POINT(value);
    } else if (data.endsWith("dm")) {
        double value = data.left(data.length() - 2).toDouble();
        points = DM_TO_POINT(value);
    } else if (data.endsWith("in")) {
        double value = data.left(data.length() - 2).toDouble();
        points = INCH_TO_POINT(value);
    } else if (data.endsWith("inch")) {
        double value = data.left(data.length() - 4).toDouble();
        points = INCH_TO_POINT(value);
    } else if (data.endsWith("pi")) {
        double value = data.left(data.length() - 4).toDouble();
        points = PI_TO_POINT(value);
    } else if (data.endsWith("dd")) {
        double value = data.left(data.length() - 4).toDouble();
        points = DD_TO_POINT(value);
    } else if (data.endsWith("cc")) {
        double value = data.left(data.length() - 4).toDouble();
        points = CC_TO_POINT(value);
    } else {
        points = 0;
    }


    return points;

}







#endif	/* CONFIG_H */

