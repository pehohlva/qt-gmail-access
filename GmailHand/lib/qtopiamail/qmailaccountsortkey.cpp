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

#include "qmailaccountsortkey.h"
#include "qmailaccountsortkey_p.h"

/*!
    \class QMailAccountSortKey

    \preliminary
    \brief The QMailAccountSortKey class defines the parameters used for sorting a subset of 
    queried accounts from the store.
    \ingroup messaginglibrary

    A QMailAccountSortKey is composed of an account property to sort and a sort order. 
    The QMailAccountSortKey class is used in conjunction with the QMailStore::query() 
    function to sort account results according to the criteria defined by the sort key.

    For example:
    To create a query for all accounts sorted by the name in ascending order:
    \code
    QMailAccountSortKey sortNameKey(QMailAccountSortKey::name(Qt::Ascending));
    QMailAccountIdList results = QMailStore::instance()->query(sortNameKey);
    \endcode
    
    \sa QMailStore
*/

/*!
    \enum QMailAccountSortKey::Property

    This enum type describes the sortable data properties of a QMailFolder.

    \value Id The ID of the account.
    \value Name The name of the account.
    \value MessageType The type of messages handled by the account.
    \value Status The status value of the account.
*/

/*!
    \typedef QMailAccountSortKey::ArgumentType
    
    Defines the type used to represent a single sort criterion of an account sort key.
*/

/*!
    Create a QMailAccountSortKey with specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) sorts no folders. 

    The result of combining an empty key with a non-empty key is the same as the original 
    non-empty key.

    The result of combining two empty keys is an empty key.
*/

QMailAccountSortKey::QMailAccountSortKey()
    : d(new QMailAccountSortKeyPrivate())
{
}

/*! \internal */
QMailAccountSortKey::QMailAccountSortKey(Property p, Qt::SortOrder order, quint64 mask)
    : d(new QMailAccountSortKeyPrivate(p, order, mask))
{
}

/*! \internal */
QMailAccountSortKey::QMailAccountSortKey(const QList<QMailAccountSortKey::ArgumentType> &args)
    : d(new QMailAccountSortKeyPrivate(args))
{
}

/*!
    Create a copy of the QMailAccountSortKey \a other.
*/
QMailAccountSortKey::QMailAccountSortKey(const QMailAccountSortKey& other)
    : d(new QMailAccountSortKeyPrivate())
{
    this->operator=(other);
}

/*!
    Destroys this QMailAccountSortKey.
*/
QMailAccountSortKey::~QMailAccountSortKey()
{
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/
QMailAccountSortKey QMailAccountSortKey::operator&(const QMailAccountSortKey& other) const
{
    return QMailAccountSortKey(d->arguments() + other.d->arguments());
}

/*!
    Performs a logical AND with this key and the key \a other and assigns the result
    to this key.
*/
QMailAccountSortKey& QMailAccountSortKey::operator&=(const QMailAccountSortKey& other)
{
    *this = *this & other;
    return *this;
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns 
    \c false otherwise.
*/
bool QMailAccountSortKey::operator==(const QMailAccountSortKey& other) const
{
    return (*d == *other.d);
}

/*!
    Returns \c true if the value of this key is not the same as the key \a other. Returns
    \c false otherwise.
*/
bool QMailAccountSortKey::operator!=(const QMailAccountSortKey& other) const
{
   return !(*this == other); 
}

/*!
    Assign the value of the QMailAccountSortKey \a other to this.
*/
QMailAccountSortKey& QMailAccountSortKey::operator=(const QMailAccountSortKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false.
*/
bool QMailAccountSortKey::isEmpty() const
{
    return d->isEmpty();
}

/*!
    Returns the list of arguments to this QMailAccountSortKey.
*/
const QList<QMailAccountSortKey::ArgumentType> &QMailAccountSortKey::arguments() const
{
    return d->arguments();
}

/*!
    \fn QMailAccountSortKey::serialize(Stream &stream) const

    Writes the contents of a QMailAccountSortKey to a \a stream.
*/
template <typename Stream> void QMailAccountSortKey::serialize(Stream &stream) const
{
    d->serialize(stream);
}

/*!
    \fn QMailAccountSortKey::deserialize(Stream &stream)

    Reads the contents of a QMailAccountSortKey from \a stream.
*/
template <typename Stream> void QMailAccountSortKey::deserialize(Stream &stream)
{
    d->deserialize(stream);
}

/*!
    Returns a key that sorts accounts by their identifiers, according to \a order.

    \sa QMailAccount::id()
*/
QMailAccountSortKey QMailAccountSortKey::id(Qt::SortOrder order)
{
    return QMailAccountSortKey(Id, order);
}

/*!
    Returns a key that sorts accounts by their names, according to \a order.

    \sa QMailAccount::name()
*/
QMailAccountSortKey QMailAccountSortKey::name(Qt::SortOrder order)
{
    return QMailAccountSortKey(Name, order);
}

/*!
    Returns a key that sorts accounts by the message type they handle, according to \a order.

    \sa QMailAccount::messageType()
*/
QMailAccountSortKey QMailAccountSortKey::messageType(Qt::SortOrder order)
{
    return QMailAccountSortKey(MessageType, order);
}

/*!
    Returns a key that sorts accounts by comparing their status value bitwise ANDed with \a mask, according to \a order.

    \sa QMailAccount::status()
*/
QMailAccountSortKey QMailAccountSortKey::status(quint64 mask, Qt::SortOrder order)
{
    return QMailAccountSortKey(Status, order, mask);
}


Q_IMPLEMENT_USER_METATYPE(QMailAccountSortKey);

