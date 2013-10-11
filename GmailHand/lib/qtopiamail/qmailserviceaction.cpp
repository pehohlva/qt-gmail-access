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

#include "qmailserviceaction_p.h"
#include "qmailipc.h"
#include "qmailmessagekey.h"
#include "qmailstore.h"
#include "qmaillog.h"
#include <QCoreApplication>
#include <QPair>
#include <QTimer>

namespace {

uint messageCounter = 0;
const uint pid = static_cast<uint>(QCoreApplication::applicationPid() & 0xffffffff);

quint64 nextMessageAction()
{
    return ((quint64(pid) << 32) | quint64(++messageCounter));
}

QPair<uint, uint> messageActionParts(quint64 action)
{
    return qMakePair(uint(action >> 32), uint(action & 0xffffffff));
}

}


template<typename Subclass>
QMailServiceActionPrivate::QMailServiceActionPrivate(Subclass *p, QMailServiceAction *i)
    : QPrivateNoncopyableBase(p),
      _interface(i),
      _server(new QMailMessageServer(this)),
      _connectivity(QMailServiceAction::Offline),
      _activity(QMailServiceAction::Successful),
      _status(QMailServiceAction::Status::ErrNoError, QString(), QMailAccountId(), QMailFolderId(), QMailMessageId()),
      _total(0),
      _progress(0),
      _action(0)
{
    connect(_server, SIGNAL(activityChanged(quint64, QMailServiceAction::Activity)),
            this, SLOT(activityChanged(quint64, QMailServiceAction::Activity)));
    connect(_server, SIGNAL(connectivityChanged(quint64, QMailServiceAction::Connectivity)),
            this, SLOT(connectivityChanged(quint64, QMailServiceAction::Connectivity)));
    connect(_server, SIGNAL(statusChanged(quint64, const QMailServiceAction::Status)),
            this, SLOT(statusChanged(quint64, const QMailServiceAction::Status)));
    connect(_server, SIGNAL(progressChanged(quint64, uint, uint)),
            this, SLOT(progressChanged(quint64, uint, uint)));
}

QMailServiceActionPrivate::~QMailServiceActionPrivate()
{
}

void QMailServiceActionPrivate::cancelOperation()
{
    if (_action != 0) {
        _server->cancelTransfer(_action);
    }
}

void QMailServiceActionPrivate::activityChanged(quint64 action, QMailServiceAction::Activity activity)
{
    if (validAction(action)) {
        setActivity(activity);

        emitChanges();
    }
}

void QMailServiceActionPrivate::connectivityChanged(quint64 action, QMailServiceAction::Connectivity connectivity)
{
    if (validAction(action)) {
        setConnectivity(connectivity);

        emitChanges();
    }
}

void QMailServiceActionPrivate::statusChanged(quint64 action, const QMailServiceAction::Status status)
{
    if (validAction(action)) {
        setStatus(status);

        emitChanges();
    }
}

void QMailServiceActionPrivate::progressChanged(quint64 action, uint progress, uint total)
{
    if (validAction(action)) {
        setProgress(progress, total);

        emitChanges();
    }
}

void QMailServiceActionPrivate::init()
{
    _connectivity = QMailServiceAction::Offline;
    _activity = QMailServiceAction::Successful;
    _status = QMailServiceAction::Status(QMailServiceAction::Status::ErrNoError, QString(), QMailAccountId(), QMailFolderId(), QMailMessageId());
    _total = 0;
    _progress = 0;
    _action = 0;
    _connectivityChanged = false;
    _activityChanged = false;
    _progressChanged = false;
    _statusChanged = false;
}

quint64 QMailServiceActionPrivate::newAction()
{
    if (_action != 0) {
        qWarning() << "Unable to allocate new action - oustanding:" << messageActionParts(_action).second;
        return _action;
    }

    init();

    _action = nextMessageAction();
    setActivity(QMailServiceAction::Pending);
    emitChanges();

    return _action;
}

bool QMailServiceActionPrivate::validAction(quint64 action)
{
    if (_action == 0)
        return false;

    QPair<uint, uint> outstanding(messageActionParts(_action));
    QPair<uint, uint> incoming(messageActionParts(action));

    if (incoming.first != outstanding.first)
        return false;

    if (incoming.second == outstanding.second)
        return true;

    return false;
}

void QMailServiceActionPrivate::setConnectivity(QMailServiceAction::Connectivity newConnectivity)
{
    if ((_action != 0) && (_connectivity != newConnectivity)) {
        _connectivity = newConnectivity;
        _connectivityChanged = true;
    }
}

void QMailServiceActionPrivate::setActivity(QMailServiceAction::Activity newActivity)
{
    if ((_action != 0) && (_activity != newActivity)) {
        _activity = newActivity;

        if (_activity == QMailServiceAction::Failed || _activity == QMailServiceAction::Successful) {
            // Reset any progress we've indicated
            _total = 0;
            _progress = 0;
            _progressChanged = true;

            // We're finished
            _action = 0;
        }

        _activityChanged = true;
    }
}

void QMailServiceActionPrivate::setStatus(const QMailServiceAction::Status &status)
{
    if (_action != 0) {
        _status = status;
        _statusChanged = true;
    }
}

void QMailServiceActionPrivate::setStatus(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    setStatus(code, text, QMailAccountId(), QMailFolderId(), QMailMessageId());
}

void QMailServiceActionPrivate::setStatus(QMailServiceAction::Status::ErrorCode code, const QString &text, const QMailAccountId &accountId, const QMailFolderId &folderId, const QMailMessageId &messageId)
{
    if (_action != 0) {
        _status = QMailServiceAction::Status(code, text, accountId, folderId, messageId);
        _statusChanged = true;
    }
}

void QMailServiceActionPrivate::setProgress(uint newProgress, uint newTotal)
{
    if (_action != 0) {
        if (newTotal != _total) {
            _total = newTotal;
            _progressChanged = true;
        }

        newProgress = qMin(newProgress, _total);
        if (newProgress != _progress) {
            _progress = newProgress;
            _progressChanged = true;
        }
    }
}

void QMailServiceActionPrivate::emitChanges()
{
    if (_connectivityChanged) {
        _connectivityChanged = false;
        emit _interface->connectivityChanged(_connectivity);
    }
    if (_activityChanged) {
        _activityChanged = false;
        emit _interface->activityChanged(_activity);
    }
    if (_progressChanged) {
        _progressChanged = false;
        emit _interface->progressChanged(_progress, _total);
    }
    if (_statusChanged) {
        _statusChanged = false;
        emit _interface->statusChanged(_status);
    }
}


/*!
    \class QMailServiceAction::Status

    \preliminary
    \ingroup messaginglibrary

    \brief The Status class encapsulates the instantaneous state of a QMailServiceAction.

    QMailServiceAction::Status contains the pieces of information that describe the state of a
    requested action.  The \l errorCode reflects the overall state, and may be supplemented
    by a description in \l text.

    If \l errorCode is not equal to \l ErrNoError, then each of \l accountId, \l folderId and 
    \l messageId may have been set to a valid identifier, if pertinent to the situation.
*/

/*!
    \enum QMailServiceAction::Status::ErrorCode

    This enum type is used to identify common error conditions encountered when implementing service actions.

    \value ErrNoError               No error was encountered.
    \value ErrNotImplemented        The requested operation is not implemented by the relevant service component.
    \value ErrFrameworkFault        A fault in the messaging framework prevented the execution of the request.
    \value ErrSystemError           A system-level error prevented the execution of the request.
    \value ErrCancel                The operation was canceled by user intervention.
    \value ErrConfiguration         The configuration needed for the requested action is invalid.
    \value ErrNoConnection          A connection could not be established to the external service.
    \value ErrConnectionInUse       The connection to the external service is exclusively held by another user.
    \value ErrConnectionNotReady    The connection to the external service is not ready for operation.
    \value ErrUnknownResponse       The response from the external service could not be handled.
    \value ErrLoginFailed           The external service did not accept the supplied authentication details.
    \value ErrFileSystemFull        The action could not be performed due to insufficient storage space.
    \value ErrNonexistentMessage    The specified message does not exist.
    \value ErrEnqueueFailed         The specified message could not be enqueued for transmission.
    \value ErrInvalidAddress        The specified address is invalid for the requested action.
    \value ErrInvalidData           The supplied data is inappropriate for the requested action.
    \value ErrTimeout               The service failed to report any activity for an extended period.
    \value ErrorCodeMinimum         The lowest value of any error condition code.
    \value ErrorCodeMaximum         The highest value of any error condition code.
*/

/*! \variable QMailServiceAction::Status::errorCode
    
    Describes the error condition encountered by the action.
*/

/*! \variable QMailServiceAction::Status::text
    
    Provides a human-readable description of the error condition in \l errorCode.
*/

/*! \variable QMailServiceAction::Status::accountId
    
    If relevant to the \l errorCode, contains the ID of the associated account.
*/

/*! \variable QMailServiceAction::Status::folderId
    
    If relevant to the \l errorCode, contains the ID of the associated folder.
*/

/*! \variable QMailServiceAction::Status::messageId
    
    If relevant to the \l errorCode, contains the ID of the associated message.
*/

/*! 
    \fn QMailServiceAction::Status::Status()

    Constructs a status object with \l errorCode set to \l{QMailServiceAction::Status::ErrNoError}{ErrNoError}.
*/
QMailServiceAction::Status::Status()
    : errorCode(ErrNoError)
{
}

/*! 
    \fn QMailServiceAction::Status::Status(ErrorCode c, const QString &t, const QMailAccountId &a, const QMailFolderId &f, const QMailMessageId &m)

    Constructs a status object with 
    \l errorCode set to \a c, 
    \l text set to \a t, 
    \l accountId set to \a a, 
    \l folderId set to \a f and 
    \l messageId set to \a m.
*/
QMailServiceAction::Status::Status(ErrorCode c, const QString &t, const QMailAccountId &a, const QMailFolderId &f, const QMailMessageId &m)
    : errorCode(c),
      text(t),
      accountId(a),
      folderId(f),
      messageId(m)
{
}

/*! 
    \fn QMailServiceAction::Status::Status(const Status&)

    Constructs a status object which is a copy of \a other.
*/
QMailServiceAction::Status::Status(const QMailServiceAction::Status &other)
    : errorCode(other.errorCode),
      text(other.text),
      accountId(other.accountId),
      folderId(other.folderId),
      messageId(other.messageId)
{
}

/*! 
    \fn QMailServiceAction::Status::serialize(Stream&) const
    \internal 
*/
template <typename Stream> 
void QMailServiceAction::Status::serialize(Stream &stream) const
{
    stream << errorCode;
    stream << text;
    stream << accountId;
    stream << folderId;
    stream << messageId;
}

template void QMailServiceAction::Status::serialize(QDataStream &) const;

/*! 
    \fn QMailServiceAction::Status::deserialize(Stream&)
    \internal 
*/
template <typename Stream> 
void QMailServiceAction::Status::deserialize(Stream &stream)
{
    stream >> errorCode;
    stream >> text;
    stream >> accountId;
    stream >> folderId;
    stream >> messageId;
}

template void QMailServiceAction::Status::deserialize(QDataStream &);

/*!
    \class QMailServiceAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailServiceAction class provides the interface for requesting actions from external message services.

    QMailServiceAction provides the mechanisms for messaging clients to request actions from 
    external services, and to receive information in response.  The details of requests
    differ for each derived action, and the specific data returned is also variable.  However,
    all actions present the same interface for communicating status, connectivity and 
    progress information.

    All actions communicate changes in their operational state by emitting the activityChanged()
    signal.  If the execution of the action requires connectivity to an external service, then
    changes in connectivity state are emitted via the connectivityChanged() signal.  Some actions
    are able to provide progress information as they execute; these changes are reported via the 
    progressChanged() signal.  If error conditions are encountered, the statusChanged() signal is
    emitted to report the new status.

    A user may attempt to cancel an operation after it has been initiated.  The cancelOperation()
    slot is provided for this purpose.

    A QMailServiceAction instance supports only a single request at any time.  An application
    may, however, use multiple QMailServiceAction instances to send independent requests concurrently.
    Each QMailServiceAction instance will report only the changes pertaining to the request
    that instance delivered.  Whether or not concurrent requests are concurrently serviced by 
    the message server depends on whether those requests must be serviced by the same 
    QMailMessageService instance.

    \sa QMailMessageService
*/

/*!
    \enum QMailServiceAction::Connectivity

    This enum type is used to describe the connectivity state of the service implementing an action.

    \value Offline          The service is offline.
    \value Connecting       The service is currently attempting to establish a connection.
    \value Connected        The service is connected.
    \value Disconnected     The service has been disconnected.
*/

/*!
    \enum QMailServiceAction::Activity

    This enum type is used to describe the activity state of the requested action.

    \value Pending          The action has not yet begun execution.
    \value InProgress       The action is currently executing.
    \value Successful       The action has completed successfully.
    \value Failed           The action could not be completed successfully, and has finished execution.
*/

/*!
    \fn QMailServiceAction::connectivityChanged(QMailServiceAction::Connectivity c)

    This signal is emitted when the connectivity status of the service performing 
    the action changes, with the new state described by \a c.

    \sa connectivity()
*/

/*!
    \fn QMailServiceAction::activityChanged(QMailServiceAction::Activity a)

    This signal is emitted when the activity status of the action changes,
    with the new state described by \a a.

    \sa activity()
*/

/*!
    \fn QMailServiceAction::statusChanged(const QMailServiceAction::Status &s)

    This signal is emitted when the error status of the action changes, with
    the new status described by \a s.

    \sa status()
*/

/*!
    \fn QMailServiceAction::progressChanged(uint value, uint total)

    This signal is emitted when the progress of the action changes, with
    the new state described by \a value and \a total.

    \sa progress()
*/

/*!
    \typedef QMailServiceAction::ImplementationType
    \internal
*/

/*!
    \fn QMailServiceAction::QMailServiceAction(Subclass*, QObject*)
    \internal
*/
template<typename Subclass>
QMailServiceAction::QMailServiceAction(Subclass* p, QObject *parent)
    : QObject(parent),
      QPrivatelyNoncopyable<QMailServiceActionPrivate>(p)
{
}

/*! \internal */
QMailServiceAction::~QMailServiceAction()
{
}

/*!
    Returns the current connectivity state of the service implementing this action.

    \sa connectivityChanged()
*/
QMailServiceAction::Connectivity QMailServiceAction::connectivity() const
{
    return impl(this)->_connectivity;
}

/*!
    Returns the current activity state of the action.

    \sa activityChanged()
*/
QMailServiceAction::Activity QMailServiceAction::activity() const
{
    return impl(this)->_activity;
}

/*!
    Returns the current status of the service action.

    \sa statusChanged()
*/
const QMailServiceAction::Status QMailServiceAction::status() const
{
    return impl(this)->_status;
}

/*!
    Returns the current progress of the service action.

    \sa progressChanged()
*/
QPair<uint, uint> QMailServiceAction::progress() const
{
    return qMakePair(impl(this)->_progress, impl(this)->_total);
}

/*!
    Attempts to cancel the last requested operation.
*/
void QMailServiceAction::cancelOperation()
{
    impl(this)->cancelOperation();
}

/*!
    Set the current status so that 
    \l{QMailServiceAction::Status::errorCode} errorCode is set to \a c and 
    \l{QMailServiceAction::Status::text} text is set to \a t.
*/
void QMailServiceAction::setStatus(QMailServiceAction::Status::ErrorCode c, const QString &t)
{
    impl(this)->setStatus(c, t, QMailAccountId(), QMailFolderId(), QMailMessageId());
}

/*!
    Set the current status so that 
    \l{QMailServiceAction::Status::errorCode} errorCode is set to \a c, 
    \l{QMailServiceAction::Status::text} text is set to \a t,
    \l{QMailServiceAction::Status::accountId} accountId is set to \a a, 
    \l{QMailServiceAction::Status::folderId} folderId is set to \a f and 
    \l{QMailServiceAction::Status::messageId} messageId is set to \a m.
*/
void QMailServiceAction::setStatus(QMailServiceAction::Status::ErrorCode c, const QString &t, const QMailAccountId &a, const QMailFolderId &f, const QMailMessageId &m)
{
    impl(this)->setStatus(c, t, a, f, m);
}


QMailRetrievalActionPrivate::QMailRetrievalActionPrivate(QMailRetrievalAction *i)
    : QMailServiceActionPrivate(this, i)
{
    connect(_server, SIGNAL(retrievalCompleted(quint64)),
            this, SLOT(retrievalCompleted(quint64)));

    init();
}

void QMailRetrievalActionPrivate::retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending)
{
    _server->retrieveFolderList(newAction(), accountId, folderId, descending);
}

void QMailRetrievalActionPrivate::retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    _server->retrieveMessageList(newAction(), accountId, folderId, minimum, sort);
}

void QMailRetrievalActionPrivate::retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec)
{
    _server->retrieveMessages(newAction(), messageIds, spec);
}

void QMailRetrievalActionPrivate::retrieveMessagePart(const QMailMessagePart::Location &partLocation)
{
    _server->retrieveMessagePart(newAction(), partLocation);
}

void QMailRetrievalActionPrivate::retrieveMessageRange(const QMailMessageId &messageId, uint minimum)
{
    _server->retrieveMessageRange(newAction(), messageId, minimum);
}

void QMailRetrievalActionPrivate::retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum)
{
    _server->retrieveMessagePartRange(newAction(), partLocation, minimum);
}

void QMailRetrievalActionPrivate::retrieveAll(const QMailAccountId &accountId)
{
    _server->retrieveAll(newAction(), accountId);
}

void QMailRetrievalActionPrivate::exportUpdates(const QMailAccountId &accountId)
{
    _server->exportUpdates(newAction(), accountId);
}

void QMailRetrievalActionPrivate::synchronize(const QMailAccountId &accountId)
{
    _server->synchronize(newAction(), accountId);
}

void QMailRetrievalActionPrivate::retrievalCompleted(quint64 action)
{
    if (validAction(action)) {
        setActivity(QMailServiceAction::Successful);
        emitChanges();
    }
}


/*!
    \class QMailRetrievalAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailRetrievalAction class provides the interface for retrieving messages from external message services.

    QMailRetrievalAction provides the mechanism for messaging clients to request that the message
    server retrieve messages from external services.  The retrieval action object reports on
    the progress and outcome of its activities via the signals inherited from QMailServiceAction.

    A range of functions are available to support varying client operations:

    The retrieveFolderList() function allows a client to retrieve the list of folders available for an account.
    The retrieveMessageList() function allows a client to retrieve the list of messages available for an account.

    The retrieveMessages() function allows a client to retrieve the flags, meta data or content of a 
    specific list of messages.

    The retrieveMessageRange() functions allows a portion of a message's content to be retrieved, rather than
    the entire content data.

    The retrieveMessagePart() function allows a specific part of a multi-part message to be retrieved.
    The retrieveMessagePartRange() function allows a portion of a specific part of a multi-part message to be retrieved.

    The retrieveAll() function allows a client to retrieve the meta data for all messages currently
    available for the specified account.  
    The exportUpdates() function allows a client to push local changes such as message-read notifications
    to the external server.

    The synchronize() function allows a client to synchronize the local representation of an account
    with that available at the external server.
*/

/*!
    \enum QMailRetrievalAction::RetrievalSpecification

    This enum type is specify the form of retrieval that the message server should perform.

    \value Flags        Changes to the state of the message should be retrieved.
    \value MetaData     Only the meta data of the message should be retrieved.
    \value Content      The entire content of the message should be retrieved.
*/

/*!
    \typedef QMailRetrievalAction::ImplementationType
    \internal
*/

/*! 
    Constructs a new retrieval action object with the supplied \a parent.
*/
QMailRetrievalAction::QMailRetrievalAction(QObject *parent)
    : QMailServiceAction(new QMailRetrievalActionPrivate(this), parent)
{
}

/*! \internal */
QMailRetrievalAction::~QMailRetrievalAction()
{
}

/*!
    Requests that the message server retrieve the list of folders available for the 
    account \a accountId.  If \a folderId is valid, only the identified folder is 
    searched for child folders; otherwise the search begins at the root of the
    account.  If \a descending is true, the search should also recursively search 
    for child folders within folders discovered during the search.

    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties will be updated for each 
    folder that is searched for child folders; these properties are not updated 
    for folders that are merely discovered by searching.
    
    \sa retrieveAll()
*/
void QMailRetrievalAction::retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending)
{
    impl(this)->retrieveFolderList(accountId, folderId, descending);
}

/*!
    Requests that the message server retrieve the list of messages available for the account \a accountId.
    If \a folderId is valid, then only messages within that folder should be retrieved; otherwise 
    messages within all folders in the account should be retrieved.  If \a minimum is non-zero,
    then that value will be used to restrict the number of messages to be retrieved from
    each folder; otherwise, all messages will be retrieved.
    
    If \a sort is not empty, the external service will report the discovered messages in the 
    ordering indicated by the sort criterion, if possible.  Services are not required to support 
    this facility.

    If a folder messages are being retrieved from contains at least \a minimum messages then the 
    messageserver should ensure that at least \a minimum messages are available from the mail 
    store for that folder; otherwise if the folder contains less than \a minimum messages the 
    messageserver should ensure all the messages for that folder are available from the mail store.
    If a folder has messages locally available, then all previously undiscovered messages will be
    retrieved for that folder, even if that number exceeds \a minimum.
    
    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties will be updated for each folder 
    from which messages are retrieved.
    
    New messages will be added to the mail store as they are discovered, and 
    marked with the \l QMailMessage::New status flag. Messages that are present
    in the mail store but found to be no longer available are marked with the 
    \l QMailMessage::Removed status flag.

    \sa retrieveAll()
*/
void QMailRetrievalAction::retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    impl(this)->retrieveMessageList(accountId, folderId, minimum, sort);
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
    
    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties will be updated for each folder 
    from which messages are retrieved.
*/
void QMailRetrievalAction::retrieveMessages(const QMailMessageIdList &messageIds, RetrievalSpecification spec)
{
    impl(this)->retrieveMessages(messageIds, spec);
}

/*!
    Requests that the message server retrieve the message part that is indicated by the 
    location \a partLocation.
    
    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties will be updated for the folder 
    from which the part is retrieved.
*/
void QMailRetrievalAction::retrieveMessagePart(const QMailMessagePart::Location &partLocation)
{
    impl(this)->retrieveMessagePart(partLocation);
}

/*!
    Requests that the message server retrieve a subset of the message \a messageId, such that
    at least \a minimum bytes are available from the mail store.
    
    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties will be updated for the folder 
    from which the message is retrieved.
*/
void QMailRetrievalAction::retrieveMessageRange(const QMailMessageId &messageId, uint minimum)
{
    impl(this)->retrieveMessageRange(messageId, minimum);
}

/*!
    Requests that the message server retrieve a subset of the message part that is indicated 
    by the location \a partLocation.  The messageserver should ensure that at least 
    \a minimum bytes are available from the mail store.
    
    The total size of the part on the server is given by QMailMessagePart::contentDisposition().size(),
    the amount of the part available locally is given by QMailMessagePart::body().length().
    
    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties will be updated for the folder 
    from which the part is retrieved.
*/
void QMailRetrievalAction::retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum)
{
    impl(this)->retrieveMessagePartRange(partLocation, minimum);
}

/*!
    Requests that the message server retrieve all folders and meta data for messages available 
    for the account \a accountId.
    
    All folders within the account will be discovered and searched for child folders.
    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties will be updated for each folder 
    in the account.

    Meta data will be retrieved for every message found in the account.
    New messages will be added to the mail store as they are retrieved, and 
    marked with the \l QMailMessage::New status flag.  Messages that are no longer 
    available will be marked with the \l QMailMessage::Removed status flag.  

    \sa retrieveFolderList(), retrieveMessageList(), synchronize()
*/
void QMailRetrievalAction::retrieveAll(const QMailAccountId &accountId)
{
    impl(this)->retrieveAll(accountId);
}

/*!
    Requests that the message server update the external server with any changes to message
    status that have been effected on the local device for account \a accountId.

    \sa synchronize()
*/
void QMailRetrievalAction::exportUpdates(const QMailAccountId &accountId)
{
    impl(this)->exportUpdates(accountId);
}

/*!
    Requests that the message server synchronize the set of known folder and message 
    identifiers with those currently available for the account identified by \a accountId.
    Newly discovered messages should have their meta data retrieved
    and local changes to message status should be exported to the external server.

    New messages will be added to the mail store as they are discovered, and 
    marked with the \l QMailMessage::New status flag.  Messages that are no longer 
    available will be marked with the \l QMailMessage::Removed status flag.  

    The folder structure of the account will be synchronized with that available from 
    the external service.  The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties will be updated for each folder.

    \sa retrieveAll(), exportUpdates()
*/
void QMailRetrievalAction::synchronize(const QMailAccountId &accountId)
{
    impl(this)->synchronize(accountId);
}


QMailTransmitActionPrivate::QMailTransmitActionPrivate(QMailTransmitAction *i)
    : QMailServiceActionPrivate(this, i)
{
    connect(_server, SIGNAL(messagesTransmitted(quint64, QMailMessageIdList)),
            this, SLOT(messagesTransmitted(quint64, QMailMessageIdList)));
    connect(_server, SIGNAL(transmissionCompleted(quint64)),
            this, SLOT(transmissionCompleted(quint64)));

    init();
}

void QMailTransmitActionPrivate::transmitMessages(const QMailAccountId &accountId)
{
    _server->transmitMessages(newAction(), accountId);

    QMailAccount account(accountId);
    _ids = QMailStore::instance()->queryMessages(QMailMessageKey::parentAccountId(accountId) & QMailMessageKey::status(QMailMessage::Outbox));

    emitChanges();
}

void QMailTransmitActionPrivate::init()
{
    QMailServiceActionPrivate::init();

    _ids.clear();
}

void QMailTransmitActionPrivate::messagesTransmitted(quint64 action, const QMailMessageIdList &ids)
{
    if (validAction(action)) {
        foreach (const QMailMessageId &id, ids)
            _ids.removeAll(id);
    }
}

void QMailTransmitActionPrivate::transmissionCompleted(quint64 action)
{
    if (validAction(action)) {
        QMailServiceAction::Activity result(_ids.isEmpty() ? QMailServiceAction::Successful : QMailServiceAction::Failed);
        setActivity(result);
        emitChanges();
    }
}


/*!
    \class QMailTransmitAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailTransmitAction class provides the interface for transmitting messages to external message services.

    QMailSearchAction provides the mechanism for messaging clients to request that the message
    server transmit messages to external services.  The transmit action object reports on
    the progress and outcome of its activities via the signals inherited from QMailServiceAction.

    The send() slot requests that the message server transmit each identified message to the
    external service associated with each messages' account.

    Note: the slots exported by QMailTransmitAction are expected to change in future releases, as 
    the message server is extended to provide a finer-grained interface for message transmission.
*/

/*!
    \typedef QMailTransmitAction::ImplementationType
    \internal
*/

/*!
    Constructs a new transmit action object with the supplied \a parent.
*/
QMailTransmitAction::QMailTransmitAction(QObject *parent)
    : QMailServiceAction(new QMailTransmitActionPrivate(this), parent)
{
}

/*! \internal */
QMailTransmitAction::~QMailTransmitAction()
{
}

/*!
    Requests that the message server transmit each message belonging to the account identified
    by \a accountId that is currently scheduled for transmission (located in the Outbox folder).

    Any message successfully transmitted will be moved to the account's Sent folder.
*/
void QMailTransmitAction::transmitMessages(const QMailAccountId &accountId)
{
    impl(this)->transmitMessages(accountId);
}


QMailStorageActionPrivate::QMailStorageActionPrivate(QMailStorageAction *i)
    : QMailServiceActionPrivate(this, i)
{
    connect(_server, SIGNAL(messagesDeleted(quint64, QMailMessageIdList)),
            this, SLOT(messagesEffected(quint64, QMailMessageIdList)));
    connect(_server, SIGNAL(messagesMoved(quint64, QMailMessageIdList)),
            this, SLOT(messagesEffected(quint64, QMailMessageIdList)));
    connect(_server, SIGNAL(messagesCopied(quint64, QMailMessageIdList)),
            this, SLOT(messagesEffected(quint64, QMailMessageIdList)));
    connect(_server, SIGNAL(messagesFlagged(quint64, QMailMessageIdList)),
            this, SLOT(messagesEffected(quint64, QMailMessageIdList)));

    connect(_server, SIGNAL(storageActionCompleted(quint64)),
            this, SLOT(storageActionCompleted(quint64)));

    init();
}

void QMailStorageActionPrivate::deleteMessages(const QMailMessageIdList &ids)
{
    _server->deleteMessages(newAction(), ids, QMailStore::CreateRemovalRecord);
    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::discardMessages(const QMailMessageIdList &ids)
{
    _server->deleteMessages(newAction(), ids, QMailStore::NoRemovalRecord);
    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destination)
{
    _server->copyMessages(newAction(), ids, destination);
    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destination)
{
    _server->moveMessages(newAction(), ids, destination);
    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask)
{
    // Ensure that nothing is both set and unset
    setMask &= ~unsetMask;
    _server->flagMessages(newAction(), ids, setMask, unsetMask);

    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::init()
{
    QMailServiceActionPrivate::init();

    _ids.clear();
}

void QMailStorageActionPrivate::messagesEffected(quint64 action, const QMailMessageIdList &ids)
{
    if (validAction(action)) {
        foreach (const QMailMessageId &id, ids)
            _ids.removeAll(id);
    }
}

void QMailStorageActionPrivate::storageActionCompleted(quint64 action)
{
    if (validAction(action)) {
        QMailServiceAction::Activity result(_ids.isEmpty() ? QMailServiceAction::Successful : QMailServiceAction::Failed);
        setActivity(result);
        emitChanges();
    }
}


/*!
    \class QMailStorageAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailStorageAction class provides the interface for requesting operations on the
    storage of messages within external message services.

    QMailStorageAction provides the mechanism for messaging clients to request that the message
    server move or copy messages within an external message service.  The storage action object 
    reports on the progress and outcome of its activities via the signals inherited from QMailServiceAction.

    The copyMessages() slot requests that the message server create a copy of each identified 
    message in the nominated destination folder.  The moveMessages() slot requests that the 
    message server move each identified message from its current location to the nominated destination 
    folder.  Messages cannot be moved or copied between accounts.
*/

/*!
    \typedef QMailStorageAction::ImplementationType
    \internal
*/

/*!
    Constructs a new transmit action object with the supplied \a parent.
*/
QMailStorageAction::QMailStorageAction(QObject *parent)
    : QMailServiceAction(new QMailStorageActionPrivate(this), parent)
{
}

/*! \internal */
QMailStorageAction::~QMailStorageAction()
{
}

/*!
    Requests that the message server delete the messages listed in \a ids, both from the local device 
    and the external message source.  Whether the external messages resources are actually removed is 
    at the discretion of the relevant QMailMessageSource object.
*/
void QMailStorageAction::deleteMessages(const QMailMessageIdList &ids)
{
    impl(this)->deleteMessages(ids);
}

/*!
    Requests that the message server delete the messages listed in \a ids from the local device only.
*/
void QMailStorageAction::discardMessages(const QMailMessageIdList &ids)
{
    impl(this)->discardMessages(ids);
}

/*!
    Requests that the message server create a new copy of each message listed in \a ids within the folder 
    identified by \a destinationId.
*/
void QMailStorageAction::copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId)
{
    impl(this)->copyMessages(ids, destinationId);
}

/*!
    Requests that the message server move each message listed in \a ids from its current location 
    to the folder identified by \a destinationId.
*/
void QMailStorageAction::moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId)
{
    impl(this)->moveMessages(ids, destinationId);
}

/*!
    Requests that the message server flag each message listed in \a ids, by setting any status flags
    set in the \a setMask, and unsetting any status flags set in the \a unsetMask.  The status
    flag values should correspond to those of QMailMessage::status().

    The service implementing the account may choose to take further actions in response to flag
    changes, such as moving or deleting messages.

    \sa QMailMessage::setStatus(), QMailStore::updateMessagesMetaData()
*/
void QMailStorageAction::flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask)
{
    impl(this)->flagMessages(ids, setMask, unsetMask);
}


QMailSearchActionPrivate::QMailSearchActionPrivate(QMailSearchAction *i)
    : QMailServiceActionPrivate(this, i)
{
    connect(_server, SIGNAL(matchingMessageIds(quint64, QMailMessageIdList)),
            this, SLOT(matchingMessageIds(quint64, QMailMessageIdList)));
    connect(_server, SIGNAL(searchCompleted(quint64)),
            this, SLOT(searchCompleted(quint64)));

    init();
}

void QMailSearchActionPrivate::searchMessages(const QMailMessageKey &filter, const QString &bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort)
{
    if ((spec == QMailSearchAction::Remote) || !bodyText.isEmpty()) {
        _server->searchMessages(newAction(), filter, bodyText, spec, sort);
    } else {
        // An action value is necessary, even if we're not communicating with the server
        newAction();

        // This search can be performed in the local process
        _matchingIds = QMailStore::instance()->queryMessages(filter, sort);
        setActivity(QMailServiceAction::InProgress);
        QTimer::singleShot(0, this, SLOT(finaliseSearch()));
    }
}

void QMailSearchActionPrivate::cancelOperation()
{
    if (_action != 0)
        _server->cancelSearch(_action);
}

void QMailSearchActionPrivate::init()
{
    QMailServiceActionPrivate::init();

    _matchingIds.clear();
}

void QMailSearchActionPrivate::matchingMessageIds(quint64 action, const QMailMessageIdList &ids)
{
    if (validAction(action)) {
        _matchingIds += ids;

        emit messageIdsMatched(ids);
    }
}

void QMailSearchActionPrivate::searchCompleted(quint64 action)
{
    if (validAction(action)) {
        setActivity(QMailServiceAction::Successful);
        emitChanges();
    }
}

void QMailSearchActionPrivate::finaliseSearch()
{
    emit messageIdsMatched(_matchingIds);

    setActivity(QMailServiceAction::Successful);
    emitChanges();
}


/*!
    \class QMailSearchAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailSearchAction class provides the interface for identifying messages that match a set of search criteria.

    QMailSearchAction provides the mechanism for messaging clients to request that the message
    server perform a search for messages that match the supplied search criteria.  For criteria
    pertaining to message meta data, the message server will search within the locally held
    meta data.  For a textual content criterion, the message server will search locally where
    the message content is held locally, and on the external server for messages whose content
    has not been retrieved (provided the external service provides a search facility).
*/

/*!
    \enum QMailSearchAction::SearchSpecification

    This enum type is specify the form of search that the message server should perform.

    \value Local    Only the message data stored on the local device should be searched.
    \value Remote   The search should be extended to message data stored on external servers.
*/

/*!
    \typedef QMailSearchAction::ImplementationType
    \internal
*/

/*!
    Constructs a new search action object with the supplied \a parent.
*/
QMailSearchAction::QMailSearchAction(QObject *parent)
    : QMailServiceAction(new QMailSearchActionPrivate(this), parent)
{
    connect(impl(this), SIGNAL(messageIdsMatched(QMailMessageIdList)), this, SIGNAL(messageIdsMatched(QMailMessageIdList)));
}

/*! \internal */
QMailSearchAction::~QMailSearchAction()
{
}

/*!
    Requests that the message server identify all messages that match the criteria
    specified by \a filter.  If \a bodyText is non-empty, identified messages must
    also contain the supplied text in their content.  

    If \a spec is \l{QMailSearchAction::Remote}{Remote}, then the external service 
    will be requested to perform the search for messages not stored locally. 

    If \a sort is not empty, the external service will return matching messages in 
    the ordering indicated by the sort criterion if possible.
*/
void QMailSearchAction::searchMessages(const QMailMessageKey &filter, const QString &bodyText, SearchSpecification spec, const QMailMessageSortKey &sort)
{
    impl(this)->searchMessages(filter, bodyText, spec, sort);
}

/*!
    Attempts to cancel the last requested search operation.
*/
void QMailSearchAction::cancelOperation()
{
    impl(this)->cancelOperation();
}

/*!
    Returns the list of message identifiers found to match the search criteria.
    Unless activity() returns \l Successful, an empty list is returned.
*/
QMailMessageIdList QMailSearchAction::matchingMessageIds() const
{
    return impl(this)->_matchingIds;
}

/*!
    \fn QMailSearchAction::messageIdsMatched(const QMailMessageIdList &ids)

    This signal is emitted when the messages in \a ids are discovered to match
    the criteria of the search in progress.

    \sa matchingMessageIds()
*/


QMailProtocolActionPrivate::QMailProtocolActionPrivate(QMailProtocolAction *i)
    : QMailServiceActionPrivate(this, i)
{
    connect(_server, SIGNAL(protocolResponse(quint64, QString, QVariant)),
            this, SLOT(protocolResponse(quint64, QString, QVariant)));
    connect(_server, SIGNAL(protocolRequestCompleted(quint64)),
            this, SLOT(protocolRequestCompleted(quint64)));

    init();
}

void QMailProtocolActionPrivate::protocolRequest(const QMailAccountId &accountId, const QString &request, const QVariant &data)
{
    _server->protocolRequest(newAction(), accountId, request, data);
}

void QMailProtocolActionPrivate::protocolResponse(quint64 action, const QString &response, const QVariant &data)
{
    if (validAction(action)) {
        emit protocolResponse(response, data);
    }
}

void QMailProtocolActionPrivate::protocolRequestCompleted(quint64 action)
{
    if (validAction(action)) {
        setActivity(QMailServiceAction::Successful);
        emitChanges();
    }
}


/*!
    \class QMailProtocolAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailProtocolAction class provides a mechanism for messageserver clients
    and services to collaborate without messageserver involvement.

    QMailProtocolAction provides a mechanism for messaging clients to request actions from
    message services that are not implemented by the messageserver.  If a client can
    determine that the service implementing a specific account supports a particular
    operation (by inspecting the output of QMailAccount::messageSources()), it may
    invoke that operation by passing appropriately formatted data to the service via the
    protocolRequest() slot.

    If the service is able to provide the requested service, and extract the necessary 
    data from the received input, it should perform the requested operation.  If any
    output is produced, it may be passed back to the client application via the 
    protocolResponse() signal.
*/

/*!
    \typedef QMailProtocolAction::ImplementationType
    \internal
*/

/*!
    Constructs a new protocol action object with the supplied \a parent.
*/
QMailProtocolAction::QMailProtocolAction(QObject *parent)
    : QMailServiceAction(new QMailProtocolActionPrivate(this), parent)
{
    connect(impl(this), SIGNAL(protocolResponse(QString, QVariant)), this, SIGNAL(protocolResponse(QString, QVariant)));
}

/*! \internal */
QMailProtocolAction::~QMailProtocolAction()
{
}

/*!
    Requests that the message server forward the protocol-specific request \a request
    to the QMailMessageSource configured for the account identified by \a accountId.
    The request may have associated \a data, in a protocol-specific form.
*/
void QMailProtocolAction::protocolRequest(const QMailAccountId &accountId, const QString &request, const QVariant &data)
{
    impl(this)->protocolRequest(accountId, request, data);
}

/*!
    \fn QMailProtocolAction::protocolResponse(const QString &response, const QVariant &data)

    This signal is emitted when the response \a response is emitted by the messageserver,
    with the associated \a data.
*/

