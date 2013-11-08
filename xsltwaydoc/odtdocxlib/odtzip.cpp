/* 
 * File:   odtzip.cpp
 * Author: pro
 * Struzzo bello in fare le cose
 * Created on 8. November 2013, 11:55
 */

#include "config.h"
#include "odtzip.h"
#include "kzip.h"
#include <QtXML>
#include <QtXmlPatterns>
#include <QAbstractMessageHandler>

class XsltMessageHandler : public QAbstractMessageHandler {

    virtual void handleMessage(QtMsgType type,
            const QString &description,
            const QUrl &identifier,
            const QSourceLocation &sourceLocation) {
        qDebug() << "Error: " << description << " at line " << sourceLocation.line()
                << " char " << sourceLocation.column() << ".";
    }
};


#if 1 //// 1 or 0 
#define DXML qDebug
#else
#define DXML if (0) qDebug
#endif


#define NSREWRITE \
              QString("fo:")

#define __FOPVERSION__ \
              QString("Ve.1.0.5")

OdtZipHandler::OdtZipHandler(const QString odtfile) : is_insert_pic(true) {
    logconsole(QString(), 0);
    if (!dir.exists(__DOCTMPDIR__)) {
        dir.mkpath(__DOCTMPDIR__);
    }
    if (!dir.exists(__DOCTMPDIR__)) {
        logconsole(QString("Unable to create dir on: %1").arg(__DOCTMPDIR__));
        return;
    }
    debugxmlfilexsl = __DOCTMPDIR__ + QString("new.xml");

    logconsole(QString("OpenFile: %1").arg(odtfile));
    Kzip = new KZip::Stream(odtfile);
    if (Kzip->canread()) {
        /// Kzip->explode_todir(__DOCTMPDIR__);
        xmlfilelist = Kzip->listData();
        bxmlbody = Kzip->fileByte("content.xml");
        sxmlbody = QString::fromAscii(bxmlbody.constData());
        if (sxmlbody.size() > 0) {
            logconsole(QString("content.xml is open ..."));
            base_header();
            /// orderin << "meta.xml" << "styles.xml" << "content.xml";
            base_append_xml(Kzip->fileByte("meta.xml"));
            base_append_xml(Kzip->fileByte("styles.xml"));
            bool takeobjects = false;
            if (findobject()) {
                logconsole(QString("Handle summ of obj: %1").arg(sumobj));
                /// check if having object inside!!
                takeobjects = true;
            }
            base_append_xml(sxmlbody.toAscii());
            if (takeobjects) {
                lbock.appendChild(objectitemlist);
            }
            wdom.appendChild(lbock);
            /// wdom ready to read or save to disc debug..
            FULLXMLBODY = wdom.toString(5);
            Tools::_write_file(debugxmlfilexsl, FULLXMLBODY, "utf-8");
            QXmlQuery query(QXmlQuery::XSLT20);
            XsltMessageHandler messageHandler;
            query.setMessageHandler(&messageHandler);
            FULLHTMLBODY.clear();
            query.setFocus(QUrl(debugxmlfilexsl));
            query.setQuery(QUrl(STYLELOCALDIR + QString("odtfullbody.xsl")));
            query.evaluateTo(&FULLHTMLBODY);
            if (query.isValid()) {
                /// insert image 
                if (is_insert_pic) {
                    int summimage = insertimage();
                    logconsole(QString("Total Image insert: %1").arg(summimage));
                }
            } else {
                logconsole(QString("Unable to read xml files!"), 3);
                return;
            }
            logconsole(QString("XHTML size: %1").arg(SystemSecure::bytesToSize(FULLHTMLBODY.size())));
            logconsole(QString("Free Space on HomeDir size: %1").arg(SystemSecure::freespaceonHome()));
        } else {
            logconsole(QString("Unable to read xml files!"), 3);
            return;
        }
        logconsole(QString("Is Open"), 3);
        return;
    }
    logconsole(QString("Not a ODT file! "), 3);

}

void OdtZipHandler::base_append_xml(const QByteArray xmlchunk) {

    QString mdd5 = Tools::fastmd5(xmlchunk);
    RamBuffer *buffer = new RamBuffer("xml");
    buffer->device()->write(xmlchunk);
    QDomDocument current = buffer->xmltoken();
    if (!current.isNull()) {
        Xsl_Include *rdom = new Xsl_Include(current, wdom, lbock);
        logconsole(QString("Success append on OdtZipHandler::base_append_xml md5=%1").arg(mdd5));
    } else {
        logconsole(QString("Unable to append on OdtZipHandler::base_append_xml!"));
    }

}

void OdtZipHandler::base_header() {
    /// QDomDocument wdom; 
    QDomProcessingInstruction header = wdom.createProcessingInstruction("xml", QString("version=\"1.0\" encoding=\"utf-8\""));
    wdom.appendChild(header);
    lbock = wdom.createElement(NSREWRITE + QString("root"));
    lbock.setAttribute("xmlns:fox", "http://xmlgraphics.apache.org/fop/extensions");
    lbock.setAttribute("xmlns:cms", "http://www.freeroad.ch/2013/CMSFormat");
    lbock.setAttribute("xmlns:svg", "http://www.w3.org/2000/svg");
    lbock.setAttribute("xmlns:fo", "http://www.w3.org/1999/XSL/Format");
    lbock.setAttribute("version", __FOPVERSION__);
}

int OdtZipHandler::insertimage() {

    if (!Kzip) {
        return false;
    }
    int found = 0;
    QRegExp expression("src=[\"\'](.*)[\"\']", Qt::CaseInsensitive);
    expression.setMinimal(true);
    int iPosition = 0;
    while ((iPosition = expression.indexIn(FULLHTMLBODY, iPosition)) != -1) {
        const QString imagesrcx = expression.cap(1); /// this must replaced
        int loengh = imagesrcx.length();
        if (imagesrcx.startsWith("Pictures/")) {
            /// grab on zip imagesrcx
            QByteArray xdata = Kzip->fileByte(imagesrcx);
            QImage pic; /// i.value() /// i.key(); 
            pic.loadFromData(xdata);
            if (xdata.size() > 0 && !pic.isNull()) {
                //// ok valid image 
                found++;
                QFileInfo pic(imagesrcx);
                QString blob = "data:image/" + pic.completeSuffix().toLower() + ";base64,";
                QString imagembed(xdata.toBase64().constData());
                blob.append(imagembed); /// format_string76(blob) 
                const QString imagehtml = Tools::f_string76(blob); /// image scroll...
                FULLHTMLBODY.replace(imagesrcx, imagehtml);
            }
        }

        iPosition += expression.matchedLength();
    }

    return found;
}

bool OdtZipHandler::findobject() {

    if (!Kzip) {
        return false;
    }

    objectitemlist = wdom.createElement(NSREWRITE + TAGNAMEITEM);
    QString newtag = QString("obj-0");
    QRegExp expression("xlink:href=[\"\'](.*)[\"\']", Qt::CaseInsensitive);
    expression.setMinimal(true);
    int iPosition = 0;
    sumobj = 0;
    int foundobj = 0;
    while ((iPosition = expression.indexIn(sxmlbody, iPosition)) != -1) {
        const QString name = expression.cap(1); /// this must replaced
        /// ./Object 1
        if (name.startsWith("./Object")) {
            QString base = name.mid(2, name.length() - 2);
            QString bodyxmlo = base + QString("/content.xml");
            QString bodystile = base + QString("/styles.xml");
            QByteArray sxa = Kzip->fileByte(bodyxmlo);
            QByteArray sostile = Kzip->fileByte(bodystile);
            if (!sxa.isEmpty() && !sostile.isEmpty()) {
                foundobj++;
                newtag = QString("obj-%1").arg(foundobj);
                const QString renamelink = MARKEROBJECT + newtag;
                sxmlbody.replace(name, renamelink);
                /// compose xml obj x 
                RamBuffer *bus = new RamBuffer("objectstyles");
                bus->device()->write(sostile);
                QDomDocument d = bus->xmltoken();
                RamBuffer *bub = new RamBuffer("objectbody");
                bub->device()->write(sxa);
                QDomDocument b = bub->xmltoken();
                bub->clear();
                bus->clear();
                if (!d.isNull() && !b.isNull()) {
                    /// compose this 2 doc ...
                    QDomElement slice = wdom.createElement(NSREWRITE + newtag);
                    logconsole(QString("Found object: %1").arg(bodyxmlo));
                    logconsole(QString("Rename object to: %1").arg(newtag));
                    Xsl_Include *rstyle = new Xsl_Include(d, wdom, slice);
                    Xsl_Include *rbody = new Xsl_Include(b, wdom, slice);
                    objectitemlist.appendChild(slice);
                }
            } else {
                sxmlbody.replace(name, MARKEROBJECT + newtag);
            }
        }

        iPosition += expression.matchedLength();
    }
    sumobj = foundobj;
    return (foundobj > 0) ? true : false;
}

QString OdtZipHandler::stream() {
    return FULLHTMLBODY;
}

bool OdtZipHandler::load() {
    return (FULLHTMLBODY.size() > 0) ? true : false;
}

void OdtZipHandler::logconsole(QString msg, int modus) {

    QTextStream out(stdout);
    QString classname = QString("***OdtZipHandler");
    QString str("*");
    if (modus == 0) {
        // init chat..
        out << classname << str.fill('*', iowi - classname.length()) << "\n";
        out << "Start: " << Tools::TimeNow() << "\n";
    }
    out << "Console: " << msg << "\n";
    if (modus == 3 || modus == 2) {
        // close chat..

        out << "End: " << Tools::TimeNow() << "\n";
        out << classname << str.fill('*', iowi - classname.length()) << "\n";
    }
    out.flush();

}

OdtZipHandler::~OdtZipHandler() {


}

Xsl_Include::Xsl_Include(const QDomDocument dom, QDomDocument wdom, QDomElement appender) {
    xml = dom.toString(5);
    if (!xml.isEmpty()) {

        d = dom;
        w = wdom;
        wroot = appender;
        root();
    }
}

void Xsl_Include::root() {

    cursor = 0;
    const QDomElement documentElement = d.documentElement();
    playelement(documentElement, 1, QDomElement());
}

void Xsl_Include::translate_fo(QString & name, QString & value) {
    //// text-underline-style
    if (name == "text-underline-style" && value != "none") {
        name = QString("text-decoration");
        value = QString("underline");
    }
    if (name == "margin") {
        name = QString("odtmargin");
    }
    if (name == "text-line-through-style" && value != "none") {
        name = QString("text-decoration");
        value = QString("line-through");
    }
    if (name == "text-overline-style" && value != "none") {
        name = QString("text-decoration");
        value = QString("overline");
    }

    if (name == "font-name") {

        name = QString("font-family");
    }

    /*   text-shadow: 1pt 1pt; chrome support lol
     * 
     * font-name= font-family  h1 {text-decoration:overline;} text-overline-style 
h2 {text-decoration:line-through;}
h3 {text-decoration:underline;}*/
    //// text-line-through-style: solid;

}

QStringList Xsl_Include::attributes_read(const QDomElement el, QDomElement appender) {
    QStringList att;
    QStringList attnone;
    const QDomNamedNodeMap qatt = el.attributes();
    const int lex = qatt.length();
    if (lex < 1) {
        attnone << "NOATT";
        return attnone;
    }
    for (int i = 0; i < lex; ++i) {
        QDomNode node = qatt.item(i);
        if (node.isAttr()) {
            QDomAttr artt = node.toAttr();
            QString one = artt.name();
            QString value = artt.value();
            translate_fo(one, value);
            appender.setAttribute(one, value);
            one.append("/");
            one.append(value);
            att << one;
            one.clear();
        }
    }
    att << "YESATT"; /// last item!

    return att;

}

void Xsl_Include::playelement(const QDomElement el, int d, QDomElement appender) {
    tag = el.tagName();
    if (!tag.isNull()) {
        if (tag == "styles" && d == 2) {
            tag = QString("css");
        }
        QDomElement cx = w.createElement(NSREWRITE + tag);
        if (d != 1) {
            cx.setAttribute("qlev", QString::number(d));
        }
        QStringList att = attributes_read(el, cx);
        QString text = el.firstChild().toText().data().simplified();
        int firstnode = (el.firstChildElement().tagName().simplified().isEmpty()) ? 0 : 1;
        int lastnode = (el.lastChildElement().tagName().simplified().isEmpty()) ? 0 : 1;
        int istext = (text.isEmpty()) ? 0 : 1;
        //// append text from ... 
        /// qt loost text!! append evry time text if having 1 byte
        QDomText wtxt = w.createTextNode(el.firstChild().toText().data());
        cx.appendChild(wtxt);

        if (d == 1) {
            /// root node from imported file...
            QDateTime timer1(QDateTime::currentDateTime());
            cx.setAttribute("converted-u", QString::number(timer1.toTime_t()));
            cx.setAttribute("converted-h", Tools::UmanTimeFromUnix(timer1.toTime_t()));
            wroot.appendChild(cx);
        } else {
            if (!appender.tagName().isEmpty()) {
                appender.appendChild(cx);
            }
        }

        dcursor.clear();
        //// DXML() << "Render:" << QString("<%1>(%2)").arg(tag).arg(text);
        //// DXML() << d << ") {istext:" << istext << "} type:" << el.nodeType() << " playelement:<" << QString("<%1>(%2)").arg(tag).arg(text) << "> At:" << att;

        if (firstnode == 1 && lastnode == 1) {
            //// having parent one or more 
            ////if (el.firstChildElement().tagName() != el.lastChildElement().tagName()) {
            ////DXML() << tag << " first/last = " << el.firstChildElement().tagName() << " - " << el.lastChildElement().tagName();
            /// iterate the list start
            QDomElement element = el.firstChildElement();
            while (!element.isNull()) {
                if (!element.tagName().simplified().isEmpty()) {
                    playelement(element, d + 1, cx);
                }
                element = element.nextSiblingElement();
            }
            /// iterate the list end    
            ///} else {
            /// read first 
            ////DXML() << ".... readsub next : " << el.firstChildElement().tagName();
            ////playelement(el.firstChildElement(), d + 1);
        } else {

            playelement(el.firstChildElement(), d + 1, cx);

        }


    }



}

Xsl_Include::~Xsl_Include() {
    delete this;
}


/*  class XmlUnion {
public:

    XmlUnion(const QString dir):dirsetting(dir) {
        
        DXML() << dirsetting;
    };

    QString render() {
        if (!dir.exists(dirsetting)) {
            dir.mkpath(dirsetting);
        }
        QString xmldir = dir.absolutePath() + QString("/");
        QDomDocument wdom;
        QDomProcessingInstruction header = wdom.createProcessingInstruction("xml", QString("version=\"1.0\" encoding=\"utf-8\""));
        wdom.appendChild(header);
        QDomElement lbock = wdom.createElement(NSREWRITE + QString("root"));
        lbock.setAttribute("xmlns:fox", "http://xmlgraphics.apache.org/fop/extensions");
        lbock.setAttribute("xmlns:cms", "http://www.freeroad.ch/2013/CMSFormat");
        lbock.setAttribute("xmlns:svg", "http://www.w3.org/2000/svg");
        lbock.setAttribute("xmlns:fo", "http://www.w3.org/1999/XSL/Format");
        lbock.setAttribute("version", __FOPVERSION__);

        for (int i = 0; i < filelist.size(); ++i) {
            const QString item = QString(filelist.at(i));
            StreamBuf *buffer = new StreamBuf("xml");
            QString filehandle = dirsetting + item;
            QFileInfo file_c(filehandle);
            DXML() << file_c.absoluteFilePath();
            buffer->LoadFile(file_c.absoluteFilePath());
            QDomDocument dom = buffer->xmltoken();
            if (!dom.isNull()) {
                Xsl_Include *rdom = new Xsl_Include(dom, wdom, lbock);
            }
            buffer->clear();
        }
        wdom.appendChild(lbock);
        return wdom.toString(5);
    };

    void append(const QString onefile) {
        filelist << onefile;
        filelist.removeDuplicates();
    };

    void append(const QStringList fileitem) {
        for (int i = 0; i < fileitem.size(); ++i) {
            const QString item = QString(fileitem.at(i));
            filelist << item;
        }
        filelist.removeDuplicates();
    };

    virtual ~XmlUnion() {

    };
private:
    QString dirsetting;
    QStringList filelist;
    QString xml;
    QDir dir;
};  */
