//
// C++ Implementation: sample to send mail on smtp from gmail
//
// Description:
// SMTP Encrypted sending mail attachment & mime chunk Editmail->document()
// ( QTextDocument ) fill from image e html tag & style color full mime mail.
//
// Author: Peter Hohl <pehohlva@gmail.com>,    19.9.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef MWINDOW_H
#define MWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
 #include <QTextStream>
#include <QtGui>
#include <QStringList>
#include "mailformat.h"
#include "qwwsmtpclient.h"

#if QT_VERSION >= 0x040500
#include <QTextDocumentWriter>
#endif



namespace Ui {
    class MWindow;
}

class MWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MWindow(QWidget *parent = 0);
    ~MWindow();

private slots:
    void on_actionQuit_triggered();
    void on_sendmailcmd_clicked();
    void on_removeattachment_button_clicked();
    void on_usernameg_editingFinished();
    void on_passwordg_editingFinished();
    void on_actionAttachment_triggered();
    void on_attachment_cmd_pressed();
    void mail_send();
    void IncommingMessage(QString txt);
    void on_actionExport_to_odp_triggered(bool checked);

private:
    void UpdateList();
    void UpdateMail();
    bool fileSave();
    bool fileSaveAs();
    MailFormat rawmail; /// rawmail.SetFromTo("ppkciz@gmail.com", "dev@liberatv.ch");
    Ui::MWindow *ui;
    QStringList attachment_item;
    QSettings setter;
    QString to_current_file;
    QwwSmtpClient *smtpserver; // socket
};

#endif // MWINDOW_H
