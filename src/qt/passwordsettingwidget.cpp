#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "passwordsettingwidget.h"
#include "forms/ui_passwordsettingwidget.h"

#include "guiconstants.h"
#include "walletmodel.h"

#include "support/allocators/secure.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include "cmessagebox.h"
#include "dpoc/TimeService.h"
#include <QSettings>
#include "log/log.h"
PasswordSettingWidget::PasswordSettingWidget(Mode _mode,QWidget *parent) :
    //QDialog(parent),
    QWidget(parent),
    ui(new Ui::PasswordSettingWidget)
{
    ui->setupUi(this);

    ui->passEdit1->setMinimumSize(ui->passEdit1->sizeHint());
    ui->passEdit2->setMinimumSize(ui->passEdit2->sizeHint());
    ui->passEdit3->setMinimumSize(ui->passEdit3->sizeHint());

    ui->passEdit1->setMaxLength(MAX_PASSPHRASE_SIZE);
    ui->passEdit2->setMaxLength(MAX_PASSPHRASE_SIZE);
    ui->passEdit3->setMaxLength(MAX_PASSPHRASE_SIZE);

    // Setup Caps Lock detection.
    ui->passEdit1->installEventFilter(this);
    ui->passEdit2->installEventFilter(this);
    ui->passEdit3->installEventFilter(this);
    switch(_mode)
        // switch(this->model->StatusCode)
    {
    case Encrypt: // Ask passphrase x2
        ui->passEdit3->hide();
        ui->passEdit3->setVisible(false);
        ui->passEdit1->setPlaceholderText(tr("6-8 characters, suggesting English numerals are mixed"));
        ui->passEdit2->setPlaceholderText(tr("Confirm password"));
        break;
    case Unlock: // Ask passphrase
        ui->passEdit2->hide();
        ui->passEdit3->hide();

        break;
    case Decrypt:   // Ask passphrase
        ui->passEdit2->hide();
        ui->passEdit3->hide();
        break;
    case ChangePass: // Ask old passphrase + new passphrase x2
        ui->passEdit1->setPlaceholderText(tr("old password"));
        ui->passEdit2->setPlaceholderText(tr("6-8 characters, suggesting English numerals are mixed"));
        ui->passEdit3->setPlaceholderText(tr("Confirm password"));
        break;
    }
    textChanged();
    connect(ui->passEdit1, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
    connect(ui->passEdit2, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
    connect(ui->passEdit3, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
    ui->label_error->setText("");

}

PasswordSettingWidget::~PasswordSettingWidget()
{
    delete ui;
}



void PasswordSettingWidget::on_pushButton_2_pressed()
{
    Q_EMIT back();

}
//20170822

void PasswordSettingWidget::setMode(Mode _mode)
{
    this->mode = _mode;
}
void PasswordSettingWidget::setModel(WalletModel *_model)
{
    this->model = _model;
}
void PasswordSettingWidget::on_pushButton_pressed()
{

   // ui->label_error->setText(tr("please wait..."));
    if(mode == Encrypt){
        if(ui->passEdit1->text().isEmpty()){
            ui->label_error->setText(tr("please set password"));
            return;
        }else if(ui->passEdit2->text().isEmpty()){
            ui->label_error->setText(tr("please set confirm password"));
            return;
        }
        int size = ui->passEdit1->text().size();
        std::string temp = ui->passEdit1->text().toStdString();
        if(size<6||size>8||temp.length()!=size){
            ui->label_error->setText(tr("please check password"));
            return;
        }
    }else if(mode == ChangePass){
        if(ui->passEdit1->text().isEmpty()){
            ui->label_error->setText(tr("please set old password"));
            return;
        }else if(ui->passEdit2->text().isEmpty()){
            ui->label_error->setText(tr("please set new password"));
            return;
        }else if(ui->passEdit3->text().isEmpty()){
            ui->label_error->setText(tr("please set confirm password"));
            return;
        }
        int size = ui->passEdit2->text().size();
        std::string temp = ui->passEdit2->text().toStdString();
        if(size<6||size>8||temp.length()!=size){
            ui->label_error->setText(tr("please check password"));
            return;
        }
    }

    ui->label_error->setText(tr("please wait..."));
    //update();

    SecureString oldpass, newpass1, newpass2;
    if(!model)
        return;
    oldpass.reserve(MAX_PASSPHRASE_SIZE);
    newpass1.reserve(MAX_PASSPHRASE_SIZE);
    newpass2.reserve(MAX_PASSPHRASE_SIZE);
    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make this input mlock()'d to begin with.
    //  oldpass.assign(ui->passEdit1->text().toStdString().c_str());
    //  newpass1.assign(ui->passEdit2->text().toStdString().c_str());
    //  newpass2.assign(ui->passEdit3->text().toStdString().c_str());

    // secureClearPassFields();
    //mode=Encrypt;
    switch(mode)
    {
    case Encrypt: {
        //oldpass.assign(ui->passEdit1->text().toStdString().c_str());
        newpass1.assign(ui->passEdit1->text().toStdString().c_str());
        newpass2.assign(ui->passEdit2->text().toStdString().c_str());

        if(newpass1.empty() || newpass2.empty())
        {
            // Cannot encrypt with empty passphrase
            break;
        }
   //     QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm wallet encryption"),
    //                                                               tr("Warning: If you encrypt your wallet and lose your passphrase, you will <b>LOSE ALL OF YOUR BITCOINS</b>!") + "<br><br>" + tr("Are you sure you wish to encrypt your wallet?"),
    //                                                               QMessageBox::Yes|QMessageBox::Cancel,
    //                                                               QMessageBox::Cancel);
       // if(retval == QMessageBox::Yes)
       // {
            if(newpass1 == newpass2)
            {
                if(model->setWalletEncrypted(true, newpass1))
                {
                   /* QMessageBox::warning(this, tr("Wallet encrypted"),
                                         "<qt>" +
                                         tr("%1 will close now to finish the encryption process. "
                                            "Remember that encrypting your wallet cannot fully protect "
                                            "your bitcoins from being stolen by malware infecting your computer.").arg(tr(PACKAGE_NAME)) +
                                         "<br><br><b>" +
                                         tr("IMPORTANT: Any previous backups you have made of your wallet file "
                                            "should be replaced with the newly generated, encrypted wallet file. "
                                            "For security reasons, previous backups of the unencrypted wallet file "
                                            "will become useless as soon as you start using the new, encrypted wallet.") +
                                         "</b></qt>");
                                         */
                    //   QApplication::quit();
                    if(2 == getTag())
                    {
                        Q_EMIT openSendCoinsAffrimwidget();
                    }
                    else
                    {
                        ui->label_error->setText(tr("passwordset success"));
                         Q_EMIT ChangePasswordSuccess();

                    }
                }
                else
                {
                   // QMessageBox::critical(this, tr("Wallet encryption failed"),
                    //                      tr("Wallet encryption failed due to an internal error. Your wallet was not encrypted."));
                    ui->label_error->setText(tr("Password mismatch"));
                }
                //  QDialog::accept(); // Success

            }
            else
            {
                //QMessageBox::critical(this, tr("Wallet encryption failed"),
                //                      tr("The supplied passphrases do not match."));
                ui->label_error->setText(tr("Wallet encryption failed"));
            }
       // }
        //else
        //{
            // QDialog::reject(); // Cancelled
       // }
    } break;
    case Unlock:
        if(!model->setWalletLocked(false, oldpass))
        {
            ui->label_error->setText(tr("Wallet unlock failed"));
            //QMessageBox::critical(this, tr("Wallet unlock failed"),
              //                    tr("The passphrase entered for the wallet decryption was incorrect."));
        }
        else
        {
            //QDialog::accept(); // Success
        }
        break;
    case Decrypt:
        if(!model->setWalletEncrypted(false, oldpass))
        {
             ui->label_error->setText(tr("Wallet decryption failed"));
            //QMessageBox::critical(this, tr("Wallet decryption failed"),
             //                     tr("The passphrase entered for the wallet decryption was incorrect."));
        }
        else
        {
            // QDialog::accept(); // Success
        }
        break;
    case ChangePass:
        oldpass.assign(ui->passEdit1->text().toStdString().c_str());
        newpass1.assign(ui->passEdit2->text().toStdString().c_str());
        newpass2.assign(ui->passEdit3->text().toStdString().c_str());
        if(newpass1 == newpass2)
        {
            int64_t timenum = 0;
            int iPasswordErrorNum = 0;
            bool was_locked = (model->getEncryptionStatus() == WalletModel::Locked)?1:0;
            if(was_locked)
            {
                QSettings settings;
                iPasswordErrorNum = settings.value("PasswordErrorNum").toInt();
                LOG_WRITE(LOG_INFO,"iPasswordErrorNum",QString::number(iPasswordErrorNum).toStdString().c_str());
                timenum =  timeService.GetCurrentTimeSeconds();  
                if(iPasswordErrorNum >= 5){
                    QString locktime = settings.value("locktime").toString();
                    LOG_WRITE(LOG_INFO,"timeService::GetCurrentTimeSeconds",QString::number(timenum).toStdString().c_str(),"locktime",locktime.toStdString().c_str());
                    QString strtimenum = QString::number(timenum);
                    if(locktime > strtimenum){
                        CMessageBox msg;
                        msg.setMessage(2);
                        msg.exec();
                        ui->label_error->setText(tr("Wallet unlock failed"));
                        return;
                    }
                }
                if(model->changePassphrase(oldpass, newpass1))
                {
                    settings.setValue("PasswordErrorNum", 0);
                    ui->label_error->setText(tr("Wallet passphrase was successfully changed."));
                    if(2 == getTag())
                    {
                        Q_EMIT openSendCoinsAffrimwidget();
                    }
                    else
                    {
                         ui->label_error->setText(tr("passwordset success"));
                         Q_EMIT ChangePasswordSuccess();
                    }
                }
                else
                {
                    timenum = timenum + (int64_t)60*60*24;
                    settings.setValue("PasswordErrorNum", iPasswordErrorNum +1);
                    settings.setValue("locktime", QString::number(timenum));
                    ui->label_error->setText(tr("Wallet encryption failed,The passphrase entered for the wallet decryption was incorrect."));
                }
            }else{
                LOG_WRITE(LOG_INFO,"no lock");
            }
        }
        else
        {
            ui->label_error->setText(tr("Wallet encryption failed"));
          //  QMessageBox::critical(this, tr("Wallet encryption failed"),
            //                      tr("The supplied passphrases do not match."));

            secureClearPassFields();
        }
        break;
    }
}
int PasswordSettingWidget::getTag()
{
    return m_tag;
}
void PasswordSettingWidget::setTag(int tag)
{
    m_tag = tag;
}
void PasswordSettingWidget::textChanged()
{
    // Validate input, set Ok button to enabled when acceptable
    bool acceptable = false;
    switch(mode)
        //  switch(this->model->StatusCode)
    {
    case Encrypt: // New passphrase x2
        acceptable = !ui->passEdit2->text().isEmpty() && !ui->passEdit3->text().isEmpty();
        break;
    case Unlock: // Old passphrase x1
    case Decrypt:
        // acceptable = !ui->passEdit1->text().isEmpty();
        break;
    case ChangePass: // Old passphrase x1, new passphrase x2
        // acceptable = !ui->passEdit1->text().isEmpty() && !ui->passEdit2->text().isEmpty() && !ui->passEdit3->text().isEmpty();
        break;
    }
    // ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(acceptable);
}

bool PasswordSettingWidget::event(QEvent *event)
{
    // Detect Caps Lock key press.
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_CapsLock) {
            fCapsLock = !fCapsLock;
        }
        if (fCapsLock) {
     //       ui->capsLabel->setText(tr("Warning: The Caps Lock key is on!"));
        } else {
      //      ui->capsLabel->clear();
        }
    }
    return QWidget::event(event);
}
/*
bool PasswordSettingWidget::eventFilter(QObject *object, QEvent *event)
{

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        QString str = ke->text();
        if (str.length() != 0) {
            const QChar *psz = str.unicode();
            bool fShift = (ke->modifiers() & Qt::ShiftModifier) != 0;
            if ((fShift && *psz >= 'a' && *psz <= 'z') || (!fShift && *psz >= 'A' && *psz <= 'Z')) {
                fCapsLock = true;
                ui->capsLabel->setText(tr("Warning: The Caps Lock key is on!"));
            } else if (psz->isLetter()) {
                fCapsLock = false;
                ui->capsLabel->clear();
            }
        }
    }
    return QDialog::eventFilter(object, event);
}
*/
static void SecureClearQLineEdit(QLineEdit* edit)
{
    // Attempt to overwrite text so that they do not linger around in memory
    edit->setText(QString(" ").repeated(edit->text().size()));
    edit->clear();
}

void PasswordSettingWidget::secureClearPassFields()
{
    //  SecureClearQLineEdit(ui->passEdit1);
    SecureClearQLineEdit(ui->passEdit2);
    SecureClearQLineEdit(ui->passEdit3);
}


//20170822

void PasswordSettingWidget::on_pushButton_clicked()
{

}
