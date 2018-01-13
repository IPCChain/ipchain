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
    SecureString oldpass, newpass1, newpass2;
    if(!model)
        return;
    oldpass.reserve(MAX_PASSPHRASE_SIZE);
    newpass1.reserve(MAX_PASSPHRASE_SIZE);
    newpass2.reserve(MAX_PASSPHRASE_SIZE);

    switch(mode)
    {
    case Encrypt: {
        newpass1.assign(ui->passEdit1->text().toStdString().c_str());
        newpass2.assign(ui->passEdit2->text().toStdString().c_str());

        if(newpass1.empty() || newpass2.empty())
        {
            // Cannot encrypt with empty passphrase
            break;
        }
        if(newpass1 == newpass2)
        {
            if(model->setWalletEncrypted(true, newpass1))
            {
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
                ui->label_error->setText(tr("Password mismatch"));
            }
        }
        else
        {
            ui->label_error->setText(tr("Wallet encryption failed"));
        }

    } break;
    case Unlock:
        if(!model->setWalletLocked(false, oldpass))
        {
            ui->label_error->setText(tr("Wallet unlock failed"));
        }
        break;
    case Decrypt:
        if(!model->setWalletEncrypted(false, oldpass))
        {
            ui->label_error->setText(tr("Wallet decryption failed"));
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
                        msg.setGeometry(this->x()+(this->width()-msg.width())/2,
                        this->y()+(this->height()-msg.height())/2,msg.width(),msg.height());

                        msg.setIsClose(false);
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
    {
    case Encrypt:
        acceptable = !ui->passEdit2->text().isEmpty() && !ui->passEdit3->text().isEmpty();
        break;
    case Unlock:
    case Decrypt:
        break;
    case ChangePass:
        break;
    }
}

bool PasswordSettingWidget::event(QEvent *event)
{
    // Detect Caps Lock key press.
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_CapsLock) {
            fCapsLock = !fCapsLock;
        }
    }
    return QWidget::event(event);
}

static void SecureClearQLineEdit(QLineEdit* edit)
{
    // Attempt to overwrite text so that they do not linger around in memory
    edit->setText(QString(" ").repeated(edit->text().size()));
    edit->clear();
}

void PasswordSettingWidget::secureClearPassFields()
{
    SecureClearQLineEdit(ui->passEdit2);
    SecureClearQLineEdit(ui->passEdit3);
}

