//
// C++ Implementation: KZip to read zip file from ODT document on QBuffer
// NOTE only read not write!
// Description:
// idea from qt QZipReader & http://code.mythtv.org/ code
// to build append LIBS += -lz 
// Author: Peter Hohl <pehohlva@gmail.com>,    24.10.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#include "kzip.h"
#include <QtCore/qdatetime.h>
#include <QtCore/qendian.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qfile.h>
#include <QtCore/qlist.h>
#include <QtCore/qshareddata.h>
#include <qplatformdefs.h>
#include <QtCore>
#include <QDomDocument>
#include <QTextDocument>
#include <QColor>
#include <QCryptographicHash>
#include <QtCore/qiodevice.h>
#include <QtCore/qbytearray.h>
#include <QDate>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QDomDocument>

#if defined(Q_OS_SYMBIAN)
#include <f32file.h>
#elif defined(Q_OS_WIN)  
#include <windows.h>
#elif defined(Q_OS_MAC)
/// separate on conflict
#endif

#if defined(Q_OS_UNIX)
#include <sys/vfs.h>
#endif

#if defined(Q_OS_MAC)
#include <sys/param.h>
#include <sys/mount.h>
#endif

/// only to debug modus!!!



namespace KZip {


    //! Local header size (excluding signature, excluding variable length fields)
#define UNZIP_LOCAL_HEADER_SIZE 26
    //! Central Directory file entry size (excluding signature, excluding variable length fields)
#define UNZIP_CD_ENTRY_SIZE_NS 42
    //! Data descriptor size (excluding signature)
#define UNZIP_DD_SIZE 12
    //! End Of Central Directory size (including signature, excluding variable length fields)
#define UNZIP_EOCD_SIZE 22
    //! Local header entry encryption header size
#define UNZIP_LOCAL_ENC_HEADER_SIZE 12

    // Some offsets inside a CD record (excluding signature)
#define UNZIP_CD_OFF_VERSION_MADE 0
#define UNZIP_CD_OFF_VERSION 2
#define UNZIP_CD_OFF_GPFLAG 4
#define UNZIP_CD_OFF_CMETHOD 6
#define UNZIP_CD_OFF_MODT 8
#define UNZIP_CD_OFF_MODD 10
#define UNZIP_CD_OFF_CRC32 12
#define UNZIP_CD_OFF_CSIZE 16
#define UNZIP_CD_OFF_USIZE 20
#define UNZIP_CD_OFF_NAMELEN 24
#define UNZIP_CD_OFF_XLEN 26
#define UNZIP_CD_OFF_COMMLEN 28
#define UNZIP_CD_OFF_LHOFFSET 38

    // Some offsets inside a local header record (excluding signature)
#define UNZIP_LH_OFF_VERSION 0
#define UNZIP_LH_OFF_GPFLAG 2
#define UNZIP_LH_OFF_CMETHOD 4
#define UNZIP_LH_OFF_MODT 6
#define UNZIP_LH_OFF_MODD 8
#define UNZIP_LH_OFF_CRC32 10
#define UNZIP_LH_OFF_CSIZE 14
#define UNZIP_LH_OFF_USIZE 18
#define UNZIP_LH_OFF_NAMELEN 22
#define UNZIP_LH_OFF_XLEN 24

    // Some offsets inside a data descriptor record (excluding signature)
#define UNZIP_DD_OFF_CRC32 0
#define UNZIP_DD_OFF_CSIZE 4
#define UNZIP_DD_OFF_USIZE 8

    // Some offsets inside a EOCD record
#define UNZIP_EOCD_OFF_ENTRIES 6
#define UNZIP_EOCD_OFF_CDOFF 12
#define UNZIP_EOCD_OFF_COMMLEN 16

    /*!
     Max version handled by this API.
     0x14 = 2.0 --> full compatibility only up to this version;
     later versions use unsupported features
     */
#define UNZIP_VERSION 0x14

    static quint32 permissionsToMode(QFile::Permissions perms) {
        quint32 mode = 0;
        if (perms & QFile::ReadOwner)
            mode |= S_IRUSR;
        if (perms & QFile::WriteOwner)
            mode |= S_IWUSR;
        if (perms & QFile::ExeOwner)
            mode |= S_IXUSR;
        if (perms & QFile::ReadUser)
            mode |= S_IRUSR;
        if (perms & QFile::WriteUser)
            mode |= S_IWUSR;
        if (perms & QFile::ExeUser)
            mode |= S_IXUSR;
        if (perms & QFile::ReadGroup)
            mode |= S_IRGRP;
        if (perms & QFile::WriteGroup)
            mode |= S_IWGRP;
        if (perms & QFile::ExeGroup)
            mode |= S_IXGRP;
        if (perms & QFile::ReadOther)
            mode |= S_IROTH;
        if (perms & QFile::WriteOther)
            mode |= S_IWOTH;
        if (perms & QFile::ExeOther)
            mode |= S_IXOTH;
        return mode;
    }

    static QFile::Permissions modeToPermissions(quint32 mode) {
        QFile::Permissions ret;
        if (mode & S_IRUSR)
            ret |= QFile::ReadOwner;
        if (mode & S_IWUSR)
            ret |= QFile::WriteOwner;
        if (mode & S_IXUSR)
            ret |= QFile::ExeOwner;
        if (mode & S_IRUSR)
            ret |= QFile::ReadUser;
        if (mode & S_IWUSR)
            ret |= QFile::WriteUser;
        if (mode & S_IXUSR)
            ret |= QFile::ExeUser;
        if (mode & S_IRGRP)
            ret |= QFile::ReadGroup;
        if (mode & S_IWGRP)
            ret |= QFile::WriteGroup;
        if (mode & S_IXGRP)
            ret |= QFile::ExeGroup;
        if (mode & S_IROTH)
            ret |= QFile::ReadOther;
        if (mode & S_IWOTH)
            ret |= QFile::WriteOther;
        if (mode & S_IXOTH)
            ret |= QFile::ExeOther;
        return ret;
    }

    static inline uint readUInt(const uchar *data) {
        return (data[0]) + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
    }

    static inline ushort readUShort(const uchar *data) {
        return (data[0]) + (data[1] << 8);
    }

    static inline void writeUInt(uchar *data, uint i) {
        data[0] = i & 0xff;
        data[1] = (i >> 8) & 0xff;
        data[2] = (i >> 16) & 0xff;
        data[3] = (i >> 24) & 0xff;
    }

    static inline void writeUShort(uchar *data, ushort i) {
        data[0] = i & 0xff;
        data[1] = (i >> 8) & 0xff;
    }

    static int deflate(Bytef *dest, ulong *destLen, const Bytef *source, ulong sourceLen) {
        z_stream stream;
        int err;

        stream.next_in = (Bytef*) source;
        stream.avail_in = (uInt) sourceLen;
        stream.next_out = dest;
        stream.avail_out = (uInt) * destLen;
        if ((uLong) stream.avail_out != *destLen) return Z_BUF_ERROR;

        stream.zalloc = (alloc_func) 0;
        stream.zfree = (free_func) 0;
        stream.opaque = (voidpf) 0;

        err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
        if (err != Z_OK) return err;

        err = deflate(&stream, Z_FINISH);
        if (err != Z_STREAM_END) {
            deflateEnd(&stream);
            return err == Z_OK ? Z_BUF_ERROR : err;
        }
        *destLen = stream.total_out;

        err = deflateEnd(&stream);
        return err;
    }

    static void writeMSDosDate(uchar *dest, const QDateTime& dt) {
        if (dt.isValid()) {
            quint16 time =
                    (dt.time().hour() << 11) // 5 bit hour
                    | (dt.time().minute() << 5) // 6 bit minute
                    | (dt.time().second() >> 1); // 5 bit double seconds

            dest[0] = time & 0xff;
            dest[1] = time >> 8;

            quint16 date =
                    ((dt.date().year() - 1980) << 9) // 7 bit year 1980-based
                    | (dt.date().month() << 5) // 4 bit month
                    | (dt.date().day()); // 5 bit day

            dest[2] = char(date);
            dest[3] = char(date >> 8);
        } else {
            dest[0] = 0;
            dest[1] = 0;
            dest[2] = 0;
            dest[3] = 0;
        }
    }

    static int inflate(Bytef *dest, ulong *destLen, const Bytef *source, ulong sourceLen) {
        z_stream stream;
        int err;

        stream.next_in = (Bytef*) source;
        stream.avail_in = (uInt) sourceLen;
        if ((uLong) stream.avail_in != sourceLen)
            return Z_BUF_ERROR;

        stream.next_out = dest;
        stream.avail_out = (uInt) * destLen;
        if ((uLong) stream.avail_out != *destLen)
            return Z_BUF_ERROR;

        stream.zalloc = (alloc_func) 0;
        stream.zfree = (free_func) 0;

        err = inflateInit2(&stream, -MAX_WBITS);
        if (err != Z_OK)
            return err;

        err = inflate(&stream, Z_FINISH);
        if (err != Z_STREAM_END) {
            inflateEnd(&stream);
            if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
                return Z_DATA_ERROR;
            return err;
        }
        *destLen = stream.total_out;

        err = inflateEnd(&stream);
        return err;
    }

    /// Stream 
#define ZIP_VERSION 20

    Stream::Stream(const QString odtfile) {

        d = new QBuffer();
        ErrorCode ec = ReadFailed;
        uBuffer = (unsigned char*) buffer1;
        //// crcTable = (quint32*) get_crc_table(); /// zlib
        is_open = false;
        if (d->open(QIODevice::ReadWrite)) {
            is_open = LoadFile(odtfile);
            ec = seekToCentralDirectory();
            ////qDebug() << "### seekToCentralDirectory return ...  " << ec;
            if (ec != Stream::OkFunky) {
                is_open = false;
            }
            if (cdEntryCount == 0) {
                is_open = false;
            }
        }

        if (!is_open) {
            clear(); // remove buffer 
        } else {
            ec = openArchive();
            if (ec == Stream::OkFunky) {
                /// KZIPDEBUG() << "open file decompress.......";
                int i;
                QStringList pieces;
                for (i = 0; i < fileHeaders.size(); ++i) {
                    FileHeader metaheader = fileHeaders.at(i);
                    const QString zfile = QString::fromLocal8Bit(metaheader.file_name);
                    if (zfile.indexOf("/")) {
                        QStringList subdir = zfile.split("/");
                        pieces << subdir;
                    }
                    QByteArray chunk = fileByte(zfile);
                    zip_files << zfile;
                    if (chunk.size() > 3) {
                        corefilelist.insert(zfile, chunk);
                        ////KZIPDEBUG() << "contenent size()=" << chunk.size();
                    }
                    //// KZIPDEBUG() << "cat ." << i << " - " << zfile;
                }
                zip_files << QString("##items##");
                zip_files << pieces;
                zip_files.removeDuplicates();
            }
        }
    }

    QByteArray Stream::fileByte(const QString &fileName) {

        int compressed_size = 0;
        int uncompressed_size = 0;
        int i;
        bool found = false;
        for (i = 0; i < fileHeaders.size(); ++i) {
            if (QString::fromLocal8Bit(fileHeaders.at(i).file_name) == fileName) {
                found = true;
                break;
            }

        }
        start(); /// seek 0;
        if (!found || !device()) {
            return QByteArray();
        }

        FileHeader metaheader = fileHeaders.at(i);
        compressed_size = readUInt(metaheader.h.compressed_size);
        uncompressed_size = readUInt(metaheader.h.uncompressed_size);
        if (uncompressed_size < 1) {
            return QByteArray();
        }
        int start = readUInt(metaheader.h.offset_local_header);
        device()->seek(start);
        LocalFileHeader lh;
        device()->read((char *) &lh, sizeof (LocalFileHeader));
        uint skip = readUShort(lh.file_name_length) + readUShort(lh.extra_field_length);
        device()->seek(device()->pos() + skip);
        int compression_method = readUShort(lh.compression_method);
        QByteArray compressed = device()->read(compressed_size);
        const QString zfile = QString::fromLocal8Bit(metaheader.file_name);
        KZIPDEBUG() << "|||" << compression_method << "|||| fileByte file " << zfile << " s:" << uncompressed_size << ":" << compressed_size;

        if (compression_method == 0) {
            // no compression
            compressed.truncate(uncompressed_size);
            return compressed;
        } else if (compression_method == 8) {
            /// real unzip part file 
            compressed.truncate(compressed_size);
            QByteArray decompress_chunk;
            ulong len = qMax(uncompressed_size, 1);
            int res;
            do {
                decompress_chunk.resize(len);
                res = inflate((uchar*) decompress_chunk.data(), &len, (uchar*) compressed.constData(), compressed_size);
                if (res == Z_OK) {
                    if ((int) len != decompress_chunk.size()) {
                        decompress_chunk.resize(len);
                    }
                    break;
                } else {
                    decompress_chunk.clear();
                    qWarning("KZip: Z_DATA_ERROR: Input data is corrupted");
                }

            } while (res == Z_BUF_ERROR);

            return decompress_chunk;
        }

        qWarning() << "KZip: Unknown compression method";
        return QByteArray();
    }

    Stream::ErrorCode Stream::openArchive() {
        Q_ASSERT(device());

        if (!canread()) {
            qDebug() << "Unable to open device for reading";
            return Stream::OpenFailed;
        }

        start(); /// seek 0;

        uchar tmp[4];
        device()->read((char *) tmp, 4);
        if (getULong(tmp, 0) != 0x04034b50) {
            qWarning() << "KZip: not a zip file!";
            return Stream::OpenFailed;
        }

        // find EndOfDirectory header
        int i = 0;
        int start_of_directory = -1;
        int num_dir_entries = 0;
        EndOfDirectory eod;
        while (start_of_directory == -1) {
            int pos = device()->size() - sizeof (EndOfDirectory) - i;
            if (pos < 0 || i > 65535) {
                qWarning() << "KZip: EndOfDirectory not found";
                return Stream::OpenFailed;
            }
            device()->seek(pos);
            device()->read((char *) &eod, sizeof (EndOfDirectory));
            if (readUInt(eod.signature) == 0x06054b50)
                break;
            ++i;
        }
        start_of_directory = readUInt(eod.dir_start_offset);
        num_dir_entries = readUShort(eod.num_dir_entries);
        if (cdEntryCount != num_dir_entries) {
            return Stream::OpenFailed;
        }
        KZIPDEBUG("start_of_directory at %d, num_dir_entries=%d", start_of_directory, num_dir_entries);
        int comment_length = readUShort(eod.comment_length);
        if (comment_length != i) {
            qWarning() << "KZip: failed to parse zip file.";
            return Stream::OpenFailed;
        }
        commentario = device()->read(qMin(comment_length, i));

        ///// KZIPDEBUG() << "### comment_length:" << comment_length;

        device()->seek(start_of_directory);
        for (i = 0; i < num_dir_entries; ++i) {
            FileHeader header;
            int read = device()->read((char *) &header.h, sizeof (GentralFileHeader));
            if (read < (int) sizeof (GentralFileHeader)) {
                qWarning() << "KZip: Failed to read complete header, index may be incomplete";
                break;
            }
            if (readUInt(header.h.signature) != 0x02014b50) {
                qWarning() << "KZip: invalid header signature, index may be incomplete";
                break;
            }

            int l = readUShort(header.h.file_name_length);
            header.file_name = device()->read(l);
            if (header.file_name.length() != l) {
                qWarning() << "KZip: Failed to read filename from zip index, index may be incomplete";
                break;
            }
            l = readUShort(header.h.extra_field_length);
            header.extra_field = device()->read(l);
            if (header.extra_field.length() != l) {
                qWarning() << "KZip: Failed to read extra field in zip file, skipping file, index may be incomplete";
                break;
            }
            l = readUShort(header.h.file_comment_length);
            header.file_comment = device()->read(l);
            if (header.file_comment.length() != l) {
                qWarning() << "KZip: Failed to read read file comment, index may be incomplete";
                break;
            }

            //// KZIPDEBUG() << "### pos:" << i; /// header.h.compressed_size
            //// KZIPDEBUG("found at file:{%s}", header.file_name.data());
            fileHeaders.append(header);
        }

        return Stream::OkFunky;
    }

    Stream::ErrorCode Stream::seekToCentralDirectory() {
        Q_ASSERT(device());

        qint64 length = device()->size();
        qint64 offset = length - UNZIP_EOCD_SIZE;
        if (length < UNZIP_EOCD_SIZE) {
            return Stream::InvalidArchive;
        }
        if (!device()->seek(offset)) {
            return Stream::SeekFailed;
        }
        if (device()->read(buffer1, UNZIP_EOCD_SIZE) != UNZIP_EOCD_SIZE) {
            return Stream::ReadFailed;
        }
        bool eocdFound = (buffer1[0] == 'P' && buffer1[1] == 'K' && buffer1[2] == 0x05 && buffer1[3] == 0x06);
        if (eocdFound) {
            // Zip file has no comment (the only variable length field in the EOCD record)
            eocdOffset = offset;
        } else {
            return Stream::HandleCommentHere;
            /* 
            qint64 read;
            char* p = 0;
            offset -= UNZIP_EOCD_SIZE;
            if (offset <= 0) {
                return Stream::InvalidArchive;
            }
            if (!device()->seek(offset)) {
                return Stream::SeekFailed;
            }
            int cursor =-1;
            while ((read = device()->read(buffer1, UNZIP_EOCD_SIZE)) >= 0) {
                cursor++;
                qDebug() << "### cursor:" << cursor << "|";
            }
             * */
        }

        if (!eocdFound) {
            return Stream::InvalidArchive;
        }
        // Parse EOCD to locate CD offset
        offset = getULong((const unsigned char*) buffer1, UNZIP_EOCD_OFF_CDOFF + 4);
        cdOffset = offset;
        cdEntryCount = getUShort((const unsigned char*) buffer1, UNZIP_EOCD_OFF_ENTRIES + 4);
        quint16 commentLength = getUShort((const unsigned char*) buffer1, UNZIP_EOCD_OFF_COMMLEN + 4);
        if (commentLength != 0) {
            return Stream::HandleCommentHere;
        }
        if (!device()->seek(cdOffset)) {
            return Stream::SeekFailed;
        }
        return Stream::OkFunky;
    }

    bool Stream::LoadFile(const QString file) {
        if (clear()) {
            QFile f(file);
            if (f.exists()) {
                if (f.open(QFile::ReadOnly)) {
                    d->write(f.readAll());
                    f.close();
                    start();
                    return true;
                }
            }
        }
        return false;
    }

    void Stream::explode_todir(const QString path, int modus) {
        QDir dir(path);
        if (!dir.exists(path)) {
            return;
        }
        int filecount = 0;
        const QString ziplocaldir = dir.canonicalPath();
        QMapIterator<QString, QByteArray> i(corefilelist);
        while (i.hasNext()) {
            i.next();
            const QString singlefile = ziplocaldir + "/" + QString(i.key());
            QFileInfo pfile(singlefile);
            const QString ext = pfile.completeSuffix().toLower();
            const QString d_path = pfile.absolutePath();
            if (!dir.exists(d_path)) {
                dir.mkpath(d_path);
            }
            if (pfile.absolutePath() != pfile.absoluteFilePath()) {

                QByteArray data = i.value();
                // _write_file
                bool ok = write_xml_modus(pfile.absoluteFilePath(), data);
                if (!ok) {
                    if (modus == 1) {
                        RamBuffer *bufferbin = new RamBuffer("tmpbinary");
                        bufferbin->device()->write(data);
                        bufferbin->flush_onfile(pfile.absoluteFilePath());
                        bufferbin->clear();
                    } else {
                        qDebug() << "Warning to write NOT xml:" << pfile.absoluteFilePath();
                    }

                } else {
                    filecount++;
                }
            }
        }
        qDebug() << "KZip Writtel total " << filecount << " file on path:" << path;
    }

    bool Stream::write_xml_modus(const QString file, QByteArray x) {
        /// debug modus
        QFileInfo fi_c(file);
        const QString ext = fi_c.completeSuffix().toLower();
        if (ext == "xml") {
            RamBuffer *buffer = new RamBuffer("tmpxml");
            buffer->device()->write(x);
            QDomDocument doc = buffer->xmltoken();
            QString xml = doc.toString(5);
            buffer->clear();
            return Tools::_write_file(file, xml.toLocal8Bit(), "utf-8");
        }
        return false;
    }

    /*!
     \internal Reads an quint16 (2 bytes) from a byte array starting at given offset.
     */
    quint16 Stream::getUShort(const unsigned char* data, quint32 offset) const {
        return (quint16) data[offset] | (((quint16) data[offset + 1]) << 8);
    }

    /*!
     \internal Reads an quint64 (8 bytes) from a byte array starting at given offset.
     */
    quint64 Stream::getULLong(const unsigned char* data, quint32 offset) const {
        quint64 res = (quint64) data[offset];
        res |= (((quint64) data[offset + 1]) << 8);
        res |= (((quint64) data[offset + 2]) << 16);
        res |= (((quint64) data[offset + 3]) << 24);
        res |= (((quint64) data[offset + 1]) << 32);
        res |= (((quint64) data[offset + 2]) << 40);
        res |= (((quint64) data[offset + 3]) << 48);
        res |= (((quint64) data[offset + 3]) << 56);

        return res;
    }

    /*!
     \internal Reads an quint32 (4 bytes) from a byte array starting at given offset.
     */
    quint32 Stream::getULong(const unsigned char* data, quint32 offset) const {
        quint32 res = (quint32) data[offset];
        res |= (((quint32) data[offset + 1]) << 8);
        res |= (((quint32) data[offset + 2]) << 16);
        res |= (((quint32) data[offset + 3]) << 24);

        return res;
    }

    bool Stream::clear() {
        d->write(QByteArray());
        start();
        return d->bytesAvailable() == 0 ? true : false;
    }

    /*!
        Desctructor
     */
    Stream::~Stream() {
        clear();
        delete d;
    }

}

namespace Tools {

    QDomDocument format_xml(QByteArray x) {

        QXmlSimpleReader reader;
        QXmlInputSource source;
        source.setData(x);
        QString errorMsg;
        QDomDocument document;
        if (!document.setContent(&source, &reader, &errorMsg)) {
            return QDomDocument();
        }
        return document;
    }

    QString f_string76(QString s) {
        ///
        QString repair = "";
        qint64 o = 0;
        for (int i = 0; i < s.size(); ++i) {
            o++;
            repair.append(s.at(i));
            if (o == 2000) {

                o = 0;
                repair.append(QString("\n"));
            }
        }
        return repair;
    }

    /* time null unix time long nummer */
    uint QTime_Null() {
        QDateTime timer1(QDateTime::currentDateTime());
        return timer1.toTime_t();
    }
    ////  DateUnix(QTime_Null())

    QString DateUnix(const uint etime) {
        QDateTime start = QDateTime::currentDateTime();
        start.setTime_t(etime);
        if (start.toString("HH:mm") == "00:00") {
            return start.toString("dd.MM.yyyy");
        } else {
            return start.toString("dd.MM.yyyy HH:mm");
        }
    }

    QString SimpleMonat(const uint etime) {
        QDateTime start = QDateTime::currentDateTime();
        start.setTime_t(etime);
        return start.toString("dd.MM.");
    }

    /* return int value from a unixtime date MMM YYY ... */
    int dtateint(const QString form, uint unixtime) {
        QDateTime fromunix;
        fromunix.setTime_t(unixtime);
        QString numeric = fromunix.toString((const QString) form);
        bool ok;
        const int rec = numeric.toInt(&ok);
        if (ok) {
            return rec;
        }
        return (0);
    }

    //// QDateTime fromunix;
    /// fromunix.setTime_t(unixtime);

    /* display a mail date format  UmanTimeFromUnix(QTime_Null())   */
    QString UmanTimeFromUnix(uint unixtime) {
        /* mail rtf Date format! http://www.faqs.org/rfcs/rfc788.html */
        QDateTime fromunix;
        bool ok;
        QDateTime now = QDateTime::currentDateTime();
        fromunix.setTime_t(unixtime);
        QStringList RTFdays = QStringList() << "day_NULL" << "Mon" << "Tue" << "Wed" << "Thu" << "Fri" << "Sat" << "Sun";
        QStringList RTFmonth = QStringList() << "month_NULL" << "Jan" << "Feb" << "Mar" << "Apr" << "May" << "Jun" << "Jul" << "Aug" << "Sep" << "Oct" << "Nov" << "Dec";
        int anno = fromunix.toString("yyyy").toInt(&ok);
        int mese = fromunix.toString("M").toInt(&ok);
        int giorno = fromunix.toString("d").toInt(&ok);
        QDate timeroad(anno, mese, giorno);
        QStringList rtfd_line;
        rtfd_line.clear();
        if (!ok) {
            rtfd_line.append("Error Time! ");
        } else {
            rtfd_line.append("Date: ");
        }

        rtfd_line.append(RTFdays.at(timeroad.dayOfWeek()));
        rtfd_line.append(", ");
        rtfd_line.append(QString::number(giorno));
        rtfd_line.append(" ");
        rtfd_line.append(RTFmonth.at(mese));
        rtfd_line.append(" ");
        rtfd_line.append(QString::number(anno));
        rtfd_line.append(" ");
        rtfd_line.append(now.toString("hh:mm:ss:zzz"));
        rtfd_line.append("");
        /*qDebug() << "### mail rtf Date format " << rtfd_line.join("");*/
        return QString(rtfd_line.join(""));
    }

    /* correct codex from xml file read only first line */
    QTextCodec *GetcodecfromXml(const QString xmlfile) {
        QString semencoding = "UTF-8";
        QTextCodec *codecsin;
        QFile *xfile = new QFile(xmlfile);
        if (!xfile->exists()) {
            codecsin = QTextCodec::codecForName(semencoding.toLocal8Bit());
            return codecsin;
        }

        QString Firstline;
        bool validxml = false;
        if (xfile->open(QIODevice::ReadOnly)) {
            char buf[1024];
            qint64 lineLength = xfile->readLine(buf, sizeof (buf));
            Firstline = QString(buf);
            if (lineLength > 10 && Firstline.contains("encoding")) {
                validxml = true;
            }
        }
        if (!validxml) {
            codecsin = QTextCodec::codecForName(semencoding.toLocal8Bit());
            return codecsin;
        }
        QRegExp expression("encoding=[\"\'](.*)[\"\']", Qt::CaseInsensitive);
        expression.setMinimal(true);
        int iPosition = 0;
        while ((iPosition = expression.indexIn(Firstline, iPosition)) != -1) {
            semencoding = expression.cap(0);
            semencoding = semencoding.mid(10, semencoding.size() - 11);
            iPosition += expression.matchedLength();
            //// qDebug() << "### semencoding" << semencoding;
        }
        if (iPosition == -1) {
            codecsin = QTextCodec::codecForName("UTF-8");
        } else {
            codecsin = QTextCodec::codecForName(semencoding.toLocal8Bit());
        }
        return codecsin;
    }

    bool _write_file(const QString fullFileName, QByteArray chunk) {

        QFile file(fullFileName);
        if (file.open(QFile::WriteOnly)) {
            file.write(chunk, chunk.length()); // write to stderr
            file.close();
            return true;
        }

        return false;
    }

    /* write a file to utf-8 format */
    bool _write_file(const QString fullFileName, const QString chunk, QByteArray charset) {
        ////QTextCodec *codecx;
        //// QTextCodec *codectxt = QTextCodec::codecForMib(106);
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

    QString fastmd5(const QByteArray xml) {
        QCryptographicHash formats(QCryptographicHash::Md5);
        formats.addData(xml);
        return QString(formats.result().toHex().constData());
    }


    /// gz compressed from search QHttpNetworkReplyPrivate 
    /// class QHttpNetworkReplyPrivate : public QObjectPrivate, public QHttpNetworkHeaderPrivate

    QByteArray inflate_byte_gz(const QByteArray& uncompressed) {
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

    QByteArray deflate_byte_gz(const QByteArray chunk) {
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
                qDebug() << "### Unable to work on tmp file gz  ... ";
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

    QString TimeNow() {
        return UmanTimeFromUnix(QTime_Null());
    }
}


namespace SystemSecure {

    bool RM_dir_local(const QString &dirName) {
        bool result = true;
        QDir dir(dirName);

        if (dir.exists(dirName)) {

            Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
                if (info.isDir()) {
                    result = RM_dir_local(info.absoluteFilePath());
                } else {
                    result = QFile::remove(info.absoluteFilePath());
                }

                if (!result) {
                    return result;
                }
            }
            result = dir.rmdir(dirName);
        }

        return result;
    }
//// 1355776
    QString bytesToSize(const qint64 size) {
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

    qint64 FreespaceonDir(const QString selectdir) {

#if !defined(Q_OS_WIN)
        struct statfs stats;
        statfs(selectdir.toLocal8Bit(), &stats);
        unsigned long long bavail = ((unsigned long long) stats.f_bavail);
        unsigned long long bsize = ((unsigned long long) stats.f_bsize);
        return (qint64) (bavail * bsize);
#else
        // MS recommend the use of GetDiskFreeSpaceEx, but this is not available on early versions
        // of windows 95.  GetDiskFreeSpace is unable to report free space larger than 2GB, but we're 
        // only concerned with much smaller amounts of free space, so this is not a hindrance.
        DWORD bytesPerSector(0);
        DWORD sectorsPerCluster(0);
        DWORD freeClusters(0);
        DWORD totalClusters(0);
        if (::GetDiskFreeSpace(selectdir.utf16(), &bytesPerSector, &sectorsPerCluster, &freeClusters, &totalClusters) == FALSE) {
            qWarning() << "Unable to get free disk space on:" << selectdir;
        }
        return (qint64) ((bytesPerSector * sectorsPerCluster * freeClusters) > boundary);
#endif
        return 0;

        /* 
         * #if defined(Q_OS_SYMBIAN)
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
        } */

    }


}


namespace KZip {

    KZip::FileInfo::FileInfo()
    : isDir(false), isFile(true), isSymLink(false), crc32(0), size(0) {
    }

    KZip::FileInfo::~FileInfo() {
    }

    KZip::FileInfo::FileInfo(const FileInfo &other) {
        operator=(other);
    }

    KZip::FileInfo& KZip::FileInfo::operator=(const FileInfo &other) {
        filePath = other.filePath;
        isDir = other.isDir;
        isFile = other.isFile;
        isSymLink = other.isSymLink;
        permissions = other.permissions;
        crc32 = other.crc32;
        size = other.size;
        return *this;
    }

    


}
