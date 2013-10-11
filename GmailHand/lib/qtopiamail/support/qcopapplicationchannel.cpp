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

#include "qcopapplicationchannel.h"
#include "qcopchannel_p.h"
#include <QtCore/qcoreapplication.h>

/* ! - documentation comments in this file are disabled:
    \class QCopApplicationChannel
    \inpublicgroup QtBaseModule
    \ingroup qws
    \brief The QCopApplicationChannel class provides access to QCop messages that were specifically sent to this application.

    \sa QCopChannel, QCopServer
*/

// Get the name of the application-specific channel, based on the pid.
static QString applicationChannelName()
{
    return QLatin1String("QPE/Pid/") +
           QString::number(QCoreApplication::applicationPid());
}

/* !
    Constructs a new QCop channel for the application's private channel with
    parent object \a parent
*/
QCopApplicationChannel::QCopApplicationChannel(QObject *parent)
    : QCopChannel(applicationChannelName(), parent)
{
    d = 0;

    QCopThreadData *td = QCopThreadData::instance();
    connect(td->clientConnection(), SIGNAL(startupComplete()),
            this, SIGNAL(startupComplete()));
}

/* !
    Destroys this QCopApplicationChannel object.
*/
QCopApplicationChannel::~QCopApplicationChannel()
{
}

/* !
    Returns true if application channel startup has completed and the
    startupComplete() signal has already been issued.

    \sa startupComplete()
*/
bool QCopApplicationChannel::isStartupComplete() const
{
    QCopThreadData *td = QCopThreadData::instance();
    return td->clientConnection()->isStartupComplete;
}

/* !
    \fn void QCopApplicationChannel::startupComplete()

    This signal is emitted once the QCop server has forwarded all queued
    messages for the application channel at startup time.

    If the application is running in background mode and does not have a
    user interface, it may elect to exit once all of the queued messages
    have been received and processed.

    \sa isStartupComplete()
*/
