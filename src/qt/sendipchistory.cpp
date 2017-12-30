#include "sendipchistory.h"
#include "ui_sendipchistory.h"
#include "transactiontablemodel.h"
#include <QMessageBox>

sendipchistory::sendipchistory(const QModelIndex &idx,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::sendipchistory)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);

    ui->scrollArea->setFrameShape(QFrame::NoFrame);  ;//隐藏表格线

   // QString desc = idx.data(TransactionTableModel::LongDescriptionRole).toString();
    QString add = idx.data(TransactionTableModel::AddressRole).toString();
    qint64 dex1 = idx.data(TransactionTableModel::AmountRole).toLongLong();

    QString add1 = idx.data(TransactionTableModel::ToAddress).toString();
    QString fee = idx.data(TransactionTableModel::feeAmount).toString();
    QString feeText = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, dex1, true, BitcoinUnits::separatorAlways);
    QString ipctitle = idx.data(TransactionTableModel::IPCTitle).toString();
    qint64 ipctype = idx.data(TransactionTableModel::IPCType).toLongLong();

    QString Authlimit = idx.data(TransactionTableModel::AuthLimit).toString();
    QString Authtype = idx.data(TransactionTableModel::AuthType).toString();
    QString Authtime = idx.data(TransactionTableModel::AuthTime).toString();

    //QString status = idx.data(TransactionTableModel::InfoStatus).toString();

    QString lang = idx.data(TransactionTableModel::LanguageRole).toString();


    QString m_status = idx.data(TransactionTableModel::InfoStatus).toString();
    QString m_time = idx.data(TransactionTableModel::InfoTime).toString();

    qint64 txfee = idx.data(TransactionTableModel::feeAmount).toLongLong();
    QString m_txfee = BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, txfee, false, BitcoinUnits::separatorAlways);
   /*
    QString c = "收入:</b>";
    QString d = "净额:</b>";
    QString e = "到:</b>";

    QString ff = "交易费:</b>";
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

    int mm=   desc.indexOf(ff);
    QString descfee = desc.mid(mm, -1);
    int mmm = descfee.indexOf("</span");

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

    QString  fee_string  =  desc.mid(mm+45,mmm-45);
*/
    ui->AddEdit->setText(add);
    // ui->AddEdit->setText(add_string);
    ui->ipcnameEdit->setText(ipctitle);


    if(Authtime == "forever")
    {
        ui->authdate->setText(tr("forever"));
    }else
    {
        if(Authtime.indexOf("forever") != -1)
        {
            int t = Authtime.indexOf("--");

            QString  X =  Authtime.mid(0,t);
            Authtime = X + tr("--")+tr("forever");
            ui->authdate->setText(Authtime);
        }
        else
        {
            ui->authdate->setText(Authtime);
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

    ui->ipctypelabel->setText(IPCType);
    // ui->ipcfeelabel->setText(feeText);


    // ui->ipctimelabel->setText(date_CHstring);
    // ui->ipcinfolabel->setText(state_string);
    ui->ipctimelabel->setText(m_time);
    ui->ipcinfolabel->setText(m_status);
    ui->ipcfeelabel->setText(m_txfee);
}

sendipchistory::~sendipchistory()
{
    delete ui;
}
void sendipchistory::showVisual(bool visual)
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
void sendipchistory::on_pushButton_pressed()
{
    Q_EMIT backMain();
}
