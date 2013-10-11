//
// C++ Implementation: sample to send mail on smtp from gmail
//
// Description:
// SMTP Encrypted sending mail attachment & mime chunk Editmail->document()
// ( QTextDocument ) fill from image e html tag & style color full mime mail.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef GMAILSMS_H
#define GMAILSMS_H

#include <QObject>

class gmailsms : public QObject {
    Q_OBJECT
public:
    explicit gmailsms(QObject *parent = 0);

signals:

public slots:

};

#endif // GMAILSMS_H
