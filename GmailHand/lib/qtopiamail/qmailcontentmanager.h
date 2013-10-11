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

#ifndef QMAILCONTENTMANAGER_H
#define QMAILCONTENTMANAGER_H

#include "qmailglobal.h"
#include "qmailstore.h"
#include <qfactoryinterface.h>
#include <QMap>
#include <QString>

class QMailMessage;
class QMailContentManager;


class QTOPIAMAIL_EXPORT QMailContentManagerFactory
{
public:
    static QStringList schemes();
    static QString defaultScheme();

    static QMailContentManager *create(const QString &scheme);

    static bool init();
    static void clearContent();
};


struct QTOPIAMAIL_EXPORT QMailContentManagerPluginInterface : public QFactoryInterface
{
    virtual QString key() const = 0;
    virtual QMailContentManager *create() = 0;
};


#define QMailContentManagerPluginInterface_iid "com.trolltech.Qtopia.Qtopiamail.QMailContentManagerPluginInterface"
Q_DECLARE_INTERFACE(QMailContentManagerPluginInterface, QMailContentManagerPluginInterface_iid)


class QTOPIAMAIL_EXPORT QMailContentManagerPlugin : public QObject, public QMailContentManagerPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QMailContentManagerPluginInterface:QFactoryInterface)

public:
    QMailContentManagerPlugin();
    ~QMailContentManagerPlugin();

    virtual QStringList keys() const;
};


class QTOPIAMAIL_EXPORT QMailContentManager
{
    friend class QMailManagerContentFactory;

protected:
    QMailContentManager();
    virtual ~QMailContentManager();

public:
    enum DurabilityRequirement { 
        EnsureDurability = 0,
        DeferDurability
    };

    virtual QMailStore::ErrorCode add(QMailMessage *message, DurabilityRequirement durability) = 0;
    virtual QMailStore::ErrorCode update(QMailMessage *message, DurabilityRequirement durability) = 0;

    virtual QMailStore::ErrorCode ensureDurability() = 0;

    virtual QMailStore::ErrorCode remove(const QString &identifier) = 0;
    virtual QMailStore::ErrorCode load(const QString &identifier, QMailMessage *message) = 0;

    virtual bool init();
    virtual void clearContent();
};

#endif

