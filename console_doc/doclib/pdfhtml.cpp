/* 
 * File:   pdfhtml.cpp
 * Author: pro
 * Struzzo bello in fare le cose
 * Created on 14. November 2013, 18:49
 */

#include "pdfhtml.h"

PortableDocument::PortableDocument(const QString docfilename) : Document("PDF"), is_loading(false) {
    bool debugmodus = false;
    bool xcontinue = false;
    QFileInfo infofromfile(docfilename);

    console(QString("Init talking this class. %1..").arg(this->name()), 0);
    bool doc_continue = true;
    console(QString("OpenFile: %1").arg(docfilename));
    this->SetStyle(__localPDFSTYLE__);
    const QString rootxmlfile = QString("%1index.xml").arg(cache_dir);
    //// const QString rootxmlfileback = QString("%1index_back.xml").arg(cache_dir);
    const QString indexhtmlfile = QString("%1index.html").arg(cache_dir);
    QFile::remove(indexhtmlfile);
    QFile::remove(rootxmlfile);
    const QString indexaa = QString("%1aa.html").arg(cache_dir);
    /// first action generate bg image if need 
    /// pdftohtml -c -noframes LaStampadiFerrovia.pdf /Users/pro/.icache/aa.html
    if (!bin_pdftohtml.isEmpty() && !bin_xsltproc.isEmpty()) {
        console(QString("Init pdf A convert...%1.").arg(bin_pdftohtml));
        QStringList apdtparam;
        apdtparam << "-c";
        apdtparam << "-noframes";
        apdtparam << docfilename;
        apdtparam << indexaa;
        const QString aerrorcmdpdf = execlocal(bin_pdftohtml, apdtparam).simplified();
        console(QString("Pdf convert... say: %1").arg(aerrorcmdpdf));
        /// remove file html indexaa not like this.. take only image..
        QFile::remove(indexaa);

    }


    /// pdftohtml -xml 22.pdf /Users/pro/.icache/index.xml
    if (!bin_pdftohtml.isEmpty() && !bin_xsltproc.isEmpty()) {
        console(QString("Init pdf B convert...%1.").arg(bin_pdftohtml));
        QStringList pdtparam;
        pdtparam << "-xml";
        pdtparam << docfilename;
        pdtparam << rootxmlfile;
        const QString errorcmdpdf = execlocal(bin_pdftohtml, pdtparam).simplified();
        console(QString("Pdf convert... say: %1").arg(errorcmdpdf));
        if (errorcmdpdf.isEmpty()) {
            /// bin_pdftohtml talk page on success
        } else {

            xcontinue = true; /// 
            /// rewrite xml file append param here
            this->readfile(rootxmlfile);
            QDomDocument doc = this->xmltoken();
            QDomElement el = doc.documentElement();
            el.setAttribute("dateconvert", Tools::TimeNow());
            el.setAttribute("docname", infofromfile.fileName()); /// fileName(); 
            el.setAttribute("doctitle", infofromfile.fileName());
            QString xmlstream = doc.toString(5);
            Tools::_write_file(rootxmlfile, xmlstream, "utf-8");
            this->swap();
        }
    }
    ScannerImage(); /// read Image on cache & delete...
    /// index.xml is on cache & image on qmap pdfimagelist. image is deleted ok
    /// now xslt convert and insert image on src="name"
    console(QString("Init xslt convert...%1.").arg(bin_xsltproc));
    QStringList xsltparam;
    xsltparam << "--param"; /// TitleDocument 
    xsltparam << "Convert_Time";
    xsltparam << QString("'%1'").arg(Tools::TimeNow());
    xsltparam << "--output";
    xsltparam << indexhtmlfile;
    xsltparam << style_file;
    xsltparam << rootxmlfile;
    /// xslt param --param data_path2 "'e:/tmp/stu/xx'"
    const QString errorcmd = execlocal(bin_xsltproc, xsltparam);
    if (errorcmd.isEmpty()) {
        xcontinue = true;
        console(QString("Xslt convert successful document..."));
    } else {
        xcontinue = false;
        console(QString("Xslt convert... say: %1").arg(errorcmd));
    }
    QFile::remove(rootxmlfile);
    QFile::remove(style_file);
    QFile::remove(__localDTPDT__);
    this->swap(); /// clear buffer 
    this->readfile(indexhtmlfile);
    /// read indexhtmlfile 
    HTMLSTREAM = this->htmlstream();
    if (HTMLSTREAM.size() > 0) {
        xcontinue = true;
    }
    ScanImageHtml();
    this->swap(); /// clear buffer 
    is_loading = Tools::_write_file(indexhtmlfile, HTMLSTREAM, "utf-8");
    if (is_loading) {
        console(QString("Writteln image to: %1").arg(indexhtmlfile),3);
    } else {
        console(QString("Warning! unable to write on: %1").arg(indexhtmlfile),3);
    }
    this->readfile(indexhtmlfile);
    if (HTMLSTREAM.size() > 0) {
        xcontinue = true;
        is_loading = true;
    } else {
        is_loading = false;
    }

    /// 

    clean();

}

QByteArray PortableDocument::docitem(const QString archive) {
    QMapIterator<QString, QByteArray> i(pdfimagelist);
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

void PortableDocument::ScannerImage() {

    //// console(QString("Scan for image on dir: %1").arg(cache_dir));
    /// load all image to pdfimagelist from cache_dir and delete file.. 
    QDir cachedir(cache_dir);
    if (cachedir.exists(cache_dir)) {
        console(QString("Scan for image on dir: %1").arg(cache_dir));

        Q_FOREACH(QFileInfo info, cachedir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (!info.isDir()) {
                /// info.absoluteFilePath()
                QByteArray xdata = this->loadbinfile(info.absoluteFilePath());
                const QString namepic = info.fileName();
                //// console(QString("Read file: %1").arg(namepic));
                QImage none;
                QImage pic;
                pic.loadFromData(xdata);
                if (!pic.isNull()) {
                    console(QString("Insert image: %1 as name %2").arg(namepic).arg(info.absoluteFilePath()));
                    pdfimagelist.insert(info.absoluteFilePath(), xdata);
                    bool deletet = QFile::remove(info.absoluteFilePath());
                    pic = none;
                    /// check deletet?? 
                }
            }
        }

    }
}

void PortableDocument::ScanImageHtml() {

    QString html = HTMLSTREAM;
    HTMLSTREAM.clear();
    QRegExp expression("src=[\"\'](.*)[\"\']", Qt::CaseInsensitive);
    expression.setMinimal(true);
    int iPosition = 0;
    while ((iPosition = expression.indexIn(html, iPosition)) != -1) {
        const QString imagesrcx = expression.cap(1); /// this must replaced
        int loengh = imagesrcx.length();
        int repos = imagesrcx.indexOf("=");
        /// KZIPDEBUG() << "HTML image found Docx:" << imagesrcx;
        if (repos > 0) {
            const QString key = imagesrcx.mid(repos + 1, loengh - (repos - 1)).simplified();
            /// KZIPDEBUG() << "HTML Translate pic :" << key;
            QByteArray xdata = docitem(key);
            QImage pic;
            pic.loadFromData(xdata);
            if (!pic.isNull()) {
                /// KZIPDEBUG() << "HTML Translate pic valid :" << key;
                const QString imagehtml = EmbeddedImage(key);
                if (!imagehtml.isEmpty()) {
                    html.replace(imagesrcx, imagehtml);
                }

            }
        }
        iPosition += expression.matchedLength();
    }
    const int sumpage = 33;

    for (int i = 0; i < sumpage; i++) {
        const QString searchstr = QString("<!--|Page_|%1|").arg(i);
        int lenghpot = searchstr.length() + 12;
        int pospoint = html.indexOf(searchstr);
        if (pospoint > 0) {
            QString capture = html.mid(pospoint, lenghpot);
            QStringList args = capture.split("|", QString::SkipEmptyParts);
            /// interessa 2 + 3 
            if (args.size() > 4) {
                QString pwidth = args.at(3);
                QString pheight = args.at(4);
                QVariant nr(i);
                QByteArray anr(nr.toByteArray());
                QByteArray Acmd("aa");
                Acmd.append(anr.rightJustified(3, '0'));
                Acmd.append(".png");
                QString fileimagebg = QString(Acmd.simplified().constData());
                fileimagebg.prepend(cache_dir);
                const QString simagehtml = EmbeddedImage(fileimagebg);
                if (!simagehtml.isEmpty()) {
                    QString insertfragment(QString("<img width=\"%1\" height=\"%2\" src=\"%3\" data=\"|BgImagePagebyqt|%1|%2|\" />").arg(pwidth).arg(pheight).arg(simagehtml));
                    html.insert(pospoint - 1, insertfragment);
                }
                //// KZIPDEBUG() << "insert imagex :" << fileimagebg;
            }
        }
    }
    html.replace("&#10; ", "");
    HTMLSTREAM = html.simplified();
    html.clear();
}

QString PortableDocument::EmbeddedImage(const QString key) {

    QString result;
    QByteArray xdata = docitem(key);
    QImage pic;
    pic.loadFromData(xdata);
    if (!pic.isNull()) {
        QFileInfo pic(key);
        QString blob = "data:image/" + pic.completeSuffix().toLower() + ";base64,";
        QString imagembed(xdata.toBase64().constData());
        blob.append(imagembed);
        result = blob;
    }
    return result;
}

PortableDocument::~PortableDocument() {

}

