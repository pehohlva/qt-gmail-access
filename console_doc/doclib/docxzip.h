/* 
 * File:   docxzip.h
 * Author: pro
 *
 * Created on 12. November 2013, 22:54
 */

#ifndef DOCXZIP_H
#define	DOCXZIP_H

#include "kzip.h"
#include "config.h"
#include "document.h"

class DocxZipHandler : public Document {
public:
    DocxZipHandler( const QString docfilename );
    QString html();
    bool load() {
        return is_loading;
    }
    virtual ~DocxZipHandler();
private:
    QByteArray docitem( const QString archive );
    bool docx_validator(const QStringList entries);
    void scanimage();
    QString read_docx_index(QByteArray xml, int findertype, const QString Xid);
    bool is_loading;
    QMap<QString, QByteArray> corefile;
    QString HTMLSTREAM;

};

#endif	/* DOCXZIP_H */

