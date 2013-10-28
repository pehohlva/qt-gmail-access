/* 
 * File:   allconfig.h
 * Author: pro
 *
 * Created on 24. Oktober 2013, 17:18
 */

#ifndef ALLCONFIG_H
#define	ALLCONFIG_H

#define _CVERSION_ \
             QString("V2510-")

#define __TMPCACHE__ \
             QString("%1/.fastcache/").arg(QDir::homePath())


/// bind format doc having
#include "docformat/ooo/document.h"
#include "docformat/ooo/converter.h"
#include "docformat/ooo/styleparser.h"
#include "docformat/ooo/styleinformation.h"
#include "docformat/ooo/formatproperty.h"
#include "tools/kzip.h"
#include "tools/docmargin.h"
/// bind format doc having

#include <QtXml/QDomDocument>
#include <QtGui/QTextCharFormat>
#include <QPointer>
#include <QFileInfo>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QFontComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMenu>
#include <QMenuBar>

#include <QTextCodec>
#include <QTextEdit>
#include <QToolBar>
#include <QTextCursor>
#include <QTextList>
#include <QtDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QTextDocumentWriter>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QFontComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMenu>
#include <QMenuBar>
#include <QTextCodec>
#include <QTextEdit>
#include <QToolBar>
#include <QTextCursor>
#include <QTextList>
#include <QtDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QGridLayout>

#include <QStatusBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkDiskCache>
#include <QTextDocumentFragment>
#include <QMimeData>

#if defined _HAVEPRINTSUPPORTFLAG_
/// qt5 compatible ??? //// on qmake file !!! QT += printsupport
#include <QtPrintSupport/qprinter.h>
#include <QtPrintSupport/qprintdialog.h>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtPrintSupport/QPrintDialog>
#endif
#if defined _QT4PRINTERSUPPORT_
/// before qt5
#include <QPrintPreviewDialog>
#include <QPrintDialog>
#include <qprinter.h>
#endif









#endif	/* ALLCONFIG_H */

