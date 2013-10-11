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

#include "qmailfolderkey.h"
#include "qmailfolderkey_p.h"

#include "qmailaccountkey.h"
#include <QStringList>

using namespace QMailKey;

/*!
    \class QMailFolderKey

    \preliminary
    \brief The QMailFolderKey class defines the parameters used for querying a subset of
    all available folders from the mail store.
    \ingroup messaginglibrary

    A QMailFolderKey is composed of a folder property, an optional comparison operator
    and a comparison value. The QMailFolderKey class is used in conjunction with the 
    QMailStore::queryFolders() and QMailStore::countFolders() functions to filter results 
    which meet the criteria defined by the key.

    QMailFolderKey's can be combined using the logical operators (&), (|) and (~) to
    build more sophisticated queries.

    For example:

    To create a query for folders with paths containing "inbox" or "sms":
    \code
    QMailFolderKey inboxKey(QMailFolderKey::path("inbox", QMailDataComparator::Includes));
    QMailFolderKey smsKey(QMailFolderKey::path("sms", QMailDataComparator::Includes));
    QMailFolderIdList results = QMailStore::instance()->queryFolders(inboxKey | smsKey);
    \endcode

    To query all folders with name containing "foo" for a specified account:
    \code
    QMailFolderIdList fooFolders(const QMailAccountId& accountId)
    {
        QMailFolderKey nameKey(QMailFolderKey::displayName("foo", QMailDataComparator::Includes);
        QMailFolderKey accountKey(QMailFolderKey::parentAccountId(accountId));

        return QMailStore::instance()->queryFolders(nameKey & accountKey);
    }
    \endcode

    \sa QMailStore, QMailFolder
*/

/*!
    \enum QMailFolderKey::Property

    This enum type describes the queryable data properties of a QMailFolder.

    \value Id The ID of the folder.
    \value Path The path of the folder in native form.
    \value ParentFolderId The ID of the parent folder for a given folder.
    \value ParentAccountId The ID of the parent account for this folder.
    \value DisplayName The name of the folder, designed for display to users.
    \value Status The status value of the folder.
    \value AncestorFolderIds The set of IDs of folders which are direct or indirect parents of this folder.
    \value ServerCount The number of messages reported to be on the server for the folder.
    \value ServerUnreadCount The number of unread messages reported to be on the server for the folder.
    \value ServerUndiscoveredCount The number of undiscovered messages reported to be on the server for the folder.
    \value Custom The custom fields of the folder.
*/

/*!
    \typedef QMailFolderKey::IdType
    \internal
*/

/*!
    \typedef QMailFolderKey::ArgumentType
    
    Defines the type used to represent a single criterion of a folder filter.

    Synonym for QMailKeyArgument<QMailFolderKey::Property>.
*/

Q_IMPLEMENT_USER_METATYPE(QMailFolderKey);

/*!
    Creates a QMailFolderKey without specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) matches all folders. 

    \sa isEmpty()
*/
QMailFolderKey::QMailFolderKey()
    : d(new QMailFolderKeyPrivate)
{
}

/*!
    Constructs a QMailFolderKey which defines a query parameter where
    QMailFolder::Property \a p is compared using comparison operator
    \a c with a value \a value.
*/
QMailFolderKey::QMailFolderKey(Property p, const QVariant& value, QMailKey::Comparator c)
    : d(new QMailFolderKeyPrivate(p, value, c))
{
}

/*! 
    \fn QMailFolderKey:: QMailFolderKey(const ListType &, Property, QMailKey::Comparator)
    \internal 
*/
template <typename ListType>
QMailFolderKey::QMailFolderKey(const ListType &valueList, QMailFolderKey::Property p, QMailKey::Comparator c)
    : d(new QMailFolderKeyPrivate(valueList, p, c))
{
}

/*!
    Creates a copy of the QMailFolderKey \a other.
*/
QMailFolderKey::QMailFolderKey(const QMailFolderKey& other)
{
    d = other.d;
}

/*!
    Destroys this QMailFolderKey.
*/
QMailFolderKey::~QMailFolderKey()
{
}

/*!
    Returns a key that is the logical NOT of the value of this key.

    If this key is empty, the result will be a non-matching key; if this key is 
    non-matching, the result will be an empty key.

    \sa isEmpty(), isNonMatching()
*/
QMailFolderKey QMailFolderKey::operator~() const
{
    return QMailFolderKeyPrivate::negate(*this);
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/
QMailFolderKey QMailFolderKey::operator&(const QMailFolderKey& other) const
{
    return QMailFolderKeyPrivate::andCombine(*this, other);
}

/*!
    Returns a key that is the logical OR of this key and the value of key \a other.
*/
QMailFolderKey QMailFolderKey::operator|(const QMailFolderKey& other) const
{
    return QMailFolderKeyPrivate::orCombine(*this, other);
}

/*!
    Performs a logical AND with this key and the key \a other and assigns the result
    to this key.
*/
const QMailFolderKey& QMailFolderKey::operator&=(const QMailFolderKey& other)
{
    return QMailFolderKeyPrivate::andAssign(*this, other);
}

/*!
    Performs a logical OR with this key and the key \a other and assigns the result
    to this key.
*/
const QMailFolderKey& QMailFolderKey::operator|=(const QMailFolderKey& other) 
{
    return QMailFolderKeyPrivate::orAssign(*this, other);
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns 
    \c false otherwise.
*/
bool QMailFolderKey::operator==(const QMailFolderKey& other) const
{
    return d->operator==(*other.d);
}

/*!
    Returns \c true if the value of this key is not the same as the key \a other. Returns
    \c false otherwise.
*/
bool QMailFolderKey::operator!=(const QMailFolderKey& other) const
{
    return !d->operator==(*other.d);
}

/*!
    Assign the value of the QMailFolderKey \a other to this.
*/
const QMailFolderKey& QMailFolderKey::operator=(const QMailFolderKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false. 

    An empty key matches all folders.

    The result of combining an empty key with a non-empty key is the original non-empty key. 
    This is true regardless of whether the combination is formed by an AND or an OR operation.

    The result of combining two empty keys is an empty key.

    \sa isNonMatching()
*/
bool QMailFolderKey::isEmpty() const
{
    return d->isEmpty();
}

/*!
    Returns true if the key is a non-matching key; otherwise returns false.

    A non-matching key does not match any folders.

    The result of ANDing a non-matching key with a matching key is a non-matching key.
    The result of ORing a non-matching key with a matching key is the original matching key.

    The result of combining two non-matching keys is a non-matching key.

    \sa nonMatchingKey(), isEmpty()
*/
bool QMailFolderKey::isNonMatching() const
{
    return d->isNonMatching();
}

/*! 
    Returns true if the key's criteria should be negated in application.
*/
bool QMailFolderKey::isNegated() const
{
    return d->negated;
}

/*!
    Returns the QVariant representation of this QMailFolderKey. 
*/
QMailFolderKey::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

/*!
    Returns the list of arguments to this QMailFolderKey.
*/
const QList<QMailFolderKey::ArgumentType> &QMailFolderKey::arguments() const
{
    return d->arguments;
}

/*!
    Returns the list of sub keys held by this QMailFolderKey.
*/
const QList<QMailFolderKey> &QMailFolderKey::subKeys() const
{
    return d->subKeys;
}

/*! 
    Returns the combiner used to combine arguments or sub keys of this QMailFolderKey.
*/
QMailKey::Combiner QMailFolderKey::combiner() const
{
    return d->combiner;
}

/*!
    \fn QMailFolderKey::serialize(Stream &stream) const

    Writes the contents of a QMailFolderKey to a \a stream.
*/
template <typename Stream> void QMailFolderKey::serialize(Stream &stream) const
{
    d->serialize(stream);
}

/*!
    \fn QMailFolderKey::deserialize(Stream &stream)

    Reads the contents of a QMailFolderKey from \a stream.
*/
template <typename Stream> void QMailFolderKey::deserialize(Stream &stream)
{
    d->deserialize(stream);
}

/*!
    Returns a key that does not match any folders (unlike an empty key).

    \sa isNonMatching(), isEmpty()
*/
QMailFolderKey QMailFolderKey::nonMatchingKey()
{
    return QMailFolderKeyPrivate::nonMatchingKey();
}

/*!
    Returns a key matching folders whose identifier matches \a id, according to \a cmp.

    \sa QMailFolder::id()
*/
QMailFolderKey QMailFolderKey::id(const QMailFolderId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailFolderKey(Id, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose identifier is a member of \a ids, according to \a cmp.

    \sa QMailFolder::id()
*/
QMailFolderKey QMailFolderKey::id(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(ids, Id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailFolder::id()
*/
QMailFolderKey QMailFolderKey::id(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(Id, key, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose path matches \a value, according to \a cmp.

    \sa QMailFolder::path()
*/
QMailFolderKey QMailFolderKey::path(const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailFolderKey(Path, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose path matches the substring \a value, according to \a cmp.

    \sa QMailFolder::path()
*/
QMailFolderKey QMailFolderKey::path(const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(Path, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose path is a member of \a values, according to \a cmp.

    \sa QMailFolder::path()
*/
QMailFolderKey QMailFolderKey::path(const QStringList &values, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(values, Path, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose parent folder's identifier matches \a id, according to \a cmp.

    \sa QMailFolder::parentFolderId()
*/
QMailFolderKey QMailFolderKey::parentFolderId(const QMailFolderId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailFolderKey(ParentFolderId, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose parent folder's identifier is a member of \a ids, according to \a cmp.

    \sa QMailFolder::parentFolderId()
*/
QMailFolderKey QMailFolderKey::parentFolderId(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(ids, ParentFolderId, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose parent folder's identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailFolder::parentFolderId()
*/
QMailFolderKey QMailFolderKey::parentFolderId(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(ParentFolderId, key, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose parent account's identifier matches \a id, according to \a cmp.

    \sa QMailFolder::parentAccountId()
*/
QMailFolderKey QMailFolderKey::parentAccountId(const QMailAccountId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailFolderKey(ParentAccountId, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose parent account's identifier is a member of \a ids, according to \a cmp.

    \sa QMailFolder::parentAccountId()
*/
QMailFolderKey QMailFolderKey::parentAccountId(const QMailAccountIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(ids, ParentAccountId, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose parent account's identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailFolder::parentAccountId()
*/
QMailFolderKey QMailFolderKey::parentAccountId(const QMailAccountKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(ParentAccountId, key, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose display name matches \a value, according to \a cmp.

    \sa QMailFolder::displayName()
*/
QMailFolderKey QMailFolderKey::displayName(const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailFolderKey(DisplayName, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose display name matches the substring \a value, according to \a cmp.

    \sa QMailFolder::displayName()
*/
QMailFolderKey QMailFolderKey::displayName(const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(DisplayName, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose display name is a member of \a values, according to \a cmp.

    \sa QMailFolder::displayName()
*/
QMailFolderKey QMailFolderKey::displayName(const QStringList &values, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(values, DisplayName, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose status matches \a value, according to \a cmp.

    \sa QMailFolder::status()
*/
QMailFolderKey QMailFolderKey::status(quint64 value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailFolderKey(Status, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose status is a bitwise match to \a mask, according to \a cmp.

    \sa QMailFolder::status()
*/
QMailFolderKey QMailFolderKey::status(quint64 mask, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(Status, mask, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose ancestor folders' identifiers contain \a id, according to \a cmp.

    \sa QMailFolder::parentFolderId()
*/
QMailFolderKey QMailFolderKey::ancestorFolderIds(const QMailFolderId &id, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(AncestorFolderIds, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose ancestor folders' identifiers contain a member of \a ids, according to \a cmp.

    \sa QMailFolder::parentFolderId()
*/
QMailFolderKey QMailFolderKey::ancestorFolderIds(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(ids, AncestorFolderIds, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose ancestor folders' identifiers contain a member of the set yielded by \a key, according to \a cmp.

    \sa QMailFolder::parentFolderId()
*/
QMailFolderKey QMailFolderKey::ancestorFolderIds(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(AncestorFolderIds, key, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose serverCount matches \a value, according to \a cmp.

    \sa QMailFolder::serverCount()
*/
QMailFolderKey QMailFolderKey::serverCount(int value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailFolderKey(ServerCount, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose serverCount has the relation to \a value that is specified by \a cmp.

    \sa QMailFolder::serverCount()
*/
QMailFolderKey QMailFolderKey::serverCount(int value, QMailDataComparator::RelationComparator cmp)
{
    return QMailFolderKey(ServerCount, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose serverUnreadCount matches \a value, according to \a cmp.

    \sa QMailFolder::serverUnreadCount()
*/
QMailFolderKey QMailFolderKey::serverUnreadCount(int value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailFolderKey(ServerUnreadCount, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose serverUnreadCount has the relation to \a value that is specified by \a cmp.

    \sa QMailFolder::serverUnreadCount()
*/
QMailFolderKey QMailFolderKey::serverUnreadCount(int value, QMailDataComparator::RelationComparator cmp)
{
    return QMailFolderKey(ServerUnreadCount, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose serverUndiscoveredCount matches \a value, according to \a cmp.

    \sa QMailFolder::serverUndiscoveredCount()
*/
QMailFolderKey QMailFolderKey::serverUndiscoveredCount(int value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailFolderKey(ServerUndiscoveredCount, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders whose serverUndiscoveredCount has the relation to \a value that is specified by \a cmp.

    \sa QMailFolder::serverUndiscoveredCount()
*/
QMailFolderKey QMailFolderKey::serverUndiscoveredCount(int value, QMailDataComparator::RelationComparator cmp)
{
    return QMailFolderKey(ServerUndiscoveredCount, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders that possess a custom field with the name \a name, according to \a cmp.

    \sa QMailFolder::customField()
*/
QMailFolderKey QMailFolderKey::customField(const QString &name, QMailDataComparator::PresenceComparator cmp)
{
    return QMailFolderKey(Custom, QStringList() << QMailKey::stringValue(name), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders that possess a custom field with the name \a name, whose value matches \a value, according to \a cmp.

    \sa QMailFolder::customField()
*/
QMailFolderKey QMailFolderKey::customField(const QString &name, const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailFolderKey(Custom, QStringList() << QMailKey::stringValue(name) << QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching folders that possess a custom field with the name \a name, whose value matches the substring \a value, according to \a cmp.

    \sa QMailFolder::customField()
*/
QMailFolderKey QMailFolderKey::customField(const QString &name, const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailFolderKey(Custom, QStringList() << QMailKey::stringValue(name) << QMailKey::stringValue(value), QMailKey::comparator(cmp));
}


