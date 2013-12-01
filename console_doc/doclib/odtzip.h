/* 
 * File:   odtzip.h
 * Author: pro
 *
 * Created on 8. November 2013, 11:55
 */
 
 //// memo union xml file http://sourceforge.net/p/qtexcel-xslt/code/HEAD/tree/_odbc_wizard/browser.cpp#l615

#ifndef ODTZIP_H
#define	ODTZIP_H
#include "kzip.h"
#include "config.h"
#include "document.h"

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
    void translate_fo(QString & name, QString & value, QDomElement appender);
    QDomDocument d;
    QDomDocument w;
    QDomElement wroot;
    QDomElement dcursor;
    int cursor;
    int deep;
    QString xml;
    QString tag;

};

class OdtZipHandler : public Document  {
public:
    OdtZipHandler(const QString odtfile);
    bool is_Load() {
        return (FULLHTMLBODY.size() > 0) ? true : false;
    }
    QString stream_html() {
        return FULLHTMLBODY;
    }
    virtual ~OdtZipHandler();
private:
    void logconsole(QString msg = QString(), int modus = 1);
    QByteArray docitem(const QString archive);
    bool findobject();
    int insertimage();
    void base_header(); /// xml header
    void base_append_xml(const QByteArray xmlchunk); // append elements
    QMap<QString, QByteArray> xmlfilelist;
    QDir dir;

    int sumobj;
    /// debug modus false
    bool is_insert_pic;
    bool success_odt;



    QString XMLTMP; /// copy paste item 
    QString FULLXMLBODY; /// the ready xmlto convert 
    /// filled this at end parse...
    QString FULLHTMLBODY; /// the full ready htmlcode + embbeded image...
    /// xml item
    QDomDocument wdom;
    QDomElement objectitemlist;
    QDomElement lbock; /// cursor to write!

};

#endif	/* ODTZIP_H */

