//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#ifndef MIME_STANDARD_H
#define	MIME_STANDARD_H

#include <QHash>
#include <QString>
#include <QBuffer>
#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <iostream>
#include <QtCore>
#include <QCoreApplication>
#include <QMap>
#include <QList>
#include <QStringList>
#include <QObject>
#include <QDateTime>
#include <QDate>
#include <QSettings>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <QIODevice>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDir>
#include <QFileInfo>
#include <QMetaType>


class Qmailf;

/// 6 okt. 2013 save on svn 
/// great this to push class on qvariant

template <class T> class VPtr {
public:

    static T* asPtr(QVariant v) {
        return (T *) v.value<void *>();
    }

    static QVariant asQVariant(T* ptr) {
        return qVariantFromValue((void *) ptr);
    }
};

/*
 http://blog.bigpixel.ro/2010/04/storing-pointer-in-qvariant/
 MyClass *p;
QVariant v = VPtr<MyClass>::asQVariant(p);

MyClass *p1 = VPtr<MyClass>::asPtr(v);
 *  MMime::MimeTypes help;
 */


namespace MMime {

    /// full list http://www.iana.org/assignments/media-types
    /// better job to make on sqlite or on xml format on net ..
    /// and sort by prioryti of usage 
    /// not to having by search text/plain become diff 
    /// all opendocumet format not inside here 
    class MimeTypes {
    public:

        MimeTypes() {
            mimeTypes.insert("txt", "text/plain");
            mimeTypes.insert("html", "text/html");
            mimeTypes.insert("323", "text/h323");
            mimeTypes.insert("ai", "application/postscript");
            mimeTypes.insert("aif", "audio/x-aiff");
            mimeTypes.insert("aifc", "audio/x-aiff");
            mimeTypes.insert("aiff", "audio/x-aiff");
            mimeTypes.insert("~", "application/x-trash");
            mimeTypes.insert("%", "application/x-trash");
            mimeTypes.insert("arj", "application/arj");
            mimeTypes.insert("art", "image/x-jg");
            mimeTypes.insert("asc", "text/plain");
            mimeTypes.insert("asf", "video/x-ms-asf");
            mimeTypes.insert("asx", "video/x-ms-asf");
            mimeTypes.insert("au", "audio/basic");
            mimeTypes.insert("avi", "video/msvideo");
            mimeTypes.insert("bak", "application/x-trash");
            mimeTypes.insert("bat", "application/x-msdos-program");
            mimeTypes.insert("bcpio", "application/x-bcpio");
            mimeTypes.insert("bin", "application/octet-stream");
            mimeTypes.insert("bmp", "image/bmp");
            mimeTypes.insert("book", "application/x-maker");
            mimeTypes.insert("bz2", "application/x-bzip2");
            mimeTypes.insert("cat", "application/vnd.ms-pki.seccat");
            mimeTypes.insert("CCAD", "application/clariscad");
            mimeTypes.insert("cc", "text/x-c++src");
            mimeTypes.insert("cdf", "application/x-cdf");
            mimeTypes.insert("cdr", "image/x-coreldraw");
            mimeTypes.insert("cdt", "image/x-coreldrawtemplate");
            mimeTypes.insert("cdy", "application/vnd.cinderella");
            mimeTypes.insert("chrt", "application/x-kchart");
            mimeTypes.insert("class", "application/x-java-vm");
            mimeTypes.insert("cls", "text/x-tex");
            mimeTypes.insert("com", "application/x-msdos-program");
            mimeTypes.insert("cpio", "application/x-cpio");
            mimeTypes.insert("cpp", "text/x-c++src");
            mimeTypes.insert("cpt", "image/x-corelphotopaint");
            mimeTypes.insert("crl", "application/x-pkcs7-crl");
            mimeTypes.insert("crt", "application/x-x509-ca-cert");
            mimeTypes.insert("csh", "text/x-csh");
            mimeTypes.insert("csm", "application/cu-seeme");
            mimeTypes.insert("css", "text/css");
            mimeTypes.insert("csv", "text/comma-separated-values");
            mimeTypes.insert("C", "text/x-csrc");
            mimeTypes.insert("c", "text/x-csrc");
            mimeTypes.insert("c++", "text/x-c++src");
            mimeTypes.insert("cu", "application/cu-seeme");
            mimeTypes.insert("cxx", "text/x-c++src");
            mimeTypes.insert("dcr", "application/x-director");
            mimeTypes.insert("deb", "application/x-debian-package");
            mimeTypes.insert("diff", "text/plain");
            mimeTypes.insert("dif", "video/x-dv");
            mimeTypes.insert("dir", "application/x-director");
            mimeTypes.insert("djv", "image/x-djvu");
            mimeTypes.insert("djvu", "image/x-djvu");
            mimeTypes.insert("dll", "application/x-msdos-program");
            mimeTypes.insert("dl", "video/dl");
            mimeTypes.insert("dms", "application/x-dms");
            mimeTypes.insert("doc", "application/msword");
            mimeTypes.insert("dot", "application/msword");
            mimeTypes.insert("DRW", "application/drafting");
            mimeTypes.insert("dvi", "application/x-dvi");
            mimeTypes.insert("dv", "video/x-dv");
            mimeTypes.insert("dwg", "application/acad");
            mimeTypes.insert("DWG", "application/acad");
            mimeTypes.insert("dxf", "application/dxf");
            mimeTypes.insert("DXF", "application/dxf");
            mimeTypes.insert("dxr", "application/x-director");
            mimeTypes.insert("eps", "application/postscript");
            mimeTypes.insert("etx", "text/x-setext");
            mimeTypes.insert("exe", "application/x-msdos-program");
            mimeTypes.insert("ez", "application/andrew-inset");
            mimeTypes.insert("f90", "text/plain");
            mimeTypes.insert("fb", "application/x-maker");
            mimeTypes.insert("fbdoc", "application/x-maker");
            mimeTypes.insert("fig", "application/x-xfig");
            mimeTypes.insert("fli", "video/fli");
            mimeTypes.insert("fm", "application/x-maker");
            mimeTypes.insert("frame", "application/x-maker");
            mimeTypes.insert("frm", "application/x-maker");
            mimeTypes.insert("gcf", "application/x-graphing-calculator");
            mimeTypes.insert("gf", "application/x-tex-gf");
            mimeTypes.insert("gif", "image/gif");
            mimeTypes.insert("gl", "video/gl");
            mimeTypes.insert("gnumeric", "application/x-gnumeric");
            mimeTypes.insert("gsf", "application/x-font");
            mimeTypes.insert("gsm", "audio/x-gsm");
            mimeTypes.insert("gtar", "application/x-gtar");
            mimeTypes.insert("g", "text/plain");
            mimeTypes.insert("gz", "application/x-gzip");
            mimeTypes.insert("gzip", "application/x-gzip");
            mimeTypes.insert("hdf", "application/x-hdf");
            mimeTypes.insert("hh", "text/x-c++hdr");
            mimeTypes.insert("hlp", "application/x-winhelp");
            mimeTypes.insert("hpp", "text/x-c++hdr");
            mimeTypes.insert("hqx", "application/mac-binhex40");
            mimeTypes.insert("hta", "application/hta");
            mimeTypes.insert("h", "text/x-chdr");
            mimeTypes.insert("h++", "text/x-c++hdr");
            mimeTypes.insert("htm", "text/html");
            mimeTypes.insert("hxx", "text/x-c++hdr");
            mimeTypes.insert("ica", "application/x-ica");
            mimeTypes.insert("ice", "x-conference/x-cooltalk");
            mimeTypes.insert("ief", "image/ief");
            mimeTypes.insert("iges", "application/iges");
            mimeTypes.insert("IGES", "application/iges");
            mimeTypes.insert("igs", "application/iges");
            mimeTypes.insert("IGS", "application/iges");
            mimeTypes.insert("iii", "application/x-iphone");
            mimeTypes.insert("ins", "application/x-internet-signup");
            mimeTypes.insert("isp", "application/x-internet-signup");
            mimeTypes.insert("jar", "application/x-java-archive");
            mimeTypes.insert("java", "text/x-java");
            mimeTypes.insert("jng", "image/x-jng");
            mimeTypes.insert("jnlp", "application/x-java-jnlp-file");
            mimeTypes.insert("jpeg", "image/jpeg");
            mimeTypes.insert("JPEG", "image/jpeg");
            mimeTypes.insert("jpe", "image/jpeg");
            mimeTypes.insert("JPE", "image/jpeg");
            mimeTypes.insert("jpg", "image/jpeg");
            mimeTypes.insert("JPG", "image/jpeg");
            mimeTypes.insert("js", "application/x-javascript");
            mimeTypes.insert("kar", "audio/midi");
            mimeTypes.insert("kil", "application/x-killustrator");
            mimeTypes.insert("klnk", "application/x-kdelnk");
            mimeTypes.insert("kpr", "application/x-kpresenter");
            mimeTypes.insert("kpt", "application/x-kpresenter");
            mimeTypes.insert("ksp", "application/x-kspread");
            mimeTypes.insert("kwd", "application/x-kword");
            mimeTypes.insert("kwt", "application/x-kword");
            mimeTypes.insert("latex", "application/x-latex");
            mimeTypes.insert("lha", "application/x-lha");
            mimeTypes.insert("lsf", "video/x-la-asf");
            mimeTypes.insert("lsx", "video/x-la-asf");
            mimeTypes.insert("ltx", "text/x-tex");
            mimeTypes.insert("lzh", "application/x-lzh");
            mimeTypes.insert("lzx", "application/x-lzx");
            mimeTypes.insert("m3u", "audio/mpegurl");
            mimeTypes.insert("maker", "application/x-maker");
            mimeTypes.insert("man", "application/x-troff-man");
            mimeTypes.insert("mdb", "application/msaccess");
            mimeTypes.insert("me", "application/x-troff-me");
            mimeTypes.insert("mesh", "model/mesh");
            mimeTypes.insert("mid", "audio/midi");
            mimeTypes.insert("midi", "audio/midi");
            mimeTypes.insert("mif", "application/x-mif");
            mimeTypes.insert("mk", "text/x-makefile");
            mimeTypes.insert("mml", "text/mathml");
            mimeTypes.insert("mng", "video/x-mng");
            mimeTypes.insert("moc", "text/x-moc");
            mimeTypes.insert("movie", "video/x-sgi-movie");
            mimeTypes.insert("mov", "video/quicktime");
            mimeTypes.insert("mp2", "audio/mpeg");
            mimeTypes.insert("mp3", "audio/mpeg");
            mimeTypes.insert("mpega", "audio/mpeg");
            mimeTypes.insert("mpeg", "video/mpeg");
            mimeTypes.insert("MPEG", "video/mpeg");
            mimeTypes.insert("mpe", "video/mpeg");
            mimeTypes.insert("MPE", "video/mpeg");
            mimeTypes.insert("mpga", "audio/mpeg");
            mimeTypes.insert("mpg", "video/mpeg");
            mimeTypes.insert("MPG", "video/mpeg");
            mimeTypes.insert("ms", "application/x-troff-ms");
            mimeTypes.insert("msh", "model/mesh");
            mimeTypes.insert("msi", "application/x-msi");
            mimeTypes.insert("m", "text/plain");
            mimeTypes.insert("mxu", "video/vnd.mpegurl");
            mimeTypes.insert("nb", "application/mathematica");
            mimeTypes.insert("nc", "application/x-netcdf");
            mimeTypes.insert("o", "application/x-object");
            mimeTypes.insert("oda", "application/oda");
            mimeTypes.insert("ogg", "application/ogg");
            mimeTypes.insert("old", "application/x-trash");
            mimeTypes.insert("oza", "application/x-oz-application");
            mimeTypes.insert("p7r", "application/x-pkcs7-certreqresp");
            mimeTypes.insert("pac", "application/x-ns-proxy-autoconfig");
            mimeTypes.insert("part", "application/pro_eng");
            mimeTypes.insert("pas", "text/x-pascal");
            mimeTypes.insert("pat", "image/x-coreldrawpattern");
            mimeTypes.insert("pbm", "image/x-portable-bitmap");
            mimeTypes.insert("pcf", "application/x-font");
            mimeTypes.insert("pcf.Z", "application/x-font");
            mimeTypes.insert("pcx", "image/pcx");
            mimeTypes.insert("pdb", "chemical/x-pdb");
            mimeTypes.insert("pdf", "application/pdf");
            mimeTypes.insert("pfa", "application/x-font");
            mimeTypes.insert("pfb", "application/x-font");
            mimeTypes.insert("pgm", "image/x-portable-graymap");
            mimeTypes.insert("pgn", "application/x-chess-pgn");
            mimeTypes.insert("pgp", "application/pgp-signature");
            mimeTypes.insert("php3", "application/x-httpd-php3");
            mimeTypes.insert("php3p", "application/x-httpd-php3-preprocessed");
            mimeTypes.insert("php4", "application/x-httpd-php4");
            mimeTypes.insert("php", "application/x-httpd-php");
            mimeTypes.insert("phps", "application/x-httpd-php-source");
            mimeTypes.insert("pht", "application/x-httpd-php");
            mimeTypes.insert("phtml", "application/x-httpd-php");
            mimeTypes.insert("pict", "image/pict");
            mimeTypes.insert("pk", "application/x-tex-pk");
            mimeTypes.insert("pl", "application/x-perl");
            mimeTypes.insert("pls", "audio/x-scpls");
            mimeTypes.insert("pm", "application/x-perl");
            mimeTypes.insert("png", "image/png");
            mimeTypes.insert("pnm", "image/x-portable-anymap");
            mimeTypes.insert("pot", "application/vnd.ms-powerpoint");
            mimeTypes.insert("ppm", "image/x-portable-pixmap");
            mimeTypes.insert("pps", "application/vnd.ms-powerpoint");
            mimeTypes.insert("ppt", "application/vnd.ms-powerpoint");
            mimeTypes.insert("prf", "application/pics-rules");
            mimeTypes.insert("prt", "application/pro_eng");
            mimeTypes.insert("PRT", "application/pro_eng");
            mimeTypes.insert("ps", "application/postscript");
            mimeTypes.insert("PS", "application/postscript");
            mimeTypes.insert("psd", "image/x-photoshop");
            mimeTypes.insert("p", "text/x-pascal");
            mimeTypes.insert("qtl", "application/x-quicktimeplayer");
            mimeTypes.insert("qt", "video/quicktime");
            mimeTypes.insert("ra", "audio/x-realaudio");
            mimeTypes.insert("ram", "audio/x-pn-realaudio");
            mimeTypes.insert("ras", "image/x-cmu-raster");
            mimeTypes.insert("rgb", "image/x-rgb");
            mimeTypes.insert("rm", "audio/x-pn-realaudio");
            mimeTypes.insert("roff", "application/x-troff");
            mimeTypes.insert("rpm", "application/x-redhat-package-manager");
            mimeTypes.insert("rtf", "text/rtf");
            mimeTypes.insert("rtx", "text/richtext");
            mimeTypes.insert("sct", "text/scriptlet");
            mimeTypes.insert("sd2", "audio/x-sd2");
            mimeTypes.insert("sda", "application/vnd.stardivision.draw");
            mimeTypes.insert("sdc", "application/vnd.stardivision.calc");
            mimeTypes.insert("sdd", "application/vnd.stardivision.impress");
            mimeTypes.insert("sdp", "application/vnd.stardivision.impress-packed");
            mimeTypes.insert("sds", "application/vnd.stardivision.chart");
            mimeTypes.insert("sdw", "application/vnd.stardivision.writer");
            mimeTypes.insert("ser", "application/x-java-serialized-object");
            mimeTypes.insert("set", "application/set");
            mimeTypes.insert("SET", "application/set");
            mimeTypes.insert("sgl", "application/vnd.stardivision.writer-global");
            mimeTypes.insert("sh", "application/x-sh");
            mimeTypes.insert("shar", "application/x-shar");
            mimeTypes.insert("sid", "audio/prs.sid");
            mimeTypes.insert("sik", "application/x-trash");
            mimeTypes.insert("silo", "model/mesh");
            mimeTypes.insert("sit", "application/x-stuffit");
            mimeTypes.insert("skd", "application/x-koan");
            mimeTypes.insert("skm", "application/x-koan");
            mimeTypes.insert("skp", "application/x-koan");
            mimeTypes.insert("skt", "application/x-koan");
            mimeTypes.insert("smd", "application/vnd.stardivision.mail");
            mimeTypes.insert("smf", "application/vnd.stardivision.math");
            mimeTypes.insert("smi", "application/smil");
            mimeTypes.insert("smil", "application/smil");
            mimeTypes.insert("snd", "audio/basic");
            mimeTypes.insert("SOL", "application/solids");
            mimeTypes.insert("spl", "application/futuresplash");
            mimeTypes.insert("src", "application/x-wais-source");
            mimeTypes.insert("step", "application/STEP");
            mimeTypes.insert("STEP", "application/STEP");
            mimeTypes.insert("stl", "application/sla");
            mimeTypes.insert("STL", "application/sla");
            mimeTypes.insert("stp", "application/STEP");
            mimeTypes.insert("STP", "application/STEP");
            mimeTypes.insert("sty", "text/x-tex");
            mimeTypes.insert("sv4cpio", "application/x-sv4cpio");
            mimeTypes.insert("sv4crc", "application/x-sv4crc");
            mimeTypes.insert("svg", "image/svg+xml");
            mimeTypes.insert("svgz", "image/svg+xml");
            mimeTypes.insert("swf", "application/x-shockwave-flash");
            mimeTypes.insert("swfl", "application/x-shockwave-flash");
            mimeTypes.insert("t", "application/x-troff");
            mimeTypes.insert("tar", "application/x-tar");
            mimeTypes.insert("taz", "application/x-gtar");
            mimeTypes.insert("tcl", "application/x-tcl");
            mimeTypes.insert("tex", "application/x-tex");
            mimeTypes.insert("texi", "application/x-texinfo");
            mimeTypes.insert("texinfo", "application/x-texinfo");
            mimeTypes.insert("text", "text/plain");
            mimeTypes.insert("tgz", "application/x-gtar");
            mimeTypes.insert("tiff", "image/tiff");
            mimeTypes.insert("tif", "image/tiff");
            mimeTypes.insert("tk", "text/x-tcl");
            mimeTypes.insert("tr", "application/x-troff");
            mimeTypes.insert("tsp", "application/dsptype");
            mimeTypes.insert("tsv", "text/tab-separated-values");
            mimeTypes.insert("eml", "message/rfc822");
            
            /// 
            
            mimeTypes.insert("uls", "text/iuls");
            mimeTypes.insert("unv", "application/i-deas");
            mimeTypes.insert("UNV", "application/i-deas");
            mimeTypes.insert("ustar", "application/x-ustar");
            mimeTypes.insert("vcd", "application/x-cdlink");
            mimeTypes.insert("vcf", "text/x-vcard");
            mimeTypes.insert("vcs", "text/x-vcalendar");
            mimeTypes.insert("vda", "application/vda");
            mimeTypes.insert("VDA", "application/vda");
            mimeTypes.insert("vor", "application/vnd.stardivision.writer");
            mimeTypes.insert("vrml", "x-world/x-vrml");
            mimeTypes.insert("vrm", "x-world/x-vrml");
            mimeTypes.insert("w6w", "application/msword");
            mimeTypes.insert("wad", "application/x-doom");
            mimeTypes.insert("wav", "audio/x-wav");
            mimeTypes.insert("wbmp", "image/vnd.wap.wbmp");
            mimeTypes.insert("wbxml", "application/vnd.wap.wbxml");
            mimeTypes.insert("wk", "application/x-123");
            mimeTypes.insert("wmlc", "application/vnd.wap.wmlc");
            mimeTypes.insert("wmlsc", "application/vnd.wap.wmlscriptc");
            mimeTypes.insert("wmls", "text/vnd.wap.wmlscript");
            mimeTypes.insert("wml", "text/vnd.wap.wml");
            mimeTypes.insert("word", "application/msword");
            mimeTypes.insert("wp5", "application/wordperfect5.1");
            mimeTypes.insert("wri", "application/mswrite");
            mimeTypes.insert("wrl", "x-world/x-vrml");
            mimeTypes.insert("wsc", "text/scriptlet");
            mimeTypes.insert("wz", "application/x-wingz");
            mimeTypes.insert("xbm", "image/x-xbitmap");
            mimeTypes.insert("xhtml", "text/html");
            mimeTypes.insert("xl", "application/excel");
            mimeTypes.insert("xlb", "application/vnd.ms-excel");
            mimeTypes.insert("xls", "application/vnd.ms-excel");
            mimeTypes.insert("xml", "text/xml");
            mimeTypes.insert("ui", "text/xml");
            mimeTypes.insert("xpm", "image/x-xpixmap");
            mimeTypes.insert("xsl", "text/xml");
            mimeTypes.insert("xwd", "image/x-xwindowdump");
            mimeTypes.insert("xyz", "chemical/x-xyz");
            mimeTypes.insert("Z", "application/x-compress");
            mimeTypes.insert("zip", "application/zip");
            mimeTypes.insert("notknow", "application/none");
        };

        QString value(QString key) {
            return mimeTypes.value(key);
        };

        QString Contenttype_Resolver(const QString x) const {
            QString found;
            QRegExp ContentT("Content-Type: +(.*)"); /// end on ;!!!
            if (ContentT.indexIn(x.simplified()) != -1) {
                QString start = ContentT.cap(1).simplified();
                QStringList words = start.split(" ", QString::SkipEmptyParts);
                if (words.size() > 0) {
                    found = words.at(0);
                    if (found.lastIndexOf(QChar(';')) != -1) {
                        found = found.mid(0, found.size() - 1);
                    }
                }
            }
            return found;
        }

        QString MimeFromContent(const QString value) {
            QHash<QString, QString>::iterator i;
            for (i = mimeTypes.begin(); i != mimeTypes.end(); ++i) {
                //// cout << i.key() << ": " << i.value() << endl;
                if (value == i.value()) {
                    return i.key();
                }
            }
            return QString();
        }

        void insert(QString key, QString value) {
            mimeTypes.insert(key, value);
        };

    private:
        QHash<QString, QString> mimeTypes;
    };

}

//// on file from Mail any type + txt & html

class Qmailf {
public:
    Qmailf(int id) : chunk(""), mime("txt"), file("file.txt"), ext("txt"), uuid(id) {

    }
    void SetChunk(const QByteArray& x) {
        chunk = x;
        ///// qDebug() << "incomming <" << uuid << ">  ";
    }
    /// set mime automatic 
    /// todo filename resolver from header && id from inline image 
    void SetMeta(const QString x) {
        meta_header = x;
        MMime::MimeTypes help;
        mime = help.Contenttype_Resolver(x);
        ext = help.MimeFromContent(mime);
        QString def(QString("mail_header_%1.%2").arg(uuid).arg(ext));
        file = def;
    }
     /// no make file or db  take the chunk from mail and send at the right place
    void SetFile(const QString dfile) {
        MMime::MimeTypes fmime;
        QFileInfo fi(dfile);
        file = fi.fileName();
        ext = fi.suffix();
        mime = fmime.value(ext);
    }
    QByteArray Chunk() {
        return chunk;
    }
    QString Mime() {
        return mime;
    }
    int Uid() {
        return uuid;
    }
    QString Filename() {
        return file;
    }
    void Info() {
        QByteArray text = QByteArray::fromBase64(chunk);
        qDebug() << "Content-Type:" << Mime() << " - " <<   Filename();
        qDebug() << "Meta:" << meta_header;
        qDebug() << "Body first 76 chunk:" << text.mid(0,76).simplified();
        text.clear();
    }
private:
    QByteArray chunk; // base64 encoded or other meta_heade having info
    QString mime;
    QString meta_header;  /// meta info  to create a file or o drupal node 
    QString file;
    QString ext;
    int uuid;
};

typedef QMap<int, QVariant> Attach_File_Map;
//// http://www.qtcentre.org/wiki/index.php?title=Using_custom_data_types_with_Qt

/// from this can create 
/// mail dir format or other 
/// i need to convert on drupal node and send on cms 

struct ICmail {
    quint64 uuid;
    QString msgid;
    QString sender;  //// long chunk history from one mail 
    QString md5; /// from incomming file server
    QString from;
    QString to;
    QString subject;
    QString charcodec;
    QString encodingsend;
    QString useragent;
    QString language;
    QString date;
    QString master;
    QStringList boundary_list;
    QString keya;
    QString keyb;
    QStringList multipart;
    QString root_cmd;
    QVariant tmp;
    QMap<int, QVariant> alist;
    operator QVariant() const {
        return QVariant::fromValue(*this);
    }
};

/// todo a textstream for console to insert chort info from this mail

inline QDataStream& operator<<(QDataStream& out, const ICmail& mail) {
    out << mail.from;
    out << mail.subject;
    out << mail.md5;
    out << mail.date;
    return out;
}
inline QDataStream& operator>>(QDataStream& in, ICmail& mail) {
    in >> mail.from;
    in >> mail.subject;
    in >> mail.md5;
    in >> mail.date;
    return in;
}

/*

  << mail.txt << ",\n"
  << mail.html << ",\n"
 */
inline QDebug &operator <<(QDebug debug, const ICmail &mail) {
    debug.nospace() << "ICmail{start(\n"
            << mail.from << ",\n"
            << mail.subject << ",\nContent-Transfer-Encoding:"
            << mail.date << "\n,"
            << mail.multipart << "\n,\nRoot cmd Content-Type:\n"
            << mail.root_cmd << ")end}\n";
    return debug.space();
}
Q_DECLARE_METATYPE(ICmail);
//// qRegisterMetaTypeStreamOperators<ICmail>("ICmail");
//// qRegisterMetaType<ICmail>("ICmail");
#endif	/* MIME_STANDARD_H */

