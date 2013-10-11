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

#include "qmailmessageserver.h"

static bool connectIpc( QObject *sender, const QByteArray& signal,
        QObject *receiver, const QByteArray& member)
{
    return QCopAdaptor::connect(sender,signal,receiver,member);
}

class QTOPIAMAIL_EXPORT QMailMessageServerPrivate : public QObject
{
    Q_OBJECT

    friend class QMailMessageServer;

public:
    QMailMessageServerPrivate(QMailMessageServer* parent);
    ~QMailMessageServerPrivate();

signals:
    void initialise();

    void transmitMessages(quint64, const QMailAccountId &accountId);

    void retrieveFolderList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);

    void retrieveMessages(quint64, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(quint64, const QMailMessagePart::Location &partLocation);

    void retrieveMessageRange(quint64, const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(quint64, const QMailMessagePart::Location &partLocation, uint minimum);

    void retrieveAll(quint64, const QMailAccountId &accountId);
    void exportUpdates(quint64, const QMailAccountId &accountId);

    void synchronize(quint64, const QMailAccountId &accountId);

    void copyMessages(quint64, const QMailMessageIdList& mailList, const QMailFolderId &destination);
    void moveMessages(quint64, const QMailMessageIdList& mailList, const QMailFolderId &destination);
    void flagMessages(quint64, const QMailMessageIdList& mailList, quint64 setMask, quint64 unsetMask);

    void cancelTransfer(quint64);

    void deleteMessages(quint64, const QMailMessageIdList& id, QMailStore::MessageRemovalOption);

    void searchMessages(quint64, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);

    void cancelSearch(quint64);

    void shutdown();

    void protocolRequest(quint64, const QMailAccountId &accountId, const QString &request, const QVariant &data);

    void acknowledgeNewMessages(const QMailMessageTypeList&);

private:
    QCopAdaptor* adaptor;
};


QMailMessageServerPrivate::QMailMessageServerPrivate(QMailMessageServer* parent)
    : QObject(parent),
      adaptor(new QCopAdaptor("QPE/QMailMessageServer", this))
{
    // Forward signals to the message server
    connectIpc(adaptor, MESSAGE(newCountChanged(QMailMessageCountMap)),
               parent, SIGNAL(newCountChanged(QMailMessageCountMap)));
    connectIpc(this, SIGNAL(acknowledgeNewMessages(QMailMessageTypeList)),
               adaptor, MESSAGE(acknowledgeNewMessages(QMailMessageTypeList)));

    connectIpc(this, SIGNAL(initialise()),
               adaptor, MESSAGE(initialise()));
    connectIpc(this, SIGNAL(transmitMessages(quint64, QMailAccountId)),
               adaptor, MESSAGE(transmitMessages(quint64, QMailAccountId)));
    connectIpc(this, SIGNAL(retrieveFolderList(quint64, QMailAccountId, QMailFolderId, bool)),
               adaptor, MESSAGE(retrieveFolderList(quint64, QMailAccountId, QMailFolderId, bool)));
    connectIpc(this, SIGNAL(retrieveMessageList(quint64, QMailAccountId, QMailFolderId, uint, QMailMessageSortKey)),
               adaptor, MESSAGE(retrieveMessageList(quint64, QMailAccountId, QMailFolderId, uint, QMailMessageSortKey)));
    connectIpc(this, SIGNAL(retrieveMessages(quint64, QMailMessageIdList, QMailRetrievalAction::RetrievalSpecification)),
               adaptor, MESSAGE(retrieveMessages(quint64, QMailMessageIdList, QMailRetrievalAction::RetrievalSpecification)));
    connectIpc(this, SIGNAL(retrieveMessagePart(quint64, QMailMessagePart::Location)),
               adaptor, MESSAGE(retrieveMessagePart(quint64, QMailMessagePart::Location)));
    connectIpc(this, SIGNAL(retrieveMessageRange(quint64, QMailMessageId, uint)),
               adaptor, MESSAGE(retrieveMessageRange(quint64, QMailMessageId, uint)));
    connectIpc(this, SIGNAL(retrieveMessagePartRange(quint64, QMailMessagePart::Location, uint)),
               adaptor, MESSAGE(retrieveMessagePartRange(quint64, QMailMessagePart::Location, uint)));
    connectIpc(this, SIGNAL(retrieveAll(quint64, QMailAccountId)),
               adaptor, MESSAGE(retrieveAll(quint64, QMailAccountId)));
    connectIpc(this, SIGNAL(exportUpdates(quint64, QMailAccountId)),
               adaptor, MESSAGE(exportUpdates(quint64, QMailAccountId)));
    connectIpc(this, SIGNAL(synchronize(quint64, QMailAccountId)),
               adaptor, MESSAGE(synchronize(quint64, QMailAccountId)));
    connectIpc(this, SIGNAL(cancelTransfer(quint64)),
               adaptor, MESSAGE(cancelTransfer(quint64)));
    connectIpc(this, SIGNAL(copyMessages(quint64, QMailMessageIdList, QMailFolderId)),
               adaptor, MESSAGE(copyMessages(quint64, QMailMessageIdList, QMailFolderId)));
    connectIpc(this, SIGNAL(moveMessages(quint64, QMailMessageIdList, QMailFolderId)),
               adaptor, MESSAGE(moveMessages(quint64, QMailMessageIdList, QMailFolderId)));
    connectIpc(this, SIGNAL(deleteMessages(quint64, QMailMessageIdList, QMailStore::MessageRemovalOption)),
               adaptor, MESSAGE(deleteMessages(quint64, QMailMessageIdList, QMailStore::MessageRemovalOption)));
    connectIpc(this, SIGNAL(flagMessages(quint64, QMailMessageIdList, quint64, quint64)),
               adaptor, MESSAGE(flagMessages(quint64, QMailMessageIdList, quint64, quint64)));
    connectIpc(this, SIGNAL(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, QMailMessageSortKey)),
               adaptor, MESSAGE(searchMessages(quint64, QMailMessageKey, QString, QMailSearchAction::SearchSpecification, QMailMessageSortKey)));
    connectIpc(this, SIGNAL(cancelSearch(quint64)),
               adaptor, MESSAGE(cancelSearch(quint64)));
    connectIpc(this, SIGNAL(shutdown()),
               adaptor, MESSAGE(shutdown()));
    connectIpc(this, SIGNAL(protocolRequest(quint64, QMailAccountId, QString, QVariant)),
               adaptor, MESSAGE(protocolRequest(quint64, QMailAccountId, QString, QVariant)));

    // Propagate received events as exposed signals
    connectIpc(adaptor, MESSAGE(activityChanged(quint64, QMailServiceAction::Activity)),
               parent, SIGNAL(activityChanged(quint64, QMailServiceAction::Activity)));
    connectIpc(adaptor, MESSAGE(connectivityChanged(quint64, QMailServiceAction::Connectivity)),
               parent, SIGNAL(connectivityChanged(quint64, QMailServiceAction::Connectivity)));
    connectIpc(adaptor, MESSAGE(statusChanged(quint64, const QMailServiceAction::Status)),
               parent, SIGNAL(statusChanged(quint64, const QMailServiceAction::Status)));
    connectIpc(adaptor, MESSAGE(progressChanged(quint64, uint, uint)),
               parent, SIGNAL(progressChanged(quint64, uint, uint)));
    connectIpc(adaptor, MESSAGE(messagesDeleted(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesDeleted(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(messagesCopied(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesCopied(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(messagesMoved(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesMoved(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(messagesFlagged(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesFlagged(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(storageActionCompleted(quint64)),
               parent, SIGNAL(storageActionCompleted(quint64)));
    connectIpc(adaptor, MESSAGE(retrievalCompleted(quint64)),
               parent, SIGNAL(retrievalCompleted(quint64)));
    connectIpc(adaptor, MESSAGE(messagesTransmitted(quint64, QMailMessageIdList)),
               parent, SIGNAL(messagesTransmitted(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(transmissionCompleted(quint64)),
               parent, SIGNAL(transmissionCompleted(quint64)));
    connectIpc(adaptor, MESSAGE(matchingMessageIds(quint64, QMailMessageIdList)),
               parent, SIGNAL(matchingMessageIds(quint64, QMailMessageIdList)));
    connectIpc(adaptor, MESSAGE(searchCompleted(quint64)),
               parent, SIGNAL(searchCompleted(quint64)));
    connectIpc(adaptor, MESSAGE(protocolResponse(quint64, QString, QVariant)),
               parent, SIGNAL(protocolResponse(quint64, QString, QVariant)));
    connectIpc(adaptor, MESSAGE(protocolRequestCompleted(quint64)),
               parent, SIGNAL(protocolRequestCompleted(quint64)));
}

QMailMessageServerPrivate::~QMailMessageServerPrivate()
{
}


/*!
    \class QMailMessageServer

    \preliminary
    \brief The QMailMessageServer class provides signals and slots which implement a convenient
    interface for communicating with the MessageServer process via IPC.

    \ingroup messaginglibrary

    Qt Extended messaging applications can send and receive messages of various types by
    communicating with the external MessageServer application.  The MessageServer application
    is a separate process, communicating with clients via inter-process messages.  
    QMailMessageServer acts as a proxy object for the server process, providing an
    interface for communicating with the MessageServer by the use of signals and slots 
    in the client process.  It provides Qt signals corresponding to messages received from 
    the MessageServer application, and Qt slots which send messages to the MessageServer 
    when invoked.

    For most messaging client applications, the QMailServiceAction objects offer a simpler
    interface for requesting actions from the messageserver, and assessing their results.

    \section1 New Messages

    When a client initiates communication with the MessageServer, the server informs the
    client of the number and type of 'new' messages, via the newCountChanged() signal.
    'New' messages are those that arrive without the client having first requested their
    retrieval.  The client may choose to invalidate the 'new' status of these messages;
    if the acknowledgeNewMessages() slot is invoked, the count of 'new' messages is reset
    to zero for the nominated message types.  If the count of 'new' messages changes while
    a client is active, the newCountChanged() signal is emitted with the updated information.

    \section1 Sending Messages

    To send messages, the client should construct instances of the QMailMessage class
    formulated to contain the desired content.  These messages should be stored to the
    mail store, within the Outbox folder configured for the parent account.

    An instance of QMailTransmitAction should be used to request transmission of the 
    outgoing messages.

    \section1 Retrieving Messages

    There are a variety of mechanisms for retrieving messages, at various levels of
    granularity.  In all cases, retrieved messages are added directly to the mail 
    store by the message server, from where clients can retrieve their meta data or
    content.

    An instance of QMailRetrievalAction should be used to request retrievel of 
    folders and messages.

    \sa QMailServiceAction, QMailStore
*/

/*!
    \fn void QMailMessageServer::activityChanged(quint64 action, QMailServiceAction::Activity activity);

    Emitted whenever the MessageServer experiences a change in the activity status of the request
    identified by \a action.  The request's new status is described by \a activity.
*/

/*!
    \fn void QMailMessageServer::connectivityChanged(quint64 action, QMailServiceAction::Connectivity connectivity);

    Emitted whenever the MessageServer has a change in connectivity while servicing the request 
    identified by \a action.  The new server connectivity status is described by \a connectivity.
*/

/*!
    \fn void QMailMessageServer::statusChanged(quint64 action, const QMailServiceAction::Status status);

    Emitted whenever the MessageServer experiences a status change that may be of interest to the client,
    while servicing the request identified by \a action.  The new server status is described by \a status.
*/

/*!
    \fn void QMailMessageServer::progressChanged(quint64 action, uint progress, uint total);

    Emitted when the progress of the request identified by \a action changes; 
    \a total indicates the extent of the operation to be performed, \a progress indicates the current degree of completion.
*/

/*!
    \fn void QMailMessageServer::newCountChanged(const QMailMessageCountMap& counts);

    Emitted when the count of 'new' messages changes; the new count is described by \a counts.

    \sa acknowledgeNewMessages()
*/

/*!
    \fn void QMailMessageServer::retrievalCompleted(quint64 action);

    Emitted when the retrieval operation identified by \a action is completed.
*/

/*!
    \fn void QMailMessageServer::messagesTransmitted(quint64 action, const QMailMessageIdList& list);

    Emitted when the messages identified by \a list have been transmitted to the external server,
    in response to the request identified by \a action.

    \sa transmitMessages()
*/

/*!
    \fn void QMailMessageServer::transmissionCompleted(quint64 action);

    Emitted when the transmit operation identified by \a action is completed.

    \sa transmitMessages()
*/

/*!
    \fn void QMailMessageServer::messagesDeleted(quint64 action, const QMailMessageIdList& list);

    Emitted when the messages identified by \a list have been deleted from the mail store,
    in response to the request identified by \a action.

    \sa deleteMessages()
*/

/*!
    \fn void QMailMessageServer::messagesCopied(quint64 action, const QMailMessageIdList& list);

    Emitted when the messages identified by \a list have been copied to the destination 
    folder on the external service, in response to the request identified by \a action.

    \sa copyMessages()
*/

/*!
    \fn void QMailMessageServer::messagesMoved(quint64 action, const QMailMessageIdList& list);

    Emitted when the messages identified by \a list have been moved to the destination 
    folder on the external service, in response to the request identified by \a action.

    \sa moveMessages()
*/

/*!
    \fn void QMailMessageServer::messagesFlagged(quint64 action, const QMailMessageIdList& list);

    Emitted when the messages identified by \a list have been flagged with the specified
    set of status flags, in response to the request identified by \a action.

    \sa flagMessages()
*/

/*!
    \fn void QMailMessageServer::storageActionCompleted(quint64 action);

    Emitted when the storage operation identified by \a action is completed.

    \sa deleteMessages(), copyMessages(), moveMessages(), flagMessages()
*/

/*!
    \fn void QMailMessageServer::searchCompleted(quint64 action);

    Emitted when the search operation identified by \a action is completed.

    \sa searchMessages()
*/

/*!
    \fn void QMailMessageServer::matchingMessageIds(quint64 action, const QMailMessageIdList& ids);

    Emitted after the successful completion of the search operation identified by \a action; 
    \a ids contains the list of message identifiers located by the search.

    \sa searchMessages()
*/

/*!
    \fn void QMailMessageServer::protocolResponse(quint64 action, const QString &response, const QVariant &data);

    Emitted when the protocol request identified by \a action generates the response
    \a response, with the associated \a data.

    \sa protocolRequest()
*/

/*!
    \fn void QMailMessageServer::protocolRequestCompleted(quint64 action);

    Emitted when the protocol request identified by \a action is completed.

    \sa protocolRequest()
*/

/*!
    Constructs a QMailMessageServer object with parent \a parent, and initiates communication with the MessageServer application.
*/
QMailMessageServer::QMailMessageServer(QObject* parent)
    : QObject(parent),
      d(new QMailMessageServerPrivate(this))
{
}

/*!
    Destroys the QMailMessageServer object.
*/
QMailMessageServer::~QMailMessageServer()
{
}

/*!
    Requests that the MessageServer application transmit any messages belonging to the
    account identified by \a accountId that are currently in the Outbox folder.
    The request has the identifier \a action.

    \sa transmissionCompleted()
*/
void QMailMessageServer::transmitMessages(quint64 action, const QMailAccountId &accountId)
{
    emit d->transmitMessages(action, accountId);
}

/*!
    Requests that the message server retrieve the list of folders available for the account \a accountId.
    If \a folderId is valid, the folders within that folder should be retrieved.  If \a descending is true,
    the search should also recursively retrieve the folders available within the previously retrieved folders.
    The request has the identifier \a action.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveFolderList(quint64 action, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending)
{
    emit d->retrieveFolderList(action, accountId, folderId, descending);
}

/*!
    Requests that the message server retrieve the list of messages available for the account \a accountId.
    If \a folderId is valid, then only messages within that folder should be retrieved; otherwise 
    messages within all folders in the account should be retrieved. If a folder messages are being 
    retrieved from contains at least \a minimum messages then the messageserver should ensure that at 
    least \a minimum messages are available from the mail store for that folder; otherwise if the
    folder contains less than \a minimum messages the messageserver should ensure all the messages for 
    that folder are available from the mail store.
    
    If \a sort is not empty, the external service will 
    discover the listed messages in the ordering indicated by the sort criterion, if possible.

    The request has the identifier \a action.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveMessageList(quint64 action, const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    emit d->retrieveMessageList(action, accountId, folderId, minimum, sort);
}

/*!
    Requests that the message server retrieve data regarding the messages identified by \a messageIds.  

    If \a spec is \l QMailRetrievalAction::Flags, then the message server should detect if 
    the messages identified by \a messageIds have been marked as read or have been removed.
    Messages that have been read will be marked with the \l QMailMessage::ReadElsewhere flag, and
    messages that have been removed will be marked with the \l QMailMessage::Removed status flag.

    If \a spec is \l QMailRetrievalAction::MetaData, then the message server should 
    retrieve the meta data of the each message listed in \a messageIds.
    
    If \a spec is \l QMailRetrievalAction::Content, then the message server should 
    retrieve the entirety of each message listed in \a messageIds.

    The request has the identifier \a action.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveMessages(quint64 action, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec)
{
    emit d->retrieveMessages(action, messageIds, spec);
}

/*!
    Requests that the message server retrieve the message part that is indicated by the 
    location \a partLocation.
    The request has the identifier \a action.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveMessagePart(quint64 action, const QMailMessagePart::Location &partLocation)
{
    emit d->retrieveMessagePart(action, partLocation);
}

/*!
    Requests that the message server retrieve a subset of the message \a messageId, such that
    at least \a minimum bytes are available from the mail store.
    The request has the identifier \a action.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveMessageRange(quint64 action, const QMailMessageId &messageId, uint minimum)
{
    emit d->retrieveMessageRange(action, messageId, minimum);
}

/*!
    Requests that the message server retrieve a subset of the message part that is indicated by 
    the location \a partLocation.  The messageserver should ensure that at least \a minimum 
    bytes are available from the mail store.
    The request has the identifier \a action.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveMessagePartRange(quint64 action, const QMailMessagePart::Location &partLocation, uint minimum)
{
    emit d->retrieveMessagePartRange(action, partLocation, minimum);
}

/*!
    Requests that the message server retrieve the meta data for all messages available 
    for the account \a accountId.
    The request has the identifier \a action.

    \sa retrievalCompleted()
*/
void QMailMessageServer::retrieveAll(quint64 action, const QMailAccountId &accountId)
{
    emit d->retrieveAll(action, accountId);
}

/*!
    Requests that the message server update the external server with any changes to message
    status that have been effected on the local device for account \a accountId.
    The request has the identifier \a action.

    \sa retrievalCompleted()
*/
void QMailMessageServer::exportUpdates(quint64 action, const QMailAccountId &accountId)
{
    emit d->exportUpdates(action, accountId);
}

/*!
    Requests that the message server synchronize the set of known message identifiers 
    with those currently available for the account identified by \a accountId.
    Newly discovered messages should have their meta data retrieved
    and local changes to message status should be exported to the external server.
    The request has the identifier \a action.

    \sa retrievalCompleted()
*/
void QMailMessageServer::synchronize(quint64 action, const QMailAccountId &accountId)
{
    emit d->synchronize(action, accountId);
}

/*!
    Requests that the MessageServer create a copy of each message listed in \a mailList 
    in the folder identified by \a destinationId.
    The request has the identifier \a action.
*/
void QMailMessageServer::copyMessages(quint64 action, const QMailMessageIdList& mailList, const QMailFolderId &destinationId)
{
    emit d->copyMessages(action, mailList, destinationId);
}

/*!
    Requests that the MessageServer move each message listed in \a mailList from its 
    current location to the folder identified by \a destinationId.
    The request has the identifier \a action.
*/
void QMailMessageServer::moveMessages(quint64 action, const QMailMessageIdList& mailList, const QMailFolderId &destinationId)
{
    emit d->moveMessages(action, mailList, destinationId);
}

/*!
    Requests that the MessageServer flag each message listed in \a mailList by setting
    the status flags set in \a setMask, and unsetting the status flags set in \a unsetMask.
    The request has the identifier \a action.

    The protocol must ensure that the local message records are appropriately modified, 
    although the external changes may be buffered and effected at the next invocation 
    of exportUpdates().
*/
void QMailMessageServer::flagMessages(quint64 action, const QMailMessageIdList& mailList, quint64 setMask, quint64 unsetMask)
{
    emit d->flagMessages(action, mailList, setMask, unsetMask);
}

/*!
    Requests that the MessageServer cancel any pending transfer operations for the request identified by \a action.

    \sa transmitMessages(), retrieveMessages()
*/
void QMailMessageServer::cancelTransfer(quint64 action)
{
    emit d->cancelTransfer(action);
}

/*!
    Requests that the MessageServer reset the counts of 'new' messages to zero, for
    each message type listed in \a types.

    \sa newCountChanged()
*/
void QMailMessageServer::acknowledgeNewMessages(const QMailMessageTypeList& types)
{
    emit d->acknowledgeNewMessages(types);
}

/*!
    Requests that the MessageServer delete the messages in \a mailList from the external
    server, if necessary for the relevant message type.  If \a option is 
    \l{QMailStore::CreateRemovalRecord}{CreateRemovalRecord} then a QMailMessageRemovalRecord
    will be created in the mail store for each deleted message.
    The request has the identifier \a action.

    Deleting messages using this slot does not initiate communication with any external
    server; instead the information needed to delete the messages is recorded.  Deletion
    from the external server will occur when messages are next retrieved from that server.
    Invoking this slot does not remove a message from the mail store.

    \sa QMailStore::removeMessage()
*/
void QMailMessageServer::deleteMessages(quint64 action, const QMailMessageIdList& mailList, QMailStore::MessageRemovalOption option)
{
    emit d->deleteMessages(action, mailList, option);
}

/*!
    Requests that the MessageServer search for messages that meet the criteria encoded
    in \a filter.  If \a bodyText is non-empty, messages must also contain the specified
    text in their content to be considered matching.  If \a spec is 
    \l{QMailSearchAction::Remote}{Remote} then the MessageServer will extend the search
    to consider messages held at external servers that are not present on the local device.
    If \a sort is not empty, the external service will return matching messages in 
    the ordering indicated by the sort criterion if possible.

    The request has the identifier \a action.

    The identifiers of all matching messages are returned via matchingMessageIds() after 
    the search is completed.

    \sa matchingMessageIds()
*/
void QMailMessageServer::searchMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort)
{
    emit d->searchMessages(action, filter, bodyText, spec, sort);
}

/*!
    Requests that the MessageServer cancel any pending search operations for the request identified by \a action.
*/
void QMailMessageServer::cancelSearch(quint64 action)
{
    emit d->cancelSearch(action);
}

/*!
    Requests that the MessageServer shutdown and terminate
*/
void QMailMessageServer::shutdown()
{
    emit d->shutdown();
}

/*!
    Requests that the MessageServer forward the protocol-specific request \a request
    to the QMailMessageSource configured for the account identified by \a accountId.
    The request, identified by \a action, may have associated \a data, in a protocol-specific form.
*/
void QMailMessageServer::protocolRequest(quint64 action, const QMailAccountId &accountId, const QString &request, const QVariant &data)
{
    emit d->protocolRequest(action, accountId, request, data);
}

Q_IMPLEMENT_USER_METATYPE_TYPEDEF(QMailMessageCountMap, QMailMessageCountMap)

#include "qmailmessageserver.moc"

