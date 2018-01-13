#include "recvhistory.h"
#include "ui_recvhistory.h"
#include "ipchainunits.h"
#include "log/log.h"
#include "transactiontablemodel.h"

RecvHistory::RecvHistory(const QModelIndex &idx,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RecvHistory)
{
    ui->setupUi(this);

    setWindowTitle(tr("Details for %1").arg(idx.data(TransactionTableModel::TxIDRole).toString()));
    QString dex = idx.data(TransactionTableModel::Amount).toString();
    qint64 dex1 = idx.data(TransactionTableModel::AmountRole).toLongLong();

    QString amountText = BitcoinUnits::formatWithUnit4(BitcoinUnit::IPC, dex1);

    if("0.0000 IPC"==amountText || "+0.0000 IPC"==amountText||"-0.0000 IPC"==amountText)
    {
        amountText = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, dex1);
    }
    QString add = idx.data(TransactionTableModel::AddressRole).toString();
    QString add1 = idx.data(TransactionTableModel::ToAddress).toString();
    QString m_status = idx.data(TransactionTableModel::InfoStatus).toString();
    QString m_time = idx.data(TransactionTableModel::InfoTime).toString();
    QString m_txid = idx.data(TransactionTableModel::TxIDRole).toString();
    LOG_WRITE(LOG_INFO,"RecvHistory++hashid",m_txid.toStdString().c_str());
    QString lang = idx.data(TransactionTableModel::LanguageRole).toString();

    ui->coinEdit->setText("+"+amountText);
    ui->add_label->setText(add);
    ui->datelabel->setText(m_time);
    ui->infolabel->setText(m_status);
}

RecvHistory::~RecvHistory()
{
    delete ui;
}

void RecvHistory::updateInfo(QString status)
{
    ui->infolabel->setText(status);
}
void RecvHistory::showVisual(bool visual)
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

