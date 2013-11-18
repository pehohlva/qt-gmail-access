//
// C++ Implementation: KZip to read zip file from ODT or DOCX document on QBuffer
// unzip on Buffer or to dir 
// NOTE only read not write!
// Description:
// idea from qt QZipReader & http://code.mythtv.org/ code
// to build append LIBS += -lz 
// Author: Peter Hohl <pehohlva@gmail.com>,    24.10.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#ifndef KZIP_H
#define	KZIP_H

#include <zlib.h>

#include <QtCore/qfile.h>
#include <QtCore/qstring.h>
#include <QtCore/QMap>
#include <QtCore/QDebug>
#include <QtCore/QBuffer>
#include <QtCore/qiodevice.h>
#include <QtCore/qbytearray.h>
#include <QStringList>
#include <QtXml/QDomDocument>
#include <QTextDocument>
#include <QCryptographicHash>
#include <QDate>
#include <QDateTime>
#include <QStringList>
#include <QDomElement>
#include <QBuffer>
#include <QTextCodec>
#include <QFile>
#include <QString>
#include <QDebug>
#include <QDir>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QDomDocument>
#include <QProcess>

#include <stdio.h>
#include <math.h>
#include <zlib.h>
#include <ctype.h>


/// 6 okt. 2013 save on svn 
/// great this to push class on qvariant only by develop  validate class!!  

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



class RamBuffer {
public:

    RamBuffer(const QString XtypenameCall)
    : d(new QBuffer()), name(XtypenameCall) {
        d->open(QIODevice::ReadWrite);
    }

    ~RamBuffer() {
        d->close();
    }

    QString type() const {
        return name;
    }

    QDomDocument xmltoken() {
        QXmlSimpleReader reader;
        QXmlInputSource source;
        source.setData(d->data());
        QString errorMsg;
        QDomDocument document;
        if (!document.setContent(&source, &reader)) {
            /////setError(QString("Invalid XML document: %1").arg(errorMsg));
            return QDomDocument();
        }
        return document;
    }

    bool clear() {
        d->write(QByteArray());
        return d->bytesAvailable() == 0 ? true : false;
    }

    bool flush_onfile(const QString filedest) {
        QFile file(filedest);
        if (file.open(QFile::WriteOnly)) {
            file.write(d->data(), d->data().length()); // write to stderr
            file.close();
            return true;
        }
    }

    bool flush_onxmlfile(const QString fullFileName) {
        QTextCodec *codectxt = QTextCodec::codecForMib(106);
        QFile f(fullFileName);
        if (f.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream sw(&f);
            sw.setCodec(codectxt);
            sw << d->data();
            f.close();
            return true;
        }
        return false;
    }

    bool LoadFile(const QString file, int from = -1) {
        /// from begin to read at line 
        //// qDebug() << "### LoadFile from " << from;
        bool filled = false;
        if (clear()) {

            QFile *f = new QFile(file);

            if (f->exists()) {
                /////
                if (f->open(QIODevice::ReadOnly)) {
                    //// read line by line 
                    int cursorline = -1;
                    if (f->isReadable()) {
                        int begin = -1;
                        if (from > -1) {
                            begin = from;
                        }
                        while (!f->atEnd()) {
                            cursorline++;
                            QByteArray crk = f->readLine();
                            //// qDebug() << "### cursorline:" << cursorline << "\n";
                            if (cursorline > begin || begin == cursorline) {
                                d->write(crk);
                            }
                        }
                        f->close();
                    }
                } else {
                    return false;
                    //// qDebug() << "######## file errors:" << f->errorString() << "\n";
                }
            }
        }

        if (d->bytesAvailable() > 23) {
            filled = true;
        }
        d->seek(0);
        return filled;
    }

    QIODevice *device() {
        return d;
    }

    QByteArray stream() {
        return d->data();
    }

    QString fromUtf8() {
        return QString::fromUtf8(stream());
    }
protected:
    QString name;
    QBuffer *d;
};







namespace Tools {
    QString f_string76(QString s);
    QString UmanTimeFromUnix(uint unixtime);
    int dtateint(const QString form, uint unixtime);
    QString SimpleMonat(const uint etime);
    QString DateUnix(const uint etime);
    uint QTime_Null();
    QTextCodec *GetcodecfromXml(const QString xmlfile);
    QDomDocument format_xml(QByteArray x);
    bool _write_file(const QString fullFileName, QByteArray chunk);
    bool _write_file(const QString fullFileName, const QString chunk, QByteArray charset);
    /// like php strip_tags to get text from html
    QByteArray strip_tags(QByteArray istring);
    QString fastmd5(const QByteArray xml);
    QByteArray inflate_byte_gz(const QByteArray& uncompressed);
    QByteArray deflate_byte_gz(const QByteArray chunk);
    bool gzipCheckHeader(QByteArray &content, int &pos);
    QString TimeNow();

}


namespace KZip {


    // zLib authors suggest using larger buffers (128K or 256K) for (de)compression (especially for inflate())
    // we use a 256K buffer here - if you want to use this code on a pre-iceage mainframe please change it ;)
#define UNZIP_READ_BUFFER (256*1024)
    ////  #define KZIPNODEBUGNOW 1

#if 0 //// 1 or 0 
#define KZIPDEBUG qDebug
#else
#define KZIPDEBUG if (0) qDebug
#endif

    struct LocalFileHeader {
        uchar signature[4]; //  0x04034b50
        uchar version_needed[2];
        uchar general_purpose_bits[2];
        uchar compression_method[2];
        uchar last_mod_file[4];
        uchar crc_32[4];
        uchar compressed_size[4];
        uchar uncompressed_size[4];
        uchar file_name_length[2];
        uchar extra_field_length[2];
    };

    struct DataDescriptor {
        uchar crc_32[4];
        uchar compressed_size[4];
        uchar uncompressed_size[4];
    };

    struct GentralFileHeader {
        uchar signature[4]; // 0x02014b50
        uchar version_made[2];
        uchar version_needed[2];
        uchar general_purpose_bits[2];
        uchar compression_method[2];
        uchar last_mod_file[4];
        uchar crc_32[4];
        uchar compressed_size[4];
        uchar uncompressed_size[4];
        uchar file_name_length[2];
        uchar extra_field_length[2];
        uchar file_comment_length[2];
        uchar disk_start[2];
        uchar internal_file_attributes[2];
        uchar external_file_attributes[4];
        uchar offset_local_header[4];
        LocalFileHeader toLocalHeader() const;
    };

    struct EndOfDirectory {
        uchar signature[4]; // 0x06054b50
        uchar this_disk[2];
        uchar start_of_directory_disk[2];
        uchar num_dir_entries_this_disk[2];
        uchar num_dir_entries[2];
        uchar directory_size[4];
        uchar dir_start_offset[4];
        uchar comment_length[2];
    };

    struct FileHeader {
        GentralFileHeader h; /// h.compressed_size
        QByteArray file_name;
        QByteArray extra_field;
        QByteArray file_comment;
    };

    struct FileInfo {
        FileInfo();
        FileInfo(const FileInfo &other);
        ~FileInfo();
        FileInfo &operator=(const FileInfo &other);
        QString filePath;
        uint isDir : 1;
        uint isFile : 1;
        uint isSymLink : 1;
        QFile::Permissions permissions;
        uint crc32;
        qint64 size;
        void *d;
    };

    //// KZip::Stream

    class Stream {
    public:
        Stream(const QString odtfile);
        ~Stream();
        void explode_todir(const QString path, int modus = 0);
        QByteArray fileByte(const QString &fileName);

        enum ErrorCode {
            OkFunky = 1000,
            ZlibInit,
            ZlibError,
            OpenFailed,
            PartiallyCorrupted,
            Corrupted,
            WrongPassword,
            NoOpenArchive,
            FileNotFound,
            ReadFailed,
            WriteFailed,
            SeekFailed,
            HandleCommentHere,
            CreateDirFailed,
            InvalidDevice,
            InvalidArchive,
            HeaderConsistencyError,
            Skip, SkipAll // internal use only
        };

        QIODevice *device() {
            return d;
        }

        QByteArray stream() {
            return d->data();
        }
        /// unique id from file

        QString md5hash() const {
            d->seek(0);
            QCryptographicHash formats(QCryptographicHash::Md5);
            formats.addData(d->data());
            d->seek(0);
            return QString(formats.result().toHex().constData());
        }
        ////

        bool canread() {
            return is_open;
        }

        /* having list from file and dir and single 
         * splittet name inside is to null data */
        QStringList filelist() const {
            return zip_files;
        }
        /// on debug modus write xml indent readable... 
        bool write_xml_modus(const QString file, QByteArray x);

        /*
         Having data filename && chunk QByteArray from his file 
         */
        QMap<QString, QByteArray> listData() {
            return corefilelist; /// having data 
        }
    protected:
        char buffer1[UNZIP_READ_BUFFER];
        char buffer2[UNZIP_READ_BUFFER];
        QList<FileHeader> fileHeaders;
        QStringList zip_files;
        QByteArray commentario;
        uint start_of_directory;

        unsigned char* uBuffer;
        //// const quint32* crcTable;
        quint32 cdOffset;
        // End of Central Directory (EOCD) offset
        quint32 eocdOffset;
        // Number of entries in the Central Directory (as to the EOCD record)
        quint16 cdEntryCount;

    private:
        QMap<QString, QByteArray> corefilelist; /// filename && chunk QByteArray inside
        ErrorCode seekToCentralDirectory();
        ErrorCode openArchive();
        quint32 getULong(const unsigned char* data, quint32 offset) const;
        inline quint64 getULLong(const unsigned char* data, quint32 offset) const;
        inline quint16 getUShort(const unsigned char* data, quint32 offset) const;

        void start() {
            d->seek(0);
        }
        bool clear(); /// remove buffer 
        bool LoadFile(const QString file);
        QBuffer *d;
        bool is_open;
    };

    
    

}


/// SystemSecure::bytesToSize
namespace SystemSecure {
    /* Remove a dir and is file recursive is write permission */

    bool RM_dir_local(const QString &dirName);
    QString bytesToSize(const qint64 size);
    qint64 FreespaceonDir(const QString selectdir);

    static inline QString freespaceonHome() {
        //// return GBxxx...
        return bytesToSize(FreespaceonDir(QDir::homePath()));
    }
}



#endif	/* KZIP_H */
