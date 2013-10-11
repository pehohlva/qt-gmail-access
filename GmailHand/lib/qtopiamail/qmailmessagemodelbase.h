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

#ifndef QMAILMESSAGEMODELBASE_H
#define QMAILMESSAGEMODELBASE_H

#include <QAbstractListModel>
#include "qmailglobal.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"


class QMailMessageModelImplementation
{
public:
    virtual ~QMailMessageModelImplementation();

    virtual QMailMessageKey key() const = 0;
    virtual void setKey(const QMailMessageKey& key) = 0;

    virtual QMailMessageSortKey sortKey() const = 0;
    virtual void setSortKey(const QMailMessageSortKey& sortKey) = 0;

    virtual bool isEmpty() const = 0;

    virtual int rowCount(const QModelIndex& idx) const = 0;
    virtual int columnCount(const QModelIndex& idx) const = 0;

    virtual QMailMessageId idFromIndex(const QModelIndex& index) const = 0;
    virtual QModelIndex indexFromId(const QMailMessageId& id) const = 0;

    virtual Qt::CheckState checkState(const QModelIndex &idx) const = 0;
    virtual void setCheckState(const QModelIndex &idx, Qt::CheckState state) = 0;

    virtual void reset() = 0;

    virtual bool ignoreMailStoreUpdates() const = 0;
    virtual bool setIgnoreMailStoreUpdates(bool ignore) = 0;

    virtual bool processMessagesAdded(const QMailMessageIdList& ids) = 0;
    virtual bool processMessagesUpdated(const QMailMessageIdList& ids) = 0;
    virtual bool processMessagesRemoved(const QMailMessageIdList& ids) = 0;
};


class QTOPIAMAIL_EXPORT QMailMessageModelBase : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles 
    {
        MessageAddressTextRole = Qt::UserRole,
        MessageSubjectTextRole,
        MessageFilterTextRole,
        MessageTimeStampTextRole,
        MessageSizeTextRole,
        MessageTypeIconRole,
        MessageStatusIconRole,
        MessageDirectionIconRole,
        MessagePresenceIconRole,
        MessageBodyTextRole,
        MessageIdRole
    };

    QMailMessageModelBase(QObject* parent = 0);
    virtual ~QMailMessageModelBase();

    int rowCount(const QModelIndex& index = QModelIndex()) const;
    int columnCount(const QModelIndex& index = QModelIndex()) const;

    bool isEmpty() const;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    QMailMessageKey key() const;
    void setKey(const QMailMessageKey& key);

    QMailMessageSortKey sortKey() const;
    void setSortKey(const QMailMessageSortKey& sortKey);

    QMailMessageId idFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromId(const QMailMessageId& id) const;

    bool ignoreMailStoreUpdates() const;
    void setIgnoreMailStoreUpdates(bool ignore);

    virtual QModelIndex generateIndex(int row, int column, void *ptr) = 0;

    void emitDataChanged(const QModelIndex &idx);

    void emitBeginInsertRows(const QModelIndex &idx, int start, int end);
    void emitEndInsertRows();

    void emitBeginRemoveRows(const QModelIndex &idx, int start, int end);
    void emitEndRemoveRows();

signals:
    void modelChanged();

protected slots:
    void messagesAdded(const QMailMessageIdList& ids); 
    void messagesUpdated(const QMailMessageIdList& ids);
    void messagesRemoved(const QMailMessageIdList& ids);

protected:
    virtual QVariant data(const QMailMessageMetaData &metaData, int role) const;

    void fullRefresh(bool modelChanged);

    virtual QMailMessageModelImplementation *impl() = 0;
    virtual const QMailMessageModelImplementation *impl() const = 0;
};

#endif
