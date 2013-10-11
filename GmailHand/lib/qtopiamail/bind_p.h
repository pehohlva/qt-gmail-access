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

#ifndef BIND_P_H
#define BIND_P_H

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


// When using GCC 4.1.1 on ARM, TR1 functional cannot be included when RTTI
// is disabled, since it automatically instantiates some code using typeid().

// Provide the small parts of functional we use - binding only to member functions,
// with up to 6 function parameters, and with crefs only to value types.

namespace nonstd {
namespace tr1 {

namespace impl {

template<typename T>
struct ReferenceWrapper
{
    T* m_t;

    ReferenceWrapper(T& t) : m_t(&t) {}

    operator T&() const { return *m_t; }
};

template<typename R, typename F, typename A1>
struct FunctionWrapper1
{
    F m_f; A1 m_a1;

    FunctionWrapper1(F f, A1 a1) : m_f(f), m_a1(a1) {}

    R operator()() { return (m_a1->*m_f)(); }
};

template<typename R, typename F, typename A1, typename E1>
struct FunctionWrapper1e1
{
    F m_f; A1 m_a1;

    FunctionWrapper1e1(F f, A1 a1) : m_f(f), m_a1(a1) {}

    R operator()(E1 e1) { return (m_a1->*m_f)(e1); }
};

template<typename R, typename F, typename A1, typename E1, typename E2>
struct FunctionWrapper1e2
{
    F m_f; A1 m_a1;

    FunctionWrapper1e2(F f, A1 a1) : m_f(f), m_a1(a1) {}

    R operator()(E1 e1, E2 e2) { return (m_a1->*m_f)(e1, e2); }
};

template<typename R, typename F, typename A1, typename A2>
struct FunctionWrapper2
{
    F m_f; A1 m_a1; A2 m_a2;

    FunctionWrapper2(F f, A1 a1, A2 a2) : m_f(f), m_a1(a1), m_a2(a2) {}

    R operator()() { return (m_a1->*m_f)(m_a2); }
};

template<typename R, typename F, typename A1, typename A2, typename E1>
struct FunctionWrapper2e1
{
    F m_f; A1 m_a1; A2 m_a2;

    FunctionWrapper2e1(F f, A1 a1, A2 a2) : m_f(f), m_a1(a1), m_a2(a2) {}

    R operator()(E1 e1) { return (m_a1->*m_f)(m_a2, e1); }
};

template<typename R, typename F, typename A1, typename A2, typename E1, typename E2>
struct FunctionWrapper2e2
{
    F m_f; A1 m_a1; A2 m_a2;

    FunctionWrapper2e2(F f, A1 a1, A2 a2) : m_f(f), m_a1(a1), m_a2(a2) {}

    R operator()(E1 e1, E2 e2) { return (m_a1->*m_f)(m_a2, e1, e2); }
};

template<typename R, typename F, typename A1, typename A2, typename A3>
struct FunctionWrapper3
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3;

    FunctionWrapper3(F f, A1 a1, A2 a2, A3 a3) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3) {}

    R operator()() { return (m_a1->*m_f)(m_a2, m_a3); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename E1>
struct FunctionWrapper3e1
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3;

    FunctionWrapper3e1(F f, A1 a1, A2 a2, A3 a3) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3) {}

    R operator()(E1 e1) { return (m_a1->*m_f)(m_a2, m_a3, e1); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename E1, typename E2>
struct FunctionWrapper3e2
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3;

    FunctionWrapper3e2(F f, A1 a1, A2 a2, A3 a3) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3) {}

    R operator()(E1 e1, E2 e2) { return (m_a1->*m_f)(m_a2, m_a3, e1, e2); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4>
struct FunctionWrapper4
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4;

    FunctionWrapper4(F f, A1 a1, A2 a2, A3 a3, A4 a4) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4) {}

    R operator()() { return (m_a1->*m_f)(m_a2, m_a3, m_a4); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename E1>
struct FunctionWrapper4e1
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4;

    FunctionWrapper4e1(F f, A1 a1, A2 a2, A3 a3, A4 a4) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4) {}

    R operator()(E1 e1) { return (m_a1->*m_f)(m_a2, m_a3, m_a4, e1); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename E1, typename E2>
struct FunctionWrapper4e2
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4;

    FunctionWrapper4e2(F f, A1 a1, A2 a2, A3 a3, A4 a4) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4) {}

    R operator()(E1 e1, E2 e2) { return (m_a1->*m_f)(m_a2, m_a3, m_a4, e1, e2); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5>
struct FunctionWrapper5
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5;

    FunctionWrapper5(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5) {}

    R operator()() { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename E1>
struct FunctionWrapper5e1
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5;

    FunctionWrapper5e1(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5) {}

    R operator()(E1 e1) { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5, e1); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename E1, typename E2>
struct FunctionWrapper5e2
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5;

    FunctionWrapper5e2(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5) {}

    R operator()(E1 e1, E2 e2) { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5, e1, e2); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct FunctionWrapper6
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6;

    FunctionWrapper6(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6) {}

    R operator()() { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5, m_a6); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename E1>
struct FunctionWrapper6e1
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6;

    FunctionWrapper6e1(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6) {}

    R operator()(E1 e1) { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5, m_a6, e1); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename E1, typename E2>
struct FunctionWrapper6e2
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6;

    FunctionWrapper6e2(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6) {}

    R operator()(E1 e1, E2 e2) { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5, m_a6, e1, e2); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct FunctionWrapper7
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7;

    FunctionWrapper7(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7) {}

    R operator()() { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5, m_a6, m_a7); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename E1>
struct FunctionWrapper7e1
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7;

    FunctionWrapper7e1(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7) {}

    R operator()(E1 e1) { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, e1); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename E1, typename E2>
struct FunctionWrapper7e2
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7;

    FunctionWrapper7e2(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7) {}

    R operator()(E1 e1, E2 e2) { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, e1, e2); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct FunctionWrapper8
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8;

    FunctionWrapper8(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7), m_a8(a8) {}

    R operator()() { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename E1>
struct FunctionWrapper8e1
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8;

    FunctionWrapper8e1(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7), m_a8(a8) {}

    R operator()(E1 e1) { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8, e1); }
};

template<typename R, typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename E1, typename E2>
struct FunctionWrapper8e2
{
    F m_f; A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8;

    FunctionWrapper8e2(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) : m_f(f), m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7), m_a8(a8) {}

    R operator()(E1 e1, E2 e2) { return (m_a1->*m_f)(m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8, e1, e2); }
};

} // namespace impl

template<typename T>
impl::ReferenceWrapper<const T> cref(const T& t)
{
    return impl::ReferenceWrapper<const T>(t);
}

template<typename R, typename T, typename A1>
impl::FunctionWrapper1<R, R (T::*)(), A1> bind(R (T::*f)(), A1 a1)
{
    return impl::FunctionWrapper1<R, R (T::*)(), A1>(f, a1);
}

template<typename R, typename T, typename A1>
impl::FunctionWrapper1<R, R (T::*)() const, A1> bind(R (T::*f)() const, A1 a1)
{
    return impl::FunctionWrapper1<R, R (T::*)() const, A1>(f, a1);
}

template<typename R, typename T, typename E1, typename A1>
impl::FunctionWrapper1e1<R, R (T::*)(E1), A1, E1> bind(R (T::*f)(E1), A1 a1)
{
    return impl::FunctionWrapper1e1<R, R (T::*)(E1), A1, E1>(f, a1);
}

template<typename R, typename T, typename E1, typename A1>
impl::FunctionWrapper1e1<R, R (T::*)(E1) const, A1, E1> bind(R (T::*f)(E1) const, A1 a1)
{
    return impl::FunctionWrapper1e1<R, R (T::*)(E1) const, A1, E1>(f, a1);
}

template<typename R, typename T, typename E1, typename E2, typename A1>
impl::FunctionWrapper1e2<R, R (T::*)(E1, E2), A1, E1, E2> bind(R (T::*f)(E1, E2), A1 a1)
{
    return impl::FunctionWrapper1e2<R, R (T::*)(E1, E2), A1, E1, E2>(f, a1);
}

template<typename R, typename T, typename E1, typename E2, typename A1>
impl::FunctionWrapper1e2<R, R (T::*)(E1, E2) const, A1, E1, E2> bind(R (T::*f)(E1, E2) const, A1 a1)
{
    return impl::FunctionWrapper1e2<R, R (T::*)(E1, E2) const, A1, E1, E2>(f, a1);
}

template<typename R, typename T, typename B1, typename A1, typename A2>
impl::FunctionWrapper2<R, R (T::*)(B1), A1, A2> bind(R (T::*f)(B1), A1 a1, A2 a2)
{
    return impl::FunctionWrapper2<R, R (T::*)(B1), A1, A2>(f, a1, a2);
}

template<typename R, typename T, typename B1, typename A1, typename A2>
impl::FunctionWrapper2<R, R (T::*)(B1) const, A1, A2> bind(R (T::*f)(B1) const, A1 a1, A2 a2)
{
    return impl::FunctionWrapper2<R, R (T::*)(B1) const, A1, A2>(f, a1, a2);
}

template<typename R, typename T, typename B1, typename E1, typename A1, typename A2>
impl::FunctionWrapper2e1<R, R (T::*)(B1, E1), A1, A2, E1> bind(R (T::*f)(B1, E1), A1 a1, A2 a2)
{
    return impl::FunctionWrapper2e1<R, R (T::*)(B1, E1), A1, A2, E1>(f, a1, a2);
}

template<typename R, typename T, typename B1, typename E1, typename A1, typename A2>
impl::FunctionWrapper2e1<R, R (T::*)(B1, E1) const, A1, A2, E1> bind(R (T::*f)(B1, E1) const, A1 a1, A2 a2)
{
    return impl::FunctionWrapper2e1<R, R (T::*)(B1, E1) const, A1, A2, E1>(f, a1, a2);
}

template<typename R, typename T, typename B1, typename E1, typename E2, typename A1, typename A2>
impl::FunctionWrapper2e2<R, R (T::*)(B1, E1, E2), A1, A2, E1, E2> bind(R (T::*f)(B1, E1, E2), A1 a1, A2 a2)
{
    return impl::FunctionWrapper2e2<R, R (T::*)(B1, E1, E2), A1, A2, E1, E2>(f, a1, a2);
}

template<typename R, typename T, typename B1, typename E1, typename E2, typename A1, typename A2>
impl::FunctionWrapper2e2<R, R (T::*)(B1, E1, E2) const, A1, A2, E1, E2> bind(R (T::*f)(B1, E1, E2) const, A1 a1, A2 a2)
{
    return impl::FunctionWrapper2e2<R, R (T::*)(B1, E1, E2) const, A1, A2, E1, E2>(f, a1, a2);
}

template<typename R, typename T, typename B1, typename B2, typename A1, typename A2, typename A3>
impl::FunctionWrapper3<R, R (T::*)(B1, B2), A1, A2, A3> bind(R (T::*f)(B1, B2), A1 a1, A2 a2, A3 a3)
{
    return impl::FunctionWrapper3<R, R (T::*)(B1, B2), A1, A2, A3>(f, a1, a2, a3);
}

template<typename R, typename T, typename B1, typename B2, typename A1, typename A2, typename A3>
impl::FunctionWrapper3<R, R (T::*)(B1, B2) const, A1, A2, A3> bind(R (T::*f)(B1, B2) const, A1 a1, A2 a2, A3 a3)
{
    return impl::FunctionWrapper3<R, R (T::*)(B1, B2) const, A1, A2, A3>(f, a1, a2, a3);
}

template<typename R, typename T, typename B1, typename B2, typename E1, typename A1, typename A2, typename A3>
impl::FunctionWrapper3e1<R, R (T::*)(B1, B2, E1), A1, A2, A3, E1> bind(R (T::*f)(B1, B2, E1), A1 a1, A2 a2, A3 a3)
{
    return impl::FunctionWrapper3e1<R, R (T::*)(B1, B2, E1), A1, A2, A3, E1>(f, a1, a2, a3);
}

template<typename R, typename T, typename B1, typename B2, typename E1, typename A1, typename A2, typename A3>
impl::FunctionWrapper3e1<R, R (T::*)(B1, B2, E1) const, A1, A2, A3, E1> bind(R (T::*f)(B1, B2, E1) const, A1 a1, A2 a2, A3 a3)
{
    return impl::FunctionWrapper3e1<R, R (T::*)(B1, B2, E1) const, A1, A2, A3, E1>(f, a1, a2, a3);
}

template<typename R, typename T, typename B1, typename B2, typename E1, typename E2, typename A1, typename A2, typename A3>
impl::FunctionWrapper3e2<R, R (T::*)(B1, B2, E1, E2), A1, A2, A3, E1, E2> bind(R (T::*f)(B1, B2, E1, E2), A1 a1, A2 a2, A3 a3)
{
    return impl::FunctionWrapper3e2<R, R (T::*)(B1, B2, E1, E2), A1, A2, A3, E1, E2>(f, a1, a2, a3);
}

template<typename R, typename T, typename B1, typename B2, typename E1, typename E2, typename A1, typename A2, typename A3>
impl::FunctionWrapper3e2<R, R (T::*)(B1, B2, E1, E2) const, A1, A2, A3, E1, E2> bind(R (T::*f)(B1, B2, E1, E2) const, A1 a1, A2 a2, A3 a3)
{
    return impl::FunctionWrapper3e2<R, R (T::*)(B1, B2, E1, E2) const, A1, A2, A3, E1, E2>(f, a1, a2, a3);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename A1, typename A2, typename A3, typename A4>
impl::FunctionWrapper4<R, R (T::*)(B1, B2, B3), A1, A2, A3, A4> bind(R (T::*f)(B1, B2, B3), A1 a1, A2 a2, A3 a3, A4 a4)
{
    return impl::FunctionWrapper4<R, R (T::*)(B1, B2, B3), A1, A2, A3, A4>(f, a1, a2, a3, a4);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename A1, typename A2, typename A3, typename A4>
impl::FunctionWrapper4<R, R (T::*)(B1, B2, B3) const, A1, A2, A3, A4> bind(R (T::*f)(B1, B2, B3) const, A1 a1, A2 a2, A3 a3, A4 a4)
{
    return impl::FunctionWrapper4<R, R (T::*)(B1, B2, B3) const, A1, A2, A3, A4>(f, a1, a2, a3, a4);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename E1, typename A1, typename A2, typename A3, typename A4>
impl::FunctionWrapper4e1<R, R (T::*)(B1, B2, B3, E1), A1, A2, A3, A4, E1> bind(R (T::*f)(B1, B2, B3, E1), A1 a1, A2 a2, A3 a3, A4 a4)
{
    return impl::FunctionWrapper4e1<R, R (T::*)(B1, B2, B3, E1), A1, A2, A3, A4, E1>(f, a1, a2, a3, a4);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename E1, typename A1, typename A2, typename A3, typename A4>
impl::FunctionWrapper4e1<R, R (T::*)(B1, B2, B3, E1) const, A1, A2, A3, A4, E1> bind(R (T::*f)(B1, B2, B3, E1) const, A1 a1, A2 a2, A3 a3, A4 a4)
{
    return impl::FunctionWrapper4e1<R, R (T::*)(B1, B2, B3, E1) const, A1, A2, A3, A4, E1>(f, a1, a2, a3, a4);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename E1, typename E2, typename A1, typename A2, typename A3, typename A4>
impl::FunctionWrapper4e2<R, R (T::*)(B1, B2, B3, E1, E2), A1, A2, A3, A4, E1, E2> bind(R (T::*f)(B1, B2, B3, E1, E2), A1 a1, A2 a2, A3 a3, A4 a4)
{
    return impl::FunctionWrapper4e2<R, R (T::*)(B1, B2, B3, E1, E2), A1, A2, A3, A4, E1, E2>(f, a1, a2, a3, a4);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename E1, typename E2, typename A1, typename A2, typename A3, typename A4>
impl::FunctionWrapper4e2<R, R (T::*)(B1, B2, B3, E1, E2) const, A1, A2, A3, A4, E1, E2> bind(R (T::*f)(B1, B2, B3, E1, E2) const, A1 a1, A2 a2, A3 a3, A4 a4)
{
    return impl::FunctionWrapper4e2<R, R (T::*)(B1, B2, B3, E1, E2) const, A1, A2, A3, A4, E1, E2>(f, a1, a2, a3, a4);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename A1, typename A2, typename A3, typename A4, typename A5>
impl::FunctionWrapper5<R, R (T::*)(B1, B2, B3, B4), A1, A2, A3, A4, A5> bind(R (T::*f)(B1, B2, B3, B4), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    return impl::FunctionWrapper5<R, R (T::*)(B1, B2, B3, B4), A1, A2, A3, A4, A5>(f, a1, a2, a3, a4, a5);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename A1, typename A2, typename A3, typename A4, typename A5>
impl::FunctionWrapper5<R, R (T::*)(B1, B2, B3, B4) const, A1, A2, A3, A4, A5> bind(R (T::*f)(B1, B2, B3, B4) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    return impl::FunctionWrapper5<R, R (T::*)(B1, B2, B3, B4) const, A1, A2, A3, A4, A5>(f, a1, a2, a3, a4, a5);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename E1, typename A1, typename A2, typename A3, typename A4, typename A5>
impl::FunctionWrapper5e1<R, R (T::*)(B1, B2, B3, B4, E1), A1, A2, A3, A4, A5, E1> bind(R (T::*f)(B1, B2, B3, B4, E1), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    return impl::FunctionWrapper5e1<R, R (T::*)(B1, B2, B3, B4, E1), A1, A2, A3, A4, A5, E1>(f, a1, a2, a3, a4, a5);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename E1, typename A1, typename A2, typename A3, typename A4, typename A5>
impl::FunctionWrapper5e1<R, R (T::*)(B1, B2, B3, B4, E1) const, A1, A2, A3, A4, A5, E1> bind(R (T::*f)(B1, B2, B3, B4, E1) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    return impl::FunctionWrapper5e1<R, R (T::*)(B1, B2, B3, B4, E1) const, A1, A2, A3, A4, A5, E1>(f, a1, a2, a3, a4, a5);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename E1, typename E2, typename A1, typename A2, typename A3, typename A4, typename A5>
impl::FunctionWrapper5e2<R, R (T::*)(B1, B2, B3, B4, E1, E2), A1, A2, A3, A4, A5, E1, E2> bind(R (T::*f)(B1, B2, B3, B4, E1, E2), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    return impl::FunctionWrapper5e2<R, R (T::*)(B1, B2, B3, B4, E1, E2), A1, A2, A3, A4, A5, E1, E2>(f, a1, a2, a3, a4, a5);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename E1, typename E2, typename A1, typename A2, typename A3, typename A4, typename A5>
impl::FunctionWrapper5e2<R, R (T::*)(B1, B2, B3, B4, E1, E2) const, A1, A2, A3, A4, A5, E1, E2> bind(R (T::*f)(B1, B2, B3, B4, E1, E2) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    return impl::FunctionWrapper5e2<R, R (T::*)(B1, B2, B3, B4, E1, E2) const, A1, A2, A3, A4, A5, E1, E2>(f, a1, a2, a3, a4, a5);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
impl::FunctionWrapper6<R, R (T::*)(B1, B2, B3, B4, B5), A1, A2, A3, A4, A5, A6> bind(R (T::*f)(B1, B2, B3, B4, B5), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    return impl::FunctionWrapper6<R, R (T::*)(B1, B2, B3, B4, B5), A1, A2, A3, A4, A5, A6>(f, a1, a2, a3, a4, a5, a6);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
impl::FunctionWrapper6<R, R (T::*)(B1, B2, B3, B4, B5) const, A1, A2, A3, A4, A5, A6> bind(R (T::*f)(B1, B2, B3, B4, B5) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    return impl::FunctionWrapper6<R, R (T::*)(B1, B2, B3, B4, B5) const, A1, A2, A3, A4, A5, A6>(f, a1, a2, a3, a4, a5, a6);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename E1, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
impl::FunctionWrapper6e1<R, R (T::*)(B1, B2, B3, B4, B5, E1), A1, A2, A3, A4, A5, A6, E1> bind(R (T::*f)(B1, B2, B3, B4, B5, E1), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    return impl::FunctionWrapper6e1<R, R (T::*)(B1, B2, B3, B4, B5, E1), A1, A2, A3, A4, A5, A6, E1>(f, a1, a2, a3, a4, a5, a6);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename E1, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
impl::FunctionWrapper6e1<R, R (T::*)(B1, B2, B3, B4, B5, E1) const, A1, A2, A3, A4, A5, A6, E1> bind(R (T::*f)(B1, B2, B3, B4, B5, E1) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    return impl::FunctionWrapper6e1<R, R (T::*)(B1, B2, B3, B4, B5, E1) const, A1, A2, A3, A4, A5, A6, E1>(f, a1, a2, a3, a4, a5, a6);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename E1, typename E2, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
impl::FunctionWrapper6e2<R, R (T::*)(B1, B2, B3, B4, B5, E1, E2), A1, A2, A3, A4, A5, A6, E1, E2> bind(R (T::*f)(B1, B2, B3, B4, B5, E1, E2), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    return impl::FunctionWrapper6e2<R, R (T::*)(B1, B2, B3, B4, B5, E1, E2), A1, A2, A3, A4, A5, A6, E1, E2>(f, a1, a2, a3, a4, a5, a6);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename E1, typename E2, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
impl::FunctionWrapper6e2<R, R (T::*)(B1, B2, B3, B4, B5, E1, E2) const, A1, A2, A3, A4, A5, A6, E1, E2> bind(R (T::*f)(B1, B2, B3, B4, B5, E1, E2) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    return impl::FunctionWrapper6e2<R, R (T::*)(B1, B2, B3, B4, B5, E1, E2) const, A1, A2, A3, A4, A5, A6, E1, E2>(f, a1, a2, a3, a4, a5, a6);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
impl::FunctionWrapper7<R, R (T::*)(B1, B2, B3, B4, B5, B6), A1, A2, A3, A4, A5, A6, A7> bind(R (T::*f)(B1, B2, B3, B4, B5, B6), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    return impl::FunctionWrapper7<R, R (T::*)(B1, B2, B3, B4, B5, B6), A1, A2, A3, A4, A5, A6, A7>(f, a1, a2, a3, a4, a5, a6, a7);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
impl::FunctionWrapper7<R, R (T::*)(B1, B2, B3, B4, B5, B6) const, A1, A2, A3, A4, A5, A6, A7> bind(R (T::*f)(B1, B2, B3, B4, B5, B6) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    return impl::FunctionWrapper7<R, R (T::*)(B1, B2, B3, B4, B5, B6) const, A1, A2, A3, A4, A5, A6, A7>(f, a1, a2, a3, a4, a5, a6, a7);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename E1, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
impl::FunctionWrapper7e1<R, R (T::*)(B1, B2, B3, B4, B5, B6, E1), A1, A2, A3, A4, A5, A6, A7, E1> bind(R (T::*f)(B1, B2, B3, B4, B5, B6, E1), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    return impl::FunctionWrapper7e1<R, R (T::*)(B1, B2, B3, B4, B5, B6, E1), A1, A2, A3, A4, A5, A6, A7, E1>(f, a1, a2, a3, a4, a5, a6, a7);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename E1, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
impl::FunctionWrapper7e1<R, R (T::*)(B1, B2, B3, B4, B5, B6, E1) const, A1, A2, A3, A4, A5, A6, A7, E1> bind(R (T::*f)(B1, B2, B3, B4, B5, B6, E1) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    return impl::FunctionWrapper7e1<R, R (T::*)(B1, B2, B3, B4, B5, B6, E1) const, A1, A2, A3, A4, A5, A6, A7, E1>(f, a1, a2, a3, a4, a5, a6, a7);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename E1, typename E2, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
impl::FunctionWrapper7e2<R, R (T::*)(B1, B2, B3, B4, B5, B6, E1, E2), A1, A2, A3, A4, A5, A6, A7, E1, E2> bind(R (T::*f)(B1, B2, B3, B4, B5, B6, E1, E2), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    return impl::FunctionWrapper7e2<R, R (T::*)(B1, B2, B3, B4, B5, B6, E1, E2), A1, A2, A3, A4, A5, A6, A7, E1, E2>(f, a1, a2, a3, a4, a5, a6, a7);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename E1, typename E2, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
impl::FunctionWrapper7e2<R, R (T::*)(B1, B2, B3, B4, B5, B6, E1, E2) const, A1, A2, A3, A4, A5, A6, A7, E1, E2> bind(R (T::*f)(B1, B2, B3, B4, B5, B6, E1, E2) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    return impl::FunctionWrapper7e2<R, R (T::*)(B1, B2, B3, B4, B5, B6, E1, E2) const, A1, A2, A3, A4, A5, A6, A7, E1, E2>(f, a1, a2, a3, a4, a5, a6, a7);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename B7, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
impl::FunctionWrapper8<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7), A1, A2, A3, A4, A5, A6, A7, A8> bind(R (T::*f)(B1, B2, B3, B4, B5, B6, B7), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    return impl::FunctionWrapper8<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7), A1, A2, A3, A4, A5, A6, A7, A8>(f, a1, a2, a3, a4, a5, a6, a7, a8);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename B7, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
impl::FunctionWrapper8<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7) const, A1, A2, A3, A4, A5, A6, A7, A8> bind(R (T::*f)(B1, B2, B3, B4, B5, B6, B7) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    return impl::FunctionWrapper8<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7) const, A1, A2, A3, A4, A5, A6, A7, A8>(f, a1, a2, a3, a4, a5, a6, a7, a8);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename B7, typename E1, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
impl::FunctionWrapper8e1<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7, E1), A1, A2, A3, A4, A5, A6, A7, A8, E1> bind(R (T::*f)(B1, B2, B3, B4, B5, B6, B7, E1), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    return impl::FunctionWrapper8e1<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7, E1), A1, A2, A3, A4, A5, A6, A7, A8, E1>(f, a1, a2, a3, a4, a5, a6, a7, a8);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename B7, typename E1, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
impl::FunctionWrapper8e1<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7, E1) const, A1, A2, A3, A4, A5, A6, A7, A8, E1> bind(R (T::*f)(B1, B2, B3, B4, B5, B6, B7, E1) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    return impl::FunctionWrapper8e1<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7, E1) const, A1, A2, A3, A4, A5, A6, A7, A8, E1>(f, a1, a2, a3, a4, a5, a6, a7, a8);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename B7, typename E1, typename E2, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
impl::FunctionWrapper8e2<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7, E1, E2), A1, A2, A3, A4, A5, A6, A7, A8, E1, E2> bind(R (T::*f)(B1, B2, B3, B4, B5, B6, B7, E1, E2), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    return impl::FunctionWrapper8e2<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7, E1, E2), A1, A2, A3, A4, A5, A6, A7, A8, E1, E2>(f, a1, a2, a3, a4, a5, a6, a7, a8);
}

template<typename R, typename T, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename B7, typename E1, typename E2, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
impl::FunctionWrapper8e2<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7, E1, E2) const, A1, A2, A3, A4, A5, A6, A7, A8, E1, E2> bind(R (T::*f)(B1, B2, B3, B4, B5, B6, B7, E1, E2) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    return impl::FunctionWrapper8e2<R, R (T::*)(B1, B2, B3, B4, B5, B6, B7, E1, E2) const, A1, A2, A3, A4, A5, A6, A7, A8, E1, E2>(f, a1, a2, a3, a4, a5, a6, a7, a8);
}

}  // namespace tr1
}  // namespace nonstd

#endif

