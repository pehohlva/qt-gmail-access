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

#include "qmailaddress.h"
#include "qmaillog.h"
#include "qmailmessage.h"
#include "qmailnamespace.h"

struct CharacterProcessor
{
    virtual ~CharacterProcessor();

    void processCharacters(const QString& input);
    virtual void process(QChar, bool, bool, int) = 0;

    virtual void finished();
};

CharacterProcessor::~CharacterProcessor()
{
}

void CharacterProcessor::processCharacters(const QString& input)
{
    int commentDepth = 0;
    bool quoted = false;
    bool escaped = false;

    const QChar* it = input.constData();
    const QChar* const end = it + input.length();
    for ( ; it != end; ++it ) {
        if ( !escaped && ( *it == '\\' ) ) {
            escaped = true;
            continue;
        }

        bool quoteProcessed = false;
        if ( *it == '(' && !escaped && !quoted ) {
            commentDepth += 1;
        }
        else if ( !quoted && *it == '"' && !escaped ) {
            quoted = true;
            quoteProcessed = true;
        }

        process((*it), quoted, escaped, commentDepth);

        if ( *it == ')' && !escaped && !quoted && ( commentDepth > 0 ) ) {
            commentDepth -= 1;
        }
        else if ( quoted && *it == '"' && !quoteProcessed && !escaped ) {
            quoted = false;
        }

        escaped = false;
    }

    finished();
}

void CharacterProcessor::finished()
{
}

struct Decommentor : public CharacterProcessor
{
    Decommentor(bool (QChar::*classifier)() const, bool accepted);

    QString _result;
    bool (QChar::*_classifier)() const;
    bool _accepted;

    virtual void process(QChar, bool, bool, int);
};

Decommentor::Decommentor(bool (QChar::*classifier)() const, bool accepted)
    : _classifier(classifier),
      _accepted(accepted)
{
}

void Decommentor::process(QChar character, bool quoted, bool escaped, int commentDepth)
{
    if ( commentDepth == 0 ) {
        if ( quoted || ((character.*_classifier)() == _accepted) )
            _result.append( character );
    }

    Q_UNUSED(escaped)
}

static QString removeComments(const QString& input, bool (QChar::*classifier)() const, bool acceptedResult = true)
{
    Decommentor decommentor(classifier, acceptedResult);
    decommentor.processCharacters(input);
    return decommentor._result;
}


struct AddressSeparator : public CharacterProcessor
{
    enum TokenType { Unknown = 0, Address, Name, Suffix, Comment, Group, TypeCount };

    AddressSeparator();

    virtual void process(QChar, bool, bool, int);
    virtual void finished();

    virtual void accept(QChar) = 0;
    virtual QString progress() const = 0;
    virtual void complete(TokenType type, bool) = 0;

private:
    void separator(bool);

    bool _inAddress;
    bool _inGroup;
    bool _tokenStarted;
    bool _tokenCompleted;
    TokenType _type;
};

AddressSeparator::AddressSeparator()
    : _inAddress(false),
      _inGroup(false),
      _tokenStarted(false),
      _tokenCompleted(false),
      _type(Unknown)
{
}

void AddressSeparator::process(QChar character, bool quoted, bool escaped, int commentDepth)
{
    if (_tokenCompleted && (!character.isSpace())) {
        separator(false);
    }

    // RFC 2822 requires comma as the separator, but we'll allow the semi-colon as well.
    if ( ( character == ',' || character == ';' || character.isSpace()) && 
         !_inGroup && !quoted && !escaped && commentDepth == 0 ) {
        if (character.isSpace()) {
            // We'll also attempt to separate on whitespace, but we need to append it to 
            // the token to preserve the input data
            accept(character);
            _tokenCompleted = true;
        } else {
            separator(true);
        }
    }
    else {
        if (commentDepth && _type == Unknown && _tokenStarted == false) {
            // This could be a purely comment element
            _type = Comment;
        }
        else if (quoted && (_type == Unknown || _type == Comment)) {
            // This must be a name element
            _type = Name;
        }

        accept(character);
        _tokenStarted = true;

        if ( character == '<' && !_inAddress && !quoted && !escaped && commentDepth == 0 ) {
            _inAddress = true;

            if (_type == Unknown || _type == Comment)
                _type = Address;
        } else if ( character == '>' && _inAddress && !quoted && !escaped && commentDepth == 0 ) {
            _inAddress = false;
        }
        else if ( character == ':' && !_inGroup && !_inAddress && !quoted && !escaped && commentDepth == 0 ) {
            static const QString collectiveTag;

            // Don't parse as a group if we match the IM format
            // TODO: what if the group name actually matches the tag?
            if (progress() != collectiveTag) {
                _inGroup = true;
                _type = Group;
            }
        }
        else if ( character == ';' && _inGroup && !_inAddress && !quoted && !escaped && commentDepth == 0 ) {
            _inGroup = false;

            // This is a soft separator, because the group construct could have a trailing comment
            separator(false);
        }
    }
}

void AddressSeparator::separator(bool hardSeparator)
{
    complete(_type, hardSeparator);

    _tokenStarted = false;
    _tokenCompleted = false;
    _type = Unknown;
}

void AddressSeparator::finished()
{
    complete(_type, true);
}


struct AddressListGenerator : public AddressSeparator
{
    virtual void accept(QChar);
    virtual QString progress() const;
    virtual void complete(TokenType, bool);

    QStringList result();

private:
    typedef QPair<TokenType, QString> Token;

    int combinableElements();
    void processPending();

    QStringList _result;
    QList<Token> _pending;
    QString _partial;
};

void AddressListGenerator::accept(QChar character)
{
    _partial.append(character);
}

QString AddressListGenerator::progress() const
{
    return _partial;
}

void AddressListGenerator::complete(TokenType type, bool hardSeparator)
{
    if (_partial.trimmed().length()) {
        if (type == Unknown) {
            // We need to know what type of token this is

            // Test whether the token is a suffix
            QRegExp suffixPattern("\\s*/TYPE=.*");
            if (suffixPattern.exactMatch(_partial)) {
                type = Suffix;
            } 
            else {
                // See if the token is a bare email address; otherwise it must be a name element
                QRegExp emailPattern(QMailAddress::emailAddressPattern());
                type = (emailPattern.exactMatch(_partial.trimmed()) ? Address : Name);
            }
        }

        _pending.append(qMakePair(type, _partial));
        _partial.clear();
    }

    if (hardSeparator) {
        // We know that this is a boundary between addresses
        processPending();
    }
}

int AddressListGenerator::combinableElements()
{
    bool used[TypeCount] = { false };

    int combinable = 0;
    int i = _pending.count();
    while (i > 0) {
        int type = _pending.value(i - 1).first;

        // If this type has already appeared in this address, this must be part of the preceding address
        if (used[type])
            return combinable;

        // A suffix can only appear at the end of an address
        if (type == Suffix && (combinable > 0))
            return combinable;

        if (type == Comment) {
            // Comments can be combined with anything else...
        } else {
            // Everything else can be used once at most, and not with groups
            if (used[Group])
                return combinable;

            if (type == Group) {
                // Groups can only be combined with comments
                if (used[Name] || used[Address] || used[Suffix])
                    return combinable;
            }

            used[type] = true;
        }

        // Combine this element
        ++combinable;
        --i;
    }

    return combinable;
}

void AddressListGenerator::processPending()
{
    if (!_pending.isEmpty()) {
        // Compress any consecutive name parts into a single part
        for (int i = 1; i < _pending.count(); ) {
            TokenType type = _pending.value(i).first;
            // Also, a name could precede a group part, since the group name may contain multiple atoms
            if ((_pending.value(i - 1).first == Name) && ((type == Name) || (type == Group))) {
                _pending.replace(i - 1, qMakePair(type, _pending.value(i - 1).second + _pending.value(i).second));
                _pending.removeAt(i);
            } 
            else {
                ++i;
            }
        }

        // Combine the tokens as necessary, proceding in reverse from the known boundary at the end
        QStringList addresses;
        int combinable = 0;
        while ((combinable = combinableElements()) != 0) {
            QString combined;
            while (combinable) {
                combined.prepend(_pending.last().second);
                _pending.removeLast();
                --combinable;
            }
            addresses.append(combined);
        }

        // Add the address to the result set, in the original order
        for (int i = addresses.count(); i > 0; --i)
            _result.append(addresses.value(i - 1));

        _pending.clear();
    }
}

QStringList AddressListGenerator::result()
{
    return _result;
}

static QStringList generateAddressList(const QString& list)
{
    AddressListGenerator generator;
    generator.processCharacters(list);
    return generator.result();
}

static bool containsMultipleFields(const QString& input)
{
    // There is no shortcut; we have to parse the addresses
    AddressListGenerator generator;
    generator.processCharacters(input);
    return (generator.result().count() > 1);
}


struct GroupDetector : public CharacterProcessor
{
    GroupDetector();

    virtual void process(QChar, bool, bool, int);

    bool result() const;

private:
    bool _nameDelimiter;
    bool _listTerminator;
};

GroupDetector::GroupDetector()
    : _nameDelimiter(false),
      _listTerminator(false)
{
}

void GroupDetector::process(QChar character, bool quoted, bool escaped, int commentDepth)
{
    if ( character == ':' && !_nameDelimiter && !quoted && !escaped && commentDepth == 0 )
        _nameDelimiter = true;
    else if ( character == ';' && !_listTerminator && _nameDelimiter && !quoted && !escaped && commentDepth == 0 )
        _listTerminator = true;
}

bool GroupDetector::result() const
{
    return _listTerminator;
}

static bool containsGroupSpecifier(const QString& input)
{
    GroupDetector detector;
    detector.processCharacters(input);
    return detector.result();
}


struct WhitespaceRemover : public CharacterProcessor
{
    virtual void process(QChar, bool, bool, int);

    QString _result;
};

void WhitespaceRemover::process(QChar character, bool quoted, bool escaped, int commentDepth)
{
    if ( !character.isSpace() || quoted || escaped || commentDepth > 0 )
        _result.append(character);
}

static QString removeWhitespace(const QString& input)
{
    WhitespaceRemover remover;
    remover.processCharacters(input);
    return remover._result;
}


/* QMailAddress */
class QMailAddressPrivate : public QSharedData
{
public:
    QMailAddressPrivate();
    QMailAddressPrivate(const QString& addressText);
    QMailAddressPrivate(const QString& name, const QString& address);
    QMailAddressPrivate(const QMailAddressPrivate& other);
    ~QMailAddressPrivate();

    bool isNull() const;

    bool isGroup() const;
    QList<QMailAddress> groupMembers() const;

    QString name() const;
    bool isPhoneNumber() const;
    bool isEmailAddress() const;

    QString minimalPhoneNumber() const;

    QString toString(bool forceDelimited) const;

    QString _name;
    QString _address;
    QString _suffix;
    bool _group;

    bool operator==(const QMailAddressPrivate& other) const;

    template <typename Stream>
    void serialize(Stream &stream) const;

    template <typename Stream>
    void deserialize(Stream &stream);

private:
    void setComponents(const QString& nameText, const QString& addressText);

    mutable bool _searchCompleted;
};

QMailAddressPrivate::QMailAddressPrivate()
    : _group(false),
      _searchCompleted(false)
{
}

static QPair<int, int> findDelimiters(const QString& text)
{
    int first = -1;
    int second = -1;

    bool quoted = false;
    bool escaped = false;

    const QChar* const begin = text.constData();
    const QChar* const end = begin + text.length();
    for (const QChar* it = begin; it != end; ++it ) {
        if ( !escaped && ( *it == '\\' ) ) {
            escaped = true;
            continue;
        }

        if ( !quoted && *it == '"' && !escaped ) {
            quoted = true;
        }
        else if ( quoted && *it == '"' && !escaped ) {
            quoted = false;
        }

        if ( !quoted ) {
            if ( first == -1 && *it == '<' ) {
                first = (it - begin);
            }
            else if ( second == -1 && *it == '>' ) {
                second = (it - begin);
                break;
            }
        }

        escaped = false;
    }

    return qMakePair(first, second);
}

static void parseMailbox(QString& input, QString& name, QString& address, QString& suffix)
{
    // See if there is a trailing suffix
    int pos = input.indexOf("/TYPE=");
    if (pos != -1)
    {
        suffix = input.mid(pos + 6);
        input = input.left(pos);
    }

    // Separate the email address from the name
    QPair<int, int> delimiters = findDelimiters(input);

    if (delimiters.first == -1 && delimiters.second == -1)
    {
        name = address = input.trimmed();
    }
    else 
    {
        if (delimiters.first == -1)
        {
            // Unmatched '>'
            address = input.left( delimiters.second );
        }
        else
        {
            name = input.left( delimiters.first );

            if (delimiters.second == -1)
                address = input.right(input.length() - delimiters.first - 1);
            else
                address = input.mid(delimiters.first + 1, (delimiters.second - delimiters.first - 1)).trimmed();
        }

        if ( name.isEmpty() ) 
            name = address;
    } 
}

QMailAddressPrivate::QMailAddressPrivate(const QString& addressText) 
    : _group(false),
      _searchCompleted(false)
{
    if (!addressText.isEmpty())
    {
        QString input = addressText.trimmed();

        // See whether this address is a group
        if (containsGroupSpecifier(input))
        {
            QRegExp groupFormat("(.*):(.*);");
            if (groupFormat.indexIn(input) != -1)
            {
                _name = groupFormat.cap(1).trimmed();
                _address = groupFormat.cap(2).trimmed();
                _group = true;
            }
        }
        else
        {
            parseMailbox(input, _name, _address, _suffix);
            setComponents(_name, _address);
        }
    }
}

QMailAddressPrivate::QMailAddressPrivate(const QString& name, const QString& address) 
    : _group(false),
      _searchCompleted(false)
{
    // See whether the address part contains a group
    if (containsMultipleFields(address))
    {
        _name = name;
        _address = address;
        _group = true;
    }
    else
    {
        setComponents(name, address);
    }
}

void QMailAddressPrivate::setComponents(const QString& nameText, const QString& addressText )
{
    _name = nameText.trimmed();
    _address = addressText.trimmed();

    int charIndex = _address.indexOf( "/TYPE=" );
    if ( charIndex != -1 ) {
        _suffix = _address.mid( charIndex + 6 );
        _address = _address.left( charIndex ).trimmed();
    }

    if ( ( charIndex = _address.indexOf( '<' ) ) != -1 )
        _address.remove( charIndex, 1 );
    if ( ( charIndex = _address.lastIndexOf( '>' ) ) != -1 )
        _address.remove( charIndex, 1 );
}

QMailAddressPrivate::QMailAddressPrivate(const QMailAddressPrivate& other) 
    : QSharedData(other),
      _searchCompleted(false)
{
    _name = other._name;
    _address = other._address;
    _suffix = other._suffix;
    _group = other._group;
}

QMailAddressPrivate::~QMailAddressPrivate()
{
}

bool QMailAddressPrivate::operator==(const QMailAddressPrivate& other) const
{
    return (_name == other._name && _address == other._address && _suffix == other._suffix && _group == other._group);
}

bool QMailAddressPrivate::isNull() const
{
    return (_name.isNull() && _address.isNull() && _suffix.isNull());
}

bool QMailAddressPrivate::isGroup() const
{
    return _group;
}

QList<QMailAddress> QMailAddressPrivate::groupMembers() const
{
    if (_group)
        return QMailAddress::fromStringList(_address);

    return QList<QMailAddress>();
}

// We need to keep a default copy of this, because constructing a new
// one is expensive and sends multiple QCOP messages, whose responses it
// will not survive to receive...

QString QMailAddressPrivate::name() const
{
    return QMail::unquoteString(_name);
}

bool QMailAddressPrivate::isPhoneNumber() const
{
    static const QRegExp pattern(QMailAddress::phoneNumberPattern());
    return pattern.exactMatch(_address);
}

bool QMailAddressPrivate::isEmailAddress() const
{
    static const QRegExp pattern(QMailAddress::emailAddressPattern());
    return pattern.exactMatch(QMailAddress::removeWhitespace(QMailAddress::removeComments(_address)));
}

QString QMailAddressPrivate::minimalPhoneNumber() const
{
    static const QRegExp nondiallingChars("[^\\d,xpwXPW\\+\\*#]");

    // Remove any characters which don't affect dialling
    QString minimal(_address);
    minimal.remove(nondiallingChars);

    // Convert any 'p' or 'x' to comma
    minimal.replace(QRegExp("[xpXP]"), ",");

    // Ensure any permitted alphabetical chars are lower-case
    return minimal.toLower();
}

static bool needsQuotes(const QString& src)
{
    static const QRegExp specials = QRegExp("[<>\\[\\]:;@\\\\,.]");

    QString characters(src);

    // Remove any quoted-pair characters, since they don't require quoting
    int index = 0;
    while ((index = characters.indexOf('\\', index)) != -1)
        characters.remove(index, 2);

    if ( specials.indexIn( characters ) != -1 )
        return true;

    // '(' and ')' also need quoting, if they don't conform to nested comments
    const QChar* it = characters.constData();
    const QChar* const end = it + characters.length();

    int commentDepth = 0;
    for (; it != end; ++it)
        if (*it == '(') {
            ++commentDepth;
        }
        else if (*it == ')') {
            if (--commentDepth < 0)
                return true;
        }

    return (commentDepth != 0);
}

QString QMailAddressPrivate::toString(bool forceDelimited) const
{
    QString result;

    if ( _name == _address )
        return _name;

    if ( _group ) {
        result.append( _name ).append( ": " ).append( _address ).append( ';' );
    } else {
        // If there are any 'special' characters in the name it needs to be quoted
        if ( !_name.isEmpty() )
            result = ( needsQuotes( _name ) ? QMail::quoteString( _name ) : _name );

        if ( !_address.isEmpty() ) {
            if ( !forceDelimited && result.isEmpty() ) {
                result = _address;
            } else {
                if ( !result.isEmpty() )
                    result.append( ' ' );
                result.append( '<' ).append( _address ).append( '>' );
            }
        }

        if ( !_suffix.isEmpty() )
            result.append( " /TYPE=" ).append( _suffix );
    }

    return result;
}

template <typename Stream> 
void QMailAddressPrivate::serialize(Stream &stream) const
{
    stream << _name << _address << _suffix << _group;
}

template <typename Stream> 
void QMailAddressPrivate::deserialize(Stream &stream)
{
    _searchCompleted = false;
    stream >> _name >> _address >> _suffix >> _group;
}


/*!
    \class QMailAddress

    \brief The QMailAddress class provides an interface for manipulating message address strings.
    \ingroup messaginglibrary

    QMailAddress provides functions for splitting the address strings of messages into name and
    address components, and for combining the individual components into correctly formatted
    address strings.  QMailAddress can be used to manipulate the address elements exposed by the 
    QMailMessage class.

    Address strings are expected to use the format "name_part '<'address_part'>'", where 
    \i name_part describes a message sender or recipient and \i address_part defines the address 
    at which they can be contacted.  The address component is not validated, so it can contain an 
    email address, phone number, or any other type of textual address representation.

    \sa QMailMessage
*/

/*!
    Constructs an empty QMailAddress object.
*/
QMailAddress::QMailAddress()
{
    d = new QMailAddressPrivate();
}

/*!
    Constructs a QMailAddress object, extracting the name and address components from \a addressText.

    If \a addressText cannot be separated into name and address components, both name() and address() 
    will return the entirety of \a addressText.

    \sa name(), address()
*/
QMailAddress::QMailAddress(const QString& addressText)
{
    d = new QMailAddressPrivate(addressText);
}

/*!
    Constructs a QMailAddress object with the given \a name and \a address.
*/
QMailAddress::QMailAddress(const QString& name, const QString& address)
{
    d = new QMailAddressPrivate(name, address);
}

/*! \internal */
QMailAddress::QMailAddress(const QMailAddress& other)
{
    this->operator=(other);
}

/*!
    Destroys a QMailAddress object.
*/
QMailAddress::~QMailAddress()
{
}

/*! \internal */
const QMailAddress& QMailAddress::operator= (const QMailAddress& other)
{
    d = other.d;
    return *this;
}

/*! \internal */
bool QMailAddress::operator== (const QMailAddress& other) const
{
    return d->operator==(*other.d);
}

/*! \internal */
bool QMailAddress::operator!= (const QMailAddress& other) const
{
    return !(d->operator==(*other.d));
}

/*!
    Returns true if the address object has not been initialized.
*/
bool QMailAddress::isNull() const
{
    return d->isNull();
}

/*!
    Returns the name component of a mail address string.
*/
QString QMailAddress::name() const
{
    return d->name();
}

/*!
    Returns the address component of a mail address string.
*/
QString QMailAddress::address() const
{
    return d->_address;
}

/*!
    Returns true if the address is that of a group.
*/
bool QMailAddress::isGroup() const
{
    return d->isGroup();
}

/*!
    Returns a list containing the individual addresses that comprise the address group.  
    If the address is not a group address, an empty list is returned.

    \sa isGroup()
*/
QList<QMailAddress> QMailAddress::groupMembers() const
{
    return d->groupMembers();
}

/*!
    Returns true if the address component has the form of a phone number; otherwise returns false.

    \sa isEmailAddress()
*/
bool QMailAddress::isPhoneNumber() const
{
    return d->isPhoneNumber();
}

/*!
    Returns true if the address component has the form of an email address; otherwise returns false.

    \sa isPhoneNumber()
*/
bool QMailAddress::isEmailAddress() const
{
    return d->isEmailAddress();
}

/*! \internal */
QString QMailAddress::minimalPhoneNumber() const
{
    return d->minimalPhoneNumber();
}

/*!
    Returns a string containing the name and address in a standardised format.
    If \a forceDelimited is true, the address element will be delimited by '<' and '>', even if this is unnecessary.
*/
QString QMailAddress::toString(bool forceDelimited) const
{
    return d->toString(forceDelimited);
}

/*!
    Returns a string list containing the result of calling toString() on each address in \a list.
    If \a forceDelimited is true, the address elements will be delimited by '<' and '>', even if this is unnecessary.

    \sa toString()
*/
QStringList QMailAddress::toStringList(const QList<QMailAddress>& list, bool forceDelimited)
{
    QStringList result;

    foreach (const QMailAddress& address, list)
        result.append(address.toString(forceDelimited));

    return result;
}

/*!
    Returns a list containing a QMailAddress object constructed from each 
    comma-separated address in \a list.
*/
QList<QMailAddress> QMailAddress::fromStringList(const QString& list)
{
    return fromStringList(generateAddressList(list));
}

/*!
    Returns a list containing a QMailAddress object constructed from each 
    address string in \a list.
*/
QList<QMailAddress> QMailAddress::fromStringList(const QStringList& list)
{
    QList<QMailAddress> result;

    foreach (const QString& address, list)
        result.append(QMailAddress(address));

    return result;
}

/*!
    Returns the content of \a input with any comment sections removed.
    Any subsequent leading or trailing whitespace is then also removed.
*/
QString QMailAddress::removeComments(const QString& input)
{
    return ::removeComments(input, &QChar::isPrint).trimmed();
}

/*!
    Returns the content of \a input with any whitespace characters removed.
    Whitespace within quotes or comment sections is preserved.
*/
QString QMailAddress::removeWhitespace(const QString& input)
{
    return ::removeWhitespace(input);
}

/*! \internal */
QString QMailAddress::phoneNumberPattern()
{
    static const QString pattern("\"?"                              // zero-or-one:'"'
                                 "("                                // start capture
                                 "(?:\\+ ?)?"                       // zero-or-one:('+', zero-or-one:space)
                                 "(?:\\(\\d+\\)[ -]?)?"             // zero-or-one:'(', one-or-more:digits, ')', zero-or-one:separator 
                                 "(?:\\d{1,14})"                    // one:(one-to-fourteen):digits
                                 "(?:[ -]?[\\d#\\*]{1,10}){0,4}"    // zero-to-four:(zero-or-one:separator), one-to-ten:(digits | '#' | '*')
                                 "(?:"                              // zero-or-one:
                                     "[ -]?"                            // zero-or-one:separator,
                                     "\\(?"                             // zero-or-one:'('
                                     "[,xpwXPW]\\d{1,4}"                // one:extension, one-to-four:digits
                                     "\\)?"                             // zero-or-one:')'
                                 ")?"                               // end of optional group
                                 ")"                                // end capture
                                 "\"?");                            // zero-or-one:'"'

    return pattern;
}

/*! \internal */
QString QMailAddress::emailAddressPattern()
{
    // Taken from: http://www.regular-expressions.info/email.html, but 
    // modified to accept uppercase characters as well as lower-case
    // Also - RFC 1034 seems to prohibit domain name elements beginning
    // with digits, but they exist in practise...
    static const QString pattern("[A-Za-z\\d!#$%&'*+/=?^_`{|}~-]+"      // one-or-more: legal chars (some punctuation permissible)
                                 "(?:"                                  // zero-or-more: 
                                     "\\."                                  // '.',
                                     "[A-Za-z\\d!#$%&'*+/=?^_`{|}~-]+"      // one-or-more: legal chars
                                 ")*"                                   // end of optional group
                                 "@"                                    // '@'
                                 "(?:"                                  // either:
                                     "localhost"                            // 'localhost'
                                 "|"                                    // or:
                                     "(?:"                                  // one-or-more: 
                                         "[A-Za-z\\d]"                          // one: legal char, 
                                         "(?:"                                  // zero-or-one:
                                             "[A-Za-z\\d-]*[A-Za-z\\d]"             // (zero-or-more: (legal char or '-'), one: legal char)
                                         ")?"                                   // end of optional group
                                         "\\."                                  // '.'
                                     ")+"                                   // end of mandatory group
                                     "[A-Za-z\\d]"                          // one: legal char
                                     "(?:"                                  // zero-or-one:
                                         "[A-Za-z\\d-]*[A-Za-z\\d]"             // (zero-or-more: (legal char or '-'), one: legal char)
                                     ")?"                                   // end of optional group
                                 ")");                                  // end of alternation

    return pattern;
}

/*! 
    \fn QMailAddress::serialize(Stream&) const
    \internal 
*/
template <typename Stream> 
void QMailAddress::serialize(Stream &stream) const
{
    d->serialize(stream);
}

/*! 
    \fn QMailAddress::deserialize(Stream&)
    \internal 
*/
template <typename Stream> 
void QMailAddress::deserialize(Stream &stream)
{
    d->deserialize(stream);
}

Q_IMPLEMENT_USER_METATYPE(QMailAddress)

Q_IMPLEMENT_USER_METATYPE_TYPEDEF(QMailAddressList, QMailAddressList)


//Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(QList<QMailAddress>)

