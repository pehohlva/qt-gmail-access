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

#include "qmailfoldersortkey.h"
#include "qmailfoldersortkey_p.h"


/*!
    \class QMailFolderSortKey

    \preliminary
    \brief The QMailFolderSortKey class defines the parameters used for sorting a subset of 
    queried folders from the mail store.
    \ingroup messaginglibrary

    A QMailFolderSortKey is composed of a folder property to sort and a sort order. 
    The QMailFolderSortKey class is used in conjunction with the QMailStore::queryFolders() 
    function to sort folder results according to the criteria defined by the sort key.

    For example:
    To create a query for all folders sorted by the path in ascending order:
    \code
    QMailFolderSortKey sortKey(QMailFolderSortKey::path(Qt::Ascending));
    QMailIdList results = QMailStore::instance()->queryFolders(QMailFolderKey(), sortKey);
    \endcode
    
    \sa QMailStore, QMailFolderKey
*/

/*!
    \enum QMailFolderSortKey::Property

    This enum type describes the sortable data properties of a QMailFolder.

    \value Id The ID of the folder.
    \value Path The path of the folder in native form.
    \value ParentFolderId The ID of the parent folder for a given folder.
    \value ParentAccountId The ID of the parent account for a given folder.
    \value DisplayName The name of the folder, designed for display to users.
    \value Status The status value of the folder.
    \value ServerCount The number of messages reported to be on the server for the folder.
    \value ServerUnreadCount The number of unread messages reported to be on the server for the folder.
    \value ServerUndiscoveredCount The number of undiscovered messages reported to be on the server for the folder.
*/

/*!
    \typedef QMailFolderSortKey::ArgumentType
    
    Defines the type used to represent a single sort criterion of a folder sort key.
*/

/*!
    Create a QMailFolderSortKey with specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) sorts no folders. 

    The result of combining an empty key with a non-empty key is the same as the original 
    non-empty key.

    The result of combining two empty keys is an empty key.
*/
QMailFolderSortKey::QMailFolderSortKey()
    : d(new QMailFolderSortKeyPrivate())
{
}

/*! \internal */
QMailFolderSortKey::QMailFolderSortKey(Property p, Qt::SortOrder order, quint64 mask)
    : d(new QMailFolderSortKeyPrivate(p, order, mask))
{
}

/*! \internal */
QMailFolderSortKey::QMailFolderSortKey(const QList<QMailFolderSortKey::ArgumentType> &args)
    : d(new QMailFolderSortKeyPrivate(args))
{
}

/*!
    Create a copy of the QMailFolderSortKey \a other.
*/
QMailFolderSortKey::QMailFolderSortKey(const QMailFolderSortKey& other)
    : d(new QMailFolderSortKeyPrivate())
{
    this->operator=(other);
}

/*!
    Destroys this QMailFolderSortKey.
*/
QMailFolderSortKey::~QMailFolderSortKey()
{
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/
QMailFolderSortKey QMailFolderSortKey::operator&(const QMailFolderSortKey& other) const
{
    return QMailFolderSortKey(d->arguments() + other.d->arguments());
}

/*!
    Performs a logical AND with this key and the key \a other and assigns the result
    to this key.
*/
QMailFolderSortKey& QMailFolderSortKey::operator&=(const QMailFolderSortKey& other)
{
    *this = *this & other;
    return *this;
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns 
    \c false otherwise.
*/
bool QMailFolderSortKey::operator==(const QMailFolderSortKey& other) const
{
    return (*d == *other.d);
}

/*!
    Returns \c true if the value of this key is not the same as the key \a other. Returns
    \c false otherwise.
*/
bool QMailFolderSortKey::operator!=(const QMailFolderSortKey& other) const
{
   return !(*this == other); 
}

/*!
    Assign the value of the QMailFolderSortKey \a other to this.
*/
QMailFolderSortKey& QMailFolderSortKey::operator=(const QMailFolderSortKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false.
*/
bool QMailFolderSortKey::isEmpty() const
{
    return d->isEmpty();
}

/*!
    Returns the list of arguments to this QMailFolderSortKey.
*/
const QList<QMailFolderSortKey::ArgumentType> &QMailFolderSortKey::arguments() const
{
    return d->arguments();
}

/*!
    \fn QMailFolderSortKey::serialize(Stream &stream) const

    Writes the contents of a QMailFolderSortKey to a \a stream.
*/
template <typename Stream> void QMailFolderSortKey::serialize(Stream &stream) const
{
    d->serialize(stream);
}

/*!
    \fn QMailFolderSortKey::deserialize(Stream &stream)

    Reads the contents of a QMailFolderSortKey from \a stream.
*/
template <typename Stream> void QMailFolderSortKey::deserialize(Stream &stream)
{
    d->deserialize(stream);
}

/*!
    Returns a key that sorts folders by their identifiers, according to \a order.

    \sa QMailFolder::id()
*/
QMailFolderSortKey QMailFolderSortKey::id(Qt::SortOrder order)
{
    return QMailFolderSortKey(Id, order);
}

/*!
    Returns a key that sorts folders by their paths, according to \a order.

    \sa QMailFolder::path()
*/
QMailFolderSortKey QMailFolderSortKey::path(Qt::SortOrder order)
{
    return QMailFolderSortKey(Path, order);
}

/*!
    Returns a key that sorts folders by their parent folders' identifiers, according to \a order.

    \sa QMailFolder::parentFolderId()
*/
QMailFolderSortKey QMailFolderSortKey::parentFolderId(Qt::SortOrder order)
{
    return QMailFolderSortKey(ParentFolderId, order);
}

/*!
    Returns a key that sorts folders by their parent accounts' identifiers, according to \a order.

    \sa QMailFolder::parentAccountId()
*/
QMailFolderSortKey QMailFolderSortKey::parentAccountId(Qt::SortOrder order)
{
    return QMailFolderSortKey(ParentAccountId, order);
}

/*!
    Returns a key that sorts folders by their display name, according to \a order.

    \sa QMailFolder::displayName()
*/
QMailFolderSortKey QMailFolderSortKey::displayName(Qt::SortOrder order)
{
    return QMailFolderSortKey(DisplayName, order);
}

/*!
    Returns a key that sorts folders by their message count on server, according to \a order.

    \sa QMailFolder::status()
*/
QMailFolderSortKey QMailFolderSortKey::serverCount(Qt::SortOrder order)
{
    return QMailFolderSortKey(ServerCount, order);
}

/*!
    Returns a key that sorts folders by their message unread count on server, according to \a order.

    \sa QMailFolder::status()
*/
QMailFolderSortKey QMailFolderSortKey::serverUnreadCount(Qt::SortOrder order)
{
    return QMailFolderSortKey(ServerUnreadCount, order);
}

/*!
    Returns a key that sorts folders by their message undiscovered count on server, according to \a order.

    \sa QMailFolder::status()
*/
QMailFolderSortKey QMailFolderSortKey::serverUndiscoveredCount(Qt::SortOrder order)
{
    return QMailFolderSortKey(ServerUndiscoveredCount, order);
}

/*!
    Returns a key that sorts folders by comparing their status value bitwise ANDed with \a mask, according to \a order.

    \sa QMailFolder::status()
*/
QMailFolderSortKey QMailFolderSortKey::status(quint64 mask, Qt::SortOrder order)
{
    return QMailFolderSortKey(Status, order, mask);
}


Q_IMPLEMENT_USER_METATYPE(QMailFolderSortKey);

