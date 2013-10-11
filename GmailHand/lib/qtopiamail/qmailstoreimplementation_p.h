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

#ifndef QMAILSTOREIMPLEMENTATION_P_H
#define QMAILSTOREIMPLEMENTATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmailstore.h"

#include <QList>
#include <QPair>
#include <QString>
#include <QTimer>


class QMailStoreImplementationBase : public QObject
{
    Q_OBJECT

public:
    QMailStoreImplementationBase(QMailStore* parent);
    virtual ~QMailStoreImplementationBase();

    void initialize();
    static QMailStore::InitializationState initializationState();

    QMailStore::ErrorCode lastError() const;
    void setLastError(QMailStore::ErrorCode code) const;

    bool asynchronousEmission() const;

    void flushIpcNotifications();

    void notifyAccountsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids);
    void notifyMessagesChange(QMailStore::ChangeType changeType, const QMailMessageIdList& ids);
    void notifyFoldersChange(QMailStore::ChangeType changeType, const QMailFolderIdList& ids);
    void notifyMessageRemovalRecordsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids);
    void notifyRetrievalInProgress(const QMailAccountIdList& ids);
    void notifyTransmissionInProgress(const QMailAccountIdList& ids);

    bool setRetrievalInProgress(const QMailAccountIdList &ids);
    bool setTransmissionInProgress(const QMailAccountIdList &ids);

    static QString accountAddedSig();
    static QString accountRemovedSig();
    static QString accountUpdatedSig();
    static QString accountContentsModifiedSig();

    static QString messageAddedSig();
    static QString messageRemovedSig();
    static QString messageUpdatedSig();
    static QString messageContentsModifiedSig();

    static QString folderAddedSig();
    static QString folderUpdatedSig();
    static QString folderRemovedSig();
    static QString folderContentsModifiedSig();

    static QString messageRemovalRecordsAddedSig();
    static QString messageRemovalRecordsRemovedSig();

    static QString retrievalInProgressSig();
    static QString transmissionInProgressSig();

    static const int maxNotifySegmentSize = 0;

public slots:
    void processIpcMessageQueue();
    void ipcMessage(const QString& message, const QByteArray& data);
    void flushNotifications();
    void aboutToQuit();

protected:
    typedef void (QMailStore::*AccountUpdateSignal)(const QMailAccountIdList&);
    typedef QMap<QString, AccountUpdateSignal> AccountUpdateSignalMap;
    static AccountUpdateSignalMap initAccountUpdateSignals();

    typedef void (QMailStore::*FolderUpdateSignal)(const QMailFolderIdList&);
    typedef QMap<QString, FolderUpdateSignal> FolderUpdateSignalMap;
    static FolderUpdateSignalMap initFolderUpdateSignals();

    typedef void (QMailStore::*MessageUpdateSignal)(const QMailMessageIdList&);
    typedef QMap<QString, MessageUpdateSignal> MessageUpdateSignalMap;
    static MessageUpdateSignalMap initMessageUpdateSignals();

    static QMailStore::InitializationState initState;
    
    virtual void emitIpcNotification(AccountUpdateSignal signal, const QMailAccountIdList &ids);
    virtual void emitIpcNotification(FolderUpdateSignal signal, const QMailFolderIdList &ids);
    virtual void emitIpcNotification(MessageUpdateSignal signal, const QMailMessageIdList &ids);

private:
    virtual bool initStore() = 0;

    bool emitIpcNotification();

    QMailStore* q;
    
    mutable QMailStore::ErrorCode errorCode;

    bool asyncEmission;

    QTimer preFlushTimer;
    QTimer flushTimer;

    QSet<QMailAccountId> addAccountsBuffer;
    QSet<QMailFolderId> addFoldersBuffer;
    QSet<QMailMessageId> addMessagesBuffer;
    QSet<QMailAccountId> addMessageRemovalRecordsBuffer;

    QSet<QMailMessageId> updateMessagesBuffer;
    QSet<QMailFolderId> updateFoldersBuffer;
    QSet<QMailAccountId> updateAccountsBuffer;

    QSet<QMailAccountId> removeMessageRemovalRecordsBuffer;
    QSet<QMailMessageId> removeMessagesBuffer;
    QSet<QMailFolderId> removeFoldersBuffer;
    QSet<QMailAccountId> removeAccountsBuffer;

    QSet<QMailFolderId> folderContentsModifiedBuffer;
    QSet<QMailAccountId> accountContentsModifiedBuffer;
    QSet<QMailMessageId> messageContentsModifiedBuffer;

    bool retrievalSetInitialized;
    bool transmissionSetInitialized;

    QSet<QMailAccountId> retrievalInProgressIds;
    QSet<QMailAccountId> transmissionInProgressIds;

    QTimer queueTimer;
    QList<QPair<QString, QByteArray> > messageQueue;
};


class QMailStoreImplementation : public QMailStoreImplementationBase
{
public:
    QMailStoreImplementation(QMailStore* parent);

    virtual void clearContent() = 0;

    virtual bool addAccount(QMailAccount *account, QMailAccountConfiguration *config,
                            QMailAccountIdList *addedAccountIds) = 0;

    virtual bool addFolder(QMailFolder *f,
                           QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool addMessages(const QList<QMailMessage *> &m,
                             QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool addMessages(const QList<QMailMessageMetaData *> &m,
                             QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool removeAccounts(const QMailAccountKey &key,
                                QMailAccountIdList *deletedAccounts, QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                               QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                                QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool updateAccount(QMailAccount *account, QMailAccountConfiguration* config,
                               QMailAccountIdList *updatedAccountIds) = 0;

    virtual bool updateAccountConfiguration(QMailAccountConfiguration* config,
                                            QMailAccountIdList *updatedAccountIds) = 0;

    virtual bool updateFolder(QMailFolder* f,
                              QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool updateMessages(const QList<QPair<QMailMessageMetaData *, QMailMessage *> > &m,
                                QMailMessageIdList *updatedMessageIds, QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data,
                                        QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, quint64 messageStatus, bool set,
                                        QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool restoreToPreviousFolder(const QMailMessageKey &key,
                                         QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds) = 0;

    virtual bool purgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids) = 0;

    virtual int countAccounts(const QMailAccountKey &key) const = 0;
    virtual int countFolders(const QMailFolderKey &key) const = 0;
    virtual int countMessages(const QMailMessageKey &key) const = 0;

    virtual int sizeOfMessages(const QMailMessageKey &key) const = 0;

    virtual QMailAccountIdList queryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey, uint limit, uint offset) const = 0;
    virtual QMailFolderIdList queryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey, uint limit, uint offset) const = 0;
    virtual QMailMessageIdList queryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey, uint limit, uint offset) const = 0;

    virtual QMailAccount account(const QMailAccountId &id) const = 0;
    virtual QMailAccountConfiguration accountConfiguration(const QMailAccountId &id) const = 0;

    virtual QMailFolder folder(const QMailFolderId &id) const = 0;

    virtual QMailMessage message(const QMailMessageId &id) const = 0;
    virtual QMailMessage message(const QString &uid, const QMailAccountId &accountId) const = 0;

    virtual QMailMessageMetaData messageMetaData(const QMailMessageId &id) const = 0;
    virtual QMailMessageMetaData messageMetaData(const QString &uid, const QMailAccountId &accountId) const = 0;
    virtual QMailMessageMetaDataList messagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option) const = 0;

    virtual QMailMessageRemovalRecordList messageRemovalRecords(const QMailAccountId &parentAccountId, const QMailFolderId &parentFolderId) const = 0;

    virtual bool registerAccountStatusFlag(const QString &name) = 0;
    virtual quint64 accountStatusMask(const QString &name) const = 0;

    virtual bool registerFolderStatusFlag(const QString &name) = 0;
    virtual quint64 folderStatusMask(const QString &name) const = 0;

    virtual bool registerMessageStatusFlag(const QString &name) = 0;
    virtual quint64 messageStatusMask(const QString &name) const = 0;
};

class QMailStoreNullImplementation : public QMailStoreImplementation
{
public:
    QMailStoreNullImplementation(QMailStore* parent);

    virtual void clearContent();

    virtual bool addAccount(QMailAccount *account, QMailAccountConfiguration *config,
                            QMailAccountIdList *addedAccountIds);

    virtual bool addFolder(QMailFolder *f,
                           QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool addMessages(const QList<QMailMessage *> &m,
                             QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool addMessages(const QList<QMailMessageMetaData *> &m,
                             QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool removeAccounts(const QMailAccountKey &key,
                                QMailAccountIdList *deletedAccounts, QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                               QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                                QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessages, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateAccount(QMailAccount *account, QMailAccountConfiguration* config,
                               QMailAccountIdList *updatedAccountIds);

    virtual bool updateAccountConfiguration(QMailAccountConfiguration* config,
                                            QMailAccountIdList *updatedAccountIds);

    virtual bool updateFolder(QMailFolder* f,
                              QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateMessages(const QList<QPair<QMailMessageMetaData *, QMailMessage *> > &m,
                                QMailMessageIdList *updatedMessageIds, QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data,
                                        QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool updateMessagesMetaData(const QMailMessageKey &key, quint64 messageStatus, bool set,
                                        QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool restoreToPreviousFolder(const QMailMessageKey &key,
                                         QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    virtual bool purgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids);

    virtual int countAccounts(const QMailAccountKey &key) const;
    virtual int countFolders(const QMailFolderKey &key) const;
    virtual int countMessages(const QMailMessageKey &key) const;

    virtual int sizeOfMessages(const QMailMessageKey &key) const;

    virtual QMailAccountIdList queryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey, uint limit, uint offset) const;
    virtual QMailFolderIdList queryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey, uint limit, uint offset) const;
    virtual QMailMessageIdList queryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey, uint limit, uint offset) const;

    virtual QMailAccount account(const QMailAccountId &id) const;
    virtual QMailAccountConfiguration accountConfiguration(const QMailAccountId &id) const;

    virtual QMailFolder folder(const QMailFolderId &id) const;

    virtual QMailMessage message(const QMailMessageId &id) const;
    virtual QMailMessage message(const QString &uid, const QMailAccountId &accountId) const;

    virtual QMailMessageMetaData messageMetaData(const QMailMessageId &id) const;
    virtual QMailMessageMetaData messageMetaData(const QString &uid, const QMailAccountId &accountId) const;
    virtual QMailMessageMetaDataList messagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option) const;

    virtual QMailMessageRemovalRecordList messageRemovalRecords(const QMailAccountId &parentAccountId, const QMailFolderId &parentFolderId) const;

    virtual bool registerAccountStatusFlag(const QString &name);
    virtual quint64 accountStatusMask(const QString &name) const;

    virtual bool registerFolderStatusFlag(const QString &name);
    virtual quint64 folderStatusMask(const QString &name) const;

    virtual bool registerMessageStatusFlag(const QString &name);
    virtual quint64 messageStatusMask(const QString &name) const;

private:
    virtual bool initStore();
};

#endif

