#include "ecoinconfirmsendaffrimdialog.h"
#include "ui_ecoinconfirmsendaffrimdialog.h"
#include "log/log.h"
ecoinconfirmsendaffrimdialog::ecoinconfirmsendaffrimdialog(QWidget *parent) :
    QWidget(parent),walletmodel(NULL),
    ui(new Ui::ecoinconfirmsendaffrimdialog)
{
    ui->setupUi(this);
}

ecoinconfirmsendaffrimdialog::~ecoinconfirmsendaffrimdialog()
{
    delete ui;
}
void ecoinconfirmsendaffrimdialog::setModel(WalletModel *_model)
{
    walletmodel = _model;
}
void ecoinconfirmsendaffrimdialog::settrainfo(QString num,QString add)
{
    ui->tokennum->setText(num);
    ui->address->setText(add);
}
void ecoinconfirmsendaffrimdialog::on_pushButton_send_pressed()
{
    if(!walletmodel) return;

    WalletModel::SendCoinsReturn sendStatus = walletmodel->sendecoinaffrim();

    if (sendStatus.status == WalletModel::OK)
    {
        Q_EMIT SendeCoinSuccess("");
    }
    else
    {

    }
}

void ecoinconfirmsendaffrimdialog::on_pushButton_back_pressed()
{
    Q_EMIT backtoecoinsend();
}
