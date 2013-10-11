//
// C++ Implementation: Get Mail as search result by subject & date since imap.
//
// Description:
// Target to having a console app to filter & get mail to insert on drupal as new node.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution
/// netbean setting  /Users/pro/project/version_sv/GmailHand/.bea

#include <QCoreApplication>
#include "net_starterimap.h"

#define _PROGRAM_NAME_ "Mail console Reader di Peter Hohl"
#define _PROGRAM_NAME_DOMAINE_ "freeroad.ch"
#define _ORGANIZATION_NAME_ "PPK-Webprogramm"

/*  basic   */
#include <QtCore/QCoreApplication>
#include <QTimer>
#include <QTextStream>
#include <QDebug>
#include <QTextStream>

/*  network  */
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>
/// cd /Users/pro/Desktop/Qt_project/GmailHand/
/// /Users/pro/Desktop/Qt_project/GmailHand/

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv); //renamed the a to app
    /// console app 
    app.setOrganizationName(_ORGANIZATION_NAME_);
    app.setOrganizationDomain(_PROGRAM_NAME_DOMAINE_);
    app.setApplicationName(_PROGRAM_NAME_);

    QStringList localip;
    QTextStream out(stdout);
    QString str("*");
    out << str.fill('*', _IMAIL_WIDTH_) << "\n";

#if !defined(Q_OS_WIN)
    ReadMail::StreamMail *readtest = new ReadMail::StreamMail();
    out << "Free Home DiskSpace:" << readtest->freeSpaceHome() << "\n";
    out << str.fill('*', _IMAIL_WIDTH_) << "\n";
#endif

    foreach(QNetworkInterface interface, QNetworkInterface::allInterfaces()) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
                !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            foreach(QNetworkAddressEntry entry, interface.addressEntries()) {
                if (interface.hardwareAddress() != "00:00:00:00:00:00" && entry.ip().toString().contains(".")) {
                    localip.append(entry.ip().toString());
                    out << interface.name() + " " + entry.ip().toString() + " " + interface.hardwareAddress() << "\n";
                }
            }
        }
    }

    out << localip.join(" - ") << "\n";
    out << "Online ok. Valid local ip to connect\n" << str.fill('*', _IMAIL_WIDTH_) << "\n";
    out.flush();

    if (localip.size() < 1) {
        out << "Offline Computer!\n" << str.fill('*', _IMAIL_WIDTH_) << "\n";

        out.flush();
        return 1;
    }
    /// param to init class 
    QString search_word, username, password;
    int dayback = -3; ///  to search since date searchword query.
    /// imap cmd:  //// UID SEARCH SUBJECT \"foto\" SINCE 20-Sep-2013



    if (username.isEmpty()) {
        out << "Please enter word to search on mail subject:\n";
        out.flush();
        QTextStream in(stdin);
        out.flush();
        search_word = in.readLine(); /// here new word to insert on class...

        out << "Please enter Gmail username:\n";
        out.flush();
        username = in.readLine();

        out << "Please enter Gmail PASSWORD:\n";
        out.flush();
        password = in.readLine();
    }


    QString word("pressrelease");
    if (!search_word.isEmpty()) {
        word = search_word;
    }

    if (password.isEmpty() or username.isEmpty()) {
        out << "No user or pass: NO JOB! By\n";
        out.flush();
        return 1;
    }


    if (QSslSocket::supportsSsl()) {
        qDebug() << "ssl supportsSsl ok main \n";
        Net_StarterImap *imap = new Net_StarterImap();
        imap->SearchWord(word, dayback);
        imap->Connect(username, password);
        QObject::connect(imap, SIGNAL(Exit_Close()), &app, SLOT(quit()));
    } else {
        qDebug() << "ssl notSupported :-( main \n";
    }

    QTimer::singleShot(10000, &app, SLOT(quit()));
    return app.exec();
}
