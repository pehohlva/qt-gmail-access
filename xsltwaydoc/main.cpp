#include <QtCore>
#include <QString>
#include <QtDebug>
#include <QApplication>
#include <QXmlQuery>
#include <QtGui>
#include <QTextStream>
#include <QtWebKit>
#include <QFileInfo>

#include "odtdocxlib/config.h"
#include "odtdocxlib/odtzip.h"


/*
./xx -h  /Users/pro/Desktop/app_mie/folle.odt
1. Subclass QAbstractMessageHandler
2. Call QXmlQuery::setMessageHandler(&yourMessageHandler);


 */


static void usagethisapp(const char *name, int size) {
    printf("Usage: %s [options] officedocument.odt\n", name);
    printf(" {%d} Options:\n", size);
    printf("\t--version or -V: show the version of conversion used\n");
    printf("\t--handle \"file.txt\" or -h \"file.txt\": open a file to convert\n");
    printf("\t--output \"file.txt\" or -o \"file.txt\": save to a given file\n");
    printf("\t--timing: display the time used\n");
    printf("\t--help: display this text.\n");
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationVersion(__DOCVERSION__);
    QStringList argl = a.arguments();
    QTextStream out(stdout);
    QString str("*");
    QString result, html, outputfile, subjectfile;
    int i;
    if (argc <= 1) {
        usagethisapp(argv[0], argc);
        return (1);
    }

    for (i = 1; i < argc; i++) {
        //// --version or -V print 
        if ((!strcmp(argv[i], "-V")) ||
                (!strcmp(argv[i], "--version")) ||
                (!strcmp(argv[i], "-v"))) {
            out << "Converter-Version:" << __DOCVERSION__ << "\n";
            out << str.fill('*', iowi) << "\n";
            out.flush();
            return (1);
            //// -o Outputfile save to xxx
        } else if ((!strcmp(argv[i], "-o")) || (!strcmp(argv[i], "--output"))) {
            int next = i + 1;
            outputfile = QString(argv[next]);
            if (!outputfile.isEmpty()) {
                out << "Outputfile go to:" << outputfile << "\n";
                out.flush();
                i++; // next param
            } else {
                out << "Outputfile is not given! for " << __APPNAME__ << " \n";
                out << str.fill('*', iowi) << "\n";
                out.flush();
                return (1);
            }
        } else if ((!strcmp(argv[i], "-h")) || (!strcmp(argv[i], "--handle"))) {
            int next = i + 1;
            subjectfile = QString(argv[next]);
            if (!subjectfile.isEmpty()) {
                out << "Open file:" << subjectfile << "\n";
                out.flush();
                i++; // next param
            } else {
                out << "Handle and open file is not given! for " << __APPNAME__ << " \n";
                out << str.fill('*', iowi) << "\n";
                out.flush();
                return (1);
            }
        } else if ((!strcmp(argv[i], "-f1")) || (!strcmp(argv[i], "--help"))) {
            usagethisapp(argv[0], 1);
            return (1);
        } else {
            out << "Param " << QString(argv[i]) << " not found for:" << __APPNAME__ << " write " << QString(argv[0]) << " -f2 to help text\n";
            out << str.fill('*', iowi) << "\n";
            out.flush();
            return (1);
        }
    }
    out << str.fill('*', iowi) << "\n";
    out.flush();
    bool brocken = true;
    if (!subjectfile.isEmpty()) {
        QFileInfo fi_c(subjectfile);
        if (fi_c.exists()) {
            subjectfile = fi_c.absoluteFilePath();
            OdtZipHandler *odtdoc = new OdtZipHandler(subjectfile);
            if ( odtdoc->load())  {
                out << "OK accept: " << subjectfile << "\n";
                result = odtdoc->stream();
                brocken = false;
                out.flush();
                if (!outputfile.isEmpty()) {
                    //// Tools::_write_file(outputfile, result, "utf-8");
                }
            }
        }
    }


    if (brocken || result.isEmpty()) {
        out << "Unable to handle file: " << subjectfile << "\n";
        out.flush();
        return (1);
    }

    QWebView view;
    view.setHtml(result);
    view.setMinimumSize(800, 500);
    view.setFocus();
    view.page()->setContentEditable(true);
    //out << "Result size:" << result.size() << "\n";
    ///out.flush();
    out << "Result to gui...." << result << "\n";
    out.flush();
    view.show();
    //// t.setPlainText(result);
    a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
    return a.exec();
};

/*  QWebView view;
    view.show();
    view.setHtml(result); 
 * 
 * 
 * QTextEdit t;
    t.setMinimumSize (800,500);
    t.show();
    t.setHtml(result);
 xsltproc DocX2Html.xslt  document.xml -o paginat.html
 
 */

