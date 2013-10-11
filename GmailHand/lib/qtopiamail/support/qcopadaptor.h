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

#ifndef QCOPADAPTOR_H
#define QCOPADAPTOR_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>

#if !defined(Q_QCOP_EXPORT)
#if defined(QT_BUILD_QCOP_LIB)
#define Q_QCOP_EXPORT Q_DECL_EXPORT
#else
#define Q_QCOP_EXPORT Q_DECL_IMPORT
#endif
#endif

class QCopAdaptorPrivate;
class QCopAdaptorEnvelopePrivate;

class Q_QCOP_EXPORT QCopAdaptorEnvelope
{
    friend class QCopAdaptor;
private:
    QCopAdaptorEnvelope(const QStringList& channels, const QString& message);

public:
    QCopAdaptorEnvelope();
    QCopAdaptorEnvelope(const QCopAdaptorEnvelope& value);
    ~QCopAdaptorEnvelope();

    QCopAdaptorEnvelope& operator=(const QCopAdaptorEnvelope& value);
    template <class T>
    QCopAdaptorEnvelope& operator<<(const T &value);

    inline QCopAdaptorEnvelope& operator<<(const char *value)
    {
        addArgument(QVariant(QString(value)));
        return *this;
    }

private:
    QCopAdaptorEnvelopePrivate *d;

    void addArgument(const QVariant& value);
};

class Q_QCOP_EXPORT QCopAdaptor : public QObject
{
    Q_OBJECT
    friend class QCopAdaptorPrivate;
    friend class QCopAdaptorEnvelope;
    friend class QCopAdaptorChannel;
public:
    explicit QCopAdaptor(const QString& channel, QObject *parent = 0);
    ~QCopAdaptor();

    QString channel() const;

    static bool connect(QObject *sender, const QByteArray& signal,
                        QObject *receiver, const QByteArray& member);

    QCopAdaptorEnvelope send(const QByteArray& member);
    void send(const QByteArray& member, const QVariant &arg1);
    void send(const QByteArray& member, const QVariant &arg1,
              const QVariant &arg2);
    void send(const QByteArray& member, const QVariant &arg1,
              const QVariant &arg2, const QVariant &arg3);
    void send(const QByteArray& member, const QList<QVariant>& args);

    bool isConnected(const QByteArray& signal);

protected:
    enum PublishType
    {
        Signals,
        Slots,
        SignalsAndSlots
    };

    bool publish(const QByteArray& member);
    void publishAll(QCopAdaptor::PublishType type);
    virtual QString memberToMessage(const QByteArray& member);
    virtual QStringList sendChannels(const QString& channel);
    virtual QString receiveChannel(const QString& channel);

private slots:
    void received(const QString& msg, const QByteArray& data);
    void receiverDestroyed();

private:
    QCopAdaptorPrivate *d;

    bool connectLocalToRemote(QObject *sender, const QByteArray& signal,
                              const QByteArray& member);
    bool connectRemoteToLocal(const QByteArray& signal, QObject *receiver,
                              const QByteArray& member);
    void sendMessage(const QString& msg, const QList<QVariant>& args);
    static void send(const QStringList& channels,
                     const QString& msg, const QList<QVariant>& args);
};

template<class T>
QCopAdaptorEnvelope& QCopAdaptorEnvelope::operator<<(const T &value)
{
    addArgument(qVariantFromValue(value));
    return *this;
}

// Useful alias to make it clearer when connecting to messages on a channel.
#define MESSAGE(x)      "3"#x
#define QMESSAGE_CODE   3

#endif
