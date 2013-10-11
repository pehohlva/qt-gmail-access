/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "longstring_p.h"
#include "qmaillog.h"
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QtDebug>

#ifndef USE_FANCY_MATCH_ALGORITHM
#include <ctype.h>
#endif

// LongString: A string/array class for processing IETF documents such as
// RFC(2)822 messages in a memory efficient method.

// The LongString string/array class implemented in this file provides 2
// primary benefits over the QString/QByteArray string/array classes namely:
//
// 1) Inbuilt support for mmap'ing a file so that a string can parsed without
//    requiring all bytes of that string to be loaded into physical memory.
//
// 2) left, mid and right methods that don't create deep copies of the
//    string data, this is achieved by using ref counting and
//    QByteArray::fromRawData
//
// Normal QByteArray methods can be used on LongStrings by utilizing the
// LongString::toQByteArray method.
//
// Known Limitations:
//
// 1) The internal representation is 8bit ascii, which is fine for 7bit ascii
//    email messages.
//
// 2) The underlying data is treated as read only.
//
// 3) mmap may not be supported on non *nix platforms.
//
// Additionally LongString provides a case insensitive indexOf method, this
// is useful as email fields/tokens (From, Date, Subject etc) are case
// insensitive.

#ifdef USE_FANCY_MATCH_ALGORITHM
#define REHASH(a) \
    if (ol_minus_1 < sizeof(uint) * CHAR_BIT) \
        hashHaystack -= (a) << ol_minus_1; \
    hashHaystack <<= 1
#endif

static int insensitiveIndexOf(const QByteArray& target, const QByteArray &source, int from, int off, int len) 
{
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

    while (it < end)
    {
        if (toupper(*it++) == toupper(*matchBegin))
        {
            const char* restart = it;

            // See if the remainder matches
            const char* searchIt = it;
            const char* matchIt = matchBegin + 1;

            do 
            {
                if (matchIt == matchEnd)
                    return ((it - 1) - begin);

                // We may find the next place to search in our scan
                if ((restart == it) && (*searchIt == *(it - 1)))
                    restart = searchIt;
            }
            while (toupper(*searchIt++) == toupper(*matchIt++));

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
        hashNeedle = ((hashNeedle<<1) + needle[idx]);
        hashHaystack = ((hashHaystack<<1) + haystack[idx]);
    }
    hashHaystack -= *(haystack + ol_minus_1);

    while (haystack <= end) {
        hashHaystack += *(haystack + ol_minus_1);
        if (hashHaystack == hashNeedle  && *needle == *haystack
             && strncasecmp(needle, haystack, ol) == 0)
        {
            return haystack - source.data();
        }
        REHASH(*haystack);
        ++haystack;
    }
    return -1;
#endif
}


class LongStringFileMapping
{
public:
    LongStringFileMapping();
    LongStringFileMapping(const QString& name);
    ~LongStringFileMapping();

    const QString &fileName() const { return filename; }
    int length() const { return len; }
    bool mapped() const { return (buffer != 0); }

    const QByteArray toQByteArray() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    void init();
    void map() const;

    QString filename;
    mutable const char* buffer;
    int len;

    // We need to keep these in an external map, because QFile is noncopyable
    struct QFileMapping
    {
        QFileMapping() : file(0), mapping(0), refCount(0), mapCount(0), size(0) {}

        QFile* file;
        char* mapping;
        int refCount;
        int mapCount;
        qint64 size;
    };

    static QMap<QString, QFileMapping> fileMap;
};

QMap<QString, LongStringFileMapping::QFileMapping> LongStringFileMapping::fileMap;

template <typename Stream> 
Stream& operator<<(Stream &stream, const LongStringFileMapping& mapping) { mapping.serialize(stream); return stream; }

template <typename Stream> 
Stream& operator>>(Stream &stream, LongStringFileMapping& mapping) { mapping.deserialize(stream); return stream; }

LongStringFileMapping::LongStringFileMapping()
    : buffer(0),
      len(0)
{
}

LongStringFileMapping::LongStringFileMapping(const QString& name)
    : filename(name),
      buffer(0),
      len(0)
{
    init();
}

LongStringFileMapping::~LongStringFileMapping()
{
    if (!filename.isEmpty()) {
        QMap<QString, QFileMapping>::iterator it = fileMap.find(filename);
        if (it == fileMap.end()) {
            qWarning() << "Unable to find mapped file:" << filename;
        } else {
            QFileMapping& fileMapping(it.value());

            if (fileMapping.refCount > 1) {
                fileMapping.refCount -= 1;

                // See if we're the last user with a mapping
                if (mapped() && (fileMapping.mapCount > 0)) {
                    fileMapping.mapCount -= 1;
                    if (fileMapping.mapCount == 0) {
                        // Unmap this file
                        if (fileMapping.file->unmap(reinterpret_cast<uchar*>(fileMapping.mapping))) {
                            fileMapping.mapping = 0;
                        } else {
                            qWarning() << "Unable to unmap file:" << filename;
                        }
                    }
                } 
            } else {
                // We're the last user - delete the file
                delete fileMapping.file;
                fileMap.erase(it);
            }
        }
    }
}

void LongStringFileMapping::init()
{
    if (!filename.isEmpty()) {
        QMap<QString, QFileMapping>::iterator it = fileMap.find(filename);
        if (it == fileMap.end()) {
            // This file is not referenced yet
            QFileInfo fi(filename);
            if (fi.exists() && fi.isFile() && fi.isReadable()) {
                filename = fi.absoluteFilePath();

                if (fi.size() > 0) {
                    QFileMapping fileMapping;

                    fileMapping.file = new QFile(filename);
                    fileMapping.size = fi.size();
                    it = fileMap.insert(filename, fileMapping);
                }
            }
        }

        if (it != fileMap.end()) {
            len = it.value().size;
            it.value().refCount += 1;
        }
    }
}

void LongStringFileMapping::map() const
{
    if ((len > 0) && !filename.isEmpty()) {
        QMap<QString, QFileMapping>::iterator it = fileMap.find(filename);
        if (it != fileMap.end()) {
            QFileMapping &fileMapping(it.value());

            if (fileMapping.mapping == 0) {
                if (fileMapping.file->open(QIODevice::ReadOnly)) {
                    fileMapping.mapping = reinterpret_cast<char*>(fileMapping.file->map(0, fileMapping.size));
                    fileMapping.file->close();

                    if (!fileMapping.mapping) {
                        qWarning() << "Unable to map file:" << filename;
                    }
                } else {
                    qWarning() << "Unable to open file for mapping:" << filename;
                }
            }

            buffer = fileMapping.mapping;
            fileMapping.mapCount += 1;
        }
    }
}

const QByteArray LongStringFileMapping::toQByteArray() const
{
    if (!mapped())
        map();

    // Does not create a copy:
    return QByteArray::fromRawData(buffer, len);
}

template <typename Stream> 
void LongStringFileMapping::serialize(Stream &stream) const
{
    stream << filename;
}

template <typename Stream> 
void LongStringFileMapping::deserialize(Stream &stream)
{
    stream >> filename;
    init();
}


class LongStringPrivate
{
public:
    LongStringPrivate();
    LongStringPrivate(const QByteArray& ba);
    LongStringPrivate(const QString& filename);
    LongStringPrivate(const LongStringPrivate& other);
    ~LongStringPrivate();

    const LongStringPrivate &operator=(const LongStringPrivate &);

    QString fileName() const;
    int length() const;
    bool isEmpty() const;

    int indexOf(const QByteArray &target, int from) const;

    void midAdjust(int i, int len);
    void leftAdjust(int i);
    void rightAdjust(int i);

    const QByteArray toQByteArray() const;

    QDataStream* dataStream() const;
    QTextStream* textStream() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    mutable LongStringFileMapping* mapping;
    mutable QByteArray data;
    int offset;
    int len;
};

template <typename Stream> 
Stream& operator<<(Stream &stream, const LongStringPrivate& ls) { ls.serialize(stream); return stream; }

template <typename Stream> 
Stream& operator>>(Stream &stream, LongStringPrivate& ls) { ls.deserialize(stream); return stream; }

LongStringPrivate::LongStringPrivate()
    : mapping(0),
      offset(0), 
      len(0) 
{
}

LongStringPrivate::LongStringPrivate(const QByteArray& ba)
    : mapping(0),
      data(ba),
      offset(0), 
      len(data.length()) 
{
}

LongStringPrivate::LongStringPrivate(const QString& filename)
    : mapping(new LongStringFileMapping(filename)),
      offset(0), 
      len(mapping->length())
{
}

LongStringPrivate::LongStringPrivate(const LongStringPrivate &other)
    : mapping(0),
      offset(0),
      len(0)
{
    this->operator=(other);
}

LongStringPrivate::~LongStringPrivate()
{
    delete mapping;
}

const LongStringPrivate &LongStringPrivate::operator=(const LongStringPrivate &other)
{
    if (&other != this) {
        delete mapping;

        mapping = (other.mapping ? new LongStringFileMapping(other.mapping->fileName()) : 0);
        data = (other.mapping ? QByteArray() : other.data);
        offset = other.offset;
        len = other.len;
    }

    return *this;
}

QString LongStringPrivate::fileName() const
{
    if (mapping) {
        return mapping->fileName();
    }

    return QString();
}

int LongStringPrivate::length() const
{
    return len;
}

bool LongStringPrivate::isEmpty() const
{
    return (len == 0);
}

int LongStringPrivate::indexOf(const QByteArray &target, int from) const
{
    if (mapping) {
        return insensitiveIndexOf(target, mapping->toQByteArray(), from, offset, len);
    }
    if (!data.isEmpty()) {
        return insensitiveIndexOf(target, data, from, offset, len);
    }

    return -1;
}

void LongStringPrivate::midAdjust(int i, int size)
{
    i = qMax(i, 0);
    if (i > len) {
        len = 0;
    } else {
        int remainder = len - i;
        if (size < 0 || size > remainder)
            size = remainder;

        offset += i;
        len = size;
    }
}

void LongStringPrivate::leftAdjust(int size)
{
    if (size < 0 || size > len)
        size = len;

    len = size;
}

void LongStringPrivate::rightAdjust(int size)
{
    if (size < 0 || size > len)
        size = len;

    offset = (len - size) + offset;
    len = size;
}

const QByteArray LongStringPrivate::toQByteArray() const
{
    if (mapping) {
        // Does not copy:
        return QByteArray::fromRawData(mapping->toQByteArray().constData() + offset, len);
    }
    if (!data.isEmpty()) {
        return QByteArray::fromRawData(data.constData() + offset, len);
    }

    return QByteArray();
}

QDataStream* LongStringPrivate::dataStream() const
{
    // This is safe because QByteArray has shared implementation objects:
    const QByteArray input = toQByteArray();
    return new QDataStream(input);
}

QTextStream* LongStringPrivate::textStream() const
{
    const QByteArray input = toQByteArray();
    return new QTextStream(input);
}

template <typename Stream> 
void LongStringPrivate::serialize(Stream &stream) const
{
    bool usesMapping(mapping != 0);

    stream << usesMapping;
    if (usesMapping) {
        stream << *mapping;
    } else {
        stream << data;
    }
    stream << offset;
    stream << len;
}

template <typename Stream> 
void LongStringPrivate::deserialize(Stream &stream)
{
    bool usesMapping;

    stream >> usesMapping;
    if (usesMapping) {
        mapping = new LongStringFileMapping();
        stream >> *mapping;
    } else {
        stream >> data;
    }
    stream >> offset;
    stream >> len;
}


LongString::LongString()
    : d(new LongStringPrivate())
{
}

LongString::LongString(const LongString &other)
    : d(new LongStringPrivate(*other.d))
{
}

LongString::LongString(const QByteArray &ba)
    : d(new LongStringPrivate(ba))
{
}

LongString::LongString(const QString &fileName)
    : d(new LongStringPrivate(fileName))
{
}

LongString::~LongString()
{
    delete d;
}

LongString &LongString::operator=(const LongString &other)
{
    if (&other != this) {
        delete d;
        d = new LongStringPrivate(*other.d);
    }

    return *this;
}

int LongString::length() const
{
    return d->length();
}

bool LongString::isEmpty() const
{
    return d->isEmpty();
}

void LongString::close()
{
    QString filename(d->fileName());
    delete d;

    if (filename.isEmpty()) {
        d = new LongStringPrivate();
    } else {
        d = new LongStringPrivate(filename);
    }
}

int LongString::indexOf(const QByteArray &target, int from) const
{
    return d->indexOf(target, from);
}

LongString LongString::mid(int i, int len) const
{
    LongString copy(*this);
    copy.d->midAdjust(i, len);
    return copy;
}

LongString LongString::left(int len) const
{
    LongString copy(*this);
    copy.d->leftAdjust(len);
    return copy;
}

LongString LongString::right(int len) const
{
    LongString copy(*this);
    copy.d->rightAdjust(len);
    return copy;
}

const QByteArray LongString::toQByteArray() const
{
    return d->toQByteArray();
}

QDataStream* LongString::dataStream() const
{
    return d->dataStream();
}

QTextStream* LongString::textStream() const
{
    return d->textStream();
}

template <typename Stream> 
void LongString::serialize(Stream &stream) const
{
    d->serialize(stream);
}

template <typename Stream> 
void LongString::deserialize(Stream &stream)
{
    d->deserialize(stream);
}


// We need to instantiate serialization functions for QDataStream
template void LongString::serialize<QDataStream>(QDataStream&) const;
template void LongString::deserialize<QDataStream>(QDataStream&);

