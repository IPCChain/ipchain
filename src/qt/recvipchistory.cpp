#include "recvipchistory.h"
#include "ui_recvipchistory.h"
#include "transactiontablemodel.h"
#include <QMessageBox>
recvipchistory::recvipchistory(const QModelIndex &idx,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::recvipchistory)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);
    ui->scrollArea->setFrameShape(QFrame::NoFrame);
   // QString desc = idx.data(TransactionTableModel::LongDescriptionRole).toString();
    QString add = idx.data(TransactionTableModel::AddressRole).toString();
    QString add1 = idx.data(TransactionTableModel::ToAddress).toString();

    QString ipctitle = idx.data(TransactionTableModel::IPCTitle).toString();
    qint64 ipctype = idx.data(TransactionTableModel::IPCType).toLongLong();

    QString Authlimit = idx.data(TransactionTableModel::AuthLimit).toString();
    QString Authtype = idx.data(TransactionTableModel::AuthType).toString();
    QString Authtime = idx.data(TransactionTableModel::AuthTime).toString();
    QString lang = idx.data(TransactionTableModel::LanguageRole).toString();


    QString m_status = idx.data(TransactionTableModel::InfoStatus).toString();
    QString m_time = idx.data(TransactionTableModel::InfoTime).toString();
  /*
    QString c = "收入:</b>";
    QString d = "净额:</b>";
    QString e = "到:</b>";


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


    int datelen ;
    if(l != -1)
    {
        datelen = l-i-8;
    }
    else
    {
        datelen = k-i-8;
    }
    QString  state_string  =  desc.mid(h+8,i-h-12-3);
    //  QString  state_string  =  desc.mid(h+10,i-h-12);
    QString  date_string  =  desc.mid(i+8,14);
    QString  count1_string  =  desc.mid(k+43,14);
    QString  count2_string  =  desc.mid(j+43,14);
    QString  add_string  =  desc.mid(h,50);
    QString  date_CHstring  =  desc.mid(i+8,datelen);
*/
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

    ui->ipclabel->setText(ipctitle);
    ui->typelabel->setText(IPCType);
    ui->add_label->setText(add);
    //  ui->infolabel->setText(state_string);
    //ui->timelabel->setText(date_CHstring);



    ui->infolabel->setText(m_status);
    ui->timelabel->setText(m_time);

}

recvipchistory::~recvipchistory()
{
    delete ui;
}

void recvipchistory::on_pushButton_pressed()
{
    Q_EMIT backMain();
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
