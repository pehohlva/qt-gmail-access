//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#ifndef TIDY_CLEAN_H
#define	TIDY_CLEAN_H

#include "parser_config.h"

#include "tidy.h" 
#include "tmbstr.h" 
#include "buffio.h" 
#include "stdio.h" 
#include "errno.h" 

namespace HTML {

#define _TIDYTMP_ \
             QString("%1/.tidy/").arg(QDir::homePath())

#define _TIDYCONF_ \
             QString("%1tidy.conf").arg(_TIDYTMP_)

class QTidy {
    
public:
    
    QTidy();
    /// QString TidyCleanfullxhtml(QString body, const QByteArray chartset);
    QString TidyCleanMailHtml(QString body, const QByteArray chartset);
    bool file_put_utf8contents(const QString fullFileName, QString xml, const QByteArray chartset);
    bool SetUp_Xhtml_Mail(const QByteArray chartset);
    virtual ~QTidy();
private:
    void Init(); // set up variable
    bool tidy_file_set_config(QString xml);
    QString Readutf8File(const QString fullFileName);
    bool  CleanTidy(QString inputfile, QString outfile);
    QStringList e_log;
    TidyDoc doc;
    int status;
};




}




#endif	/* TIDY_CLEAN_H */

