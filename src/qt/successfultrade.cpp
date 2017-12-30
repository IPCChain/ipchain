#include "successfultrade.h"
#include "ui_successfultrade.h"

SuccessfulTrade::SuccessfulTrade(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SuccessfulTrade)
{
    ui->setupUi(this);
    ui->pushButton_ok->hide();

}

SuccessfulTrade::~SuccessfulTrade()
{
    delete ui;
}

void SuccessfulTrade::on_pushButton_ok_pressed()
{
    Q_EMIT back();
}
void SuccessfulTrade::setSuccessText(int type)
{
    if(type==successtrade)
    {
        ui->label_successtext->setText(tr("successful trade"));
    }
    else if(type==successexport)
    {
        ui->label_successtext->setText(tr("successful export"));
    }
    else if(type == successrevovery)
    {
        ui->label_successtext->setText(tr("successful revovery"));
    }
    else if(type ==successpsdset)
    {
        ui->label_successtext->setText(tr("successful psdset"));
    }
    else
    {
        ui->label_successtext->setText("");
    }

}
