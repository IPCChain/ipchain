#include "setmessagesignature.h"
#include "ui_setmessagesignature.h"
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
SetMessageSignature::SetMessageSignature(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SetMessageSignature),walletModel(NULL)
{
    ui->setupUi(this);
    ui->label_msg->setText("");
}

SetMessageSignature::~SetMessageSignature()
{
    delete ui;
}

void SetMessageSignature::on_pushButton_sign_pressed()
{
    ui->lineEdit_message->setText("");
//ui->textEdit_sign->toPlainText().isEmpty()
    if (ui->lineEdit_address->text().isEmpty())
    {
        ui->label_msg->setStyleSheet("QLabel { color: red; }");
        ui->label_msg->setText(tr("Please input info"));
        return;

    }
//||ui->textEdit_sign->toPlainText().isEmpty()
    if (!walletModel||ui->lineEdit_address->text().isEmpty())
    {
        ui->label_msg->setStyleSheet("QLabel { color: red; }");
        ui->label_msg->setText(tr("The address entered is illegal. Please check and try again."));
        return;
    }

    /* Clear old signature to ensure users don't get confused on error with an old signature displayed */
   // uCHashWriteri->signatureOut_SM->clear();

    CBitcoinAddress addr(ui->lineEdit_address->text().toStdString());
    if (!addr.IsValid())
    {
        ui->label_msg->setStyleSheet("QLabel { color: red; }");
        ui->label_msg->setText(tr("The address entered is illegal. Please check and try again."));
        return;
    }

    if(!walletModel->CheckPassword())
    {
        LOG_WRITE(LOG_INFO,"SetMessageAuthenticationTab","password error");
        ui->label_msg->setStyleSheet("QLabel { color: red; }");
        ui->label_msg->setText(tr("password error"));
        return ;
    }

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
    {

        ui->label_msg->setStyleSheet("QLabel { color: red; }");
        ui->label_msg->setText(tr("The entered address does not refer to a key.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }
    if(!walletModel->CheckPassword())
    {
        ui->label_msg->setStyleSheet("QLabel { color: red; }");
        ui->label_msg->setText(tr("Wallet unlock was cancelled."));
        return;
    }
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid())
    {
        ui->label_msg->setStyleSheet("QLabel { color: red; }");
        ui->label_msg->setText(tr("Wallet unlock was cancelled."));
        return;
    }

    CKey key;
    if (!walletModel->getPrivKey(keyID, key))
    {
        ui->label_msg->setStyleSheet("QLabel { color: red; }");
        ui->label_msg->setText(tr("Private key for the entered address is not available."));
        return;
    }

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << ui->textEdit_sign->document()->toPlainText().toStdString();

    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig))
    {
        ui->label_msg->setStyleSheet("QLabel { color: red; }");
        ui->label_msg->setText(QString("<nobr>") + tr("Message signing failed.") + QString("</nobr>"));
        return;
    }

    ui->label_msg->setStyleSheet("QLabel { color: green; }");
    ui->label_msg->setText(QString("<nobr>") + tr("Message signed.") + QString("</nobr>"));

    ui->lineEdit_message->setText(QString::fromStdString(EncodeBase64(&vchSig[0], vchSig.size())));
}
