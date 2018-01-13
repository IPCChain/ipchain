#include "updatedialog.h"
#include "./forms/ui_updatedialog.h"
#include <QDesktopServices>
#include <QUrl>
extern bool g_bIpchainShutdown;
UpdateDialog::UpdateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateDialog)
{
    ui->setupUi(this);
    ui->textEdit_info->setEnabled(false);
}

UpdateDialog::~UpdateDialog()
{
    delete ui;
}
void UpdateDialog::setinfo(QString type,QString sugtext,QString url)
{
    ui->label_address->setText(url);
    if(type == "1")
        ui->pushButton_laterUpdate->hide();
    ui->textEdit_info->setText(sugtext);
}

void UpdateDialog::on_pushButton_goUpdate_pressed()
{
    this->accept();
    QString address = ui->label_address->text();
    QDesktopServices::openUrl(QUrl(address));
    g_bIpchainShutdown = true;
    QApplication::quit();
}

void UpdateDialog::on_pushButton_laterUpdate_pressed()
{
    this->accept();
}
