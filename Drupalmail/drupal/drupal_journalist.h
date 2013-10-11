/* 
 * File:   drupal_journalist.h
 * Author: pro
 *
 * Created on 3. Oktober 2013, 22:06
 */

#ifndef DRUPAL_JOURNALIST_H
#define	DRUPAL_JOURNALIST_H

#include <QHash>
#include <QString>
#include <QtCore>
#include <QCoreApplication>
#include <QMap>
#include "mime_standard.h"
#include <QDomElement>
//// #include <QTextDocument>
#include <QtCore>
#include <QObject>
#include <QCryptographicHash>

#ifdef DRUPAL_REMOTE
const int sender_host_yes = 1;
#else
const int sender_host_yes = 0;
#endif


const QString DrupalHost = "http://www.freeroad.ch/";







#endif	/* DRUPAL_JOURNALIST_H */

