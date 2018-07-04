#include "unionacounthistorydetailr.h"
#include "ui_unionacounthistorydetailr.h"
#include "ipchainunits.h"
#include "log/log.h"
#include "walletmodel.h"
extern std::map<uint256,const CWalletTx*> vCoins;
unionacounthistorydetailR::unionacounthistorydetailR(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::unionacounthistorydetailR)
{
    ui->setupUi(this);
}

unionacounthistorydetailR::~unionacounthistorydetailR()
{
    delete ui;
}
void unionacounthistorydetailR::onmessagetra(QString &status)
{
    LOG_WRITE(LOG_INFO,"STATUS",status.toStdString().c_str());
    if(status.indexOf("Open")!=-1)
    {
        int i = status.indexOf("Open until");
        QString  s1= status.mid(i,-1);
        ui->label_info->setText(tr("Open until") + s1);
    }
    else if(status.indexOf("conflicted")!=-1)
    {
        int i = status.indexOf("conflicted with a transaction with");
        QString  s1= status.mid(i,-1);
        ui->label_info->setText(tr("conflicted with a transaction with") + s1);
    }
    else if(status.indexOf("offline")!=-1)
    {
        int i = status.indexOf("offline");
        QString  s1= status.mid(0,i);
        ui->label_info->setText(s1+ tr("offline"));
    }
    else if(status.indexOf("unconfirmed") !=-1)
    {
        int i = status.indexOf("unconfirmed");
        QString  s1= status.mid(0,i);
        ui->label_info->setText(s1+ tr("unconfirmed"));
    }
    else if(status.indexOf("confirmations") != -1)
    {
        int i = status.indexOf("confirmations");
        QString  s1= status.mid(0,i);
        LOG_WRITE(LOG_INFO,"confirmations",s1.toStdString().c_str());
        ui->label_info->setText(s1+ tr("confirmations"));
    }
    else
        ui->label_info->setText(tr("unconfirmed"));
}

void unionacounthistorydetailR::setinfo(std::string add, CAmount fee,bool isSend,CAmount num,QString status,QString strtime,QString txid)
{
    m_txid = txid;
    ui->label_add->setText(QString::fromStdString(add));
    ui->textEdit->setText("+"+BitcoinUnits::formatWithUnit(0, num, false, BitcoinUnits::separatorAlways));
    ui->label_date->setText(strtime);
    //ui->label_info->setText(status);
    uint256 rv;
    rv.SetHex(m_txid.toStdString());
    auto itor = vCoins.find(rv);
    if(itor!= vCoins.end()){
       status = m_walletmodel->FormatTxStatus(*(itor->second));
    }
    onmessagetra(status);

}

void unionacounthistorydetailR::on_btn_back_pressed()
{
   Q_EMIT unionRPage_back();
}
