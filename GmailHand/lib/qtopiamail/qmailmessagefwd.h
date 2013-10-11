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
#ifndef QMAILMESSAGEFWD_H
#define QMAILMESSAGEFWD_H

#include "qmailglobal.h"
#include <QPair>

class QMailMessageHeaderField;

class QTOPIAMAIL_EXPORT QMailMessageHeaderFieldFwd
{
public:
    enum FieldType
    {
        StructuredField = 1,
        UnstructuredField = 2
    };
};

class QMailMessageContentDisposition;

class QTOPIAMAIL_EXPORT QMailMessageContentDispositionFwd
{
public:
    enum DispositionType
    {
        None = 0,
        Inline = 1,
        Attachment = 2
    };
};

class QMailMessageBody;

class QTOPIAMAIL_EXPORT QMailMessageBodyFwd
{
public:
    enum TransferEncoding 
    {
        NoEncoding = 0,
        SevenBit = 1, 
        EightBit = 2, 
        Base64 = 3,
        QuotedPrintable = 4,
        Binary = 5, 
    };

    enum EncodingStatus
    {
        AlreadyEncoded = 1,
        RequiresEncoding = 2
    };

    enum EncodingFormat
    {
        Encoded = 1,
        Decoded = 2
    };
};

class QMailMessagePartContainer;

class QTOPIAMAIL_EXPORT QMailMessagePartContainerFwd
{
public:
    enum MultipartType 
    {
        MultipartNone = 0,
        MultipartSigned = 1,
        MultipartEncrypted = 2,
        MultipartMixed = 3,
        MultipartAlternative = 4,
        MultipartDigest = 5,
        MultipartParallel = 6,
        MultipartRelated = 7,
        MultipartFormData = 8,
        MultipartReport = 9
    };
};

class QMailMessagePart;

class QTOPIAMAIL_EXPORT QMailMessagePartFwd
{
public:
    enum ReferenceType {
        None = 0,
        MessageReference,
        PartReference
    };
};
        
class QMailMessageMetaData;

class QTOPIAMAIL_EXPORT QMailMessageMetaDataFwd
{
public:
    enum MessageType
    {
        Mms     = 0x1,
        // was: Ems = 0x2
        Sms     = 0x4,
        Email   = 0x8,
        System  = 0x10,
        Instant = 0x20,
        None    = 0,
        AnyType = Mms | Sms | Email | System | Instant
    };

    enum ContentType {
        UnknownContent        = 0,
        NoContent             = 1,
        PlainTextContent      = 2,
        RichTextContent       = 3,
        HtmlContent           = 4,
        ImageContent          = 5,
        AudioContent          = 6,
        VideoContent          = 7,
        MultipartContent      = 8,
        SmilContent           = 9,
        VoicemailContent      = 10,
        VideomailContent      = 11,
        VCardContent          = 12,
        VCalendarContent      = 13,
        ICalendarContent      = 14,
        DeliveryReportContent = 15,
        UserContent           = 64
    };

    enum ResponseType {
        NoResponse          = 0,
        Reply               = 1, 
        ReplyToAll          = 2,
        Forward             = 3,
        ForwardPart         = 4,
        Redirect            = 5
    };
};

class QMailMessage;

class QTOPIAMAIL_EXPORT QMailMessageFwd
{
public:
    enum AttachmentsAction {
        LinkToAttachments = 0,
        CopyAttachments,
        CopyAndDeleteAttachments
    };

    enum EncodingFormat {
        HeaderOnlyFormat = 1,
        StorageFormat = 2,
        TransmissionFormat = 3,
        IdentityFormat = 4,
    }; 

    enum ChunkType {
        Text = 0,
        Reference
    };

    typedef QPair<ChunkType, QByteArray> MessageChunk;
};

#endif

