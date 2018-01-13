#include "walletpassword.h"
#include "ui_walletpassword.h"
#include <QDesktopWidget>
WalletPassword::WalletPassword(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WalletPassword)
{
    Qt::WindowFlags flags=Qt::Dialog;
    flags |=Qt::WindowCloseButtonHint;
    setWindowFlags(flags);
    ui->setupUi(this);
}

WalletPassword::~WalletPassword()
{
    delete ui;
}

void WalletPassword::on_pushButton_ok_pressed()
{
    m_password = ui->lineEdit->text();
    this->accept();
}
void WalletPassword::setDlgParams()
{
    QDesktopWidget* desktop = QApplication::desktop();
    move((desktop->width() - this->width())/2, (desktop->height() - this->height())/2);
}

