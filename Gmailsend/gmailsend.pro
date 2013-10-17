#-------------------------------------------------
#
# Project created by QtCreator 2013-09-18T12:13:17
#         console debug 
#
#-------------------------------------------------

cache()

QT       += core gui xml network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = gmailsend
TEMPLATE = app
DESTDIR	+= ./


CONFIG +=   warn_on \
            qt \
            silent \
            thread 

#########CONFIG += debug  release
CONFIG += debug

 INCLUDEPATH += /Users/pro/qt/qt51/qtbase/include



## CONFIG-=app_bundle

SOURCES += main.cpp\
        mwindow.cpp \
    gmailsms.cpp \
    mailformat.cpp \
    qwwsmtpclient.cpp

HEADERS  += mwindow.h \
    gmailsms.h \
    mime_type.h \
    mailformat.h \
    qwwsmtpclient.h

FORMS    += mwindow.ui


 MOC_DIR =  .moc
 OBJECTS_DIR =  .obj
 UI_DIR =  .ui

RESOURCES += box.qrc







