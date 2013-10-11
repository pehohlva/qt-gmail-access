QT += network \
    xmlpatterns
QT -= gui
TARGET = omapd
CONFIG += console
CONFIG += debug
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    server.cpp \
    identifier.cpp \
    metadata.cpp \
    mapsessions.cpp \
    omapdconfig.cpp \
    maprequest.cpp \
    mapresponse.cpp \
    subscription.cpp \
    clienthandler.cpp \
    clientparser.cpp \
    mapclient.cpp \
    clientconfiguration.cpp \
    managementserver.cpp \
    json.cpp
HEADERS += server.h \
    identifier.h \
    metadata.h \
    mapsessions.h \
    omapdconfig.h \
    maprequest.h \
    mapresponse.h \
    subscription.h \
    mapgraphinterface.h \
    server.h \
    clienthandler.h \
    clientparser.h \
    mapclient.h \
    clientconfiguration.h \
    managementserver.h \
    json.h
INCLUDEPATH += $$[QT_INSTALL_PREFIX]/src/3rdparty/zlib
