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
    \namespace QMailDataComparator
    \ingroup messaginglibrary

    \brief The QMailDataComparator namespace contains types used in specifying the comparison
    of QMailStore objects with user-defined values.
*/

/*!
    \enum QMailDataComparator::EqualityComparator

    Defines the comparison operations that can be used to compare data elements 
    of QMailStore objects for equality and inequality.

    \value Equal Represents the '==' operator.
    \value NotEqual Represents the '!=' operator.
*/

/*!
    \enum QMailDataComparator::InclusionComparator

    Defines the comparison operations that can be used to compare data elements 
    of QMailStore objects for inclusion or exclusion.

    \value Includes Represents an operation in which an associated property is tested to 
                    determine whether it is equal to any of a supplied set of values.
                    Alternatively, it may be used to determine whether a single supplied
                    value is included within the associated QMailStore property.
    \value Excludes Represents an operation in which an associated property is tested to 
                    determine whether it is equal to none of a supplied set of values.
                    Alternatively, it may be used to determine whether a single supplied
                    value is not included within the associated QMailStore property.
*/

/*!
    \enum QMailDataComparator::RelationComparator

    Defines the comparison operations that can be used to compare data elements 
    of QMailStore objects, according to a specific relation.

    \value LessThan Represents the '<' operator.
    \value LessThanEqual Represents the '<=' operator.
    \value GreaterThan Represents the '>' operator.
    \value GreaterThanEqual Represents the '>= operator'.
*/

/*!
    \enum QMailDataComparator::PresenceComparator

    Defines the comparison operations that can be used to compare data elements 
    of QMailStore objects, according to presence or absence.

    \value Present Tests whether the specified property is present in the QMailStore object.
    \value Absent Tests whether the specified property is absent in the QMailStore object.
*/

