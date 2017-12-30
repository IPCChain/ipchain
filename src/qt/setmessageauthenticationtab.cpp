#include "setmessageauthenticationtab.h"
#include "ui_setmessageauthenticationtab.h"
#include "walletmodel.h"
#include "base58.h"
#include "pubkey.h"
#include "hash.h"
#include "guiutil.h"
#include "validation.h"
#include <vector>
#include "wallet/wallet.h"
#include "log/log.h"
using namespace std;
SetMessageAuthenticationTab::SetMessageAuthenticationTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SetMessageAuthenticationTab),walletModel(NULL)
{
    ui->setupUi(this);
    ui->statusLabel_VM->setText("");
}

SetMessageAuthenticationTab::~SetMessageAuthenticationTab()
{
    delete ui;
}

void SetMessageAuthenticationTab::on_pushButton_check_pressed()
{

     if(NULL==ui->lineEdit_address->text() ||NULL==ui->lineEdit_message->text())
     {
         ui->statusLabel_VM->setStyleSheet("QLabel { color: red; }");
         ui->statusLabel_VM->setText(tr("Please input info"));
         return;
     }

    CBitcoinAddress addr(ui->lineEdit_address->text().toStdString());
    if (!addr.IsValid())
    {
        ui->statusLabel_VM->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_VM->setText(tr("The entered address is invalid.Please check the address and try again."));
        return;
    }
    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
    {
       // ui->addressIn_VM->setValid(false);
        ui->statusLabel_VM->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_VM->setText(tr("The entered address does not refer to a key.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }
    if(!walletModel->CheckPassword())
    {
        LOG_WRITE(LOG_INFO,"SetMessageAuthenticationTab","password error");
        ui->statusLabel_VM->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_VM->setText(tr("password error"));
        return ;
    }
        WalletModel::UnlockContext ctx(walletModel, true, true);


    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(ui->lineEdit_message->text().toStdString().c_str(), &fInvalid);

    if (fInvalid)
    {
       // ui->signatureIn_VM->setValid(false);
        ui->statusLabel_VM->setStyleSheet("QLabel { color: red; }");
        //ui->statusLabel_VM->setText(tr("The signature could not be decoded.Please check the signature and try again."));
        ui->statusLabel_VM->setText(tr("Message verification failed."));
        return;
    }

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << ui->textEdit_info->document()->toPlainText().toStdString();

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
    {
        //ui->signatureIn_VM->setValid(false);
        ui->statusLabel_VM->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_VM->setText(tr("The signature did not match the message digest.") + QString(" ") + tr("Please check the signature and try again."));
        return;
    }

    if (!(CBitcoinAddress(pubkey.GetID()) == addr))
    {
        ui->statusLabel_VM->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_VM->setText(QString("<nobr>") + tr("Message verification failed.") + QString("</nobr>"));
        return;
    }

    ui->statusLabel_VM->setStyleSheet("QLabel { color: green; }");
    ui->statusLabel_VM->setText(QString("<nobr>") + tr("Message verified."));
}
