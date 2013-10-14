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




/*  const  QString tidicaches = QString("%2/.qtidy/").arg(QDir::homePath());
                QTidy *tidy = new QTidy();    QTidy  *tidy; 
                tidy->Init(tidicaches);   tidy cache remove on last event 
                externhtml = tidy->TidyExternalHtml(externhtml);
 */

namespace Utils {

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
        QByteArray chunka = unicode_tr(html.toAscii());
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

        Q_FOREACH(QByteArray line, chunk.split(QChar(QChar::LineSeparator).toAscii())) {
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
                    wert.append(vox.toAscii());
                }
                break;
            }
        }
        return word.toLower();
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
        QByteArray t = QByteArray(text.toAscii());
        QByteArray word = token(t, unicode, unicode);
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
                newone.append(vox.toAscii());
                continue;
            }
        }
        return newone;
    }

    QString token(QString str, QChar c1, QChar c2, int *index) {
        int start, stop;

        // The strings we're tokenizing use CRLF as the line delimiters - assume that the
        // caller considers the sequence to be atomic.
        if (c1 == CarriageReturn_)
            c1 = LineFeed_;
        start = str.indexOf(c1, *index, Qt::CaseInsensitive);
        if (start == -1)
            return str;

        if (c2 == CarriageReturn_)
            c2 = LineFeed_;
        stop = str.indexOf(c2, ++start, Qt::CaseInsensitive);
        if (stop == -1)
            return str;

        // Exclude the CR if necessary
        if (stop && (str[stop - 1] == CarriageReturn_))
            --stop;

        // Bypass the LF if necessary
        *index = stop + 1;
        return str.mid(start, stop - start);
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
                text = QByteArray(result.toAscii());
                if (text.length() > 76) {
                    text.clear();
                }
            }
        }
        return text;
    }


}







