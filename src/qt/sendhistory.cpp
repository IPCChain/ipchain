#include "sendhistory.h"
#include "ui_sendhistory.h"
#include "transactiontablemodel.h"
#include <QMessageBox>
#include "log/log.h"

sendhistory::sendhistory(const QModelIndex &idx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sendhistory)
{
    ui->setupUi(this);
    qint64 dex1 = idx.data(TransactionTableModel::AmountRole).toLongLong();
    qint64 over_amount = idx.data(TransactionTableModel::overAmount).toLongLong();
    QString amountText = BitcoinUnits::formatWithUnit4(BitcoinUnit::IPC, dex1, true, BitcoinUnits::separatorAlways);
    if("0.0000 IPC"==amountText || "+0.0000 IPC"==amountText||"-0.0000 IPC"==amountText)
    {
        amountText = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, dex1);
    }
    qint64 fee = idx.data(TransactionTableModel::Amount).toLongLong();
    QString feeText = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, dex1, true, BitcoinUnits::separatorAlways);
    QString add = idx.data(TransactionTableModel::AddressRole).toString();
    QString m_txid = idx.data(TransactionTableModel::TxIDRole).toString();
    LOG_WRITE(LOG_INFO,"sendhistory++hashid",m_txid.toStdString().c_str());
    qint64 txfee = idx.data(TransactionTableModel::feeAmount).toLongLong();
    QString m_txfee = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, txfee, false, BitcoinUnits::separatorAlways);
    QString add1 = idx.data(TransactionTableModel::ToAddress).toString();
    QString lang = idx.data(TransactionTableModel::LanguageRole).toString();
    QString m_status = idx.data(TransactionTableModel::InfoStatus).toString();
    QString m_time = idx.data(TransactionTableModel::InfoTime).toString();
    ui->coinEdit->setText(amountText);
    ui->AddEdit->setText(add);
    QString mtr_status;
    ui->timelabel->setText(m_time);
    ui->infolabel->setText(m_status);
    ui->feelabel->setText(m_txfee);
}
sendhistory::~sendhistory()
{
    delete ui;
}
void sendhistory::updateInfo(QString status)
{
    ui->infolabel->setText(status);
}
void sendhistory::showVisual(bool visual)
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
