#include "setimport.h"
#include "ui_setimport.h"
#include "walletmodel.h"
setimport::setimport(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::setimport)
{
    ui->setupUi(this);
    ui->label_error->setText("");
}

setimport::~setimport()
{
    delete ui;
}

void setimport::on_pushButton_import_pressed()
{
     std::string strError;
     if(walletModel->importprivkeybibipay(ui->lineEdit->text().toStdString(),strError)){
         ui->lineEdit->clear();
         Q_EMIT confirm(4);
     }
     else{
         if(strError == "Invalid private key size")
             ui->label_error->setText(tr("Invalid private key size"));
         else if(strError == "Invalid private key encoding")
             ui->label_error->setText(tr("Invalid private key encoding"));
         else if(strError == "Invalid private key network")
             ui->label_error->setText(tr("Invalid private key network"));
         else if(strError == "invalid Base58 Input String")
             ui->label_error->setText(tr("invalid Base58 Input String"));
         else if(strError == "This is already had the private key")
             ui->label_error->setText(tr("This is already had the private key"));
         else if(strError == "Invalid private key size")
             ui->label_error->setText(tr("Invalid private key size"));
         else if(strError == "Error adding key to wallet")
             ui->label_error->setText(tr("Error adding key to wallet"));
         else
             ui->label_error->setText(tr("Other error"));

     }
}

void setimport::setWalletModel(WalletModel * model)
{
    walletModel = model;

}

