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

#ifndef QMAILIPC_H
#define QMAILIPC_H

#include "qcopadaptor.h"
#include "qcopchannel.h"
#include <QDataStream>
#include "qcopserver.h"

template <typename T>
struct QMetaTypeRegister
{
    static int registerType() { return 1; }
};

#ifdef Q_CC_GNU
# define _QATOMIC_ONCE() do {} while(0)
#else
# define _QATOMIC_ONCE()                \
    static QAtomicInt once;             \
    if ( once.fetchAndStoreOrdered(1) ) \
        return 1
#endif

#define Q_DECLARE_USER_METATYPE_NO_OPERATORS(TYPE) \
    Q_DECLARE_METATYPE(TYPE) \
    template<> \
    struct QMetaTypeRegister< TYPE > \
    { \
        static int registerType() \
        { \
            _QATOMIC_ONCE(); \
            int id = qMetaTypeId( reinterpret_cast<TYPE *>(0) ); \
            if ( id >= static_cast<int>(QMetaType::User) ) \
                qRegisterMetaTypeStreamOperators< TYPE >( #TYPE ); \
            return 1; \
        } \
        static int __init_variable__; \
    };

#define Q_DECLARE_USER_METATYPE(TYPE) \
    Q_DECLARE_USER_METATYPE_NO_OPERATORS(TYPE) \
    QTOPIAMAIL_EXPORT QDataStream &operator<<(QDataStream &stream, const TYPE &var); \
    QTOPIAMAIL_EXPORT QDataStream &operator>>( QDataStream &stream, TYPE &var );

#define Q_DECLARE_USER_METATYPE_TYPEDEF(TAG,TYPE)       \
    template <typename T> \
    struct QMetaTypeRegister##TAG \
    { \
        static int registerType() { return 1; } \
    }; \
    template<> struct QMetaTypeRegister##TAG< TYPE > { \
        static int registerType() { \
            _QATOMIC_ONCE(); \
            qRegisterMetaType< TYPE >( #TYPE ); \
            qRegisterMetaTypeStreamOperators< TYPE >( #TYPE ); \
            return 1; \
        } \
        static int __init_variable__; \
    };

#define Q_DECLARE_USER_METATYPE_ENUM(TYPE)      \
    Q_DECLARE_USER_METATYPE(TYPE)

#define Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(TYPE) \
    int QMetaTypeRegister< TYPE >::__init_variable__ = \
        QMetaTypeRegister< TYPE >::registerType();

#define Q_IMPLEMENT_USER_METATYPE(TYPE) \
    QDataStream &operator<<(QDataStream &stream, const TYPE &var) \
    { \
        var.serialize(stream); \
        return stream; \
    } \
    \
    QDataStream &operator>>( QDataStream &stream, TYPE &var ) \
    { \
        var.deserialize(stream); \
        return stream; \
    } \
    Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(TYPE)

#define Q_IMPLEMENT_USER_METATYPE_TYPEDEF(TAG,TYPE)     \
    int QMetaTypeRegister##TAG< TYPE >::__init_variable__ = \
        QMetaTypeRegister##TAG< TYPE >::registerType();

#define Q_IMPLEMENT_USER_METATYPE_ENUM(TYPE)    \
    QDataStream& operator<<( QDataStream& stream, const TYPE &v ) \
    { \
        stream << static_cast<qint32>(v); \
        return stream; \
    } \
    QDataStream& operator>>( QDataStream& stream, TYPE& v ) \
    { \
        qint32 _v; \
        stream >> _v; \
        v = static_cast<TYPE>(_v); \
        return stream; \
    } \
    Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(TYPE)

#endif //QMAILIPC_H

