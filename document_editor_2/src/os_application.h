
#ifndef OS_APPLICATION_H
#define	OS_APPLICATION_H

#include <QApplication>
#include <QtGui>
#include "textedit.h"
#include "allconfig.h"

/* 
 * /Users/pro/Desktop/Qt_code/document_editor_2/   src/os_application.h
 This is config to QCoreApplication  open document on Finder by drag file on app icon
 * http://www.qtcentre.org/wiki/index.php?title=Opening_documents_in_the_Mac_OSX_Finder
 */


/* catch event from QCoreApplication if haveit more */
#define osApplication \
    (static_cast<OS_application*>(QCoreApplication::instance()))

class OS_application : public QApplication {

    Q_OBJECT
    //
public:
    OS_application(int &argc, char **argv)
    : QApplication(argc, argv) {

#if QT_VERSION >= 0x040500
        qDebug() << "### QT_VERSION main  -> " << QT_VERSION;
        qDebug() << "### QT_VERSION_STR main -> " << QT_VERSION_STR;
#endif
        
#if defined Q_WS_MAC
        QStringList path;
        path.append(applicationDirPath());
        QDir dir(applicationDirPath());
        dir.cdUp();
        path.append(dir.absolutePath());
        dir.cd("plugins");
        path.append(dir.absolutePath());
        dir.cdUp();
        path.append(dir.absolutePath());
        setLibraryPaths(path);
        QDir::setCurrent(dir.absolutePath()); /* here down -> Frameworks */
#endif

        /* QSetting setup to access */
        setOrganizationName("Oasis Test Reader");
        setOrganizationDomain("QTuser");
        setApplicationName("Mini Office");
        setWindowIcon(QIcon(":/images/ODTicon.png"));

        /* start  your QMainWindow */
        TextEdit::self()->setWindowTitle("Oasis Test Reader"); /* static QPointer<Gmain> _self; */
        TextEdit::self()->setWindowIcon(QIcon(":/images/ODTicon.png"));
        TextEdit::self()->show(); /* an show its */
        ////TextEdit::self()->resize(800,600);
        //// TextEdit::self()->load(":/images/image_test.odt"); /// "
    }
protected:

    bool event(QEvent *ev) {
        bool eaten;
        switch (ev->type()) {
            case QEvent::FileOpen:
                /* nr 116 */
                //////filecheck.fromLocalFile(QString());
                osfile = static_cast<QFileOpenEvent *> (ev)->file();
                TextEdit::self()->load(osfile); /* open file */
                TextEdit::self()->showMaximized();
                //// Gmain::self()->RemoveNullFile();  /* clean null file and empyty widget at set correct if nullfile  */
                eaten = true;
                break;
            case QEvent::Close:
            {
                QCloseEvent *closeEvent = static_cast<QCloseEvent *> (ev);
                /* clean all choise and close dbs if is open */
                TextEdit::self()->maybeSave();
                eaten = true;
                break;
            }
            default:
                eaten = QApplication::event(ev);
                break;
        }
        return eaten;
    }
private:
    QString osfile;
};


#endif	/* OS_APPLICATION_H */

