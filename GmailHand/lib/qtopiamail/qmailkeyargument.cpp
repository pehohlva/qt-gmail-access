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
    \class QMailKeyArgument

    \preliminary
    \brief The QMailKeyArgument class template defines a class representing a single criterion 
    to be applied when filtering the QMailStore constent with a key object.
    \ingroup messaginglibrary

    A QMailKeyArgument\<PropertyType, ComparatorType\> is composed of a property indicator, 
    a comparison operator and a value or set of values to compare with.  The type of the 
    property indicator depends on the type that is to be filtered.
*/

/*!
    \typedef QMailKeyArgument::Property
    
    Defines the type used to represent the property that the criterion is applied to.

    A synomyn for the PropertyType template parameter with which the template is instantiated.
*/

/*!
    \typedef QMailKeyArgument::Comparator
    
    Defines the type used to represent the comparison operation that the criterion requires.

    A synomyn for the ComparatorType template parameter with which the template is instantiated; defaults to QMailDataComparator::Comparator.
*/

/*!
    \variable QMailKeyArgument::property
    
    Indicates the property of the filtered entity to be compared.
*/

/*!
    \variable QMailKeyArgument::op
    
    Indicates the comparison operation to be used when filtering entities.
*/

/*!
    \variable QMailKeyArgument::valueList
    
    Contains the values to be compared with when filtering entities.
*/

/*!
    \fn QMailKeyArgument::QMailKeyArgument()
    \internal
*/

/*!
    \class QMailKeyArgument::ValueList
    \ingroup messaginglibrary

    \brief The ValueList class provides a list of variant values that can be serialized to a stream, and compared.

    The ValueList class inherits from QVariantList.

    \sa QVariantList
*/

/*!
    \fn bool QMailKeyArgument::ValueList::operator==(const ValueList &other) const

    Returns true if this list and \a other contain equivalent values.
*/

/*!
    \fn void QMailKeyArgument::ValueList::serialize(Stream &stream) const

    Writes the contents of a ValueList to \a stream.
*/

/*!
    \fn void QMailKeyArgument::ValueList::deserialize(Stream &stream)

    Reads the contents of a ValueList from \a stream.
*/

/*!
    \fn QMailKeyArgument::QMailKeyArgument(Property p, Comparator c, const QVariant& v)

    Creates a criterion testing the property \a p against the value \a v, using the comparison operator \a c.
*/
    
/*!
    \fn QMailKeyArgument::QMailKeyArgument(const ListType& l, Property p, Comparator c)

    Creates a criterion testing the property \a p against the value list \a l, using the comparison operator \a c.
*/
    
/*!
    \fn bool QMailKeyArgument::operator==(const QMailKeyArgument<PropertyType, ComparatorType>& other) const
    \internal
*/

/*!
    \fn void QMailKeyArgument::serialize(Stream &stream) const

    Writes the contents of a QMailKeyArgument to \a stream.
*/

/*!
    \fn void QMailKeyArgument::deserialize(Stream &stream)

    Reads the contents of a QMailKeyArgument from \a stream.
*/

