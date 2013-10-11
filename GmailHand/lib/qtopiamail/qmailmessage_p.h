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

#ifndef QMAILMESSAGE_P_H
#define QMAILMESSAGE_P_H

#include "qmailmessage.h"
#include "longstring_p.h"


// These classes are implemented via qmailmessage.cpp and qmailinstantiations.cpp

class QMailMessageHeaderFieldPrivate : public QPrivateImplementationBase
{
public:
    QMailMessageHeaderFieldPrivate();
    QMailMessageHeaderFieldPrivate(const QByteArray& text, bool structured);
    QMailMessageHeaderFieldPrivate(const QByteArray& name, const QByteArray& text, bool structured);

    bool operator== (const QMailMessageHeaderFieldPrivate& other) const;

    void addParameter(const QByteArray& name, const QByteArray& value);
    void parse(const QByteArray& text, bool structured);

    bool isNull() const;

    QByteArray id() const;
    void setId(const QByteArray& text);

    QByteArray content() const;
    void setContent(const QByteArray& text);

    QByteArray parameter(const QByteArray& name) const;
    void setParameter(const QByteArray& name, const QByteArray& value);

    bool isParameterEncoded(const QByteArray& name) const;
    void setParameterEncoded(const QByteArray& name);

    QList<QMailMessageHeaderField::ParameterType> parameters() const;

    QByteArray toString(bool includeName = true, bool presentable = true) const;

    void output(QDataStream& out) const;

    QString decodedContent() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    QByteArray _id;
    QByteArray _content;
    bool _structured;
    QList<QMailMessageHeaderField::ParameterType> _parameters;
};


class QMailMessageHeaderPrivate : public QPrivateImplementationBase
{
public:
    QMailMessageHeaderPrivate();
    QMailMessageHeaderPrivate(const QByteArray& input);

    void update(const QByteArray &id, const QByteArray &content);
    void append(const QByteArray &id, const QByteArray &content);
    void remove(const QByteArray &id);

    QList<QMailMessageHeaderField> fields(const QByteArray& id, int maximum = -1) const;

    void output(QDataStream& out, const QList<QByteArray>& exclusions, bool excludeInternalFields) const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    friend class QMailMessageHeader;

    QList<QByteArray> _headerFields;
};


class QMailMessageBodyPrivate : public QPrivateImplementationBase
{
public:
    QMailMessageBodyPrivate();

    void fromLongString(LongString& ls, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding, QMailMessageBody::EncodingStatus status);
    void fromFile(const QString& filename, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding, QMailMessageBody::EncodingStatus status);
    void fromStream(QDataStream& in, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding, QMailMessageBody::EncodingStatus status);
    void fromStream(QTextStream& in, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);

    bool toFile(const QString& filename, QMailMessageBody::EncodingFormat format) const;
    bool toStream(QDataStream& out, QMailMessageBody::EncodingFormat format) const;
    bool toStream(QTextStream& out) const;

    QMailMessageBody::TransferEncoding transferEncoding() const;
    QMailMessageContentType contentType() const;

    bool isEmpty() const;
    int length() const;

    uint indicativeSize() const;

    void output(QDataStream& out, bool includeAttachments) const;

    // We will express the indicative size of the body in units of this:
    static const uint IndicativeSizeUnit = 2048;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    friend class QMailMessageBody;

    QMailMessageBody::TransferEncoding _encoding;
    LongString _bodyData;
    QString _filename;
    bool _encoded;
    QMailMessageContentType _type;
};


class QMailMessagePartContainerPrivate : public QPrivateImplementationBase
{
public:
    template<typename Derived>
    QMailMessagePartContainerPrivate(Derived* p);

    void setLocation(const QMailMessageId& id, const QList<uint>& indices);
    int partNumber() const;

    const QMailMessagePartContainerPrivate* parentContainerPrivate() const;

    const QMailMessagePart& partAt(const QMailMessagePart::Location& location) const;
    QMailMessagePart& partAt(const QMailMessagePart::Location& location);

    void setHeader(const QMailMessageHeader& header, const QMailMessagePartContainerPrivate* parent = 0);

    QByteArray headerField( const QByteArray &headerName ) const;
    QList<QByteArray> headerFields( const QByteArray &headerName, int maximum = 0 ) const;

    QList<QByteArray> headerFields() const;

    void updateHeaderField(const QByteArray &id, const QByteArray &content);
    void updateHeaderField(const QByteArray &id, const QString &content);

    void appendHeaderField(const QByteArray &id, const QByteArray &content);
    void appendHeaderField(const QByteArray &id, const QString &content);

    void removeHeaderField(const QByteArray &id);

    void setMultipartType(QMailMessagePartContainer::MultipartType type);
    void appendPart(const QMailMessagePart &part);
    void prependPart(const QMailMessagePart &part);
    void removePartAt(uint pos);
    void clear();

    QMailMessageContentType contentType() const;
    QMailMessageBody::TransferEncoding transferEncoding() const;

    uint indicativeSize() const;

protected:
    friend class QMailMessagePartContainer;
    friend class QMailMessagePart;
    friend class QMailMessagePart::Location;
    friend class QMailMessagePrivate;

    void defaultContentType(const QMailMessagePartContainerPrivate* parent);

    template <typename F>
    void outputParts(QDataStream **out, bool addMimePreamble, bool includeAttachments, bool excludeInternalFields, F *func) const;

    void outputBody(QDataStream& out, bool includeAttachments) const;

    QString headerFieldText( const QString &id ) const;

    QByteArray boundary() const;
    void setBoundary(const QByteArray& text);

    // Note: this returns a reference:
    QMailMessageBody& body();
    const QMailMessageBody& body() const;
    void setBody(const QMailMessageBody& body);
    void setBodyProperties(const QMailMessageContentType &type, QMailMessageBody::TransferEncoding encoding);

    bool hasBody() const;

    void parseMimeSinglePart(const QMailMessageHeader& partHeader, LongString body);
    void parseMimeMultipart(const QMailMessageHeader& partHeader, LongString body, bool insertIntoSelf);
    bool parseMimePart(LongString body);

    static QMailMessagePartContainerPrivate* privatePointer(QMailMessagePart& part);

    bool dirty(bool recursive = false) const;
    void setDirty(bool value = true, bool recursive = false);

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    QMailMessagePartContainer::MultipartType _multipartType;
    QList<QMailMessagePart> _messageParts;
    mutable QByteArray _boundary;
    QMailMessageHeader _header;
    QMailMessageBody _body;
    QMailMessageId _messageId;
    QList<uint> _indices;
    bool _hasBody;
    bool _dirty;
};


class QMailMessagePartPrivate : public QMailMessagePartContainerPrivate
{
public:
    QMailMessagePartPrivate();

    QMailMessagePart::ReferenceType referenceType() const;

    QMailMessageId messageReference() const;
    QMailMessagePart::Location partReference() const;

    QString referenceResolution() const;
    void setReferenceResolution(const QString &uri);

    bool contentAvailable() const;
    bool partialContentAvailable() const;

    template <typename F>
    void output(QDataStream **out, bool addMimePreamble, bool includeAttachments, bool excludeInternalFields, F *func) const;

    bool contentModified() const;
    void setUnmodified();

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    friend class QMailMessagePart;

    void setReference(const QMailMessageId &id, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);
    void setReference(const QMailMessagePart::Location &location, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);

    QMailMessageId _referenceId;
    QMailMessagePart::Location _referenceLocation;
    QString _resolution;
};


class QMailMessageMetaDataPrivate : public QPrivateImplementationBase
{
public:
    QMailMessageMetaDataPrivate();

    static void initializeFlags();

    void setMessageType(QMailMessage::MessageType type);
    void setParentFolderId(const QMailFolderId& id);
    void setPreviousParentFolderId(const QMailFolderId& id);
    void setId(const QMailMessageId& id);
    void setStatus(quint64 status);
    void setParentAccountId(const QMailAccountId& id);
    void setServerUid(const QString &server);
    void setSize(uint size);
    void setContent(QMailMessage::ContentType type);

    void setSubject(const QString& s);
    void setDate(const QMailTimeStamp& timeStamp);
    void setReceivedDate(const QMailTimeStamp& timeStamp);
    void setFrom(const QString& s);
    void setTo(const QString& s);

    void setContentScheme(const QString& scheme);
    void setContentIdentifier(const QString& identifier);

    void setInResponseTo(const QMailMessageId &id);
    void setResponseType(QMailMessage::ResponseType type);

    uint indicativeSize() const;

    bool dataModified() const;
    void setUnmodified();

    QString customField(const QString &name) const;
    void setCustomField(const QString &name, const QString &value);
    void removeCustomField(const QString &name);
    void setCustomFields(const QMap<QString, QString> &fields);

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    QMailMessage::MessageType _messageType;
    quint64 _status;
    QMailMessage::ContentType _contentType;

    QMailAccountId _parentAccountId;
    QString _serverUid;
    uint _size;
    QMailMessageId _id;
    QMailFolderId _parentFolderId;
    QMailFolderId _previousParentFolderId;

    QString _subject;
    QMailTimeStamp _date;
    QMailTimeStamp _receivedDate;
    QString _from;
    QString _to;

    QString _contentScheme;
    QString _contentIdentifier;

    QMailMessageId _responseId;
    QMailMessage::ResponseType _responseType;

    QMap<QString, QString> _customFields;
    bool _customFieldsModified;

    template <typename T>
    void updateMember(T& value, const T& newValue)
    {
        if (value != newValue) {
            value = newValue;
            _dirty = true;
        }
    }

    bool _dirty;

private:
    static quint64 registerFlag(const QString &name);
};


class QMailMessagePrivate : public QMailMessagePartContainerPrivate
{
public:
    QMailMessagePrivate();

    void fromRfc2822(const LongString &ls);

    template <typename F>
    void toRfc2822(QDataStream **out, QMailMessage::EncodingFormat format, quint64 messageStatus, F *func) const;

    void setMessageType(QMailMessage::MessageType type);

    void setId(const QMailMessageId& id);
    void setSubject(const QString& s);
    void setDate(const QMailTimeStamp& timeStamp);
    void setFrom(const QString& from);
    void setReplyTo(const QString& s);
    void setInReplyTo(const QString& s);

    void setTo(const QString& s);
    void setCc(const QString& s);
    void setBcc(const QString& s);

    bool hasRecipients() const;

    uint indicativeSize() const;

    bool contentModified() const;
    void setUnmodified();

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    void outputHeaders(QDataStream& out, bool addTimeStamp, bool addContentHeaders, bool includeBcc, bool excludeInternalFields) const;
};

#endif

