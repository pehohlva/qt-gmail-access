//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#ifndef PARSER_EML_H
#define	PARSER_EML_H

#include "parser_config.h"




namespace ReadMail {

    /// class to handle eml file on full email format
    /// use NetBean to follow function and class  by CTRL window or apple key and click

    class Parser {
    public:
        Parser(const QString file);
        /* return all header field wo find in  init_Header() */
        QString HeaderField(const QString name) const;
        ///// utf8 iso xx preaction from base64 or quoteprintable
        QString Makedecoder(const QByteArray charset , QByteArray chunk , int preaction = 0 ) const;
        /// attachmend inline image ecc..
        // multipart/alternative 200
        // multipart/signed 201
        // multipart/related 202
        // multipart/digest 203
        // multipart/mixed 204
        /// text only 100
        // unknow 404
        
        /// get a list of all boundary= key from mail 
        /// not parse  after Meta header --- if mail attachment having
        /// a ***.eml message/rfc822 file format... 
        /// maximum boundary= is 2x on multipart/mixed && multipart/alternative
        QStringList GetMailKey( bool full = false ) const;
        ~Parser();
        /////  DocSlice MagicPair;
    private:
        void init_Header();
        int Get_MultipartMime( ICmail& qmail) const;
        //// get the text plain and write to ICmail
        int CaptureText( ICmail& qmail , const int mode ) const;
        /// return exact lenght from part next body to extract doc like apple slice 
        /// hear on slice document saved all on struct ICmail && list of attachment on Qmailf
        /// a class to contain all inline element or attachment on mime_standard.h 
        int SliceBodyPart( const int from , const int stop , ICmail& qmail   ) const; 
        /* search by key word to separate correct mail header and body part  
         many mail having to long null line or to many new line to the next boundary section */
        bool Header_Code_Grep(const QString line ) const;
        void RangePosition( QByteArray *in , int pos , int range );
        //// only as debug help fulll
        void take_slice( const int start , const int diff , QByteArray name =  QByteArray("Noname set!")) const;
        int wi_line;
        int MAGIC_POSITION;
        QString Mimelist;
        QByteArray Mheader;
        QByteArray Mfull;
        QByteArray Mfull_A;
        ReadMail::StreamMail *_d;
        Mail_Field_Format field_h;
        
    };


}

#endif	/* PARSER_EML_H */

