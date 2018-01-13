#include "recvipchistory.h"
#include "ui_recvipchistory.h"
#include "transactiontablemodel.h"
#include "log/log.h"
#include <QMessageBox>
recvipchistory::recvipchistory(const QModelIndex &idx,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::recvipchistory)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);
    ui->scrollArea->setFrameShape(QFrame::NoFrame);
    QString add = idx.data(TransactionTableModel::AddressRole).toString();
    QString add1 = idx.data(TransactionTableModel::ToAddress).toString();

    QString ipctitle = idx.data(TransactionTableModel::IPCTitle).toString();
    qint64 ipctype = idx.data(TransactionTableModel::IPCType).toLongLong();

    QString Authlimit = idx.data(TransactionTableModel::AuthLimit).toString();
    QString Authtype = idx.data(TransactionTableModel::AuthType).toString();
    QString Authtime = idx.data(TransactionTableModel::AuthTime).toString();
    QString lang = idx.data(TransactionTableModel::LanguageRole).toString();
    QString m_txid = idx.data(TransactionTableModel::TxIDRole).toString();
    LOG_WRITE(LOG_INFO,"recvipchistory++hashid",m_txid.toStdString().c_str());
    QString m_status = idx.data(TransactionTableModel::InfoStatus).toString();
    QString m_time = idx.data(TransactionTableModel::InfoTime).toString();
    qint64 txfee = idx.data(TransactionTableModel::feeAmount).toLongLong();
    QString m_txfee = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, txfee, false, BitcoinUnits::separatorAlways);
    if(txfee != 0)
    {
        QString m_txfee = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, txfee, false, BitcoinUnits::separatorAlways);
        ui->feeLabel->show();
        ui->feelabel->show();
        ui->widget_6->show();
        ui->line_9->show();
        ui->feeLabel->setText(m_txfee);
    }else{
        ui->feeLabel->hide();
        ui->feelabel->hide();
        ui->widget_6->hide();
        ui->line_9->hide();
    }

    if(Authtime == "forever")
    {
        ui->authdata->setText(tr("forever"));
    }else
    {
        if(Authtime.indexOf("forever") != -1)
        {
            int t = Authtime.indexOf("--");
            QString  X =  Authtime.mid(0,t);
            Authtime = X + tr("--")+tr("forever");
            ui->authdata->setText(Authtime);
        }
        else
        {
            ui->authdata->setText(Authtime);
        }

    }
    if("ownership"==Authtype){
        ui->authtype->setText(tr("ownership"));
    }else{
        ui->authtype->setText(tr("Use right"));
    }
    if("can authorization"==Authlimit){
        ui->authlimit->setText(tr("can authorization"));
    }else{
        ui->authlimit->setText(tr("cannot authorization"));
    }

    QStringList type = QStringList()<<tr("patent")<<tr("trademark")<<tr("Electronic document")<<tr("Photo")<<tr("Journalism")<<tr("video")<<tr("audio frequency")<<tr("security code");
    QString IPCType;
    if(ipctype>=0&&ipctype<8)
    {
        IPCType = type.at(ipctype);
    }
    else
    {
        IPCType =tr("patent");
    }
    ui->typelabel->setText(IPCType);
    ui->add_label->setText(add);
    ui->infolabel->setText(m_status);
    ui->timelabel->setText(m_time);
    ui->ipclabel->setText(ipctitle);

}

recvipchistory::~recvipchistory()
{
    delete ui;
}
void recvipchistory::updateInfo(QString status)
{
    ui->infolabel->setText(status);
}

void recvipchistory::showVisual(bool visual)
{
    if(visual)
    {
        setVisible(true);
    }
    else
    {
        setVisible(false);
    }
}
