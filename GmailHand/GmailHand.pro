#-------------------------------------------------
# d /Users/pro/project/GmailHand/
# Project created by QtCreator 2013-09-19T14:20:54
# qmake -spec /usr/local/Qt4.8/mkspecs/macx-g++ -o Makefile GmailHand.pro
# cd /Users/pro/project/version_sv/GmailHand
#-------------------------------------------------

QT       += core  core network xml
QT       -= gui

TARGET = getmesg
CONFIG   += debug
CONFIG += qt console x86_64
CONFIG -= app_bundle
TEMPLATE = app

CONFIG +=   warn_on \
            qt \
            silent \
            thread 

LIBS += -lz

DEFINES += MY_SOCKET_DEBUG



DEFINES += DEFLATE_PLAY_YES
DEFINES += DRUPAL_REMOTE  
# DEFINES += RETRYCASE_PUT_TRASH  

macx {
# DEFINES += USE_FANCY_MATCH_ALGORITHM
# mac core native  function full window size on widget ?? 
LIBS += -framework CoreFoundation
LIBS += -framework IOKit
}

SOURCES += main.cpp \
    net_starterimap.cpp \
    drupal_journalist.cpp \
    drupal_journalist.cpp \
    net_imap_standard.cpp \
    parser_eml.cpp \
    mail_handler.cpp

HEADERS += \
    net_starterimap.h \
    net_imap_standard.h \
    drupal_journalist.h \
    parser_eml.h \
    mime_standard.h \
    drupal_journalist.h \
    mail_handler.h

 MOC_DIR =  .moc
 OBJECTS_DIR =  .obj
 UI_DIR =  .ui
