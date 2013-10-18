//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#ifndef PARSER_CONFIG_H
#define	PARSER_CONFIG_H


#include <QDataStream>
#include <iostream>
#include <QtCore>
#include <QCoreApplication>
#include <QMap>
#include <QtGlobal>
#include "mail_handler.h"
#include "mime_standard.h"
#include "kcodecs.h"
#include "parser_utils.h"
#include <QTextDocument>

#ifndef QT_NO_EMIT
# define emit
#endif
///// Q_ASSERT_X()
#define _BOUNDARY_MAXL_  110 /// rcf say 70
#define _IMAIL_MAXL_  100

#define _IMAIL_LARGE_  76

const int _COMPRESSCACHEFILE_  = 1;
const int MAX_NR_ATTACMENT  = 66;

#define APOSSIMPLE \
             QChar(120) /// unicode 27 '   120

#define QUOTEDCHAR \
             QChar(22) /// unicode 22 "

#define WORD2000APOQUOTE \
             QByteArray("##39#183##") /// word quote 
/// #

#define __NULLDATA__ \
             QString(QChar('*')) /// unicode 62

#define __MA__ \
             QString("alternative")) /// 

#define __MX__ \
             QString("mixed")) /// 

#define __SI__ \
             QString("signed")) /// 

#define __DI__ \
             QString("digest")) /// 

#define __RE__ \
             QString("related")) /// 

#define _READMAILTMPDIR_ \
             QString("%1/.GMaildir/").arg(QDir::homePath())

#define _TESTDOCDIR_ \
             QString("%1/doc_test/").arg(QDir::currentPath())

#define __RUNBOUNDARY__ \
             QRegExp("boundary[\\S'](.*)[\\s]",Qt::CaseInsensitive)

#define __OLDBOUNDARY__ \
             QRegExp("boundary=(.*)[\"\'\\s\\n\\r]",Qt::CaseInsensitive)

/// name=
#define _FNAMEATT_ \
             QRegExp("Content-ID: +[<](.*)[>]",Qt::CaseInsensitive)

/// charset=
#define _CHARTSETMM_ \
             QRegExp("charset=+(.*)[;\\s\\t\\n]",Qt::CaseInsensitive)

/// charset=
#define _CHARTSETMMA_ \
             QRegExp("charset=+(.*)[;\\s\\t\\n]",Qt::CaseInsensitive)
/// Content-Transfer-Encoding
#define _COTRANSFERENCODING_ \
             QRegExp("Content-Transfer-Encoding: +(.*)[\\s\\t\\n]",Qt::CaseInsensitive)
/// PHP X mailer PHPMailer 5.2.4 (http://code.google.com/a/apache-extras.org/p/phpmailer/)
#define _XMAILERSENDER_ \
             QRegExp("X-Mailer:+(.*)[;\\s\\t\\n]",Qt::CaseInsensitive)

/// filename= or name=
#define _FILENAMEATT_ \
             QRegExp("ame[\\S'](.*)$",Qt::CaseInsensitive)


/// run medium = Content-Type:+(.*)[;\\s\\t\\n]
/// Content-Type   //// Content-Type:+(.*)[;|\s|\n]
#define _CONTENTYPE_ \
             QRegExp("Content-Type:+(.*)[;|\\s|\n]",Qt::CaseInsensitive)


/* 
 Content-ID: <ii_1419f1d6df51ba5b>
X-Attachment-Id: ii_1419f1d6df51ba5b
 */
#define _IDCOATTACMENT_ \
             QRegExp("t-ID:(.*)",Qt::CaseInsensitive)



typedef QMap<QString, QString> Mail_Field_Format;
const QByteArray _FILESTARTER_ = "***LOCK***";
#ifdef Q_WS_MAC
const QString multipartchunk = " ##end##";
const QByteArray _FILECLOSER_ = "\n\rLOCK***LAS_TOS***\n\r";
#else
const QString multipartchunk = " ##grepend##";

const QByteArray _FILECLOSER_ = "\n\rLOCK***LAS_WIN***\n\r";
#endif
const QString PHPMAILERFROMSERVER = "#MOUNTBODY#";
#define MINIMUMHEADERLENGTH 76
#define MINIMUMBODYLENGTH 11
static const int MINFULMAIL_LENGTH = (MINIMUMBODYLENGTH + MINIMUMHEADERLENGTH);
typedef QPair<int, int> DocSlice;






/** @short Convert a hex digit into a number */
static inline int hexValueOfChar(const char input) {
    if (input >= '0' && input <= '9') {
        return input - '0';
    } else if (input >= 'A' && input <= 'F') {
        return 0x0a + input - 'A';
    } else if (input >= 'a' && input <= 'f') {
        return 0x0a + input - 'a';
    } else {
        return -1;
    }
}

/** @short Translate a quoted-printable-encoded array of bytes into binary characters

The transformations performed are according to RFC 2047; underscores are transferred into spaces
and the three-character =12 escapes are turned into a single byte value.
 */
inline static QByteArray _translateFromQuotedPrintable(const QByteArray &input) {
    
    
    
     ///// this make link html broken!!!
    QByteArray res;
    for (int i = 0; i < input.size(); ++i) {
        if (input[i] == '_') {
            res += ' ';
        } else if (input[i] == '=' && i < input.size() - 2) {
            int hi = hexValueOfChar(input[++i]);
            int lo = hexValueOfChar(input[++i]);
            if (hi != -1 && lo != -1) {
                res += static_cast<char> ((hi << 4) + lo);
            } else {
                res += input.mid(i - 2, 3);
            }
        } else {
            res += input[i];
        }
    }
    return res;
     
}


#ifdef DRUPAL_REMOTE
const QString drupalsecks = " ##end##";
const quint64 SIBILLING_MAIL = 2364161961;
#include <ctype.h>
#include "drupal_journalist.h"
#else
const quint64 SIBILLING_MAIL = 1095710611;
const QString drupalsecks = " ##cat##";
#endif







#include <QBuffer>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QList>
#include <QStringList>
#include <QObject>
#include <QDateTime>
#include <QDate>
#include <QSettings>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <QIODevice>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDir>


#include <QByteArray>
#include <QTCore>
#include <QtCore>

/* 
 * unicode letter nr.
 letter: 'A' = 65 
letter: 'B' = 66 
letter: 'C' = 67 
letter: 'D' = 68 
letter: 'E' = 69 
letter: 'F' = 70 
letter: 'G' = 71 
letter: 'H' = 72 
letter: 'I' = 73 
letter: 'J' = 74 
letter: 'K' = 75 
letter: 'L' = 76 
letter: 'M' = 77 
letter: 'N' = 78 
letter: 'O' = 79 
letter: 'P' = 80 
letter: 'Q' = 81 
letter: 'R' = 82 
letter: 'S' = 83 
letter: 'T' = 84 
letter: 'U' = 85 
letter: 'V' = 86 
letter: 'W' = 87 
letter: 'Y' = 89 
letter: 'Z' = 90 
letter: '*' = 42 
 * space == 32!
 */
#endif	/* PARSER_CONFIG_H */

