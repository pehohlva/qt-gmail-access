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

#include "qmailstoreimplementation_p.h"
#include "qmailipc.h"
#include "qmaillog.h"
#include <QCoreApplication>

namespace {

// Events occurring within this period after a previous event begin batching
const int preFlushTimeout = 250;

// Events occurring within this period are batched
const int flushTimeout = 1000;

const uint pid = static_cast<uint>(QCoreApplication::applicationPid() & 0xffffffff);

typedef QPair<int,int> Segment; //start,end - end non inclusive
typedef QList<Segment> SegmentList;

// Process lists in size-limited batches
SegmentList createSegments(int numItems, int segmentSize)
{
    Q_ASSERT(segmentSize > 0);

    if(numItems <= 0)
        return SegmentList();

    int segmentCount = numItems % segmentSize ? 1 : 0;
    segmentCount += numItems / segmentSize;

    SegmentList segments;
    for(int i = 0; i < segmentCount ; ++i) {
        int start = segmentSize * i;
        int end = (i+1) == segmentCount ? numItems : (i+1) * segmentSize;
        segments.append(Segment(start,end)); 
    }

    return segments;
}

template<typename IDListType>
void emitIpcUpdates(const IDListType& ids, const QString& sig, int max = QMailStoreImplementationBase::maxNotifySegmentSize)
{
    if (!sig.isEmpty()) {
        if (max > 0) {
            SegmentList segmentList = createSegments(ids.count(), max);
            foreach (const Segment& segment, segmentList) {
                IDListType idSegment = ids.mid(segment.first, (segment.second - segment.first));

                QCopAdaptor a("QPE/Qtopiamail");
                QCopAdaptorEnvelope e = a.send(sig.toLatin1());
				e << pid;
                e << idSegment; 
            }
        } else {

            QCopAdaptor a("QPE/Qtopiamail");
            QCopAdaptorEnvelope e = a.send(sig.toLatin1());
            e << pid;
            e << ids;
        }
    } else {
        qWarning() << "No signature for IPC updates!";
    }
}

template<typename IDSetType>
void dispatchNotifications(IDSetType &ids, const QString &sig)
{
    if (!ids.isEmpty()) {
        emitIpcUpdates(ids.toList(), sig);
        ids.clear();
    }
} 

} 


QMailStore::InitializationState QMailStoreImplementationBase::initState = QMailStore::Uninitialized;

QMailStoreImplementationBase::QMailStoreImplementationBase(QMailStore* parent)
    : QObject(parent),
      q(parent),
      errorCode(QMailStore::NoError),
      asyncEmission(false),
      retrievalSetInitialized(false),
      transmissionSetInitialized(false)
{
    Q_ASSERT(q);

    QCopChannel* ipcChannel = new QCopChannel("QPE/Qtopiamail");

    connect(ipcChannel,
            SIGNAL(received(QString,QByteArray)),
            this,
            SLOT(ipcMessage(QString,QByteArray)));

    preFlushTimer.setSingleShot(true);

    flushTimer.setSingleShot(true);
    connect(&flushTimer,
            SIGNAL(timeout()),
            this,
            SLOT(flushNotifications()));
    connect(&queueTimer,
            SIGNAL(timeout()),
            this,
            SLOT(processIpcMessageQueue()));

    connect(qApp,
            SIGNAL(aboutToQuit()),
            this,
            SLOT(aboutToQuit()));
}

QMailStoreImplementationBase::~QMailStoreImplementationBase()
{
}

void QMailStoreImplementationBase::initialize()
{
    initState = (initStore() ? QMailStore::Initialized : QMailStore::InitializationFailed);
}

QMailStore::InitializationState QMailStoreImplementationBase::initializationState()
{
    return initState;
}

QMailStore::ErrorCode QMailStoreImplementationBase::lastError() const
{
    return errorCode;
}

void QMailStoreImplementationBase::setLastError(QMailStore::ErrorCode code) const
{
    if (initState == QMailStore::InitializationFailed) {
        // Enforce the error code to be this if we can't init:
        code = QMailStore::StorageInaccessible;
    }

    if (errorCode != code) {
        errorCode = code;

        if (errorCode != QMailStore::NoError) {
            q->emitErrorNotification(errorCode);
        }
    }
}

bool QMailStoreImplementationBase::asynchronousEmission() const
{
    return asyncEmission;
}

void QMailStoreImplementationBase::flushIpcNotifications()
{
    // We need to emit all pending IPC notifications
    flushNotifications();

    // Tell the recipients to process the notifications synchronously
    QCopAdaptor a("QPE/Qtopiamail");
    QCopAdaptorEnvelope e = a.send("forceIpcFlush");
    e << pid;

    if (flushTimer.isActive()) {
        // We interrupted a batching period - reset the flush timer to its full period
        flushTimer.start(flushTimeout);
    }
}

void QMailStoreImplementationBase::processIpcMessageQueue()
{
    if (messageQueue.isEmpty()) {
        queueTimer.stop();
        return;
    }

   if (emitIpcNotification())
        queueTimer.start(0);
}

void QMailStoreImplementationBase::aboutToQuit()
{
    // Ensure that any pending updates are flushed
    flushNotifications();
}

typedef QMap<QMailStore::ChangeType, QString> NotifyFunctionMap;

static NotifyFunctionMap initAccountFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = QMailStoreImplementationBase::accountAddedSig();
    sig[QMailStore::Updated] = QMailStoreImplementationBase::accountUpdatedSig();
    sig[QMailStore::Removed] = QMailStoreImplementationBase::accountRemovedSig();
    sig[QMailStore::ContentsModified] = QMailStoreImplementationBase::accountContentsModifiedSig();
    return sig;
}

static NotifyFunctionMap initMessageFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = QMailStoreImplementationBase::messageAddedSig();
    sig[QMailStore::Updated] = QMailStoreImplementationBase::messageUpdatedSig();
    sig[QMailStore::Removed] = QMailStoreImplementationBase::messageRemovedSig();
    sig[QMailStore::ContentsModified] = QMailStoreImplementationBase::messageContentsModifiedSig();
    return sig;
}

static NotifyFunctionMap initFolderFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = QMailStoreImplementationBase::folderAddedSig();
    sig[QMailStore::Updated] = QMailStoreImplementationBase::folderUpdatedSig();
    sig[QMailStore::Removed] = QMailStoreImplementationBase::folderRemovedSig();
    sig[QMailStore::ContentsModified] = QMailStoreImplementationBase::folderContentsModifiedSig();
    return sig;
}

static NotifyFunctionMap initMessageRemovalRecordFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = QMailStoreImplementationBase::messageRemovalRecordsAddedSig();
    sig[QMailStore::Removed] = QMailStoreImplementationBase::messageRemovalRecordsRemovedSig();
    return sig;
}

void QMailStoreImplementationBase::notifyAccountsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids)
{
    static NotifyFunctionMap sig(initAccountFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        QSet<QMailAccountId> idsSet = QSet<QMailAccountId>::fromList(ids);
        switch (changeType)
        {
        case QMailStore::Added:
            addAccountsBuffer += idsSet;
            break;
        case QMailStore::Removed:
            removeAccountsBuffer += idsSet;
            break;
        case QMailStore::Updated:
            updateAccountsBuffer += idsSet;
            break;
        case QMailStore::ContentsModified:
            accountContentsModifiedBuffer += idsSet;
            break;
        default:
            qMailLog(Messaging) << "Unhandled account notification received";
            break;
        }
    } else {
        emitIpcUpdates(ids, sig[changeType]);
        
        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreImplementationBase::notifyMessagesChange(QMailStore::ChangeType changeType, const QMailMessageIdList& ids)
{
    static NotifyFunctionMap sig(initMessageFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        QSet<QMailMessageId> idsSet = QSet<QMailMessageId>::fromList(ids);
        switch (changeType)
        {
        case QMailStore::Added:
            addMessagesBuffer += idsSet;
            break;
        case QMailStore::Removed:
            removeMessagesBuffer += idsSet;
            break;
        case QMailStore::Updated:
            updateMessagesBuffer += idsSet;
            break;
        case QMailStore::ContentsModified:
            messageContentsModifiedBuffer += idsSet;
            break;
        default:
            qMailLog(Messaging) << "Unhandled message notification received";
            break;
        }
    } else {
        emitIpcUpdates(ids, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreImplementationBase::notifyFoldersChange(QMailStore::ChangeType changeType, const QMailFolderIdList& ids)
{
    static NotifyFunctionMap sig(initFolderFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        QSet<QMailFolderId> idsSet = QSet<QMailFolderId>::fromList(ids);
        switch (changeType)
        {
        case QMailStore::Added:
            addFoldersBuffer += idsSet;
            break;
        case QMailStore::Removed:
            removeFoldersBuffer += idsSet;
            break;
        case QMailStore::Updated:
            updateFoldersBuffer += idsSet;
            break;
        case QMailStore::ContentsModified:
            folderContentsModifiedBuffer += idsSet;
            break;
        default:
            qMailLog(Messaging) << "Unhandled folder notification received";
            break;
        }
    } else {
        emitIpcUpdates(ids, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreImplementationBase::notifyMessageRemovalRecordsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids)
{
    static NotifyFunctionMap sig(initMessageRemovalRecordFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        QSet<QMailAccountId> idsSet = QSet<QMailAccountId>::fromList(ids);
        switch (changeType)
        {
        case QMailStore::Added:
            addMessageRemovalRecordsBuffer += idsSet;
            break;
        case QMailStore::Removed:
            removeMessageRemovalRecordsBuffer += idsSet;
            break;
        default:
            qMailLog(Messaging) << "Unhandled message removal record notification received";
            break;
        }
    } else {
        emitIpcUpdates(ids, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreImplementationBase::notifyRetrievalInProgress(const QMailAccountIdList& ids)
{
    // Clients may want to enable or disable event handling based on this event, therefore
    // we must ensure that all previous events are actually delivered before this one is.
    flushIpcNotifications();

    emitIpcUpdates(ids, retrievalInProgressSig());
}

void QMailStoreImplementationBase::notifyTransmissionInProgress(const QMailAccountIdList& ids)
{
    flushIpcNotifications();

    emitIpcUpdates(ids, transmissionInProgressSig());
}

bool QMailStoreImplementationBase::setRetrievalInProgress(const QMailAccountIdList& ids)
{
    QSet<QMailAccountId> idSet(ids.toSet());
    if ((idSet != retrievalInProgressIds) || !retrievalSetInitialized) {
        retrievalInProgressIds = idSet;
        retrievalSetInitialized = true;
        return true;
    }

    return false;
}

bool QMailStoreImplementationBase::setTransmissionInProgress(const QMailAccountIdList& ids)
{
    QSet<QMailAccountId> idSet(ids.toSet());
    if ((idSet != transmissionInProgressIds) || !transmissionSetInitialized) {
        transmissionInProgressIds = idSet;
        transmissionSetInitialized = true;
        return true;
    }

    return false;
}

QString QMailStoreImplementationBase::accountAddedSig()
{
    static QString s("accountAdded(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::accountRemovedSig()
{
    static QString s("accountRemoved(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::accountUpdatedSig()
{
    static QString s("accountUpdated(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::accountContentsModifiedSig()
{
    static QString s("accountContentsModified(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::messageAddedSig()
{
    static QString s("messageAdded(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::messageRemovedSig()
{
    static QString s("messageRemoved(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::messageUpdatedSig()
{
    static QString s("messageUpdated(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::messageContentsModifiedSig()
{
    static QString s("messageContentsModified(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::folderAddedSig()
{
    static QString s("folderAdded(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::folderRemovedSig()
{
    static QString s("folderRemoved(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::folderUpdatedSig()
{
    static QString s("folderUpdated(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::folderContentsModifiedSig()
{
    static QString s("folderContentsModified(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::messageRemovalRecordsAddedSig()
{
    static QString s("messageRemovalRecordsAdded(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::messageRemovalRecordsRemovedSig()
{
    static QString s("messageRemovalRecordsRemoved(uint,QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::retrievalInProgressSig()
{
	// TODO: why no PID in theses messages?
    static QString s("retrievalInProgress(QList<quint64>)");
    return s;
}

QString QMailStoreImplementationBase::transmissionInProgressSig()
{
    static QString s("transmissionInProgress(QList<quint64>)");
    return s;
}

QMailStoreImplementationBase::AccountUpdateSignalMap QMailStoreImplementationBase::initAccountUpdateSignals()
{
    AccountUpdateSignalMap sig;
    sig[QMailStoreImplementationBase::accountAddedSig()] = &QMailStore::accountsAdded;
    sig[QMailStoreImplementationBase::accountUpdatedSig()] = &QMailStore::accountsUpdated;
    sig[QMailStoreImplementationBase::accountRemovedSig()] = &QMailStore::accountsRemoved;
    sig[QMailStoreImplementationBase::accountContentsModifiedSig()] = &QMailStore::accountContentsModified;
    sig[QMailStoreImplementationBase::messageRemovalRecordsAddedSig()] = &QMailStore::messageRemovalRecordsAdded;
    sig[QMailStoreImplementationBase::messageRemovalRecordsRemovedSig()] = &QMailStore::messageRemovalRecordsRemoved;
    return sig;
}

QMailStoreImplementationBase::FolderUpdateSignalMap QMailStoreImplementationBase::initFolderUpdateSignals()
{
    FolderUpdateSignalMap sig;
    sig[QMailStoreImplementationBase::folderAddedSig()] = &QMailStore::foldersAdded;
    sig[QMailStoreImplementationBase::folderUpdatedSig()] = &QMailStore::foldersUpdated;
    sig[QMailStoreImplementationBase::folderRemovedSig()] = &QMailStore::foldersRemoved;
    sig[QMailStoreImplementationBase::folderContentsModifiedSig()] = &QMailStore::folderContentsModified;
    return sig;
}

QMailStoreImplementationBase::MessageUpdateSignalMap QMailStoreImplementationBase::initMessageUpdateSignals()
{
    MessageUpdateSignalMap sig;
    sig[QMailStoreImplementationBase::messageAddedSig()] = &QMailStore::messagesAdded;
    sig[QMailStoreImplementationBase::messageUpdatedSig()] = &QMailStore::messagesUpdated;
    sig[QMailStoreImplementationBase::messageRemovedSig()] = &QMailStore::messagesRemoved;
    sig[QMailStoreImplementationBase::messageContentsModifiedSig()] = &QMailStore::messageContentsModified;
    return sig;
}

void QMailStoreImplementationBase::flushNotifications()
{
    static NotifyFunctionMap sigAccount(initAccountFunctions());
    static NotifyFunctionMap sigFolder(initFolderFunctions());
    static NotifyFunctionMap sigMessage(initMessageFunctions());
    static NotifyFunctionMap sigRemoval(initMessageRemovalRecordFunctions());
 
    // There is no need to emit content modification notifications for items subsequently deleted
    folderContentsModifiedBuffer -= removeFoldersBuffer;
    accountContentsModifiedBuffer -= removeAccountsBuffer;

    // The order of emission is significant:
    dispatchNotifications(addAccountsBuffer, sigAccount[QMailStore::Added]);
    dispatchNotifications(addFoldersBuffer, sigFolder[QMailStore::Added]);
    dispatchNotifications(addMessagesBuffer, sigMessage[QMailStore::Added]);
    dispatchNotifications(addMessageRemovalRecordsBuffer, sigRemoval[QMailStore::Added]);

    dispatchNotifications(messageContentsModifiedBuffer, sigMessage[QMailStore::ContentsModified]);
    dispatchNotifications(updateMessagesBuffer, sigMessage[QMailStore::Updated]);
    dispatchNotifications(updateFoldersBuffer, sigFolder[QMailStore::Updated]);
    dispatchNotifications(updateAccountsBuffer, sigAccount[QMailStore::Updated]);
    
    dispatchNotifications(removeMessageRemovalRecordsBuffer, sigRemoval[QMailStore::Removed]);
    dispatchNotifications(removeMessagesBuffer, sigMessage[QMailStore::Removed]);
    dispatchNotifications(removeFoldersBuffer, sigFolder[QMailStore::Removed]);
    dispatchNotifications(removeAccountsBuffer, sigAccount[QMailStore::Removed]);

    dispatchNotifications(folderContentsModifiedBuffer, sigFolder[QMailStore::ContentsModified]);
    dispatchNotifications(accountContentsModifiedBuffer, sigAccount[QMailStore::ContentsModified]);
}

void QMailStoreImplementationBase::ipcMessage(const QString& message, const QByteArray& data) 
{
    QDataStream ds(data);

    uint origin;
    ds >> origin;

    if (pid == origin) //dont notify ourselves 
        return;

    if (message == "forceIpcFlush") {
        // We have been told to flush any pending ipc notifications
        queueTimer.stop();
        while (emitIpcNotification()) {}
    } else if ((message == retrievalInProgressSig()) || (message == transmissionInProgressSig())) {
        // Emit this message immediately
        QMailAccountIdList ids;
        ds >> ids;

        if (message == retrievalInProgressSig()) {
            emitIpcNotification(&QMailStore::retrievalInProgress, ids);
        } else {
            emitIpcNotification(&QMailStore::transmissionInProgress, ids);
        }
    } else {
        // Queue this message for batched delivery
        messageQueue.append(qMakePair(message, data));
        queueTimer.start(0);
    }
}

bool QMailStoreImplementationBase::emitIpcNotification()
{
    if (messageQueue.isEmpty())
        return false;
    
    const QPair<QString, QByteArray> &notification = messageQueue.first();
    const QString &message = notification.first;
    const QByteArray &data = notification.second;

    QDataStream ds(data);

    uint origin;
    ds >> origin;

    static AccountUpdateSignalMap accountUpdateSignals(initAccountUpdateSignals());
    static FolderUpdateSignalMap folderUpdateSignals(initFolderUpdateSignals());
    static MessageUpdateSignalMap messageUpdateSignals(initMessageUpdateSignals());

    AccountUpdateSignalMap::const_iterator ait;
    FolderUpdateSignalMap::const_iterator fit;
    MessageUpdateSignalMap::const_iterator mit;

    if ((ait = accountUpdateSignals.find(message)) != accountUpdateSignals.end()) {
        QMailAccountIdList ids;
        ds >> ids;

        emitIpcNotification(ait.value(), ids);
    } else if ((fit = folderUpdateSignals.find(message)) != folderUpdateSignals.end()) {
        QMailFolderIdList ids;
        ds >> ids;

        emitIpcNotification(fit.value(), ids);
    } else if ((mit = messageUpdateSignals.find(message)) != messageUpdateSignals.end()) {
        QMailMessageIdList ids;
        ds >> ids;

        emitIpcNotification(mit.value(), ids);
    } else {
        qWarning() << "No update signal for message:" << message;
    }
    
    messageQueue.removeFirst();
    return !messageQueue.isEmpty();
}

void QMailStoreImplementationBase::emitIpcNotification(AccountUpdateSignal signal, const QMailAccountIdList &ids)
{
    asyncEmission = true;
    emit (q->*signal)(ids);
    asyncEmission = false;
}

void QMailStoreImplementationBase::emitIpcNotification(FolderUpdateSignal signal, const QMailFolderIdList &ids)
{
    asyncEmission = true;
    emit (q->*signal)(ids);
    asyncEmission = false;
}

void QMailStoreImplementationBase::emitIpcNotification(MessageUpdateSignal signal, const QMailMessageIdList &ids)
{
    asyncEmission = true;
    emit (q->*signal)(ids);
    asyncEmission = false;
}


QMailStoreImplementation::QMailStoreImplementation(QMailStore* parent)
    : QMailStoreImplementationBase(parent)
{
}


QMailStoreNullImplementation::QMailStoreNullImplementation(QMailStore* parent)
    : QMailStoreImplementation(parent)
{
    setLastError(QMailStore::StorageInaccessible);
}

void QMailStoreNullImplementation::clearContent()
{
}

bool QMailStoreNullImplementation::addAccount(QMailAccount *, QMailAccountConfiguration *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::addFolder(QMailFolder *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::addMessages(const QList<QMailMessage *> &, QMailMessageIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::addMessages(const QList<QMailMessageMetaData *> &, QMailMessageIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::removeAccounts(const QMailAccountKey &, QMailAccountIdList *, QMailFolderIdList *, QMailMessageIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::removeFolders(const QMailFolderKey &, QMailStore::MessageRemovalOption, QMailFolderIdList *, QMailMessageIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::removeMessages(const QMailMessageKey &, QMailStore::MessageRemovalOption, QMailMessageIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateAccount(QMailAccount *, QMailAccountConfiguration *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateAccountConfiguration(QMailAccountConfiguration *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateFolder(QMailFolder *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateMessages(const QList<QPair<QMailMessageMetaData *, QMailMessage *> > &, QMailMessageIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateMessagesMetaData(const QMailMessageKey &, const QMailMessageKey::Properties &, const QMailMessageMetaData &, QMailMessageIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateMessagesMetaData(const QMailMessageKey &, quint64, bool, QMailMessageIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::restoreToPreviousFolder(const QMailMessageKey &, QMailMessageIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::purgeMessageRemovalRecords(const QMailAccountId &, const QStringList &)
{
    return false;
}

int QMailStoreNullImplementation::countAccounts(const QMailAccountKey &) const
{
    return 0;
}

int QMailStoreNullImplementation::countFolders(const QMailFolderKey &) const
{
    return 0;
}

int QMailStoreNullImplementation::countMessages(const QMailMessageKey &) const
{
    return 0;
}

int QMailStoreNullImplementation::sizeOfMessages(const QMailMessageKey &) const
{
    return 0;
}

QMailAccountIdList QMailStoreNullImplementation::queryAccounts(const QMailAccountKey &, const QMailAccountSortKey &, uint, uint) const
{
    return QMailAccountIdList();
}

QMailFolderIdList QMailStoreNullImplementation::queryFolders(const QMailFolderKey &, const QMailFolderSortKey &, uint, uint) const
{
    return QMailFolderIdList();
}

QMailMessageIdList QMailStoreNullImplementation::queryMessages(const QMailMessageKey &, const QMailMessageSortKey &, uint, uint) const
{
    return QMailMessageIdList();
}

QMailAccount QMailStoreNullImplementation::account(const QMailAccountId &) const
{
    return QMailAccount();
}

QMailAccountConfiguration QMailStoreNullImplementation::accountConfiguration(const QMailAccountId &) const
{
    return QMailAccountConfiguration();
}

QMailFolder QMailStoreNullImplementation::folder(const QMailFolderId &) const
{
    return QMailFolder();
}

QMailMessage QMailStoreNullImplementation::message(const QMailMessageId &) const
{
    return QMailMessage();
}

QMailMessage QMailStoreNullImplementation::message(const QString &, const QMailAccountId &) const
{
    return QMailMessage();
}

QMailMessageMetaData QMailStoreNullImplementation::messageMetaData(const QMailMessageId &) const
{
    return QMailMessageMetaData();
}

QMailMessageMetaData QMailStoreNullImplementation::messageMetaData(const QString &, const QMailAccountId &) const
{
    return QMailMessageMetaData();
}

QMailMessageMetaDataList QMailStoreNullImplementation::messagesMetaData(const QMailMessageKey &, const QMailMessageKey::Properties &, QMailStore::ReturnOption) const
{
    return QMailMessageMetaDataList();
}

QMailMessageRemovalRecordList QMailStoreNullImplementation::messageRemovalRecords(const QMailAccountId &, const QMailFolderId &) const
{
    return QMailMessageRemovalRecordList();
}

bool QMailStoreNullImplementation::registerAccountStatusFlag(const QString &)
{
    return false;
}

quint64 QMailStoreNullImplementation::accountStatusMask(const QString &) const
{
    return 0;
}

bool QMailStoreNullImplementation::registerFolderStatusFlag(const QString &)
{
    return false;
}

quint64 QMailStoreNullImplementation::folderStatusMask(const QString &) const
{
    return 0;
}

bool QMailStoreNullImplementation::registerMessageStatusFlag(const QString &)
{
    return false;
}

quint64 QMailStoreNullImplementation::messageStatusMask(const QString &) const
{
    return 0;
}

bool QMailStoreNullImplementation::initStore()
{
    return false;
}

