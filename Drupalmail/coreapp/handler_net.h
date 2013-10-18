//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#ifndef HANDLER_NET_H
#define HANDLER_NET_H

#include <QObject>
#include <QtCore>
#include "net_starterimap.h"
#include "net_imap_standard.h"
#include "parser_config.h"


class Handler_Net : public QObject
{
    Q_OBJECT
public:
    explicit Handler_Net(QObject *parent = 0);
    
signals:
void Full_Quit();
public slots:
     void Init_Parse( const int uid );
     void Close_Imap();
     void NextComand();
private:
    Net_StarterImap *imap;
    
};

#endif // HANDLER_NET_H
