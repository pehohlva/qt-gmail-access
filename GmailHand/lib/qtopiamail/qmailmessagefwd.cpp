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

/*!
    \class QMailMessageHeaderFieldFwd
    \preliminary
    \brief The QMailMessageHeaderFieldFwd class declares enumerations used by QMailMessageHeaderField
   
    QMailMessageHeaderFieldFwd allows QMailMessageHeaderField::FieldType 
    to be used without including all of \c qmailmessage.h.
*/

/*!
    \enum QMailMessageHeaderFieldFwd::FieldType
    
    This enum type is used to describe the formatting of field content.

    \value StructuredField      The field content should be parsed assuming it is structured according to the specification for RFC 2045 'Content-Type' fields.
    \value UnstructuredField    The field content has no internal structure.
*/

/*!
    \class QMailMessageContentDispositionFwd
    \preliminary
    \brief The QMailMessageContentDispositionFwd class declares enumerations used by QMailMessageContentDisposition
   
    QMailMessageContentDispositionFwd allows QMailMessageContentDisposition::DispositionType 
    to be used without including all of \c qmailmessage.h.
*/

/*!
    \enum QMailMessageContentDispositionFwd::DispositionType
    
    This enum type is used to describe the disposition of a message part.

    \value Attachment   The part data should be presented as an attachment.
    \value Inline       The part data should be presented inline.
    \value None         The disposition of the part is unknown.
*/

/*!
    \class QMailMessageBodyFwd
    \preliminary
    \brief The QMailMessageBodyFwd class declares enumerations used by QMailMessageBody
   
    QMailMessageBodyFwd allows QMailMessageBody::TransferEncoding and QMailMessageBody::EncodingStatus
    to be used without including all of \c qmailmessage.h.
*/

/*!
    \enum QMailMessageBodyFwd::TransferEncoding
    
    This enum type is used to describe a type of binary to text encoding.
    Encoding types used here are documented in 
    \l {http://www.ietf.org/rfc/rfc2045.txt}{RFC 2045} "Format of Internet Message Bodies"

    \value NoEncoding          The encoding is not specified.
    \value SevenBit            The data is not encoded, but contains only 7-bit ASCII data.
    \value EightBit            The data is not encoded, but contains data using only 8-bit characters which form a superset of ASCII.
    \value Base64              A 65-character subset of US-ASCII is used, enabling 6 bits to be represented per printable character.
    \value QuotedPrintable     A method of encoding that tends to leave text similar to US-ASCII unmodified for readability.
    \value Binary              The data is not encoded to any limited subset of octet values.

    \sa QMailCodec
*/

/*!
    \enum QMailMessageBodyFwd::EncodingStatus
    
    This enum type is used to describe the encoding status of body data.

    \value AlreadyEncoded       The body data is already encoded to the necessary encoding.
    \value RequiresEncoding     The body data is unencoded, and thus requires encoding for transmission.
*/

/*!
    \enum QMailMessageBodyFwd::EncodingFormat
    
    This enum type is used to describe the format in which body data should be presented.

    \value Encoded      The body data should be presented in encoded form. 
    \value Decoded      The body data should be presented in unencoded form. 
*/

/*!
    \class QMailMessagePartContainerFwd
    \preliminary
    \brief The QMailMessagePartContainerFwd class declares enumerations used by QMailMessagePartContainer
   
    QMailMessagePartContainerFwd allows QMailMessagePartContainerFwd::MultipartType
    to be used without including all of \c qmailmessage.h.
*/

/*!
    \enum QMailMessagePartContainerFwd::MultipartType

    This enumerated type is used to describe the multipart encoding of a message or message part.

    \value MultipartNone        The container does not hold parts.
    \value MultipartSigned      The container holds parts encoded according to \l {http://www.ietf.org/rfc/rfc1847.txt}{RFC 1847} "multipart/signed"
    \value MultipartEncrypted   The container holds parts encoded according to \l {http://www.ietf.org/rfc/rfc1847.txt}{RFC 1847} "multipart/encrypted"
    \value MultipartMixed       The container holds parts encoded according to \l {http://www.ietf.org/rfc/rfc2046.txt}{RFC 2046} "multipart/mixed"
    \value MultipartAlternative The container holds parts encoded according to \l {http://www.ietf.org/rfc/rfc2046.txt}{RFC 2046} "multipart/alternative"
    \value MultipartDigest      The container holds parts encoded according to \l {http://www.ietf.org/rfc/rfc2046.txt}{RFC 2046} "multipart/digest"
    \value MultipartParallel    The container holds parts encoded according to \l {http://www.ietf.org/rfc/rfc2046.txt}{RFC 2046} "multipart/parallel"
    \value MultipartRelated     The container holds parts encoded according to \l {http://www.ietf.org/rfc/rfc2387.txt}{RFC 2387} "multipart/related"
    \value MultipartFormData    The container holds parts encoded according to \l {http://www.ietf.org/rfc/rfc2388.txt}{RFC 2388} "multipart/form-data"
    \value MultipartReport      The container holds parts encoded according to \l {http://www.ietf.org/rfc/rfc3462.txt}{RFC 3462} "multipart/report"
*/

/*!
    \class QMailMessagePartFwd 
    \preliminary
    \brief The QMailMessagePartFwd class declares enumerations used by QMailMessagePart
   
    QMailMessagePartFwd allows QMailMessagePartFwd::ReferenceType
    to be used without including all of \c qmailmessage.h.
*/

/*!
    \enum QMailMessagePartFwd::ReferenceType

    This enumerated type is used to describe the type of reference that a part constitutes.

    \value None                 The part is not a reference.
    \value MessageReference     The part is a reference to a message.
    \value PartReference        The part is a reference to another part.
*/

/*!
    \class QMailMessageMetaDataFwd
    \preliminary
    \brief The QMailMessageMetaDataFwd class declares enumerations used by QMailMessageMetaData
   
    QMailMessageMetaDataFwd allows QMailMessageMetaData::MessageType, QMailMessageMetaData::ContentType and QMailMessageMetaData::ResponseType
    to be used without including all of \c qmailmessage.h.
*/

/*!
    \enum QMailMessageMetaDataFwd::MessageType

    This enum type is used to describe the type of a message.

    \value Mms       The message is an MMS.
    \value Sms       The message is an SMS.
    \value Email     The message is an Email.
    \value Instant   The message is an instant message.
    \value System    The message is a system report.
    \value None      Indicates no message type.
    \value AnyType   Indicates any type of message.
*/

/*!
    \enum QMailMessageMetaDataFwd::ContentType

    This enum type is used to describe the type of data contained within a message.

    \value UnknownContent The content of the message has not been specified.
    \value NoContent The message does not contain content and is completely described by its meta data.
    \value PlainTextContent Plain text content.
    \value RichTextContent Text content described via QTextBrowser rich text markup.
    \value HtmlContent Content marked up via HyperText Markup Language.
    \value ImageContent Image content.
    \value AudioContent Audio content.
    \value VideoContent Video content.
    \value MultipartContent Content consisting of multiple individual parts related according to RFC 2046.
    \value SmilContent Dynamic content described via Synchronized Multimedia Integration Language.
    \value VoicemailContent Content that should be presented as a recorded audio message from a contact.
    \value VideomailContent Content that should be presented as a recorded video message from a contact.
    \value VCardContent A contact description, as defined by RFC 2425.
    \value VCalendarContent A scheduling element description as defined by the vCalendar 1.0 specification.
    \value ICalendarContent A scheduling element description as defined by RFC 2445.
    \value DeliveryReportContent A message delivery report.
    \value UserContent The first value that can be used for application-specific purposes.
*/

/*!
    \enum QMailMessageMetaDataFwd::ResponseType

    This enum type is used to describe the type of response that a message is created as.

    \value NoResponse   The message was not created as a response to another message.
    \value Reply        The message was created as a reply to the sender of another message.
    \value ReplyToAll   The message was created in reply to all recipients another message.
    \value Forward      The message was created to forward the content of another message.
    \value ForwardPart  The message was created to forward part of the content of another message.
    \value Redirect     The message was created to redirect another message to a different address.
*/

/*!
    \class QMailMessageFwd
    \preliminary
    \brief The QMailMessageFwd class declares enumerations used by QMailMessage
   
    QMailMessageFwd allows QMailMessage::AttachmentsAction and QMailMessage::EncodingFormat
    to be used without including all of \c qmailmessage.h.
*/

/*!
    \enum QMailMessageFwd::AttachmentsAction

    This enum type is used to describe the action that should be performed on 
    each message attachment.

    \value LinkToAttachments        Add a part to the message containing a link to the 
                                    supplied attachment. If the document is removed, the 
                                    message will no longer have access to the data.
    \value CopyAttachments          Add a part to the message containing a copy of the
                                    data in the supplied attachment. If the document is 
                                    removed, the message will still contain the data.
    \value CopyAndDeleteAttachments Add a part to the message containing a copy of the
                                    data in the supplied attachment, then delete the
                                    document from which the data was copied.
*/

/*!
    \enum QMailMessageFwd::EncodingFormat
    
    This enum type is used to describe the format in which a message should be serialized.

    \value HeaderOnlyFormat     Only the header portion of the message is serialized, to RFC 2822 form.
    \value StorageFormat        The message is serialized to RFC 2822 form, without attachments.
    \value TransmissionFormat   The entire message is serialized to RFC 2822 form, with additional header fields added if necessary, and 'bcc' header field omitted.
    \value IdentityFormat       The entire message is serialized to RFC 2822 form, with only Content-Type and Content-Transfer-Encoding headers added where required.
*/

/*!
    \enum QMailMessageFwd::ChunkType
    
    This enum type is used to denote the content of a single chunk in a partitioned output sequence.

    \value Text         The chunk contains verbatim output text.
    \value Reference    The chunk contains a reference to an external datum.
*/

/*!
    \typedef QMailMessageFwd::MessageChunk

    This type defines a single chunk in a sequence of partitioned ouput data.
*/

