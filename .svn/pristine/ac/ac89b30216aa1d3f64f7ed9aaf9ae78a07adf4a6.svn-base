#include "sendtokenhistory.h"
#include "transactiontablemodel.h"
#include "ui_sendtokenhistory.h"
#include <QMessageBox>
#include "log/log.h"
SendTokenHistory::SendTokenHistory(const QModelIndex &idx,QWidget *parent)  :
    QWidget(parent),
    ui(new Ui::SendTokenHistory)
{
    ui->setupUi(this);

    QString m_txid = idx.data(TransactionTableModel::TxIDRole).toString();
    LOG_WRITE(LOG_INFO,"sendtokenhistory++hashid",m_txid.toStdString().c_str());

    QString name = idx.data(TransactionTableModel::eCoinType).toString();

    QString num = idx.data(TransactionTableModel::eCoinNum).toString();
    QString add = idx.data(TransactionTableModel::AddressRole).toString();
    qint64 dex1 = idx.data(TransactionTableModel::AmountRole).toLongLong();

    QString add1 = idx.data(TransactionTableModel::ToAddress).toString();
    QString fee = idx.data(TransactionTableModel::feeAmount).toString();
    QString feeText = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, dex1, true, BitcoinUnits::separatorAlways);
    QString lang = idx.data(TransactionTableModel::LanguageRole).toString();


    qint64 txfee = idx.data(TransactionTableModel::feeAmount).toLongLong();
    QString m_txfee = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, txfee, false, BitcoinUnits::separatorAlways);

    QString m_status = idx.data(TransactionTableModel::InfoStatus).toString();
    QString m_time = idx.data(TransactionTableModel::InfoTime).toString();
    ui->addText->setText(add);
    ui->namelabel->setText("-"+num+" "+name);
    ui->timelabel->setText(m_time);
    ui->infolabel->setText(m_status);
    ui->feeText->setText(m_txfee);
}

SendTokenHistory::~SendTokenHistory()
{
    delete ui;
}

void SendTokenHistory::updateInfo(QString status)
{
    ui->infolabel->setText(status);
}
void SendTokenHistory::showVisual(bool visual)
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
