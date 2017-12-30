#include "recvhistory.h"
#include "ui_recvhistory.h"

#include "bitcoinunits.h"
#include "log/log.h"
#include "transactiontablemodel.h"
#include <QMessageBox>

RecvHistory::RecvHistory(const QModelIndex &idx,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RecvHistory)
{
    ui->setupUi(this);

    setWindowTitle(tr("Details for %1").arg(idx.data(TransactionTableModel::TxIDRole).toString()));
  //  QString desc = idx.data(TransactionTableModel::LongDescriptionRole).toString();
    QString dex = idx.data(TransactionTableModel::Amount).toString();
    qint64 dex1 = idx.data(TransactionTableModel::AmountRole).toLongLong();

    ///QString amountText = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, dex1, true, BitcoinUnits::separatorAlways);
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
    QString lang = idx.data(TransactionTableModel::LanguageRole).toString();

    QString c = "收入:</b>";
    QString d = "净额:</b>";
    QString e = "到:</b>";
    QString sour ="源:</b>";
    QString fr ="来自:</b>";


    QString a ;
    QString b ;
    if("Chinese"== lang)
    {
        a = "状态:</b>";
        b = "日期:</b>";

    }
    else
    {
        a = "Status:</b>";
        b = "Date:</b>";
    }





   // int h=   desc.indexOf(a);
   // int i=   desc.indexOf(b);
   // int k=   desc.indexOf(c);
  //  int j=   desc.indexOf(d);
  //  int l=   desc.indexOf(e);
/*
    int frint = desc.indexOf(fr);
    int soint = desc.indexOf(sour);
    int datelen;
    if(soint != -1)
    {

        datelen = soint-i-8;
    }
    else
    {
        datelen = frint-i-8;
    }
    */
    //QString  state_string  =  desc.mid(h+10,i-h-12);
    QString  state_string ;
    QString  date_string  ;
  /*
    if("Chinese"== lang)
    {
     //   QString  state_string  =  desc.mid(h+8,i-h-12-3);
        QString  date_string  =  desc.mid(i+8,16);
    }
    else
    {
       // QString  state_string  =  desc.mid(h+8,i-h-12-3);
        QString  date_string  =  desc.mid(i+10,16);
    }
    QString  count1_string  =  desc.mid(k+43,14);
    QString  count2_string  =  desc.mid(j+43,14);
    QString  add_string  =  desc.mid(l+4,30);
    //     QString  date_CHstring  =  desc.mid(i+8,l-i-8);
    QString  date_CHstring2  =  desc.mid(i+8,datelen);
    QString  date_CHstring1  =  desc.mid(i+8,l-i-8-24);


    QString f = "金额=";
    int m = desc.indexOf(f);
    QString amount = desc.mid(m,50);
*/

    ui->coinEdit->setText(amountText);
    ui->add_label->setText(add);
    ui->datelabel->setText(m_time);
    ui->infolabel->setText(m_status);
}

RecvHistory::~RecvHistory()
{
    delete ui;
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
void RecvHistory::on_pushButton_pressed()
{
    Q_EMIT backMain();
}
