#include "ecoinsendresultdialog.h"
#include "forms/ui_ecoinsendresultdialog.h"

eCoinSendresultDialog::eCoinSendresultDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::eCoinSendresultDialog)
{
    ui->setupUi(this);
}

eCoinSendresultDialog::~eCoinSendresultDialog()
{
    delete ui;
}

void eCoinSendresultDialog::on_GoBtn_pressed()
{
    Q_EMIT GoSendeCoinSuccess();
}
