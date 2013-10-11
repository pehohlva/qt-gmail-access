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

#include "qmailmessage_p.h"
#include "qmailmessageset_p.h"
#include "qmailserviceaction_p.h"
#include "qprivateimplementationdef.h"

template class QPrivateImplementationPointer<QMailMessageHeaderFieldPrivate>;
template class QPrivatelyImplemented<QMailMessageHeaderFieldPrivate>;
template class QPrivateImplementationPointer<QMailMessageHeaderPrivate>;
template class QPrivatelyImplemented<QMailMessageHeaderPrivate>;
template class QPrivateImplementationPointer<QMailMessageBodyPrivate>;
template class QPrivatelyImplemented<QMailMessageBodyPrivate>;
template class QPrivateImplementationPointer<QMailMessagePartContainerPrivate>;
template class QPrivatelyImplemented<QMailMessagePartContainerPrivate>;
template class QPrivateImplementationPointer<QMailMessageMetaDataPrivate>;
template class QPrivatelyImplemented<QMailMessageMetaDataPrivate>;

template class QPrivateNoncopyablePointer<QMailMessageSetContainerPrivate>;
template class QPrivatelyNoncopyable<QMailMessageSetContainerPrivate>;

template class QPrivateNoncopyablePointer<QMailServiceActionPrivate>;
template class QPrivatelyNoncopyable<QMailServiceActionPrivate>;
template class QPrivateNoncopyablePointer<QMailRetrievalActionPrivate>;
template class QPrivatelyNoncopyable<QMailRetrievalActionPrivate>;
template class QPrivateNoncopyablePointer<QMailTransmitActionPrivate>;
template class QPrivatelyNoncopyable<QMailTransmitActionPrivate>;
template class QPrivateNoncopyablePointer<QMailStorageActionPrivate>;
template class QPrivatelyNoncopyable<QMailStorageActionPrivate>;
template class QPrivateNoncopyablePointer<QMailSearchActionPrivate>;
template class QPrivatelyNoncopyable<QMailSearchActionPrivate>;
template class QPrivateNoncopyablePointer<QMailProtocolActionPrivate>;
template class QPrivatelyNoncopyable<QMailProtocolActionPrivate>;

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessageBody::TransferEncoding)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessagePartContainer::MultipartType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::MessageType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::ContentType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::ResponseType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::AttachmentsAction)

Q_IMPLEMENT_USER_METATYPE(QMailMessage)
Q_IMPLEMENT_USER_METATYPE(QMailMessageMetaData)
Q_IMPLEMENT_USER_METATYPE(QMailMessagePart::Location)

Q_IMPLEMENT_USER_METATYPE_TYPEDEF(QMailMessageList, QMailMessageList)
Q_IMPLEMENT_USER_METATYPE_TYPEDEF(QMailMessageMetaDataList, QMailMessageMetaDataList)
Q_IMPLEMENT_USER_METATYPE_TYPEDEF(QMailMessageTypeList, QMailMessageTypeList)

Q_IMPLEMENT_USER_METATYPE(QMailServiceAction::Status)

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailServiceAction::Connectivity)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailServiceAction::Activity)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailServiceAction::Status::ErrorCode)

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailRetrievalAction::RetrievalSpecification)

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailSearchAction::SearchSpecification)

