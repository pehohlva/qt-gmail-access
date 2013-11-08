/* 
 * File:   odtzip.h
 * Author: pro
 *
 * Created on 8. November 2013, 11:55
 */

#ifndef ODTZIP_H
#define	ODTZIP_H
#include "kzip.h"
#include "config.h"
/// xml inside a calc table or draw ???
#define MARKEROBJECT \
              QString("/fo:item/fo:")

#define TAGNAMEITEM \
              QString("item")

/*  read dom && rewrite dom if valid */
class Xsl_Include {
public:

    Xsl_Include(const QDomDocument dom, QDomDocument wdom, QDomElement appender);
    virtual ~Xsl_Include();
private:
    void root();
    void playelement(const QDomElement el, int d, QDomElement e);
    QStringList attributes_read(const QDomElement el, QDomElement e);
    void translate_fo(QString & name, QString & value);
    QDomDocument d;
    QDomDocument w;
    QDomElement wroot;
    QDomElement dcursor;
    int cursor;
    int deep;
    QString xml;
    QString tag;

};

class OdtZipHandler {
public:
    OdtZipHandler( const QString odtfile  );
    bool load();
    QString stream();
    virtual ~OdtZipHandler();
private:
    void logconsole(QString msg = QString() , int modus = 1);
    bool findobject();
    int  insertimage();
    void base_header(); /// xml header
    void base_append_xml(const QByteArray xmlchunk  ); // append elements
    KZip::Stream *Kzip;
    QMap<QString, QByteArray> xmlfilelist;
    QDir dir;
    int sumobj;
    bool is_insert_pic;
    QString sxmlbody;
    QString debugxmlfilexsl;
    QString FULLXMLBODY;
    QString FULLHTMLBODY;
    QByteArray bxmlbody;
    /// xml item
    QDomDocument wdom;
    QDomElement objectitemlist;
    QDomElement lbock; /// cursor to write!

};

#endif	/* ODTZIP_H */

