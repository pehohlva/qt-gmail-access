/* 
 * File:   rtfhtml.h
 * Author: pro
 *
 * Created on 17. November 2013, 22:27
 */

#ifndef RTFHTML_H
#define	RTFHTML_H

#include "kzip.h"
#include "config.h"
#include "document.h"
#include <QIODevice>


namespace RTF {

    enum TokenType {
        StartGroupToken,
        EndGroupToken,
        ControlWordToken,
        TextToken
    };

    class Tokenizer {
       

    public:
        Tokenizer();

        bool hasNext() const;
        bool hasValue() const;
        QByteArray hex() const;
        QByteArray text() const;
        TokenType type() const;
        qint32 value() const;

        void readNext();
        void setDevice(QIODevice* device);

    private:
        char next();

    private:
        QIODevice* m_device;
        QByteArray m_buffer;
        int m_position;

        TokenType m_type;
        QByteArray m_hex;
        QByteArray m_text;
        qint32 m_value;
        bool m_has_value;
    };

    inline bool Tokenizer::hasValue() const {
        return m_has_value;
    }

    inline QByteArray Tokenizer::hex() const {
        return m_hex;
    }

    inline QByteArray Tokenizer::text() const {
        return m_text;
    }

    inline TokenType Tokenizer::type() const {
        return m_type;
    }

    inline qint32 Tokenizer::value() const {
        return m_value;
    }

    class Reader {
      

    public:
        Reader();
        ~Reader();

        QByteArray codePage() const;
        QString errorString() const;
        bool hasError() const;

        void read(QIODevice* device, const QTextCursor& cursor);

    private:
        void endBlock(qint32);
        void ignoreGroup(qint32);
        void ignoreText(qint32);
        void insertHexSymbol(qint32);
        void insertSymbol(qint32 value);
        void insertUnicodeSymbol(qint32 value);
        void pushState();
        void popState();
        void resetBlockFormatting(qint32);
        void resetTextFormatting(qint32);
        void setBlockAlignment(qint32 value);
        void setBlockDirection(qint32 value);
        void setBlockIndent(qint32 value);
        void setTextBold(qint32 value);
        void setTextItalic(qint32 value);
        void setTextStrikeOut(qint32 value);
        void setTextUnderline(qint32 value);
        void setTextVerticalAlignment(qint32 value);
        void setSkipCharacters(qint32 value);
        void setCodepage(qint32 value);
        void setFont(qint32 value);
        void setFontCharset(qint32 value);
        void setFontCodepage(qint32 value);
        void setCodec(QTextCodec* codec);

    private:
        Tokenizer m_token;
        bool m_in_block;

        struct State {
            QTextBlockFormat block_format;
            QTextCharFormat char_format;
            bool ignore_control_word;
            bool ignore_text;
            int skip;
            int active_codepage;
        };
        QStack<State> m_states;
        State m_state;
        QTextBlockFormat m_block_format;

        QTextCodec* m_codec;
        QTextDecoder* m_decoder;
        QTextCodec* m_codepage;
        QVector<QTextCodec*> m_codepages;
        QByteArray m_codepage_name;

        QString m_error;

        QTextCursor m_cursor;
    };
}

class RtfDocument : public Document {
public:
    RtfDocument( const QString docfilename );
    virtual ~RtfDocument();
private:
    bool is_loading;
    QString HTMLSTREAM;
};

#endif	/* RTFHTML_H */

