#include "sendhistory.h"
#include "ui_sendhistory.h"
#include "transactiontablemodel.h"
#include <QMessageBox>

sendhistory::sendhistory(const QModelIndex &idx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sendhistory)
{
    ui->setupUi(this);
  //  QString desc = idx.data(TransactionTableModel::LongDescriptionRole).toString();
    qint64 dex1 = idx.data(TransactionTableModel::AmountRole).toLongLong();

    qint64 over_amount = idx.data(TransactionTableModel::overAmount).toLongLong();

    ///QString amountText = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, dex1, true, BitcoinUnits::separatorAlways);
    QString amountText = BitcoinUnits::formatWithUnit4(BitcoinUnit::IPC, dex1, true, BitcoinUnits::separatorAlways);
    if("0.0000 IPC"==amountText || "+0.0000 IPC"==amountText||"-0.0000 IPC"==amountText)
    {
        amountText = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, dex1);

    }
    qint64 fee = idx.data(TransactionTableModel::Amount).toLongLong();
    QString feeText = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, dex1, true, BitcoinUnits::separatorAlways);
    QString add = idx.data(TransactionTableModel::AddressRole).toString();




    qint64 txfee = idx.data(TransactionTableModel::feeAmount).toLongLong();
    QString m_txfee = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, txfee, false, BitcoinUnits::separatorAlways);

    QString add1 = idx.data(TransactionTableModel::ToAddress).toString();


    //  QString status = idx.data(TransactionTableModel::InfoStatus).toString();
    QString lang = idx.data(TransactionTableModel::LanguageRole).toString();


    QString m_status = idx.data(TransactionTableModel::InfoStatus).toString();
    QString m_time = idx.data(TransactionTableModel::InfoTime).toString();
   /*
    QString c = "支出:</b>";
    QString d = "交易费用:</b>";
    QString e = "到:</b>";
    QString ff = "交易费:</b>";



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


    int h=   desc.indexOf(a);
    int i=   desc.indexOf(b);
    int k=   desc.indexOf(c);
    int j=   desc.indexOf(d);
    int l=   desc.indexOf(e);
    int mm=   desc.indexOf(ff);



    QString descc = desc.mid(k, -1);
    int cc = descc.indexOf("</span");


    QString descfee = desc.mid(mm, -1);
    int mmm = descfee.indexOf("</span");

    //  QString  state_string  =  desc.mid(h+10,i-h-12);
    QString  state_string  ;
    QString  date_CHstring ;
    if("Chinese"== lang)
    {
        state_string  =  desc.mid(h+8,i-h-12-3);
        date_CHstring  =  desc.mid(i+8,l-i-8);
    }
    else
    {
        state_string  =  desc.mid(h+10,i-h-12-3);
        date_CHstring  =  desc.mid(i+10,l-i-8);

    }
    QString  date_string  =  desc.mid(i+8,14);
    QString  count1_string  =  desc.mid(k+43,14);

    QString  count2_string  =  desc.mid(k+43,cc-14);//+43,jjj);//14);
    QString  add_string  =  desc.mid(l+7,35);

    QString  fee_string  =  desc.mid(mm+45,mmm-45);
    QString f = "金额:</b>";
    int m = desc.indexOf(f);
    QString amount = desc.mid(m,50);
    //ui->coinEdit->setText(amountText);
    //交易费:
*/
    ui->coinEdit->setText(amountText);//count2_string);
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
void sendhistory::on_pushButton_pressed()
{
    Q_EMIT backMain();
}
