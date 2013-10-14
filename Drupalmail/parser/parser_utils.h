//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#ifndef PARSER_UTILS_H
#define	PARSER_UTILS_H
#include <QtCore>
#include "parser_config.h"


namespace Utils {

    static const char CarriageReturn_ = '\015';
    static const char LineFeed_ = '\012';

    QString cleanDocFromChartset(const QByteArray chartset , QByteArray chunk );
    
    QByteArray strip_tags(QByteArray istring);
    QByteArray unicode_tr(QByteArray istring);

    QString testDocAttachmentDir(QString file = "");
    QString fastmd5(const QByteArray xml);
    QByteArray search_byline(QByteArray chunk, QByteArray key);
    /// signature md5 from a mail slice & position from document
    QString _partmd5(const QByteArray xml, int position);
    QString token(QString str, QChar c1, QChar c2, int *index);
    QString token(QString text, int unicode);
    QString quotecheck(QString str);
    /// QChar tonicode == caret    "name"  return name or |name| return name 
    /// caret is unicode char separator to take inside;
    QByteArray token(QByteArray text, int caret, int aftercaret = 0);
    QString _stripcore(QString x);
    QByteArray _RX_resolver(const QRegExp rx, QVariant x);
    QString _format_string76(QString s);
    //// text file writteln  default to UTF-8 
    bool _write_file(const QString fullFileName, const QString chunk, QByteArray charset = QByteArray("UTF-8"));
    QStringList _multipart_resolver(QString x);
    QByteArray _reformat_html(const QByteArray in);
    bool _writebin_tofile(const QString xfile, const QByteArray chunk);
    int _distance_position(const int a, const int b);

}


#endif	/* PARSER_UTILS_H */

