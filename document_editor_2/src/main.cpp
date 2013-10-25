#include "textedit.h"
#include <QApplication>
#include "os_application.h"
#include "allconfig.h"

//// /Users/pro/Desktop/Qt_code/document_editor_2/

#if defined _COMPOSE_STATIC_
#include <QtPlugin>
#if defined _USE_qjpeg

Q_IMPORT_PLUGIN(qjpeg)
#endif
#if defined _USE_qmng
Q_IMPORT_PLUGIN(qmng)
#endif
#if defined _USE_qgif
Q_IMPORT_PLUGIN(qgif)
#endif
#if defined _USE_qtiff
Q_IMPORT_PLUGIN(qtiff)
#endif
#endif

int main(int argc, char *argv[]) {
    ////Q_INIT_RESOURCE("textedit");
    //////Q_INIT_RESOURCE(osApplication);  /* if you have icon and file osApplication.qrc to load before*/
    OS_application a(argc, argv);
    a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
    return a.exec();
} 