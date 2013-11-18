/* 
 * File:   odtzip.cpp
 * Author: pro
 * Struzzo bello in fare le cose
 * Created on 8. November 2013, 11:55
 */

#include "config.h"
#include "odtzip.h"
#include "kzip.h"
#include <QtXmlPatterns>



#if 1 //// 1 or 0 
#define DXML qDebug
#else
#define DXML if (0) qDebug
#endif


#define NSREWRITE \
              QString("fo:")

#define __FOPVERSION__ \
              QString("Ve.1.0.5")

OdtZipHandler::OdtZipHandler(const QString odtfile) : Document("ODT"), is_insert_pic(true), success_odt(false) {

    QFileInfo infofromfile(odtfile);
    bool doc_continue = true;
    bool debugmodus = (DEBUGMODUS == 1) ? true : false;
    console(QString("Init talking this class. %1..").arg(this->name()), 0);
    const QString rootxmlfile = QString("%1index.xml").arg(cache_dir);
    const QString indexhtmlfile = QString("%1index.html").arg(cache_dir);
    QFile::remove(indexhtmlfile);
    QFile::remove(rootxmlfile);
    /// xslt style is __localODTSTYLE__ fresch download from net ... evry time updated
    this->SetStyle(__localODTSTYLE__); /// xslt style file
    logconsole(QString("OpenFile: %1").arg(odtfile));
    KZip::Stream *unzip = new KZip::Stream(odtfile);
    if (!unzip->canread()) {
        doc_continue = false;
    }
    if (doc_continue) {
        if (debugmodus) {
            unzip->explode_todir(cache_dir, 1);
        }
        /// validate file list from zip unzip->filelist()
        xmlfilelist = unzip->listData(); /// qmap data name & QMap<QString, QByteArray>
        /// close zip all piece having here on xmlfilelist image to..
    }
    //// close on all case
    unzip->~Stream();
    unzip = NULL;
    /// open file and read 
    if (doc_continue) {
        QByteArray xbase = docitem("content.xml");
        RamBuffer *bxsmll = new RamBuffer("basedocumentodtxml");
        bxsmll->device()->write(xbase);
        XMLTMP = bxsmll->fromUtf8();
        bxsmll->clear();
    }

    /// const QString rootxmlfile = QString("%1index.xml").arg(cache_dir);
    /// const QString indexhtmlfile = QString("%1index.html").arg(cache_dir);
    if (XMLTMP.size() < 2) {
        doc_continue = false;
    }

    if (doc_continue && XMLTMP.size() > 0) {
        logconsole(QString("content.xml is open ..."));
        base_header(); /// append style object meta ecc...
        base_append_xml(docitem("meta.xml"));
        base_append_xml(docitem("styles.xml"));
        bool takeobjects = false;
        if (findobject()) { //// here is rewritten XMLTMP if calc sheet inside 
            logconsole(QString("Handle summ of obj: %1").arg(sumobj));
            /// check if having object inside!!
            takeobjects = true;
        }
        base_append_xml(XMLTMP.toLocal8Bit());
        if (takeobjects) {
            lbock.appendChild(objectitemlist);
        }
        wdom.appendChild(lbock);
        /// wdom ready to read or save to disc debug..
        XMLTMP.clear();
        //// QDomDocument doc = this->xmltoken();
        QDomElement el = wdom.documentElement();
        el.setAttribute("dateconvert", Tools::TimeNow());
        el.setAttribute("docname", infofromfile.fileName()); /// fileName(); 
        el.setAttribute("doctitle", infofromfile.baseName());
        FULLXMLBODY = wdom.toString(5);
        logconsole(QString("Begin write xml result item..."));
        doc_continue = Tools::_write_file(rootxmlfile, FULLXMLBODY, "utf-8"); /// write xml to convert ..
    }

    QString xslt2_result;
    if (doc_continue && style_file.size() > 0) {
        /// xslt2 extern app
        //// style/odtfullbody.xsl  cache/new.xml -output index.html -param oggiarriva="il bello di sempre"
        QStringList xsltarg;
        xsltarg << style_file;
        xsltarg << rootxmlfile;
        xsltarg << "-output";
        xsltarg << indexhtmlfile;
        //// xsltarg << "-param";
        ////xsltarg << QString("TimeXsltConvert=\"%1\"").arg(Tools::TimeNow());
        logconsole(QString("Begin xslt processor item..."));
        /// xslt to bin_xsltproc or qt5 QTXSLT2 
        QStringList argqt5;
        argqt5 << "xmlpatterns";
        const QString qt5_xmlpatterns = execlocal(QString("which"), argqt5).simplified();
        if (qt5_xmlpatterns.size() > 0) {
            logconsole(QString("App xmlpatterns  found on %1 ").arg(qt5_xmlpatterns));
        } else {
            logconsole(QString("Warning! xmlpatterns NOT found alternate is  %1 ").arg(QTXSLT2));
        }

        /// check if exist this tool and preference order of converter xslt 
        if (qt5_xmlpatterns.size() == QTXSLT2.size()) {
            /// on server working .. not on bundle or window ... 
            xslt2_result = this->execlocal(qt5_xmlpatterns, xsltarg);
        } else if (QTXSLT2.size() > 0) {
            xslt2_result = this->execlocal(QTXSLT2, xsltarg);
        } else if (bin_xsltproc.size() > 0) {
            xslt2_result = this->execlocal(bin_xsltproc, xsltarg);
        }
        logconsole(QString("Xslt proc end response...%1...").arg(xslt2_result));
    }

    /// search results from xslt 
    RamBuffer *bhtml = new RamBuffer("htmlresultxslt");
    bhtml->LoadFile(indexhtmlfile);
    FULLHTMLBODY = bhtml->fromUtf8();
    bhtml->clear();
    //// clean line here at place ////
    if (FULLHTMLBODY.indexOf("WrapperPage") > 0) {
        if (is_insert_pic) {
            int summimage = insertimage();
            logconsole(QString("Total Image insert: %1").arg(summimage));
        }
        success_odt = Tools::_write_file(indexhtmlfile, FULLHTMLBODY, "utf-8");
        console(QString("Sucess convert document"), 3);

    } else {
        FULLHTMLBODY = QString("Unable to read this odt file %1 .. ").arg(odtfile);
        console(FULLHTMLBODY, 3);
    }
    clean();
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
        logconsole(QString("Warning Unable to append on OdtZipHandler::base_append_xml!"));
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

    bool cok;
    bool bok;
    logconsole(QString("Begin image handle..."));
    int found = 0;
    QRegExp expression("src=[\"\'](.*)[\"\']", Qt::CaseInsensitive);
    expression.setMinimal(true);
    int iPosition = 0;
    while ((iPosition = expression.indexIn(FULLHTMLBODY, iPosition)) != -1) {

        int greppoint = iPosition;
        const QString imagesrcx = expression.cap(1); /// this must replaced
        int loengh = imagesrcx.length();
        int backsearch = 26;
        if (imagesrcx.startsWith("Pictures/")) {
            /// grab on zip imagesrcx
            QByteArray xdata = docitem(imagesrcx);
            QImage ipic; /// i.value() /// i.key(); 
            ipic.loadFromData(xdata);
            if (xdata.size() > 0 && !ipic.isNull()) {
                int imgWI = ipic.width();
                int imgHI = ipic.height();
                int picWI = 0;
                int picHI = 0;
                bool resize = false;
                //// ok valid image & search width height if is to resize to save kb.
                QString whifind = FULLHTMLBODY.mid(greppoint - backsearch, backsearch); // back 15from src
                /// height is last..
                QStringList piece = whifind.split("|", QString::SkipEmptyParts);
                QStringList numbers = piece.filter(".");

                if (numbers.size() == 2) {
                    picWI = numbers.at(0).toFloat(&cok);
                    picHI = numbers.at(1).toFloat(&bok);
                    if (cok && bok && picWI > 0 && picHI > 0) {
                        // logconsole(QString("Largo (%1) ").arg(picWI));
                        //logconsole(QString("alto (%1) ").arg(picHI));
                        if (imgWI != picWI) {
                            resize = true;
                        }
                    }
                }
                found++;
                QFileInfo pic(imagesrcx);
                //// tif image not work on browser!!!
                QString blob;
                const QString ext = pic.completeSuffix().toLower();
                if (ext == "tif" || ext == "tiff" || resize) {
                    if (resize) {
                        logconsole(QString("Image resize (%1x%2) ").arg(picWI).arg(picHI));
                        xdata = this->convertPNG(ipic, picWI, picHI);
                    } else {
                        xdata = this->convertPNG(ipic);
                    }
                    blob = "data:image/png;base64,";
                } else {
                    blob = "data:image/" + pic.completeSuffix().toLower() + ";base64,";
                }
                QString imagembed(xdata.toBase64().constData());
                blob.append(imagembed); /// format_string76(blob) 
                const QString imagehtml = blob; //// Tools::f_string76(blob); /// image scroll...
                FULLHTMLBODY.replace(imagesrcx, imagehtml);
            }
        }

        iPosition += expression.matchedLength();
    }

    logconsole(QString("End image handle..."));
    /////QStringList	split ( const QChar & sep, SplitBehavior behavior = KeepEmptyParts, Qt::CaseSensitivity cs = Qt::CaseSensitive ) const
    QString htmltmp;
    QStringList lines = FULLHTMLBODY.split(QRegExp("(\\r\\n)|(\\n\\r)|\\n"), QString::SkipEmptyParts);
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines.at(i);
        QString lc = line.simplified();
        if (!lc.isEmpty()) {
            htmltmp.append(line);
            htmltmp.append("\n");
        }
    }
    FULLHTMLBODY = htmltmp;
    //// QString::fromUtf8(stream()); 
    return found;
}

bool OdtZipHandler::findobject() {

    QByteArray xbase = docitem("content.xml");
    QString sxmlbody = QString::fromUtf8(xbase); /// base contenent 
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
            /// ./Object 1 / ./Object 2 calc inside odt text
            /// like paste a excel row inside doc... 
            QString base = name.mid(2, name.length() - 2);
            QString bodyxmlo = base + QString("/content.xml");
            QString bodystile = base + QString("/styles.xml");
            QByteArray sxa = docitem(bodyxmlo);
            QByteArray sostile = docitem(bodystile);
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
    if (foundobj > 0) {
        XMLTMP = sxmlbody;
    }
    return (foundobj > 0) ? true : false;
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

QByteArray OdtZipHandler::docitem(const QString archive) {
    QMapIterator<QString, QByteArray> i(xmlfilelist);
    while (i.hasNext()) {
        i.next();
        const QString internname = QString(i.key());
        if (archive == internname) {
            return i.value();
        }
    }
    setError(QString("Not found name %1 on zip stream!").arg(archive));

    return QByteArray();
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

void Xsl_Include::translate_fo(QString & name, QString & value, QDomElement appender) {
    //// text-underline-style
    if (name == "text-underline-style" && value != "none") {
        name = QString("text-decoration");
        value = QString("underline");
    }
    //// horizontal-pos="center" 
    if (name == "margin") {
        name = QString("odtmargin");
    }
    if (name == "horizontal-pos" && value == "center") {
        name = QString("text-align");
        appender.setAttribute("margin-left", "auto");
        appender.setAttribute("margin-right", "auto");
    }
    if (name == "text-line-through-style" && value != "none") {
        name = QString("text-decoration");
        value = QString("line-through");
    }
    if (name == "text-overline-style" && value != "none") {
        name = QString("text-decoration");
        value = QString("overline");
    }
    if (name == "background-transparency") {
        name = QString("opacity");
        ///  / 100 if > 1 
        QString newone = value.left( value.size() -1 );
        bool ok;
        qreal nnac = newone.toFloat(&ok);
        if (ok && nnac < 99 && nnac > 1) {
            qreal write = (100 - nnac) / 100;
            value = QString::number(write);
        }
        
    }
    if (name == "font-name") {
        name = QString("font-family");
    }

    if (name == "width" | name == "x" || name == "y" || name == "height") {


        const qreal xpixel = unitfromodt(value);
        const QString valuenumberin = QString::number(xpixel);

        if (name == "width") {
            appender.setAttribute("wipx", valuenumberin);
        }
        if (name == "height") {
            appender.setAttribute("hipx", valuenumberin);
        }
        if (name == "x") {
            appender.setAttribute("xpx", valuenumberin);
        }
        if (name == "y") {
            appender.setAttribute("ypx", valuenumberin);
        }

    }

    if (name == "border-bottom" || name == "border-right" || name == "border-top" || name == "border-left") {
        /// or zero or minimum 1 px check 
        if (value != "none") {
            QStringList parts = value.split(" ");
            if (parts.size() == 3) {
                QString valex = parts.at(0).simplified();
                const qreal mapixel = unitfromodt(valex);
                if (mapixel > 0 && mapixel < 1) {
                    const QString setval = QString("1px %1 %2").arg(parts.at(1)).arg(parts.at(2));
                    value = setval;
                }
            }
        }

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
            translate_fo(one, value, appender);
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
