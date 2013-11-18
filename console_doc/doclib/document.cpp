/* 
 * File:   document.cpp
 * Author: pro
 * Struzzo bello in fare le cose
 * Created on 13. November 2013, 08:54
 */

#include "document.h"
#include "kzip.h"
#include "config.h"
#include <QDomDocument>
#include <QDir>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QDomDocument>
#include <QProcess>
#include <QStringList>

QString Document::execlocal(const QString cmd, const QStringList args) {

    QString debughere = cmd;
    int success = -1;
    debughere.append(" ");
    debughere.append(args.join(" "));
    console(QString("Document::execlocal: %1").arg(debughere));
    QString rec;
    QProcess *process = new QProcess(NULL);
    process->setReadChannelMode(QProcess::MergedChannels);
    process->start(cmd, args, QIODevice::ReadOnly);
    ////  waitForFinished default int msecs = 30000 ok
    if (!process->waitForFinished(90000)) {
        success = -1;
    } else {
        success = 1;
        rec = process->readAll();
    }
    if (success == 1) {
        process->close();
        return rec;
    } else {
        process->close();
        return QString("ERROR: Time out by %1").arg(debughere);
    }
}

QByteArray Document::convertPNG(QImage image, int w, int h) {

    QByteArray ba;
    QImage crep;
    QBuffer buffer(&ba);
    if (w > 0 && h > 0) {
        /// resize image 
        crep = image.scaled(w, h);
        if (!crep.isNull()) {
            buffer.open(QIODevice::WriteOnly);
            crep.save(&buffer, "PNG");
            return ba;
        }
    }

    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG"); // writes image into ba in PNG format
    return ba;
}

QString Document::xslt2_qt5(const QStringList args) {
    QString action;
    //// const QString cmd = QTXSLT2;
    QString debughere = QTXSLT2;
    debughere.append(" ");
    debughere.append(args.join(" "));
    console(QString("EXEC: %1").arg(debughere));

    QProcess *process = new QProcess(NULL);
    process->setReadChannelMode(QProcess::MergedChannels);
    process->start(QTXSLT2, args, QIODevice::ReadOnly);
    ////  waitForFinished default int msecs = 30000 ok
    if (!process->waitForFinished()) {
        action.append("ERROR: Time out...");
    } else {
        action.append(process->readAll());
        action.append("End success...");
    }
    process->close();
    return action;
}

bool Document::swap() {
    d->close();
    d = NULL;
    d = new QBuffer();
    if (!d->open(QIODevice::ReadWrite)) {
        setError("Unable to open Buffer!");
    }
    return bufferisNull();
}

Document::Document(const QString n)
: d(new QBuffer()), document_name(n) {
    QStringList args;
    args << "xsltproc";
    bin_xsltproc = execlocal(QString("which"), args).simplified();

    QStringList argsa;
    argsa << "pdftohtml";
    bin_pdftohtml = execlocal(QString("which"), argsa).simplified();

    if (!d->open(QIODevice::ReadWrite)) {
        setError("Unable to open Buffer!");
    }
    cache_dir = APPCACHEDOC;

    if (!dir.exists(cache_dir)) {
        dir.mkpath(cache_dir);
    }

    if (!dir.exists(cache_dir)) {
        setError(QString("Unable to create dir on: %1").arg(cache_dir));
    }


}

void Document::readfile(const QString file) {
    document_file = file;
    _loader();
}

void Document::setError(const QString &error) {
    /// to warning handle at end ..
    console(QString("Error: %1").arg(error), 1);
}

void Document::console(QString msg, int modus) {

    QTextStream out(stdout);
    const QString classname = document_name;
    QString str("*");
    if (modus == 0) {
        // init chat..
        out << classname << str.fill('*', pointo - classname.length()) << "\n";
        out << "Start: " << Tools::TimeNow() << "\n";
    }
    out << "Console: " << msg << "\n";
    if (modus == 3 || modus == 2) {
        // close chat..

        out << "End: " << Tools::TimeNow() << "\n";
        out << classname << str.fill('*', pointo - classname.length()) << "\n";
    }
    out.flush();

}

QByteArray Document::loadbinfile(const QString file) {
    QFile *f = new QFile(file);
    if (f->exists()) {
        if (f->open(QIODevice::ReadOnly)) {
            if (f->isReadable()) {
                return f->readAll();
            }
        }
    }
    f->close();
    return QByteArray();
}

void Document::_loader() {
    d->write(QByteArray());
    if (d->bytesAvailable() == 0) {
        QFile *f = new QFile(document_file);
        if (f->exists()) {
            if (f->open(QIODevice::ReadOnly)) {
                if (f->isReadable()) {
                    d->write(f->readAll());
                    f->close();
                }
            }
        }
    }
}

bool Document::bufferisNull() {
    if (d->bytesAvailable() == 0) {
        return true;
    } else {
        return false;
    }
}

bool Document::flush_onfile(const QString filedest) {
    QFile file(filedest);
    if (file.open(QFile::WriteOnly)) {
        file.write(d->data(), d->data().length()); // write to stderr
        file.close();
        d->write(QByteArray());
        if (d->bytesAvailable() == 0) {
            return true;
        }
    }

    return false;
}

bool Document::flush_onxmlfile(const QString fullFileName) {
    QTextCodec *codectxt = QTextCodec::codecForMib(106);
    QDomDocument doc = xmltoken();
    QFile f(fullFileName);
    if (f.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream sw(&f);
        sw.setCodec(codectxt);
        sw << doc.toString(5);
        f.close();
        d->write(QByteArray());
        if (d->bytesAvailable() == 0) {
            return true;
        }
    }
    return false;
}

QDomDocument Document::xmltoken() {
    QXmlSimpleReader reader;
    QXmlInputSource source;
    source.setData(d->data());
    QString errorMsg;
    QDomDocument document;
    if (!document.setContent(&source, &reader, &errorMsg)) {
        setError(QString("Invalid XML document: %1").arg(errorMsg));
        setError(QString("Invalid XML Size: %1").arg(d->data().size()));
        return QDomDocument();
    }
    return document;
}

void Document::clean() {
    html_stream.clear();
    QDir dir(cache_dir);
    //// RM_dir_local();
    QFile::remove(style_file);
    if (dir.exists(cache_dir)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (!info.isDir()) {
                if (info.suffix().toLower() !="html" && info.suffix().toLower() !="xml") {
                    QFile::remove(info.absoluteFilePath());
                }
            }
        }

    }
}

Document::~Document() {


}

