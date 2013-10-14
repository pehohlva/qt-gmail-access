//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution


#include "mime_standard.h"
#include "parser_utils.h"

bool Qmailf::TestWriteln(int mode ) {
    ////return true;
    QByteArray data = QByteArray::fromBase64(chunk);
    QString fileout = Utils::testDocAttachmentDir(Filename());
    QString base64fileout = Utils::testDocAttachmentDir(Filename()+QString(".txt"));
    ///Utils::CarriageReturn_;
    if (mode == 1) {
        return Utils::_write_file(fileout,QString(data.constData()),txt_charset);
    } else {
        QString base64data = QString(chunk.constData());
        QString base64 = Utils::_format_string76(base64data);
        bool one = Utils::_write_file(base64fileout,base64,"utf-8");
        if (one) {
          return Utils::_writebin_tofile(fileout,data);  
        } 
    }
    return false;
}


 QString Qmailf::EmbeddedImage() {
        QString base64data = QString(chunk.constData());
        QString isrc = QString("data:%1;base64,").arg( Mime() );
        isrc.append(base64data);
        return Utils::_format_string76(isrc);
    }


