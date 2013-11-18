#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkDiskCache>
#include <QTextStream>
#include "doclib/config.h"
#include "doclib/pdfhtml.h"
#include "doclib/docxzip.h"
#include "doclib/odtzip.h"
#include "doclib/rtfhtml.h"

class DocumentManager : public QObject {
    Q_OBJECT
    QStringList handlefilelist;
    QNetworkAccessManager manager;
    QList<QNetworkReply *> currentDownloads;

public:
    DocumentManager();
    void doDownload(const QUrl &url);
    QString saveFileName(const QUrl &url);
    bool saveToDisk(const QString &filename, QIODevice *data);

public slots:
    void execute();
    void get_remote_file(const QString &httpfile);
    void downloadFinished(QNetworkReply *reply);

private:
    bool validdoc(const QString doc);
    void console(QString msg, int modus = 0);
    void HandleDoc(QFileInfo fi);
    int cursor;
    QString type;
    QString extensionfind;
};

DocumentManager::DocumentManager() {
    QDir dir;
    if (!dir.exists(APPCACHEDOC)) {
        dir.mkpath(APPCACHEDOC);
    }
    cursor = 0;
    connect(&manager, SIGNAL(finished(QNetworkReply*)),
            SLOT(downloadFinished(QNetworkReply*)));
}

void DocumentManager::doDownload(const QUrl &url) {
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

    currentDownloads.append(reply);
}

QString DocumentManager::saveFileName(const QUrl &url) {
    QString path = url.path();
    QString basename = QFileInfo(path).fileName();

    if (basename.isEmpty()) {
        return QString("NullName");
    }
    const QString fullpathfile = APPCACHEDOC + basename;
    return fullpathfile;
}

bool DocumentManager::saveToDisk(const QString &filename, QIODevice *data) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        fprintf(stderr, "Could not open %s for writing: %s\n",
                qPrintable(filename),
                qPrintable(file.errorString()));
        return false;
    }

    file.write(data->readAll());
    file.close();

    return true;
}

void DocumentManager::get_remote_file(const QString &httpfile) {
    doDownload(QUrl(httpfile));
}

void DocumentManager::execute() {
    QStringList args = QCoreApplication::instance()->arguments();
    // skip the first argument, which is the program's name
    args.takeFirst();
    if (args.isEmpty()) {
        QCoreApplication::instance()->quit();
        return;
    }

    foreach(QString arg, args) {
        const QString officedoc = QString(arg.toLocal8Bit());
        QFileInfo filex(officedoc);
        console(QString("Try File open %1 - %2").arg(SystemSecure::bytesToSize(filex.size())).arg(officedoc) );
        if (validdoc(filex.absoluteFilePath())) {
            handlefilelist << officedoc;
            console(QString("Main: Handle document: %1").arg(extensionfind), 2);
        } else {
            console(QString("Main:Type %2 .. Sorry File %1 not exist or unable to handle or to big for html web..!").arg(officedoc).arg(extensionfind), 2);
            QCoreApplication::instance()->quit();
        }
    }


}

bool DocumentManager::validdoc(const QString doc) {
    QFileInfo tinfo(doc);
    QString ex;
    if ( tinfo.size() > LIMITSIZE  ) {
        return false;
    }
    ////console(QString("Main validate doc: %1").arg(doc));
    /// download the correct converter fresh updatet..
    const QString basicname = tinfo.completeBaseName();
    //// const QString ex = tinfo.completeSuffix().toLower();
    QFile file(tinfo.absoluteFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray firsti = file.peek(4);
        file.reset();
        file.seek(30);
        const QByteArray flong = file.read(19).simplified(); /// after zip header 
        //// console(QString("Peek first: %1").arg( QString(firsti.constData()) ));
        ////console(QString("Peek long: %1").arg( QString(flong.constData()) ));
        file.reset();
        ///// zip type PK
        if (file.peek(2) == "PK") {
            file.seek(30);
            if (file.read(47) == "mimetypeapplication/vnd.oasis.opendocument.text") {
                ex = "odt";
            } else if ( flong == "[Content_Types].xml" ) {
                ex = "docx";
            }
        } else if (file.peek(5) == "{\\rtf") {
            ex = "rtf";
        } else if ( firsti == "%PDF") {
           ex = "pdf"; 
        }
        file.close();
    }

    type = tinfo.fileName();
    extensionfind = ex;
    if (ex == "pdf") {
        get_remote_file(__netPDFDTD__);
        get_remote_file(__netPDFSTYLE__);
        return true;
    } else if (ex == "docx") {
        get_remote_file(__netDOCXSTYLE__);
        return true;
    } else if (ex == "odt") {
        get_remote_file(__netODTSTYLE__);
        return true;
    } else if (ex == "rtf") {
        get_remote_file(__netPDFDTD__); /// dummy download ...
        return true;
    } else {
        return false;
    }
}

void DocumentManager::console(QString msg, int modus) {

    QTextStream out(stdout);
    /// const QString classname = document_name;
    QString str("*");
    if (modus == 3 || modus == 2) {
        out << "Time: " << Tools::TimeNow() << "\n";
        out << msg << "\n";
        out << str.fill('*', pointo) << "\n";
    } else {
        out << msg << "\n";
    }

    out.flush();

}

void DocumentManager::downloadFinished(QNetworkReply *reply) {

    ////printf("Signal call downloadFinished ...  \n");
    QUrl url = reply->url();
    if (reply->error()) {
        fprintf(stderr, "Download of %s failed: %s\n",
                url.toEncoded().constData(),
                qPrintable(reply->errorString()));
    } else {
        QString filename = saveFileName(url);
        if (saveToDisk(filename, reply)) {
            console(QString("Init handle document type:..%1..").arg(type));
            /// printf("Download of %s succeded (saved to %s)\n",url.toEncoded().constData(), qPrintable(filename));
        }
    }

    currentDownloads.removeAll(reply);
    reply->deleteLater();

    if (currentDownloads.isEmpty()) {
        /// start to convert document 
        if (handlefilelist.size() > 0) {

            foreach(QString file, handlefilelist) {
                HandleDoc(QFileInfo(file));
            }

        } else {
            // all downloads finished && doc converted..
            QCoreApplication::instance()->quit();
        }

    }

}

void DocumentManager::HandleDoc(QFileInfo fi) {
    cursor++;
    const QString ex = extensionfind;
    if (ex == "pdf") {
        PortableDocument *hdoc = new PortableDocument(fi.absoluteFilePath());
    } else if (ex == "docx") {
        DocxZipHandler *ddoc = new DocxZipHandler(fi.absoluteFilePath());
    } else if (ex == "odt") {
        OdtZipHandler *dodt = new OdtZipHandler(fi.absoluteFilePath());
    } else if (ex == "rtf") {
        RtfDocument *drtf = new RtfDocument(fi.absoluteFilePath());
    } else {
        console(QString("Warning! unknow type file %1").arg( extensionfind ));
        QCoreApplication::instance()->quit();
    }
    if (cursor >= handlefilelist.size()) {
        QCoreApplication::instance()->quit();
    }
}

static void usagethisapp(const char *name, int size) {
    printf("Usage: %s officedocumentc.type\n", name);
    printf(" {%d} Options:\n", size);
    printf("\t--version or -V: show the version of conversion used\n");
    printf("\t--help: display this text.\n");
}

int main(int argc, char **argv) {
    QTextStream out(stdout);
    QString str("*");
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
            out << str.fill('*', pointo) << "\n";
            out << "Converter-Version:" << __DOCVERSION__ << "\n";
            out << str.fill('*', pointo) << "\n";
            out.flush();
            return (1);
        }

    }

    QCoreApplication app(argc, argv);
    DocumentManager manager;
    QTimer::singleShot(0, &manager, SLOT(execute()));
    app.exec();
}

#include "main.moc"
