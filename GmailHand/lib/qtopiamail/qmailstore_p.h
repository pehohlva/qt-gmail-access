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

#ifndef QMAILSTORE_P_H
#define QMAILSTORE_P_H

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

#include "qmailstoreimplementation_p.h"
#include <QSqlDatabase>
#include <QCache>

//#define QMAILSTORE_LOG_SQL //define to enable SQL query logging
//#define QMAILSTORE_USE_RTTI //define if RTTI is available to assist debugging

#ifdef QMAILSTORE_USE_RTTI
#include <typeinfo>
#endif

class ProcessMutex;
class ProcessReadLock;


class QMailStorePrivate : public QMailStoreImplementation
{
    Q_OBJECT

public:
    typedef QMap<QMailMessageKey::Property, QString> MessagePropertyMap;
    typedef QList<QMailMessageKey::Property> MessagePropertyList;

    class Transaction;
    class ReadLock;
    class Key;

    struct ReadAccess {};
    struct WriteAccess {};

    QMailStorePrivate(QMailStore* parent);
    ~QMailStorePrivate();

    virtual bool initStore();
    void clearContent();

    bool addAccount(QMailAccount *account, QMailAccountConfiguration *config,
                    QMailAccountIdList *addedAccountIds);

    bool addFolder(QMailFolder *f,
                   QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool addMessages(const QList<QMailMessage *> &m,
                     QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool addMessages(const QList<QMailMessageMetaData *> &m,
                     QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool removeAccounts(const QMailAccountKey &key,
                        QMailAccountIdList *deletedAccounts, QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool removeFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option,
                       QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool removeMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option,
                        QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool updateAccount(QMailAccount *account, QMailAccountConfiguration* config,
                       QMailAccountIdList *updatedAccountIds);

    bool updateAccountConfiguration(QMailAccountConfiguration* config,
                                    QMailAccountIdList *updatedAccountIds);

    bool updateFolder(QMailFolder* f,
                      QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool updateMessages(const QList<QPair<QMailMessageMetaData *, QMailMessage *> > &m,
                        QMailMessageIdList *updatedMessageIds, QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool updateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, const QMailMessageMetaData &data,
                                QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool updateMessagesMetaData(const QMailMessageKey &key, quint64 messageStatus, bool set,
                                QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool restoreToPreviousFolder(const QMailMessageKey &key,
                                 QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds);

    bool purgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids);

    int countAccounts(const QMailAccountKey &key) const;
    int countFolders(const QMailFolderKey &key) const;
    int countMessages(const QMailMessageKey &key) const;

    int sizeOfMessages(const QMailMessageKey &key) const;

    QMailAccountIdList queryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey, uint limit, uint offset) const;
    QMailFolderIdList queryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey, uint limit, uint offset) const;
    QMailMessageIdList queryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey, uint limit, uint offset) const;

    QMailAccount account(const QMailAccountId &id) const;
    QMailAccountConfiguration accountConfiguration(const QMailAccountId &id) const;

    QMailFolder folder(const QMailFolderId &id) const;

    QMailMessage message(const QMailMessageId &id) const;
    QMailMessage message(const QString &uid, const QMailAccountId &accountId) const;

    QMailMessageMetaData messageMetaData(const QMailMessageId &id) const;
    QMailMessageMetaData messageMetaData(const QString &uid, const QMailAccountId &accountId) const;
    QMailMessageMetaDataList messagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option) const;

    QMailMessageRemovalRecordList messageRemovalRecords(const QMailAccountId &parentAccountId, const QMailFolderId &parentFolderId) const;

    bool registerAccountStatusFlag(const QString &name);
    quint64 accountStatusMask(const QString &name) const;

    bool registerFolderStatusFlag(const QString &name);
    quint64 folderStatusMask(const QString &name) const;

    bool registerMessageStatusFlag(const QString &name);
    quint64 messageStatusMask(const QString &name) const;

    QString buildOrderClause(const Key& key) const;

    QString buildWhereClause(const Key& key, bool nested = false, bool firstClause = true) const;
    QVariantList whereClauseValues(const Key& key) const;

    static QString expandValueList(const QVariantList& valueList);
    static QString expandValueList(int valueCount);

    static QString temporaryTableName(const QMailMessageKey::ArgumentType &arg);

    template<typename ValueType>
    static ValueType extractValue(const QVariant& var, const ValueType &defaultValue = ValueType());

    enum AttemptResult { Success = 0, Failure, DatabaseFailure };
    
private:
    friend class Transaction;
    friend class ReadLock;

    static ProcessMutex& contentManagerMutex(void);

    ProcessMutex& databaseMutex(void) const;
    ProcessReadLock& databaseReadLock(void) const;

    static const MessagePropertyMap& messagePropertyMap();
    static const MessagePropertyList& messagePropertyList();

    static const QMailMessageKey::Properties &updatableMessageProperties();
    static const QMailMessageKey::Properties &allMessageProperties();

    QString expandProperties(const QMailMessageKey::Properties& p, bool update = false) const;

    QString databaseIdentifier() const;

    bool ensureVersionInfo();
    qint64 tableVersion(const QString &name) const;
    bool setTableVersion(const QString &name, qint64 version);

    qint64 incrementTableVersion(const QString &name, qint64 current);
    bool upgradeTableVersion(const QString &name, qint64 current, qint64 final);

    bool createTable(const QString &name);

    typedef QPair<QString, qint64> TableInfo;
    bool setupTables(const QList<TableInfo> &tableList);

    typedef QPair<quint64, QString> FolderInfo;
    bool setupFolders(const QList<FolderInfo> &folderList);

    bool purgeMissingAncestors();
    bool purgeObsoleteFiles();

    bool performMaintenanceTask(const QString &task, uint secondsFrequency, bool (QMailStorePrivate::*func)(void));

    bool performMaintenance();

    void createTemporaryTable(const QMailMessageKey::ArgumentType &arg, const QString &dataType) const;
    void destroyTemporaryTables(void);

    bool transaction(void);
    bool commit(void);
    void rollback(void);

    void setQueryError(const QSqlError&, const QString& description = QString(), const QString& statement = QString());
    void clearQueryError(void);

    QSqlQuery prepare(const QString& sql);
    bool execute(QSqlQuery& q, bool batch = false);
    int queryError(void) const;

    QSqlQuery performQuery(const QString& statement, bool batch, const QVariantList& bindValues, const QList<Key>& keys, const QPair<uint, uint> &constraint, const QString& descriptor);

    bool executeFile(QFile &file);

    QSqlQuery simpleQuery(const QString& statement, const QString& descriptor);
    QSqlQuery simpleQuery(const QString& statement, const QVariantList& bindValues, const QString& descriptor);

    QSqlQuery simpleQuery(const QString& statement, const Key& key, const QString& descriptor);
    QSqlQuery simpleQuery(const QString& statement, const QVariantList& bindValues, const Key& key, const QString& descriptor);
    QSqlQuery simpleQuery(const QString& statement, const QVariantList& bindValues, const QList<Key>& keys, const QString& descriptor);
    QSqlQuery simpleQuery(const QString& statement, const QVariantList& bindValues, const QList<Key>& keys, const QPair<uint, uint> &constraint, const QString& descriptor);

    QSqlQuery batchQuery(const QString& statement, const QVariantList& bindValues, const QString& descriptor);
    QSqlQuery batchQuery(const QString& statement, const QVariantList& bindValues, const Key& key, const QString& descriptor);
    QSqlQuery batchQuery(const QString& statement, const QVariantList& bindValues, const QList<Key>& keys, const QString& descriptor);

    bool idValueExists(quint64 id, const QString& table);

    bool idExists(const QMailAccountId& id, const QString& table = QString());
    bool idExists(const QMailFolderId& id, const QString& table = QString());
    bool idExists(const QMailMessageId& id, const QString& table = QString());

    bool checkPreconditions(const QMailFolder& folder, bool update = false);

    void preloadHeaderCache(const QMailMessageId& id) const;

    QMailFolderIdList folderAncestorIds(const QMailFolderIdList& ids, bool inTransaction, AttemptResult *result) const;

    quint64 queryStatusMap(const QString &name, const QString &context, QMap<QString, quint64> &map) const;

    bool deleteMessages(const QMailMessageKey& key,
                        QMailStore::MessageRemovalOption option,
                        QMailMessageIdList& deletedMessageIds,
                        QStringList& expiredMailfiles,
                        QMailMessageIdList& updatedMessageIds,
                        QMailFolderIdList& modifiedFolders,
                        QMailAccountIdList& modifiedAccounts);

    bool deleteFolders(const QMailFolderKey& key,
                       QMailStore::MessageRemovalOption option,
                       QMailFolderIdList& deletedFolderIds,
                       QMailMessageIdList& deletedMessageIds,
                       QStringList& expiredMailfiles,
                       QMailMessageIdList& updatedMessageIds,
                       QMailFolderIdList& modifiedFolderIds,
                       QMailAccountIdList& modifiedAccountIds);

    bool deleteAccounts(const QMailAccountKey& key,
                        QMailAccountIdList& deletedAccountIds,
                        QMailFolderIdList& deletedFolderIds,
                        QMailMessageIdList& deletedMessageIds,
                        QStringList& expiredMailfiles,
                        QMailMessageIdList& updatedMessageIds,
                        QMailFolderIdList& modifiedFolderIds,
                        QMailAccountIdList& modifiedAccountIds);

    void removeExpiredData(const QMailMessageIdList& messageIds,
                           const QStringList& mailfiles,
                           const QMailFolderIdList& folderIds = QMailFolderIdList(),
                           const QMailAccountIdList& accountIds = QMailAccountIdList());

    bool obsoleteContent(const QString& identifier);

    template<typename AccessType, typename FunctionType>
    bool repeatedly(FunctionType func, const QString &description, Transaction *t = 0) const;

    AttemptResult addCustomFields(quint64 id, const QMap<QString, QString> &fields, const QString &tableName);
    AttemptResult updateCustomFields(quint64 id, const QMap<QString, QString> &fields, const QString &tableName);
    AttemptResult customFields(quint64 id, QMap<QString, QString> *fields, const QString &tableName);

    AttemptResult attemptAddAccount(QMailAccount *account, QMailAccountConfiguration* config, 
                                    QMailAccountIdList *addedAccountIds, 
                                    Transaction &t, bool commitOnSuccess);

    AttemptResult attemptAddFolder(QMailFolder *folder, 
                                   QMailFolderIdList *addedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                   Transaction &t, bool commitOnSuccess);

    AttemptResult attemptAddMessage(QMailMessage *message, const QString &identifier, const QStringList &references,
                                    QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, 
                                    Transaction &t, bool commitOnSuccess);

    AttemptResult attemptAddMessage(QMailMessageMetaData *metaData, const QString &identifier, const QStringList &references,
                                    QMailMessageIdList *addedMessageIds, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, 
                                    Transaction &t, bool commitOnSuccess);

    AttemptResult attemptRemoveAccounts(const QMailAccountKey &key, 
                                        QMailAccountIdList *deletedAccounts, QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                        Transaction &t, bool commitOnSuccess);

    AttemptResult attemptRemoveFolders(const QMailFolderKey &key, QMailStore::MessageRemovalOption option, 
                                       QMailFolderIdList *deletedFolders, QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                       Transaction &t, bool commitOnSuccess);

    AttemptResult attemptRemoveMessages(const QMailMessageKey &key, QMailStore::MessageRemovalOption option, 
                                        QMailMessageIdList *deletedMessages, QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                        Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateAccount(QMailAccount *account, QMailAccountConfiguration *config, 
                                       QMailAccountIdList *updatedAccountIds,
                                       Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateAccountConfiguration(QMailAccountConfiguration *config, 
                                                    QMailAccountIdList *updatedAccountIds,
                                                    Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateFolder(QMailFolder *folder, 
                                      QMailFolderIdList *updatedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                      Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateMessage(QMailMessageMetaData *metaData, QMailMessage *mail, 
                                       QMailMessageIdList *updatedMessageIds, QMailMessageIdList *modifiedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                       Transaction &t, bool commitOnSuccess);

    AttemptResult attemptUpdateMessagesMetaData(const QMailMessageKey &key, const QMailMessageKey::Properties &props, const QMailMessageMetaData &data, 
                                                QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds,
                                                Transaction &t, bool commitOnSuccess); 

    AttemptResult attemptUpdateMessagesStatus(const QMailMessageKey &key, quint64 status, bool set, 
                                              QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, 
                                              Transaction &t, bool commitOnSuccess);

    AttemptResult attemptRestoreToPreviousFolder(const QMailMessageKey &key, 
                                                 QMailMessageIdList *updatedMessageIds, QMailFolderIdList *modifiedFolderIds, QMailAccountIdList *modifiedAccountIds, 
                                                 Transaction &t, bool commitOnSuccess);

    AttemptResult attemptPurgeMessageRemovalRecords(const QMailAccountId &accountId, const QStringList &serverUids,
                                                    Transaction &t, bool commitOnSuccess);

    AttemptResult attemptCountAccounts(const QMailAccountKey &key, int *result, 
                                       ReadLock &);

    AttemptResult attemptCountFolders(const QMailFolderKey &key, int *result, 
                                      ReadLock &);

    AttemptResult attemptCountMessages(const QMailMessageKey &key, 
                                       int *result, 
                                       ReadLock &);

    AttemptResult attemptSizeOfMessages(const QMailMessageKey &key, 
                                        int *result, 
                                        ReadLock &);

    AttemptResult attemptQueryAccounts(const QMailAccountKey &key, const QMailAccountSortKey &sortKey, uint limit, uint offset,
                                       QMailAccountIdList *ids, 
                                       ReadLock &);

    AttemptResult attemptQueryFolders(const QMailFolderKey &key, const QMailFolderSortKey &sortKey, uint limit, uint offset,
                                      QMailFolderIdList *ids, 
                                      ReadLock &);

    AttemptResult attemptQueryMessages(const QMailMessageKey &key, const QMailMessageSortKey &sortKey, uint limit, uint offset,
                                       QMailMessageIdList *ids, 
                                       ReadLock &);

    AttemptResult attemptAccount(const QMailAccountId &id, 
                                 QMailAccount *result, 
                                 ReadLock &);

    AttemptResult attemptAccountConfiguration(const QMailAccountId &id, 
                                              QMailAccountConfiguration *result, 
                                              ReadLock &);

    AttemptResult attemptFolder(const QMailFolderId &id, 
                                QMailFolder *result, 
                                ReadLock &);

    AttemptResult attemptMessage(const QMailMessageId &id, 
                                 QMailMessage *result, 
                                 ReadLock &);

    AttemptResult attemptMessage(const QString &uid, const QMailAccountId &accountId, 
                                 QMailMessage *result, 
                                 ReadLock &);

    AttemptResult attemptMessageMetaData(const QMailMessageId &id, 
                                         QMailMessageMetaData *result, 
                                         ReadLock &);

    AttemptResult attemptMessageMetaData(const QString &uid, const QMailAccountId &accountId, 
                                         QMailMessageMetaData *result, 
                                         ReadLock &);

    AttemptResult attemptMessagesMetaData(const QMailMessageKey& key, const QMailMessageKey::Properties &properties, QMailStore::ReturnOption option, 
                                          QMailMessageMetaDataList *result, 
                                          ReadLock &);

    AttemptResult attemptMessageRemovalRecords(const QMailAccountId &accountId, const QMailFolderId &parentFolderId, 
                                               QMailMessageRemovalRecordList *result,
                                               ReadLock &);

    AttemptResult attemptMessageFolderIds(const QMailMessageKey &key, 
                                          QMailFolderIdList *result, 
                                          ReadLock &);

    AttemptResult attemptFolderAccountIds(const QMailFolderKey &key, 
                                          QMailAccountIdList *result, 
                                          ReadLock &);

    AttemptResult attemptFolderAncestorIds(const QMailFolderIdList &ids, 
                                           QMailFolderIdList *result, 
                                           ReadLock &);

    AttemptResult attemptStatusBit(const QString &name, const QString &context, 
                                   int *result, 
                                   ReadLock &);

    AttemptResult attemptRegisterStatusBit(const QString &name, const QString &context, int maximum, 
                                           Transaction &t, bool commitOnSuccess);

    AttemptResult attemptMessageId(const QString &uid, const QMailAccountId &accountId, 
                                   quint64 *result, 
                                   ReadLock &);

    AttemptResult affectedByMessageIds(const QMailMessageIdList &messages, QMailFolderIdList *folderIds, QMailAccountIdList *accountIds) const;

    AttemptResult affectedByFolderIds(const QMailFolderIdList &folders, QMailFolderIdList *folderIds, QMailAccountIdList *accountIds) const;

    AttemptResult messagePredecessor(QMailMessageMetaData *metaData, const QStringList &references, const QString &baseSubject, bool sameSubject, QStringList *missingReferences, bool *missingAncestor);

    AttemptResult identifyAncestors(const QMailMessageId &predecessorId, const QMailMessageIdList &childIds, QMailMessageIdList *ancestorIds);

    AttemptResult resolveMissingMessages(const QString &identifier, const QMailMessageId &predecessorId, const QString &baseSubject, quint64 messageId, QMailMessageIdList *updatedMessageIds);

    AttemptResult registerSubject(const QString &baseSubject, quint64 messageId, const QMailMessageId &predecessorId, bool missingAncestor);

    QMailAccount extractAccount(const QSqlRecord& r);
    QMailFolder extractFolder(const QSqlRecord& r);
    QMailMessageMetaData extractMessageMetaData(const QSqlRecord& r, QMailMessageKey::Properties recordProperties, const QMailMessageKey::Properties& properties = allMessageProperties());
    QMailMessageMetaData extractMessageMetaData(const QSqlRecord& r, const QMap<QString, QString> &customFields, const QMailMessageKey::Properties& properties = allMessageProperties());
    QMailMessage extractMessage(const QSqlRecord& r, const QMap<QString, QString> &customFields, const QMailMessageKey::Properties& properties = allMessageProperties());
    QMailMessageRemovalRecord extractMessageRemovalRecord(const QSqlRecord& r);

    virtual void emitIpcNotification(QMailStoreImplementation::AccountUpdateSignal signal, const QMailAccountIdList &ids);
    virtual void emitIpcNotification(QMailStoreImplementation::FolderUpdateSignal signal, const QMailFolderIdList &ids);
    virtual void emitIpcNotification(QMailStoreImplementation::MessageUpdateSignal signal, const QMailMessageIdList &ids);

    static const int messageCacheSize = 100;
    static const int uidCacheSize = 500;
    static const int folderCacheSize = 10;
    static const int accountCacheSize = 10;
    static const int lookAhead = 5;

    static QString parseSql(QTextStream& ts);

    static QVariantList messageValues(const QMailMessageKey::Properties& properties, const QMailMessageMetaData& data);
    static void updateMessageValues(const QMailMessageKey::Properties& properties, const QVariantList& values, const QMap<QString, QString>& customFields, QMailMessageMetaData& metaData);

    static const QString &defaultContentScheme();
    static const QString &messagesBodyPath();
    static QString messageFilePath(const QString &fileName);

    static void extractMessageMetaData(const QSqlRecord& r, QMailMessageKey::Properties recordProperties, const QMailMessageKey::Properties& properties, QMailMessageMetaData* metaData);

private:
    template <typename T, typename KeyType> 
    class Cache
    {
    public:
        Cache(unsigned int size = 10);
        ~Cache();

        T lookup(const KeyType& key) const;
        void insert(const KeyType& key, const T& item);
        bool contains(const KeyType& key) const;
        void remove(const KeyType& key);
        void clear();

    private:
        QCache<KeyType,T> mCache;
    };

    template <typename T, typename ID> 
    class IdCache : public Cache<T, quint64>
    {
    public:
        IdCache(unsigned int size = 10) : Cache<T, quint64>(size) {}

        T lookup(const ID& id) const;
        void insert(const T& item);
        bool contains(const ID& id) const;
        void remove(const ID& id);
    };

    mutable QSqlDatabase database;
    
    mutable QMailMessageIdList lastQueryMessageResult;

    mutable IdCache<QMailMessageMetaData, QMailMessageId> messageCache;
    mutable Cache<QMailMessageId, QPair<QMailAccountId, QString> > uidCache;
    mutable IdCache<QMailFolder, QMailFolderId> folderCache;
    mutable IdCache<QMailAccount, QMailAccountId> accountCache;

    mutable QList<QPair<const QMailMessageKey::ArgumentType*, QString> > requiredTableKeys;
    mutable QList<const QMailMessageKey::ArgumentType*> temporaryTableKeys;
    QList<const QMailMessageKey::ArgumentType*> expiredTableKeys;

    bool inTransaction;
    mutable int lastQueryError;

    ProcessMutex *mutex;
    ProcessReadLock *readLock;

    static ProcessMutex *contentMutex;
};

template <typename ValueType>
ValueType QMailStorePrivate::extractValue(const QVariant &var, const ValueType &defaultValue)
{
    if (!qVariantCanConvert<ValueType>(var)) {
        qWarning() << "QMailStorePrivate::extractValue - Cannot convert variant to:"
#ifdef QMAILSTORE_USE_RTTI
                   << typeid(ValueType).name();
#else
                   << "requested type";
#endif
        return defaultValue;
    }

    return qVariantValue<ValueType>(var);
}


template <typename T, typename KeyType> 
QMailStorePrivate::Cache<T, KeyType>::Cache(unsigned int cacheSize)
    : mCache(cacheSize)
{
}

template <typename T, typename KeyType> 
QMailStorePrivate::Cache<T, KeyType>::~Cache()
{
}

template <typename T, typename KeyType> 
T QMailStorePrivate::Cache<T, KeyType>::lookup(const KeyType& key) const
{
    if (T* cachedItem = mCache.object(key))
        return *cachedItem;

    return T();
}

template <typename T, typename KeyType> 
void QMailStorePrivate::Cache<T, KeyType>::insert(const KeyType& key, const T& item)
{
    mCache.insert(key,new T(item));
}

template <typename T, typename KeyType> 
bool QMailStorePrivate::Cache<T, KeyType>::contains(const KeyType& key) const
{
    return mCache.contains(key);
}

template <typename T, typename KeyType> 
void QMailStorePrivate::Cache<T, KeyType>::remove(const KeyType& key)
{
    mCache.remove(key);
}

template <typename T, typename KeyType> 
void QMailStorePrivate::Cache<T, KeyType>::clear()
{
    mCache.clear();
}


template <typename T, typename ID> 
T QMailStorePrivate::IdCache<T, ID>::lookup(const ID& id) const
{
    if (id.isValid()) {
        return Cache<T, quint64>::lookup(id.toULongLong());
    }

    return T();
}

template <typename T, typename ID> 
void QMailStorePrivate::IdCache<T, ID>::insert(const T& item)
{
    if (item.id().isValid()) {
        Cache<T, quint64>::insert(item.id().toULongLong(), item);
    }
}

template <typename T, typename ID> 
bool QMailStorePrivate::IdCache<T, ID>::contains(const ID& id) const
{
    return Cache<T, quint64>::contains(id.toULongLong());
}

template <typename T, typename ID> 
void QMailStorePrivate::IdCache<T, ID>::remove(const ID& id)
{
    Cache<T, quint64>::remove(id.toULongLong());
}

#endif
