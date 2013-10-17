#-------------------------------------------------
#
# Project created by QtCreator 2013-09-18T12:13:17
#         console debug 
#
# compile dir
# /Users/pro/qt/qt51/  // self build 
# /Users/pro/qt/qt5lang/    /Users/pro/qt/qt5lang/  ready to use package
#-------------------------------------------------

INCLUDEPATH += /Users/pro/qt/qt51/qtbase/include
# INCLUDEPATH += /Users/pro/qt/qt5lang/5.1.1/clang_64/include
# INCLUDEPATH += /usr/local/Qt-5.1.1/include 

# cache() /// bug from source build 

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







