QT       += core
QT	 -= gui

TARGET = $$qtLibraryTarget(RAMHashTables)
TEMPLATE = lib
CONFIG += plugin
CONFIG += debug

DESTDIR = ../

INCLUDEPATH += ../..

SOURCES += mapgraphplugin.cpp \
           ../../identifier.cpp \
           ../../metadata.cpp

HEADERS += mapgraphplugin.h \
           ../../identifier.h \
           ../../metadata.h
