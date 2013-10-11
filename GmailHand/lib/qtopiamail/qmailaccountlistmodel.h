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

#ifndef QMAILACCOUNTLISTMODEL_H
#define QMAILACCOUNTLISTMODEL_H

#include <QAbstractListModel>
#include "qmailaccountkey.h"
#include "qmailaccountsortkey.h"

class QMailAccountListModelPrivate;

class QTOPIAMAIL_EXPORT QMailAccountListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        NameTextRole = Qt::UserRole,
        MessageTypeRole,
        MessageSourcesRole,
        MessageSinksRole
    };

public:
    QMailAccountListModel(QObject* parent = 0);
    virtual ~QMailAccountListModel();

    int rowCount(const QModelIndex& index = QModelIndex()) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    QMailAccountKey key() const;
    void setKey(const QMailAccountKey& key);

    QMailAccountSortKey sortKey() const;
    void setSortKey(const QMailAccountSortKey& sortKey);

    QMailAccountId idFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromId(const QMailAccountId& id) const;

    bool synchronizeEnabled() const;
    void setSynchronizeEnabled(bool val);

private slots:
    void accountsAdded(const QMailAccountIdList& ids); 
    void accountsUpdated(const QMailAccountIdList& ids);
    void accountsRemoved(const QMailAccountIdList& ids);

private:
    void fullRefresh();

private:
    QMailAccountListModelPrivate* d;

};

#endif
