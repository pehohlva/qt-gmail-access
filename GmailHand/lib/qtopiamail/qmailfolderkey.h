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

#ifndef QMAILFOLDERKEY_H
#define QMAILFOLDERKEY_H

#include "qmaildatacomparator.h"
#include "qmailkeyargument.h"
#include <QList>
#include <QSharedData>
#include "qmailid.h"
#include <QVariant>
#include "qmailipc.h"
#include "qmailglobal.h"

class QMailAccountKey;

class QMailFolderKeyPrivate;

template <typename Key>
class MailKeyImpl;

class QTOPIAMAIL_EXPORT QMailFolderKey
{
public:
    enum Property
    {
        Id = (1 << 0),
        Path = (1 << 1),
        ParentFolderId = (1 << 2),
        ParentAccountId = (1 << 3),
        DisplayName = (1 << 4),
        Status = (1 << 5),
        AncestorFolderIds = (1 << 6),
        ServerCount = (1 << 7),
        ServerUnreadCount = (1 << 8),
        ServerUndiscoveredCount = (1 << 9),
        Custom = (1 << 10)
    };

    typedef QMailFolderId IdType;
    typedef QMailKeyArgument<Property> ArgumentType;

    QMailFolderKey();
    QMailFolderKey(const QMailFolderKey& other);
    virtual ~QMailFolderKey();

    QMailFolderKey operator~() const;
    QMailFolderKey operator&(const QMailFolderKey& other) const;
    QMailFolderKey operator|(const QMailFolderKey& other) const;
    const QMailFolderKey& operator&=(const QMailFolderKey& other);
    const QMailFolderKey& operator|=(const QMailFolderKey& other);

    bool operator==(const QMailFolderKey& other) const;
    bool operator !=(const QMailFolderKey& other) const;

    const QMailFolderKey& operator=(const QMailFolderKey& other);

    bool isEmpty() const;
    bool isNonMatching() const;
    bool isNegated() const;

    //for subqueries 
    operator QVariant() const;

    const QList<ArgumentType> &arguments() const;
    const QList<QMailFolderKey> &subKeys() const;

    QMailKey::Combiner combiner() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    static QMailFolderKey nonMatchingKey();

    static QMailFolderKey id(const QMailFolderId &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailFolderKey id(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailFolderKey id(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailFolderKey path(const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailFolderKey path(const QString &value, QMailDataComparator::InclusionComparator cmp);
    static QMailFolderKey path(const QStringList &values, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailFolderKey parentFolderId(const QMailFolderId &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailFolderKey parentFolderId(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailFolderKey parentFolderId(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailFolderKey parentAccountId(const QMailAccountId &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailFolderKey parentAccountId(const QMailAccountIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailFolderKey parentAccountId(const QMailAccountKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailFolderKey displayName(const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailFolderKey displayName(const QString &value, QMailDataComparator::InclusionComparator cmp);
    static QMailFolderKey displayName(const QStringList &values, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailFolderKey status(quint64 mask, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailFolderKey status(quint64 mask, QMailDataComparator::EqualityComparator cmp);

    static QMailFolderKey ancestorFolderIds(const QMailFolderId &id, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailFolderKey ancestorFolderIds(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailFolderKey ancestorFolderIds(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailFolderKey serverCount(int value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailFolderKey serverCount(int value, QMailDataComparator::RelationComparator cmp);

    static QMailFolderKey serverUnreadCount(int value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailFolderKey serverUnreadCount(int value, QMailDataComparator::RelationComparator cmp);

    static QMailFolderKey serverUndiscoveredCount(int value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailFolderKey serverUndiscoveredCount(int value, QMailDataComparator::RelationComparator cmp);

    static QMailFolderKey customField(const QString &name, QMailDataComparator::PresenceComparator cmp = QMailDataComparator::Present);
    static QMailFolderKey customField(const QString &name, const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailFolderKey customField(const QString &name, const QString &value, QMailDataComparator::InclusionComparator cmp);

private:
    QMailFolderKey(Property p, const QVariant& value, QMailKey::Comparator c);

    template <typename ListType>
    QMailFolderKey(const ListType &valueList, Property p, QMailKey::Comparator c);

    friend class QMailFolderKeyPrivate;
    friend class MailKeyImpl<QMailFolderKey>;

    QSharedDataPointer<QMailFolderKeyPrivate> d;
};

Q_DECLARE_USER_METATYPE(QMailFolderKey);

#endif
