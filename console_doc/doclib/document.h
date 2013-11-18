/* 
 * File:   document.h
 * Author: pro
 *
 * Created on 13. November 2013, 08:54
 */

#ifndef DOCUMENT_H
#define	DOCUMENT_H

#include "kzip.h"
#include "config.h"

class Document {
public:
    Document( const QString n = "DOCUMENT" );
    void clean();
    bool swap();
    void readfile(const QString file);
    QByteArray loadbinfile(const QString file);
    bool bufferisNull();
    bool flush_onfile(const QString filedest);
    bool flush_onxmlfile(const QString fullFileName);
    QString execlocal(const QString cmd , const QStringList args);
    QString xslt2_qt5(const QStringList args);
    QDomDocument xmltoken();
    QByteArray convertPNG(QImage image,  int w = 0 , int h = 0);
    void console(QString msg, int modus = 1);

    QIODevice *device() {
        return d;
    }
    
    QString htmlstream() {
        return QString::fromUtf8(d->data().constData()); //       d->data();
    }

    QByteArray stream() {
        return d->data();
    }
    
    

    void SetStyle(const QString file) {
        style_file = file;
    }
    
    void SetBaseFileDoc(const QString file) {
        document_file = file;
    }

    void SetCache(const QString dir) {
        cache_dir = dir;
    }
    
    QString name() const  {
        return document_name;
    }
    QString get_cachedir() const {
        return cache_dir;
    }
    void setError(const QString &error);
    virtual ~Document();
    QString style_file;
    QString document_file;
    QString cache_dir;
    QString html_stream;
    QString document_name;
    QString bin_xsltproc;
    QString bin_pdftohtml;
    QDir dir;
protected:
    
    void _loader();
    QBuffer *d;
};

#endif	/* DOCUMENT_H */

