//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

//// this file become obsolete 
/// target to stay on parser_eml.h && mime_standard & mail_handler.h 


#ifndef MAIL_HANDLER_H
#define	MAIL_HANDLER_H

#include <QBuffer>
#include <QDomDocument>
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
#include <QPair>

#include "drupal_journalist.h"
#include "mime_standard.h"

#include <zlib.h>
#include <ctype.h>
#include "parser_config.h"



static QString fastmd5(const QByteArray xml) {
    QCryptographicHash formats(QCryptographicHash::Md5);
    formats.addData(xml);
    return QString(formats.result().toHex().constData());
}

const QChar newline_br = QChar(10);

// From qtsdk-2010.05/qt/src/network/access/qhttpnetworkreply_p.h
static const unsigned char gz_magic[2] = {0x1f, 0x8b}; // gzip magic header
// gzip flag byte
#define HEAD_CRC     0x02 // bit 1 set: header CRC present
#define EXTRA_FIELD  0x04 // bit 2 set: extra field present
#define ORIG_NAME    0x08 // bit 3 set: original file name present
#define COMMENT      0x10 // bit 4 set: file comment present
#define RESERVED     0xE0 // bits 5..7: reserved
#define CHUNK 16384



/* 
 * class to handle mail stream
 * class method PutOnEml can so read line by line to 
 * caputre header and boody data
 * 
 * longstream fix:
 * https://code.google.com/p/qt-gmail-access/source/browse/trunk/GmailHand/lib/qtopiamail/longstream.cpp
 */

/* RFC1521 says that a boundary "must be no longer 
 * than 70 characters, not counting the two leading hyphens".
 */



namespace ReadMail {


    

    static inline QString encodeQP(QString s) {
        for (int i = 0; i < s.size(); i++) {
            int c = s.at(i).unicode();
            if (c == 9 || c == 10 || c == 13) continue;
            if ((c < 32 || s.at(i) == 61 || c >= 126)) {
                //the char has to be replaced (only "=" and non-ascii characters)
                QString hex("=");
                if (QString::number(c, 16).size() == 1) hex.append(QString::number(0));
                hex.append(QString::number(c, 16).toUpper());

                s.replace(i, 1, "=" + QString::number(c, 16).toUpper());
                i += QString::number(c, 16).size(); //jump over inserted hex-code
            }
        }
        return s;
    }
    /// ReadMail
    static inline QByteArray& decodeQP(const QByteArray &input) {
        //                    0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?  @  A   B   C   D   E   F
        const int hexVal[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15};

        QByteArray *output = new QByteArray();

        for (int i = 0; i < input.length(); ++i) {
            if (input.at(i) == '=') {
                output->append((hexVal[input.at(++i) - '0'] << 4) + hexVal[input.at(++i) - '0']);
            } else {
                output->append(input.at(i));
            }
        }

        return *output;
    }

    ////static QByteArray key_start(const QString& key);
    ////static QByteArray key_end(const QString& key);

    QString decodeWordSequence(const QByteArray& str);
    int insensitindex(const QByteArray& target, const QByteArray &source, int from = -1, int offset = 0, int len = -1);

    static inline QString bytesToSize(qint64 size) {
        if (size < 0)
            return QString();
        if (size < 1024)
            return QObject::tr("%1 B").arg(QString::number(((double) size), 'f', 0));
        if ((size >= 1024) && (size < 1048576))
            return QObject::tr("%1 KB").arg(QString::number(((double) size) / 1024, 'f', 0));
        if ((size >= 1048576) && (size < 1073741824))
            return QObject::tr("%1 MB").arg(QString::number(((double) size) / 1048576, 'f', 2));
        if (size >= 1073741824)
            return QObject::tr("%1 GB").arg(QString::number(((double) size) / 1073741824, 'f', 2));
        return QString();
    }

    /////  From qtsdk - 2010.05 / qt / src / network / access / qhttpnetworkreply_p.h
    static const unsigned char gz_magic[2] = {0x1f, 0x8b}; // gzip magic header
    // gzip flag byte
#define HEAD_CRC     0x02 // bit 1 set: header CRC present
#define EXTRA_FIELD  0x04 // bit 2 set: extra field present
#define ORIG_NAME    0x08 // bit 3 set: original file name present
#define COMMENT      0x10 // bit 4 set: file comment present
#define RESERVED     0xE0 // bits 5..7: reserved
#define CHUNK 16384

    // From qtsdk-2010.05/qt/src/network/access/qhttpnetworkreply.cpp
    // && bool ClientParser::gzipCheckHeader(QByteArray &content, int &pos)  same file 
    ///  bool ClientParser::gzipCheckHeader(QByteArray &content, int &pos)

    static inline void deflateCompress(const QByteArray& uncompressed, QByteArray& deflated) {
        deflated = qCompress(uncompressed);
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
    }

    class StreamMail {
    public:

        StreamMail();
#if !defined(Q_OS_WIN)
        QString freeSpaceHome() const;
#endif

        enum Status {
            Ok, OutOfSpace
        };

        ~StreamMail() {
            d->close();
        }
        bool clear();

        void start() {
            d->seek(0);
        }

        bool LoadFile(const QString file);

        bool LoadLongFile(const QString file);

        bool WriteOnFile(const QString file);

        const quint16 size_line();
        /// save current stream inside here from remote imap to on eml format file 
        /// inside variable can parse...
        /// or data from filled by QBuffer
        bool PutOnEml(const QString emlfile, QString& s_title, bool field = false);

        QBuffer *device() {
            return d; /// device().write()
        }

        QByteArray bstream() {
            return d->data();
        }

        const QString stream() {
            return QString(d->data().constData());
        }

        void updateStatus();

        Status status() {
            return DISKSPACE; /// if avaiable for this stream x2 
        }

    private:
        bool freeSpaceDir(const QString &path = "", int min = 200000);
        QByteArray HEADER_PART;
        QByteArray BODY_PART;
        /// QMailFormat imail;
        Status DISKSPACE;
        QBuffer *d;
        int linenr;
        static const unsigned long long minFree = 1024 * 100;
        static const uint minCheck = 1024 * 10;
    };

}

#endif	/* MAIL_HANDLER_H */

