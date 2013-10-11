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

#include "qmailnamespace.h"
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QDir>
#include <QSqlDatabase>
#include <QtDebug>
#include <QMutex>
#include <QRegExp>
#include <stdio.h>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

static const char* QMF_DATA_ENV="QMF_DATA";
static const char* QMF_PLUGINS_ENV="QMF_PLUGINS";
static const char* QMF_SERVER_ENV="QMF_SERVER";
static const char* QMF_SETTINGS_ENV="QMF_SETTINGS";

/*!
    \namespace QMail

    \brief The QMail namespace contains miscellaneous functionality used by the Messaging framework.
*/

/*!
    \fn StringType QMail::unquoteString(const StringType& src)

    If \a src has double-quote as the first and last characters, return the string between those characters;
    otherwise, return the original string.
*/

/*!
    \fn StringType QMail::quoteString(const StringType& src)

    Returns \a src surrounded by double-quotes, which are added if not already present.
*/

#ifdef Q_OS_WIN
static QMap<int, HANDLE> lockedFiles;
#endif

/*!
    Convenience function that attempts to obtain a lock on a file with name \a lockFile.
    It is not necessary to create \a lockFile as this file is created temporarily.

    Returns the id of the lockFile if successful or \c -1 for failure.

    \sa QMail::fileUnlock()
*/
int QMail::fileLock(const QString& lockFile)
{
    QString path = QDir::tempPath() + "/" + lockFile;

#ifdef Q_OS_WIN
    static int lockedCount = 0;

	if (!QFile::exists(path)) {
		QFile file(path);
		file.open(QIODevice::WriteOnly);
		file.close();
	}

    HANDLE handle = ::CreateFile(reinterpret_cast<const wchar_t*>(path.utf16()),
                                 GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        qWarning() << "Unable to open file for locking:" << path;
    } else {
        if (::LockFile(handle, 0, 0, 1, 0) == FALSE) {
            qWarning() << "Unable to lock file:" << path;
        } else {
            ++lockedCount;
            lockedFiles.insert(lockedCount, handle);
            return lockedCount;
        }
    }

    return -1;
#else
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    int fdlock = -1;
    if((fdlock = ::open(path.toLatin1(), O_WRONLY|O_CREAT, 0666)) == -1)
        return -1;

    if(::fcntl(fdlock, F_SETLK, &fl) == -1)
        return -1;

    return fdlock;
#endif
}

/*!
    Convenience function that attempts to unlock the file with identifier \a id that was locked by \c QMail::fileLock.

    Returns \c true for success or \c false otherwise.

    \sa QMail::fileLock()
*/
bool QMail::fileUnlock(int id)
{
#ifdef Q_OS_WIN
    QMap<int, HANDLE>::iterator it = lockedFiles.find(id);
    if (it != lockedFiles.end()) {
        if (::UnlockFile(it.value(), 0, 0, 1, 0) == FALSE) {
            qWarning() << "Unable to unlock file:" << lastSystemErrorMessage();
        } else {
            if (::CloseHandle(it.value()) == FALSE) {
                qWarning() << "Unable to close handle:" << lastSystemErrorMessage();
            }

            lockedFiles.erase(it);
            return true;
        }
    }

    return false;
#else
    struct flock fl;

    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    int result = -1;

    if((result = ::fcntl(id,F_SETLK, &fl)) == -1)
        return false;

    if((result = ::close(id)) == -1)
        return false;

    return true;
#endif
}

/*!
    Returns the path to where the Messaging framework will store its data files.
*/
QString QMail::dataPath()
{
    static QString dataEnv(qgetenv(QMF_DATA_ENV));
    if(!dataEnv.isEmpty())
        return dataEnv + "/";
    //default to ~/.qmf if not env set
    return QDir::homePath() + "/.qmf/";
}

/*!
    Returns the path to where the Messaging framework will store its temporary files.
*/
QString QMail::tempPath()
{
    return QDir::tempPath();
}

/*!
    Returns the path to where the Messaging framework will look for its plugin directories
*/
QString QMail::pluginsPath()
{
    static QString pluginsEnv(qgetenv(QMF_PLUGINS_ENV));
    if(!pluginsEnv.isEmpty())
        return pluginsEnv + "/";
    //default to "." if no env set
    return pluginsEnv;
}

/*!
    Returns the path to where the Messaging framework will search for SSL certificates.
*/
QString QMail::sslCertsPath()
{
    return "/etc/ssl/certs/";
}

/*!
    Returns the path to where the Messaging framework will invoke the messageserver process.
*/
QString QMail::messageServerPath()
{
    static QString serverEnv(qgetenv(QMF_SERVER_ENV));
    if(!serverEnv.isEmpty())
        return serverEnv + "/";
    return QApplication::applicationDirPath() + "/";
}

/*!
    Returns the path to where the Messaging framework will search for settings information.
*/
QString QMail::messageSettingsPath()
{
    static QString settingsEnv(qgetenv(QMF_SETTINGS_ENV));
    if(!settingsEnv.isEmpty())
        return settingsEnv + "/";
    return QApplication::applicationDirPath() + "/";
}

/*!
    Returns the database where the Messaging framework will store its message meta-data. 
    If the database does not exist, it is created.
*/
QSqlDatabase QMail::createDatabase()
{
    static bool init = false;
    QSqlDatabase db;
    if(!init)
    {
        db = QSqlDatabase::addDatabase("QSQLITE");
        QDir dp(dataPath());
        if(!dp.exists())
            if(!dp.mkpath(dataPath()))
                qCritical() << "Cannot create data path";
        db.setDatabaseName(dataPath() + "qmailstore.db");
        if(!db.open())
            qCritical() << "Cannot open database";
        else
            init = true;
    }
    return db;
}

/*!
    \internal
    Returns the next word, given the input and starting position.
*/
static QString nextString( const char *line, int& posn )
{
    if ( line[posn] == '\0' )
        return QString::null;
    int end = posn;
    char ch;
    for (;;) {
        ch = line[end];
        if ( ch == '\0' || ch == ' ' || ch == '\t' ||
             ch == '\r' || ch == '\n' ) {
            break;
        }
        ++end;
    }
    const char *result = line + posn;
    int resultLen = end - posn;
    for (;;) {
        ch = line[end];
        if ( ch == '\0' )
            break;
        if ( ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n' )
            break;
        ++end;
    }
    posn = end;
    return QString::fromLocal8Bit(result, resultLen);
}

typedef QHash<QString, QString> typeForType;
Q_GLOBAL_STATIC(typeForType, typeFor);
typedef QHash<QString, QStringList> extForType;
Q_GLOBAL_STATIC(extForType, extFor);

/*!
    \internal
    Loads the mime type to extensions mapping
*/
static void loadExtensions()
{
    QMutex mutex;
    mutex.lock();
    static bool loaded = false;

    if(loaded)
    {
        mutex.unlock();
        return;
    }

    QFile file(":qtopiamail/mime.types");
    if ( file.open(QIODevice::ReadOnly) ) {
        char line[1024];

        while (file.readLine(line, sizeof(line)) > 0) {
            if (line[0] == '\0' || line[0] == '#')
                continue;
            int posn = 0;
            QString id = nextString(line, posn);
            if ( id.isEmpty() )
                continue;
            id = id.toLower();

            QStringList exts = extFor()->value(id);

            for( QString ext = nextString( line, posn ); !ext.isEmpty(); ext = nextString(line, posn).toLower() )
            {
                if( !exts.contains( ext ) )
                {
                    exts.append( ext );

                    typeFor()->insert(ext, id);
                }
            }
            (*extFor())[ id ] = exts;
        }
        loaded = true;
    }
    mutex.unlock();
}

/*!
    Returns the string mime type based on the filename \a filename.
*/
QString QMail::mimeTypeFromFileName(const QString& filename)
{
    if (filename.isEmpty())
        return QString();

    loadExtensions();

    // do a case insensitive search for a known mime type.
    QString lwrExtOrId = filename.toLower();
    QHash<QString,QStringList>::const_iterator it = extFor()->find(lwrExtOrId);
    if (it != extFor()->end()) {
        return lwrExtOrId;
    }

    // either it doesnt have exactly one mime-separator, or it has
    // a path separator at the beginning
    QString mime_sep = QLatin1String("/");
    bool doesntLookLikeMimeString = (filename.count(mime_sep) != 1) || (filename[0] == QDir::separator());

    if (doesntLookLikeMimeString || QFile::exists(filename)) {
        int dot = filename.lastIndexOf('.');
        QString ext = dot >= 0 ? filename.mid(dot+1) : filename;

        QHash<QString,QString>::const_iterator it = typeFor()->find(ext.toLower());
        if (it != typeFor()->end()) {
            return *it;
        }

        const char elfMagic[] = { '\177', 'E', 'L', 'F', '\0' };
        QFile ef(filename);
        if (ef.exists() && (ef.size() > 5) && ef.open(QIODevice::ReadOnly) && (ef.peek(5) == elfMagic)) { // try to find from magic
            return QLatin1String("application/x-executable");  // could be a shared library or an exe
        } else {
            return QLatin1String("application/octet-stream");
        }
    }

    // could be something like application/vnd.oma.rights+object
    return lwrExtOrId;
}

/*!
    Returns a list of valid file extensions for the mime type string \a mimeType
    or an empty list if the mime type is unrecognized.
*/
QStringList QMail::extensionsForMimeType(const QString& mimeType)
{
    loadExtensions();
    return extFor()->value(mimeType);
}

/*!
    Suspends the current process for \a usecs microseconds.
*/
void QMail::usleep(unsigned long usecs)
{
#ifdef Q_OS_WIN
    ::Sleep((usecs + 500) / 1000);
#else
    static const int factor(1000 * 1000);

    unsigned long seconds(usecs / factor);
    usecs = (usecs % factor);

    if (seconds) {
        ::sleep(seconds);
    }
    if (!seconds || usecs) {
        ::usleep(usecs);
    }
#endif
}

/*!
    Returns the 'base' form of \a subject, using the transformation defined by RFC5256.
    If the original subject contains any variant of the tokens "Re" or "Fwd" recognized by
    RFC5256, then \a replyOrForward will be set to true.
*/
QString QMail::baseSubject(const QString& subject, bool *replyOrForward)
{
    // Implements the conversion from subject to 'base subject' defined by RFC 5256
    int pos = 0;
    QString result(subject);

    bool repeat = false;
    do {
        repeat = false;

        // Remove any subj-trailer
        QRegExp subjTrailer("(?:"
                                "[ \\t]+"               // WSP
                            "|"
                                "(\\([Ff][Ww][Dd]\\))"    // "(fwd)"
                            ")$");
        while ((pos = subjTrailer.indexIn(result)) != -1) {
            if (!subjTrailer.cap(1).isEmpty()) {
                *replyOrForward = true;
            }
            result = result.left(pos);
        }

        bool modified = false;
        do {
            modified = false;

            // Remove any subj-leader
            QRegExp subjLeader("^(?:"
                                    "[ \\t]+"       // WSP
                               "|"
                                    "(?:\\[[^\\[\\]]*\\][ \\t]*)*"        // ( '[' 'blobchar'* ']' WSP* )*
                                    "([Rr][Ee]|[Ff][Ww][Dd]?)[ \\t]*"   // ( "Re" | "Fw" | "Fwd") WSP*
                                    "(?:\\[[^\\[\\]]*\\][ \\t]*)?"        // optional: ( '[' 'blobchar'* ']' WSP* )
                                    ":"                                 // ':'
                               ")");
            while ((pos = subjLeader.indexIn(result)) == 0) {
                if (!subjLeader.cap(1).isEmpty()) {
                    *replyOrForward = true;
                }
                result = result.mid(subjLeader.cap(0).length());
                modified = true;
            }

            // Remove subj-blob, if there would be a remainder
            QRegExp subjBlob("^(\\[[^\\[\\]]*\\][ \\t]*)");             // '[' 'blobchar'* ']' WSP*
            if ((subjBlob.indexIn(result) == 0) && (subjBlob.cap(0).length() < result.length())) {
                result = result.mid(subjBlob.cap(0).length());
                modified = true;
            }
        } while (modified);

        // Remove subj-fwd-hdr and subj-fwd-trl if both are present
        QRegExp subjFwdHdr("^\\[[Ff][Ww][Dd]:");
        QRegExp subjFwdTrl("\\]$");
        if ((subjFwdHdr.indexIn(result) == 0) && (subjFwdTrl.indexIn(result) != -1)) {
            *replyOrForward = true;
            result = result.mid(subjFwdHdr.cap(0).length(), result.length() - (subjFwdHdr.cap(0).length() + subjFwdTrl.cap(0).length()));
            repeat = true;
        }
    } while (repeat);

    return result;
}

static QString normaliseIdentifier(const QString& str)
{
    // Don't permit space, tab or quote marks
    static const QChar skip[] = { QChar(' '), QChar('\t'), QChar('"') };

    QString result;
    result.reserve(str.length());

    QString::const_iterator it = str.begin(), end = str.end();
    while (it != end) {
        if ((*it != skip[0]) && (*it != skip[1]) && (*it != skip[2])) {
            result.append(*it);
        }
        ++it;
    }

    return result;
}

/*!
    Returns the sequence of message identifiers that can be extracted from \a str.
    Message identifiers must conform to the definition given by RFC 5256.
*/
QStringList QMail::messageIdentifiers(const QString& str)
{
    QStringList result;

    QRegExp identifierPattern("("
                                "(?:[ \\t]*)"           // Optional leading whitespace
                                "[^ \\t\\<\\>@]+"       // Leading part
                                "(?:[ \\t]*)"           // Optional whitespace allowed before '@'?
                                "@"
                                "(?:[ \\t]*)"           // Optional whitespace allowed after '@'?
                                "[^ \\t\\<\\>]+"        // Trailing part
                              ")");

    // Extracts message identifiers from \a str, matching the definition used in RFC 5256
    int index = str.indexOf('<');
    if (index != -1) {
        // This may contain other information besides the IDs delimited by < and >
        do {
            // Extract only the delimited content
            if (str.indexOf(identifierPattern, index + 1) == (index + 1)) {
                result.append(normaliseIdentifier(identifierPattern.cap(1)));
                index += identifierPattern.cap(0).length();
            } else {
                index += 1;
            }

            index = str.indexOf('<', index);
        } while (index != -1);
    } else {
        // No delimiters - consider the entirety as an identifier
        if (str.indexOf(identifierPattern) != -1) {
            result.append(normaliseIdentifier(identifierPattern.cap(1)));
        }
    }

    return result;
}

/*!
    Returns the text describing the last error reported by the underlying platform.
*/
QString QMail::lastSystemErrorMessage()
{
#ifdef Q_OS_WIN
    LPVOID buffer;

    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    ::GetLastError(),
                    0,
                    reinterpret_cast<LPTSTR>(&buffer),
                    0,
                    NULL);

    QString result(QString::fromUtf16(reinterpret_cast<const ushort*>(buffer)));
    ::LocalFree(buffer);

    return result;
#else
    return QString(::strerror(errno));
#endif
}

