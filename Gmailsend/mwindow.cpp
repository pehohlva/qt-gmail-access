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
#include "mwindow.h"
#include "ui_mwindow.h"

MWindow::MWindow(QWidget *parent) :
QMainWindow(parent), to_current_file(""), ui(new Ui::MWindow) {
    ui->setupUi(this);
    ui->usernameg->setText(QString("username@gmail.com"));
    ui->passwordg->setText(QString("password"));
    if (!setter.value("UserName").toString().isEmpty()) {
        ui->usernameg->setText(setter.value("UserName").toString());
    }
    if (!setter.value("Passwords").toString().isEmpty()) {
        ui->passwordg->setText(setter.value("Passwords").toString());
    }
    attachment_item.clear();
    IncommingMessage(QString("Wakeup...."));
    /*
     *QLineEdit *usernameg;
    QLabel *labelmail;
    QLabel *labelpass;
    QLineEdit *passwordg;
     */
}

void MWindow::UpdateMail() {
    this->setCursor(Qt::WaitCursor);
    IncommingMessage(QString("Update mail chunk...."));
    QApplication::processEvents();
    QString username = ui->usernameg->text();
    QString passw = ui->passwordg->text();
    QString betreff = ui->subject_text->text();
    setter.setValue("UserName", username);
    setter.setValue("Passwords", passw);
    /// subject_text
    rawmail.SetFromTo(ui->usernameg->text(), ui->mailtotext->text());
    rawmail.Clear();
    /// m.AppendAttachment(QFileInfo("/Users/pro/Downloads/savoia.jpg"));
    ///
    attachment_item.removeDuplicates();
    for (int i = 0; i < attachment_item.size(); ++i) {
        QString mailfile = QString(attachment_item.at(i).toLocal8Bit().constData());
        rawmail.AppendAttachment(QFileInfo(mailfile));
    }
    rawmail.SetMessage(betreff, ui->Editmail->document()); /// Editmail
    this->setCursor(Qt::ArrowCursor);
    IncommingMessage(QString("Wakeup...."));

}

void MWindow::mail_send() {
    qDebug() << "Mail send:" << __FUNCTION__; /// disconnected
    //// IncommingMessage(QString("Disconnected.... from remote server..."));
    smtpserver->deleteLater();
    rawmail.Clear();
    attachment_item.clear();
    ui->listattachment->clear();
}

/*

void MWindow::SmsinErrors(const QList<QSslError) {
    IncommingMessage("Incomming QSslError.... from remote server...");
}
 *  */

void MWindow::on_sendmailcmd_clicked() {
    /// mail send & check
    UpdateMail();
    QString username = ui->usernameg->text();
    QString passw = ui->passwordg->text();
    const QString chunk_mail = rawmail.FullChunkMail();
    QMessageBox msgBox;
    QPushButton *sendbuttun = msgBox.addButton(tr("Send mail %1").arg(InfoHumanSize_human(chunk_mail.size())), QMessageBox::ActionRole);
    QPushButton *abortButton = msgBox.addButton(QMessageBox::Abort);
    msgBox.exec();

    if (msgBox.clickedButton() == sendbuttun) {
        // connect
        smtpserver = new QwwSmtpClient(this);
        connect(smtpserver, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(SmsinErrors(const QList < QSslError)));
        int rec = smtpserver->connectToHostEncrypted("smtp.gmail.com", 465);
        qDebug() << "connectToHost:" << rec;
        smtpserver->authenticate(username, passw, QwwSmtpClient::AuthLogin);
        int success = smtpserver->sendMail(rawmail.From(), rawmail.To(), rawmail.FullChunkMail());
        qDebug() << "sendMail:" << success;
        smtpserver->disconnectFromHost();
        connect(smtpserver, SIGNAL(disconnected()), this, SLOT(mail_send()));
    }
    if (msgBox.clickedButton() == abortButton) {
        // connect
        qDebug() << "sendmail noooo....";
        rawmail.Clear();
        attachment_item.clear();
        ui->listattachment->clear();
    }

}

void MWindow::IncommingMessage(QString txt) {
    ui->l_result->setText(txt);
}

void MWindow::on_usernameg_editingFinished() {
    QString username = ui->usernameg->text();
    QString passw = ui->passwordg->text();
    setter.setValue("UserName", username);
    setter.setValue("Passwords", passw);
}

void MWindow::on_passwordg_editingFinished() {
    QString username = ui->usernameg->text();
    QString passw = ui->passwordg->text();
    setter.setValue("UserName", username);
    setter.setValue("Passwords", passw);
}

void MWindow::on_actionQuit_triggered() {
    close();
    qApp->quit();

}

void MWindow::on_attachment_cmd_pressed() {
    qDebug() << "run :" << __FUNCTION__;
    QString n_file = QFileDialog::getOpenFileName(this, tr("Open File..."), QString(setter.value("LastDir").toString()), "*");

    if (!n_file.isEmpty()) {
        QFileInfo fi(n_file);
        setter.setValue("LastDir", fi.absolutePath() + "/");
        attachment_item.removeDuplicates();
        attachment_item.append(n_file);
        ui->listattachment->clear();
        ui->listattachment->addItems(attachment_item);
        qDebug() << "run 1:" << __FUNCTION__;
        return;
    }
    qDebug() << "run end 2 :" << __FUNCTION__;
}

void MWindow::on_actionAttachment_triggered() {

    qDebug() << "run :" << __FUNCTION__;

}

void MWindow::on_removeattachment_button_clicked() {
    attachment_item.clear();
    ui->listattachment->clear();
}

MWindow::~MWindow() {
    delete ui;
}

bool MWindow::fileSave() {
    if (to_current_file.isEmpty())
        return fileSaveAs();

    bool canodt = false;
    QTextDocument *doc = ui->Editmail->document();
#if QT_VERSION >= 0x040500
    canodt = true;
#endif
    const QString ext = QFileInfo(to_current_file).completeSuffix().toLower();
    if (ext == "odt" && canodt) {
#if QT_VERSION >= 0x040500
        QTextDocumentWriter writer(to_current_file);
        return writer.write(doc);
#endif
        return false;
    } else {
        QFile file(to_current_file);
        if (!file.open(QFile::WriteOnly))
            return false;
        QTextStream ts(&file);
        ts.setCodec(QTextCodec::codecForName("UTF-8"));
        ts << doc->toHtml("UTF-8");
        /// textEdit->document()->setModified(false);
    }
    return true;
}

bool MWindow::fileSaveAs() {

    QString support;
#if QT_VERSION >= 0x040500
    support = tr("ODF files (*.odt);;HTML-Files (*.htm *.html);;All Files (*)");
#else
    support = tr("HTML-Files (*.htm *.html);;All Files (*)");
#endif
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
            QString(), support);
    if (fn.isEmpty())
        return false;
    to_current_file = fn;
    return fileSave();
}

void MWindow::on_actionExport_to_odp_triggered(bool checked) {
    fileSaveAs();
}
