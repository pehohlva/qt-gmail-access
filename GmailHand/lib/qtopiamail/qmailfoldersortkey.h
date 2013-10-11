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

#ifndef QMAILFOLDERSORTKEY_H
#define QMAILFOLDERSORTKEY_H

#include "qmailglobal.h"
#include "qmailipc.h"
#include "qmailsortkeyargument.h"
#include <QSharedData>
#include <QtGlobal>

class QMailFolderSortKeyPrivate;

class QTOPIAMAIL_EXPORT QMailFolderSortKey
{
public:
    enum Property
    {
        Id,
        Path,
        ParentFolderId,
        ParentAccountId,
        DisplayName,
        Status,
        ServerCount,
        ServerUnreadCount,
        ServerUndiscoveredCount,
    };

    typedef QMailSortKeyArgument<Property> ArgumentType;

public:
    QMailFolderSortKey();
    QMailFolderSortKey(const QMailFolderSortKey& other);
    virtual ~QMailFolderSortKey();

    QMailFolderSortKey operator&(const QMailFolderSortKey& other) const;
    QMailFolderSortKey& operator&=(const QMailFolderSortKey& other);

    bool operator==(const QMailFolderSortKey& other) const;
    bool operator!=(const QMailFolderSortKey& other) const;

    QMailFolderSortKey& operator=(const QMailFolderSortKey& other);

    bool isEmpty() const;

    const QList<ArgumentType> &arguments() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    static QMailFolderSortKey id(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey path(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey parentFolderId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey parentAccountId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey displayName(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey serverCount(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey serverUnreadCount(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey serverUndiscoveredCount(Qt::SortOrder order = Qt::AscendingOrder);

    static QMailFolderSortKey status(quint64 mask, Qt::SortOrder order = Qt::DescendingOrder);

private:
    QMailFolderSortKey(Property p, Qt::SortOrder order, quint64 mask = 0);
    QMailFolderSortKey(const QList<ArgumentType> &args);

    friend class QMailStore;
    friend class QMailStorePrivate;

    QSharedDataPointer<QMailFolderSortKeyPrivate> d;
};

Q_DECLARE_USER_METATYPE(QMailFolderSortKey);

#endif
