//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#include "tidy.h" 
#include "tmbstr.h" 
#include "buffio.h" 
#include "stdio.h" 
#include "errno.h" 

#include "tidy_clean.h"
#include "parser_utils.h"

namespace HTML {

QTidy::QTidy() {
    Init();
}

QTidy::~QTidy() {
    //// file_put_utf8contents(_TIDYCONF_, QString("\n"), "utf-8");
}

void QTidy::Init() {

    status = 0;
    ///QDateTime timer1(QDateTime::currentDateTime());
    ///uint timert = timer1.toTime_t();
    ////// tidy_config_tmp = QString("%1tidy.conf").arg(_TIDYTMP_);
    ////setcurrentcodec = QTextCodec::codecForMib(106); /* utf8 */
    QDir dira(_TIDYTMP_);
    if (dira.mkpath(_TIDYTMP_)) {
        /// reset file if exist
        e_log.append("Init load");
        //// file_put_utf8contents(_TIDYCONF_, QString("\n"), "utf-8");
        e_log.append("Writteln null config");
    }
}

bool QTidy::file_put_utf8contents(const QString fullFileName, QString xml, const QByteArray chartset) {
    e_log.append("write file");
    e_log.append(fullFileName);
    return Utils::_write_file(fullFileName, xml, chartset);
}

/*
 body to clean html
 * chartset original iso *** or utf8
 */
QString QTidy::TidyCleanMailHtml(QString body, const QByteArray chartset) {
    QString newbody;
    bool ok = false;
    const QString debugfile = QString("%1tidydebug.xhtml").arg(_TIDYTMP_);
    file_put_utf8contents(debugfile, body,chartset);
    ///// Utils::_writebin_tofile(debugfile,body.toLocal8Bit()  );
    const QString tocleanfile = QString("%1tidy_tmp.xhtml").arg(_TIDYTMP_);
    const QString cleanresult = QString("%1tidy_tmp_out.xhtml").arg(_TIDYTMP_);
    ok = Utils::_writebin_tofile(tocleanfile,body.toLocal8Bit());
    if (!ok) {
        return body;
    } else {
        SetUp_Xhtml_Mail(chartset);
        qDebug() << "### set up ...  " << chartset;
        CleanTidy(tocleanfile, cleanresult);
        newbody = Readutf8File(cleanresult);
        if (newbody.size() > 2) {
            qDebug() << "### success set up end...  " << newbody;
            return newbody;
        }
    }
    qDebug() << "### failddd !!!!!! ..  " << body;
    return body;
}

/* prepare config tidy config file xhtml clean */
bool QTidy::SetUp_Xhtml_Mail(const QByteArray chartset) {
    int status = 0;
    //// QString codecfile(chartset.constData());
    doc = tidyCreate();
    qDebug() << "### load config. xhtml ...  ";
    
    /* 
     *  --doctype "strict" --output-xhtml y --replace-color y 
     * --uppercase-attributes y --uppercase-tags y --word-2000 y --indent "yes" 
     * --indent-attributes y --indent-spaces "2" --wrap "90" 
     * --wrap-attributes y --char-encoding "raw"
     */
    
    
    bool ok;
    QStringList tidiconfigfile;
    tidiconfigfile.clear();
    tidiconfigfile.append("output-xhtml: yes");
    tidiconfigfile.append("doctype: \"strict\"");
    tidiconfigfile.append("clean: yes");
    tidiconfigfile.append("wrap: 1000");
    tidiconfigfile.append("output-encoding: utf8");
    tidiconfigfile.append("word-2000: yes");
    tidiconfigfile.append("wrap-attributes: no");
    tidiconfigfile.append("drop-proprietary-attributes: yes");
    
    //////qDebug() << "### tidiconfigfile...  " << tidiconfigfile;
    
    //// tidiconfigfile.append("show-body-only: yes"); /* only body checks */
    QString newoneconfiglist = tidiconfigfile.join("\n");
    ok = tidy_file_set_config(newoneconfiglist); /* _TIDYCONF_ */
    if (ok) {
        QByteArray configfileti = _TIDYCONF_.toLocal8Bit();
        status = tidyLoadConfig(doc, configfileti.data()); /* http://ch2.php.net/manual/de/function.tidy-load-config.php */
        if (status !=-1) {
            return true;
        }
    } 
    return false;
}




/*  Readutf8File file_put_utf8contents */

/* read the contenet of a local file as qstring */
QString QTidy::Readutf8File(const QString fullFileName) {
    QString inside = "";
    QFile file(fullFileName);
    if (file.exists()) {
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            inside = QString::fromLocal8Bit(file.readAll());
            file.close();
        }
    }
    return inside;
}

/* write the config file */
bool QTidy::tidy_file_set_config(QString xml) {
    return Utils::_write_file(_TIDYTMP_, xml, "utf-8");
    /* file_put_utf8contents   Readutf8File  */
}

/* QString clean file inputfile and put to outfile */
bool QTidy::CleanTidy(QString inputfile, QString outfile) {
    /*qDebug() << "### load inputfile....  "  << inputfile;*/
    /*qDebug() << "### load outfile....  "  << outfile;*/
    Bool ok;
    int rc = -1;
    int base = -1;
    int status = 0;
    QByteArray infile = inputfile.toLocal8Bit();
    QByteArray outfi = outfile.toLocal8Bit();
    ctmbstr htmlfil = infile.data(); /* incomming file entra */
    ctmbstr outputfil = outfi.data();
    rc = tidyParseFile(doc, htmlfil);

    if (outputfil) {
        status = tidySaveFile(doc, outputfil);

    } else {
        status = tidySaveStdout(doc);
    }
    /* check if file exist if not copy in to out */

    return false;
}


}