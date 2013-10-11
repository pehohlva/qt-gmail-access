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

#ifndef QPRIVATEIMPLEMENTATION_H
#define QPRIVATEIMPLEMENTATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

/* Rationale:

QSharedDataPointer has some deficiencies when used for implementation hiding,
which are exposed by its use in the messaging library:
1. It cannot easily be used with incomplete classes, requiring client classes
   to unnecessarily define destructors, copy constructors and assignment
   operators.
2. It is not polymorphic, and must be reused redundantly where a class with
   a hidden implementation is derived from another class with a hidden
   implementation.
3. Type-bridging functions are required to provide a supertype with access
   to the supertype data allocated within a subtype object.

The QPrivateImplementation class stores a pointer to the correct destructor
and copy-constructor, given the type of the actual object instantiated.
This allows it to be copied and deleted in contexts where the true type of
the implementation object is not recorded.

The QPrivateImplementationPointer provides QSharedDataPointer semantics,
providing the pointee type with necessary derived-type information. The
pointee type must derive from QPrivateImplementation.

The QPrivatelyImplemented<> template provides correct copy and assignment
functions, and allows the shared implementation object to be cast to the
different object types in the implementation type hierarchy.

*/

#include "qmailglobal.h"
#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>

class QPrivateImplementationBase
{
public:
    template<typename Subclass>
    inline QPrivateImplementationBase(Subclass* p)
        : ref_count(0),
          self(p),
          delete_function(&QPrivateImplementationBase::typed_delete<Subclass>),
          copy_function(&QPrivateImplementationBase::typed_copy_construct<Subclass>)
    {
    }

    inline QPrivateImplementationBase(const QPrivateImplementationBase& other)
        : ref_count(0),
          self(other.self),
          delete_function(other.delete_function),
          copy_function(other.copy_function)
    {
    }

    inline void ref()
    {
        ref_count.ref();
    }

    inline bool deref()
    {
        if (ref_count.deref() == 0 && delete_function && self) {
            (*delete_function)(self);
            return true;
        } else  {
            return false;
        }
    }

    inline void* detach()
    {
        if (copy_function && self && ref_count != 1) {
            void* copy = (*copy_function)(self);
            reinterpret_cast<QPrivateImplementationBase*>(copy)->self = copy;
            return copy;
        } else {
            return 0;
        }
    }

private:
    QAtomicInt ref_count;

    void *self;
    void (*delete_function)(void *p);
    void *(*copy_function)(const void *p);

    template<class T>
    static inline void typed_delete(void *p)
    {
        delete static_cast<T*>(p);
    }

    template<class T>
    static inline void* typed_copy_construct(const void *p)
    {
        return new T(*static_cast<const T*>(p));
    }

    // using the assignment operator would lead to corruption in the ref-counting
    QPrivateImplementationBase &operator=(const QPrivateImplementationBase &);
};

template <class T> class QPrivateImplementationPointer
{
public:
    inline T &operator*() { return *detach(); }
    inline const T &operator*() const { return *d; }

    inline T *operator->() { return detach(); }
    inline const T *operator->() const { return d; }

    inline operator T *() { return detach(); }
    inline operator const T *() const { return d; }

    inline T *data() { return detach(); }
    inline const T *data() const { return d; }

    inline const T *constData() const { return d; }

    inline bool operator==(const QPrivateImplementationPointer<T> &other) const { return d == other.d; }
    inline bool operator!=(const QPrivateImplementationPointer<T> &other) const { return d != other.d; }

    inline QPrivateImplementationPointer()
        : d(0)
    {
    }

    inline explicit QPrivateImplementationPointer(T *p)
        : d(p)
    {
        increment(d);
    }

    template<typename U>
    inline explicit QPrivateImplementationPointer(U *p)
        : d(static_cast<T*>(p))
    {
        increment(d);
    }

    inline QPrivateImplementationPointer(const QPrivateImplementationPointer<T> &o)
        : d(o.d)
    {
        increment(d);
    }

    inline ~QPrivateImplementationPointer()
    {
        decrement(d);
    }

    inline QPrivateImplementationPointer<T> &operator=(T *p)
    {
        assign_helper(p);
        return *this;
    }

    inline QPrivateImplementationPointer<T> &operator=(const QPrivateImplementationPointer<T> &o)
    {
        assign_helper(o.d);
        return *this;
    }

    inline bool operator!() const { return !d; }

private:
    void increment(T*& p);

    void decrement(T*& p);

    inline T* assign_helper(T *p)
    {
        if (p != d) {
            increment(p);
            decrement(d);
            d = p;
        }
        return d;
    }

    inline T* detach()
    {
        if (!d) return 0;

        if (T* detached = static_cast<T*>(d->detach())) {
            return assign_helper(detached);
        } else {
            return d;
        }
    }

public:
    T *d;
};

template<typename ImplementationType>
class QTOPIAMAIL_EXPORT QPrivatelyImplemented
{
public:
    QPrivatelyImplemented(ImplementationType* p);
    QPrivatelyImplemented(const QPrivatelyImplemented& other);

    template<typename A1>
    QTOPIAMAIL_EXPORT QPrivatelyImplemented(ImplementationType* p, A1 a1);

    virtual ~QPrivatelyImplemented();

    const QPrivatelyImplemented<ImplementationType>& operator=(const QPrivatelyImplemented<ImplementationType>& other);

    template<typename ImplementationSubclass>
    inline ImplementationSubclass* impl()
    {
        return static_cast<ImplementationSubclass*>(static_cast<ImplementationType*>(d));
    }

    template<typename InterfaceType>
    inline typename InterfaceType::ImplementationType* impl(InterfaceType*)
    {
        return impl<typename InterfaceType::ImplementationType>();
    }

    template<typename ImplementationSubclass>
    inline const ImplementationSubclass* impl() const
    {
        return static_cast<const ImplementationSubclass*>(static_cast<const ImplementationType*>(d));
    }

    template<typename InterfaceType>
    inline const typename InterfaceType::ImplementationType* impl(const InterfaceType*) const
    {
        return impl<const typename InterfaceType::ImplementationType>();
    }

protected:
    QPrivateImplementationPointer<ImplementationType> d;
};


class QPrivateNoncopyableBase
{
public:
    template<typename Subclass>
    inline QPrivateNoncopyableBase(Subclass* p)
        : self(p),
          delete_function(&QPrivateNoncopyableBase::typed_delete<Subclass>)
    {
    }

    inline void delete_self()
    {
        if (delete_function && self) {
            (*delete_function)(self);
        }
    }

private:
    void *self;
    void (*delete_function)(void *p);

    template<class T>
    static inline void typed_delete(void *p)
    {
        delete static_cast<T*>(p);
    }

    // do not permit copying
    QPrivateNoncopyableBase(const QPrivateNoncopyableBase &);

    QPrivateNoncopyableBase &operator=(const QPrivateNoncopyableBase &);
};

template <class T> class QPrivateNoncopyablePointer
{
public:
    inline T &operator*() { return *d; }
    inline const T &operator*() const { return *d; }

    inline T *operator->() { return d; }
    inline const T *operator->() const { return d; }

    inline operator T *() { return d; }
    inline operator const T *() const { return d; }

    inline T *data() { return d; }
    inline const T *data() const { return d; }

    inline const T *constData() const { return d; }

    inline bool operator==(const QPrivateNoncopyablePointer<T> &other) const { return d == other.d; }
    inline bool operator!=(const QPrivateNoncopyablePointer<T> &other) const { return d != other.d; }

    inline QPrivateNoncopyablePointer()
        : d(0)
    {
    }

    inline explicit QPrivateNoncopyablePointer(T *p)
        : d(p)
    {
    }

    template<typename U>
    inline explicit QPrivateNoncopyablePointer(U *p)
        : d(static_cast<T*>(p))
    {
    }

    ~QPrivateNoncopyablePointer();

    inline bool operator!() const { return !d; }

private:
    inline QPrivateNoncopyablePointer<T> &operator=(T *) { return *this; }

    inline QPrivateNoncopyablePointer<T> &operator=(const QPrivateNoncopyablePointer<T> &) { return *this; }

public:
    T *d;
};

template<typename ImplementationType>
class QTOPIAMAIL_EXPORT QPrivatelyNoncopyable
{
public:
    QPrivatelyNoncopyable(ImplementationType* p);

    template<typename A1>
    QTOPIAMAIL_EXPORT QPrivatelyNoncopyable(ImplementationType* p, A1 a1);

    virtual ~QPrivatelyNoncopyable();

    template<typename ImplementationSubclass>
    inline ImplementationSubclass* impl()
    {
        return static_cast<ImplementationSubclass*>(static_cast<ImplementationType*>(d));
    }

    template<typename InterfaceType>
    inline typename InterfaceType::ImplementationType* impl(InterfaceType*)
    {
        return impl<typename InterfaceType::ImplementationType>();
    }

    template<typename ImplementationSubclass>
    inline const ImplementationSubclass* impl() const
    {
        return static_cast<const ImplementationSubclass*>(static_cast<const ImplementationType*>(d));
    }

    template<typename InterfaceType>
    inline const typename InterfaceType::ImplementationType* impl(const InterfaceType*) const
    {
        return impl<const typename InterfaceType::ImplementationType>();
    }

protected:
    QPrivateNoncopyablePointer<ImplementationType> d;
};

#endif
