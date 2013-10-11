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

#include "qmailmessagekey.h"
#include "qmailmessagekey_p.h"

#include "qmailaccountkey.h"
#include "qmailfolderkey.h"
#include <QDateTime>
#include <QStringList>

#ifndef USE_ALTERNATE_MAILSTORE_IMPLEMENTATION
// This value is defined in qmailstore_p.cpp
const int IdLookupThreshold = 256;
#endif

using namespace QMailKey;

/*!
    \class QMailMessageKey

    \preliminary
    \brief The QMailMessageKey class defines the parameters used for querying a subset of
    all available messages from the mail store.
    \ingroup messaginglibrary

    A QMailMessageKey is composed of a message property, an optional comparison operator
    and a comparison value. The QMailMessageKey class is used in conjunction with the 
    QMailStore::queryMessages() and QMailStore::countMessages() functions to filter results 
    which meet the criteria defined by the key.

    QMailMessageKeys can be combined using the logical operators (&), (|) and (~) to
    create more refined queries.

    For example:

    To create a query for all messages sent from "joe@user.com" with subject "meeting":
    \code
    QMailMessageKey subjectKey(QMailMessageKey::subject("meeting"));
    QMailMessageKey senderKey(QMailMessageKey::sender("joe@user.com"));
    QMailMessageIdList results = QMailStore::instance()->queryMessages(subjectKey & senderKey);
    \endcode

    To query all unread messages from a specific folder:
    \code
    QMailMessageIdList unreadMessagesInFolder(const QMailFolderId& folderId)
    {
        QMailMessageKey parentFolderKey(QMailMessageKey::parentFolderId(folderId));
        QMailMessageKey unreadKey(QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Excludes));

        return QMailStore::instance()->queryMessages(parentFolderKey & unreadKey);
    }
    \endcode

    \sa QMailStore, QMailMessage
*/

/*!
    \enum QMailMessageKey::Property

    This enum type describes the data query properties of a QMailMessage.

    \value Id The ID of the message.
    \value Type The type of the message.
    \value ParentFolderId The parent folder ID this message is contained in.
    \value Sender The message sender address string.
    \value Recipients The message recipient address string.
    \value Subject The message subject string.
    \value TimeStamp The message origination timestamp.
    \value ReceptionTimeStamp The message reception timestamp.
    \value Status The message status flags.
    \value Conversation The set of related messages containing the specified message.
    \value ServerUid The IMAP server UID of the message.
    \value Size The size of the message.
    \value ParentAccountId The ID of the account the message was downloaded from.
    \value AncestorFolderIds The set of IDs of folders which are direct or indirect parents of this message.
    \value ContentType The type of data contained within the message.
    \value PreviousParentFolderId The parent folder ID this message was contained in, prior to moving to the current parent folder.
    \value ContentScheme The scheme used to store the content of the message.
    \value ContentIdentifier The identifier used to store the content of the message.
    \value InResponseTo The identifier of the other message that the message was created in response to.
    \value ResponseType The type of response that the message was created as.
    \value Custom The custom fields of the message.
*/

/*!
    \typedef QMailMessageKey::IdType
    \internal
*/

/*!
    \typedef QMailMessageKey::ArgumentType
    
    Defines the type used to represent a single criterion of a message filter.

    Synonym for QMailKeyArgument<QMailMessageKey::Property>.
*/

/*!
    Creates a QMailMessageKey without specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) matches all messages. 

    \sa isEmpty()
*/
QMailMessageKey::QMailMessageKey()
    : d(new QMailMessageKeyPrivate)
{
}

/*!
    Constructs a QMailMessageKey which defines a query parameter where
    QMailMessage::Property \a p is compared using comparison operator
    \a c with a value \a value.
*/
QMailMessageKey::QMailMessageKey(Property p, const QVariant& value, QMailKey::Comparator c)
    : d(new QMailMessageKeyPrivate(p, value, c))
{
}

/*! 
    \fn QMailMessageKey::QMailMessageKey(const ListType &, Property, QMailKey::Comparator)
    \internal
*/
template <typename ListType>
QMailMessageKey::QMailMessageKey(const ListType &valueList, QMailMessageKey::Property p, QMailKey::Comparator c)
    : d(new QMailMessageKeyPrivate(valueList, p, c))
{
}

/*!
    Creates a copy of the QMailMessageKey \a other.
*/
QMailMessageKey::QMailMessageKey(const QMailMessageKey& other)
{
    d = other.d;
}

/*!
    Destroys the QMailMessageKey
*/
QMailMessageKey::~QMailMessageKey()
{
}

/*!
    Returns a key that is the logical NOT of the value of this key.

    If this key is empty, the result will be a non-matching key; if this key is 
    non-matching, the result will be an empty key.

    \sa isEmpty(), isNonMatching()
*/
QMailMessageKey QMailMessageKey::operator~() const
{
    return QMailMessageKeyPrivate::negate(*this);
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/
QMailMessageKey QMailMessageKey::operator&(const QMailMessageKey& other) const
{
    return QMailMessageKeyPrivate::andCombine(*this, other);
}

/*!
    Returns a key that is the logical OR of this key and the value of key \a other.
*/
QMailMessageKey QMailMessageKey::operator|(const QMailMessageKey& other) const
{
    return QMailMessageKeyPrivate::orCombine(*this, other);
}

/*!
    Performs a logical AND with this key and the key \a other and assigns the result
    to this key.
*/
const QMailMessageKey& QMailMessageKey::operator&=(const QMailMessageKey& other)
{
    return QMailMessageKeyPrivate::andAssign(*this, other);
}

/*!
    Performs a logical OR with this key and the key \a other and assigns the result
    to this key.
*/
const QMailMessageKey& QMailMessageKey::operator|=(const QMailMessageKey& other) 
{
    return QMailMessageKeyPrivate::orAssign(*this, other);
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns 
    \c false otherwise.
*/
bool QMailMessageKey::operator==(const QMailMessageKey& other) const
{
    return d->operator==(*other.d);
}

/*!
    Returns \c true if the value of this key is not the same as the key \a other. Returns
    \c false otherwise.
*/
bool QMailMessageKey::operator!=(const QMailMessageKey& other) const
{
    return !d->operator==(*other.d);
}

/*!
    Assign the value of the QMailMessageKey \a other to this.
*/
const QMailMessageKey& QMailMessageKey::operator=(const QMailMessageKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false. 

    An empty key matches all messages.

    The result of combining an empty key with a non-empty key is the original non-empty key. 
    This is true regardless of whether the combination is formed by an AND or an OR operation.

    The result of combining two empty keys is an empty key.

    \sa isNonMatching()
*/
bool QMailMessageKey::isEmpty() const
{
    return d->isEmpty();
}

/*!
    Returns true if the key is a non-matching key; otherwise returns false.

    A non-matching key does not match any messages.

    The result of ANDing a non-matching key with a matching key is a non-matching key.
    The result of ORing a non-matching key with a matching key is the original matching key.

    The result of combining two non-matching keys is a non-matching key.

    \sa nonMatchingKey(), isEmpty()
*/
bool QMailMessageKey::isNonMatching() const
{
    return d->isNonMatching();
}

/*! 
    Returns true if the key's criteria should be negated in application.
*/
bool QMailMessageKey::isNegated() const
{
    return d->negated;
}

/*!
    Returns the QVariant representation of this QMailMessageKey. 
*/
QMailMessageKey::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

/*!
    Returns the list of arguments to this QMailMessageKey.
*/
const QList<QMailMessageKey::ArgumentType> &QMailMessageKey::arguments() const
{
    return d->arguments;
}

/*!
    Returns the list of sub keys held by this QMailMessageKey.
*/
const QList<QMailMessageKey> &QMailMessageKey::subKeys() const
{
    return d->subKeys;
}

/*! 
    Returns the combiner used to combine arguments or sub keys of this QMailMessageKey.
*/
QMailKey::Combiner QMailMessageKey::combiner() const
{
    return d->combiner;
}

/*!
    \fn QMailMessageKey::serialize(Stream &stream) const

    Writes the contents of a QMailMessageKey to a \a stream.
*/
template <typename Stream> void QMailMessageKey::serialize(Stream &stream) const
{
    d->serialize(stream);
}

/*!
    \fn QMailMessageKey::deserialize(Stream &stream)

    Reads the contents of a QMailMessageKey from \a stream.
*/
template <typename Stream> void QMailMessageKey::deserialize(Stream &stream)
{
    d->deserialize(stream);
}

/*!
    Returns a key that does not match any messages (unlike an empty key).

    \sa isNonMatching(), isEmpty()
*/
QMailMessageKey QMailMessageKey::nonMatchingKey()
{
    return QMailMessageKeyPrivate::nonMatchingKey();
}

/*!
    Returns a key matching messages whose identifier matches \a id, according to \a cmp.

    \sa QMailMessage::id()
*/
QMailMessageKey QMailMessageKey::id(const QMailMessageId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(Id, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose identifier is a member of \a ids, according to \a cmp.

    \sa QMailMessage::id()
*/
QMailMessageKey QMailMessageKey::id(const QMailMessageIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
#ifndef USE_ALTERNATE_MAILSTORE_IMPLEMENTATION
    if (ids.count() >= IdLookupThreshold) {
        // If there are a large number of IDs, they will be inserted into a temporary table
        // with a uniqueness constraint; ensure only unique values are supplied
        return QMailMessageKey(ids.toSet().toList(), Id, QMailKey::comparator(cmp));
    }
#endif

    return QMailMessageKey(ids, Id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailMessage::id()
*/
QMailMessageKey QMailMessageKey::id(const QMailMessageKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(Id, key, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose messageType matches \a type, according to \a cmp.

    \sa QMailMessage::messageType()
*/
QMailMessageKey QMailMessageKey::messageType(QMailMessageMetaDataFwd::MessageType type, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(Type, static_cast<int>(type), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching accounts whose messageType is a bitwise match to \a type, according to \a cmp.

    \sa QMailMessage::messageType()
*/
QMailMessageKey QMailMessageKey::messageType(int type, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(Type, type, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose parent folder's identifier matches \a id, according to \a cmp.

    \sa QMailMessage::parentFolderId()
*/
QMailMessageKey QMailMessageKey::parentFolderId(const QMailFolderId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(ParentFolderId, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose parent folder's identifier is a member of \a ids, according to \a cmp.

    \sa QMailMessage::parentFolderId()
*/
QMailMessageKey QMailMessageKey::parentFolderId(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(ids, ParentFolderId, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose parent folder's identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailMessage::parentFolderId()
*/
QMailMessageKey QMailMessageKey::parentFolderId(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(ParentFolderId, key, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose sender matches \a value, according to \a cmp.

    \sa QMailMessage::from()
*/
QMailMessageKey QMailMessageKey::sender(const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(Sender, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose sender matches the substring \a value, according to \a cmp.

    \sa QMailMessage::from()
*/
QMailMessageKey QMailMessageKey::sender(const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(Sender, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose sender is a member of \a values, according to \a cmp.

    \sa QMailMessage::from()
*/
QMailMessageKey QMailMessageKey::sender(const QStringList &values, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(values, Sender, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose recipients include \a value, according to \a cmp.

    \sa QMailMessage::to(), QMailMessage::cc(), QMailMessage::bcc()
*/
QMailMessageKey QMailMessageKey::recipients(const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(Recipients, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose recipients include the substring \a value, according to \a cmp.

    \sa QMailMessage::to(), QMailMessage::cc(), QMailMessage::bcc()
*/
QMailMessageKey QMailMessageKey::recipients(const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(Recipients, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose subject matches \a value, according to \a cmp.

    \sa QMailMessage::subject()
*/
QMailMessageKey QMailMessageKey::subject(const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(Subject, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose subject matches the substring \a value, according to \a cmp.

    \sa QMailMessage::subject()
*/
QMailMessageKey QMailMessageKey::subject(const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(Subject, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose subject is a member of \a values, according to \a cmp.

    \sa QMailMessage::subject()
*/
QMailMessageKey QMailMessageKey::subject(const QStringList &values, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(values, Subject, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose timestamp matches \a value, according to \a cmp.

    \sa QMailMessage::date()
*/
QMailMessageKey QMailMessageKey::timeStamp(const QDateTime &value, QMailDataComparator::EqualityComparator cmp)
{
    // An invalid QDateTime does not exist-compare correctly, so use a substitute value
    QDateTime x(value.isNull() ? QDateTime::fromTime_t(0) : value);
    return QMailMessageKey(TimeStamp, x, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose timestamp has the relation to \a value that is specified by \a cmp.

    \sa QMailMessage::date()
*/
QMailMessageKey QMailMessageKey::timeStamp(const QDateTime &value, QMailDataComparator::RelationComparator cmp)
{
    return QMailMessageKey(TimeStamp, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose reception timestamp matches \a value, according to \a cmp.

    \sa QMailMessage::receivedDate()
*/
QMailMessageKey QMailMessageKey::receptionTimeStamp(const QDateTime &value, QMailDataComparator::EqualityComparator cmp)
{
    // An invalid QDateTime does not exist-compare correctly, so use a substitute value
    QDateTime x(value.isNull() ? QDateTime::fromTime_t(0) : value);
    return QMailMessageKey(ReceptionTimeStamp, x, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose reception timestamp has the relation to \a value that is specified by \a cmp.

    \sa QMailMessage::receivedDate()
*/
QMailMessageKey QMailMessageKey::receptionTimeStamp(const QDateTime &value, QMailDataComparator::RelationComparator cmp)
{
    return QMailMessageKey(ReceptionTimeStamp, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose status matches \a value, according to \a cmp.

    \sa QMailMessage::status()
*/
QMailMessageKey QMailMessageKey::status(quint64 value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(Status, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose status is a bitwise match to \a mask, according to \a cmp.

    \sa QMailMessage::status()
*/
QMailMessageKey QMailMessageKey::status(quint64 mask, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(Status, mask, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose serverUid matches \a uid, according to \a cmp.

    \sa QMailMessage::serverUid()
*/
QMailMessageKey QMailMessageKey::serverUid(const QString &uid, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(ServerUid, QMailKey::stringValue(uid), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose serverUid matches the substring \a uid, according to \a cmp.

    \sa QMailMessage::serverUid()
*/
QMailMessageKey QMailMessageKey::serverUid(const QString &uid, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(ServerUid, QMailKey::stringValue(uid), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose serverUid is a member of \a uids, according to \a cmp.

    \sa QMailMessage::serverUid()
*/
QMailMessageKey QMailMessageKey::serverUid(const QStringList &uids, QMailDataComparator::InclusionComparator cmp)
{
#ifndef USE_ALTERNATE_MAILSTORE_IMPLEMENTATION
    if (uids.count() >= IdLookupThreshold) {
        // If there are a large number of UIDs, they will be inserted into a temporary table
        // with a uniqueness constraint; ensure only unique values are supplied
        return QMailMessageKey(uids.toSet().toList(), ServerUid, QMailKey::comparator(cmp));
    }
#endif

    return QMailMessageKey(uids, ServerUid, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose size matches \a value, according to \a cmp.

    \sa QMailMessage::size()
*/
QMailMessageKey QMailMessageKey::size(int value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(Size, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose size has the relation to \a value that is specified by \a cmp.

    \sa QMailMessage::size()
*/
QMailMessageKey QMailMessageKey::size(int value, QMailDataComparator::RelationComparator cmp)
{
    return QMailMessageKey(Size, value, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose parent account's identifier matches \a id, according to \a cmp.

    \sa QMailMessage::parentAccountId()
*/
QMailMessageKey QMailMessageKey::parentAccountId(const QMailAccountId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(ParentAccountId, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose parent account's identifier is a member of \a ids, according to \a cmp.

    \sa QMailMessage::parentAccountId()
*/
QMailMessageKey QMailMessageKey::parentAccountId(const QMailAccountIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(ids, ParentAccountId, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose parent account's identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailMessage::parentAccountId()
*/
QMailMessageKey QMailMessageKey::parentAccountId(const QMailAccountKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(ParentAccountId, key, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose ancestor folders' identifiers contain \a id, according to \a cmp.

    \sa QMailMessage::parentFolderId()
*/
QMailMessageKey QMailMessageKey::ancestorFolderIds(const QMailFolderId &id, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(AncestorFolderIds, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose ancestor folders' identifiers contain a member of \a ids, according to \a cmp.

    \sa QMailMessage::parentFolderId()
*/
QMailMessageKey QMailMessageKey::ancestorFolderIds(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(ids, AncestorFolderIds, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose ancestor folders' identifiers contain a member of the set yielded by \a key, according to \a cmp.

    \sa QMailMessage::parentFolderId()
*/
QMailMessageKey QMailMessageKey::ancestorFolderIds(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(AncestorFolderIds, key, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose content type matches \a type, according to \a cmp.

    \sa QMailMessage::content()
*/
QMailMessageKey QMailMessageKey::contentType(QMailMessageMetaDataFwd::ContentType type, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(ContentType, static_cast<int>(type), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose content type is a member of \a types, according to \a cmp.

    \sa QMailMessage::content()
*/
QMailMessageKey QMailMessageKey::contentType(const QList<QMailMessageMetaDataFwd::ContentType> &types, QMailDataComparator::InclusionComparator cmp)
{
    QList<int> x;
    foreach (QMailMessageMetaDataFwd::ContentType type, types)
        x.append(static_cast<int>(type));

    return QMailMessageKey(x, ContentType, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose previous parent folder's identifier matches \a id, according to \a cmp.

    \sa QMailMessage::previousParentFolderId()
*/
QMailMessageKey QMailMessageKey::previousParentFolderId(const QMailFolderId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(PreviousParentFolderId, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose previous parent folder's identifier is a member of \a ids, according to \a cmp.

    \sa QMailMessage::previousParentFolderId()
*/
QMailMessageKey QMailMessageKey::previousParentFolderId(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(ids, PreviousParentFolderId, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose previous parent folder's identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailMessage::previousParentFolderId()
*/
QMailMessageKey QMailMessageKey::previousParentFolderId(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(PreviousParentFolderId, key, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose content scheme matches \a value, according to \a cmp.

    \sa QMailMessage::contentScheme()
*/
QMailMessageKey QMailMessageKey::contentScheme(const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(ContentScheme, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose content scheme matches the substring \a value, according to \a cmp.

    \sa QMailMessage::contentScheme()
*/
QMailMessageKey QMailMessageKey::contentScheme(const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(ContentScheme, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose content identifier matches \a value, according to \a cmp.

    \sa QMailMessage::contentIdentifier()
*/
QMailMessageKey QMailMessageKey::contentIdentifier(const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(ContentIdentifier, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose content identifier matches the substring \a value, according to \a cmp.

    \sa QMailMessage::contentIdentifier()
*/
QMailMessageKey QMailMessageKey::contentIdentifier(const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(ContentIdentifier, QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose precursor message's identifier matches \a id, according to \a cmp.

    \sa QMailMessage::inResponseTo()
*/
QMailMessageKey QMailMessageKey::inResponseTo(const QMailMessageId &id, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(InResponseTo, id, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose precursor message's identifier is a member of \a ids, according to \a cmp.

    \sa QMailMessage::inResponseTo()
*/
QMailMessageKey QMailMessageKey::inResponseTo(const QMailMessageIdList &ids, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(ids, InResponseTo, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose precursor message's identifier is a member of the set yielded by \a key, according to \a cmp.

    \sa QMailMessage::inResponseTo()
*/
QMailMessageKey QMailMessageKey::inResponseTo(const QMailMessageKey &key, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(InResponseTo, key, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose response type matches \a type, according to \a cmp.

    \sa QMailMessage::responseType()
*/
QMailMessageKey QMailMessageKey::responseType(QMailMessageMetaDataFwd::ResponseType type, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(ResponseType, static_cast<int>(type), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages whose response type is a member of \a types, according to \a cmp.

    \sa QMailMessage::responseType()
*/
QMailMessageKey QMailMessageKey::responseType(const QList<QMailMessageMetaDataFwd::ResponseType> &types, QMailDataComparator::InclusionComparator cmp)
{
    QList<int> x;
    foreach (QMailMessageMetaDataFwd::ResponseType type, types)
        x.append(static_cast<int>(type));

    return QMailMessageKey(x, ResponseType, QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages that possess a custom field with the name \a name, according to \a cmp.

    \sa QMailMessage::customField()
*/
QMailMessageKey QMailMessageKey::customField(const QString &name, QMailDataComparator::PresenceComparator cmp)
{
    return QMailMessageKey(Custom, QStringList() << QMailKey::stringValue(name), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages that possess a custom field with the name \a name, whose value matches \a value, according to \a cmp.

    \sa QMailMessage::customField()
*/
QMailMessageKey QMailMessageKey::customField(const QString &name, const QString &value, QMailDataComparator::EqualityComparator cmp)
{
    return QMailMessageKey(Custom, QStringList() << QMailKey::stringValue(name) << QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages that possess a custom field with the name \a name, whose value matches the substring \a value, according to \a cmp.

    \sa QMailMessage::customField()
*/
QMailMessageKey QMailMessageKey::customField(const QString &name, const QString &value, QMailDataComparator::InclusionComparator cmp)
{
    return QMailMessageKey(Custom, QStringList() << QMailKey::stringValue(name) << QMailKey::stringValue(value), QMailKey::comparator(cmp));
}

/*!
    Returns a key matching messages that are participants in the conversation containing the message identified by \a id.

    \sa QMailMessage::inResponseTo()
*/
QMailMessageKey QMailMessageKey::conversation(const QMailMessageId &id)
{
    return QMailMessageKey(Conversation, id, QMailKey::Equal);
}

/*!
    Returns a key matching messages that are participants in any of the conversations containing the messages
    whose identifiers are members of \a ids.

    \sa QMailMessage::inResponseTo()
*/
QMailMessageKey QMailMessageKey::conversation(const QMailMessageIdList &ids)
{
    return QMailMessageKey(ids, Conversation, QMailKey::Includes);
}

/*!
    Returns a key matching messages that are participants in any of the conversations containing the messages
    whose identifiers are members of the set yielded by \a key.

    \sa QMailMessage::inResponseTo()
*/
QMailMessageKey QMailMessageKey::conversation(const QMailMessageKey &key)
{
    return QMailMessageKey(Conversation, key, QMailKey::Includes);
}

Q_IMPLEMENT_USER_METATYPE(QMailMessageKey);

