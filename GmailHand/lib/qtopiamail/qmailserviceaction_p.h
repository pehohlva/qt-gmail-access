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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QMAILSERVICEACTION_P_H
#define QMAILSERVICEACTION_P_H

#include "qmailserviceaction.h"
#include "qmailmessageserver.h"


// These classes are implemented via qmailmessage.cpp and qmailinstantiations.cpp

class QMailServiceActionPrivate : public QObject, public QPrivateNoncopyableBase
{
    Q_OBJECT

public:
    template<typename Subclass>
    QMailServiceActionPrivate(Subclass *p, QMailServiceAction *i);

    ~QMailServiceActionPrivate();

    void cancelOperation();

protected slots:
    void activityChanged(quint64, QMailServiceAction::Activity activity);
    void connectivityChanged(quint64, QMailServiceAction::Connectivity connectivity);
    void statusChanged(quint64, const QMailServiceAction::Status status);
    void progressChanged(quint64, uint progress, uint total);

protected:
    friend class QMailServiceAction;

    virtual void init();

    quint64 newAction();
    bool validAction(quint64 action);

    void setActivity(QMailServiceAction::Activity newActivity);
    void setConnectivity(QMailServiceAction::Connectivity newConnectivity);

    void setStatus(const QMailServiceAction::Status &status);
    void setStatus(QMailServiceAction::Status::ErrorCode code, const QString &text);
    void setStatus(QMailServiceAction::Status::ErrorCode code, const QString &text, const QMailAccountId &accountId, const QMailFolderId &folderId, const QMailMessageId &messageId);

    void setProgress(uint newProgress, uint newTotal);

    void emitChanges();

    QMailServiceAction *_interface;
    QMailMessageServer *_server;

    QMailServiceAction::Connectivity _connectivity;
    QMailServiceAction::Activity _activity;
    QMailServiceAction::Status _status;

    uint _total;
    uint _progress;

    quint64 _action;

    bool _connectivityChanged;
    bool _activityChanged;
    bool _progressChanged;
    bool _statusChanged;
};


class QMailRetrievalActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailRetrievalActionPrivate(QMailRetrievalAction *);

    void retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);

    void retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(const QMailMessagePart::Location &partLocation);

    void retrieveMessageRange(const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum);

    void retrieveAll(const QMailAccountId &accountId);
    void exportUpdates(const QMailAccountId &accountId);

    void synchronize(const QMailAccountId &accountId);

protected slots:
    void retrievalCompleted(quint64);

private:
    friend class QMailRetrievalAction;
};


class QMailTransmitActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailTransmitActionPrivate(QMailTransmitAction *i);

    void transmitMessages(const QMailAccountId &accountId);

protected:
    virtual void init();

protected slots:
    void messagesTransmitted(quint64, const QMailMessageIdList &id);
    void transmissionCompleted(quint64);

private:
    friend class QMailTransmitAction;

    QMailMessageIdList _ids;
};


class QMailStorageActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailStorageActionPrivate(QMailStorageAction *i);

    void deleteMessages(const QMailMessageIdList &ids);
    void discardMessages(const QMailMessageIdList &ids);

    void copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destination);
    void moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destination);

    void flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask);

protected:
    virtual void init();

protected slots:
    void messagesEffected(quint64, const QMailMessageIdList &id);
    void storageActionCompleted(quint64);

private:
    friend class QMailStorageAction;

    QMailMessageIdList _ids;
};


class QMailSearchActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailSearchActionPrivate(QMailSearchAction *i);

    void searchMessages(const QMailMessageKey &filter, const QString &bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);
    void cancelOperation();

protected:
    virtual void init();

signals:
    void messageIdsMatched(const QMailMessageIdList &ids);

private slots:
    void matchingMessageIds(quint64 action, const QMailMessageIdList &ids);
    void searchCompleted(quint64 action);

    void finaliseSearch();

private:
    friend class QMailSearchAction;

    QMailMessageIdList _matchingIds;
};


class QMailProtocolActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailProtocolActionPrivate(QMailProtocolAction *i);

    void protocolRequest(const QMailAccountId &accountId, const QString &request, const QVariant &data);

signals:
    void protocolResponse(const QString &response, const QVariant &data);

private slots:
    void protocolResponse(quint64 action, const QString &response, const QVariant &data);
    void protocolRequestCompleted(quint64 action);

private:
    friend class QMailProtocolAction;
};

#endif
