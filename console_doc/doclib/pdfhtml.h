/* 
 * File:   pdfhtml.h
 * Author: pro
 *
 * Created on 14. November 2013, 18:49
 */

#ifndef PDFHTML_H
#define	PDFHTML_H

#include "kzip.h"
#include "config.h"
#include "document.h"



class PortableDocument : public Document {
public:
    PortableDocument( const QString docfilename );
    virtual ~PortableDocument();
private:
    bool is_loading;
    void ScanImageHtml(); // on html insert image 
    void ScannerImage(); /// on cache dir read image 
    QByteArray docitem(const QString archive);
    QString EmbeddedImage(const QString key);
    QMap<QString, QByteArray> pdfimagelist;
    QString HTMLSTREAM;
};

#endif	/* PDFHTML_H */

