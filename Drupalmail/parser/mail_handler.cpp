//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#include "mail_handler.h"
#include "net_imap_standard.h"
#include <QTextCodec>
#include <QTextCodec>
#include "parser_config.h"
#include <QtGlobal>

#ifndef USE_FANCY_MATCH_ALGORITHM
#include <ctype.h>
#endif

namespace ReadMail {





#ifdef USE_FANCY_MATCH_ALGORITHM
#define REHASH(a) \
    if (ol_minus_1 < sizeof(uint) * CHAR_BIT) \
        hashHaystack -= (a) << ol_minus_1; \
    hashHaystack <<= 1
#endif

    /*
     param target / find item
     param source / find on array
     from the pos to init or -1
     off ofset range from begin search
     len length
     const char* const begin = source.constData() + off;
     const char* const end = begin + len - (target.length() - 1);
     */
    int insensitivepos(const QByteArray& target, const QByteArray &source, int from, int off, int len) {

        qDebug() << "search tag -> " << target << "\n";

#ifndef USE_FANCY_MATCH_ALGORITHM
        const char* const matchBegin = target.constData();
        const char* const matchEnd = matchBegin + target.length();

        const char* const begin = source.constData() + off;
        const char* const end = begin + len - (target.length() - 1);

        const char* it = 0;
        if (from >= 0) {
            it = begin + from;
        } else {
            it = begin + len + from;
        }

        while (it < end) {
            if (toupper(*it++) == toupper(*matchBegin)) {
                const char* restart = it;

                // See if the remainder matches
                const char* searchIt = it;
                const char* matchIt = matchBegin + 1;

                do {
                    if (matchIt == matchEnd)
                        return ((it - 1) - begin);

                    // We may find the next place to search in our scan
                    if ((restart == it) && (*searchIt == *(it - 1)))
                        restart = searchIt;
                } while (toupper(*searchIt++) == toupper(*matchIt++));

                // No match
                it = restart;
            }
        }

        return -1;
#else
        // Based on QByteArray::indexOf, except use strncasecmp for
        // case-insensitive string comparison
        const int ol = target.length();
        if (from > len || ol + from > len)
            return -1;
        if (ol == 0)
            return from;

        const char *needle = target.data();
        const char *haystack = source.data() + off + from;
        const char *end = source.data() + off + (len - ol);
        const uint ol_minus_1 = ol - 1;
        uint hashNeedle = 0, hashHaystack = 0;
        int idx;
        for (idx = 0; idx < ol; ++idx) {
            hashNeedle = ((hashNeedle << 1) + needle[idx]);
            hashHaystack = ((hashHaystack << 1) + haystack[idx]);
        }
        hashHaystack -= *(haystack + ol_minus_1);

        while (haystack <= end) {
            hashHaystack += *(haystack + ol_minus_1);
            if (hashHaystack == hashNeedle && *needle == *haystack
                    && strncasecmp(needle, haystack, ol) == 0) {
                return haystack - source.data();
            }
            REHASH(*haystack);
            ++haystack;
        }
        return -1;
#endif
    }

    QString decodeByteArray(const QByteArray &encoded, const QString &charset) {
        if (QTextCodec * codec = QTextCodec::codecForName(charset.toLatin1())) {
            return codec->toUnicode(encoded);
        }
        return QString::fromUtf8(encoded, encoded.size());
    }

    /** @short Convert a hex digit into a number */
    static inline int hexValueOfChar(const char input) {
        if (input >= '0' && input <= '9') {
            return input - '0';
        } else if (input >= 'A' && input <= 'F') {
            return 0x0a + input - 'A';
        } else if (input >= 'a' && input <= 'f') {
            return 0x0a + input - 'a';
        } else {
            return -1;
        }
    }

    /** @short Decode an encoded-word as per RFC2047 into a unicode string */
    static QString decodeWord(const QByteArray &fullWord, const QByteArray &charset, const QByteArray &encoding, const QByteArray &encoded) {
        if (encoding == "Q") {
            return decodeByteArray(_translateFromQuotedPrintable(encoded), charset);
        } else if (encoding == "B") {
            return decodeByteArray(QByteArray::fromBase64(encoded), charset);
        } else {
            return fullWord;
        }
    }

    int insensitindex(const QByteArray& target, const QByteArray &source, int from, int offset, int len) {
        return insensitivepos(target, source, from, offset, len);
    }

    /** @short Decode a header in the RFC 2047 format into a unicode string */
    QString decodeWordSequence(const QByteArray& str) {
        QRegExp whitespace("^\\s+$");

        QString out;

        // Any idea why this isn't matching?
        //QRegExp encodedWord("\\b=\\?\\S+\\?\\S+\\?\\S*\\?=\\b");
        QRegExp encodedWord("\"?=\\?(\\S+)\\?(\\S+)\\?(.*)\\?=\"?");

        // set minimal=true, to match sequences which do not have whit space in between 2 encoded words; otherwise by default greedy matching is performed
        // eg. "Sm=?ISO-8859-1?B?9g==?=rg=?ISO-8859-1?B?5Q==?=sbord" will match "=?ISO-8859-1?B?9g==?=rg=?ISO-8859-1?B?5Q==?=" as a single encoded word without minimal=true
        // with minimal=true, "=?ISO-8859-1?B?9g==?=" will be the first encoded word and "=?ISO-8859-1?B?5Q==?=" the second.
        // -- assuming there are no nested encodings, will there be?
        encodedWord.setMinimal(true);

        int pos = 0;
        int lastPos = 0;

        while (pos != -1) {
            pos = encodedWord.indexIn(str, pos);
            if (pos != -1) {
                int endPos = pos + encodedWord.matchedLength();

                QString preceding(str.mid(lastPos, (pos - lastPos)));
                QString decoded = decodeWord(str.mid(pos, (endPos - pos)), encodedWord.cap(1).toLatin1(),
                        encodedWord.cap(2).toUpper().toLatin1(), encodedWord.cap(3).toLatin1());

                // If there is only whitespace between two encoded words, it should not be included
                if (!whitespace.exactMatch(preceding))
                    out.append(preceding);

                out.append(decoded);

                pos = endPos;
                lastPos = pos;
            }
        }

        // Copy anything left
        out.append(QString::fromUtf8(str.mid(lastPos)));

        return out;
    }

    static QString decodeWordSequence(const QString str) {
        const QByteArray faketmp = str.toAscii();
        return decodeWordSequence(faketmp);
    }




#if defined(Q_OS_SYMBIAN)
#include <f32file.h>
#elif defined(Q_OS_WIN)  
#include <windows.h>
#elif defined(Q_OS_MAC)
    /// separate on conflict
#endif

#if defined(Q_OS_UNIX)
    /// #include <sys/vfs.h>
#endif

#if defined(Q_OS_MAC)
#include <sys/param.h>
#include <sys/mount.h>
#endif

    static QString tempDirMail() {
        QString path = _READMAILTMPDIR_;
        QDir dir;
        if (!dir.exists(path))
            dir.mkpath(path);
        return path;
    }
    

    StreamMail::StreamMail()
    : d(new QBuffer()) {
        d->open(QIODevice::ReadWrite);
        start();
    }

    //// only imap client use this to save on disk mail /// readable from os client mail

    bool StreamMail::PutOnEml(const QString emlfile, QString& s_title, bool field) {
        //// Q_UNUSED(emlfile);
        updateStatus();
        start();

        QTextCodec *codecx;
        codecx = QTextCodec::codecForMib(106);

        if (DISKSPACE != Ok) {
            /// activate warning!!
            return false;
        }
        QFileInfo info_file_eml(emlfile);
        if (field) {
            return WriteOnFile(emlfile);
        }

        //////  s_title = "arriva...";

        quint16 cursor = 0;
        const quint16 totline = size_line(); /// line tot on chunk 
        bool canread_body, firstnull_found = false;
        QByteArray body_part;
        QTextStream doc(&body_part);
        QRegExp subjectMatch("Subject: +(.*)");
        QByteArray onlyMETAHEADER = QByteArray();
        while (d->canReadLine()) {
            QByteArray chunk = d->readLine();
            ///// const QString t_line = QString(chunk.constData()); //// decodeWordSequence(chunk);
            const QByteArray searchnull = chunk.simplified();
            cursor++;
            /// range from line first and last 
            if (cursor > 1 && cursor < totline) {
                doc << cursor << ") " << chunk; /// save all 

                if (searchnull.isEmpty() && !firstnull_found) {
                    firstnull_found = true;
                    canread_body = true;
                }
                if (!firstnull_found) {
                    onlyMETAHEADER.append(chunk);
                    ///// HEADER PARTS!!!!!!!///// HEADER PARTS!!!!!!!///// HEADER PARTS!!!!!!!
                }

            } //// not read after this line!
        }

        doc.flush();
        BODY_PART = body_part; /// decoded 
        start();

        QString clean_header = decodeWordSequence(onlyMETAHEADER);
        onlyMETAHEADER.clear();
        //// now can parse this header 
        quint16 lastposition = 0;
        QString Hbody_debug, real_line = "";
        QStringList h_lines = clean_header.split(QRegExp("(\\r\\n)|(\\n\\r)|\\r|\\n"), QString::SkipEmptyParts);
        for (int i = 0; i < h_lines.size(); ++i) {
            const QString line = QString(h_lines.at(i).toAscii().constData());
            QChar fchar(line.at(0));
            if (subjectMatch.indexIn(line) != -1) {
                s_title = QString("Subject:") + subjectMatch.cap(1).simplified(); /// Subject
            }
            if (fchar.isUpper()) {
                ////Hbody_debug.append(QString("!%1!: ").arg(fchar.unicode()));
                Hbody_debug.append(line.simplified());
                if (real_line.size() > 0) {
                    Hbody_debug.insert(lastposition, real_line.simplified());
                    real_line = "";
                } else {
                    ////Hbody_debug.append("\n\r");
                }
                lastposition = (Hbody_debug.length() - 2);
                Hbody_debug.append("\n\r");
            }
            if (fchar.unicode() == 32 || fchar.unicode() == 9) {
                real_line.append(QString(" - "));
                ////real_line.append(QString("_____%1!: ").arg(fchar.unicode()));
                real_line.append(line.simplified());
                ///// real_line.prepend(QString("!%1!").arg(fchar.unicode()));
            }
        }
        const QString sepheader = _READMAILTMPDIR_ + QString("%1.mailheader.txt").arg(info_file_eml.completeBaseName()); //+".header"; /// debug header 
        bool headerOK = write_file(sepheader, Hbody_debug);

        if (info_file_eml.exists() && headerOK) {
            return true;
        }


        QFile f(emlfile); /// sepheader
        if (f.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream sw(&f);
            sw.setCodec(codecx);
            sw << body_part;
            f.close();
            //// continue here to handle header & body part 
            //  to extract after all chunk ist saved on file
            if (headerOK) {
                return true;
            } else {
                return false;
            }


        }
        return false;
    }

    bool StreamMail::clear() {
        /// remove all chunk inside
        d->seek(0);
        d->write(QByteArray());
        /////qDebug() << "######## d->bytesAvailable():" << d->bytesAvailable() << "\n";
        return d->bytesAvailable() == 0 ? true : false;
    }

    bool StreamMail::LoadFile(const QString file) {
        ////qFatal(" unable to read ... ");
        bool filled = false;
        if (clear()) {

            QFile *f = new QFile(file);

            if (f->exists()) {
                /////
                if (f->open(QIODevice::ReadOnly)) {
                    //// read line by line 
                    if (f->isReadable()) {
                        linenr = -1;
                        while (!f->atEnd()) {
                            linenr++;
                            QByteArray crk = f->read(76);
                            d->write(crk);
                        }
                        f->close();
                        d->write(QByteArray("\n--\n--\n--\n--"));
                    }
                } else {
                    qDebug() << "######## file errors:" << f->errorString() << "\n";
                }
            }
        }

        if (d->bytesAvailable() > 23) {
            filled = true;
        }
        d->seek(0);
        return filled;
    }

    bool StreamMail::LoadLongFile(const QString fileeml) {
        QFile f(fileeml);

        qDebug() << "######## read " << fileeml << " try  " << __LINE__ << "\n";

        QByteArray out;
        bool canread = false;
        qDebug() << "######## read " << fileeml << " try  " << __LINE__ << "\n";
        if (f.exists()) {
            qDebug() << "######## read " << fileeml << " try  " << __LINE__ << "\n";
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                clear();
                qDebug() << "######## read " << fileeml << " try  " << __LINE__ << "\n";
                QDataStream in(&f); /// QIODevice::ReadOnly | QIODevice::Text)
                while (!in.atEnd()) {
                    in >> out;
                    d->write(out);
                    qDebug() << "##" << out.mid(0, 3) << " not ok  ";
                }
                f.close();
                return true;
            }
        } else {
            qDebug() << "######## read " << fileeml << " try  " << __LINE__ << "\n";
            if (bstream().size() > 1) {
                canread = true;
            }
            qDebug() << "######## read " << fileeml << " try  " << __LINE__ << "\n";
            return canread;
        }

        ///char buffer[1024];
        ////while (int readBytes = gzread(file, buffer, 1024)) {
        ///input.append(QByteArray(buffer, readBytes));
        ///}
        //self format as chunk 
        //// http://fop-miniscribus.googlecode.com/svn/trunk/http_debugger.1.0/gmain.cpp
    }

    /// return the total of line 

    const quint16 StreamMail::size_line() {
        quint16 cursor = 0;
        start();
        while (d->canReadLine()) {
            const QByteArray chunk = d->readLine();
            Q_UNUSED(chunk);
            cursor++;
        }
        start();
        return cursor;
    }

    bool StreamMail::WriteOnFile(const QString file) {

        QFile f(file);
        updateStatus();

        if (DISKSPACE != Ok) {
            /// activate warning!!
            return false;
        }

        start();
        if (f.open(QFile::WriteOnly)) {
            uint bi = f.write(d->readAll());
            f.close();
            start();
            return bi > 0 ? true : false;
        }
        return false;
    }

    void StreamMail::updateStatus() {
        quint64 havingbite = (d->data().size() * 2);
        if (minFree < havingbite) {
            havingbite = minFree;
        }
        bool canhandle = freeSpaceDir(QString(_READMAILTMPDIR_), havingbite);
        /// Status { Ok, OutOfSpace };
        if (canhandle) {
            DISKSPACE = Ok;
        } else {
            DISKSPACE = OutOfSpace;
        }
    }

#if !defined(Q_OS_WIN)

    QString StreamMail::freeSpaceHome() const {
        QString partitionPath = tempDirMail() + ".";
        struct statfs stats;
        statfs(partitionPath.toLocal8Bit(), &stats);
        unsigned long long bavail = ((unsigned long long) stats.f_bavail);
        unsigned long long bsize = ((unsigned long long) stats.f_bsize);
        return bytesToSize(bavail * bsize);
    }
#endif

    bool StreamMail::freeSpaceDir(const QString &path, int min) {
        unsigned long long boundary = minFree;
        if (min >= 0)
            boundary = min;

        QString partitionPath = tempDirMail() + ".";
        if (!path.isEmpty())
            partitionPath = path;

#if defined(Q_OS_SYMBIAN)
        bool result(false);

        RFs fsSession;
        TInt rv;
        if ((rv = fsSession.Connect()) != KErrNone) {
            qDebug() << "Unable to connect to FS:" << rv;
        } else {
            TParse parse;
            TPtrC name(path.utf16(), path.length());

            if ((rv = fsSession.Parse(name, parse)) != KErrNone) {
                qDebug() << "Unable to parse:" << path << rv;
            } else {
                TInt drive;
                if ((rv = fsSession.CharToDrive(parse.Drive()[0], drive)) != KErrNone) {
                    qDebug() << "Unable to convert:" << QString::fromUtf16(parse.Drive().Ptr(), parse.Drive().Length()) << rv;
                } else {
                    TVolumeInfo info;
                    if ((rv = fsSession.Volume(info, drive)) != KErrNone) {
                        qDebug() << "Unable to volume:" << drive << rv;
                    } else {
                        result = (info.iFree > boundary);
                    }
                }
            }

            fsSession.Close();
        }

        return result;
#elif !defined(Q_OS_WIN)
        struct statfs stats;

        statfs(partitionPath.toLocal8Bit(), &stats);
        unsigned long long bavail = ((unsigned long long) stats.f_bavail);
        unsigned long long bsize = ((unsigned long long) stats.f_bsize);
        qDebug() << "bavail DISK:" << bytesToSize(bavail * bsize);
        return ((bavail * bsize) > boundary);
#else
        // MS recommend the use of GetDiskFreeSpaceEx, but this is not available on early versions
        // of windows 95.  GetDiskFreeSpace is unable to report free space larger than 2GB, but we're 
        // only concerned with much smaller amounts of free space, so this is not a hindrance.
        DWORD bytesPerSector(0);
        DWORD sectorsPerCluster(0);
        DWORD freeClusters(0);
        DWORD totalClusters(0);

        if (::GetDiskFreeSpace(partitionPath.utf16(), &bytesPerSector, &sectorsPerCluster, &freeClusters, &totalClusters) == FALSE) {
            qWarning() << "Unable to get free disk space:" << partitionPath;
        }

        return ((bytesPerSector * sectorsPerCluster * freeClusters) > boundary);
#endif
    }

}

