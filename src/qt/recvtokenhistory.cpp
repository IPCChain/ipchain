#include "recvtokenhistory.h"
#include "transactiontablemodel.h"
#include "ui_recvtokenhistory.h"
#include "log/log.h"

RecvTokenHistory::RecvTokenHistory(const QModelIndex &idx,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RecvTokenHistory)
{


    ui->setupUi(this);

    QString name = idx.data(TransactionTableModel::eCoinType).toString();

    QString num = idx.data(TransactionTableModel::eCoinNum).toString();
    QString desc = idx.data(TransactionTableModel::LongDescriptionRole).toString();

    QString add = idx.data(TransactionTableModel::AddressRole).toString();
    QString add1 = idx.data(TransactionTableModel::ToAddress).toString();

    QString lang = idx.data(TransactionTableModel::LanguageRole).toString();


    QString m_txid = idx.data(TransactionTableModel::TxIDRole).toString();
LOG_WRITE(LOG_INFO,"whatwhatwaaaa",m_txid.toStdString().c_str());

    QString m_status = idx.data(TransactionTableModel::InfoStatus).toString();
    QString m_time = idx.data(TransactionTableModel::InfoTime).toString();

  //  QString a = "状态:</b>";
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
     int datelen ;
    if(l != -1)
    {
        datelen = l-i-8;
    }
    else
    {
        datelen = k-i-8;
    }


   // QString  state_string  =  desc.mid(h+10,i-h-12);
  QString  state_string  =  desc.mid(h+8,i-h-12-3);
   // QString  date_string  =  desc.mid(i+8,12);
    QString  date_string  =  desc.mid(i+8,14);
    QString  count1_string  =  desc.mid(k+43,14);
    QString  count2_string  =  desc.mid(j+43,14);
    QString  add_string  =  desc.mid(l+7,34);
//QString  date_CHstring  =  desc.mid(i+8,l-i-8);
QString date_CHstring  =  desc.mid(i+8,datelen);
/*
    ui->nameEdit->setText(name);

    ui->timeEdit->setText(num);

    ui->infoEdit->setText(state_string);

    */
    ui->namelabel->setText(num+" "+name);

  //  ui->timelabel->setText(num);
   // ui->timelabel->setText(date_string);
    ui->add_label->setText(add);



    ui->timelabel->setText(m_time);
        ui->infolabel->setText(m_status);
}

RecvTokenHistory::~RecvTokenHistory()
{
    delete ui;
}
void RecvTokenHistory::showVisual(bool visual)
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
