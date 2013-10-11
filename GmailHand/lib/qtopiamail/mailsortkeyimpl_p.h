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

#ifndef MAILSORTKEYIMPL_H
#define MAILSORTKEYIMPL_H

#include "qmailglobal.h"
#include <QSharedData>
#include <QtGlobal>
#include <QPair>


template<typename Key>
class MailSortKeyImpl : public QSharedData
{
public:
    typedef Key KeyType;
    typedef typename Key::Property Property;
    typedef typename Key::ArgumentType Argument;

    MailSortKeyImpl();
    MailSortKeyImpl(Property p, Qt::SortOrder order, quint64 mask);
    MailSortKeyImpl(const QList<Argument> &args);

    bool operator==(const MailSortKeyImpl& other) const;
    bool operator!=(const MailSortKeyImpl& other) const;

    MailSortKeyImpl& operator=(const MailSortKeyImpl& other);

    bool isEmpty() const;

    const QList<Argument> &arguments() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    QList<Argument> _arguments;
};


template<typename Key>
MailSortKeyImpl<Key>::MailSortKeyImpl()
    : QSharedData()
{
}

template<typename Key>
MailSortKeyImpl<Key>::MailSortKeyImpl(Property p, Qt::SortOrder order, quint64 mask)
    : QSharedData()
{
    _arguments.append(Argument(p, order, mask));
}

template<typename Key>
MailSortKeyImpl<Key>::MailSortKeyImpl(const QList<Argument> &args)
    : QSharedData()
{
    _arguments = args;
}

template<typename Key>
bool MailSortKeyImpl<Key>::operator==(const MailSortKeyImpl& other) const
{
    return _arguments == other._arguments;
}

template<typename Key>
bool MailSortKeyImpl<Key>::operator!=(const MailSortKeyImpl& other) const
{
   return !(*this == other); 
}

template<typename Key>
bool MailSortKeyImpl<Key>::isEmpty() const
{
    return _arguments.isEmpty();
}

template<typename Key>
const QList<typename MailSortKeyImpl<Key>::Argument> &MailSortKeyImpl<Key>::arguments() const
{
    return _arguments;
}

template<typename Key>
template <typename Stream> 
void MailSortKeyImpl<Key>::serialize(Stream &stream) const
{
    stream << _arguments.count();
    foreach (const Argument& a, _arguments) {
        a.serialize(stream);
    }
}

template<typename Key>
template <typename Stream> 
void MailSortKeyImpl<Key>::deserialize(Stream &stream)
{
    int i = 0;
    stream >> i;
    for (int j = 0; j < i; ++j) {
        Argument a;
        a.deserialize(stream);
        _arguments.append(a);
    }
}

#endif

