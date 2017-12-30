#include "sendtokenhistory.h"
#include "transactiontablemodel.h"
#include "ui_sendtokenhistory.h"
#include <QMessageBox>

SendTokenHistory::SendTokenHistory(const QModelIndex &idx,QWidget *parent)  :
    QWidget(parent),
    ui(new Ui::SendTokenHistory)
{
    ui->setupUi(this);


    QString name = idx.data(TransactionTableModel::eCoinType).toString();

    QString num = idx.data(TransactionTableModel::eCoinNum).toString();
    QString desc = idx.data(TransactionTableModel::LongDescriptionRole).toString();

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


   // QString a = "状态:</b>";
   // QString b = "日期:</b>";
    QString c = "收入:</b>";
    QString d = "净额:</b>";
    QString e = "到:</b>";


    QString f = "IPC名称";
    QString g ="IPC类型";






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




    int m =desc.indexOf(f);
    int n = desc.indexOf(g);
    QString ipctitl = desc.mid(m,100);
    QString ipctyp = desc.mid(n,100);



    int h=   desc.indexOf(a);
    int i=   desc.indexOf(b);
    int k=   desc.indexOf(c);
    int j=   desc.indexOf(d);
    int l=   desc.indexOf(e);
   // QString  state_string  =  desc.mid(h+10,i-h-12);
      QString  state_string  =  desc.mid(h+8,i-h-12-3);
    QString  date_string  =  desc.mid(i+8,14);
    QString  count1_string  =  desc.mid(k+43,14);
    QString  count2_string  =  desc.mid(j+43,14);
    QString  add_string  =  desc.mid(l+7,34);
    QString  date_CHstring  =  desc.mid(i+8,l-i-8);
    /*
    ui->nameEdit->setText(name);

    ui->timeEdit->setText(num);

    ui->infoEdit->setText(state_string);
    */
    ui->addText->setText(add);
 //   ui->feeText->setText(feeText);
    ui->namelabel->setText("-"+num+" "+name);
    //ui->timelabel->setText( date_CHstring);
    //ui->infolabel->setText(state_string);
    ui->timelabel->setText(m_time);
    ui->infolabel->setText(m_status);
    ui->feeText->setText(m_txfee);
}

SendTokenHistory::~SendTokenHistory()
{
    delete ui;
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
