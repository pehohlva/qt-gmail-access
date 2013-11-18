/* 
 * File:   rtfhtml.cpp
 * Author: pro
 * Struzzo bello in fare le cose
 * Created on 17. November 2013, 22:27
 */

#include "rtfhtml.h"

#include <QFile>
#include <QTextBlock>
#include <QTextCodec>
#include <QTextDecoder>

RtfDocument::RtfDocument(const QString docfilename) : Document("RTF"), is_loading(false) {

    QFileInfo infofromfile(docfilename);
    const QString indexhtmlfile = QString("%1index.html").arg(cache_dir);
    bool debugmodus = (DEBUGMODUS == 1) ? true : false;
    console(QString("Init talking this class. %1..").arg(this->name()), 0);
    bool doc_continue = false;
    console(QString("OpenFile: %1").arg(docfilename));
    //// this->readfile(docfilename); /// open on stream 
    QFile file(docfilename);
    /// result write to:
    if (file.open(QIODevice::ReadOnly)) {
        RTF::Reader reader;
        console(QString("device is open yesss ... {%1}   ").arg(DEBUGMODUS));
        QTextDocument *doc = new QTextDocument();
        QTextCursor cursor(doc);
        reader.read(&file, cursor);
        QByteArray m_codepage = reader.codePage();
        file.close();
        HTMLSTREAM = doc->toHtml("utf-8");
        is_loading = Tools::_write_file(indexhtmlfile, HTMLSTREAM, "utf-8");
        if (is_loading) {
            console(QString("Write to file ... %1   ").arg(indexhtmlfile));
        }
        if (reader.hasError()) {
            console(QString("device Error reader.errorString() {%1}   ").arg(reader.errorString()));
        }
        /// console(QString("device is open yesss ... {%1}   ").arg( QString(m_codepage.constData() )));
    }
    console(QString("End Rtf line ...."), 3);
    /*
    QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
                if (file.peek(2) == "PK") {
                        file.seek(30);
                        if (file.read(47) == "mimetypeapplication/vnd.oasis.opendocument.text") {
                                type = "odt";
                        }
                        file.reset();
                } else if (file.peek(5) == "{\\rtf") {
                        type = "rtf";
                }
        }
     */



    ////const QString indexhtmlfile = QString("%1index.html").arg(cache_dir);

}

RtfDocument::~RtfDocument() {

}



//-----------------------------------------------------------------------------

namespace {

    class Function {
    public:

        Function(void (RTF::Reader::*func)(qint32) = 0, qint32 value = 0)
        : m_func(func),
        m_value(value) {
        }

        void call(RTF::Reader* reader, const RTF::Tokenizer& token) const {
            (reader->*m_func)(token.hasValue() ? token.value() : m_value);
        }

    private:
        void (RTF::Reader::*m_func)(qint32);
        qint32 m_value;
    };
    QHash<QByteArray, Function> functions;

    QTextCodec* codecForCodePage(qint32 value, QByteArray* codepage = 0) {
        QByteArray name = "CP" + QByteArray::number(value);
        QByteArray codec;
        if (value == 932) {
            codec = "Shift-JIS";
        } else if (value == 10000) {
            codec = "Apple Roman";
        } else if (value == 65001) {
            codec = "UTF-8";
        } else {
            codec = name;
        }
        if (codepage) {
            *codepage = name;
        }
        return QTextCodec::codecForName(codec);
    }
}





//-----------------------------------------------------------------------------

RTF::Reader::Reader()
: m_in_block(true),
m_codec(0),
m_decoder(0) {
    if (functions.isEmpty()) {
        functions["\\"] = Function(&Reader::insertSymbol, '\\');
        functions["_"] = Function(&Reader::insertSymbol, 0x2011);
        functions["{"] = Function(&Reader::insertSymbol, '{');
        functions["|"] = Function(&Reader::insertSymbol, 0x00b7);
        functions["}"] = Function(&Reader::insertSymbol, '}');
        functions["~"] = Function(&Reader::insertSymbol, 0x00a0);
        functions["-"] = Function(&Reader::insertSymbol, 0x00ad);

        functions["bullet"] = Function(&Reader::insertSymbol, 0x2022);
        functions["emdash"] = Function(&Reader::insertSymbol, 0x2014);
        functions["emspace"] = Function(&Reader::insertSymbol, 0x2003);
        functions["endash"] = Function(&Reader::insertSymbol, 0x2013);
        functions["enspace"] = Function(&Reader::insertSymbol, 0x2002);
        functions["ldblquote"] = Function(&Reader::insertSymbol, 0x201c);
        functions["lquote"] = Function(&Reader::insertSymbol, 0x2018);
        functions["line"] = Function(&Reader::insertSymbol, 0x2028);
        functions["ltrmark"] = Function(&Reader::insertSymbol, 0x200e);
        functions["qmspace"] = Function(&Reader::insertSymbol, 0x2004);
        functions["rdblquote"] = Function(&Reader::insertSymbol, 0x201d);
        functions["rquote"] = Function(&Reader::insertSymbol, 0x2019);
        functions["rtlmark"] = Function(&Reader::insertSymbol, 0x200f);
        functions["tab"] = Function(&Reader::insertSymbol, 0x0009);
        functions["zwj"] = Function(&Reader::insertSymbol, 0x200d);
        functions["zwnj"] = Function(&Reader::insertSymbol, 0x200c);

        functions["\'"] = Function(&Reader::insertHexSymbol);
        functions["u"] = Function(&Reader::insertUnicodeSymbol);
        functions["uc"] = Function(&Reader::setSkipCharacters);
        functions["par"] = Function(&Reader::endBlock);
        functions["\n"] = Function(&Reader::endBlock);
        functions["\r"] = Function(&Reader::endBlock);

        functions["pard"] = Function(&Reader::resetBlockFormatting);
        functions["plain"] = Function(&Reader::resetTextFormatting);

        functions["qc"] = Function(&Reader::setBlockAlignment, Qt::AlignHCenter);
        functions["qj"] = Function(&Reader::setBlockAlignment, Qt::AlignJustify);
        functions["ql"] = Function(&Reader::setBlockAlignment, Qt::AlignLeft | Qt::AlignAbsolute);
        functions["qr"] = Function(&Reader::setBlockAlignment, Qt::AlignRight | Qt::AlignAbsolute);

        functions["li"] = Function(&Reader::setBlockIndent);

        functions["ltrpar"] = Function(&Reader::setBlockDirection, Qt::LeftToRight);
        functions["rtlpar"] = Function(&Reader::setBlockDirection, Qt::RightToLeft);

        functions["b"] = Function(&Reader::setTextBold, true);
        functions["i"] = Function(&Reader::setTextItalic, true);
        functions["strike"] = Function(&Reader::setTextStrikeOut, true);
        functions["striked"] = Function(&Reader::setTextStrikeOut, true);
        functions["ul"] = Function(&Reader::setTextUnderline, true);
        functions["uld"] = Function(&Reader::setTextUnderline, true);
        functions["uldash"] = Function(&Reader::setTextUnderline, true);
        functions["uldashd"] = Function(&Reader::setTextUnderline, true);
        functions["uldb"] = Function(&Reader::setTextUnderline, true);
        functions["ulnone"] = Function(&Reader::setTextUnderline, false);
        functions["ulth"] = Function(&Reader::setTextUnderline, true);
        functions["ulw"] = Function(&Reader::setTextUnderline, true);
        functions["ulwave"] = Function(&Reader::setTextUnderline, true);
        functions["ulhwave"] = Function(&Reader::setTextUnderline, true);
        functions["ululdbwave"] = Function(&Reader::setTextUnderline, true);

        functions["sub"] = Function(&Reader::setTextVerticalAlignment, QTextCharFormat::AlignSubScript);
        functions["super"] = Function(&Reader::setTextVerticalAlignment, QTextCharFormat::AlignSuperScript);
        functions["nosupersub"] = Function(&Reader::setTextVerticalAlignment, QTextCharFormat::AlignNormal);

        functions["ansicpg"] = Function(&Reader::setCodepage);
        functions["ansi"] = Function(&Reader::setCodepage, 1252);
        functions["mac"] = Function(&Reader::setCodepage, 10000);
        functions["pc"] = Function(&Reader::setCodepage, 850);
        functions["pca"] = Function(&Reader::setCodepage, 850);

        functions["deff"] = Function(&Reader::setFont);
        functions["f"] = Function(&Reader::setFont);
        functions["cpg"] = Function(&Reader::setFontCodepage);
        functions["fcharset"] = Function(&Reader::setFontCharset);

        functions["filetbl"] = Function(&Reader::ignoreGroup);
        functions["colortbl"] = Function(&Reader::ignoreGroup);
        functions["fonttbl"] = Function(&Reader::ignoreText);
        functions["stylesheet"] = Function(&Reader::ignoreGroup);
        functions["info"] = Function(&Reader::ignoreGroup);
        functions["*"] = Function(&Reader::ignoreGroup);
    }

    m_state.ignore_control_word = false;
    m_state.ignore_text = false;
    m_state.skip = 1;
    m_state.active_codepage = 0;

    setCodepage(1252);
}

//-----------------------------------------------------------------------------

RTF::Reader::~Reader() {
    delete m_decoder;
}

//-----------------------------------------------------------------------------

QByteArray RTF::Reader::codePage() const {
    return m_codepage_name;
}

//-----------------------------------------------------------------------------

QString RTF::Reader::errorString() const {
    return m_error;
}

//-----------------------------------------------------------------------------

bool RTF::Reader::hasError() const {
    return !m_error.isEmpty();
}

//-----------------------------------------------------------------------------

void RTF::Reader::read(QIODevice* device, const QTextCursor& cursor) {
    try {
        // Use theme spacings
        m_block_format = cursor.blockFormat();
        m_state.block_format = m_block_format;

        // Open file
        m_cursor = cursor;
        m_cursor.beginEditBlock();
        m_token.setDevice(device);
        setBlockDirection(Qt::LeftToRight);

        // Check file type
        m_token.readNext();
        if (m_token.type() == StartGroupToken) {
            pushState();
        } else {
            throw i18n("Not a supported RTF file.");
        }
        m_token.readNext();
        if (m_token.type() != ControlWordToken || m_token.text() != "rtf" || m_token.value() != 1) {
            throw i18n("Not a supported RTF file.");
        }

        // Parse file contents
        while (!m_states.isEmpty() && m_token.hasNext()) {
            m_token.readNext();

            if ((m_token.type() != EndGroupToken) && !m_in_block) {
                m_cursor.insertBlock(m_state.block_format);
                m_in_block = true;
            }

            if (m_token.type() == StartGroupToken) {
                pushState();
            } else if (m_token.type() == EndGroupToken) {
                popState();
            } else if (m_token.type() == ControlWordToken) {
                if (!m_state.ignore_control_word && functions.contains(m_token.text())) {
                    functions[m_token.text()].call(this, m_token);
                }
            } else if (m_token.type() == TextToken) {
                if (!m_state.ignore_text) {
                    m_cursor.insertText(m_decoder->toUnicode(m_token.text()));
                }
            }
        }
    } catch (const QString& error) {
        m_error = error;
    }
    m_cursor.endEditBlock();
}

//-----------------------------------------------------------------------------

void RTF::Reader::endBlock(qint32) {
    m_in_block = false;
}

//-----------------------------------------------------------------------------

void RTF::Reader::ignoreGroup(qint32) {
    m_state.ignore_control_word = true;
    m_state.ignore_text = true;
}

//-----------------------------------------------------------------------------

void RTF::Reader::ignoreText(qint32) {
    m_state.ignore_text = true;
}

//-----------------------------------------------------------------------------

void RTF::Reader::insertHexSymbol(qint32) {
    m_cursor.insertText(m_decoder->toUnicode(m_token.hex()));
}

//-----------------------------------------------------------------------------

void RTF::Reader::insertSymbol(qint32 value) {
    m_cursor.insertText(QChar(value));
}

//-----------------------------------------------------------------------------

void RTF::Reader::insertUnicodeSymbol(qint32 value) {
    m_cursor.insertText(QChar(value));

    for (int i = m_state.skip; i > 0;) {
        m_token.readNext();

        if (m_token.type() == TextToken) {
            int len = m_token.text().count();
            if (len > i) {
                m_cursor.insertText(m_decoder->toUnicode(m_token.text().mid(i)));
                break;
            } else {
                i -= len;
            }
        } else if (m_token.type() == ControlWordToken) {
            --i;
        } else if (m_token.type() == StartGroupToken) {
            pushState();
            break;
        } else if (m_token.type() == EndGroupToken) {
            popState();
            break;
        }
    }
}

//-----------------------------------------------------------------------------

void RTF::Reader::pushState() {
    m_states.push(m_state);
}

//-----------------------------------------------------------------------------

void RTF::Reader::popState() {
    if (m_states.isEmpty()) {
        return;
    }
    m_state = m_states.pop();
    m_cursor.setCharFormat(m_state.char_format);
    setFont(m_state.active_codepage);
}

//-----------------------------------------------------------------------------

void RTF::Reader::resetBlockFormatting(qint32) {
    m_state.block_format = m_block_format;
    m_cursor.setBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::resetTextFormatting(qint32) {
    m_state.char_format = QTextCharFormat();
    m_cursor.setCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setBlockAlignment(qint32 value) {
    m_state.block_format.setAlignment(Qt::Alignment(value));
    m_cursor.mergeBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setBlockDirection(qint32 value) {
    m_state.block_format.setLayoutDirection(Qt::LayoutDirection(value));
    Qt::Alignment alignment = m_state.block_format.alignment();
    if (alignment & Qt::AlignLeft) {
        alignment |= Qt::AlignAbsolute;
        m_state.block_format.setAlignment(alignment);
    }
    m_cursor.mergeBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setBlockIndent(qint32 value) {
    m_state.block_format.setIndent(value / 15);
    m_cursor.mergeBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setTextBold(qint32 value) {
    m_state.char_format.setFontWeight(value ? QFont::Bold : QFont::Normal);
    m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setTextItalic(qint32 value) {
    m_state.char_format.setFontItalic(value);
    m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setTextStrikeOut(qint32 value) {
    m_state.char_format.setFontStrikeOut(value);
    m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setTextUnderline(qint32 value) {
    m_state.char_format.setFontUnderline(value);
    m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setTextVerticalAlignment(qint32 value) {
    m_state.char_format.setVerticalAlignment(QTextCharFormat::VerticalAlignment(value));
    m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setSkipCharacters(qint32 value) {
    m_state.skip = value;
}

//-----------------------------------------------------------------------------

void RTF::Reader::setCodepage(qint32 value) {
    QByteArray codepage;
    QTextCodec* codec = codecForCodePage(value, &codepage);
    if (codec != 0) {
        m_codepage = codec;
        m_codepage_name = codepage;
        setCodec(codec);
    }
}

//-----------------------------------------------------------------------------

void RTF::Reader::setFont(qint32 value) {
    m_state.active_codepage = value;

    if (value < m_codepages.count()) {
        setCodec(m_codepages[value]);
    } else {
        setCodec(0);
        m_codepages.resize(value + 1);
    }

    if (m_codec == 0) {
        setCodec(m_codepage);
    }
}

//-----------------------------------------------------------------------------

void RTF::Reader::setFontCodepage(qint32 value) {
    if (m_state.active_codepage >= m_codepages.count()) {
        m_state.ignore_control_word = true;
        m_state.ignore_text = true;
        return;
    }

    QTextCodec* codec = codecForCodePage(value);
    if (codec != 0) {
        m_codepages[m_state.active_codepage] = codec;
        setCodec(codec);
    }
    m_state.ignore_control_word = true;
    m_state.ignore_text = true;
}

//-----------------------------------------------------------------------------

void RTF::Reader::setFontCharset(qint32 value) {
    if (m_state.active_codepage >= m_codepages.count()) {
        m_state.ignore_text = true;
        return;
    }

    if (m_codepages[m_state.active_codepage] != 0) {
        setCodec(m_codepages[m_state.active_codepage]);
        m_state.ignore_text = true;
        return;
    }

    QByteArray charset;
    switch (value) {
        case 0: charset = "CP1252";
            break;
        case 1: charset = "CP1252";
            break;
        case 77: charset = "Apple Roman";
            break;
        case 128: charset = "Shift-JIS";
            break;
        case 129: charset = "eucKR";
            break;
        case 130: charset = "CP1361";
            break;
        case 134: charset = "GB2312";
            break;
        case 136: charset = "Big5-HKSCS";
            break;
        case 161: charset = "CP1253";
            break;
        case 162: charset = "CP1254";
            break;
        case 163: charset = "CP1258";
            break;
        case 177: charset = "CP1255";
            break;
        case 178: charset = "CP1256";
            break;
        case 186: charset = "CP1257";
            break;
        case 204: charset = "CP1251";
            break;
        case 222: charset = "CP874";
            break;
        case 238: charset = "CP1250";
            break;
        case 255: charset = "CP850";
            break;
        default: return;
    }

    QTextCodec* codec = QTextCodec::codecForName(charset);
    if (codec != 0) {
        m_codepages[m_state.active_codepage] = codec;
        setCodec(codec);
    }
    m_state.ignore_text = true;
}

//-----------------------------------------------------------------------------

void RTF::Reader::setCodec(QTextCodec* codec) {
    if (m_codec != codec) {
        m_codec = codec;
        if (m_codec) {
            delete m_decoder;
            m_decoder = m_codec->makeDecoder();
        }
    }
}

//-----------------------------------------------------------------------------

RTF::Tokenizer::Tokenizer()
: m_device(0),
m_position(0),
m_value(0),
m_has_value(false) {
    m_buffer.reserve(8192);
    m_text.reserve(8192);
}

//-----------------------------------------------------------------------------

bool RTF::Tokenizer::hasNext() const {
    return (m_position < m_buffer.size() - 1) || !m_device->atEnd();
}

//-----------------------------------------------------------------------------

void RTF::Tokenizer::readNext() {
    // Reset values
    m_type = TextToken;
    m_hex.clear();
    m_text.resize(0);
    m_value = 0;
    m_has_value = false;
    if (!m_device) {
        return;
    }

    // Read first character
    char c;
    do {
        c = next();
    } while (c == '\n' || c == '\r');

    // Determine token type
    if (c == '{') {
        m_type = StartGroupToken;
    } else if (c == '}') {
        m_type = EndGroupToken;
    } else if (c == '\\') {
        m_type = ControlWordToken;

        c = next();

        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            // Read control word
            while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                m_text.append(c);
                c = next();
            }

            // Read integer value
            int sign = (c != '-') ? 1 : -1;
            if (sign == -1) {
                c = next();
            }
            QByteArray value;
            while (isdigit(c)) {
                value.append(c);
                c = next();
            }
            m_has_value = !value.isEmpty();
            m_value = value.toInt() * sign;

            // Eat space after control word
            if (c != ' ') {
                --m_position;
            }

            // Eat binary value
            if (m_text == "bin") {
                if (m_value > 0) {
                    for (int i = 0; i < m_value; i++) {
                        c = next();
                    }
                }
                return readNext();
            }
        } else if (c == '\'') {
            // Read hexadecimal value
            m_text.append(c);
            QByteArray hex(2, 0);
            hex[0] = next();
            hex[1] = next();
            m_hex.append(hex.toInt(0, 16));
        } else {
            // Read escaped character
            m_text.append(c);
        }
    } else {
        // Read text
        m_type = TextToken;
        while (c != '\\' && c != '{' && c != '}' && c != '\n' && c != '\r') {
            m_text.append(c);
            c = next();
        }
        m_position--;
    }
}

//-----------------------------------------------------------------------------

void RTF::Tokenizer::setDevice(QIODevice* device) {
    m_device = device;
}

//-----------------------------------------------------------------------------

char RTF::Tokenizer::next() {
    m_position++;
    if (m_position >= m_buffer.size()) {
        m_buffer.resize(8192);
        int size = m_device->read(m_buffer.data(), m_buffer.size());
        if (size < 1) {
            throw i18n("Unexpectedly reached end of file.");
        }
        m_buffer.resize(size);
        m_position = 0;
        /// QApplication::processEvents();
        //// ::instance()->processEvents();
    }
    return m_buffer.at(m_position);
}


