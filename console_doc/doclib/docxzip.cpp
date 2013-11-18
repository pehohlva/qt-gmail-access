/* 
 * File:   docxzip.cpp
 * Author: pro
 * Struzzo bello in fare le cose
 * Created on 12. November 2013, 22:54
 */

#include <QStringList>
#include <QDomDocument>
#include "docxzip.h"
#include <QImage>

QString DocxZipHandler::html() {
    return HTMLSTREAM; /// buffer html 
}

DocxZipHandler::DocxZipHandler(const QString docfilename) : Document("DOCX"), is_loading(false) {

    QFileInfo infofromfile(docfilename);
    bool debugmodus = (DEBUGMODUS == 1)? true : false;
    console(QString("Init talking this class. %1..").arg(this->name()), 0);
    bool doc_continue = true;
    console(QString("OpenFile: %1").arg(docfilename));
    this->SetBaseFileDoc(docfilename);
    this->SetStyle(__localDOCXSTYLE__);
    KZip::Stream *unzip = new KZip::Stream(docfilename);
    if (debugmodus) {
        unzip->explode_todir(cache_dir, 1);
    }
    if (unzip->canread() && docx_validator(unzip->filelist())) {
        console(QString("Zip is open OK doc valid ...."));
        corefile = unzip->listData();
        unzip->~Stream();
        unzip = NULL;
        QByteArray xmldataword = docitem("word/document.xml");
        const QString rootxmlfile = QString("%1index.xml").arg(cache_dir);
        const QString indexhtmlfile = QString("%1index.html").arg(cache_dir);
        QFile::remove(rootxmlfile);
        QFile::remove(indexhtmlfile);
        if (xmldataword.size() > 0) {
            doc_continue = Tools::_write_file(rootxmlfile, xmldataword);
        }
        ///// contibue ok...
        if (doc_continue && !bin_xsltproc.isEmpty()) {
            /// convert xslt now
            console(QString("Init xslt convert...%1.").arg(bin_xsltproc));
            QStringList xsltparam;
            xsltparam << "--param";
            xsltparam << "convert_time";
            xsltparam << QString("'%1'").arg(Tools::TimeNow());
            xsltparam << "--output";
            xsltparam << indexhtmlfile;
            xsltparam << style_file;
            xsltparam << rootxmlfile;
            /// xslt param --param data_path2 "'e:/tmp/stu/xx'"
            const QString errorcmd = execlocal(bin_xsltproc, xsltparam);
            if (errorcmd.isEmpty()) {
                console(QString("Xslt convert successful document..."));
            } else {
                console(QString("Xslt convert... say: %1").arg(errorcmd));
            }
        }

        QByteArray prehtml = this->loadbinfile(indexhtmlfile);
        HTMLSTREAM = QString::fromUtf8(prehtml.constData());
        if (doc_continue && prehtml.size() > 0) {
            scanimage(); /// insert image on html embedet
            is_loading = Tools::_write_file(indexhtmlfile, HTMLSTREAM, "utf-8");
            console(QString("Write last file: %1").arg(indexhtmlfile));
        }
        if (!doc_continue) {
            is_loading = false;
            console(QString("Error on Buffer...."), 3);
        }

    } else {
        is_loading = false;
        console(QString("Document bad format "), 3);
    }
    clean();
}

QByteArray DocxZipHandler::docitem(const QString archive) {
    QMapIterator<QString, QByteArray> i(corefile);
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

void DocxZipHandler::scanimage() {


    QString html = HTMLSTREAM;
    HTMLSTREAM.clear();
    QByteArray docindex = docitem("word/_rels/document.xml.rels");
    QRegExp expression("src=[\"\'](.*)[\"\']", Qt::CaseInsensitive);
    expression.setMinimal(true);
    int iPosition = 0;
    while ((iPosition = expression.indexIn(html, iPosition)) != -1) {
        const QString imagesrcx = expression.cap(1); /// this must replaced
        int loengh = imagesrcx.length();
        int repos = imagesrcx.indexOf("=");
        /// nul pixel __ONEPIXELGIF__
        BASICDOCDEBUG() << "HTML image found Docx:" << imagesrcx;
        if (repos > 0) {
            const QString key = imagesrcx.mid(repos + 1, loengh - (repos - 1)).simplified();
            QString imagechunker = read_docx_index(docindex, 1, key);
            if (!imagechunker.isEmpty()) {
                html.replace(imagesrcx, imagechunker);
                //// BASICDOCDEBUG() << "HTML image found Docx:" << imagesrcx;
                //// BASICDOCDEBUG() << "HTML image found Docx:" << key;
            } else {
                BASICDOCDEBUG() << "HTML image insert null pixel :" << imagesrcx;
               html.replace(imagesrcx,__ONEPIXELGIF__); 
            }
        } else {
           html.replace(imagesrcx,__ONEPIXELGIF__); 
        }
        iPosition += expression.matchedLength();
    }

    ////QTextDocument *doc = new QTextDocument();
    ///doc->setHtml(html);
    /////html.clear(); doc->toHtml("utf-8"); 
    html.replace("&#10; ", "");
    HTMLSTREAM = html.simplified();
    html.clear();
}

QString DocxZipHandler::read_docx_index(QByteArray xml, int findertype, const QString Xid) {
    QXmlSimpleReader reader;
    QXmlInputSource source;
    source.setData(xml);
    QString errorMsg;
    QDomDocument document;
    if (!document.setContent(&source, &reader, &errorMsg)) {
        setError(QString("Invalid XML document: %1").arg(errorMsg));
        return QString();
    }
    /* TargetMode="External"/> is link http to image */
    const QDomElement documentElement = document.documentElement();
    QDomElement element = documentElement.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QLatin1String("Relationship")) {
            const QString target = element.attribute("Target", "NULL");
            QString pathimage = "word/" + target; /// if 1??findertype
            const QString uuid = element.attribute("Id", "NULL");
            const QString type = element.attribute("Type", "NULL");
            ////BASICDOCDEBUG() << "image:" << target << " Id:" << uuid;
            //// check image == 1 findertype if need other?? 
            if (target.startsWith("media") && type.endsWith("image") &&
                    findertype == 1 && uuid == Xid) {
                QByteArray xdata = docitem(pathimage);
                QImage pic;
                pic.loadFromData(xdata);
                if (!pic.isNull()) {
                    QFileInfo pic(pathimage);
                    QString blob = "data:image/" + pic.completeSuffix().toLower() + ";base64,";
                    QString imagembed(xdata.toBase64().constData());
                    blob.append(imagembed);
                    const QString imagehtml = blob; /// image scroll...
                    /// search <img src="IDFROM_RELS=rId9" width="295px ....  && insert chunk lol
                    return imagehtml;
                    ////BASICDOCDEBUG() << "image:" << pathimage;
                    //// BASICDOCDEBUG() << "image blob size:" << blob.size();
                }
            }
        }
        element = element.nextSiblingElement();
    }

    ///// BASICDOCDEBUG() << "index docx xml:" << document.toString(5);

    return QString();
}

bool DocxZipHandler::docx_validator(const QStringList entries) {

    //// /Users/pro/code/minisvn/doc/Xsltqt5/cache/65ae8b517da59b9dc7a6bd03316cda9a/word/_rels/document.xml.rels
    if (!entries.contains("word/_rels/document.xml.rels")) {
        /// the most important to discovery image link and all item parts
        setError(i18n("Invalid document structure (word/_rels/document.xml.rels file is missing)"));
        return false;
    }

    if (!entries.contains("settings.xml")) {
        setError(i18n("Invalid document structure (settings.xml file is missing)"));
        /// return false;
    }
    if (!entries.contains("word")) {
        setError(i18n("Invalid document structure (word directory is missing)"));
        return false;
    }
    /// for doc only txt media not exist lol
    /// if (!entries.contains("media")) {
    //// setError(i18n("Invalid document structure (media directory is missing)"));
    /// return false;
    //// }
    if (!entries.contains("theme")) {
        setError(i18n("Invalid document structure (theme directory is missing)"));
        return false;
    }
    if (!entries.contains("styles.xml")) {
        setError(i18n("Invalid document structure (styles.xml file is missing)"));
        //// return false;
    }
    if (!entries.contains("webSettings.xml")) {
        setError(i18n("Invalid document structure (webSettings.xml file is missing)"));
        //// return false;
    }
    if (!entries.contains("[Content_Types].xml")) {
        setError(i18n("Invalid document structure ([Content_Types].xml file is missing)"));
        return false;
    }
    return true;
}

DocxZipHandler::~DocxZipHandler() {
}

