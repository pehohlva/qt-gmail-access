//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#include "parser_utils.h"
#include "tidy_clean.h"
#include <zlib.h>



/*  const  QString tidicaches = QString("%2/.qtidy/").arg(QDir::homePath());
                QTidy *tidy = new QTidy();    QTidy  *tidy; 
                tidy->Init(tidicaches);   tidy cache remove on last event 
                externhtml = tidy->TidyExternalHtml(externhtml);
 */

namespace Utils {

    // From qtsdk-2010.05/qt/src/network/access/qhttpnetworkreply_p.h
    //// https://qt.gitorious.org/qt/qt/raw/af7d398158e5be1e809df1fe4feb074d7d3eafb6:src/network/access/qhttpnetworkreply.cpp
    static const unsigned char gz_magic[2] = {0x1f, 0x8b}; // gzip magic header
    // gzip flag byte
#define HEAD_CRC     0x02 // bit 1 set: header CRC present
#define EXTRA_FIELD  0x04 // bit 2 set: extra field present
#define ORIG_NAME    0x08 // bit 3 set: original file name present
#define COMMENT      0x10 // bit 4 set: file comment present
#define RESERVED     0xE0 // bits 5..7: reserved
#define CHUNK 16384

    QString InfoHumanSize(qint64 x) {
        float num = x;
        QStringList list;
        list << "KB" << "MB" << "GB" << "TB";

        QStringListIterator i(list);
        QString unit("bytes");

        while (num >= 1024.0 && i.hasNext()) {
            unit = i.next();
            num /= 1024.0;
        }
        return QString().setNum(num, 'f', 2) + " " + unit;
    }

    QString cleanDocFromChartset(const QByteArray chartset, QByteArray chunk) {

        bool in_debug = true;
        QString afteretidy = testDocAttachmentDir("aftertidy.html");
        QString beforetidy = testDocAttachmentDir("bevoretidy.html");
        QString crudonudo = testDocAttachmentDir("fromclass.html");
        const QByteArray workon_chars = chartset.toLower();
        /*    QTextDocument *doc = new QTextDocument(0);
        doc->setHtml(QString(html.constData()));
        QString uhtml = doc->toHtml("utf-8");
         */
        if (in_debug) {
            _writebin_tofile(crudonudo, chunk);
        }
        QString xhtml, html;
        if (workon_chars.startsWith("utf-8")) {
            xhtml = QString::fromLocal8Bit(chunk.data(), chunk.size());
        } else {
            xhtml = QString::fromLatin1(chunk.data(), chunk.size());
        }
        QTextDocument *doc = new QTextDocument(0);
        doc->setHtml(xhtml);
        html = doc->toHtml("utf-8");
        //// https://csstidy.svn.sourceforge.net/svnroot/csstidy
        //// make error to leave attributes tidy remove so style
        //// /Users/pro/project/github/Drupalmail/
        html.replace(QString("class=\""), QString("_my=\""));
        html.replace(QString("style=\""), QString("_my=\""));
        QByteArray chunka = unicode_tr(html.toLocal8Bit());
        html = QString::fromLocal8Bit(chunka.data(), chunka.size());
        if (in_debug) {
            _write_file(beforetidy, html, QByteArray("utf-8"));
        }
        HTML::QTidy *cleaner = new HTML::QTidy();
        QString xhtmlbodyonly = cleaner->TidyCleanMailHtml(html, workon_chars);
        if (in_debug) {
            _write_file(afteretidy, xhtmlbodyonly.simplified(), QByteArray("utf-8"));
        }
        return xhtmlbodyonly.simplified();
    }

    QByteArray strip_tags(QByteArray istring) {
        bool intag = false;
        QByteArray new_string;

        for (int i = 0; i < istring.length(); i++) {
            QChar vox(istring.at(i));
            int letter = vox.unicode(); /// <60 62> 

            if (letter != 60 && !intag) {
                new_string += istring.at(i);
            }
            if (letter == 60 && !intag) {
                intag = true;
            }
            if (letter == 62 && intag) {
                intag = false;
            }
        }
        return new_string;
    }

    QByteArray unicode_tr(QByteArray istring) {
        bool intag = false;
        QByteArray new_string;
        for (int i = 0; i < istring.length(); i++) {
            QChar vox(istring.at(i));
            int letter = vox.unicode(); /// <60 62> 
            if (letter != 60 && !intag) {
                if (letter > 128) {
                    new_string.append("&#");
                    new_string.append(QByteArray::number(letter));
                    new_string.append(";");
                } else {
                    new_string += istring.at(i);
                }
            } else {
                new_string += istring.at(i);
            }
            if (letter == 60 && !intag) {
                intag = true;
            }
            if (letter == 62 && intag) {
                intag = false;
            }
        }
        return new_string.simplified();
    }

    QString testDocAttachmentDir(QString file) {
        QString path = _TESTDOCDIR_;
        QDir dir;
        if (!dir.exists(path)) {
            dir.mkpath(path);
        }
        if (!file.isEmpty()) {
            return path + file;
        }
        return path;
    }

    QString fastmd5(const QByteArray xml) {
        QCryptographicHash formats(QCryptographicHash::Md5);
        formats.addData(xml);
        return QString(formats.result().toHex().constData());
    }

    QByteArray search_byline(QByteArray chunk, QByteArray key) {
        QByteArray lc, la, word, wert;
        const int sizek = key.size();
        ///QChar cc('s');
        //cc.toLatin1();

        Q_FOREACH(QByteArray line, chunk.split(QChar(QChar::LineSeparator).toLatin1())) {
            lc = line.simplified().toLower();
            la = line.simplified();
            if (line.isEmpty() || line.size() < sizek) {
                continue;
            }
            if (lc.startsWith(key.toLower()) && line.size() > sizek) {
                word = la.mid(sizek, 99).simplified();
                //// return word;
                if (word.indexOf(";", 0) != -1) {
                    word.replace(QByteArray(";"), QByteArray());
                }
                if (word.indexOf(":", 0) != -1) {
                    word.replace(QByteArray(":"), QByteArray());
                }
                /// check space
                for (int i = 0; i < word.size(); ++i) {
                    QChar vox(word.at(i));
                    int letter = vox.unicode();
                    //// qDebug() << "l: " << letter << " "  << vox <<  "\n";
                    if (letter == 32) {
                        return wert.toLower();
                        break;
                    }
                    wert.append(word.at(i));
                }
                break;
            }
        }
        return wert;
    }

    QString quotecheck(QString str) {
        bool noquote = true;
        if (str.size() > 5) {
            for (int i = 0; i < str.size(); ++i) {
                QChar vox(str.at(i));
                int letter = vox.unicode();
                if (letter == 39 || letter == 34) {

                    return token(str, letter);
                }
            }

        }
        return str;
    }

    QString token(QString text, int unicode) {
        QByteArray t = QByteArray(text.toUtf8());
        QByteArray word = token(t, unicode, unicode);
        return QString(word.constData());
    }

    QString token(QString text, int unicode, int endchars) {
        QByteArray t = QByteArray(text.toUtf8());
        QByteArray word = token(t, unicode, endchars);
        return QString(word.constData());
    }

    QByteArray token(QByteArray text, int caret, int aftercaret) {
        QByteArray newone;
        int dk = 1;
        QByteArray *base = new QByteArray(text.simplified());
        bool founda = false;
        for (int i = 0; i < base->size(); ++i) {
            QChar vox(base->at(i));
            int letter = vox.unicode();
            if (letter == caret && dk == 1 || letter == aftercaret && dk == 1) {
                dk = 2;
                continue;
            }
            if (letter == caret && dk > 1 || letter == aftercaret && dk > 1) {
                return newone;
                break;
            }
            if (letter != caret && dk == 2 && letter != aftercaret) {
                newone.append(base->at(i));

                continue;
            }
        }
        return newone;
    }

    int _distance_position(const int a, const int b) {
        if (a == b) {
            return 0;
        }
        int docup = qMin(a, b);
        int atend = qMax(a, b);

        return ( atend - docup);
    }

    QString _partmd5(const QByteArray xml, int position) {
        QCryptographicHash formats(QCryptographicHash::Md5);
        QString found = QString("FilePosition=%1").arg(position);
        formats.addData(xml);
        formats.addData(xml);

        return QString(formats.result().toHex().constData());
    }

    //// to insert inline image 

    QByteArray _reformat_html(const QByteArray in) {

        QByteArray html, lc;

        Q_FOREACH(QByteArray line, in.split('\n')) {
            lc = line.simplified();
            if (!lc.isEmpty()) {

                html.append(lc);
                html.append("\n");
            }
        }
        return html;
    }

    QStringList _multipart_resolver(QString x) {
        QStringList keylist;
        QRegExp expression("multipart/(.*)[;\\s]", Qt::CaseInsensitive);
        expression.setMinimal(true);
        int iPosition = 0;
        //// search only on Mail Header part...
        while ((iPosition = expression.indexIn(x, iPosition)) != -1) {

            keylist.append(expression.cap(1));
            iPosition += expression.matchedLength();
        }
        return keylist;
    }

    bool _writebin_tofile(const QString xfile, const QByteArray chunk) {
        QFile file(xfile);
        if (file.open(QFile::WriteOnly)) {
            file.write(chunk, chunk.length()); // write to stderr
            file.close();
            return true;
        } else {

            return false;
        }
    }

    /* write a file to utf-8 format */
    bool _write_file(const QString fullFileName, const QString chunk, QByteArray charset) {
        ////QTextCodec *codecx;
        //// codecx = QTextCodec::codecForMib(106);
        QTextCodec *codectxt = QTextCodec::codecForName(charset);
        if (!codectxt) {
            return false;
        }

        QFile f(fullFileName);
        if (f.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream sw(&f);
            sw.setCodec(codectxt);
            sw << chunk;
            f.close();

            return true;
        }
        return false;
    }

    QString _format_string76(QString s) {
        ///
        QString repair = "";
        qint64 o = 0;
        for (int i = 0; i < s.size(); ++i) {
            o++;
            repair.append(s.at(i));
            if (o == 76) {

                o = 0;
                repair.append(QString("\n"));
            }
        }
        return repair;
    }

    QString _stripcore(QString x) {
        return quotecheck(x);
        //// qDebug() << vox.unicode() << ":" << vox << "_";
        const int less = x.size() - 1;
        QString newstr;
        for (int i = 0; i < x.size(); ++i) {
            QChar vox(x.at(i));
            int letter = vox.unicode();

            if (i == 0 || i == less) {
                if (letter == 39 ||
                        letter == 34 ||
                        letter == 59 ||
                        letter == 58) {
                    /// error at end or first!!
                } else {
                    newstr.append(vox);
                }

            } else {

                newstr.append(vox);
            }
        }
        return newstr;
    }

    QByteArray _RX_resolver(const QRegExp rx, QVariant x) {
        QByteArray text;
        QString result;
        if (x.canConvert(QVariant::ByteArray)) {
            text = x.toByteArray();
        }
        if (rx.indexIn(text) != -1) {
            QString start(rx.cap(1).simplified());
            QStringList words = start.split(" ", QString::SkipEmptyParts);
            if (words.size() > 0) {
                result = Utils::_stripcore(words.at(0));
                text = QByteArray(result.toLocal8Bit());
                if (text.length() > 76) {
                    text.clear();
                }
            }
        }
        return text;
    }

    /// gz compressed from search QHttpNetworkReplyPrivate 
    /// class QHttpNetworkReplyPrivate : public QObjectPrivate, public QHttpNetworkHeaderPrivate

    QByteArray compress_byte_gz(const QByteArray& uncompressed) {
        QByteArray deflated = qCompress(uncompressed);
        // eliminate qCompress size on first 4 bytes and 2 byte header
        deflated = deflated.right(deflated.size() - 6);
        // remove qCompress 4 byte footer
        deflated = deflated.left(deflated.size() - 4);

        QByteArray header;
        header.resize(10);
        header[0] = 0x1f; // gzip-magic[0]
        header[1] = 0x8b; // gzip-magic[1]
        header[2] = 0x08; // Compression method = DEFLATE
        header[3] = 0x00; // Flags
        header[4] = 0x00; // 4-7 is mtime
        header[5] = 0x00;
        header[6] = 0x00;
        header[7] = 0x00;
        header[8] = 0x00; // XFL
        header[9] = 0x03; // OS=Unix

        //// to decompress remove top 10 bottom 8 

        deflated.prepend(header);

        QByteArray footer;
        quint32 crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, (const uchar*) uncompressed.data(), uncompressed.size());
        footer.resize(8);
        footer[3] = (crc & 0xff000000) >> 24;
        footer[2] = (crc & 0x00ff0000) >> 16;
        footer[1] = (crc & 0x0000ff00) >> 8;
        footer[0] = (crc & 0x000000ff);

        quint32 isize = uncompressed.size();
        footer[7] = (isize & 0xff000000) >> 24;
        footer[6] = (isize & 0x00ff0000) >> 16;
        footer[5] = (isize & 0x0000ff00) >> 8;
        footer[4] = (isize & 0x000000ff);
        deflated.append(footer);

        return deflated;
    }

    bool gzipCheckHeader(QByteArray &content, int &pos) {
        int method = 0; // method byte
        int flags = 0; // flags byte
        bool ret = false;

        // Assure two bytes in the buffer so we can peek ahead -- handle case
        // where first byte of header is at the end of the buffer after the last
        // gzip segment
        pos = -1;
        QByteArray &body = content;
        int maxPos = body.size() - 1;
        if (maxPos < 1) {
            return ret;
        }

        // Peek ahead to check the gzip magic header
        if (body[0] != char(gz_magic[0]) ||
                body[1] != char(gz_magic[1])) {
            return ret;
        }
        pos += 2;
        // Check the rest of the gzip header
        if (++pos <= maxPos)
            method = body[pos];
        if (pos++ <= maxPos)
            flags = body[pos];
        if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
            return ret;
        }

        // Discard time, xflags and OS code:
        pos += 6;
        if (pos > maxPos)
            return ret;
        if ((flags & EXTRA_FIELD) && ((pos + 2) <= maxPos)) { // skip the extra field
            unsigned len = (unsigned) body[++pos];
            len += ((unsigned) body[++pos]) << 8;
            pos += len;
            if (pos > maxPos)
                return ret;
        }
        if ((flags & ORIG_NAME) != 0) { // skip the original file name
            while (++pos <= maxPos && body[pos]) {
            }
        }
        if ((flags & COMMENT) != 0) { // skip the .gz file comment
            while (++pos <= maxPos && body[pos]) {
            }
        }
        if ((flags & HEAD_CRC) != 0) { // skip the header crc
            pos += 2;
            if (pos > maxPos)
                return ret;
        }
        ret = (pos < maxPos); // return failed, if no more bytes left
        return ret;
    }

    QByteArray deflate_gz(const QByteArray chunk) {
        QTemporaryFile file;
        /* must not go on file solution gunzip buffer ?  go cache from net location */
        ///// const QString tmpfiler = QString(file.fileName().toStdString());
        /// tmpfiler.toStdString();
        const char *tmpgzfile = qPrintable(file.fileName()); //// .toStdString();
        QByteArray input;
        if (file.open()) {
            file.write(chunk);
            file.close();
            gzFile filegunzip;
            filegunzip = gzopen(tmpgzfile, "rb");
            if (!filegunzip) {
                qDebug() << "### Unable to work on tmp file ... ";
                return QByteArray();
            }
            char buffer[1024];
            while (int readBytes = gzread(filegunzip, buffer, 1024)) {
                input.append(QByteArray(buffer, readBytes));
            }
            gzclose(filegunzip);
            file.remove();
        }
        return input;
    }

    int dateswap(const QString format, uint unixtime) {
        QDateTime fromunix;
        fromunix.setTime_t(unixtime);
        const QString numeric = fromunix.toString(format);
        bool ok;
        int num = numeric.toInt(&ok);
        if (ok) {
            return num;
        } else {
            return 0;
        }
    }

    QString date_imap_format(const qint64 unixtime) {
        QStringList RTFmonth = QStringList() << "month_NULL" << "Jan" << "Feb" << "Mar" << "Apr" << "May" << "Jun" << "Jul" << "Aug" << "Sep" << "Oct" << "Nov" << "Dec";
        const QString Monthrcf = RTFmonth.at(dateswap("M", unixtime));
        QString datex = QString::number(dateswap("d", unixtime)) + "-" + Monthrcf + "-" + QString::number(dateswap("yyyy", unixtime)); //// 20-Sep-2013
        return datex;
    }

    QString utf8loadfromfile(const QString file) {
        QFile *f = new QFile(file);
        QByteArray ccbyline;
        if (f->open(QIODevice::ReadOnly)) {
            //// read line by line 
            if (f->isReadable()) {
                int linenr = -1;
                while (!f->atEnd()) {
                    linenr++;
                    ccbyline.append(f->read(76));
                }
                f->close();
            }
        }
        return QString::fromUtf8(ccbyline.constData(), ccbyline.size());
    }







}


namespace Filter {

    bool headerfilter(QString line) {
        QStringList xfilter = QStringList() << "Message-ID:" << "Subject:" << "To:" << "From:"
                << "Received:" << "Delivered-To:" << "CC:" << "Delivered-To:" << "Return-Path:" << "Date:";
        const QString lcl = line.simplified().toLower();
        for (int i = 0; i < xfilter.size(); ++i) {
            QString xkey = QString(xfilter.at(i).toLocal8Bit().simplified().toLower());
            if (lcl.startsWith(xkey)) {
                //// ok new line!!
                return true;
            }

        }
        return false;
    }

    QString lineonPos(const QString need, QString chunk) {
        QStringList h_lines = chunk.split(QRegExp("(\\r\\n)|(\\n\\r)|\\r|\\n"), QString::SkipEmptyParts);
        for (int i = 0; i < h_lines.size(); ++i) {
            const QString line = QString(h_lines.at(i).simplified());
            if (!line.isEmpty()) {
                if (line.startsWith(need)) {
                    int all = line.length();
                    int dwo = need.length();
                    return line.mid(dwo, all - dwo).simplified();
                }
            }
        }
        return QString();
    }


}







