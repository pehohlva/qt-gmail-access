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

#include "qmailstore_p.h"
#include "locks_p.h"
#include "qmailcontentmanager.h"
#include "qmailmessageremovalrecord.h"
#include "qmailtimestamp.h"
#include "qmailnamespace.h"
#include "qmaillog.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QTextCodec>

#define Q_USE_SQLITE

// When using GCC 4.1.1 on ARM, TR1 functional cannot be included when RTTI
// is disabled, since it automatically instantiates some code using typeid().
//#include <tr1/functional>
//using std::tr1::bind;
//using std::tr1::cref;
#include "bind_p.h"

using nonstd::tr1::bind;
using nonstd::tr1::cref;


class QMailStorePrivate::Key
{
    enum Type {
        Account = 0,
        AccountSort,
        Folder,
        FolderSort,
        Message,
        MessageSort,
        Text
    };

    Type m_type;
    const void* m_key;
    const QString* m_alias;
    const QString* m_field;

    static QString s_null;

    template<typename NonKeyType>
    bool isType(NonKeyType) const { return false; }

    bool isType(QMailAccountKey*) const { return (m_type == Account); }
    bool isType(QMailAccountSortKey*) const { return (m_type == AccountSort); }
    bool isType(QMailFolderKey*) const { return (m_type == Folder); }
    bool isType(QMailFolderSortKey*) const { return (m_type == FolderSort); }
    bool isType(QMailMessageKey*) const { return (m_type == Message); }
    bool isType(QMailMessageSortKey*) const { return (m_type == MessageSort); }
    bool isType(QString*) const { return (m_type == Text); }

    const QMailAccountKey &key(QMailAccountKey*) const { return *reinterpret_cast<const QMailAccountKey*>(m_key); }
    const QMailAccountSortKey &key(QMailAccountSortKey*) const { return *reinterpret_cast<const QMailAccountSortKey*>(m_key); }
    const QMailFolderKey &key(QMailFolderKey*) const { return *reinterpret_cast<const QMailFolderKey*>(m_key); }
    const QMailFolderSortKey &key(QMailFolderSortKey*) const { return *reinterpret_cast<const QMailFolderSortKey*>(m_key); }
    const QMailMessageKey &key(QMailMessageKey*) const { return *reinterpret_cast<const QMailMessageKey*>(m_key); }
    const QMailMessageSortKey &key(QMailMessageSortKey*) const { return *reinterpret_cast<const QMailMessageSortKey*>(m_key); }
    const QString &key(QString*) const { return *m_alias; }

public:
    explicit Key(const QMailAccountKey &key, const QString &alias = QString()) : m_type(Account), m_key(&key), m_alias(&alias), m_field(0) {}
    Key(const QString &field, const QMailAccountKey &key, const QString &alias = QString()) : m_type(Account), m_key(&key), m_alias(&alias), m_field(&field) {}
    explicit Key(const QMailAccountSortKey &key, const QString &alias = QString()) : m_type(AccountSort), m_key(&key), m_alias(&alias), m_field(0) {}

    explicit Key(const QMailFolderKey &key, const QString &alias = QString()) : m_type(Folder), m_key(&key), m_alias(&alias), m_field(0) {}
    Key(const QString &field, const QMailFolderKey &key, const QString &alias = QString()) : m_type(Folder), m_key(&key), m_alias(&alias), m_field(&field) {}
    explicit Key(const QMailFolderSortKey &key, const QString &alias = QString()) : m_type(FolderSort), m_key(&key), m_alias(&alias), m_field(0) {}

    explicit Key(const QMailMessageKey &key, const QString &alias = QString()) : m_type(Message), m_key(&key), m_alias(&alias), m_field(0) {}
    Key(const QString &field, const QMailMessageKey &key, const QString &alias = QString()) : m_type(Message), m_key(&key), m_alias(&alias), m_field(&field) {}
    explicit Key(const QMailMessageSortKey &key, const QString &alias = QString()) : m_type(MessageSort), m_key(&key), m_alias(&alias), m_field(0) {}

    explicit Key(const QString &text) : m_type(Text), m_key(0), m_alias(&text) {}

    template<typename KeyType>
    bool isType() const { return isType(reinterpret_cast<KeyType*>(0)); }

    template<typename KeyType>
    const KeyType &key() const { return key(reinterpret_cast<KeyType*>(0)); }

    const QString &alias() const { return *m_alias; }

    const QString &field() const { return (m_field ? *m_field : s_null); }
};

QString QMailStorePrivate::Key::s_null;


namespace { // none of this code is externally visible:

//using namespace QMailDataComparator;
using namespace QMailKey;

// We allow queries to be specified by supplying a list of message IDs against
// which candidates will be matched; this list can become too large to be
// expressed directly in SQL.  Instead, we will build a temporary table to
// match against when required...
// The most IDs we can include in a query is currently 999; set an upper limit
// below this to allow for other variables in the same query, bearing in mind
// that there may be more than one clause containing this number of IDs in the
// same query...
const int IdLookupThreshold = 256;

// Note on retry logic - it appears that SQLite3 will return a SQLITE_BUSY error (5)
// whenever there is contention on file locks or mutexes, and that these occurrences
// are not handled by the handler installed by either sqlite3_busy_timeout or
// sqlite3_busy_handler.  Furthermore, the comments for sqlite3_step state that if
// the SQLITE_BUSY error is returned whilst in a transaction, the transaction should
// be rolled back.  Therefore, it appears that we must handle this error by retrying
// at the QMailStore level, since this is the level where we perform transactions.
const int Sqlite3BusyErrorNumber = 5;

const int Sqlite3ConstraintErrorNumber = 19;

const uint pid = static_cast<uint>(QCoreApplication::applicationPid() & 0xffffffff);

// Helper class for automatic unlocking
template<typename Mutex>
class Guard
{
    Mutex &mutex;
    bool locked;

public:
    enum { DefaultTimeout = 1000 };

    Guard(Mutex& m)
        : mutex(m),
          locked(false) 
    {
    }

    ~Guard()
    {
        unlock();
    }

    bool lock(int timeout = DefaultTimeout)
    {
        return (locked = mutex.lock(timeout));
    }

    void unlock()
    {
        if (locked) {
            mutex.unlock();
            locked = false; 
        }
    }
};

typedef Guard<ProcessMutex> MutexGuard;


QString escape(const QString &original, const QChar &escapee, const QChar &escaper = '\\')
{
    QString result(original);
    return result.replace(escapee, QString(escaper) + escapee);
}

QString unescape(const QString &original, const QChar &escapee, const QChar &escaper = '\\')
{
    QString result(original);
    return result.replace(QString(escaper) + escapee, escapee);
}

QString contentUri(const QString &scheme, const QString &identifier)
{
    if (scheme.isEmpty())
        return QString();

    // Formulate a URI from the content scheme and identifier
    return escape(scheme, ':') + ':' + escape(identifier, ':');
}

QString contentUri(const QMailMessageMetaData &message)
{
    return contentUri(message.contentScheme(), message.contentIdentifier());
}

QPair<QString, QString> extractUriElements(const QString &uri)
{
    int index = uri.indexOf(':');
    while ((index != -1) && (uri.at(index - 1) == '\\'))
        index = uri.indexOf(':', index + 1);

    return qMakePair(unescape(uri.mid(0, index), ':'), unescape(uri.mid(index + 1), ':'));
}

QString identifierValue(const QString &str)
{
    QStringList identifiers(QMail::messageIdentifiers(str));
    if (!identifiers.isEmpty()) {
        return identifiers.first();
    }

    return QString();
}

QStringList identifierValues(const QString &str)
{
    return QMail::messageIdentifiers(str);
}

template<typename ValueContainer>
class MessageValueExtractor;

// Class to extract QMailMessageMetaData properties to QVariant form
template<>
class MessageValueExtractor<QMailMessageMetaData>
{
    const QMailMessageMetaData &_data;
    
public:
    MessageValueExtractor(const QMailMessageMetaData &d) : _data(d) {}

    QVariant id() const { return _data.id().toULongLong(); }

    QVariant messageType() const { return static_cast<int>(_data.messageType()); }

    QVariant parentFolderId() const { return _data.parentFolderId().toULongLong(); }

    QVariant from() const { return _data.from().toString(); }

    QVariant to() const { return QMailAddress::toStringList(_data.to()).join(","); }

    QVariant subject() const { return _data.subject(); }

    QVariant date() const { return _data.date().toLocalTime(); }

    QVariant receivedDate() const { return _data.receivedDate().toLocalTime(); }

    // Don't record the value of the UnloadedData flag:
    QVariant status() const { return (_data.status() & ~QMailMessage::UnloadedData); }

    QVariant parentAccountId() const { return _data.parentAccountId().toULongLong(); }

    QVariant serverUid() const { return _data.serverUid(); }

    QVariant size() const { return _data.size(); }

    QVariant content() const { return static_cast<int>(_data.content()); }

    QVariant previousParentFolderId() const { return _data.previousParentFolderId().toULongLong(); }

    QVariant contentScheme() const { return _data.contentScheme(); }

    QVariant contentIdentifier() const { return _data.contentIdentifier(); }

    QVariant inResponseTo() const { return _data.inResponseTo().toULongLong(); }

    QVariant responseType() const { return static_cast<int>(_data.responseType()); }
};

// Class to extract QMailMessageMetaData properties from QVariant object
template<>
class MessageValueExtractor<QVariant>
{
    const QVariant &_value;
    
public:
    MessageValueExtractor(const QVariant &v) : _value(v) {}

    QMailMessageId id() const { return QMailMessageId(QMailStorePrivate::extractValue<quint64>(_value)); }

    QMailMessage::MessageType messageType() const { return static_cast<QMailMessage::MessageType>(QMailStorePrivate::extractValue<int>(_value)); }

    QMailFolderId parentFolderId() const { return QMailFolderId(QMailStorePrivate::extractValue<quint64>(_value)); }

    QMailAddress from() const { return QMailAddress(QMailStorePrivate::extractValue<QString>(_value)); }

    QList<QMailAddress> to() const { return QMailAddress::fromStringList(QMailStorePrivate::extractValue<QString>(_value)); }

    QString subject() const { return QMailStorePrivate::extractValue<QString>(_value); }

    QMailTimeStamp date() const { return QMailTimeStamp(QMailStorePrivate::extractValue<QDateTime>(_value)); }

    QMailTimeStamp receivedDate() const { return QMailTimeStamp(QMailStorePrivate::extractValue<QDateTime>(_value)); }

    quint64 status() const { return QMailStorePrivate::extractValue<quint64>(_value); }

    QMailAccountId parentAccountId() const { return QMailAccountId(QMailStorePrivate::extractValue<quint64>(_value)); }

    QString serverUid() const { return QMailStorePrivate::extractValue<QString>(_value); }

    int size() const { return QMailStorePrivate::extractValue<int>(_value); }

    QMailMessage::ContentType content() const { return static_cast<QMailMessage::ContentType>(QMailStorePrivate::extractValue<int>(_value)); }

    QMailFolderId previousParentFolderId() const { return QMailFolderId(QMailStorePrivate::extractValue<quint64>(_value)); }

    QString contentUri() const { return QMailStorePrivate::extractValue<QString>(_value); }

    QMailMessageId inResponseTo() const { return QMailMessageId(QMailStorePrivate::extractValue<quint64>(_value)); }

    QMailMessage::ResponseType responseType() const { return static_cast<QMailMessage::ResponseType>(QMailStorePrivate::extractValue<int>(_value)); }
};


// Properties of the mailmessages table
static QMailStorePrivate::MessagePropertyMap messagePropertyMap() 
{
    QMailStorePrivate::MessagePropertyMap map; 

    map.insert(QMailMessageKey::Id,"id");
    map.insert(QMailMessageKey::Type,"type");
    map.insert(QMailMessageKey::ParentFolderId,"parentfolderid");
    map.insert(QMailMessageKey::Sender,"sender");
    map.insert(QMailMessageKey::Recipients,"recipients");
    map.insert(QMailMessageKey::Subject,"subject");
    map.insert(QMailMessageKey::TimeStamp,"stamp");
    map.insert(QMailMessageKey::ReceptionTimeStamp,"receivedstamp");
    map.insert(QMailMessageKey::Status,"status");
    map.insert(QMailMessageKey::ParentAccountId,"parentaccountid");
    map.insert(QMailMessageKey::ServerUid,"serveruid");
    map.insert(QMailMessageKey::Size,"size");
    map.insert(QMailMessageKey::ContentType,"contenttype");
    map.insert(QMailMessageKey::PreviousParentFolderId,"previousparentfolderid");
    map.insert(QMailMessageKey::ContentScheme,"mailfile");
    map.insert(QMailMessageKey::ContentIdentifier,"mailfile");
    map.insert(QMailMessageKey::InResponseTo,"responseid");
    map.insert(QMailMessageKey::ResponseType,"responsetype");
    map.insert(QMailMessageKey::Conversation,"id");

    return map;
}

static QString messagePropertyName(QMailMessageKey::Property property)
{
    static const QMailStorePrivate::MessagePropertyMap map(messagePropertyMap());

    QMailStorePrivate::MessagePropertyMap::const_iterator it = map.find(property);
    if (it != map.end())
        return it.value();

    if ((property != QMailMessageKey::AncestorFolderIds) &&
        (property != QMailMessageKey::Custom))
        qWarning() << "Unknown message property:" << property;
    
    return QString();
}

typedef QMap<QMailAccountKey::Property, QString> AccountPropertyMap;

// Properties of the mailaccounts table
static AccountPropertyMap accountPropertyMap() 
{
    AccountPropertyMap map; 

    map.insert(QMailAccountKey::Id,"id");
    map.insert(QMailAccountKey::Name,"name");
    map.insert(QMailAccountKey::MessageType,"type");
    map.insert(QMailAccountKey::FromAddress,"emailaddress");
    map.insert(QMailAccountKey::Status,"status");

    return map;
}

static QString accountPropertyName(QMailAccountKey::Property property)
{
    static const AccountPropertyMap map(accountPropertyMap());

    AccountPropertyMap::const_iterator it = map.find(property);
    if (it != map.end())
        return it.value();

    if (property != QMailAccountKey::Custom)
        qWarning() << "Unknown account property:" << property;

    return QString();
}

typedef QMap<QMailFolderKey::Property, QString> FolderPropertyMap;

// Properties of the mailfolders table
static FolderPropertyMap folderPropertyMap() 
{
    FolderPropertyMap map; 

    map.insert(QMailFolderKey::Id,"id");
    map.insert(QMailFolderKey::Path,"name");
    map.insert(QMailFolderKey::ParentFolderId,"parentid");
    map.insert(QMailFolderKey::ParentAccountId,"parentaccountid");
    map.insert(QMailFolderKey::DisplayName,"displayname");
    map.insert(QMailFolderKey::Status,"status");
    map.insert(QMailFolderKey::ServerCount,"servercount");
    map.insert(QMailFolderKey::ServerUnreadCount,"serverunreadcount");
    map.insert(QMailFolderKey::ServerUndiscoveredCount,"serverundiscoveredcount");

    return map;
}

static QString folderPropertyName(QMailFolderKey::Property property)
{
    static const FolderPropertyMap map(folderPropertyMap());

    FolderPropertyMap::const_iterator it = map.find(property);
    if (it != map.end())
        return it.value();

    if ((property != QMailFolderKey::AncestorFolderIds) &&
        (property != QMailFolderKey::Custom))
        qWarning() << "Unknown folder property:" << property;

    return QString();
}

// Build lists of column names from property values

static QString qualifiedName(const QString &name, const QString &alias)
{
    if (alias.isEmpty())
        return name;

    return (alias + '.' + name);
}

template<typename PropertyType>
QString fieldName(PropertyType property, const QString &alias);

template<>
QString fieldName<QMailMessageKey::Property>(QMailMessageKey::Property property, const QString& alias)
{
    return qualifiedName(messagePropertyName(property), alias);
}

template<>
QString fieldName<QMailFolderKey::Property>(QMailFolderKey::Property property, const QString& alias)
{
    return qualifiedName(folderPropertyName(property), alias);
}

template<>
QString fieldName<QMailAccountKey::Property>(QMailAccountKey::Property property, const QString& alias)
{
    return qualifiedName(accountPropertyName(property), alias);
}

template<typename SourceType, typename TargetType>
TargetType matchingProperty(SourceType source);

static QMap<QMailMessageSortKey::Property, QMailMessageKey::Property> messageSortMapInit()
{
    QMap<QMailMessageSortKey::Property, QMailMessageKey::Property> map;

    // Provide a mapping of sort key properties to the corresponding filter key
    map.insert(QMailMessageSortKey::Id, QMailMessageKey::Id);
    map.insert(QMailMessageSortKey::Type, QMailMessageKey::Type);
    map.insert(QMailMessageSortKey::ParentFolderId, QMailMessageKey::ParentFolderId);
    map.insert(QMailMessageSortKey::Sender, QMailMessageKey::Sender);
    map.insert(QMailMessageSortKey::Recipients, QMailMessageKey::Recipients);
    map.insert(QMailMessageSortKey::Subject, QMailMessageKey::Subject);
    map.insert(QMailMessageSortKey::TimeStamp, QMailMessageKey::TimeStamp);
    map.insert(QMailMessageSortKey::ReceptionTimeStamp, QMailMessageKey::ReceptionTimeStamp);
    map.insert(QMailMessageSortKey::Status, QMailMessageKey::Status);
    map.insert(QMailMessageSortKey::ParentAccountId, QMailMessageKey::ParentAccountId);
    map.insert(QMailMessageSortKey::ServerUid, QMailMessageKey::ServerUid);
    map.insert(QMailMessageSortKey::Size, QMailMessageKey::Size);
    map.insert(QMailMessageSortKey::ContentType, QMailMessageKey::ContentType);
    map.insert(QMailMessageSortKey::PreviousParentFolderId, QMailMessageKey::PreviousParentFolderId);

    return map;
}

template<>
QMailMessageKey::Property matchingProperty<QMailMessageSortKey::Property, QMailMessageKey::Property>(QMailMessageSortKey::Property source)
{
    static QMap<QMailMessageSortKey::Property, QMailMessageKey::Property> map(messageSortMapInit());
    return map.value(source);
}

static QMap<QMailFolderSortKey::Property, QMailFolderKey::Property> folderSortMapInit()
{
    QMap<QMailFolderSortKey::Property, QMailFolderKey::Property> map;

    // Provide a mapping of sort key properties to the corresponding filter key
    map.insert(QMailFolderSortKey::Id, QMailFolderKey::Id);
    map.insert(QMailFolderSortKey::Path, QMailFolderKey::Path);
    map.insert(QMailFolderSortKey::ParentFolderId, QMailFolderKey::ParentFolderId);
    map.insert(QMailFolderSortKey::ParentAccountId, QMailFolderKey::ParentAccountId);
    map.insert(QMailFolderSortKey::DisplayName, QMailFolderKey::DisplayName);
    map.insert(QMailFolderSortKey::Status, QMailFolderKey::Status);
    map.insert(QMailFolderSortKey::ServerCount, QMailFolderKey::ServerCount);
    map.insert(QMailFolderSortKey::ServerUnreadCount, QMailFolderKey::ServerUnreadCount);
    map.insert(QMailFolderSortKey::ServerUndiscoveredCount, QMailFolderKey::ServerUndiscoveredCount);

    return map;
}

template<>
QMailFolderKey::Property matchingProperty<QMailFolderSortKey::Property, QMailFolderKey::Property>(QMailFolderSortKey::Property source)
{
    static QMap<QMailFolderSortKey::Property, QMailFolderKey::Property> map(folderSortMapInit());
    return map.value(source);
}

static QMap<QMailAccountSortKey::Property, QMailAccountKey::Property> accountSortMapInit()
{
    QMap<QMailAccountSortKey::Property, QMailAccountKey::Property> map;

    // Provide a mapping of sort key properties to the corresponding filter key
    map.insert(QMailAccountSortKey::Id, QMailAccountKey::Id);
    map.insert(QMailAccountSortKey::Name, QMailAccountKey::Name);
    map.insert(QMailAccountSortKey::MessageType, QMailAccountKey::MessageType);
    map.insert(QMailAccountSortKey::Status, QMailAccountKey::Status);

    return map;
}

template<>
QMailAccountKey::Property matchingProperty<QMailAccountSortKey::Property, QMailAccountKey::Property>(QMailAccountSortKey::Property source)
{
    static QMap<QMailAccountSortKey::Property, QMailAccountKey::Property> map(accountSortMapInit());
    return map.value(source);
}

template<>
QString fieldName<QMailMessageSortKey::Property>(QMailMessageSortKey::Property property, const QString &alias)
{
    return qualifiedName(messagePropertyName(matchingProperty<QMailMessageSortKey::Property, QMailMessageKey::Property>(property)), alias);
}

template<>
QString fieldName<QMailFolderSortKey::Property>(QMailFolderSortKey::Property property, const QString &alias)
{
    return qualifiedName(folderPropertyName(matchingProperty<QMailFolderSortKey::Property, QMailFolderKey::Property>(property)), alias);
}

template<>
QString fieldName<QMailAccountSortKey::Property>(QMailAccountSortKey::Property property, const QString &alias)
{
    return qualifiedName(accountPropertyName(matchingProperty<QMailAccountSortKey::Property, QMailAccountKey::Property>(property)), alias);
}

template<typename PropertyType>
QString fieldNames(const QList<PropertyType> &properties, const QString &separator, const QString &alias)
{
    QStringList fields;
    foreach (const PropertyType &property, properties)
        fields.append(fieldName(property, alias));

    return fields.join(separator);
}

template<typename ArgumentType>
void appendWhereValues(const ArgumentType &a, QVariantList &values);

template<typename KeyType>
QVariantList whereClauseValues(const KeyType& key)
{
    QVariantList values;

    foreach (const typename KeyType::ArgumentType& a, key.arguments())
        ::appendWhereValues(a, values);

    foreach (const KeyType& subkey, key.subKeys())
        values += ::whereClauseValues<KeyType>(subkey);

    return values;
}

template <typename Key, typename Argument = typename Key::ArgumentType>
class ArgumentExtractorBase
{
protected:
    const Argument &arg;
    
    ArgumentExtractorBase(const Argument &a) : arg(a) {}

    QString minimalString(const QString &s) const
    {
        // If the argument is a phone number, ensure it is in minimal form
        QMailAddress address(s);
        if (address.isPhoneNumber()) {
            QString minimal(address.minimalPhoneNumber());

            // Rather than compare exact numbers, we will only use the trailing
            // digits to compare phone numbers - otherwise, slightly different 
            // forms of the same number will not be matched
            static const int significantDigits = 8;

            int extraneous = minimal.length() - significantDigits;
            if (extraneous > 0)
                minimal.remove(0, extraneous);

            return minimal;
        }

        return s;
    }

    QString submatchString(const QString &s, bool valueMinimalised) const
    {
        if (!s.isEmpty()) {
            // Delimit data for sql "LIKE" operator
            if (((arg.op == Includes) || (arg.op == Excludes)) || (((arg.op == Equal) || (arg.op == NotEqual)) && valueMinimalised))
                return QString("%" + s + "%");
        } else if ((arg.op == Includes) || (arg.op == Excludes)) {
            return QString("%");
        }

        return s;
    }

    QString addressStringValue() const
    {
        return submatchString(minimalString(QMailStorePrivate::extractValue<QString>(arg.valueList.first())), true);
    }

    QString stringValue() const
    {
        return submatchString(QMailStorePrivate::extractValue<QString>(arg.valueList.first()), false);
    }

    QVariantList stringValues() const
    {
        QVariantList values;

        if (arg.valueList.count() == 1) {
            values.append(stringValue());
        } else {
            // Includes/Excludes is not a pattern match with multiple values
            foreach (const QVariant &item, arg.valueList)
                values.append(QMailStorePrivate::extractValue<QString>(item));
        }

        return values;
    }

    template<typename ID>
    quint64 idValue() const
    {
        return QMailStorePrivate::extractValue<ID>(arg.valueList.first()).toULongLong(); 
    }

    template<typename ClauseKey>
    QVariantList idValues() const
    {
        const QVariant& var = arg.valueList.first();

        if (qVariantCanConvert<ClauseKey>(var)) {
            return ::whereClauseValues(qVariantValue<ClauseKey>(var));
        } else {
            QVariantList values;

            foreach (const QVariant &item, arg.valueList)
                values.append(QMailStorePrivate::extractValue<typename ClauseKey::IdType>(item).toULongLong());

            return values;
        }
    }

    int intValue() const
    {
        return QMailStorePrivate::extractValue<int>(arg.valueList.first());
    }

    QVariantList intValues() const
    {
        QVariantList values;

        foreach (const QVariant &item, arg.valueList)
            values.append(QMailStorePrivate::extractValue<int>(item));

        return values;
    }

    int quint64Value() const
    {
        return QMailStorePrivate::extractValue<quint64>(arg.valueList.first());
    }

    QVariantList customValues() const
    {
        QVariantList values;

        QStringList constraints = QMailStorePrivate::extractValue<QStringList>(arg.valueList.first());
        // Field name required for existence or value test
        values.append(constraints.takeFirst());

        if (!constraints.isEmpty()) {
            // For a value test, we need the comparison value also
            values.append(submatchString(constraints.takeFirst(), false));
        }

        return values;
    }
};


template<typename PropertyType, typename BitmapType = int>
class RecordExtractorBase
{
protected:
    const QSqlRecord &record;
    const BitmapType bitmap;

    RecordExtractorBase(const QSqlRecord &r, BitmapType b = 0) : record(r), bitmap(b) {}
    virtual ~RecordExtractorBase() {}
    
    template<typename ValueType>
    ValueType value(const QString &field, const ValueType &defaultValue = ValueType()) const 
    { 
        int index(fieldIndex(field, bitmap));

        if (record.isNull(index))
            return defaultValue;
        else
            return QMailStorePrivate::extractValue<ValueType>(record.value(index), defaultValue);
    }
    
    template<typename ValueType>
    ValueType value(PropertyType p, const ValueType &defaultValue = ValueType()) const 
    { 
        return value(fieldName(p, QString()), defaultValue);
    }

    virtual int fieldIndex(const QString &field, BitmapType b) const = 0;

    int mappedFieldIndex(const QString &field, BitmapType bitmap, QMap<BitmapType, QMap<QString, int> > &fieldIndex) const
    {
        typename QMap<BitmapType, QMap<QString, int> >::iterator it = fieldIndex.find(bitmap);
        if (it == fieldIndex.end()) {
            it = fieldIndex.insert(bitmap, QMap<QString, int>());
        }

        QMap<QString, int> &fields(it.value());

        QMap<QString, int>::iterator fit = fields.find(field);
        if (fit != fields.end())
            return fit.value();

        int index = record.indexOf(field);
        fields.insert(field, index);
        return index;
    }
};


// Class to extract data from records of the mailmessages table
class MessageRecord : public RecordExtractorBase<QMailMessageKey::Property, QMailMessageKey::Properties>
{
public:
    MessageRecord(const QSqlRecord &r, QMailMessageKey::Properties props) 
        : RecordExtractorBase<QMailMessageKey::Property, QMailMessageKey::Properties>(r, props) {}

    QMailMessageId id() const { return QMailMessageId(value<quint64>(QMailMessageKey::Id)); }

    QMailMessage::MessageType messageType() const { return static_cast<QMailMessage::MessageType>(value<int>(QMailMessageKey::Type, QMailMessage::None)); }

    QMailFolderId parentFolderId() const { return QMailFolderId(value<quint64>(QMailMessageKey::ParentFolderId)); }

    QMailAddress from() const { return QMailAddress(value<QString>(QMailMessageKey::Sender)); }

    QList<QMailAddress> to() const { return QMailAddress::fromStringList(value<QString>(QMailMessageKey::Recipients)); }

    QString subject() const { return value<QString>(QMailMessageKey::Subject); }

    QMailTimeStamp date() const { return QMailTimeStamp(value<QDateTime>(QMailMessageKey::TimeStamp)); }

    QMailTimeStamp receivedDate() const { return QMailTimeStamp(value<QDateTime>(QMailMessageKey::TimeStamp)); }

    quint64 status() const { return value<quint64>(QMailMessageKey::Status, 0); }

    QMailAccountId parentAccountId() const { return QMailAccountId(value<quint64>(QMailMessageKey::ParentAccountId)); }

    QString serverUid() const { return value<QString>(QMailMessageKey::ServerUid); }

    int size() const { return value<int>(QMailMessageKey::Size); }

    QMailMessage::ContentType content() const { return static_cast<QMailMessage::ContentType>(value<int>(QMailMessageKey::ContentType, QMailMessage::UnknownContent)); }

    QMailFolderId previousParentFolderId() const { return QMailFolderId(value<quint64>(QMailMessageKey::PreviousParentFolderId)); }

    QString contentScheme() const 
    { 
        if (_uriElements.first.isNull()) 
            _uriElements = extractUriElements(value<QString>(QMailMessageKey::ContentScheme)); 

        return _uriElements.first;
    }

    QString contentIdentifier() const 
    { 
        if (_uriElements.first.isNull()) 
            _uriElements = extractUriElements(value<QString>(QMailMessageKey::ContentIdentifier)); 

        return _uriElements.second;
    }

    QMailMessageId inResponseTo() const { return QMailMessageId(value<quint64>(QMailMessageKey::InResponseTo)); }

    QMailMessage::ResponseType responseType() const { return static_cast<QMailMessage::ResponseType>(value<int>(QMailMessageKey::ResponseType, QMailMessage::NoResponse)); }

private:
    int fieldIndex(const QString &field, QMailMessageKey::Properties props) const
    {
        return mappedFieldIndex(field, props, _fieldIndex);
    }

    mutable QPair<QString, QString> _uriElements;

    static QMap<QMailMessageKey::Properties, QMap<QString, int> > _fieldIndex;
};

QMap<QMailMessageKey::Properties, QMap<QString, int> > MessageRecord::_fieldIndex;


// Class to convert QMailMessageKey argument values to SQL bind values
class MessageKeyArgumentExtractor : public ArgumentExtractorBase<QMailMessageKey>
{
public:
    MessageKeyArgumentExtractor(const QMailMessageKey::ArgumentType &a) 
        : ArgumentExtractorBase<QMailMessageKey>(a) {}

    QVariantList id() const { return idValues<QMailMessageKey>(); }

    QVariant messageType() const { return intValue(); }

    QVariantList parentFolderId() const { return idValues<QMailFolderKey>(); }

    QVariantList ancestorFolderIds() const {  return idValues<QMailFolderKey>(); }

    QVariantList sender() const { return stringValues(); }

    QVariant recipients() const { return addressStringValue(); }

    QVariantList subject() const { return stringValues(); }

    QVariant date() const { return QMailStorePrivate::extractValue<QDateTime>(arg.valueList.first()); }

    QVariant receivedDate() const { return QMailStorePrivate::extractValue<QDateTime>(arg.valueList.first()); }

    QVariant status() const
    {
        // The UnloadedData flag has no meaningful persistent value
        return (QMailStorePrivate::extractValue<quint64>(arg.valueList.first()) & ~QMailMessage::UnloadedData);
    }

    QVariantList parentAccountId() const { return idValues<QMailAccountKey>(); }

    QVariantList serverUid() const { return stringValues(); }

    QVariant size() const { return intValue(); }

    QVariantList content() const { return intValues(); }

    QVariantList previousParentFolderId() const { return idValues<QMailFolderKey>(); }

    QVariant contentScheme() const 
    { 
        // Any colons in the field will be stored in escaped format
        QString value(escape(QMailStorePrivate::extractValue<QString>(arg.valueList.first()), ':')); 

        if ((arg.op == Includes) || (arg.op == Excludes)) {
            value.prepend('%').append('%');
        } else if ((arg.op == Equal) || (arg.op == NotEqual)) {
            value.append(":%");
        }
        return value;
    }

    QVariant contentIdentifier() const 
    { 
        // Any colons in the field will be stored in escaped format
        QString value(escape(QMailStorePrivate::extractValue<QString>(arg.valueList.first()), ':')); 

        if ((arg.op == Includes) || (arg.op == Excludes)) {
            value.prepend('%').append('%');
        } else if ((arg.op == Equal) || (arg.op == NotEqual)) {
            value.prepend("%:");
        }
        return value;
    }

    QVariantList inResponseTo() const { return idValues<QMailMessageKey>(); }

    QVariantList responseType() const { return intValues(); }

    QVariantList conversation() const { return idValues<QMailMessageKey>(); }

    QVariantList custom() const { return customValues(); }
};

template<>
void appendWhereValues<QMailMessageKey::ArgumentType>(const QMailMessageKey::ArgumentType &a, QVariantList &values)
{
    const MessageKeyArgumentExtractor extractor(a);

    switch (a.property)
    { 
    case QMailMessageKey::Id:
        if (a.valueList.count() < IdLookupThreshold) {
            values += extractor.id();
        } else {
            // This value match has been replaced by a table lookup
        }
        break;

    case QMailMessageKey::Type:
        values += extractor.messageType();
        break;

    case QMailMessageKey::ParentFolderId:
        values += extractor.parentFolderId();
        break;

    case QMailMessageKey::AncestorFolderIds:
        values += extractor.ancestorFolderIds();
        break;

    case QMailMessageKey::Sender:
        values += extractor.sender();
        break;

    case QMailMessageKey::Recipients:
        values += extractor.recipients();
        break;

    case QMailMessageKey::Subject:
        values += extractor.subject();
        break;

    case QMailMessageKey::TimeStamp:
        values += extractor.date();
        break;

    case QMailMessageKey::ReceptionTimeStamp:
        values += extractor.receivedDate();
        break;

    case QMailMessageKey::Status:
        values += extractor.status();
        break;

    case QMailMessageKey::ParentAccountId:
        values += extractor.parentAccountId();
        break;

    case QMailMessageKey::ServerUid:
        if (a.valueList.count() < IdLookupThreshold) {
            values += extractor.serverUid();
        } else {
            // This value match has been replaced by a table lookup
        }
        break;

    case QMailMessageKey::Size:
        values += extractor.size();
        break;

    case QMailMessageKey::ContentType:
        values += extractor.content();
        break;

    case QMailMessageKey::PreviousParentFolderId:
        values += extractor.previousParentFolderId();
        break;

    case QMailMessageKey::ContentScheme:
        values += extractor.contentScheme();
        break;

    case QMailMessageKey::ContentIdentifier:
        values += extractor.contentIdentifier();
        break;

    case QMailMessageKey::InResponseTo:
        values += extractor.inResponseTo();
        break;

    case QMailMessageKey::ResponseType:
        values += extractor.responseType();
        break;

    case QMailMessageKey::Conversation:
        values += extractor.conversation();
        break;

    case QMailMessageKey::Custom:
        values += extractor.custom();
        break;
    }
}


// Class to extract data from records of the mailaccounts table
class AccountRecord : public RecordExtractorBase<QMailAccountKey::Property>
{
public:
    AccountRecord(const QSqlRecord &r) 
        : RecordExtractorBase<QMailAccountKey::Property>(r) {}

    QMailAccountId id() const { return QMailAccountId(value<quint64>(QMailAccountKey::Id)); }

    QString name() const { return value<QString>(QMailAccountKey::Name); }

    QMailMessage::MessageType messageType() const { return static_cast<QMailMessage::MessageType>(value<int>(QMailAccountKey::MessageType, -1)); }

    QString fromAddress() const { return value<QString>(QMailAccountKey::FromAddress); }

    quint64 status() const { return value<quint64>(QMailAccountKey::Status); }

    QString signature() const { return value<QString>("signature"); }

private:
    int fieldIndex(const QString &field, int props) const
    {
        return mappedFieldIndex(field, props, _fieldIndex);
    }

    static QMap<int, QMap<QString, int> > _fieldIndex;
};

QMap<int, QMap<QString, int> > AccountRecord::_fieldIndex;


// Class to convert QMailAccountKey argument values to SQL bind values
class AccountKeyArgumentExtractor : public ArgumentExtractorBase<QMailAccountKey>
{
public:
    AccountKeyArgumentExtractor(const QMailAccountKey::ArgumentType &a)
        : ArgumentExtractorBase<QMailAccountKey>(a) {}

    QVariantList id() const { return idValues<QMailAccountKey>(); }

    QVariantList name() const { return stringValues(); }

    QVariant messageType() const { return intValue(); }

    QVariant fromAddress() const 
    { 
        QString value(QMailStorePrivate::extractValue<QString>(arg.valueList.first()));

        // This test will be converted to a LIKE test, for all comparators
        if (arg.op == Equal || arg.op == NotEqual) {
            // Ensure exact match by testing for address delimiters
            value.prepend('<').append('>');
        }

        return value.prepend('%').append('%');
    }

    QVariant status() const { return quint64Value(); }

    QVariantList custom() const { return customValues(); }
};

template<>
void appendWhereValues<QMailAccountKey::ArgumentType>(const QMailAccountKey::ArgumentType &a, QVariantList &values)
{
    const AccountKeyArgumentExtractor extractor(a);

    switch (a.property)
    {
    case QMailAccountKey::Id:
        values += extractor.id();
        break;

    case QMailAccountKey::Name:
        values += extractor.name();
        break;

    case QMailAccountKey::MessageType:
        values += extractor.messageType();
        break;

    case QMailAccountKey::FromAddress:
        values += extractor.fromAddress();
        break;

    case QMailAccountKey::Status:
        values += extractor.status();
        break;

    case QMailAccountKey::Custom:
        values += extractor.custom();
        break;
    }
}


// Class to extract data from records of the mailfolders table
class FolderRecord : public RecordExtractorBase<QMailFolderKey::Property>
{
public:
    FolderRecord(const QSqlRecord &r)
        : RecordExtractorBase<QMailFolderKey::Property>(r) {}

    QMailFolderId id() const { return QMailFolderId(value<quint64>(QMailFolderKey::Id)); }

    QString path() const { return value<QString>(QMailFolderKey::Path); }

    QMailFolderId parentFolderId() const { return QMailFolderId(value<quint64>(QMailFolderKey::ParentFolderId)); }

    QMailAccountId parentAccountId() const { return QMailAccountId(value<quint64>(QMailFolderKey::ParentAccountId)); }

    QString displayName() const { return value<QString>(QMailFolderKey::DisplayName); }

    quint64 status() const { return value<quint64>(QMailFolderKey::Status); }

    uint serverCount() const { return value<uint>(QMailFolderKey::ServerCount); }

    uint serverUnreadCount() const { return value<uint>(QMailFolderKey::ServerUnreadCount); }

    uint serverUndiscoveredCount() const { return value<uint>(QMailFolderKey::ServerUndiscoveredCount); }

private:
    int fieldIndex(const QString &field, int props) const
    {
        return mappedFieldIndex(field, props, _fieldIndex);
    }

    static QMap<int, QMap<QString, int> > _fieldIndex;
};

QMap<int, QMap<QString, int> > FolderRecord::_fieldIndex;


// Class to convert QMailFolderKey argument values to SQL bind values
class FolderKeyArgumentExtractor : public ArgumentExtractorBase<QMailFolderKey>
{
public:
    FolderKeyArgumentExtractor(const QMailFolderKey::ArgumentType &a)
        : ArgumentExtractorBase<QMailFolderKey>(a) {}

    QVariantList id() const { return idValues<QMailFolderKey>(); }

    QVariantList path() const { return stringValues(); }

    QVariantList parentFolderId() const { return idValues<QMailFolderKey>(); }

    QVariantList ancestorFolderIds() const {  return idValues<QMailFolderKey>(); }

    QVariantList parentAccountId() const { return idValues<QMailAccountKey>(); }

    QVariantList displayName() const { return stringValues(); }

    QVariant status() const { return quint64Value(); }

    QVariant serverCount() const { return intValue(); }

    QVariant serverUnreadCount() const { return intValue(); }

    QVariant serverUndiscoveredCount() const { return intValue(); }

    QVariantList custom() const { return customValues(); }
};

template<>
void appendWhereValues<QMailFolderKey::ArgumentType>(const QMailFolderKey::ArgumentType &a, QVariantList &values)
{
    const FolderKeyArgumentExtractor extractor(a);

    switch (a.property)
    {
    case QMailFolderKey::Id:
        values += extractor.id();
        break;

    case QMailFolderKey::Path:
        values += extractor.path();
        break;

    case QMailFolderKey::ParentFolderId:
        values += extractor.parentFolderId();
        break;

    case QMailFolderKey::AncestorFolderIds:
        values += extractor.ancestorFolderIds();
        break;

    case QMailFolderKey::ParentAccountId:
        values += extractor.parentAccountId();
        break;

    case QMailFolderKey::DisplayName:
        values += extractor.displayName();
        break;

    case QMailFolderKey::Status:
        values += extractor.status();
        break;

    case QMailFolderKey::ServerCount:
        values += extractor.serverCount();
        break;

    case QMailFolderKey::ServerUnreadCount:
        values += extractor.serverUnreadCount();
        break;

    case QMailFolderKey::ServerUndiscoveredCount:
        values += extractor.serverUndiscoveredCount();
        break;

    case QMailFolderKey::Custom:
        values += extractor.custom();
        break;
    }
}


// Class to extract data from records of the deletedmessages table
class MessageRemovalRecord : public RecordExtractorBase<int>
{
public:
    MessageRemovalRecord(const QSqlRecord &r)
        : RecordExtractorBase<int>(r) {}

    quint64 id() const { return value<quint64>("id"); }

    QMailAccountId parentAccountId() const { return QMailAccountId(value<quint64>("parentaccountid")); }

    QString serverUid() const { return value<QString>("serveruid"); }

    QMailFolderId parentFolderId() const { return QMailFolderId(value<quint64>("parentfolderid")); }

private:
    int fieldIndex(const QString &field, int props) const
    {
        return mappedFieldIndex(field, props, _fieldIndex);
    }

    static QMap<int, QMap<QString, int> > _fieldIndex;
};

QMap<int, QMap<QString, int> > MessageRemovalRecord::_fieldIndex;


static QString incrementAlias(const QString &alias)
{
    QRegExp aliasPattern("([a-z]+)([0-9]+)");
    if (aliasPattern.exactMatch(alias)) {
        return aliasPattern.cap(1) + QString::number(aliasPattern.cap(2).toInt() + 1);
    }

    return QString();
}

template<typename ArgumentListType>
QString buildOrderClause(const ArgumentListType &list, const QString &alias)
{
    if (list.isEmpty())
        return QString();

    QStringList sortColumns;
    foreach (typename ArgumentListType::const_reference arg, list) {
        QString field(fieldName(arg.property, alias));
        if (arg.mask) {
            field = QString("(%1 & %2)").arg(field).arg(QString::number(arg.mask));
        }
        sortColumns.append(field + ' ' + (arg.order == Qt::AscendingOrder ? "ASC" : "DESC"));
    }

    return QString(" ORDER BY ") + sortColumns.join(",");
}


QString operatorString(QMailKey::Comparator op, bool multipleArgs = false, bool patternMatch = false, bool bitwiseMultiples = false)
{
    switch (op) 
    {
    case Equal:
        return (multipleArgs ? " IN " : (patternMatch ? " LIKE " : " = "));
        break;

    case NotEqual:
        return (multipleArgs ? " NOT IN " : (patternMatch ? " NOT LIKE " : " <> "));
        break;

    case LessThan:
        return " < ";
        break;

    case LessThanEqual:
        return " <= ";
        break;

    case GreaterThan:
        return " > ";
        break;

    case GreaterThanEqual:
        return " >= ";
        break;

    case Includes:
    case Present:
        return (multipleArgs ? " IN " : (bitwiseMultiples ? " & " : " LIKE "));
        break;

    case Excludes:
    case Absent:
        // Note: the result is not correct in the bitwiseMultiples case!
        return (multipleArgs ? " NOT IN " : (bitwiseMultiples ? " & " : " NOT LIKE "));
        break;
    }

    return QString();
}

QString combineOperatorString(QMailKey::Combiner op)
{
    switch (op) 
    {
    case And:
        return " AND ";
        break;

    case Or:
        return " OR ";
        break;

    case None:
        break;
    }

    return QString();
}

QString columnExpression(const QString &column, QMailKey::Comparator op, const QString &value, bool multipleArgs = false, bool patternMatch = false, bool bitwiseMultiples = false)
{
    QString operation(operatorString(op, multipleArgs, patternMatch, bitwiseMultiples));

    QString expression(column + operation);

    // Special case handling:
    if (bitwiseMultiples && (op == QMailKey::Excludes)) {
        if (!value.isEmpty()) {
            return "0 = (" + expression + value + ")";
        } else {
            return "0 = " + expression;
        }
    }

    return expression + value;
}

QString columnExpression(const QString &column, QMailKey::Comparator op, const QVariantList &valueList, bool patternMatch = false, bool bitwiseMultiples = false)
{
    QString value(QMailStorePrivate::expandValueList(valueList)); 

    return columnExpression(column, op, value, (valueList.count() > 1), patternMatch, bitwiseMultiples);
}

QString baseExpression(const QString &column, QMailKey::Comparator op, bool multipleArgs = false, bool patternMatch = false, bool bitwiseMultiples = false)
{
    return columnExpression(column, op, QString(), multipleArgs, patternMatch, bitwiseMultiples);
}


template<typename Key>
QString whereClauseItem(const Key &key, const typename Key::ArgumentType &arg, const QString &alias, const QString &field, const QMailStorePrivate &store);

template<>
QString whereClauseItem<QMailAccountKey>(const QMailAccountKey &, const QMailAccountKey::ArgumentType &a, const QString &alias, const QString &field, const QMailStorePrivate &store)
{
    QString item;
    {
        QTextStream q(&item);

        QString columnName;
        if (!field.isEmpty()) {
            columnName = qualifiedName(field, alias);
        } else {
            columnName = fieldName(a.property, alias);
        }

        bool bitwise((a.property == QMailAccountKey::Status) || (a.property == QMailAccountKey::MessageType));
        bool patternMatching(a.property == QMailAccountKey::FromAddress);

        QString expression = columnExpression(columnName, a.op, a.valueList, patternMatching, bitwise);
        
        switch(a.property)
        {
        case QMailAccountKey::Id:
            if (a.valueList.first().canConvert<QMailAccountKey>()) {
                QMailAccountKey subKey = a.valueList.first().value<QMailAccountKey>();
                QString nestedAlias(incrementAlias(alias));

                // Expand comparison to sub-query result
                q << baseExpression(columnName, a.op, true) << "( SELECT " << qualifiedName("id", nestedAlias) << " FROM mailaccounts " << nestedAlias;
                q << store.buildWhereClause(QMailStorePrivate::Key(subKey, nestedAlias)) << ")";
            } else {
                q << expression;
            }
            break;

        case QMailAccountKey::Custom:
            // Match on custom field
            {
                QString nestedAlias(incrementAlias(alias));

                // Is this an existence test or a value test?
                if ((a.op == QMailKey::Present) || (a.op == QMailKey::Absent)) {
                    q << qualifiedName("id", alias) << operatorString(a.op, true) << "( SELECT " << qualifiedName("id", nestedAlias);
                    q << " FROM mailaccountcustom " << nestedAlias << " WHERE name=? )";
                } else {
                    q << qualifiedName("id", alias) << " IN ( SELECT " << qualifiedName("id", nestedAlias); q << " FROM mailaccountcustom " << nestedAlias;
                    q << " WHERE " << qualifiedName("name", nestedAlias) << "=? AND " << qualifiedName("value", nestedAlias) << operatorString(a.op, false) << "? )";
                }
            }
            break;

        case QMailAccountKey::Status:
        case QMailAccountKey::MessageType:
        case QMailAccountKey::Name:
        case QMailAccountKey::FromAddress:

            q << expression;
            break;
        }
    }
    return item;
}

template<>
QString whereClauseItem<QMailMessageKey>(const QMailMessageKey &, const QMailMessageKey::ArgumentType &a, const QString &alias, const QString &field, const QMailStorePrivate &store)
{
    QString item;
    {
        QTextStream q(&item);

        QString columnName;
        if (!field.isEmpty()) {
            columnName = qualifiedName(field, alias);
        } else {
            columnName = fieldName(a.property, alias);
        }

        bool bitwise((a.property == QMailMessageKey::Type) || (a.property == QMailMessageKey::Status));
        bool patternMatching((a.property == QMailMessageKey::Sender) || (a.property == QMailMessageKey::Recipients) ||
                             (a.property == QMailMessageKey::ContentScheme) || (a.property == QMailMessageKey::ContentIdentifier));

        QString expression = columnExpression(columnName, a.op, a.valueList, patternMatching, bitwise);
        
        switch(a.property)
        {
        case QMailMessageKey::Id:
            if (a.valueList.count() >= IdLookupThreshold) {
                q << baseExpression(columnName, a.op, true) << "( SELECT id FROM " << QMailStorePrivate::temporaryTableName(a) << ")";
            } else {
                if (a.valueList.first().canConvert<QMailMessageKey>()) {
                    QMailMessageKey subKey = a.valueList.first().value<QMailMessageKey>();
                    QString nestedAlias(incrementAlias(alias));

                    // Expand comparison to sub-query result
                    q << baseExpression(columnName, a.op, true) << "( SELECT " << qualifiedName("id", nestedAlias) << " FROM mailmessages " << nestedAlias;
                    q << store.buildWhereClause(QMailStorePrivate::Key(subKey, nestedAlias)) << ")";
                } else {
                    q << expression;
                }
            }
            break;

        case QMailMessageKey::ParentFolderId:
        case QMailMessageKey::PreviousParentFolderId:
            if(a.valueList.first().canConvert<QMailFolderKey>()) {
                QMailFolderKey parentFolderKey = a.valueList.first().value<QMailFolderKey>();
                QString nestedAlias(incrementAlias(alias));

                q << baseExpression(columnName, a.op, true) << "( SELECT " << qualifiedName("id", nestedAlias) << " FROM mailfolders " << nestedAlias;
                q << store.buildWhereClause(QMailStorePrivate::Key(parentFolderKey, nestedAlias)) << ")";
            } else {
                q << expression;
            }
            break;

        case QMailMessageKey::AncestorFolderIds:
            if (a.valueList.first().canConvert<QMailFolderKey>()) {
                QMailFolderKey folderSubKey = a.valueList.first().value<QMailFolderKey>();
                QString nestedAlias(incrementAlias(alias));

                q << baseExpression(fieldName(QMailMessageKey::ParentFolderId, alias), a.op, true);
                q << "( SELECT DISTINCT descendantid FROM mailfolderlinks WHERE id IN ( SELECT " << qualifiedName("id", nestedAlias) << " FROM mailfolders" << nestedAlias;
                q << store.buildWhereClause(QMailStorePrivate::Key(folderSubKey, nestedAlias)) << ") )";
            } else {
                q << baseExpression(fieldName(QMailMessageKey::ParentFolderId, alias), a.op, true) << "( SELECT DISTINCT descendantid FROM mailfolderlinks WHERE id";
                if (a.valueList.count() > 1) {
                    q << " IN " << QMailStorePrivate::expandValueList(a.valueList) << ")";
                } else {
                    q << "=? )";
                }
            }
            break;

        case QMailMessageKey::ParentAccountId:
            if(a.valueList.first().canConvert<QMailAccountKey>()) {
                QMailAccountKey parentAccountKey = a.valueList.first().value<QMailAccountKey>();
                QString nestedAlias(incrementAlias(alias));

                q << baseExpression(columnName, a.op, true) << "( SELECT " << qualifiedName("id", nestedAlias) << " FROM mailaccounts " << nestedAlias;
                q << store.buildWhereClause(QMailStorePrivate::Key(parentAccountKey, nestedAlias)) << ")";
            } else {
                q << expression;
            }
            break;

        case QMailMessageKey::Custom:
            // Match on custom field
            {
                QString nestedAlias(incrementAlias(alias));

                // Is this an existence test or a value test?
                if ((a.op == QMailKey::Present) || (a.op == QMailKey::Absent)) {
                    q << qualifiedName("id", alias) << operatorString(a.op, true) << "( SELECT " << qualifiedName("id", nestedAlias);
                    q << " FROM mailmessagecustom " << nestedAlias << " WHERE name=? )";
                } else {
                    q << qualifiedName("id", alias) << " IN ( SELECT " << qualifiedName("id", nestedAlias); q << " FROM mailmessagecustom " << nestedAlias;
                    q << " WHERE " << qualifiedName("name", nestedAlias) << "=? AND " << qualifiedName("value", nestedAlias) << operatorString(a.op, false) << "? )";
                }
            }
            break;

        case QMailMessageKey::InResponseTo:
            if (a.valueList.first().canConvert<QMailMessageKey>()) {
                QMailMessageKey messageKey = a.valueList.first().value<QMailMessageKey>();
                QString nestedAlias(incrementAlias(alias));

                q << baseExpression(columnName, a.op, true) << "( SELECT " << qualifiedName("id", nestedAlias) << " FROM mailmessages " << nestedAlias;
                q << store.buildWhereClause(QMailStorePrivate::Key(messageKey, nestedAlias)) << ")";
            } else {
                q << expression;
            }
            break;

        case QMailMessageKey::Conversation:
            {
                // We need to do a double lookup on the mailthreadmessages table, converting message key to threads, and then back to messages...
                QString nestedAlias1(incrementAlias(alias));
                QString nestedAlias2(incrementAlias(nestedAlias1));

                q << baseExpression(columnName, Includes, true);
                q << "( SELECT " << qualifiedName("messageid", nestedAlias1) << " FROM mailthreadmessages " << nestedAlias1 << " WHERE threadid IN ";
                q << "( SELECT DISTINCT " << qualifiedName("threadid", nestedAlias2) << " FROM mailthreadmessages " << nestedAlias2 << " WHERE ";

                if (a.valueList.first().canConvert<QMailMessageKey>()) {
                    QMailMessageKey messageKey = a.valueList.first().value<QMailMessageKey>();
                    QString nestedAlias3(incrementAlias(nestedAlias2));

                    q << baseExpression(qualifiedName("messageid", nestedAlias2), Includes, true) << "( SELECT " << qualifiedName("id", nestedAlias3) << " FROM mailmessages " << nestedAlias3;
                    q << store.buildWhereClause(QMailStorePrivate::Key(messageKey, nestedAlias3)) << " )";
                } else {
                    q << columnExpression(qualifiedName("messageid", nestedAlias2), a.op, a.valueList);
                }

                q << " ) )";
            }
            break;

        case QMailMessageKey::ServerUid:
            if (a.valueList.count() >= IdLookupThreshold) {
                q << baseExpression(columnName, a.op, true) << "( SELECT id FROM " << QMailStorePrivate::temporaryTableName(a) << ")";
            } else {
                q << expression;
            }
            break;

        case QMailMessageKey::Type:
        case QMailMessageKey::Status:
        case QMailMessageKey::Sender:
        case QMailMessageKey::Recipients:
        case QMailMessageKey::Subject:
        case QMailMessageKey::TimeStamp:
        case QMailMessageKey::ReceptionTimeStamp:
        case QMailMessageKey::Size:
        case QMailMessageKey::ContentType:
        case QMailMessageKey::ContentScheme:
        case QMailMessageKey::ContentIdentifier:
        case QMailMessageKey::ResponseType:
            q << expression;
            break;     
        }
    }
    return item;
}

template<>
QString whereClauseItem<QMailFolderKey>(const QMailFolderKey &, const QMailFolderKey::ArgumentType &a, const QString &alias, const QString &field, const QMailStorePrivate &store)
{
    QString item;
    {
        QTextStream q(&item);

        QString columnName;
        if (!field.isEmpty()) {
            columnName = qualifiedName(field, alias);
        } else {
            columnName = fieldName(a.property, alias);
        }

        bool bitwise(a.property == QMailFolderKey::Status);
        QString expression = columnExpression(columnName, a.op, a.valueList, false, bitwise);
        
        switch (a.property)
        {
        case QMailFolderKey::Id:
            if (a.valueList.first().canConvert<QMailFolderKey>()) {
                QMailFolderKey subKey = a.valueList.first().value<QMailFolderKey>();
                QString nestedAlias(incrementAlias(alias));

                // Expand comparison to sub-query result
                q << baseExpression(columnName, a.op, true) << "( SELECT " << qualifiedName("id", nestedAlias) << " FROM mailfolders " << nestedAlias;
                q << store.buildWhereClause(QMailStorePrivate::Key(subKey, nestedAlias)) << ")";
            } else {
                q << expression;
            }
            break;

        case QMailFolderKey::ParentFolderId:
            if(a.valueList.first().canConvert<QMailFolderKey>()) {
                QMailFolderKey folderSubKey = a.valueList.first().value<QMailFolderKey>();
                QString nestedAlias(incrementAlias(alias));

                q << baseExpression(columnName, a.op, true) << "( SELECT " << qualifiedName("id", nestedAlias) << " FROM mailfolders " << nestedAlias;
                q << store.buildWhereClause(QMailStorePrivate::Key(folderSubKey, nestedAlias)) << ")";
            } else {
                q << expression;
            }
            break;

        case QMailFolderKey::AncestorFolderIds:
            if (a.valueList.first().canConvert<QMailFolderKey>()) {
                QMailFolderKey folderSubKey = a.valueList.first().value<QMailFolderKey>();
                QString nestedAlias(incrementAlias(alias));

                q << baseExpression(fieldName(QMailFolderKey::Id, alias), a.op, true);
                q << "( SELECT DISTINCT descendantid FROM mailfolderlinks WHERE id IN ( SELECT " << qualifiedName("id", nestedAlias) << " FROM mailfolders" << nestedAlias;
                q << store.buildWhereClause(QMailStorePrivate::Key(folderSubKey, nestedAlias)) << ") )";
            } else {
                q << baseExpression(fieldName(QMailFolderKey::Id, alias), a.op, true) << "( SELECT DISTINCT descendantid FROM mailfolderlinks WHERE id";
                if (a.valueList.count() > 1) {
                    q << " IN " << QMailStorePrivate::expandValueList(a.valueList) << ")";
                } else {
                    q << "=? )";
                }
            }
            break;

        case QMailFolderKey::ParentAccountId:
            if(a.valueList.first().canConvert<QMailAccountKey>()) {
                QMailAccountKey accountSubKey = a.valueList.first().value<QMailAccountKey>();
                QString nestedAlias(incrementAlias(alias));

                q << baseExpression(columnName, a.op, true) << "( SELECT " << qualifiedName("id", nestedAlias) << " FROM mailaccounts " << nestedAlias;
                q << store.buildWhereClause(QMailStorePrivate::Key(accountSubKey, nestedAlias)) << ")";
            } else {
                q << expression;
            }
            break;

        case QMailFolderKey::Custom:
            // Match on custom field
            {
                QString nestedAlias(incrementAlias(alias));

                // Is this an existence test or a value test?
                if ((a.op == QMailKey::Present) || (a.op == QMailKey::Absent)) {
                    q << qualifiedName("id", alias) << operatorString(a.op, true) << "( SELECT " << qualifiedName("id", nestedAlias);
                    q << " FROM mailfoldercustom " << nestedAlias << " WHERE name=? )";
                } else {
                    q << qualifiedName("id", alias) << " IN ( SELECT " << qualifiedName("id", nestedAlias); q << " FROM mailfoldercustom " << nestedAlias;
                    q << " WHERE " << qualifiedName("name", nestedAlias) << "=? AND " << qualifiedName("value", nestedAlias) << operatorString(a.op, false) << "? )";
                }
            }
            break;

        case QMailFolderKey::Status:
        case QMailFolderKey::Path:
        case QMailFolderKey::DisplayName:
        case QMailFolderKey::ServerCount:
        case QMailFolderKey::ServerUnreadCount:
        case QMailFolderKey::ServerUndiscoveredCount:

            q << expression;
            break;
        }
    }
    return item;
}

template<typename KeyType, typename ArgumentListType, typename KeyListType, typename CombineType>
QString buildWhereClause(const KeyType &key, 
                         const ArgumentListType &args, 
                         const KeyListType &subKeys, 
                         CombineType combine, 
                         bool negated, 
                         bool nested,
                         bool firstClause,
                         const QString &alias, 
                         const QString &field, 
                         const QMailStorePrivate& store)
{
    QString whereClause;
    QString logicalOpString(combineOperatorString(combine));

    if (!key.isEmpty()) {
        QTextStream s(&whereClause);

        QString op = " ";
        foreach (typename ArgumentListType::const_reference a, args) {
            s << op << whereClauseItem(key, a, alias, field, store);
            op = logicalOpString;
        }

        // subkeys
        s.flush();
        if (whereClause.isEmpty())
            op = " ";

        foreach (typename KeyListType::const_reference subkey, subKeys) {
            QString nestedWhere(store.buildWhereClause(QMailStorePrivate::Key(subkey, alias), true));
            if (!nestedWhere.isEmpty()) 
                s << op << " (" << nestedWhere << ") ";

            op = logicalOpString;
        }       
    }       

    // Finalise the where clause
    if (!whereClause.isEmpty()) {
        if (negated) {
            whereClause = " NOT (" + whereClause + ")";
        }
        if (!nested) {
            whereClause.prepend(firstClause ? " WHERE " : " AND ");
        }
    }

    return whereClause;
}

QPair<QString, qint64> tableInfo(const QString &name, qint64 version)
{
    return qMakePair(name, version);
}

QPair<quint64, QString> folderInfo(quint64 id, const QString &name)
{
    return qMakePair(id, name);
}

QMailContentManager::DurabilityRequirement durability(bool commitOnSuccess)
{
    return (commitOnSuccess ? QMailContentManager::EnsureDurability : QMailContentManager::DeferDurability);
}

} // namespace


// We need to support recursive locking, per-process
static volatile int mutexLockCount = 0;
static volatile int readLockCount = 0;

class QMailStorePrivate::Transaction
{
    QMailStorePrivate *m_d;
    bool m_initted;
    bool m_committed;

public:
    Transaction(QMailStorePrivate *);
    ~Transaction();

    bool commit();

    bool committed() const;
};

QMailStorePrivate::Transaction::Transaction(QMailStorePrivate* d)
    : m_d(d), 
      m_initted(false),
      m_committed(false)
{
    if (mutexLockCount > 0) {
        // Increase lock recursion depth
        ++mutexLockCount;
        m_initted = true;
    } else {
        // This process does not yet have a mutex lock
        if (m_d->databaseMutex().lock(10000)) {
            // Wait for any readers to complete
            if (m_d->databaseReadLock().wait(10000)) {
                if (m_d->transaction()) {
                    ++mutexLockCount;
                    m_initted = true;
                }
            } else {
                qWarning() << "Unable to wait for database read lock to reach zero!";
            }

            if (!m_initted) {
                m_d->databaseMutex().unlock();
            }
        } else {
            qWarning() << "Unable to lock database mutex for transaction!";
        }
    }
}

QMailStorePrivate::Transaction::~Transaction()
{
    if (m_initted && !m_committed) {
        m_d->rollback();

        --mutexLockCount;
        if (mutexLockCount == 0)
            m_d->databaseMutex().unlock();
    }
}

bool QMailStorePrivate::Transaction::commit()
{
    if (m_initted && !m_committed) {
        if ((m_committed = m_d->commit())) {
            --mutexLockCount;
            if (mutexLockCount == 0)
                m_d->databaseMutex().unlock();
        }
    }

    return m_committed;
}

bool QMailStorePrivate::Transaction::committed() const
{
    return m_committed;
}


class QMailStorePrivate::ReadLock
{
    QMailStorePrivate *m_d;
    bool m_locked;

public:
    ReadLock(QMailStorePrivate *);
    ~ReadLock();
};

QMailStorePrivate::ReadLock::ReadLock(QMailStorePrivate* d)
    : m_d(d),
      m_locked(false)
{
    if (readLockCount > 0) {
        // Increase lock recursion depth
        ++readLockCount;
        m_locked = true;
    } else {
        // This process does not yet have a read lock
        // Lock the mutex to ensure no writers are active or waiting (unless we have already locked it)
        if ((mutexLockCount > 0) || m_d->databaseMutex().lock(10000)) {
            m_d->databaseReadLock().lock();
            ++readLockCount;
            m_locked = true;

            if (mutexLockCount == 0)
                m_d->databaseMutex().unlock();
        } else {
            qWarning() << "Unable to lock database mutex for read lock!";
        }
    }
}

QMailStorePrivate::ReadLock::~ReadLock()
{
    if (m_locked) {
        --readLockCount;
        if (readLockCount == 0)
            m_d->databaseReadLock().unlock();
    }
}


template<typename FunctionType>
QMailStorePrivate::AttemptResult evaluate(QMailStorePrivate::WriteAccess, FunctionType func, QMailStorePrivate::Transaction &t)
{
    // Use the supplied transaction, and do not commit
    return func(t, false);
}

template<typename FunctionType>
QMailStorePrivate::AttemptResult evaluate(QMailStorePrivate::ReadAccess, FunctionType, QMailStorePrivate::Transaction &)
{
    return QMailStorePrivate::Failure;
}

template<typename FunctionType>
QMailStorePrivate::AttemptResult evaluate(QMailStorePrivate::WriteAccess, FunctionType func, const QString& description, QMailStorePrivate* d)
{
    QMailStorePrivate::Transaction t(d);

    // Perform the task and commit the transaction
    QMailStorePrivate::AttemptResult result = func(t, true);

    // Ensure that the transaction was committed
    if ((result == QMailStorePrivate::Success) && !t.committed()) {
        qMailLog(Messaging) << pid << "Failed to commit successful" << qPrintable(description) << "!";
    }

    return result;
}

template<typename FunctionType>
QMailStorePrivate::AttemptResult evaluate(QMailStorePrivate::ReadAccess, FunctionType func, const QString&, QMailStorePrivate* d)
{
    QMailStorePrivate::ReadLock l(d);

    return func(l);
}


QMailStore::ErrorCode errorType(QMailStorePrivate::ReadAccess)
{
    return QMailStore::InvalidId;
}

QMailStore::ErrorCode errorType(QMailStorePrivate::WriteAccess)
{
    return QMailStore::ConstraintFailure;
}


const QMailMessageKey::Properties &QMailStorePrivate::updatableMessageProperties()
{
    static QMailMessageKey::Properties p = QMailMessageKey::ParentFolderId |
                                           QMailMessageKey::Type |
                                           QMailMessageKey::Sender |
                                           QMailMessageKey::Recipients |
                                           QMailMessageKey::Subject |
                                           QMailMessageKey::TimeStamp |
                                           QMailMessageKey::ReceptionTimeStamp |
                                           QMailMessageKey::Status |
                                           QMailMessageKey::ParentAccountId |
                                           QMailMessageKey::ServerUid |
                                           QMailMessageKey::Size |
                                           QMailMessageKey::ContentType |
                                           QMailMessageKey::PreviousParentFolderId |
                                           QMailMessageKey::ContentScheme |
                                           QMailMessageKey::ContentIdentifier |
                                           QMailMessageKey::InResponseTo |
                                           QMailMessageKey::ResponseType;
    return p;
}

const QMailMessageKey::Properties &QMailStorePrivate::allMessageProperties()
{
    static QMailMessageKey::Properties p = QMailMessageKey::Id | updatableMessageProperties();
    return p;
}

const QMailStorePrivate::MessagePropertyMap& QMailStorePrivate::messagePropertyMap() 
{
    static const MessagePropertyMap map(::messagePropertyMap());
    return map;
}

const QMailStorePrivate::MessagePropertyList& QMailStorePrivate::messagePropertyList() 
{
    static const MessagePropertyList list(messagePropertyMap().keys());
    return list;
}

const QString &QMailStorePrivate::defaultContentScheme() 
{
    static QString scheme(QMailContentManagerFactory::defaultScheme());
    return scheme;
}

QString QMailStorePrivate::databaseIdentifier() const
{
    return database.databaseName();
}


ProcessMutex* QMailStorePrivate::contentMutex = 0;

QMailStorePrivate::QMailStorePrivate(QMailStore* parent)
    : QMailStoreImplementation(parent),
      messageCache(messageCacheSize),
      uidCache(uidCacheSize),
      folderCache(folderCacheSize),
      accountCache(accountCacheSize),
      inTransaction(false),
      lastQueryError(0),
      mutex(0),
      readLock(0)
{
    ProcessMutex creationMutex(QDir::rootPath());
    MutexGuard guard(creationMutex);
    if (guard.lock(1000)) {
        //open the database
        database = QMail::createDatabase();
    }
    mutex = new ProcessMutex(databaseIdentifier(), 1);
    readLock = new ProcessReadLock(databaseIdentifier(), 2);
    if (contentMutex == 0) {
        contentMutex = new ProcessMutex(databaseIdentifier(), 3);
    }
}

QMailStorePrivate::~QMailStorePrivate()
{
    delete mutex;
    delete readLock;
}

ProcessMutex& QMailStorePrivate::databaseMutex(void) const
{
    return *mutex;
}

ProcessReadLock& QMailStorePrivate::databaseReadLock(void) const
{
    return *readLock;
}

ProcessMutex& QMailStorePrivate::contentManagerMutex(void)
{
    return *contentMutex;
}

bool QMailStorePrivate::initStore()
{
    ProcessMutex creationMutex(QDir::rootPath());
    MutexGuard guard(creationMutex);
    if (!guard.lock(1000)) {
        return false;
    }

    if (!database.isOpen()) {
        qMailLog(Messaging) << "Unable to open database in initStore!";
        return false;
    }

    {
        Transaction t(this);

        if (!ensureVersionInfo() ||
            !setupTables(QList<TableInfo>() << tableInfo("maintenancerecord", 100)
                                            << tableInfo("mailaccounts", 106)
                                            << tableInfo("mailaccountcustom", 100)
                                            << tableInfo("mailaccountconfig", 100)
                                            << tableInfo("mailaccountfolders", 100)
                                            << tableInfo("mailfolders", 104)
                                            << tableInfo("mailfoldercustom", 100)
                                            << tableInfo("mailfolderlinks", 100)
                                            << tableInfo("mailmessages", 105)
                                            << tableInfo("mailmessagecustom", 100)
                                            << tableInfo("mailstatusflags", 101)
                                            << tableInfo("mailmessageidentifiers", 101)
                                            << tableInfo("mailsubjects", 100)
                                            << tableInfo("mailthreads", 100)
                                            << tableInfo("mailthreadsubjects", 100)
                                            << tableInfo("mailthreadmessages", 100)
                                            << tableInfo("missingancestors", 101)
                                            << tableInfo("missingmessages", 101)
                                            << tableInfo("deletedmessages", 101)
                                            << tableInfo("obsoletefiles", 100)) ||
            !setupFolders(QList<FolderInfo>() << folderInfo(QMailFolder::LocalStorageFolderId, tr("Local Storage")))) {
            return false;
        }

        if (!t.commit()) {
            qMailLog(Messaging) << "Could not commit setup operation to database";
            return false;
        }

        QMailAccount::initStore();
        QMailFolder::initStore();
        QMailMessage::initStore();
    }

#if defined(Q_USE_SQLITE)
    // default sqlite cache_size of 2000*1.5KB is too large, as we only want
    // to cache 100 metadata records 
    QSqlQuery query( database );
    query.exec(QLatin1String("PRAGMA cache_size=50"));
#endif

    if (!QMailContentManagerFactory::init()) {
        qMailLog(Messaging) << "Could not initialize content manager factory";
        return false;
    }

    if (!performMaintenance()) {
        return false;
    }

    // We are now correctly initialized
    return true;
}

void QMailStorePrivate::clearContent()
{
    // Clear all caches
    accountCache.clear();
    folderCache.clear();
    messageCache.clear();
    uidCache.clear();

    Transaction t(this);

    // Drop all data
    foreach (const QString &table, database.tables()) {
        if (table != "versioninfo") {
            QString sql("DELETE FROM %1");
            QSqlQuery query(database);
            if (!query.exec(sql.arg(table))) {
                qMailLog(Messaging) << "Failed to delete from table - query:" << sql << "- error:" << query.lastError().text();
            }
        }
    }

    if (!t.commit()) {
        qMailLog(Messaging) << "Could not commit clearContent operation to database";
    }
    
    // Remove all content
    QMailContentManagerFactory::clearContent();
}

bool QMailStorePrivate::transaction(void)
{
    if (inTransaction) {
        qMailLog(Messaging) << "(" << pid << ")" << "Transaction already exists at begin!";
        qWarning() << "Transaction already exists at begin!";
    }

    clearQueryError();

    // Ensure any outstanding temp tables are removed before we begin this transaction
    destroyTemporaryTables();

    if (!database.transaction()) {
        setQueryError(database.lastError(), "Failed to initiate transaction");
        return false;
    }

    inTransaction = true;
    return true;
}

static QString queryText(const QString &query, const QList<QVariant> &values)
{
    static const QChar marker('?');
    static const QChar quote('\'');

    QString result(query);

    QList<QVariant>::const_iterator it = values.begin(), end = values.end();
    int index = result.indexOf(marker);
    while ((index != -1) && (it != end)) {
        QString substitute((*it).toString());
        if ((*it).type() == QVariant::String)
            substitute.prepend(quote).append(quote);

        result.replace(index, 1, substitute);

        ++it;
        index = result.indexOf(marker, index + substitute.length());
    }

    return result;
}

static QString queryText(const QSqlQuery &query)
{
    // Note: we currently only handle positional parameters
    return queryText(query.lastQuery().simplified(), query.boundValues().values());
}

QSqlQuery QMailStorePrivate::prepare(const QString& sql)
{
    if (!inTransaction) {
        // Ensure any outstanding temp tables are removed before we begin this query
        destroyTemporaryTables();
    }

    clearQueryError();

    // Create any temporary tables needed for this query
    while (!requiredTableKeys.isEmpty()) {
        QPair<const QMailMessageKey::ArgumentType *, QString> key(requiredTableKeys.takeFirst());
        const QMailMessageKey::ArgumentType *arg = key.first;
        if (!temporaryTableKeys.contains(arg)) {
            QString tableName = temporaryTableName(*arg);

            {
                QSqlQuery createQuery(database);
                if (!createQuery.exec(QString("CREATE TEMP TABLE %1 ( id %2 PRIMARY KEY )").arg(tableName).arg(key.second))) { 
                    setQueryError(createQuery.lastError(), "Failed to create temporary table", queryText(createQuery));
                    qMailLog(Messaging) << "Unable to prepare query:" << sql;
                    return QSqlQuery();
                }
            }

            temporaryTableKeys.append(arg);

            QVariantList idValues;

            if (key.second == "INTEGER") {
                int type = 0;
                if (qVariantCanConvert<QMailMessageId>(arg->valueList.first())) {
                    type = 1;
                } else if (qVariantCanConvert<QMailFolderId>(arg->valueList.first())) {
                    type = 2;
                } else if (qVariantCanConvert<QMailAccountId>(arg->valueList.first())) {
                    type = 3;
                }

                // Extract the ID values to INTEGER variants
                foreach (const QVariant &var, arg->valueList) {
                    quint64 id = 0;

                    switch (type) {
                    case 1:
                        id = var.value<QMailMessageId>().toULongLong(); 
                        break;
                    case 2:
                        id = var.value<QMailFolderId>().toULongLong(); 
                        break;
                    case 3:
                        id = var.value<QMailAccountId>().toULongLong(); 
                        break;
                    default:
                        qMailLog(Messaging) << "Unable to extract ID value from valuelist!";
                        qMailLog(Messaging) << "Unable to prepare query:" << sql;
                        return QSqlQuery();
                    }

                    idValues.append(QVariant(id));
                }

                // Add the ID values to the temp table
                {
                    QSqlQuery insertQuery(database);
                    insertQuery.prepare(QString("INSERT INTO %1 VALUES (?)").arg(tableName));
                    insertQuery.addBindValue(idValues);
                    if (!insertQuery.execBatch()) { 
                        setQueryError(insertQuery.lastError(), "Failed to populate integer temporary table", queryText(insertQuery));
                        qMailLog(Messaging) << "Unable to prepare query:" << sql;
                        return QSqlQuery();
                    }
                }
            } else if (key.second == "VARCHAR") {
                foreach (const QVariant &var, arg->valueList) {
                    idValues.append(QVariant(var.value<QString>()));
                }

                {
                    QSqlQuery insertQuery(database);
                    insertQuery.prepare(QString("INSERT INTO %1 VALUES (?)").arg(tableName));
                    insertQuery.addBindValue(idValues);
                    if (!insertQuery.execBatch()) { 
                        setQueryError(insertQuery.lastError(), "Failed to populate varchar temporary table", queryText(insertQuery));
                        qMailLog(Messaging) << "Unable to prepare query:" << sql;
                        return QSqlQuery();
                    }
                }
            }
        }
    }

    QSqlQuery query(database);
    query.setForwardOnly(true);
    if (!query.prepare(sql)) {
        setQueryError(query.lastError(), "Failed to prepare query", queryText(query));
    }

    return query;
}

bool QMailStorePrivate::execute(QSqlQuery& query, bool batch)
{
    bool success = (batch ? query.execBatch() : query.exec());
    if (!success) {
        setQueryError(query.lastError(), "Failed to execute query", queryText(query));
        return false;
    }

#ifdef QMAILSTORE_LOG_SQL 
    qMailLog(Messaging) << "(" << pid << ")" << qPrintable(queryText(query));
#endif

    if (!inTransaction) {
        // We should be finished with these temporary tables
        expiredTableKeys = temporaryTableKeys;
        temporaryTableKeys.clear();
    }

    return true;
}

bool QMailStorePrivate::commit(void)
{
    if (!inTransaction) {
        qMailLog(Messaging) << "(" << pid << ")" << "Transaction does not exist at commit!";
        qWarning() << "Transaction does not exist at commit!";
    }
    
    if (!database.commit()) {
        setQueryError(database.lastError(), "Failed to commit transaction");
        return false;
    } else {
        inTransaction = false;

        // Expire any temporary tables we were using
        expiredTableKeys = temporaryTableKeys;
        temporaryTableKeys.clear();
    }

    return true;
}

void QMailStorePrivate::rollback(void)
{
    if (!inTransaction) {
        qMailLog(Messaging) << "(" << pid << ")" << "Transaction does not exist at rollback!";
        qWarning() << "Transaction does not exist at rollback!";
    }
    
    inTransaction = false;

    if (!database.rollback()) {
        setQueryError(database.lastError(), "Failed to rollback transaction");
    }
}

int QMailStorePrivate::queryError() const
{
    return lastQueryError;
}

void QMailStorePrivate::setQueryError(const QSqlError &error, const QString &description, const QString &statement)
{
    QString s;
    QTextStream ts(&s);

    lastQueryError = error.number();

    ts << qPrintable(description) << "; error:\"" << error.text() << '"';
    if (!statement.isEmpty())
        ts << "; statement:\"" << statement.simplified() << '"';

    qMailLog(Messaging) << "(" << pid << ")" << qPrintable(s);
    qWarning() << qPrintable(s);
}

void QMailStorePrivate::clearQueryError(void) 
{
    lastQueryError = QSqlError::NoError;
}

template<bool PtrSizeExceedsLongSize>
QString numericPtrValue(const void *ptr)
{
    return QString::number(reinterpret_cast<unsigned long long>(ptr), 16).rightJustified(16, '0');
}

template<>
QString numericPtrValue<false>(const void *ptr)
{
    return QString::number(reinterpret_cast<unsigned long>(ptr), 16).rightJustified(8, '0');;
}

QString QMailStorePrivate::temporaryTableName(const QMailMessageKey::ArgumentType& arg)
{
    const QMailMessageKey::ArgumentType *ptr = &arg;
    return QString("qtopiamail_idmatch_%1").arg(numericPtrValue<(sizeof(void*) > sizeof(unsigned long))>(ptr));
}

void QMailStorePrivate::createTemporaryTable(const QMailMessageKey::ArgumentType& arg, const QString &dataType) const
{
    requiredTableKeys.append(qMakePair(&arg, dataType));
}

void QMailStorePrivate::destroyTemporaryTables()
{
    while (!expiredTableKeys.isEmpty()) {
        const QMailMessageKey::ArgumentType *arg = expiredTableKeys.takeFirst();
        QString tableName = temporaryTableName(*arg);

        QSqlQuery query(database);
        if (!query.exec(QString("DROP TABLE %1").arg(tableName))) {
            QString sql = queryText(query);
            QString err = query.lastError().text();

            qMailLog(Messaging) << "(" << pid << ")" << "Failed to drop temporary table - query:" << qPrintable(sql) << "; error:" << qPrintable(err);
            qWarning() << "Failed to drop temporary table - query:" << qPrintable(sql) << "; error:" << qPrintable(err);
        }
    }
}

bool QMailStorePrivate::idValueExists(quint64 id, const QString& table)
{
    QSqlQuery query(database);
    QString sql = "SELECT id FROM " + table + " WHERE id=?";
    if(!query.prepare(sql)) {
        setQueryError(query.lastError(), "Failed to prepare idExists query", queryText(query));
        return false;
    }

    query.addBindValue(id);

    if(!query.exec()) {
        setQueryError(query.lastError(), "Failed to execute idExists query", queryText(query));
        return false;
    }

    return (query.first());
}

bool QMailStorePrivate::idExists(const QMailAccountId& id, const QString& table)
{
    return idValueExists(id.toULongLong(), (table.isEmpty() ? "mailaccounts" : table));
}

bool QMailStorePrivate::idExists(const QMailFolderId& id, const QString& table)
{
    return idValueExists(id.toULongLong(), (table.isEmpty() ? "mailfolders" : table));
}

bool QMailStorePrivate::idExists(const QMailMessageId& id, const QString& table)
{
    return idValueExists(id.toULongLong(), (table.isEmpty() ? "mailmessages" : table));
}

QMailAccount QMailStorePrivate::extractAccount(const QSqlRecord& r)
{
    const AccountRecord record(r);

    QMailAccount result;
    result.setId(record.id());
    result.setName(record.name());
    result.setMessageType(record.messageType());
    result.setStatus(record.status());
    result.setSignature(record.signature());
    result.setFromAddress(QMailAddress(record.fromAddress()));

    return result;
}

QMailFolder QMailStorePrivate::extractFolder(const QSqlRecord& r)
{
    const FolderRecord record(r);

    QMailFolder result(record.path(), record.parentFolderId(), record.parentAccountId());
    result.setId(record.id());
    result.setDisplayName(record.displayName());
    result.setStatus(record.status());
    result.setServerCount(record.serverCount());
    result.setServerUnreadCount(record.serverUnreadCount());
    result.setServerUndiscoveredCount(record.serverUndiscoveredCount());
    return result;
}

void QMailStorePrivate::extractMessageMetaData(const QSqlRecord& r,
                                               QMailMessageKey::Properties recordProperties,
                                               const QMailMessageKey::Properties& properties,
                                               QMailMessageMetaData* metaData)
{
    // Record whether we have loaded all data for this message
    bool unloadedProperties = (properties != allMessageProperties());
    if (!unloadedProperties) {
        // If there is message content, mark the object as not completely loaded
        if (!r.value("mailfile").toString().isEmpty())
            unloadedProperties = true;
    }

    // Use wrapper to extract data items
    const MessageRecord messageRecord(r, recordProperties);

    foreach (QMailMessageKey::Property p, messagePropertyList()) {
        switch (properties & p)
        {
            case QMailMessageKey::Id:
                metaData->setId(messageRecord.id());
                break;

            case QMailMessageKey::Type:
                metaData->setMessageType(messageRecord.messageType());
                break;

            case QMailMessageKey::ParentFolderId:
                metaData->setParentFolderId(messageRecord.parentFolderId());
                break;

            case QMailMessageKey::Sender:
                metaData->setFrom(messageRecord.from());
                break;

            case QMailMessageKey::Recipients:
                metaData->setTo(messageRecord.to());
                break;

            case QMailMessageKey::Subject:
                metaData->setSubject(messageRecord.subject());
                break;

            case QMailMessageKey::TimeStamp:
                metaData->setDate(messageRecord.date());
                break;

            case QMailMessageKey::ReceptionTimeStamp:
                metaData->setReceivedDate(messageRecord.receivedDate());
                break;

            case QMailMessageKey::Status:
                metaData->setStatus(messageRecord.status());
                break;

            case QMailMessageKey::ParentAccountId:
                metaData->setParentAccountId(messageRecord.parentAccountId());
                break;

            case QMailMessageKey::ServerUid:
                metaData->setServerUid(messageRecord.serverUid());
                break;

            case QMailMessageKey::Size:
                metaData->setSize(messageRecord.size());
                break;

            case QMailMessageKey::ContentType:
                metaData->setContent(messageRecord.content());
                break;

            case QMailMessageKey::PreviousParentFolderId:
                metaData->setPreviousParentFolderId(messageRecord.previousParentFolderId());
                break;

            case QMailMessageKey::ContentScheme:
                metaData->setContentScheme(messageRecord.contentScheme());
                break;

            case QMailMessageKey::ContentIdentifier:
                metaData->setContentIdentifier(messageRecord.contentIdentifier());
                break;

            case QMailMessageKey::InResponseTo:
                metaData->setInResponseTo(messageRecord.inResponseTo());
                break;

            case QMailMessageKey::ResponseType:
                metaData->setResponseType(messageRecord.responseType());
                break;
        }
    }
    
    if (unloadedProperties) {
        // This message is not completely loaded
        metaData->setStatus(QMailMessage::UnloadedData, true);
    }

    metaData->setUnmodified();
}

QMailMessageMetaData QMailStorePrivate::extractMessageMetaData(const QSqlRecord& r, QMailMessageKey::Properties recordProperties, const QMailMessageKey::Properties& properties)
{
    QMailMessageMetaData metaData;

    extractMessageMetaData(r, recordProperties, properties, &metaData);
    return metaData;
}

QMailMessageMetaData QMailStorePrivate::extractMessageMetaData(const QSqlRecord& r, const QMap<QString, QString> &customFields, const QMailMessageKey::Properties& properties)
{
    QMailMessageMetaData metaData;

    // Load the meta data items (note 'SELECT *' does not give the same result as 'SELECT expand(allMessageProperties())')
    extractMessageMetaData(r, QMailMessageKey::Properties(0), properties, &metaData);

    metaData.setCustomFields(customFields);
    metaData.setCustomFieldsModified(false);

    return metaData;
}

QMailMessage QMailStorePrivate::extractMessage(const QSqlRecord& r, const QMap<QString, QString> &customFields, const QMailMessageKey::Properties& properties)
{
    QMailMessage newMessage;

    // Load the meta data items (note 'SELECT *' does not give the same result as 'SELECT expand(allMessageProperties())')
    extractMessageMetaData(r, QMailMessageKey::Properties(0), properties, &newMessage);

    newMessage.setCustomFields(customFields);
    newMessage.setCustomFieldsModified(false);

    QString contentUri(r.value("mailfile").toString());
    if (!contentUri.isEmpty()) {
        QPair<QString, QString> elements(extractUriElements(contentUri));

        MutexGuard lock(contentManagerMutex());
        if (!lock.lock(1000)) {
            qMailLog(Messaging) << "Unable to acquire message body mutex in extractMessage!";
            return QMailMessage();
        } 

        QMailContentManager *contentManager = QMailContentManagerFactory::create(elements.first);
        if (contentManager) {
            // Load the message content (manager should be able to access the metadata also)
            QMailStore::ErrorCode code = contentManager->load(elements.second, &newMessage);
            if (code != QMailStore::NoError) {
                setLastError(code);
                qMailLog(Messaging) << "Unable to load message content:" << contentUri;
                return QMailMessage();
            }
        } else {
            qMailLog(Messaging) << "Unable to create content manager for scheme:" << elements.first;
            return QMailMessage();
        }

        // Re-load the meta data items so that they take precedence over the loaded content
        extractMessageMetaData(r, QMailMessageKey::Properties(0), properties, &newMessage);

        newMessage.setCustomFields(customFields);
        newMessage.setCustomFieldsModified(false);
    }

    return newMessage;
}

QMailMessageRemovalRecord QMailStorePrivate::extractMessageRemovalRecord(const QSqlRecord& r)
{
    const MessageRemovalRecord record(r);

    QMailMessageRemovalRecord result(record.parentAccountId(), record.serverUid(), record.parentFolderId());
    return result;
}

QString QMailStorePrivate::buildOrderClause(const Key& key) const
{
    if (key.isType<QMailMessageSortKey>()) {
        const QMailMessageSortKey &sortKey(key.key<QMailMessageSortKey>());
        return ::buildOrderClause(sortKey.arguments(), key.alias());
    } else if (key.isType<QMailFolderSortKey>()) {
        const QMailFolderSortKey &sortKey(key.key<QMailFolderSortKey>());
        return ::buildOrderClause(sortKey.arguments(), key.alias());
    } else if (key.isType<QMailAccountSortKey>()) {
        const QMailAccountSortKey &sortKey(key.key<QMailAccountSortKey>());
        return ::buildOrderClause(sortKey.arguments(), key.alias());
    } 

    return QString();
}

QString QMailStorePrivate::buildWhereClause(const Key& key, bool nested, bool firstClause) const
{
    if (key.isType<QMailMessageKey>()) {
        const QMailMessageKey &messageKey(key.key<QMailMessageKey>());

        // See if we need to create any temporary tables to use in this query
        foreach (const QMailMessageKey::ArgumentType &a, messageKey.arguments()) {
            if (a.property == QMailMessageKey::Id && a.valueList.count() >= IdLookupThreshold) {
                createTemporaryTable(a, "INTEGER");
            } else if (a.property == QMailMessageKey::ServerUid && a.valueList.count() >= IdLookupThreshold) {
                createTemporaryTable(a, "VARCHAR");
            }
        }

        return ::buildWhereClause(messageKey, messageKey.arguments(), messageKey.subKeys(), messageKey.combiner(), messageKey.isNegated(), nested, firstClause, key.alias(), key.field(), *this);
    } else if (key.isType<QMailFolderKey>()) {
        const QMailFolderKey &folderKey(key.key<QMailFolderKey>());
        return ::buildWhereClause(folderKey, folderKey.arguments(), folderKey.subKeys(), folderKey.combiner(), folderKey.isNegated(), nested, firstClause, key.alias(), key.field(), *this);
    } else if (key.isType<QMailAccountKey>()) {
        const QMailAccountKey &accountKey(key.key<QMailAccountKey>());
        return ::buildWhereClause(accountKey, accountKey.arguments(), accountKey.subKeys(), accountKey.combiner(), accountKey.isNegated(), nested, firstClause, key.alias(), key.field(), *this);
    }

    return QString();
}

QVariantList QMailStorePrivate::whereClauseValues(const Key& key) const
{
    if (key.isType<QMailMessageKey>()) {
        const QMailMessageKey &messageKey(key.key<QMailMessageKey>());
        return ::whereClauseValues(messageKey);
    } else if (key.isType<QMailFolderKey>()) {
        const QMailFolderKey &folderKey(key.key<QMailFolderKey>());
        return ::whereClauseValues(folderKey);
    } else if (key.isType<QMailAccountKey>()) {
        const QMailAccountKey &accountKey(key.key<QMailAccountKey>());
        return ::whereClauseValues(accountKey);
    } 

    return QVariantList();
}

QVariantList QMailStorePrivate::messageValues(const QMailMessageKey::Properties& prop, const QMailMessageMetaData& data)
{
    QVariantList values;

    const MessageValueExtractor<QMailMessageMetaData> extractor(data);

    // The ContentScheme and ContentIdentifier properties map to the same field
    QMailMessageKey::Properties properties(prop);
    if ((properties & QMailMessageKey::ContentScheme) && (properties & QMailMessageKey::ContentIdentifier))
        properties &= ~QMailMessageKey::ContentIdentifier;

    foreach (QMailMessageKey::Property p, messagePropertyList()) {
        switch (properties & p)
        {
            case QMailMessageKey::Id:
                values.append(extractor.id());
                break;

            case QMailMessageKey::Type:
                values.append(extractor.messageType());
                break;

            case QMailMessageKey::ParentFolderId:
                values.append(extractor.parentFolderId());
                break;

            case QMailMessageKey::Sender:
                values.append(extractor.from());
                break;

            case QMailMessageKey::Recipients:
                values.append(extractor.to());
                break;

            case QMailMessageKey::Subject:
                values.append(extractor.subject());
                break;

            case QMailMessageKey::TimeStamp:
                values.append(extractor.date());
                break;

            case QMailMessageKey::ReceptionTimeStamp:
                values.append(extractor.receivedDate());
                break;

            case QMailMessageKey::Status:
                values.append(extractor.status());
                break;

            case QMailMessageKey::ParentAccountId:
                values.append(extractor.parentAccountId());
                break;

            case QMailMessageKey::ServerUid:
                values.append(extractor.serverUid());
                break;

            case QMailMessageKey::Size:
                values.append(extractor.size());
                break;

            case QMailMessageKey::ContentType:
                values.append(extractor.content());
                break;

            case QMailMessageKey::PreviousParentFolderId:
                values.append(extractor.previousParentFolderId());
                break;

            case QMailMessageKey::ContentScheme:
            case QMailMessageKey::ContentIdentifier:
                // For either of these (there can be only one) we want to produce the entire URI
                values.append(::contentUri(extractor.contentScheme().toString(), extractor.contentIdentifier().toString()));
                break;

            case QMailMessageKey::InResponseTo:
                values.append(extractor.inResponseTo());
                break;

            case QMailMessageKey::ResponseType:
                values.append(extractor.responseType());
                break;
        }
    }

    return values;
}

void QMailStorePrivate::updateMessageValues(const QMailMessageKey::Properties& properties, const QVariantList& values, const QMap<QString, QString>& customFields, QMailMessageMetaData& metaData)
{
    QPair<QString, QString> uriElements;
    QVariantList::const_iterator it = values.constBegin();

    foreach (QMailMessageKey::Property p, messagePropertyList()) {
        const MessageValueExtractor<QVariant> extractor(*it);
        bool valueConsumed(true);

        switch (properties & p)
        {
            case QMailMessageKey::Id:
                metaData.setId(extractor.id());
                break;

            case QMailMessageKey::Type:
                metaData.setMessageType(extractor.messageType());
                break;

            case QMailMessageKey::ParentFolderId:
                metaData.setParentFolderId(extractor.parentFolderId());
                break;

            case QMailMessageKey::Sender:
                metaData.setFrom(extractor.from());
                break;

            case QMailMessageKey::Recipients:
                metaData.setTo(extractor.to());
                break;

            case QMailMessageKey::Subject:
                metaData.setSubject(extractor.subject());
                break;

            case QMailMessageKey::TimeStamp:
                metaData.setDate(extractor.date());
                break;

            case QMailMessageKey::ReceptionTimeStamp:
                metaData.setReceivedDate(extractor.receivedDate());
                break;

            case QMailMessageKey::Status:
                metaData.setStatus(extractor.status());
                break;

            case QMailMessageKey::ParentAccountId:
                metaData.setParentAccountId(extractor.parentAccountId());
                break;

            case QMailMessageKey::ServerUid:
                metaData.setServerUid(extractor.serverUid());
                break;

            case QMailMessageKey::Size:
                metaData.setSize(extractor.size());
                break;

            case QMailMessageKey::ContentType:
                metaData.setContent(extractor.content());
                break;

            case QMailMessageKey::PreviousParentFolderId:
                metaData.setPreviousParentFolderId(extractor.previousParentFolderId());
                break;

            case QMailMessageKey::ContentScheme:
                if (uriElements.first.isEmpty()) {
                    uriElements = extractUriElements(extractor.contentUri());
                } else {
                    valueConsumed = false;
                }
                metaData.setContentScheme(uriElements.first);
                break;

            case QMailMessageKey::ContentIdentifier:
                if (uriElements.first.isEmpty()) {
                    uriElements = extractUriElements(extractor.contentUri());
                } else {
                    valueConsumed = false;
                }
                metaData.setContentIdentifier(uriElements.second);
                break;

            case QMailMessageKey::InResponseTo:
                metaData.setInResponseTo(extractor.inResponseTo());
                break;

            case QMailMessageKey::ResponseType:
                metaData.setResponseType(extractor.responseType());
                break;

            case QMailMessageKey::Custom:
                metaData.setCustomFields(customFields);
                break;

            default:
                valueConsumed = false;
                break;
        }

        if (valueConsumed)
            ++it;
    }

    if (it != values.constEnd())
        qWarning() << QString("updateMessageValues: %1 values not consumed!").arg(values.constEnd() - it);

    // The target message is not completely loaded
    metaData.setStatus(QMailMessage::UnloadedData, true);
}

bool QMailStorePrivate::executeFile(QFile &file)
{
    bool result(true);

    // read assuming utf8 encoding.
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("utf8"));
    ts.setAutoDetectUnicode(true);
    
    QString sql = parseSql(ts);
    while (result && !sql.isEmpty()) {
        QSqlQuery query(database);
        if (!query.exec(sql)) {
            qMailLog(Messaging) << "Failed to exec table creation SQL query:" << sql << "- error:" << query.lastError().text();
            result = false;
        }
        sql = parseSql(ts);
    }

    return result;
}

bool QMailStorePrivate::ensureVersionInfo()
{
    if (!database.tables().contains("versioninfo", Qt::CaseInsensitive)) {
        // Use the same version scheme as dbmigrate, in case we need to cooperate later
        QString sql("CREATE TABLE versioninfo ("
                    "   tableName NVARCHAR (255) NOT NULL,"
                    "   versionNum INTEGER NOT NULL,"
                    "   lastUpdated NVARCHAR(20) NOT NULL,"
                    "   PRIMARY KEY(tableName, versionNum))");

        QSqlQuery query(database);
        if (!query.exec(sql)) {
            qMailLog(Messaging) << "Failed to create versioninfo table - query:" << sql << "- error:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

qint64 QMailStorePrivate::tableVersion(const QString &name) const
{
    QString sql("SELECT COALESCE(MAX(versionNum), 0) FROM versioninfo WHERE tableName=?");

    QSqlQuery query(database);
    query.prepare(sql);
    query.addBindValue(name);
    if (query.exec() && query.first())
        return query.value(0).value<qint64>();

    qMailLog(Messaging) << "Failed to query versioninfo - query:" << sql << "- error:" << query.lastError().text();
    return 0;
}

bool QMailStorePrivate::setTableVersion(const QString &name, qint64 version)
{
    QString sql("DELETE FROM versioninfo WHERE tableName=? AND versionNum=?");

    // Delete any existing entry for this table
    QSqlQuery query(database);
    query.prepare(sql);
    query.addBindValue(name);
    query.addBindValue(version);

    if (!query.exec()) {
        qMailLog(Messaging) << "Failed to delete versioninfo - query:" << sql << "- error:" << query.lastError().text();
        return false;
    } else {
        sql = "INSERT INTO versioninfo (tablename,versionNum,lastUpdated) VALUES (?,?,?)";

        // Insert the updated info
        query = QSqlQuery(database);
        query.prepare(sql);
        query.addBindValue(name);
        query.addBindValue(version);
        query.addBindValue(QDateTime::currentDateTime().toString());

        if (!query.exec()) {
            qMailLog(Messaging) << "Failed to insert versioninfo - query:" << sql << "- error:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

qint64 QMailStorePrivate::incrementTableVersion(const QString &name, qint64 current)
{
    qint64 next = current + 1;

    QString versionInfo("-" + QString::number(current) + "-" + QString::number(next));
    QString scriptName(":/QtopiaSql/" + database.driverName() + "/" + name + versionInfo);

    QFile data(scriptName);
    if (!data.open(QIODevice::ReadOnly)) {
        qMailLog(Messaging) << "Failed to load table upgrade resource:" << name;
    } else {
        if (executeFile(data)) {
            // Update the table version number
            if (setTableVersion(name, next))
                current = next;
        }
    }

    return current;
}

bool QMailStorePrivate::upgradeTableVersion(const QString &name, qint64 current, qint64 final)
{
    while (current < final) {
        int newVersion = incrementTableVersion(name, current);
        if (newVersion == current) {
            qMailLog(Messaging) << "Failed to increment table version from:" << current << "(" << name << ")";
            break;
        } else {
            current = newVersion;
        }
    }

    return (current == final);
}

bool QMailStorePrivate::createTable(const QString &name)
{
    bool result = true;

    // load schema.
    QFile data(":/QtopiaSql/" + database.driverName() + "/" + name);
    if (!data.open(QIODevice::ReadOnly)) {
        qMailLog(Messaging) << "Failed to load table schema resource:" << name;
        result = false;
    } else {
        result = executeFile(data);
    }

    return result;
}

bool QMailStorePrivate::setupTables(const QList<TableInfo> &tableList)
{
    bool result = true;

    QStringList tables = database.tables();

    foreach (const TableInfo &table, tableList) {
        const QString &tableName(table.first);
        qint64 version(table.second);

        if (!tables.contains(tableName, Qt::CaseInsensitive)) {
            // Create the table
            result &= (createTable(tableName) && setTableVersion(tableName, version));
        } else {
            // Ensure the table does not have an incompatible version
            qint64 dbVersion = tableVersion(tableName);
            if (dbVersion == 0) {
                qWarning() << "No version for existing table:" << tableName;
                result = false;
            } else if (dbVersion != version) {
                if (version > dbVersion) {
                    // Try upgrading the table
                    result = upgradeTableVersion(tableName, dbVersion, version);
                    qMailLog(Messaging) << (result ? "Upgraded" : "Unable to upgrade") << "version for table:" << tableName << " from" << dbVersion << "to" << version;
                } else {
                    qWarning() << "Incompatible version for table:" << tableName << "- existing" << dbVersion << "!=" << version;
                    result = false;
                }
            }
        }
    }
        
    return result;
}

bool QMailStorePrivate::setupFolders(const QList<FolderInfo> &folderList)
{
    QSet<quint64> folderIds;

    {
        QSqlQuery query(simpleQuery("SELECT id FROM mailfolders", 
                                    "folder ids query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;

        while (query.next())
            folderIds.insert(query.value(0).toULongLong());
    }

    foreach (const FolderInfo &folder, folderList) {
        if (folderIds.contains(folder.first))
            continue;
        QSqlQuery query(simpleQuery("INSERT INTO mailfolders (id,name,parentid,parentaccountid,displayname,status,servercount,serverunreadcount,serverundiscoveredcount) VALUES (?,?,?,?,?,?,?,?,?)",
                                    QVariantList() << folder.first
                                                   << folder.second
                                                   << quint64(0)
                                                   << quint64(0)
                                                   << QString()
                                                   << quint64(0)
                                                   << int(0)
                                                   << int(0)
                                                   << int(0),
                                    "setupFolders insert query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    return true;
}

bool QMailStorePrivate::purgeMissingAncestors()
{
    QString sql("DELETE FROM missingancestors WHERE state=1");

    QSqlQuery query(database);
    query.prepare(sql);
    if (!query.exec()) {
        qMailLog(Messaging) << "Failed to purge missing ancestors - query:" << sql << "- error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool QMailStorePrivate::purgeObsoleteFiles()
{
    QStringList identifiers;

    {
        QString sql("SELECT mailfile FROM obsoletefiles");

        QSqlQuery query(database);
        query.prepare(sql);
        if (!query.exec()) {
            qMailLog(Messaging) << "Failed to purge obsolete files - query:" << sql << "- error:" << query.lastError().text();
            return false;
        } else {
            while (query.next()) {
                identifiers.append(query.value(0).toString());
            }
        }
    }

    if (!identifiers.isEmpty()) {
        foreach (const QString& contentUri, identifiers) {
            QPair<QString, QString> elements(extractUriElements(contentUri));

            if (QMailContentManager *contentManager = QMailContentManagerFactory::create(elements.first)) {
                QMailStore::ErrorCode code = contentManager->remove(elements.second);
                if (code != QMailStore::NoError) {
                    qMailLog(Messaging) << "Unable to remove obsolete message content:" << contentUri;
                } else {
                    QString sql("DELETE FROM obsoletefiles WHERE mailfile=?");

                    QSqlQuery query(database);
                    query.prepare(sql);
                    query.addBindValue(contentUri);
                    if (!query.exec()) {
                        qMailLog(Messaging) << "Failed to purge obsolete file - query:" << sql << "- error:" << query.lastError().text();
                        return false;
                    }
                }
            } else {
                qMailLog(Messaging) << "Unable to create content manager for scheme:" << elements.first;
            }
        }
    }

    return true;
}

bool QMailStorePrivate::performMaintenanceTask(const QString &task, uint secondsFrequency, bool (QMailStorePrivate::*func)(void))
{
    QDateTime lastPerformed(QDateTime::fromTime_t(0));

    {
        QString sql("SELECT performed FROM maintenancerecord WHERE task=?");

        QSqlQuery query(database);
        query.prepare(sql);
        query.addBindValue(task);
        if (!query.exec()) {
            qMailLog(Messaging) << "Failed to query performed timestamp - query:" << sql << "- error:" << query.lastError().text();
            return false;
        } else {
            if (query.first()) {
                lastPerformed = query.value(0).value<QDateTime>();
            }
        }
    }

    QDateTime nextTime(lastPerformed.addSecs(secondsFrequency));
    QDateTime currentTime(QDateTime::currentDateTime());
    if (currentTime >= nextTime) {
        if (!(this->*func)()) {
            return false;
        }

        // Update the timestamp
        QString sql;
        if (lastPerformed.toTime_t() == 0) {
            sql = "INSERT INTO maintenancerecord (performed,task) VALUES(?,?)";
        } else {
            sql = "UPDATE maintenancerecord SET performed=? WHERE task=?";
        }

        QSqlQuery query(database);
        query.prepare(sql);
        query.addBindValue(currentTime);
        query.addBindValue(task);
        if (!query.exec()) {
            qMailLog(Messaging) << "Failed to update performed timestamp - query:" << sql << "- error:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool QMailStorePrivate::performMaintenance()
{
    // Perform this task no more than once every 24 hours
    if (!performMaintenanceTask("purge missing ancestors", 24*60*60, &QMailStorePrivate::purgeMissingAncestors))
        return false;

    // Perform this task no more than once every hour
    if (!performMaintenanceTask("purge obsolete files", 60*60, &QMailStorePrivate::purgeObsoleteFiles))
        return false;

    return true;
}

QString QMailStorePrivate::parseSql(QTextStream& ts)
{
    QString qry = "";
    while(!ts.atEnd())
    {
        QString line = ts.readLine();
        // comment, remove.
        if (line.contains(QLatin1String("--")))
            line.truncate (line.indexOf (QLatin1String("--")));
        if (line.trimmed ().length () == 0)
            continue;
        qry += line;
        
        if ( line.contains( ';' ) == false) 
            qry += QLatin1String(" ");
        else
            return qry;
    }
    return qry;
}

QString QMailStorePrivate::expandValueList(const QVariantList& valueList)
{
    Q_ASSERT(!valueList.isEmpty());
    return expandValueList(valueList.count());
}

QString QMailStorePrivate::expandValueList(int valueCount)
{
    Q_ASSERT(valueCount > 0);

    if (valueCount == 1) {
        return "(?)";
    } else {
        QString inList = " (?";
        for (int i = 1; i < valueCount; ++i)
            inList += ",?";
        inList += ")";
        return inList;
    }
}

QString QMailStorePrivate::expandProperties(const QMailMessageKey::Properties& prop, bool update) const 
{
    QString out;

    // The ContentScheme and ContentIdentifier properties map to the same field
    QMailMessageKey::Properties properties(prop);
    if ((properties & QMailMessageKey::ContentScheme) && (properties & QMailMessageKey::ContentIdentifier))
        properties &= ~QMailMessageKey::ContentIdentifier;

    const QMailStorePrivate::MessagePropertyMap &map(messagePropertyMap());
    foreach (QMailMessageKey::Property p, messagePropertyList()) {
        if (properties & p) {
            if (!out.isEmpty())
                out += ",";                     
            out += map.value(p);
            if (update)
                out += "=?";
        }
    }

    return out;
}

bool QMailStorePrivate::addAccount(QMailAccount *account, QMailAccountConfiguration *config,
                                   QMailAccountIdList *addedAccountIds)
{
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptAddAccount, this, 
                                        account, config, 
                                        addedAccountIds), 
                                   "addAccount");
}

bool QMailStorePrivate::addFolder(QMailFolder *folder,
                                  QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds)
{   
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptAddFolder, this, 
                                        folder, 
                                        addedFolderIds, modifiedAccountIds), 
                                   "addFolder");
}

bool QMailStorePrivate::addMessages(const QList<QMailMessage *> &messages,
                                    QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    // Resolve from overloaded member functions:
    AttemptResult (QMailStorePrivate::*func)(QMailMessage*, const QString&, const QStringList&, QMailMessageIdList*, QMailMessageIdList*, QMailFolderIdList*, QMailAccountIdList*, Transaction&, bool) = &QMailStorePrivate::attemptAddMessage;
    QSet<QString> contentSchemes;

    Transaction t(this);

    foreach (QMailMessage *message, messages) {
        // Find the message identifier and references from the header
        QString identifier(identifierValue(message->headerFieldText("Message-ID")));
        QStringList references(identifierValues(message->headerFieldText("References")));
        QString predecessor(identifierValue(message->headerFieldText("In-Reply-To")));
        if (!predecessor.isEmpty()) {
            if (references.isEmpty() || (references.last() != predecessor)) {
                references.append(predecessor);
            }
        }

        if (!repeatedly<WriteAccess>(bind(func, this, 
                                          message, cref(identifier), cref(references),
                                          addedMessageIds, updatedMessageIds, modifiedFolderIds, modifiedAccountIds), 
                                     "addMessages",
                                     &t)) {
            return false;
        }

        contentSchemes.insert(message->contentScheme());
    }

    // Ensure that the content manager makes the changes durable before we return
    foreach (const QString &scheme, contentSchemes) {
        if (QMailContentManager *contentManager = QMailContentManagerFactory::create(scheme)) {
            QMailStore::ErrorCode code = contentManager->ensureDurability();
            if (code != QMailStore::NoError) {
                setLastError(code);
                qMailLog(Messaging) << "Unable to ensure message content durability for scheme:" << scheme;
                return false;
            }
        } else {
            setLastError(QMailStore::FrameworkFault);
            qMailLog(Messaging) << "Unable to create content manager for scheme:" << scheme;
            return false;
        }
    }

    if (!t.commit()) {
        qMailLog(Messaging) << "Unable to commit successful addMessages!";
        return false;
    }

    return true;
}

bool QMailStorePrivate::addMessages(const QList<QMailMessageMetaData *> &messages,
                                    QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    // Resolve from overloaded member functions:
    AttemptResult (QMailStorePrivate::*func)(QMailMessageMetaData*, const QString&, const QStringList&, QMailMessageIdList*, QMailMessageIdList*, QMailFolderIdList*, QMailAccountIdList*, Transaction&, bool) = &QMailStorePrivate::attemptAddMessage;

    Transaction t(this);

    foreach (QMailMessageMetaData *metaData, messages) {
        QString identifier;
        QStringList references;

        if (!repeatedly<WriteAccess>(bind(func, this, 
                                          metaData, cref(identifier), cref(references),
                                          addedMessageIds, updatedMessageIds, modifiedFolderIds, modifiedAccountIds), 
                                     "addMessages",
                                     &t)) {
            return false;
        }
    }

    if (!t.commit()) {
        qMailLog(Messaging) << "Unable to commit successful addMessages!";
        return false;
    }

    return true;
}

bool QMailStorePrivate::removeAccounts(const QMailAccountKey &key,
                                       QMailAccountIdList *deletedAccountIds, QMailFolderIdList *deletedFolderIds, QMailMessageIdList *deletedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptRemoveAccounts, this, 
                                        cref(key), 
                                        deletedAccountIds, deletedFolderIds, deletedMessageIds, updatedMessageIds, modifiedFolderIds, modifiedAccountIds), 
                                   "removeAccounts");
}

bool QMailStorePrivate::removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                                      QMailFolderIdList *deletedFolderIds, QMailMessageIdList *deletedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptRemoveFolders, this, 
                                        cref(key), option, 
                                        deletedFolderIds, deletedMessageIds, updatedMessageIds, modifiedFolderIds, modifiedAccountIds), 
                                   "removeFolders");
}

bool QMailStorePrivate::removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                                       QMailMessageIdList *deletedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptRemoveMessages, this, 
                                        cref(key), option, 
                                        deletedMessageIds, updatedMessageIds, modifiedFolderIds, modifiedAccountIds), 
                                   "removeMessages");
}

bool QMailStorePrivate::updateAccount(QMailAccount *account, QMailAccountConfiguration *config,
                                      QMailAccountIdList *updatedAccountIds)
{
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptUpdateAccount, this, 
                                        account, config, 
                                        updatedAccountIds), 
                                   "updateAccount");
}

bool QMailStorePrivate::updateAccountConfiguration(QMailAccountConfiguration *config,
                                                   QMailAccountIdList *updatedAccountIds)
{
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptUpdateAccount, this, 
                                        reinterpret_cast<QMailAccount*>(0), config, 
                                        updatedAccountIds), 
                                   "updateAccount");
}

bool QMailStorePrivate::updateFolder(QMailFolder *folder,
                                     QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptUpdateFolder, this, 
                                        folder, 
                                        updatedFolderIds, modifiedAccountIds), 
                                   "updateFolder");
}

bool QMailStorePrivate::updateMessages(const QList<QPair<QMailMessageMetaData*, QMailMessage*> > &messages,
                                       QMailMessageIdList *updatedMessageIds, QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    QSet<QString> contentSchemes;

    Transaction t(this);

    typedef QPair<QMailMessageMetaData*, QMailMessage*> PairType;

    foreach (const PairType &pair, messages) {
        if (!repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptUpdateMessage, this, 
                                          pair.first, pair.second,
                                          updatedMessageIds, modifiedMessageIds, modifiedFolderIds, modifiedAccountIds), 
                                     "updateMessages",
                                     &t)) {
            return false;
        }

        contentSchemes.insert(pair.first->contentScheme());
    }

    // Ensure that the content manager makes the changes durable before we return
    foreach (const QString &scheme, contentSchemes) {
        if (QMailContentManager *contentManager = QMailContentManagerFactory::create(scheme)) {
            QMailStore::ErrorCode code = contentManager->ensureDurability();
            if (code != QMailStore::NoError) {
                setLastError(code);
                qMailLog(Messaging) << "Unable to ensure message content durability for scheme:" << scheme;
                return false;
            }
        } else {
            setLastError(QMailStore::FrameworkFault);
            qMailLog(Messaging) << "Unable to create content manager for scheme:" << scheme;
            return false;
        }
    }

    if (!t.commit()) {
        qMailLog(Messaging) << "Unable to commit successful updateMessages!";
        return false;
    }

    return true;
}

bool QMailStorePrivate::updateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data,
                                               QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptUpdateMessagesMetaData, this, 
                                        cref(key), cref(properties), cref(data), 
                                        updatedMessageIds, modifiedFolderIds, modifiedAccountIds), 
                                   "updateMessagesMetaData");
}

bool QMailStorePrivate::updateMessagesMetaData(const QMailMessageKey &key, quint64 status, bool set,
                                               QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptUpdateMessagesStatus, this, 
                                        cref(key), status, set,
                                        updatedMessageIds, modifiedFolderIds, modifiedAccountIds), 
                                   "updateMessagesMetaData"); // not 'updateMessagesStatus', due to function name exported by QMailStore
}

bool QMailStorePrivate::restoreToPreviousFolder(const QMailMessageKey &key,
                                                QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds)
{
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptRestoreToPreviousFolder, this, 
                                        cref(key), 
                                        updatedMessageIds, modifiedFolderIds, modifiedAccountIds), 
                                   "restoreToPreviousFolder");
}

bool QMailStorePrivate::purgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids)
{
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptPurgeMessageRemovalRecords, this, 
                                        cref(accountId), cref(serverUids)), 
                                   "purgeMessageRemovalRecords");
}

int QMailStorePrivate::countAccounts(const QMailAccountKey &key) const
{
    int result(0);
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptCountAccounts, const_cast<QMailStorePrivate*>(this), 
                                cref(key), &result), 
                           "countAccounts");
    return result;
}

int QMailStorePrivate::countFolders(const QMailFolderKey &key) const
{
    int result(0);
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptCountFolders, const_cast<QMailStorePrivate*>(this), 
                                cref(key), &result), 
                           "countFolders");
    return result;
}

int QMailStorePrivate::countMessages(const QMailMessageKey &key) const
{
    int result(0);
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptCountMessages, const_cast<QMailStorePrivate*>(this), 
                                cref(key), &result), 
                           "countMessages");
    return result;
}

int QMailStorePrivate::sizeOfMessages(const QMailMessageKey &key) const
{
    int result(0);
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptSizeOfMessages, const_cast<QMailStorePrivate*>(this), 
                                cref(key), &result), 
                           "sizeOfMessages");
    return result;
}

QMailAccountIdList QMailStorePrivate::queryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey, uint limit, uint offset) const
{
    QMailAccountIdList ids;
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptQueryAccounts, const_cast<QMailStorePrivate*>(this), 
                                cref(key), cref(sortKey), limit, offset, &ids), 
                           "queryAccounts");
    return ids;
}

QMailFolderIdList QMailStorePrivate::queryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey, uint limit, uint offset) const
{
    QMailFolderIdList ids;
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptQueryFolders, const_cast<QMailStorePrivate*>(this), 
                                cref(key), cref(sortKey), limit, offset, &ids), 
                           "queryFolders");
    return ids;
}

QMailMessageIdList QMailStorePrivate::queryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey, uint limit, uint offset) const
{
    QMailMessageIdList ids;
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptQueryMessages, const_cast<QMailStorePrivate*>(this), 
                                cref(key), cref(sortKey), limit, offset, &ids), 
                           "queryMessages");
    return ids;
}

QMailAccount QMailStorePrivate::account(const QMailAccountId &id) const
{
    if (accountCache.contains(id))
        return accountCache.lookup(id);

    QMailAccount account;
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptAccount, const_cast<QMailStorePrivate*>(this), 
                                cref(id), &account), 
                           "account");
    return account;
}

QMailAccountConfiguration QMailStorePrivate::accountConfiguration(const QMailAccountId &id) const
{
    QMailAccountConfiguration config;
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptAccountConfiguration, const_cast<QMailStorePrivate*>(this), 
                                cref(id), &config), 
                           "accountConfiguration");
    return config;
}

QMailFolder QMailStorePrivate::folder(const QMailFolderId &id) const
{
    if (folderCache.contains(id))
        return folderCache.lookup(id);

    QMailFolder folder;
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptFolder, const_cast<QMailStorePrivate*>(this), 
                                cref(id), &folder), 
                           "folder");
    return folder;
}

QMailMessage QMailStorePrivate::message(const QMailMessageId &id) const
{
    // Resolve from overloaded member functions:
    AttemptResult (QMailStorePrivate::*func)(const QMailMessageId&, QMailMessage*, ReadLock&) = &QMailStorePrivate::attemptMessage;

    QMailMessage msg;
    repeatedly<ReadAccess>(bind(func, const_cast<QMailStorePrivate*>(this), 
                                cref(id), &msg), 
                           "message(id)");
    return msg;
}

QMailMessage QMailStorePrivate::message(const QString &uid, const QMailAccountId &accountId) const
{
    // Resolve from overloaded member functions:
    AttemptResult (QMailStorePrivate::*func)(const QString&, const QMailAccountId&, QMailMessage*, ReadLock&) = &QMailStorePrivate::attemptMessage;

    QMailMessage msg;
    repeatedly<ReadAccess>(bind(func, const_cast<QMailStorePrivate*>(this), 
                                cref(uid), cref(accountId), &msg), 
                           "message(uid, accountId)");
    return msg;
}

QMailMessageMetaData QMailStorePrivate::messageMetaData(const QMailMessageId &id) const
{
    if (messageCache.contains(id))
        return messageCache.lookup(id);

    //if not in the cache, then preload the cache with the id and its most likely requested siblings
    preloadHeaderCache(id);

    return messageCache.lookup(id);
}

QMailMessageMetaData QMailStorePrivate::messageMetaData(const QString &uid, const QMailAccountId &accountId) const
{
    QMailMessageMetaData metaData;
    bool success;

    QPair<QMailAccountId, QString> key(accountId, uid);
    if (uidCache.contains(key)) {
        // We can look this message up in the cache
        QMailMessageId id(uidCache.lookup(key));

        if (messageCache.contains(id))
            return messageCache.lookup(id);

        // Resolve from overloaded member functions:
        AttemptResult (QMailStorePrivate::*func)(const QMailMessageId&, QMailMessageMetaData*, ReadLock&) = &QMailStorePrivate::attemptMessageMetaData;

        success = repeatedly<ReadAccess>(bind(func, const_cast<QMailStorePrivate*>(this), 
                                              cref(id), &metaData), 
                                         "messageMetaData(id)");
    } else {
        // Resolve from overloaded member functions:
        AttemptResult (QMailStorePrivate::*func)(const QString&, const QMailAccountId&, QMailMessageMetaData*, ReadLock&) = &QMailStorePrivate::attemptMessageMetaData;

        success = repeatedly<ReadAccess>(bind(func, const_cast<QMailStorePrivate*>(this), 
                                              cref(uid), cref(accountId), &metaData), 
                                         "messageMetaData(uid/accountId)");
    }

    if (success) {
        messageCache.insert(metaData);
        uidCache.insert(qMakePair(metaData.parentAccountId(), metaData.serverUid()), metaData.id());
    }

    return metaData;
}

QMailMessageMetaDataList QMailStorePrivate::messagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option) const
{
    QMailMessageMetaDataList metaData;
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptMessagesMetaData, const_cast<QMailStorePrivate*>(this), 
                                cref(key), cref(properties), option, &metaData), 
                           "messagesMetaData");
    return metaData;
}

QMailMessageRemovalRecordList QMailStorePrivate::messageRemovalRecords(const QMailAccountId &accountId, const QMailFolderId &folderId) const
{
    QMailMessageRemovalRecordList removalRecords;
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptMessageRemovalRecords, const_cast<QMailStorePrivate*>(this), 
                                cref(accountId), cref(folderId), &removalRecords), 
                           "messageRemovalRecords(accountId, folderId)");
    return removalRecords;
}

bool QMailStorePrivate::registerAccountStatusFlag(const QString &name)
{
    if (accountStatusMask(name) != 0)
        return true;

    static const QString context("accountstatus");
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptRegisterStatusBit, this,
                                        cref(name), cref(context), 64), 
                                   "registerAccountStatusBit");
}

quint64 QMailStorePrivate::accountStatusMask(const QString &name) const
{
    static QMap<QString, quint64> statusMap;
    static const QString context("accountstatus");

    return queryStatusMap(name, context, statusMap);
}

bool QMailStorePrivate::registerFolderStatusFlag(const QString &name)
{
    if (folderStatusMask(name) != 0)
        return true;

    static const QString context("folderstatus");
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptRegisterStatusBit, this,
                                        cref(name), cref(context), 64), 
                                   "registerFolderStatusBit");
}

quint64 QMailStorePrivate::folderStatusMask(const QString &name) const
{
    static QMap<QString, quint64> statusMap;
    static const QString context("folderstatus");

    return queryStatusMap(name, context, statusMap);
}

bool QMailStorePrivate::registerMessageStatusFlag(const QString &name)
{
    if (messageStatusMask(name) != 0)
        return true;

    static const QString context("messagestatus");
    return repeatedly<WriteAccess>(bind(&QMailStorePrivate::attemptRegisterStatusBit, this,
                                        cref(name), cref(context), 64), 
                                   "registerMessageStatusBit");
}

quint64 QMailStorePrivate::messageStatusMask(const QString &name) const
{
    static QMap<QString, quint64> statusMap;
    static const QString context("messagestatus");

    return queryStatusMap(name, context, statusMap);
}

quint64 QMailStorePrivate::queryStatusMap(const QString &name, const QString &context, QMap<QString, quint64> &map) const
{
    QMap<QString, quint64>::const_iterator it = map.find(name);
    if (it != map.end())
        return it.value();

    int result(0);
    repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptStatusBit, const_cast<QMailStorePrivate*>(this), 
                                cref(name), cref(context), &result), 
                           "folderStatusMask");
    if (result == 0)
        return 0;

    quint64 maskValue = (1 << (result - 1));
    map[name] = maskValue;
    return maskValue;
}

QMailFolderIdList QMailStorePrivate::folderAncestorIds(const QMailFolderIdList& ids, bool inTransaction, AttemptResult *result) const
{
    QMailFolderIdList ancestorIds;

    QMailStorePrivate *self(const_cast<QMailStorePrivate*>(this));
    if (inTransaction) {
        // We can't retry this query after a busy error if we're in a transaction
        ReadLock l(self);
        *result = self->attemptFolderAncestorIds(ids, &ancestorIds, l);
    } else {
        bool ok = repeatedly<ReadAccess>(bind(&QMailStorePrivate::attemptFolderAncestorIds, self,
                                              cref(ids), &ancestorIds), 
                                         "folderAncestorIds");
        if (result)
            *result = ok ? Success : Failure;
    }

    return ancestorIds;
}

void QMailStorePrivate::removeExpiredData(const QMailMessageIdList& messageIds, const QStringList& contentUris, const QMailFolderIdList& folderIds, const QMailAccountIdList& accountIds)
{
    foreach (const QMailMessageId& id, messageIds) {
        messageCache.remove(id);
    }

    {
        MutexGuard lock(contentManagerMutex());
        if (!lock.lock(1000)) {
            qMailLog(Messaging) << "Unable to acquire message body mutex in removeExpiredData!";
        } else {
            foreach (const QString& contentUri, contentUris) {
                QPair<QString, QString> elements(extractUriElements(contentUri));

                if (QMailContentManager *contentManager = QMailContentManagerFactory::create(elements.first)) {
                    QMailStore::ErrorCode code = contentManager->remove(elements.second);
                    if (code != QMailStore::NoError) {
                        qMailLog(Messaging) << "Unable to remove expired message content:" << contentUri;
                        if (code == QMailStore::ContentNotRemoved) {
                            // The existing content could not be removed - try again later
                            obsoleteContent(contentUri);
                        }
                    }
                } else {
                    qMailLog(Messaging) << "Unable to create content manager for scheme:" << elements.first;
                }
            }
        }
    }

    foreach (const QMailFolderId& id, folderIds) {
        folderCache.remove(id);
    }

    foreach (const QMailAccountId& id, accountIds) {
        accountCache.remove(id);
    }
}

template<typename AccessType, typename FunctionType>
bool QMailStorePrivate::repeatedly(FunctionType func, const QString &description, Transaction *t) const
{
    static const unsigned int MinRetryDelay = 64;
    static const unsigned int MaxRetryDelay = 2048;
    static const unsigned int MaxAttempts = 10;

    // This function calls the supplied function repeatedly, retrying whenever it
    // returns the DatabaseFailure result and the database's last error is SQLITE_BUSY.
    // It sleeps between repeated attempts, for increasing amounts of time.
    // The argument should be an object allowing nullary invocation returning an
    // AttemptResult value, created with tr1::bind if necessary.

    unsigned int attemptCount = 0;
    unsigned int delay = MinRetryDelay;

     while (true) {
        AttemptResult result;
        if (t) {
            result = evaluate(AccessType(), func, *t);
        } else {
            result = evaluate(AccessType(), func, description, const_cast<QMailStorePrivate*>(this));
        }

        if (result == Success) {
            if (attemptCount > 0) {
                qMailLog(Messaging) << pid << "Able to" << qPrintable(description) << "after" << attemptCount << "failed attempts";
            }
            return true;
        } else if (result == Failure) {
            qMailLog(Messaging) << pid << "Unable to" << qPrintable(description);
            if (lastError() == QMailStore::NoError) {
                setLastError(errorType(AccessType()));
            }
            return false;
        } else { 
            // result == DatabaseFailure
            if (queryError() == Sqlite3BusyErrorNumber) {
                if (attemptCount < MaxAttempts) {
                    qMailLog(Messaging) << pid << "Failed to" << qPrintable(description) << "- busy, pausing to retry";

                    // Pause before we retry
                    QMail::usleep(delay * 1000);
                    if (delay < MaxRetryDelay)
                        delay *= 2;

                    ++attemptCount;
                } else {
                    qMailLog(Messaging) << pid << "Retry count exceeded - failed to" << qPrintable(description);
                    break;
                }
            } else if (queryError() == Sqlite3ConstraintErrorNumber) {
                qMailLog(Messaging) << pid << "Unable to" << qPrintable(description) << "- constraint failure";
                setLastError(QMailStore::ConstraintFailure);
                break;
            } else {
                qMailLog(Messaging) << pid << "Unable to" << qPrintable(description) << "- code:" << queryError();
                break;
            }
        }
    }

    // We experienced a database-related failure
    if (lastError() == QMailStore::NoError) {
        setLastError(QMailStore::FrameworkFault);
    }
    return false;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::addCustomFields(quint64 id, const QMap<QString, QString> &fields, const QString &tableName)
{
    if (!fields.isEmpty()) {
        QVariantList customFields;
        QVariantList customValues;

        // Insert any custom fields belonging to this account
        QMap<QString, QString>::const_iterator it = fields.begin(), end = fields.end();
        for ( ; it != end; ++it) {
            customFields.append(QVariant(it.key()));
            customValues.append(QVariant(it.value()));
        }

        // Batch insert the custom fields
        QString sql("INSERT INTO %1 (id,name,value) VALUES (%2,?,?)");
        QSqlQuery query(batchQuery(sql.arg(tableName).arg(QString::number(id)),
                                   QVariantList() << QVariant(customFields)
                                                  << QVariant(customValues),
                                   QString("%1 custom field insert query").arg(tableName)));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::updateCustomFields(quint64 id, const QMap<QString, QString> &fields, const QString &tableName)
{
    QMap<QString, QString> existing;

    {
        // Find the existing fields
        QString sql("SELECT name,value FROM %1 WHERE id=?");
        QSqlQuery query(simpleQuery(sql.arg(tableName),
                                    QVariantList() << id,
                                    QString("%1 update custom select query").arg(tableName)));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        while (query.next())
            existing.insert(query.value(0).toString(), query.value(1).toString());
    }

    QVariantList obsoleteFields;
    QVariantList modifiedFields;
    QVariantList modifiedValues;
    QVariantList addedFields;
    QVariantList addedValues;

    // Compare the sets
    QMap<QString, QString>::const_iterator fend = fields.end(), eend = existing.end();
    QMap<QString, QString>::const_iterator it = existing.begin();
    for ( ; it != eend; ++it) {
        QMap<QString, QString>::const_iterator current = fields.find(it.key());
        if (current == fend) {
            obsoleteFields.append(QVariant(it.key()));
        } else if (*current != *it) {
            modifiedFields.append(QVariant(current.key()));
            modifiedValues.append(QVariant(current.value()));
        }
    }

    for (it = fields.begin(); it != fend; ++it) {
        if (existing.find(it.key()) == eend) {
            addedFields.append(QVariant(it.key()));
            addedValues.append(QVariant(it.value()));
        }
    }

    if (!obsoleteFields.isEmpty()) {
        // Remove the obsolete fields
        QString sql("DELETE FROM %1 WHERE id=? AND name IN %2");
        QSqlQuery query(simpleQuery(sql.arg(tableName).arg(expandValueList(obsoleteFields)),
                                    QVariantList() << id << obsoleteFields,
                                    QString("%1 update custom delete query").arg(tableName)));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    }

    if (!modifiedFields.isEmpty()) {
        // Batch update the modified fields
        QString sql("UPDATE %1 SET value=? WHERE id=%2 AND name=?");
        QSqlQuery query(batchQuery(sql.arg(tableName).arg(QString::number(id)),
                                   QVariantList() << QVariant(modifiedValues)
                                                  << QVariant(modifiedFields),
                                   QString("%1 update custom update query").arg(tableName)));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    }

    if (!addedFields.isEmpty()) {
        // Batch insert the added fields
        QString sql("INSERT INTO %1 (id,name,value) VALUES (%2,?,?)");
        QSqlQuery query(batchQuery(sql.arg(tableName).arg(QString::number(id)),
                                   QVariantList() << QVariant(addedFields)
                                                  << QVariant(addedValues),
                                   QString("%1 update custom insert query").arg(tableName)));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::customFields(quint64 id, QMap<QString, QString> *fields, const QString &tableName)
{
    QString sql("SELECT name,value FROM %1 WHERE id=?");
    QSqlQuery query(simpleQuery(sql.arg(tableName),
                                QVariantList() << id,
                                QString("%1 custom field query").arg(tableName)));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    while (query.next())
        fields->insert(query.value(0).toString(), query.value(1).toString());

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptAddAccount(QMailAccount *account, QMailAccountConfiguration* config, 
                                                                      QMailAccountIdList *addedAccountIds, 
                                                                      Transaction &t, bool commitOnSuccess)
{
    if (account->id().isValid() && idExists(account->id())) {
        qMailLog(Messaging) << "Account already exists in database, use update instead";
        return Failure;
    }

    QMailAccountId insertId;

    {
        QString properties("type,name,status,signature,emailaddress");
        QString values("?,?,?,?,?");
        QVariantList propertyValues;
        propertyValues << static_cast<int>(account->messageType()) 
                       << account->name() 
                       << account->status()
                       << account->signature()
                       << account->fromAddress().toString(true);

        {
            QSqlQuery query(simpleQuery(QString("INSERT INTO mailaccounts (%1) VALUES (%2)").arg(properties).arg(values),
                                        propertyValues,
                                        "addAccount mailaccounts query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            //Extract the insert id
            insertId = QMailAccountId(extractValue<quint64>(query.lastInsertId()));
        }

        // Insert any standard folders configured for this account
        const QMap<QMailFolder::StandardFolder, QMailFolderId> &folders(account->standardFolders());
        if (!folders.isEmpty()) {
            QVariantList types;
            QVariantList folderIds;

            QMap<QMailFolder::StandardFolder, QMailFolderId>::const_iterator it = folders.begin(), end = folders.end();
            for ( ; it != end; ++it) {
                types.append(QVariant(static_cast<int>(it.key())));
                folderIds.append(QVariant(it.value().toULongLong()));
            }

            // Batch insert the folders
            QString sql("INSERT into mailaccountfolders (id,foldertype,folderid) VALUES (%1,?,?)");
            QSqlQuery query(batchQuery(sql.arg(QString::number(insertId.toULongLong())),
                                       QVariantList() << QVariant(types)
                                                      << QVariant(folderIds),
                                       "addAccount mailaccountfolders query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        // Insert any custom fields belonging to this account
        AttemptResult result = addCustomFields(insertId.toULongLong(), account->customFields(), "mailaccountcustom");
        if (result != Success)
            return result;
    }

    if (config) {
        foreach (const QString &service, config->services()) {
            QMailAccountConfiguration::ServiceConfiguration &serviceConfig(config->serviceConfiguration(service));
            const QMap<QString, QString> &fields = serviceConfig.values();

            QVariantList configFields;
            QVariantList configValues;

            // Insert any configuration fields belonging to this account
            QMap<QString, QString>::const_iterator it = fields.begin(), end = fields.end();
            for ( ; it != end; ++it) {
                configFields.append(QVariant(it.key()));
                configValues.append(QVariant(it.value()));
            }

            // Batch insert the custom fields
            QString sql("INSERT INTO mailaccountconfig (id,service,name,value) VALUES (%1,'%2',?,?)");
            QSqlQuery query(batchQuery(sql.arg(QString::number(insertId.toULongLong())).arg(service),
                                       QVariantList() << QVariant(configFields)
                                                      << QVariant(configValues),
                                       "addAccount mailaccountconfig query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        config->setId(insertId);
    }

    account->setId(insertId);

    if (commitOnSuccess && !t.commit()) {
        qMailLog(Messaging) << "Could not commit account changes to database";

        account->setId(QMailAccountId()); //revert the id
        return DatabaseFailure;
    }

    addedAccountIds->append(insertId);
    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptAddFolder(QMailFolder *folder, 
                                                                     QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                                                     Transaction &t, bool commitOnSuccess)
{   
    //check that the parent folder actually exists
    if (!checkPreconditions(*folder))
        return Failure;

    QMailFolderId insertId;

    {
        {
            QSqlQuery query(simpleQuery("INSERT INTO mailfolders (name,parentid,parentaccountid,displayname,status,servercount,serverunreadcount,serverundiscoveredcount) VALUES (?,?,?,?,?,?,?,?)",
                                        QVariantList() << folder->path()
                                                       << folder->parentFolderId().toULongLong()
                                                       << folder->parentAccountId().toULongLong()
                                                       << folder->displayName()
                                                       << folder->status()
                                                       << folder->serverCount()
                                                       << folder->serverUnreadCount()
                                                       << folder->serverUndiscoveredCount(),
                                        "addFolder mailfolders query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            // Extract the inserted id
            insertId = QMailFolderId(extractValue<quint64>(query.lastInsertId()));
        }

        // Insert any custom fields belonging to this folder
        AttemptResult result = addCustomFields(insertId.toULongLong(), folder->customFields(), "mailfoldercustom");
        if (result != Success)
            return result;
    }

    folder->setId(insertId);

    //create links to ancestor folders
    if (folder->parentFolderId().isValid()) {
        {
            //add records for each ancestor folder
            QSqlQuery query(simpleQuery("INSERT INTO mailfolderlinks "
                                        "SELECT DISTINCT id,? FROM mailfolderlinks WHERE descendantid=?",
                                        QVariantList() << folder->id().toULongLong() 
                                                       << folder->parentFolderId().toULongLong(),
                                        "mailfolderlinks insert ancestors"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        {
            // Our direct parent is also an ancestor
            QSqlQuery query(simpleQuery("INSERT INTO mailfolderlinks VALUES (?,?)",
                                        QVariantList() << folder->parentFolderId().toULongLong() 
                                                       << folder->id().toULongLong(),
                                        "mailfolderlinks insert parent"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }
    }

    if (commitOnSuccess && !t.commit()) {
        qMailLog(Messaging) << "Could not commit folder changes to database";

        folder->setId(QMailFolderId()); //revert the id
        return DatabaseFailure;
    }
   
    addedFolderIds->append(insertId);
    if (folder->parentAccountId().isValid())
        modifiedAccountIds->append(folder->parentAccountId());
    return Success;
}

struct ReferenceStorer
{
    QMailMessage *message;

    ReferenceStorer(QMailMessage *m) : message(m) {}

    bool operator()(const QMailMessagePart &part)
    {
        QString value;

        if (part.referenceType() == QMailMessagePart::MessageReference) {
            value = "message:" + QString::number(part.messageReference().toULongLong());
        } else if (part.referenceType() == QMailMessagePart::PartReference) {
            value = "part:" + part.partReference().toString(true);
        }

        if (!value.isEmpty()) {
            QString loc(part.location().toString(false));

            // Store the reference location into the message
            QString key("qtopiamail-reference-location-" + loc);
            if (message->customField(key) != value) {
                message->setCustomField(key, value);
            }

            // Store the reference resolution into the message
            key = "qtopiamail-reference-resolution-" + loc;
            value = part.referenceResolution();
            if (message->customField(key) != value) {
                message->setCustomField(key, value);
            }
        }

        return true;
    }
};

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptAddMessage(QMailMessage *message, const QString &identifier, const QStringList &references,
                                                                      QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                                                      Transaction &t, bool commitOnSuccess)
{
    if (!message->parentAccountId().isValid()) {
        // Require a parent account - possibly relax this later
        qMailLog(Messaging) << "Unable to add message without parent account";
        return Failure;
    }

    if (message->contentScheme().isEmpty()) {
        // Use the default storage scheme
        message->setContentScheme(defaultContentScheme());
    }

    MutexGuard lock(contentManagerMutex());
    if (!lock.lock(1000)) {
        qMailLog(Messaging) << "Unable to acquire message body mutex in addMessage!";
        return Failure;
    } 

    ReferenceStorer refStorer(message);
    const_cast<const QMailMessage*>(message)->foreachPart<ReferenceStorer&>(refStorer);

    if (QMailContentManager *contentManager = QMailContentManagerFactory::create(message->contentScheme())) {
        QMailStore::ErrorCode code = contentManager->add(message, durability(commitOnSuccess));
        if (code != QMailStore::NoError) {
            setLastError(code);
            qMailLog(Messaging) << "Unable to add message content to URI:" << ::contentUri(*message);
            return Failure;
        }

        AttemptResult result = attemptAddMessage(static_cast<QMailMessageMetaData*>(message), identifier, references, addedMessageIds, updatedMessageIds, modifiedFolderIds, modifiedAccountIds, t, commitOnSuccess);
        if (result != Success) {
            // Try to remove the content file we added
            QMailStore::ErrorCode code = contentManager->remove(message->contentIdentifier());
            if (code != QMailStore::NoError) {
                qMailLog(Messaging) << "Could not remove extraneous message content:" << ::contentUri(*message);
                if (code == QMailStore::ContentNotRemoved) {
                    // The existing content could not be removed - try again later
                    if (!obsoleteContent(message->contentIdentifier())) {
                        setLastError(QMailStore::FrameworkFault);
                    }
                } else {
                    setLastError(code);
                }
            }

            return result;
        }
    } else {
        qMailLog(Messaging) << "Unable to create content manager for scheme:" << message->contentScheme();
        return Failure;
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptAddMessage(QMailMessageMetaData *metaData, const QString &identifier, const QStringList &references,
                                                                      QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, 
                                                                      Transaction &t, bool commitOnSuccess)
{
    if (!metaData->parentFolderId().isValid()) {
        qMailLog(Messaging) << "Unable to add message. Invalid parent folder id";
        return Failure;
    }

    if (metaData->id().isValid() && idExists(metaData->id())) {
        qMailLog(Messaging) << "Message ID" << metaData->id() << "already exists in database, use update instead";
        return Failure;
    }

    bool replyOrForward(false);
    QString baseSubject(QMail::baseSubject(metaData->subject(), &replyOrForward));
    QStringList missingReferences;
    bool missingAncestor(false);

    if (!metaData->inResponseTo().isValid()) {
        // Does this message have any references to resolve?
        AttemptResult result = messagePredecessor(metaData, references, baseSubject, replyOrForward, &missingReferences, &missingAncestor);
        if (result != Success)
            return result;
    }

    // Ensure that any phone numbers are added in minimal form
    QMailAddress from(metaData->from());
    QString fromText(from.isPhoneNumber() ? from.minimalPhoneNumber() : from.toString());

    QStringList recipients;
    foreach (const QMailAddress& address, metaData->to())
        recipients.append(address.isPhoneNumber() ? address.minimalPhoneNumber() : address.toString());

    quint64 insertId;

    {
        QMap<QString, QVariant> values;

        values.insert("type", static_cast<int>(metaData->messageType()));
        values.insert("parentfolderid", metaData->parentFolderId().toULongLong());
        values.insert("sender", fromText);
        values.insert("recipients", recipients.join(","));
        values.insert("subject", metaData->subject());
        values.insert("stamp", QMailTimeStamp(metaData->date()).toLocalTime());
        values.insert("status", static_cast<int>(metaData->status()));
        values.insert("parentaccountid", metaData->parentAccountId().toULongLong());
        values.insert("mailfile", ::contentUri(*metaData));
        values.insert("serveruid", metaData->serverUid());
        values.insert("size", metaData->size());
        values.insert("contenttype", static_cast<int>(metaData->content()));
        values.insert("responseid", metaData->inResponseTo().toULongLong());
        values.insert("responsetype", metaData->responseType());
        values.insert("receivedstamp", QMailTimeStamp(metaData->receivedDate()).toLocalTime());
        if (metaData->previousParentFolderId().isValid()) {
            values.insert("previousparentfolderid", metaData->previousParentFolderId().toULongLong());
        }

        const QStringList &list(values.keys());
        QString columns = list.join(",");

        // Add the record to the mailmessages table
        QSqlQuery query(simpleQuery(QString("INSERT INTO mailmessages (%1) VALUES %2").arg(columns).arg(expandValueList(values.count())),
                                    values.values(),
                                    "addMessage mailmessages query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        //retrieve the insert id
        insertId = extractValue<quint64>(query.lastInsertId());
    }

    // Insert any custom fields belonging to this message
    AttemptResult result = addCustomFields(insertId, metaData->customFields(), "mailmessagecustom");
    if (result != Success)
        return result;

    // Attach this message to a thread
    if (metaData->inResponseTo().isValid()) {
        // Use the thread of the parent message
        QSqlQuery query(simpleQuery("INSERT INTO mailthreadmessages (threadid,messageid) SELECT threadid,? FROM mailthreadmessages WHERE messageid=?",
                                    QVariantList() << insertId << metaData->inResponseTo().toULongLong(),
                                    "addMessage mailthreadmessages insert query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    } else {
        // Add a new thread for this message
        quint64 threadId;

        {
            QSqlQuery query(simpleQuery("INSERT INTO mailthreads (id) SELECT COALESCE(MAX(id),0) + 1 FROM mailthreads",
                                        "addMessage mailthreads insert query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            threadId = extractValue<quint64>(query.lastInsertId());
        }

        {
            QSqlQuery query(simpleQuery("INSERT INTO mailthreadmessages (threadid,messageid) VALUES (?,?)",
                                        QVariantList() << threadId << insertId,
                                        "addMessage mailthreads insert query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }
    }

    if (!baseSubject.isEmpty()) {
        // Ensure that this subject is in the subjects table
        result = registerSubject(baseSubject, insertId, metaData->inResponseTo(), missingAncestor);
        if (result != Success)
            return result;
    }

    // Does this message have any identifier?
    if (!identifier.isEmpty()) {
        QSqlQuery query(simpleQuery("INSERT INTO mailmessageidentifiers (id,identifier) VALUES (?,?)",
                                    QVariantList() << insertId << identifier,
                                    "addMessage mailmessageidentifiers query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    }

    // See if this message resolves any missing message items
    result = resolveMissingMessages(identifier, metaData->inResponseTo(), baseSubject, insertId, updatedMessageIds);
    if (result != Success)
        return result;

    if (!updatedMessageIds->isEmpty()) {
        // Find the set of folders and accounts whose contents are modified by these messages
        QMailMessageKey modifiedMessageKey(QMailMessageKey::id(*updatedMessageIds));
        result = affectedByMessageIds(*updatedMessageIds, modifiedFolderIds, modifiedAccountIds);
        if (result != Success)
            return result;
    }

    if (!missingReferences.isEmpty()) {
        // Add the missing references to the missing messages table
        QVariantList refs;
        QVariantList levels;

        int level = missingReferences.count();
        foreach (const QString &ref, missingReferences) {
            refs.append(QVariant(ref));
            levels.append(QVariant(--level));
        }

        QString sql("INSERT INTO missingmessages (id,identifier,level) VALUES (%1,?,?)");
        QSqlQuery query(batchQuery(sql.arg(QString::number(insertId)),
                                   QVariantList() << QVariant(refs) << QVariant(levels),
                                   "addMessage missingmessages insert query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    }

    // Find the complete set of modified folders, including ancestor folders
    QMailFolderIdList folderIds;
    folderIds.append(metaData->parentFolderId());
    folderIds += folderAncestorIds(folderIds, true, &result);
    if (result != Success)
        return result;

    if (commitOnSuccess && !t.commit()) {
        qMailLog(Messaging) << "Could not commit message changes to database";
        return DatabaseFailure;
    }

    metaData->setId(QMailMessageId(insertId));
    addedMessageIds->append(metaData->id());
    *modifiedFolderIds = folderIds;
    if (metaData->parentAccountId().isValid())
        modifiedAccountIds->append(metaData->parentAccountId());
    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptRemoveAccounts(const QMailAccountKey &key, 
                                                                          QMailAccountIdList *deletedAccountIds, QMailFolderIdList *deletedFolderIds, QMailMessageIdList *deletedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                                                          Transaction &t, bool commitOnSuccess)
{
    QStringList expiredContent;

    if (deleteAccounts(key, *deletedAccountIds, *deletedFolderIds, *deletedMessageIds, expiredContent, *updatedMessageIds, *modifiedFolderIds, *modifiedAccountIds)) {
        if (commitOnSuccess && t.commit()) {
            //remove deleted objects from caches
            removeExpiredData(*deletedMessageIds, expiredContent, *deletedFolderIds, *deletedAccountIds);
            return Success;
        }
    }

    return DatabaseFailure;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptRemoveFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option, 
                                                                         QMailFolderIdList *deletedFolderIds, QMailMessageIdList *deletedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                                                         Transaction &t, bool commitOnSuccess)
{
    QStringList expiredContent;

    if (deleteFolders(key, option, *deletedFolderIds, *deletedMessageIds, expiredContent, *updatedMessageIds, *modifiedFolderIds, *modifiedAccountIds)) {
        if (commitOnSuccess && t.commit()) {
            //remove deleted objects from caches
            removeExpiredData(*deletedMessageIds, expiredContent, *deletedFolderIds);
            return Success;
        }
    }

    return DatabaseFailure;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptRemoveMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option, 
                                                                          QMailMessageIdList *deletedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                                                          Transaction &t, bool commitOnSuccess)
{
    QStringList expiredContent;

    if (deleteMessages(key, option, *deletedMessageIds, expiredContent, *updatedMessageIds, *modifiedFolderIds, *modifiedAccountIds)) {
        if (commitOnSuccess && t.commit()) {
            //remove deleted objects from caches
            removeExpiredData(*deletedMessageIds, expiredContent);
            return Success;
        }
    }

    return DatabaseFailure;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptUpdateAccount(QMailAccount *account, QMailAccountConfiguration *config, 
                                                                         QMailAccountIdList *updatedAccountIds,
                                                                         Transaction &t, bool commitOnSuccess)
{
    QMailAccountId id(account ? account->id() : config ? config->id() : QMailAccountId());
    if (!id.isValid())
        return Failure;

    if (account) {
        QString properties("type=?, name=?, status=?, signature=?, emailaddress=?");
        QVariantList propertyValues;
        propertyValues << static_cast<int>(account->messageType()) 
                       << account->name() 
                       << account->status()
                       << account->signature()
                       << account->fromAddress().toString(true);

        {
            QSqlQuery query(simpleQuery(QString("UPDATE mailaccounts SET %1 WHERE id=?").arg(properties),
                                        propertyValues << id.toULongLong(),
                                        "updateAccount mailaccounts query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        // Update any standard folders configured
        const QMap<QMailFolder::StandardFolder, QMailFolderId> &folders(account->standardFolders());
        QMap<QMailFolder::StandardFolder, QMailFolderId> existingFolders;

        {
            // Find the existing folders
            QSqlQuery query(simpleQuery("SELECT foldertype,folderid FROM mailaccountfolders WHERE id=?", 
                                        QVariantList() << id.toULongLong(),
                                        "updateAccount mailaccountfolders select query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            while (query.next())
                existingFolders.insert(QMailFolder::StandardFolder(query.value(0).toInt()), QMailFolderId(query.value(1).toULongLong()));
        }

        QVariantList obsoleteTypes;
        QVariantList modifiedTypes;
        QVariantList modifiedFolderIds;
        QVariantList addedTypes;
        QVariantList addedFolders;

        // Compare the sets
        QMap<QMailFolder::StandardFolder, QMailFolderId>::const_iterator fend = folders.end(), eend = existingFolders.end();
        QMap<QMailFolder::StandardFolder, QMailFolderId>::const_iterator it = existingFolders.begin();
        for ( ; it != eend; ++it) {
            QMap<QMailFolder::StandardFolder, QMailFolderId>::const_iterator current = folders.find(it.key());
            if (current == fend) {
                obsoleteTypes.append(QVariant(static_cast<int>(it.key())));
            } else if (*current != *it) {
                modifiedTypes.append(QVariant(static_cast<int>(current.key())));
                modifiedFolderIds.append(QVariant(current.value().toULongLong()));
            }
        }

        for (it = folders.begin(); it != fend; ++it) {
            if (existingFolders.find(it.key()) == eend) {
                addedTypes.append(QVariant(static_cast<int>(it.key())));
                addedFolders.append(QVariant(it.value().toULongLong()));
            }
        }

        if (!obsoleteTypes.isEmpty()) {
            // Remove the obsolete folders
            QString sql("DELETE FROM mailaccountfolders WHERE id=? AND foldertype IN %2");
            QSqlQuery query(simpleQuery(sql.arg(expandValueList(obsoleteTypes)),
                                        QVariantList() << id.toULongLong() << obsoleteTypes,
                                        "updateAccount mailaccountfolders delete query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        if (!modifiedTypes.isEmpty()) {
            // Batch update the modified folders
            QString sql("UPDATE mailaccountfolders SET folderid=? WHERE id=%2 AND foldertype=?");
            QSqlQuery query(batchQuery(sql.arg(QString::number(id.toULongLong())),
                                       QVariantList() << QVariant(modifiedFolderIds)
                                                      << QVariant(modifiedTypes),
                                       "updateAccount mailaccountfolders update query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        if (!addedTypes.isEmpty()) {
            // Batch insert the added fields
            QString sql("INSERT INTO mailaccountfolders (id,foldertype,folderid) VALUES (%2,?,?)");
            QSqlQuery query(batchQuery(sql.arg(QString::number(id.toULongLong())),
                                       QVariantList() << QVariant(addedTypes)
                                                      << QVariant(addedFolders),
                                       "updateAccount mailaccountfolders insert query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        if (account->customFieldsModified()) {
            AttemptResult result = updateCustomFields(id.toULongLong(), account->customFields(), "mailaccountcustom");
            if (result != Success)
                return result;
        }
    }

    if (config) {
        // Find the complete set of configuration fields
        QMap<QPair<QString, QString>, QString> fields;

        foreach (const QString &service, config->services()) {
            QMailAccountConfiguration::ServiceConfiguration &serviceConfig(config->serviceConfiguration(service));
            const QMap<QString, QString> &values = serviceConfig.values();

            // Insert any configuration fields belonging to this account
            QMap<QString, QString>::const_iterator it = values.begin(), end = values.end();
            for ( ; it != end; ++it)
                fields.insert(qMakePair(service, it.key()), it.value());
        }

        // Find the existing fields in the database
        QMap<QPair<QString, QString>, QString> existing;

        {
            QSqlQuery query(simpleQuery("SELECT service,name,value FROM mailaccountconfig WHERE id=?",
                                        QVariantList() << id.toULongLong(),
                                        "updateAccount mailaccountconfig select query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            while (query.next())
                existing.insert(qMakePair(query.value(0).toString(), query.value(1).toString()), query.value(2).toString());
        }

        QMap<QString, QVariantList> obsoleteFields;
        QMap<QString, QVariantList> modifiedFields;
        QMap<QString, QVariantList> modifiedValues;
        QMap<QString, QVariantList> addedFields;
        QMap<QString, QVariantList> addedValues;

        // Compare the sets
        QMap<QPair<QString, QString>, QString>::const_iterator fend = fields.end(), eend = existing.end();
        QMap<QPair<QString, QString>, QString>::const_iterator it = existing.begin();
        for ( ; it != eend; ++it) {
            const QPair<QString, QString> &name = it.key();
            QMap<QPair<QString, QString>, QString>::const_iterator current = fields.find(name);
            if (current == fend) {
                obsoleteFields[name.first].append(QVariant(name.second));
            } else if (*current != *it) {
                modifiedFields[name.first].append(QVariant(name.second));
                modifiedValues[name.first].append(QVariant(current.value()));
            }
        }

        for (it = fields.begin(); it != fend; ++it) {
            const QPair<QString, QString> &name = it.key();
            if (existing.find(name) == eend) {
                addedFields[name.first].append(QVariant(name.second));
                addedValues[name.first].append(QVariant(it.value()));
            }
        }

        if (!obsoleteFields.isEmpty()) {
            // Remove the obsolete fields
            QMap<QString, QVariantList>::const_iterator it = obsoleteFields.begin(), end = obsoleteFields.end();
            for ( ; it != end; ++it) {
                const QString &service = it.key();
                const QVariantList &fields = it.value();
                
                QString sql("DELETE FROM mailaccountconfig WHERE id=? AND service='%1' AND name IN %2");
                QSqlQuery query(simpleQuery(sql.arg(service).arg(expandValueList(fields)),
                                            QVariantList() << id.toULongLong() << fields,
                                            "updateAccount mailaccountconfig delete query"));
                if (query.lastError().type() != QSqlError::NoError)
                    return DatabaseFailure;
            }
        }

        if (!modifiedFields.isEmpty()) {
            // Batch update the modified fields
            QMap<QString, QVariantList>::const_iterator it = modifiedFields.begin(), end = modifiedFields.end();
            for (QMap<QString, QVariantList>::const_iterator vit = modifiedValues.begin(); it != end; ++it, ++vit) {
                const QString &service = it.key();
                const QVariantList &fields = it.value();
                const QVariantList &values = vit.value();
                
                QString sql("UPDATE mailaccountconfig SET value=? WHERE id=%1 AND service='%2' AND name=?");
                QSqlQuery query(batchQuery(sql.arg(QString::number(id.toULongLong())).arg(service),
                                           QVariantList() << QVariant(values) << QVariant(fields),
                                           "updateAccount mailaccountconfig update query"));
                if (query.lastError().type() != QSqlError::NoError)
                    return DatabaseFailure;
            }
        }

        if (!addedFields.isEmpty()) {
            // Batch insert the added fields
            QMap<QString, QVariantList>::const_iterator it = addedFields.begin(), end = addedFields.end();
            for (QMap<QString, QVariantList>::const_iterator vit = addedValues.begin(); it != end; ++it, ++vit) {
                const QString &service = it.key();
                const QVariantList &fields = it.value();
                const QVariantList &values = vit.value();
                
                QString sql("INSERT INTO mailaccountconfig (id,service,name,value) VALUES (%1,'%2',?,?)");
                QSqlQuery query(batchQuery(sql.arg(QString::number(id.toULongLong())).arg(service),
                                           QVariantList() << QVariant(fields) << QVariant(values),
                                           "updateAccount mailaccountconfig insert query"));
                if (query.lastError().type() != QSqlError::NoError)
                    return DatabaseFailure;
            }
        }
    }

    if (commitOnSuccess && !t.commit()) {
        qMailLog(Messaging) << "Could not commit account update to database";
        return DatabaseFailure;
    }
        
    if (account) {
        // Update the account cache
        if (accountCache.contains(id))
            accountCache.insert(*account);
    }

    updatedAccountIds->append(id);
    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptUpdateFolder(QMailFolder *folder, 
                                                                        QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                                                        Transaction &t, bool commitOnSuccess)
{
    //check that the parent folder actually exists
    if(!checkPreconditions(*folder, true))
        return Failure;

    QMailFolderId parentFolderId;
    QMailAccountId parentAccountId;

    {
        //find the current parent folder
        QSqlQuery query(simpleQuery("SELECT parentid, parentaccountid FROM mailfolders WHERE id=?",
                                    QVariantList() << folder->id().toULongLong(),
                                    "mailfolder parent query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        if (query.first()) {
            parentFolderId = QMailFolderId(extractValue<quint64>(query.value(0)));
            parentAccountId = QMailAccountId(extractValue<quint64>(query.value(1)));
        }
    }

    {
        QSqlQuery query(simpleQuery("UPDATE mailfolders SET name=?,parentid=?,parentaccountid=?,displayname=?,status=?,servercount=?,serverunreadcount=?,serverundiscoveredcount=? WHERE id=?",
                                    QVariantList() << folder->path()
                                                   << folder->parentFolderId().toULongLong()
                                                   << folder->parentAccountId().toULongLong()
                                                   << folder->displayName()
                                                   << folder->status()
                                                   << folder->serverCount()
                                                   << folder->serverUnreadCount()
                                                   << folder->serverUndiscoveredCount()
                                                   << folder->id().toULongLong(),
                                    "updateFolder mailfolders query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    }
    
    if (folder->customFieldsModified()) {
        AttemptResult result = updateCustomFields(folder->id().toULongLong(), folder->customFields(), "mailfoldercustom");
        if (result != Success)
            return result;
    }

    if (parentFolderId != folder->parentFolderId()) {
        // QMailAccount contains a copy of the folder data; we need to tell it to reload
        if (parentFolderId.isValid())
            modifiedAccountIds->append(parentAccountId);
        if (folder->parentFolderId().isValid() && !modifiedAccountIds->contains(folder->parentAccountId()))
            modifiedAccountIds->append(folder->parentAccountId());

        {
            //remove existing links to this folder
            QSqlQuery query(simpleQuery("DELETE FROM mailfolderlinks WHERE descendantid = ?",
                                        QVariantList() << folder->id().toULongLong(),
                                        "mailfolderlinks delete in update"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        {
            //add links to the new parent
            QSqlQuery query(simpleQuery("INSERT INTO mailfolderlinks "
                                        "SELECT DISTINCT id,? FROM mailfolderlinks WHERE descendantid=?",
                                        QVariantList() << folder->id().toULongLong() 
                                                       << folder->parentFolderId().toULongLong(),
                                        "mailfolderlinks insert ancestors"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        {
            QSqlQuery query(simpleQuery("INSERT INTO mailfolderlinks VALUES (?,?)",
                                        QVariantList() << folder->parentFolderId().toULongLong()
                                                       << folder->id().toULongLong(),
                                        "mailfolderlinks insert parent"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }
    }
        
    if (commitOnSuccess && !t.commit()) {
        qMailLog(Messaging) << "Could not commit folder update to database";
        return DatabaseFailure;
    }

    //update the folder cache
    if (folderCache.contains(folder->id()))
        folderCache.insert(*folder);

    updatedFolderIds->append(folder->id());
    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptUpdateMessage(QMailMessageMetaData *metaData, QMailMessage *message, 
                                                                         QMailMessageIdList *updatedMessageIds, QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                                                         Transaction &t, bool commitOnSuccess)
{
    if (!metaData->id().isValid())
        return Failure;

    quint64 updateId = metaData->id().toULongLong();

    QMailAccountId parentAccountId;
    QMailFolderId parentFolderId;
    QMailMessageId responseId;
    QString contentUri;
    QMailFolderIdList folderIds;

    QMailMessageKey::Properties updateProperties;
    QVariantList extractedValues;

    if (message) {
        // Ensure the part reference info is stored into the message
        ReferenceStorer refStorer(message);
        const_cast<const QMailMessage*>(message)->foreachPart<ReferenceStorer&>(refStorer);
    }

    if (metaData->dataModified()) {
        // Assume all the meta data fields have been updated
        updateProperties = QMailStorePrivate::updatableMessageProperties();
    }

    // Do we actually have an update to perform?
    bool updateContent(message && message->contentModified());
    if (metaData->dataModified() || updateContent) {
        // Find the existing properties 
        {
            QSqlQuery query(simpleQuery("SELECT parentaccountid,parentfolderid,responseid,mailfile FROM mailmessages WHERE id=?",
                                        QVariantList() << updateId,
                                        "updateMessage existing properties query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            if (query.first()) {
                parentAccountId = QMailAccountId(extractValue<quint64>(query.value(0)));
                parentFolderId = QMailFolderId(extractValue<quint64>(query.value(1)));
                responseId = QMailMessageId(extractValue<quint64>(query.value(2)));
                contentUri = extractValue<QString>(query.value(3));

                // Find any folders affected by this update
                folderIds.append(metaData->parentFolderId());
                if (parentFolderId != metaData->parentFolderId()) {
                    // The previous location folder has also changed
                    folderIds.append(parentFolderId);
                    metaData->setPreviousParentFolderId(parentFolderId);
                }

                // Ancestor folders are also considered to be affected
                AttemptResult result;
                folderIds += folderAncestorIds(folderIds, true, &result);
                if (result != Success)
                    return result;
            } else {
                qMailLog(Messaging) << "Could not query parent account, folder and content URI";
                return Failure;
            }
        }

        bool replyOrForward(false);
        QString baseSubject(QMail::baseSubject(metaData->subject(), &replyOrForward));
        QStringList missingReferences;
        bool missingAncestor(false);

        if (updateContent || (message && (!metaData->inResponseTo().isValid() || (metaData->inResponseTo() != responseId)))) {
            // Does this message have any references to resolve?
            QStringList references(identifierValues(message->headerFieldText("References")));
            QString predecessor(identifierValue(message->headerFieldText("In-Reply-To")));
            if (!predecessor.isEmpty()) {
                if (references.isEmpty() || (references.last() != predecessor)) {
                    references.append(predecessor);
                }
            }

            AttemptResult result = messagePredecessor(metaData, references, baseSubject, replyOrForward, &missingReferences, &missingAncestor);
            if (result != Success)
                return result;
        }

        if (updateContent) {
            updateProperties |= QMailMessageKey::ContentIdentifier;

            bool addContent(updateContent && contentUri.isEmpty());
            if (addContent)
                updateProperties |= QMailMessageKey::ContentScheme;

            // We need to update the content for this message
            if (metaData->contentScheme().isEmpty()) {
                // Use the default storage scheme
                metaData->setContentScheme(defaultContentScheme());
            }

            MutexGuard lock(contentManagerMutex());
            if (!lock.lock(1000)) {
                qMailLog(Messaging) << "Unable to acquire message body mutex in updateMessage!";
                return Failure;
            } 

            if (QMailContentManager *contentManager = QMailContentManagerFactory::create(metaData->contentScheme())) {
                QString contentUri(::contentUri(*metaData));

                if (addContent) {
                    // We need to add this content to the message
                    QMailStore::ErrorCode code = contentManager->add(message, durability(commitOnSuccess));
                    if (code != QMailStore::NoError) {
                        setLastError(code);
                        qMailLog(Messaging) << "Unable to add message content to URI:" << contentUri;
                        return Failure;
                    }
                } else {
                    QMailStore::ErrorCode code = contentManager->update(message, durability(commitOnSuccess));
                    if (code != QMailStore::NoError) {
                        qMailLog(Messaging) << "Unable to update message content:" << contentUri;
                        if (code == QMailStore::ContentNotRemoved) {
                            // The existing content could not be removed - try again later
                            if (!obsoleteContent(contentUri)) {
                                setLastError(QMailStore::FrameworkFault);
                                return Failure;
                            }
                        } else {
                            setLastError(code);
                            return Failure;
                        }
                    }
                }

                metaData->setContentIdentifier(message->contentIdentifier());
            } else {
                qMailLog(Messaging) << "Unable to create content manager for scheme:" << metaData->contentScheme();
                return Failure;
            }
        }

        if (metaData->inResponseTo() != responseId) {
            // We need to record this change
            updateProperties |= QMailMessageKey::InResponseTo;
            updateProperties |= QMailMessageKey::ResponseType;

            // Join this message's thread to the predecessor's thread
            quint64 threadId;

            if (metaData->inResponseTo().isValid()) {
                {
                    QSqlQuery query(simpleQuery("SELECT threadid FROM mailthreadmessages WHERE messageid=?",
                                                QVariantList() << updateId,
                                                "updateMessage mailthreadmessages query"));
                    if (query.lastError().type() != QSqlError::NoError)
                        return DatabaseFailure;

                    if (query.first()) {
                        threadId = extractValue<quint64>(query.value(0));
                    }
                }

                {
                    QSqlQuery query(simpleQuery("UPDATE mailthreadmessages SET threadid=(SELECT threadid FROM mailthreadmessages WHERE messageid=?) WHERE threadid=?",
                                                QVariantList() << metaData->inResponseTo().toULongLong() << threadId,
                                                "updateMessage mailthreadmessages update query"));
                    if (query.lastError().type() != QSqlError::NoError)
                        return DatabaseFailure;
                }

                {
                    QSqlQuery query(simpleQuery("DELETE FROM mailthreads WHERE id=?",
                                                QVariantList() << threadId,
                                                "updateMessage mailthreads delete query"));
                    if (query.lastError().type() != QSqlError::NoError)
                        return DatabaseFailure;
                }
            } else {
                // This message is no longer associated with the thread of the former predecessor
                QMailMessageIdList descendantIds;

                {
                    QMailMessageIdList parentIds;

                    parentIds.append(QMailMessageId(updateId));

                    // Find all descendants of this message
                    while (!parentIds.isEmpty()) {
                        QSqlQuery query(simpleQuery("SELECT id FROM mailmessages",
                                                    Key("responseid", QMailMessageKey::id(parentIds)),
                                                    "updateMessage mailmessages responseid query"));
                        if (query.lastError().type() != QSqlError::NoError)
                            return DatabaseFailure;

                        while (!parentIds.isEmpty()) {
                            descendantIds.append(parentIds.takeFirst());
                        }

                        while (query.next()) {
                            parentIds.append(QMailMessageId(extractValue<quint64>(query.value(0))));
                        }
                    }
                }

                {
                    // Allocate a new thread for this message
                    QSqlQuery query(simpleQuery("INSERT INTO mailthreads (id) SELECT COALESCE(MAX(id),0) + 1 FROM mailthreads",
                                                "updateMessage mailthreads insert query"));
                    if (query.lastError().type() != QSqlError::NoError)
                        return DatabaseFailure;

                    threadId = extractValue<quint64>(query.lastInsertId());
                }

                {
                    // Migrate descendants to the new thread
                    QSqlQuery query(simpleQuery("UPDATE mailthreadmessages SET threadid=?",
                                                QVariantList() << threadId,
                                                Key("messageid", QMailMessageKey::id(descendantIds)),
                                                "updateMessage mailthreadmessages update query"));
                    if (query.lastError().type() != QSqlError::NoError)
                        return DatabaseFailure;
                }
            }
            
            // Remove any missing message/ancestor references associated with this message

            {
                QSqlQuery query(simpleQuery("DELETE FROM missingmessages WHERE id=?",
                                            QVariantList() << updateId,
                                            "updateMessage missingmessages delete query"));
                if (query.lastError().type() != QSqlError::NoError)
                    return DatabaseFailure;
            }

            {
                QSqlQuery query(simpleQuery("DELETE FROM missingancestors WHERE messageid=?",
                                            QVariantList() << updateId,
                                            "updateMessage missingancestors delete query"));
                if (query.lastError().type() != QSqlError::NoError)
                    return DatabaseFailure;
            }
        }

        // Don't update the previous parent folder if it isn't set
        if (!metaData->previousParentFolderId().isValid())
            updateProperties &= ~QMailMessageKey::PreviousParentFolderId;

        if (updateProperties != QMailMessageKey::Properties()) {
            extractedValues = messageValues(updateProperties, *metaData);

            QString sql("UPDATE mailmessages SET %1 WHERE id=?");
            QSqlQuery query(simpleQuery(sql.arg(expandProperties(updateProperties, true)),
                                        extractedValues + (QVariantList() << updateId),
                                        "updateMessage mailmessages update"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        if (metaData->customFieldsModified()) {
            AttemptResult result = updateCustomFields(updateId, metaData->customFields(), "mailmessagecustom");
            if (result != Success)
                return result;

            updateProperties |= QMailMessageKey::Custom;
        }

        if (updateProperties & QMailMessageKey::Subject) {
            if (!baseSubject.isEmpty()) {
                // Ensure that this subject is in the subjects table
                AttemptResult result = registerSubject(baseSubject, updateId, metaData->inResponseTo(), missingAncestor);
                if (result != Success)
                    return result;
            }
        }

        bool updatedIdentifier(false);
        QString messageIdentifier;

        if (updateContent) {
            // We may have a change in the message identifier
            QString existingIdentifier;

            {
                QSqlQuery query(simpleQuery("SELECT identifier FROM mailmessageidentifiers WHERE id=?",
                                            QVariantList() << updateId,
                                            "updateMessage existing identifier query"));
                if (query.lastError().type() != QSqlError::NoError)
                    return DatabaseFailure;

                if (query.first()) {
                    existingIdentifier = extractValue<QString>(query.value(0));
                }
            }

            messageIdentifier = identifierValue(message->headerFieldText("Message-ID"));

            if (messageIdentifier != existingIdentifier) {
                if (!messageIdentifier.isEmpty()) {
                    updatedIdentifier = true;

                    if (!existingIdentifier.isEmpty()) {
                        QSqlQuery query(simpleQuery("UPDATE mailmessageidentifiers SET identifier=? WHERE id=?",
                                                    QVariantList() << messageIdentifier << updateId,
                                                    "updateMessage mailmessageidentifiers update query"));
                        if (query.lastError().type() != QSqlError::NoError)
                            return DatabaseFailure;
                    } else {
                        // Add the new value
                        QSqlQuery query(simpleQuery("INSERT INTO mailmessageidentifiers (id,identifier) VALUES (?,?)",
                                                    QVariantList() << updateId << messageIdentifier,
                                                    "updateMessage mailmessageidentifiers insert query"));
                        if (query.lastError().type() != QSqlError::NoError)
                            return DatabaseFailure;
                    }
                } else {
                    if (!existingIdentifier.isEmpty()) {
                        // Remove any existing value
                        QSqlQuery query(simpleQuery("DELETE FROM mailmessageidentifiers WHERE id=?",
                                                    QVariantList() << updateId,
                                                    "updateMessage mailmessageidentifiers delete query"));
                        if (query.lastError().type() != QSqlError::NoError)
                            return DatabaseFailure;
                    }
                }
            }

            if (!missingReferences.isEmpty()) {
                // Add the missing references to the missing messages table
                QVariantList refs;
                QVariantList levels;

                int level = missingReferences.count();
                foreach (const QString &ref, missingReferences) {
                    refs.append(QVariant(ref));
                    levels.append(QVariant(--level));
                }

                {
                    QSqlQuery query(simpleQuery("DELETE FROM missingmessages WHERE id=?",
                                                QVariantList() << updateId,
                                                "addMessage missingmessages delete query"));
                    if (query.lastError().type() != QSqlError::NoError)
                        return DatabaseFailure;
                }

                {
                    QString sql("INSERT INTO missingmessages (id,identifier,level) VALUES (%1,?,?)");
                    QSqlQuery query(batchQuery(sql.arg(QString::number(updateId)),
                                               QVariantList() << QVariant(refs) << QVariant(levels),
                                               "addMessage missingmessages insert query"));
                    if (query.lastError().type() != QSqlError::NoError)
                        return DatabaseFailure;
                }
            }
        }

        if (updatedIdentifier || (updateProperties & QMailMessageKey::InResponseTo)) {
            // See if this message resolves any missing message items
            AttemptResult result = resolveMissingMessages(messageIdentifier, metaData->inResponseTo(), baseSubject, updateId, updatedMessageIds);
            if (result != Success)
                return result;

            if (!updatedMessageIds->isEmpty()) {
                // Find the set of folders and accounts whose contents are modified by these messages
                QMailMessageKey modifiedMessageKey(QMailMessageKey::id(*updatedMessageIds));
                result = affectedByMessageIds(*updatedMessageIds, modifiedFolderIds, modifiedAccountIds);
                if (result != Success)
                    return result;
            }
        }
    }

    if (commitOnSuccess && !t.commit()) {
        qMailLog(Messaging) << "Could not commit message update to database";
        return DatabaseFailure;
    }

    if (parentAccountId.isValid()) {
        // The message is now up-to-date with data store
        metaData->setUnmodified();

        if (messageCache.contains(metaData->id())) {
            QMailMessageMetaData cachedMetaData = messageCache.lookup(metaData->id());
            if (!extractedValues.isEmpty()) {
                // Update the cache with the modifications we recorded
                updateMessageValues(updateProperties, extractedValues, metaData->customFields(), cachedMetaData);
                cachedMetaData.setUnmodified();
                messageCache.insert(cachedMetaData);
            }
            uidCache.insert(qMakePair(cachedMetaData.parentAccountId(), cachedMetaData.serverUid()), cachedMetaData.id());
        }

        updatedMessageIds->append(metaData->id());
        *modifiedFolderIds = folderIds;

        if (metaData->parentAccountId().isValid())
            modifiedAccountIds->append(metaData->parentAccountId());
        if (parentAccountId.isValid()) {
            if (parentAccountId != metaData->parentAccountId())
                modifiedAccountIds->append(parentAccountId);
        }
    }

    if (updateContent) {
        modifiedMessageIds->append(metaData->id());
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptUpdateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &props, const QMailMessageMetaData &data, 
                                                                                  QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                                                                  Transaction &t, bool commitOnSuccess) 
{
    //do some checks first
    if (props & QMailMessageKey::Id) {
        qMailLog(Messaging) << "Updating of messages IDs is not supported";
        return Failure;
    }
    
    QMailMessageKey::Properties properties(props);

    if (properties & QMailMessageKey::ParentFolderId) {
        if (!idExists(data.parentFolderId())) {
            qMailLog(Messaging) << "Update of messages failed. Parent folder does not exist";
            return Failure;
        }
    }

    QVariantList extractedValues;

    //get the valid ids
    *updatedMessageIds = queryMessages(key, QMailMessageSortKey(), 0, 0);
    if (!updatedMessageIds->isEmpty()) {
        // Find the set of folders and accounts whose contents are modified by this update
        QMailMessageKey modifiedMessageKey(QMailMessageKey::id(*updatedMessageIds));
        AttemptResult result = affectedByMessageIds(*updatedMessageIds, modifiedFolderIds, modifiedAccountIds);
        if (result != Success)
            return result;

        // If we're setting parentFolderId, that folder is modified also
        if (properties & QMailMessageKey::ParentFolderId) {
            if (!modifiedFolderIds->contains(data.parentFolderId()))
                modifiedFolderIds->append(data.parentFolderId());

            // All these messages need to have previousparentfolderid updated, where it will change
            QSqlQuery query(simpleQuery("UPDATE mailmessages SET previousparentfolderid=parentfolderid",
                                        QVariantList(),
                                        QList<Key>() << Key(modifiedMessageKey),
                                        "updateMessagesMetaData mailmessages previousparentfolderid update query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        if (properties & QMailMessageKey::Custom) {
            // Here, we can't compare the input to each target individually.  Instead, remove
            // all custom fields from the affected messages, and add (or re-add) the new ones
            QVariantList addedFields;
            QVariantList addedValues;

            const QMap<QString, QString> &fields = data.customFields();
            QMap<QString, QString>::const_iterator it = fields.begin(), end = fields.end();
            for ( ; it != end; ++it) {
                addedFields.append(QVariant(it.key()));
                addedValues.append(QVariant(it.value()));
            }

            {
                // Remove the obsolete fields
                QSqlQuery query(simpleQuery("DELETE FROM mailmessagecustom",
                                            Key(modifiedMessageKey),
                                            "updateMessagesMetaData mailmessagecustom delete query"));
                if (query.lastError().type() != QSqlError::NoError)
                    return DatabaseFailure;
            }

            if (!addedFields.isEmpty()) {
                foreach (const QMailMessageId &id, *updatedMessageIds) {
                    // Batch insert the added fields
                    QString sql("INSERT INTO mailmessagecustom (id,name,value) VALUES (%1,?,?)");
                    QSqlQuery query(batchQuery(sql.arg(QString::number(id.toULongLong())),
                                               QVariantList() << QVariant(addedFields)
                                                              << QVariant(addedValues),
                                               "updateMessagesMetaData mailmessagecustom insert query"));
                    if (query.lastError().type() != QSqlError::NoError)
                        return DatabaseFailure;
                }
            }

            properties &= ~QMailMessageKey::Custom;
        }

        if (properties != 0) {
            extractedValues = messageValues(properties, data);

            QString sql("UPDATE mailmessages SET %1");
            QSqlQuery query(simpleQuery(sql.arg(expandProperties(properties, true)),
                                        extractedValues,
                                        Key(modifiedMessageKey),
                                        "updateMessagesMetaData mailmessages query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }
    }

    if (commitOnSuccess && !t.commit()) {
        qMailLog(Messaging) << "Could not commit metadata update to database";
        return DatabaseFailure;
    }

    // Update the header cache
    foreach (const QMailMessageId& id, *updatedMessageIds) {
        if (messageCache.contains(id)) {
            QMailMessageMetaData cachedMetaData = messageCache.lookup(id);
            updateMessageValues(properties, extractedValues, data.customFields(), cachedMetaData);
            cachedMetaData.setUnmodified();
            messageCache.insert(cachedMetaData);
            uidCache.insert(qMakePair(cachedMetaData.parentAccountId(), cachedMetaData.serverUid()), cachedMetaData.id());
        }
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptUpdateMessagesStatus(const QMailMessageKey &key, quint64 status, bool set, 
                                                                                QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, 
                                                                                Transaction &t, bool commitOnSuccess)
{
    //get the valid ids
    *updatedMessageIds = queryMessages(key, QMailMessageSortKey(), 0, 0);
    if (!updatedMessageIds->isEmpty()) {
        // Find the set of folders and accounts whose contents are modified by this update
        AttemptResult result = affectedByMessageIds(*updatedMessageIds, modifiedFolderIds, modifiedAccountIds);
        if (result != Success)
            return result;

        {
            QString sql("UPDATE mailmessages SET status=(status %1 ?)");
            QSqlQuery query(simpleQuery(sql.arg(set ? "|" : "&"),
                                        QVariantList() << (set ? status : ~status),
                                        Key(QMailMessageKey::id(*updatedMessageIds)),
                                        "updateMessagesMetaData status query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }
    }

    if (commitOnSuccess && !t.commit()) {
        qMailLog(Messaging) << "Could not commit metadata status update to database";
        return DatabaseFailure;
    }

    // Update the header cache
    foreach (const QMailMessageId& id, *updatedMessageIds) {
        if (messageCache.contains(id)) {
            QMailMessageMetaData cachedMetaData = messageCache.lookup(id);
            quint64 newStatus = cachedMetaData.status();
            newStatus = set ? (newStatus | status) : (newStatus & ~status);
            cachedMetaData.setStatus(newStatus);
            cachedMetaData.setUnmodified();
            messageCache.insert(cachedMetaData);
            uidCache.insert(qMakePair(cachedMetaData.parentAccountId(), cachedMetaData.serverUid()), cachedMetaData.id());
        }
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptRestoreToPreviousFolder(const QMailMessageKey &key, 
                                                                                   QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, 
                                                                                   Transaction &t, bool commitOnSuccess)
{
    // Find the message and folders that are affected by this update
    QSqlQuery query(simpleQuery("SELECT t0.id, t0.parentfolderid, t0.previousparentfolderid FROM mailmessages t0",
                                Key(key, "t0"),
                                "restoreToPreviousFolder info query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    QSet<quint64> folderIdSet;
    while (query.next()) {
        updatedMessageIds->append(QMailMessageId(extractValue<quint64>(query.value(0))));

        folderIdSet.insert(extractValue<quint64>(query.value(1)));
        folderIdSet.insert(extractValue<quint64>(query.value(2)));
    }

    if (!folderIdSet.isEmpty()) {
        QMailFolderIdList folderIds;
        foreach (quint64 id, folderIdSet)
            folderIds.append(QMailFolderId(id));

        // Find the set of folders and accounts whose contents are modified by this update
        AttemptResult result = affectedByFolderIds(folderIds, modifiedFolderIds, modifiedAccountIds);
        if (result != Success)
            return result;

        // Update the message records
        QSqlQuery query(simpleQuery("UPDATE mailmessages SET parentfolderid=previousparentfolderid, previousparentfolderid=NULL",
                                    Key(QMailMessageKey::id(*updatedMessageIds)),
                                    "restoreToPreviousFolder update query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    }

    if (commitOnSuccess && !t.commit()) {
        qMailLog(Messaging) << "Could not commit message folder restoration to database";
        return DatabaseFailure;
    }

    // Update the header cache
    foreach (const QMailMessageId &id, *updatedMessageIds) {
        if (messageCache.contains(id)) {
            QMailMessageMetaData cachedMetaData = messageCache.lookup(id);
            cachedMetaData.setParentFolderId(cachedMetaData.previousParentFolderId());
            cachedMetaData.setPreviousParentFolderId(QMailFolderId());
            cachedMetaData.setUnmodified();
            messageCache.insert(cachedMetaData);
            uidCache.insert(qMakePair(cachedMetaData.parentAccountId(), cachedMetaData.serverUid()), cachedMetaData.id());
        }
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptPurgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids,
                                                                                      Transaction &t, bool commitOnSuccess)
{
    QMailMessageIdList removalIds;

    {
        QString sql("SELECT id FROM deletedmessages WHERE parentaccountid=?");

        QVariantList bindValues;
        bindValues << accountId.toULongLong();

        if (!serverUids.isEmpty()) {
            QVariantList uidValues;
            foreach (const QString& uid, serverUids)
                uidValues.append(uid);

            sql.append(" AND serveruid IN %1");
            sql = sql.arg(expandValueList(uidValues));

            bindValues << uidValues;
        }

        QSqlQuery query(simpleQuery(sql, 
                                    bindValues,
                                    "purgeMessageRemovalRecord info query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        while (query.next())
            removalIds.append(QMailMessageId(extractValue<quint64>(query.value(0))));
    }

    // anything to remove?
    if (!removalIds.isEmpty()) {
        QSqlQuery query(simpleQuery("DELETE FROM deletedmessages",
                                    Key(QMailMessageKey::id(removalIds)),
                                    "purgeMessageRemovalRecord delete query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    }

    if (commitOnSuccess && !t.commit()) {
        qMailLog(Messaging) << "Could not commit message removal record deletion to database";
        return DatabaseFailure;
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptCountAccounts(const QMailAccountKey &key, int *result, 
                                                                         ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT COUNT(*) FROM mailaccounts",
                                Key(key),
                                "countAccounts mailaccounts query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    if (query.first())
        *result = extractValue<int>(query.value(0));

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptCountFolders(const QMailFolderKey &key, int *result, 
                                                                        ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT COUNT(*) FROM mailfolders",
                                Key(key),
                                "countFolders mailfolders query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    if (query.first())
        *result = extractValue<int>(query.value(0));

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptCountMessages(const QMailMessageKey &key, 
                                                                         int *result, 
                                                                         ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT COUNT(*) FROM mailmessages",
                                Key(key),
                                "countMessages mailmessages query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    if (query.first())
        *result = extractValue<int>(query.value(0));

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptSizeOfMessages(const QMailMessageKey &key, 
                                                                          int *result, 
                                                                          ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT SUM(size FROM mailmessages",
                                Key(key),
                                "sizeOfMessages mailmessages query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    if (query.first())
        *result = extractValue<int>(query.value(0));

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptQueryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey, uint limit, uint offset,
                                                                         QMailAccountIdList *ids, 
                                                                         ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT id FROM mailaccounts",
                                QVariantList(),
                                QList<Key>() << Key(key) << Key(sortKey),
                                qMakePair(limit, offset),
                                "queryAccounts mailaccounts query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    while (query.next())
        ids->append(QMailAccountId(extractValue<quint64>(query.value(0))));

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptQueryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey, uint limit, uint offset,
                                                                        QMailFolderIdList *ids, 
                                                                        ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT id FROM mailfolders",
                                QVariantList(),
                                QList<Key>() << Key(key) << Key(sortKey),
                                qMakePair(limit, offset),
                                "queryFolders mailfolders query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    while (query.next())
        ids->append(QMailFolderId(extractValue<quint64>(query.value(0))));

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptQueryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey, uint limit, uint offset,
                                                                         QMailMessageIdList *ids, 
                                                                         ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT id FROM mailmessages",
                                QVariantList(),
                                QList<Key>() << Key(key) << Key(sortKey),
                                qMakePair(limit, offset),
                                "queryMessages mailmessages query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    while (query.next())
        ids->append(QMailMessageId(extractValue<quint64>(query.value(0))));

    //store the results of this call for cache preloading
    lastQueryMessageResult = *ids;

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptAccount(const QMailAccountId &id, 
                                                                   QMailAccount *result, 
                                                                   ReadLock &)
{
    {
        QSqlQuery query(simpleQuery("SELECT * FROM mailaccounts WHERE id=?",
                                    QVariantList() << id.toULongLong(),
                                    "account mailaccounts query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        if (query.first()) {
            *result = extractAccount(query.record());
        }
    }

    if (result->id().isValid()) {
        {
            // Find any standard folders configured for this account
            QSqlQuery query(simpleQuery("SELECT foldertype,folderid FROM mailaccountfolders WHERE id=?",
                                        QVariantList() << id.toULongLong(),
                                        "account mailaccountfolders query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            while (query.next())
                result->setStandardFolder(QMailFolder::StandardFolder(query.value(0).toInt()), QMailFolderId(query.value(1).toULongLong()));
        }

        // Find any custom fields for this account
        QMap<QString, QString> fields;
        AttemptResult attemptResult = customFields(id.toULongLong(), &fields, "mailaccountcustom");
        if (attemptResult != Success)
            return attemptResult;

        result->setCustomFields(fields);
        result->setCustomFieldsModified(false);

        {
            // Find the type of the account
            QSqlQuery query(simpleQuery("SELECT service,value FROM mailaccountconfig WHERE id=? AND name='servicetype'",
                                        QVariantList() << id.toULongLong(),
                                        "account mailaccountconfig query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            while (query.next()) {
                QString service(query.value(0).toString());
                QString type(query.value(1).toString());

                if (type.contains("source")) {
                    result->addMessageSource(service);
                }
                if (type.contains("sink")) {
                    result->addMessageSink(service);
                }
            }
        }

        //update cache 
        accountCache.insert(*result);
        return Success;
    }

    return Failure;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptAccountConfiguration(const QMailAccountId &id, 
                                                                                QMailAccountConfiguration *result, 
                                                                                ReadLock &)
{
    // Find any configuration fields for this account
    QSqlQuery query(simpleQuery("SELECT service,name,value FROM mailaccountconfig WHERE id=? ORDER BY service",
                                QVariantList() << id.toULongLong(),
                                "accountConfiguration mailaccountconfig query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    QString service;
    QMailAccountConfiguration::ServiceConfiguration *serviceConfig = 0;

    while (query.next()) {
        QString svc(query.value(0).toString());
        if (svc != service) {
            service = svc;

            if (!result->services().contains(service)) {
                // Add this service to the configuration
                result->addServiceConfiguration(service);
            }

            serviceConfig = &result->serviceConfiguration(service);
        }

        serviceConfig->setValue(query.value(1).toString(), query.value(2).toString());
    }

    if (service.isEmpty()) {
        // No services - is this an error?
        QSqlQuery query(simpleQuery("SELECT COUNT(*) FROM mailaccounts WHERE id=?",
                                    QVariantList() << id.toULongLong(),
                                    "accountConfiguration mailaccounts query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        if (query.first()) {
            if (extractValue<int>(query.value(0)) == 0)
                return Failure;
        }
    } 

    result->setId(id);
    result->setModified(false);

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptFolder(const QMailFolderId &id, 
                                                                  QMailFolder *result, 
                                                                  ReadLock &)
{
    {
        QSqlQuery query(simpleQuery("SELECT * FROM mailfolders WHERE id=?",
                                    QVariantList() << id.toULongLong(),
                                    "folder mailfolders query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        if (query.first()) {
            *result = extractFolder(query.record());
        }
    }

    if (result->id().isValid()) {
        // Find any custom fields for this folder
        QMap<QString, QString> fields;
        AttemptResult attemptResult = customFields(id.toULongLong(), &fields, "mailfoldercustom");
        if (attemptResult != Success)
            return attemptResult;

        result->setCustomFields(fields);
        result->setCustomFieldsModified(false);

        //update cache 
        folderCache.insert(*result);
        return Success;
    }

    return Failure;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptMessage(const QMailMessageId &id, 
                                                                   QMailMessage *result, 
                                                                   ReadLock &)
{
    // Find any custom fields for this message
    QMap<QString, QString> fields;
    AttemptResult attemptResult = customFields(id.toULongLong(), &fields, "mailmessagecustom");
    if (attemptResult != Success)
        return attemptResult;

    QSqlQuery query(simpleQuery("SELECT * FROM mailmessages WHERE id=?",
                                QVariantList() << id.toULongLong(),
                                "message mailmessages id query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    if (query.first()) {
        *result = extractMessage(query.record(), fields);
        if (result->id().isValid())
            return Success;
    }

    return Failure;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptMessage(const QString &uid, const QMailAccountId &accountId, 
                                                                   QMailMessage *result, 
                                                                   ReadLock &lock)
{
    quint64 id(0);

    AttemptResult attemptResult = attemptMessageId(uid, accountId, &id, lock);
    if (attemptResult != Success)
        return attemptResult;
            
    if (id != 0) {
        return attemptMessage(QMailMessageId(id), result, lock);
    }

    return Failure;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptMessageMetaData(const QMailMessageId &id,
                                                                           QMailMessageMetaData *result, 
                                                                           ReadLock &)
{
    // Find any custom fields for this message
    QMap<QString, QString> fields;
    AttemptResult attemptResult = customFields(id.toULongLong(), &fields, "mailmessagecustom");
    if (attemptResult != Success)
        return attemptResult;

    QSqlQuery query(simpleQuery("SELECT * FROM mailmessages WHERE id=?",
                                QVariantList() << id.toULongLong(),
                                "message mailmessages id query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    if (query.first()) {
        *result = extractMessageMetaData(query.record(), fields);
        if (result->id().isValid())
            return Success;
    }

    return Failure;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptMessageMetaData(const QString &uid, const QMailAccountId &accountId, 
                                                                           QMailMessageMetaData *result, 
                                                                           ReadLock &lock)
{
    quint64 id(0);

    AttemptResult attemptResult = attemptMessageId(uid, accountId, &id, lock);
    if (attemptResult != Success)
        return attemptResult;
            
    if (id != 0) {
        return attemptMessageMetaData(QMailMessageId(id), result, lock);
    }

    return Failure;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptMessagesMetaData(const QMailMessageKey& key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option, 
                                                                            QMailMessageMetaDataList *result, 
                                                                            ReadLock &)
{
    if (properties == QMailMessageKey::Custom) {
        // We're only selecting custom fields
        QString sql("SELECT %1 name, value FROM mailmessagecustom WHERE id IN ( SELECT t0.id FROM mailmessages t0");
        sql += buildWhereClause(Key(key, "t0")) + " )";

        QVariantList whereValues(::whereClauseValues(key));
        QSqlQuery query(simpleQuery(sql.arg(option == QMailStore::ReturnDistinct ? "DISTINCT " : ""),
                                    whereValues,
                                    "messagesMetaData combined query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        // Find all the values for each parameter name in the set
        QMap<QString, QStringList> fields;
        while (query.next())
            fields[query.value(0).toString()].append(query.value(1).toString());

        // Create records for each of these parameters
        int maxLen = 0;
        foreach (const QStringList &list, fields.values())
            maxLen = qMax<uint>(maxLen, list.count());

        for (int i = 0; i < maxLen; ++i)
            result->append(QMailMessageMetaData());

        // Add all pairs to the results
        foreach (const QString &name, fields.keys()) {
            QMailMessageMetaDataList::iterator it = result->begin();
            foreach (const QString &value, fields[name]) {
                (*it).setCustomField(name, value);
                ++it;
            }
        }

        QMailMessageMetaDataList::iterator it = result->begin(), end = result->end();
        for ( ; it != end; ++it)
            (*it).setCustomFieldsModified(false);
    } else {
        bool includeCustom(properties & QMailMessageKey::Custom);
        if (includeCustom && (option == QMailStore::ReturnDistinct)) {
            qWarning() << "Warning: Distinct-ness is not supported with custom fields!";
        }

        QString sql("SELECT %1 %2 FROM mailmessages t0");
        sql = sql.arg(option == QMailStore::ReturnDistinct ? "DISTINCT " : "");

        QMailMessageKey::Properties props(properties);

        bool removeId(false);
        if (includeCustom && !(props & QMailMessageKey::Id)) {
            // We need the ID to match against the custom table
            props |= QMailMessageKey::Id;
            removeId = true;
        }

        {
            QSqlQuery query(simpleQuery(sql.arg(expandProperties(props, false)),
                                        Key(key, "t0"),
                                        "messagesMetaData mailmessages query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            while (query.next())
                result->append(extractMessageMetaData(query.record(), props, props));
        }

        if (includeCustom) {
            QMailMessageMetaDataList::iterator it = result->begin(), end = result->end();
            for ( ; it != end; ++it) {
                // Add the custom fields to the record
                QMap<QString, QString> fields;
                AttemptResult attemptResult = customFields((*it).id().toULongLong(), &fields, "mailmessagecustom");
                if (attemptResult != Success)
                    return attemptResult;

                QMailMessageMetaData &metaData(*it);
                metaData.setCustomFields(fields);
                metaData.setCustomFieldsModified(false);

                if (removeId)
                    metaData.setId(QMailMessageId());
            }
        }
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptMessageRemovalRecords(const QMailAccountId &accountId, const QMailFolderId &folderId, 
                                                                                 QMailMessageRemovalRecordList *result, 
                                                                                 ReadLock &)
{
    QVariantList values;
    values << accountId.toULongLong();

    QString sql("SELECT * FROM deletedmessages WHERE parentaccountid=?");
    if (folderId.isValid()) {
        sql += " AND parentfolderid=?";
        values << folderId.toULongLong();
    }

    QSqlQuery query(simpleQuery(sql,
                                values,
                                "messageRemovalRecords deletedmessages query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    while (query.next())
        result->append(extractMessageRemovalRecord(query.record()));

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptMessageFolderIds(const QMailMessageKey &key, 
                                                                            QMailFolderIdList *result, 
                                                                            ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT DISTINCT t0.parentfolderid FROM mailmessages t0",
                                Key(key, "t0"),
                                "messageFolderIds folder select query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    while (query.next())
        result->append(QMailFolderId(extractValue<quint64>(query.value(0))));

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptFolderAccountIds(const QMailFolderKey &key, 
                                                                            QMailAccountIdList *result, 
                                                                            ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT DISTINCT parentaccountid FROM mailfolders t0",
                                Key(key, "t0"),
                                "folderAccountIds account select query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    while (query.next())
        result->append(QMailAccountId(extractValue<quint64>(query.value(0))));

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptFolderAncestorIds(const QMailFolderIdList &ids, 
                                                                             QMailFolderIdList *result, 
                                                                             ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT DISTINCT id FROM mailfolderlinks",
                                Key("descendantid", QMailFolderKey::id(ids)),
                                "folderAncestorIds id select query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    while (query.next())
        result->append(QMailFolderId(extractValue<quint64>(query.value(0))));

    return Success;
}

void QMailStorePrivate::preloadHeaderCache(const QMailMessageId& id) const
{
    QMailMessageIdList idBatch;
    idBatch.append(id);

    int index = lastQueryMessageResult.indexOf(id);
    if (index != -1) {
        // Preload based on result of last call to queryMessages
        int count = 1;

        QMailMessageIdList::const_iterator begin = lastQueryMessageResult.begin();
        QMailMessageIdList::const_iterator end = lastQueryMessageResult.end();
        QMailMessageIdList::const_iterator lowIt = begin + index;
        QMailMessageIdList::const_iterator highIt = lowIt;

        bool ascend(true);
        bool descend(lowIt != begin);

        while ((count < (QMailStorePrivate::lookAhead * 2)) && (ascend || descend)) {
            if (ascend) {
                ++highIt;
                if (highIt == end) {
                    ascend = false;
                } else  {
                    if (!messageCache.contains(*highIt)) {
                        idBatch.append(*highIt);
                        ++count;
                    } else {
                        // Most likely, a sequence in the other direction will be more useful
                        ascend = false;
                    }
                }
            }

            if (descend) {
                --lowIt;
                if (!messageCache.contains(*lowIt)) {
                    idBatch.prepend(*lowIt);
                    ++count;

                    if (lowIt == begin) {
                        descend = false;
                    }
                } else {
                    // Most likely, a sequence in the other direction will be more useful
                    descend = false;
                }
            }
        }
    } else {
        // Don't bother preloading - if there is a query result, we have now searched outside it;
        // we should consider it to have outlived its usefulness
        if (!lastQueryMessageResult.isEmpty())
            lastQueryMessageResult = QMailMessageIdList();
    }

    QMailMessageMetaData result;
    QMailMessageKey key(QMailMessageKey::id(idBatch));
    foreach (const QMailMessageMetaData& metaData, messagesMetaData(key, allMessageProperties(), QMailStore::ReturnAll)) {
        if (metaData.id().isValid()) {
            messageCache.insert(metaData);
            uidCache.insert(qMakePair(metaData.parentAccountId(), metaData.serverUid()), metaData.id());
            if (metaData.id() == id)
                result = metaData;
        }
    }
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptStatusBit(const QString &name, const QString &context, 
                                                                     int *result, 
                                                                     ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT COALESCE(statusbit,0) FROM mailstatusflags WHERE name=? AND context=?",
                                QVariantList() << name << context,
                                "mailstatusflags select"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    *result = 0;
    if (query.next())
        *result = extractValue<int>(query.value(0));

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptRegisterStatusBit(const QString &name, const QString &context, int maximum, 
                                                                             Transaction &t, bool commitOnSuccess)
{
    int highest = 0;

    {
        // Find the highest 
        QSqlQuery query(simpleQuery("SELECT MAX(statusbit) FROM mailstatusflags WHERE context=?",
                                    QVariantList() << context,
                                    "mailstatusflags register select"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        if (query.next())
            highest = extractValue<int>(query.value(0));
    }

    if (highest == maximum) {
        return Failure;
    } else {
        QSqlQuery query(simpleQuery("INSERT INTO mailstatusflags (name,context,statusbit) VALUES (?,?,?)",
                                    QVariantList() << name << context << (highest + 1),
                                    "mailstatusflags register insert"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    }

    if (commitOnSuccess && !t.commit()) {
        qMailLog(Messaging) << "Could not commit statusflag changes to database";
        return DatabaseFailure;
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::attemptMessageId(const QString &uid, const QMailAccountId &accountId, 
                                                                     quint64 *result, 
                                                                     ReadLock &)
{
    QSqlQuery query(simpleQuery("SELECT id FROM mailmessages WHERE serveruid=? AND parentaccountid=?",
                                QVariantList() << uid << accountId.toULongLong(),
                                "message mailmessages uid/parentaccountid query"));
    if (query.lastError().type() != QSqlError::NoError)
        return DatabaseFailure;

    if (query.first()) {
        *result = extractValue<quint64>(query.value(0));
        return Success;
    }
        
    return Failure;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::affectedByMessageIds(const QMailMessageIdList &messages, QMailFolderIdList *folderIds, QMailAccountIdList *accountIds) const
{
    AttemptResult result;

    // Find the set of folders whose contents are modified by this update
    QMailFolderIdList messageFolderIds;

    QMailStorePrivate *self(const_cast<QMailStorePrivate*>(this));
    {
        ReadLock l(self);
        result = self->attemptMessageFolderIds(QMailMessageKey::id(messages), &messageFolderIds, l);
    }

    if (result != Success)
        return result;

    return affectedByFolderIds(messageFolderIds, folderIds, accountIds);
}

QMailStorePrivate::AttemptResult QMailStorePrivate::affectedByFolderIds(const QMailFolderIdList &folders, QMailFolderIdList *folderIds, QMailAccountIdList *accountIds) const
{
    AttemptResult result;

    // Any ancestor folders are also modified
    QMailFolderIdList ancestorIds;

    QMailStorePrivate *self(const_cast<QMailStorePrivate*>(this));
    {
        ReadLock l(self);
        result = self->attemptFolderAncestorIds(folders, &ancestorIds, l);
    }

    if (result != Success)
        return result;

    *folderIds = folders + ancestorIds;

    // Find the set of accounts whose contents are modified by this update
    ReadLock l(self);
    result = self->attemptFolderAccountIds(QMailFolderKey::id(*folderIds), accountIds, l);
    return result;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::messagePredecessor(QMailMessageMetaData *metaData, const QStringList &references, const QString &baseSubject, bool replyOrForward,
                                                                       QStringList *missingReferences, bool *missingAncestor)
{
    QList<quint64> potentialPredecessors;

    if (!references.isEmpty()) {
        // Find any messages that correspond to these references
        QMap<QString, QList<quint64> > referencedMessages;

        QVariantList refs;
        foreach (const QString &ref, references) {
            refs.append(QVariant(ref));
        }

        {
            QString sql("SELECT id,identifier FROM mailmessageidentifiers WHERE identifier IN %1");
            QSqlQuery query(simpleQuery(sql.arg(expandValueList(refs)),
                                        refs,
                                        "messagePredecessor mailmessageidentifiers select query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            while (query.next()) {
                referencedMessages[extractValue<QString>(query.value(1))].append(extractValue<quint64>(query.value(0).toInt()));
            }
        }

        if (referencedMessages.isEmpty()) {
            // All the references are missing
            *missingReferences = references;
        } else {
            for (int i = references.count() - 1; i >= 0; --i) {
                const QString &refId(references.at(i));

                QMap<QString, QList<quint64> >::const_iterator it = referencedMessages.find(refId);
                if (it != referencedMessages.end()) {
                    const QList<quint64> &messageIds(it.value());

                    if (messageIds.count() == 1) {
                        // This is the best parent message choice
                        potentialPredecessors.append(messageIds.first());
                        break;
                    } else {
                        // TODO: We need to choose a best selection from amongst these messages
                        // For now, just process the order the DB gave us
                        potentialPredecessors = messageIds;
                        break;
                    }
                } else {
                    missingReferences->append(refId);
                }
            }
        }
    } else if (!baseSubject.isEmpty() && replyOrForward) {
        // This message has a thread ancestor, but we can only estimate which is the best choice
        *missingAncestor = true;

        // Find the preceding messages of all thread matching this base subject
        QSqlQuery query(simpleQuery("SELECT id FROM mailmessages "
                                    "WHERE id!=? "
                                    "AND parentaccountid=? "
                                    "AND stamp<? "
                                    "AND id IN ("
                                        "SELECT messageid FROM mailthreadmessages mtm WHERE threadid IN ("
                                            "SELECT threadid FROM mailthreadsubjects WHERE subjectid = ("
                                                "SELECT id FROM mailsubjects WHERE basesubject=?"
                                            ")"
                                        ")"
                                    ") "
                                    "ORDER BY stamp DESC",
                                    QVariantList() << metaData->id().toULongLong() 
                                                    << metaData->parentAccountId().toULongLong() 
                                                    << metaData->date().toLocalTime() 
                                                    << baseSubject,
                                    "messagePredecessor mailmessages select query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        while (query.next()) {
            potentialPredecessors.append(extractValue<quint64>(query.value(0)));
        }
    }

    if (!potentialPredecessors.isEmpty()) {
        quint64 predecessorId(0);
        quint64 messageId(metaData->id().toULongLong());

        if (messageId != 0) {
            // We already exist - therefore we must ensure that we do not create a response ID cycle
            QMap<quint64, quint64> predecessor;

            {

                // Find the predecessor message for every message in the same thread as us
                QSqlQuery query(simpleQuery("SELECT id,responseid FROM mailmessages WHERE id IN ("
                                                "SELECT messageid FROM mailthreadmessages WHERE threadid = ("
                                                    "SELECT threadid FROM mailthreadmessages WHERE messageid=?"
                                                ")"
                                            ")",
                                            QVariantList() << metaData->id().toULongLong(),
                                            "identifyAncestors mailmessages query"));
                if (query.lastError().type() != QSqlError::NoError)
                    return DatabaseFailure;

                while (query.next())
                    predecessor.insert(extractValue<quint64>(query.value(0)), extractValue<quint64>(query.value(1)));
            }

            // Choose the best predecessor, ensuring that we don't pick a message whose own ancestors include us
            while (!potentialPredecessors.isEmpty()) {
                quint64 ancestorId = potentialPredecessors.first();

                bool descendant(false);
                while (ancestorId) {
                    if (ancestorId == messageId) {
                        // This message is a descendant of ourself
                        descendant = true;
                        break;
                    } else {
                        ancestorId = predecessor[ancestorId];
                    }
                }

                if (!descendant) {
                    // This message can become our predecessor
                    predecessorId = potentialPredecessors.first();
                    break;
                } else {
                    // Try the next option, if any
                    potentialPredecessors.takeFirst();
                }
            }
        } else {
            // Just take the first selection
            predecessorId = potentialPredecessors.first();
        }

        if (predecessorId) {
            metaData->setInResponseTo(QMailMessageId(predecessorId));

            // TODO: What kind of response is this?  If the predecessor is from the same
            // account as the new message then it is probably a reply.  Otherwise, forward?
            metaData->setResponseType(QMailMessage::Reply);
        }
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::identifyAncestors(const QMailMessageId &predecessorId, const QMailMessageIdList &childIds, QMailMessageIdList *ancestorIds)
{
    if (!childIds.isEmpty() && predecessorId.isValid()) {
        QMap<quint64, quint64> predecessor;

        {
            // Find the predecessor message for every message in the same thread as the predecessor
            QSqlQuery query(simpleQuery("SELECT id,responseid FROM mailmessages WHERE id IN ("
                                            "SELECT messageid FROM mailthreadmessages WHERE threadid = ("
                                                "SELECT threadid FROM mailthreadmessages WHERE messageid=?"
                                            ")"
                                        ")",
                                        QVariantList() << predecessorId.toULongLong(),
                                        "identifyAncestors mailmessages query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            while (query.next())
                predecessor.insert(extractValue<quint64>(query.value(0)), extractValue<quint64>(query.value(1)));
        }

        // Ensure that none of the prospective children are predecessors of this message
        quint64 messageId = predecessorId.toULongLong();
        while (messageId) {
            if (childIds.contains(QMailMessageId(messageId))) {
                ancestorIds->append(QMailMessageId(messageId));
            }

            messageId = predecessor[messageId];
        }
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::resolveMissingMessages(const QString &identifier, const QMailMessageId &predecessorId, const QString &baseSubject, quint64 messageId, QMailMessageIdList *updatedMessageIds)
{
    QMap<QMailMessageId, quint64> descendants;

    if (!identifier.isEmpty()) {
        QSqlQuery query(simpleQuery("SELECT DISTINCT id,level FROM missingmessages WHERE identifier=?",
                                    QVariantList() << identifier,
                                    "resolveMissingMessages missingmessages query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        while (query.next())
            descendants.insert(QMailMessageId(extractValue<quint64>(query.value(0))), extractValue<quint64>(query.value(1)));
    }

    if (!descendants.isEmpty() && predecessorId.isValid()) {
        QMailMessageIdList ancestorIds;

        // Do not create a cycle - ensure that none of these messages is an ancestor of the new message
        AttemptResult result = identifyAncestors(predecessorId, descendants.keys(), &ancestorIds);
        if (result != Success)
            return result;

        // Ensure that none of the ancestors become descendants of this message
        foreach (const QMailMessageId &id, ancestorIds) {
            descendants.remove(id);
        }
    }

    if (!descendants.isEmpty()) {
        QVariantList descendantIds;
        QVariantList descendantLevels;

        QMap<QMailMessageId, quint64>::const_iterator it = descendants.begin(), end = descendants.end();
        for ( ; it != end; ++it) {
            updatedMessageIds->append(it.key());

            descendantIds.append(QVariant(it.key().toULongLong()));
            descendantLevels.append(QVariant(it.value()));
        }

        {
            // Update these descendant messages to have the new message as their predecessor
            QSqlQuery query(simpleQuery("UPDATE mailmessages SET responseid=?",
                                        QVariantList() << messageId,
                                        Key(QMailMessageKey::id(*updatedMessageIds)),
                                        "resolveMissingMessages mailmessages update query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        {
            // Truncate the missingmessages entries for each updated messages
            QSqlQuery query(batchQuery("DELETE FROM missingmessages WHERE id=? AND level>=?",
                                       QVariantList() << QVariant(descendantIds) << QVariant(descendantLevels),
                                       "resolveMissingMessages missingmessages delete query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        QVariantList obsoleteThreadIds;

        {
            // Find the threads that the descendants currently belong to
            QSqlQuery query(simpleQuery("SELECT DISTINCT threadid FROM mailthreadmessages",
                                        Key("messageid", QMailMessageKey::id(*updatedMessageIds)),
                                        "resolveMissingMessages mailthreadmessages query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            while (query.next())
                obsoleteThreadIds.append(QVariant(extractValue<quint64>(query.value(0))));
        }

        {
            // Attach the descendants to the thread of their new predecessor
            QSqlQuery query(simpleQuery("UPDATE mailthreadmessages SET threadid=(SELECT threadid FROM mailthreadmessages WHERE messageid=?)",
                                        QVariantList() << messageId,
                                        Key("messageid", QMailMessageKey::id(*updatedMessageIds)),
                                        "resolveMissingMessages mailthreadmessages update query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }

        if (!obsoleteThreadIds.isEmpty()) {
            // Delete the obsolete threads
            QString sql("DELETE FROM mailthreads WHERE id IN %1");
            QSqlQuery query(simpleQuery(sql.arg(expandValueList(obsoleteThreadIds)),
                                        obsoleteThreadIds,
                                        "resolveMissingMessages mailthreads delete query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }
    }

    if (!baseSubject.isEmpty()) {
        QMailMessageIdList ids;

        {
            // See if there are any messages waiting for a thread ancestor message with this subject
            // (or who have one that is older than this message)
            QSqlQuery query(simpleQuery("SELECT id FROM mailmessages mm "
                                        "WHERE id IN ("
                                            "SELECT messageid FROM missingancestors WHERE subjectid=(SELECT id FROM mailsubjects WHERE basesubject=?) "
                                        ") AND "
                                            "stamp > (SELECT stamp FROM mailmessages WHERE id=?) "
                                        "AND ("
                                            "mm.responseid=0 "
                                        "OR "
                                            "(SELECT stamp FROM mailmessages WHERE id=?) > (SELECT stamp FROM mailmessages WHERE id=mm.responseid)"
                                        ")",
                                        QVariantList() << baseSubject << messageId << messageId,
                                        "resolveMissingMessages missingancestors query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            while (query.next())
                ids.append(QMailMessageId(extractValue<quint64>(query.value(0))));
        }

        if (!ids.isEmpty() && predecessorId.isValid()) {
            QMailMessageIdList ancestorIds;

            // Do not create a cycle - ensure that none of these messages is an ancestor of the new message
            AttemptResult result = identifyAncestors(predecessorId, ids, &ancestorIds);
            if (result != Success)
                return result;

            // Ensure that none of the ancestors become descendants of this message
            foreach (const QMailMessageId &id, ancestorIds) {
                ids.removeAll(id);
            }
        }

        if (!ids.isEmpty()) {
            {
                // Update these descendant messages to have the new message as their predecessor
                QSqlQuery query(simpleQuery("UPDATE mailmessages SET responseid=?",
                                            QVariantList() << messageId,
                                            Key(QMailMessageKey::id(ids)),
                                            "resolveMissingMessages mailmessages update root query"));
                if (query.lastError().type() != QSqlError::NoError)
                    return DatabaseFailure;
            }

            {
                // Remove the missing ancestor records
                QSqlQuery query(simpleQuery("UPDATE missingancestors SET state=1",
                                            Key("messageid", QMailMessageKey::id(ids)),
                                            "resolveMissingMessages missingancestors delete query"));
                if (query.lastError().type() != QSqlError::NoError)
                    return DatabaseFailure;
            }

            *updatedMessageIds += ids;
        }
    }

    return Success;
}

QMailStorePrivate::AttemptResult QMailStorePrivate::registerSubject(const QString &baseSubject, quint64 messageId, const QMailMessageId &predecessorId, bool missingAncestor)
{
    int subjectId = 0;

    {
        QSqlQuery query(simpleQuery("SELECT id FROM mailsubjects WHERE basesubject=?",
                                    QVariantList() << baseSubject,
                                    "registerSubject mailsubjects query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        if (query.next())
            subjectId = extractValue<quint64>(query.value(0));
    }
    
    if (subjectId == 0) {
        QSqlQuery query(simpleQuery("INSERT INTO mailsubjects (basesubject) VALUES (?)",
                                    QVariantList() << baseSubject,
                                    "registerSubject mailsubjects insert query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        // Retrieve the insert id
        subjectId = extractValue<quint64>(query.lastInsertId());
    }

    // Ensure that this thread is linked to the base subject of this message
    int count = 0;
    {
        QSqlQuery query(simpleQuery("SELECT COUNT(*) FROM mailthreadsubjects "
                                    "WHERE subjectid=? AND threadid = (SELECT threadid FROM mailthreadmessages WHERE messageid=?)",
                                    QVariantList() << subjectId << messageId,
                                    "registerSubject mailthreadsubjects query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;

        if (query.next())
            count = extractValue<int>(query.value(0));
    }
    
    if (count == 0) {
        QSqlQuery query(simpleQuery("INSERT INTO mailthreadsubjects (threadid,subjectid) SELECT threadid,? FROM mailthreadmessages WHERE messageid=?",
                                    QVariantList() << subjectId << messageId,
                                    "registerSubject mailthreadsubjects insert query"));
        if (query.lastError().type() != QSqlError::NoError)
            return DatabaseFailure;
    }

    if (missingAncestor) {
        count = 0;

        {
            // We need to record that this message's ancestor is currently missing
            QSqlQuery query(simpleQuery("SELECT COUNT(*) FROM missingancestors WHERE messageid=?",
                                        QVariantList() << messageId,
                                        "registerSubject missingancestors query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;

            if (query.next())
                count = extractValue<int>(query.value(0));
        }

        if (count == 0) {
            quint64 state(predecessorId.isValid() ? 1 : 0);
            QSqlQuery query(simpleQuery("INSERT INTO missingancestors (messageid,subjectid,state) VALUES(?,?,?)",
                                        QVariantList() << messageId << subjectId << state,
                                        "registerSubject missingancestors insert query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        } else {
            QSqlQuery query(simpleQuery("UPDATE missingancestors SET subjectid=? WHERE messageid=?",
                                        QVariantList() << subjectId << messageId,
                                        "registerSubject missingancestors update query"));
            if (query.lastError().type() != QSqlError::NoError)
                return DatabaseFailure;
        }
    }

    return Success;
}

bool QMailStorePrivate::checkPreconditions(const QMailFolder& folder, bool update)
{
    //if the parent is valid, check that it exists 
    //if the account is valid, check that is exists 

    if(!update)
    {
        if(folder.id().isValid())
        {
            qMailLog(Messaging) << "Folder exists, use update instead of add.";
            return false;
        }
    }
    else 
    {
        if(!folder.id().isValid())
        {
            qMailLog(Messaging) << "Folder does not exist, use add instead of update.";
            return false;
        }

        if(folder.parentFolderId().isValid() && folder.parentFolderId() == folder.id())
        {
            qMailLog(Messaging) << "A folder cannot be a child to itself";
            return false;
        }
    }

    if(folder.parentFolderId().isValid())
    {
        if(!idExists(folder.parentFolderId(),"mailfolders"))
        {
            qMailLog(Messaging) << "Parent folder does not exist!";
            return false;
        }
    }

    if(folder.parentAccountId().isValid())
    {
        if(!idExists(folder.parentAccountId(),"mailaccounts"))
        {
            qMailLog(Messaging) << "Parent account does not exist!";
            return false;
        }
    }

    return true;
}

bool QMailStorePrivate::deleteMessages(const QMailMessageKey& key, 
                                       QMailStore::MessageRemovalOption option, 
                                       QMailMessageIdList& deletedMessageIds, 
                                       QStringList& expiredContent, 
                                       QMailMessageIdList& updatedMessageIds, 
                                       QMailFolderIdList& modifiedFolderIds,
                                       QMailAccountIdList& modifiedAccountIds)
{
    QString elements("id,mailfile,parentaccountid,parentfolderId");
    if (option == QMailStore::CreateRemovalRecord)
        elements += ",serveruid";

    QVariantList removalAccountIds;
    QVariantList removalServerUids;
    QVariantList removalFolderIds;

    {
        // Get the information we need to delete these messages
        QSqlQuery query(simpleQuery(QString("SELECT %1 FROM mailmessages").arg(elements),
                                    Key(key),
                                    "deleteMessages info query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;

        while (query.next()) {
            QMailMessageId messageId(extractValue<quint64>(query.value(0)));
            deletedMessageIds.append(messageId);
            
            QString contentUri(extractValue<QString>(query.value(1)));
            if (!contentUri.isEmpty())
                expiredContent.append(contentUri);

            QMailAccountId parentAccountId(extractValue<quint64>(query.value(2)));
            if (!modifiedAccountIds.contains(parentAccountId))
                modifiedAccountIds.append(parentAccountId);

            QMailFolderId folderId(extractValue<quint64>(query.value(3)));
            if (!modifiedFolderIds.contains(folderId))
                modifiedFolderIds.append(folderId);

            if (option == QMailStore::CreateRemovalRecord) {
                // Extract the info needed to create removal records
                removalAccountIds.append(parentAccountId.toULongLong());
                removalServerUids.append(extractValue<QString>(query.value(4)));
                removalFolderIds.append(folderId.toULongLong());
            }
        }
    }

    // No messages? Then we're already done
    if (deletedMessageIds.isEmpty())
        return true;

    if (!modifiedFolderIds.isEmpty()) {
        // Any ancestor folders of the directly modified folders are indirectly modified
        QSqlQuery query(simpleQuery("SELECT DISTINCT id FROM mailfolderlinks",
                                    Key("descendantid", QMailFolderKey::id(modifiedFolderIds)),
                                    "deleteMessages mailfolderlinks ancestor query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;

        while (query.next())
            modifiedFolderIds.append(QMailFolderId(extractValue<quint64>(query.value(0))));
    }

    // Insert the removal records
    if (!removalAccountIds.isEmpty()) {
        // WARNING - QList::operator<<(QList) actually appends the list items to the object,
        // rather than insert the actual list!
        QSqlQuery query(batchQuery("INSERT INTO deletedmessages (parentaccountid,serveruid,parentfolderid) VALUES (?,?,?)",
                                   QVariantList() << QVariant(removalAccountIds)
                                                  << QVariant(removalServerUids)
                                                  << QVariant(removalFolderIds),
                                   "deleteMessages insert removal records query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Delete any custom fields associated with these messages
        QSqlQuery query(simpleQuery("DELETE FROM mailmessagecustom",
                                    Key(QMailMessageKey::id(deletedMessageIds)),
                                    "deleteMessages delete mailmessagecustom query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Delete any identifiers associated with these messages
        QSqlQuery query(simpleQuery("DELETE FROM mailmessageidentifiers",
                                    Key(QMailMessageKey::id(deletedMessageIds)),
                                    "deleteMessages delete mailmessageidentifiers query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Delete any missing message identifiers associated with these messages
        QSqlQuery query(simpleQuery("DELETE FROM missingmessages",
                                    Key(QMailMessageKey::id(deletedMessageIds)),
                                    "deleteMessages delete missingmessages query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Delete any missing ancestor records for these messages
        QSqlQuery query(simpleQuery("DELETE FROM missingancestors",
                                    Key("messageid", QMailMessageKey::id(deletedMessageIds)),
                                    "deleteMessages missing ancestors delete query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Find any messages that need to updated
        QSqlQuery query(simpleQuery("SELECT id FROM mailmessages",
                                    Key("responseid", QMailMessageKey::id(deletedMessageIds)),
                                    "deleteMessages mailmessages updated query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;

        while (query.next())
            updatedMessageIds.append(QMailMessageId(extractValue<quint64>(query.value(0))));
    }

    {
        QMap<QMailMessageId, quint64> predecessors;

        {
            // Find the predecessors for any messages we're removing
            QSqlQuery query(simpleQuery("SELECT id,responseid FROM mailmessages",
                                        Key(QMailMessageKey::id(deletedMessageIds)),
                                        "deleteMessages mailmessages predecessor query"));
            if (query.lastError().type() != QSqlError::NoError)
                return false;

            while (query.next())
                predecessors.insert(QMailMessageId(extractValue<quint64>(query.value(0))), extractValue<quint64>(query.value(1)));
        }

        {
            QVariantList newPredecessorValues;
            QVariantList deletedValues;
            foreach (const QMailMessageId &id, deletedMessageIds) {
                newPredecessorValues.append(QVariant(predecessors[id]));
                deletedValues.append(QVariant(id));
            }

            // Link any descendants of the messages to the deleted messages' predecessor
            QSqlQuery query(batchQuery("UPDATE mailmessages SET responseid=? WHERE responseid=?",
                                       QVariantList() << QVariant(newPredecessorValues) << QVariant(deletedValues),
                                       "deleteMessages mailmessages update query"));
            if (query.lastError().type() != QSqlError::NoError)
                return false;
        }
    }

    {
        // Perform the message deletion
        QSqlQuery query(simpleQuery("DELETE FROM mailmessages",
                                    Key(QMailMessageKey::id(deletedMessageIds)),
                                    "deleteMessages mailmessages delete query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Delete the thread associations of these messages
        QSqlQuery query(simpleQuery("DELETE FROM mailthreadmessages",
                                    Key("messageid", QMailMessageKey::id(deletedMessageIds)),
                                    "deleteMessages mailthreadmessages delete query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Remove any threads that are empty after this deletion
        QSqlQuery query(simpleQuery("DELETE FROM mailthreads WHERE id IN (SELECT id FROM mailthreads mt WHERE 0 = (SELECT COUNT(*) FROM mailthreadmessages WHERE threadid=mt.id) )",
                                    "deleteMessages mailthreads delete query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Remove any subjects that are unreferenced after this deletion
        {
            QSqlQuery query(simpleQuery("DELETE FROM mailthreadsubjects WHERE threadid NOT IN (SELECT id FROM mailthreads)",
                                        "deleteMessages mailthreadsubjects delete query"));
            if (query.lastError().type() != QSqlError::NoError)
                return false;
        }

        {
            QSqlQuery query(simpleQuery("DELETE FROM mailsubjects WHERE id NOT IN (SELECT subjectid FROM mailthreadsubjects)",
                                        "deleteMessages mailthreadsubjects delete query"));
            if (query.lastError().type() != QSqlError::NoError)
                return false;
        }
    }

    // Do not report any deleted entities as updated
    for (QMailMessageIdList::iterator mit = updatedMessageIds.begin(); mit != updatedMessageIds.end(); ) {
        if (deletedMessageIds.contains(*mit)) {
            mit = updatedMessageIds.erase(mit);
        } else {
            ++mit;
        }
    }

    return true;
}

bool QMailStorePrivate::deleteFolders(const QMailFolderKey& key, 
                                      QMailStore::MessageRemovalOption option, 
                                      QMailFolderIdList& deletedFolderIds, 
                                      QMailMessageIdList& deletedMessageIds, 
                                      QStringList& expiredContent, 
                                      QMailMessageIdList& updatedMessageIds, 
                                      QMailFolderIdList& modifiedFolderIds, 
                                      QMailAccountIdList& modifiedAccountIds)
{
    {
        // Get the identifiers for all the folders we're deleting
        QSqlQuery query(simpleQuery("SELECT t0.id FROM mailfolders t0",
                                    Key(key, "t0"),
                                    "deleteFolders info query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;

        while (query.next())
            deletedFolderIds.append(QMailFolderId(extractValue<quint64>(query.value(0))));
    }

    // No folders? Then we're already done
    if (deletedFolderIds.isEmpty()) 
        return true;

    // Create a key to select messages in the folders to be deleted
    QMailMessageKey messagesKey(QMailMessageKey::parentFolderId(key));
    
    // Delete all the messages contained by the folders we're deleting
    if (!deleteMessages(messagesKey, option, deletedMessageIds, expiredContent, updatedMessageIds, modifiedFolderIds, modifiedAccountIds))
        return false;
    
    // Delete any references to these folders in the mailfolderlinks table
    QString statement("DELETE FROM mailfolderlinks WHERE %1 IN ( SELECT t0.id FROM mailfolders t0");
    statement += buildWhereClause(Key(key, "t0")) + " )";

    QVariantList whereValues(::whereClauseValues(key));

    {
        // Delete where target folders are ancestors
        QSqlQuery query(simpleQuery(statement.arg("id"),
                                    whereValues,
                                    "deleteFolders mailfolderlinks ancestor query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Delete where target folders are descendants
        QSqlQuery query(simpleQuery(statement.arg("descendantid"),
                                    whereValues,
                                    "deleteFolders mailfolderlinks descendant query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Delete any custom fields associated with these folders
        QString sql("DELETE FROM mailfoldercustom");
        QSqlQuery query(simpleQuery(sql, Key(QMailFolderKey::id(deletedFolderIds)),
                                    "deleteFolders delete mailfoldercustom query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Perform the folder deletion
        QString sql("DELETE FROM mailfolders");
        QSqlQuery query(simpleQuery(sql, Key(QMailFolderKey::id(deletedFolderIds)),
                                    "deleteFolders delete mailfolders query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    // Do not report any deleted entities as updated
    for (QMailMessageIdList::iterator mit = updatedMessageIds.begin(); mit != updatedMessageIds.end(); ) {
        if (deletedMessageIds.contains(*mit)) {
            mit = updatedMessageIds.erase(mit);
        } else {
            ++mit;
        }
    }

    for (QMailFolderIdList::iterator fit = modifiedFolderIds.begin(); fit != modifiedFolderIds.end(); ) {
        if (deletedFolderIds.contains(*fit)) {
            fit = modifiedFolderIds.erase(fit);
        } else {
            ++fit;
        }
    }

    return true;
}

bool QMailStorePrivate::deleteAccounts(const QMailAccountKey& key, 
                                       QMailAccountIdList& deletedAccountIds, 
                                       QMailFolderIdList& deletedFolderIds, 
                                       QMailMessageIdList& deletedMessageIds, 
                                       QStringList& expiredContent, 
                                       QMailMessageIdList& updatedMessageIds, 
                                       QMailFolderIdList& modifiedFolderIds, 
                                       QMailAccountIdList& modifiedAccountIds)
{
    {
        // Get the identifiers for all the accounts we're deleting
        QSqlQuery query(simpleQuery("SELECT t0.id FROM mailaccounts t0",
                                    Key(key, "t0"),
                                    "deleteAccounts info query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;

        while (query.next())
            deletedAccountIds.append(QMailAccountId(extractValue<quint64>(query.value(0))));
    }

    // No accounts? Then we're already done
    if (deletedAccountIds.isEmpty()) 
        return true;

    // Create a key to select folders from the accounts to be deleted
    QMailFolderKey foldersKey(QMailFolderKey::parentAccountId(key));
    
    // We won't create new message removal records, since there will be no account to link them to
    QMailStore::MessageRemovalOption option(QMailStore::NoRemovalRecord);

    // Delete all the folders contained by the accounts we're deleting
    if (!deleteFolders(foldersKey, option, deletedFolderIds, deletedMessageIds, expiredContent, updatedMessageIds, modifiedFolderIds, modifiedAccountIds))
        return false;
    
    // Also delete any messages belonging to these accounts, that aren't in folders owned by the accounts

    // Create a key to select messages for the accounts to be deleted
    QMailMessageKey messagesKey(QMailMessageKey::parentAccountId(key));

    // Delete all the messages contained by the folders we're deleting
    if (!deleteMessages(messagesKey, option, deletedMessageIds, expiredContent, updatedMessageIds, modifiedFolderIds, modifiedAccountIds))
        return false;

    {
        // Delete the removal records related to these accounts
        QSqlQuery query(simpleQuery("DELETE FROM deletedmessages",
                                    Key("parentaccountid", QMailAccountKey::id(deletedAccountIds)),
                                    "deleteAccounts removal record delete query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Remove any standard folders associated with these accounts
        QSqlQuery query(simpleQuery("DELETE FROM mailaccountfolders",
                                    Key("id", QMailAccountKey::id(deletedAccountIds)),
                                    "deleteAccounts delete mailaccountfolders query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Remove any custom fields associated with these accounts
        QSqlQuery query(simpleQuery("DELETE FROM mailaccountcustom",
                                    Key("id", QMailAccountKey::id(deletedAccountIds)),
                                    "deleteAccounts delete mailaccountcustom query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Remove any configuration fields associated with these accounts
        QSqlQuery query(simpleQuery("DELETE FROM mailaccountconfig",
                                    Key("id", QMailAccountKey::id(deletedAccountIds)),
                                    "deleteAccounts delete mailaccountconfig query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    {
        // Perform the account deletion
        QSqlQuery query(simpleQuery("DELETE FROM mailaccounts",
                                    Key("id", QMailAccountKey::id(deletedAccountIds)),
                                    "deleteAccounts delete mailaccounts query"));
        if (query.lastError().type() != QSqlError::NoError)
            return false;
    }

    // Do not report any deleted entities as updated
    for (QMailMessageIdList::iterator mit = updatedMessageIds.begin(); mit != updatedMessageIds.end(); ) {
        if (deletedMessageIds.contains(*mit)) {
            mit = updatedMessageIds.erase(mit);
        } else {
            ++mit;
        }
    }

    for (QMailFolderIdList::iterator fit = modifiedFolderIds.begin(); fit != modifiedFolderIds.end(); ) {
        if (deletedFolderIds.contains(*fit)) {
            fit = modifiedFolderIds.erase(fit);
        } else {
            ++fit;
        }
    }

    for (QMailAccountIdList::iterator ait = modifiedAccountIds.begin(); ait != modifiedAccountIds.end(); ) {
        if (deletedAccountIds.contains(*ait)) {
            ait = modifiedAccountIds.erase(ait);
        } else {
            ++ait;
        }
    }

    return true;
}

bool QMailStorePrivate::obsoleteContent(const QString& identifier)
{
    QSqlQuery query(simpleQuery("INSERT INTO obsoletefiles (mailfile) VALUES (?)",
                                QVariantList() << QVariant(identifier),
                                "obsoleteContent files insert query"));
    if (query.lastError().type() != QSqlError::NoError) {
        qWarning() << "Unable to record obsolete content:" << identifier;
        return false;
    }

    return true;
}

QSqlQuery QMailStorePrivate::simpleQuery(const QString& statement, const QString& descriptor)
{
    return performQuery(statement, false, QVariantList(), QList<Key>(), qMakePair(0u, 0u), descriptor);
}

QSqlQuery QMailStorePrivate::simpleQuery(const QString& statement, const QVariantList& bindValues, const QString& descriptor)
{
    return performQuery(statement, false, bindValues, QList<Key>(), qMakePair(0u, 0u), descriptor);
}

QSqlQuery QMailStorePrivate::simpleQuery(const QString& statement, const Key& key, const QString& descriptor)
{
    return performQuery(statement, false, QVariantList(), QList<Key>() << key, qMakePair(0u, 0u), descriptor);
}

QSqlQuery QMailStorePrivate::simpleQuery(const QString& statement, const QVariantList& bindValues, const Key& key, const QString& descriptor)
{
    return performQuery(statement, false, bindValues, QList<Key>() << key, qMakePair(0u, 0u), descriptor);
}

QSqlQuery QMailStorePrivate::simpleQuery(const QString& statement, const QVariantList& bindValues, const QList<Key>& keys, const QString& descriptor)
{
    return performQuery(statement, false, bindValues, keys, qMakePair(0u, 0u), descriptor);
}

QSqlQuery QMailStorePrivate::simpleQuery(const QString& statement, const QVariantList& bindValues, const QList<Key>& keys, const QPair<uint, uint> &constraint, const QString& descriptor)
{
    return performQuery(statement, false, bindValues, keys, constraint, descriptor);
}

QSqlQuery QMailStorePrivate::batchQuery(const QString& statement, const QVariantList& bindValues, const QString& descriptor)
{
    return performQuery(statement, true, bindValues, QList<Key>(), qMakePair(0u, 0u), descriptor);
}

QSqlQuery QMailStorePrivate::batchQuery(const QString& statement, const QVariantList& bindValues, const Key& key, const QString& descriptor)
{
    return performQuery(statement, true, bindValues, QList<Key>() << key, qMakePair(0u, 0u), descriptor);
}

QSqlQuery QMailStorePrivate::batchQuery(const QString& statement, const QVariantList& bindValues, const QList<Key>& keys, const QString& descriptor)
{
    return performQuery(statement, true, bindValues, keys, qMakePair(0u, 0u), descriptor);
}

QSqlQuery QMailStorePrivate::performQuery(const QString& statement, bool batch, const QVariantList& bindValues, const QList<Key>& keys, const QPair<uint, uint> &constraint, const QString& descriptor)
{
    QString keyStatements;
    QVariantList keyValues;

    bool firstClause(true);
    foreach (const Key &key, keys) {
        if (key.isType<QMailMessageKey>() || key.isType<QMailFolderKey>() || key.isType<QMailAccountKey>()) {
            keyStatements.append(buildWhereClause(key, false, firstClause));
            keyValues << whereClauseValues(key);
        } else if (key.isType<QMailMessageSortKey>() || key.isType<QMailFolderSortKey>() || key.isType<QMailAccountSortKey>()) {
            keyStatements.append(buildOrderClause(key));
        } else if (key.isType<QString>()) {
            keyStatements.append(key.key<QString>());
        }

        firstClause = false;
    }

    QString constraintStatements;
    if ((constraint.first > 0) || (constraint.second > 0)) {
        if (constraint.first > 0) {
            constraintStatements.append(QString(" LIMIT %1").arg(constraint.first));
        }
        if (constraint.second > 0) {
            constraintStatements.append(QString(" OFFSET %1").arg(constraint.second));
        }
    }

    QSqlQuery query(prepare(statement + keyStatements + constraintStatements));
    if (queryError() != QSqlError::NoError) {
        qMailLog(Messaging) << "Could not prepare query" << descriptor;
    } else {
        foreach (const QVariant& value, bindValues)
            query.addBindValue(value);
        foreach (const QVariant& value, keyValues)
            query.addBindValue(value);

        if (!execute(query, batch)){
            qMailLog(Messaging) << "Could not execute query" << descriptor;
        }
    }

    return query;
}

void QMailStorePrivate::emitIpcNotification(QMailStoreImplementation::AccountUpdateSignal signal, const QMailAccountIdList &ids)
{
    if ((signal == &QMailStore::accountsUpdated) || (signal == &QMailStore::accountsRemoved)) {
        foreach (const QMailAccountId &id, ids)
            accountCache.remove(id);
    }

    QMailStoreImplementation::emitIpcNotification(signal, ids);
}

void QMailStorePrivate::emitIpcNotification(QMailStoreImplementation::FolderUpdateSignal signal, const QMailFolderIdList &ids)
{
    if ((signal == &QMailStore::foldersUpdated) || (signal == &QMailStore::foldersRemoved)) {
        foreach (const QMailFolderId &id, ids)
            folderCache.remove(id);
    }

    QMailStoreImplementation::emitIpcNotification(signal, ids);
}

void QMailStorePrivate::emitIpcNotification(QMailStoreImplementation::MessageUpdateSignal signal, const QMailMessageIdList &ids)
{
    if ((signal == &QMailStore::messagesUpdated) || (signal == &QMailStore::messagesRemoved)) {
        foreach (const QMailMessageId &id, ids)
            messageCache.remove(id);
    }

    QMailStoreImplementation::emitIpcNotification(signal, ids);
}

