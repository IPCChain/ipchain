#include "tallyoutaccount.h"
#include "forms/ui_tallyoutaccount.h"
#include "coincontroldialog.h"
#include "optionsmodel.h"
#include "bitcoinunits.h"
#include "wallet/coincontrol.h"
#include "guiutil.h"
#include "log/log.h"
TallyOutAccount::TallyOutAccount(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TallyOutAccount)
{
    ui->setupUi(this);
    ui->label_errmsg->setText("");
}

TallyOutAccount::~TallyOutAccount()
{
    delete ui;
}
void TallyOutAccount::setModel(WalletModel * model)
{
    walletmodel = model;
    if(walletmodel && walletmodel->getOptionsModel())
    {
    }

}
void TallyOutAccount::on_pushButton_pressed()
{

}

void TallyOutAccount::on_pushButton_Cancle_pressed()
{
   Q_EMIT next(false);
}
void TallyOutAccount::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPalette pa;
    pa.setColor(QPalette::WindowText,Qt::black);

    switch(sendCoinsReturn.status)
    {
    case WalletModel::InvalidAddress:
        LOG_WRITE(LOG_INFO,"TallyOutAccount","InvalidAddress");
        ui->label_errmsg->setText(tr("InvalidAddress"));
        break;
    case WalletModel::InvalidAmount:
        LOG_WRITE(LOG_INFO,"TallyOutAccount","InvalidAmount");
        ui->label_errmsg->setText(tr("InvalidAmount"));
        break;
    case WalletModel::AmountExceedsBalance:
        LOG_WRITE(LOG_INFO,"TallyOutAccount","AmountExceedsBalance");
        ui->label_errmsg->setText(tr("AmountExceedsBalance"));
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        LOG_WRITE(LOG_INFO,"TallyOutAccount","AmountWithFeeExceedsBalance");
        ui->label_errmsg->setText(tr("AmountWithFeeExceedsBalance"));
        break;
    case WalletModel::DuplicateAddress:
        LOG_WRITE(LOG_INFO,"TallyOutAccount","DuplicateAddress");
        ui->label_errmsg->setText(tr("DuplicateAddress"));
        break;
    case WalletModel::TransactionCreationFailed:
        LOG_WRITE(LOG_INFO,"TallyOutAccount","TransactionCreationFailed",m_error.c_str());
        if(!setlabel_errmsg())
            ui->label_errmsg->setText(tr("TransactionCreationFailed") + tr(" ") + m_error.c_str());
        break;
    case WalletModel::TransactionCommitFailed:
        LOG_WRITE(LOG_INFO,"TallyOutAccount","TransactionCommitFailed",m_error.c_str());
        if(!setlabel_errmsg())
            ui->label_errmsg->setText(tr("TransactionCommitFailed") + tr(" ") + m_error.c_str());
        break;
    case WalletModel::AbsurdFee:
        LOG_WRITE(LOG_INFO,"TallyOutAccount","AbsurdFee");
        ui->label_errmsg->setText(tr("AbsurdFee"));
        break;
    case WalletModel::PaymentRequestExpired:
        LOG_WRITE(LOG_INFO,"TallyOutAccount","PaymentRequestExpired");

        ui->label_errmsg->setText(tr("PaymentRequestExpired"));
        break;
    case WalletModel::OK:
        LOG_WRITE(LOG_INFO,"TallyOutAccount","OK");
        ui->label_errmsg->setText(tr(""));
    default:
        LOG_WRITE(LOG_INFO,"TallyOutAccount",\
                 QString::number(sendCoinsReturn.status).toStdString().c_str());
        ui->label_errmsg->setText(tr("other error"));
        return;
    }

  //  Q_EMIT message(tr("Send Coins"), msgParams.first, msgParams.second);
}



void TallyOutAccount::setinfo( WalletModel::keepupaccountInfo info)
{
   info_.Add_=info.Add_;
   info_.Coin_=info.Coin_;
   clearinfo();
}
 WalletModel::keepupaccountInfo  TallyOutAccount::getinfo()
{
  return info_;
}
void TallyOutAccount::on_pushButton_OK_pressed()
{
    m_error = "";
    if(!walletmodel || !walletmodel->getOptionsModel())
        return;
    if(!walletmodel->CheckPassword())
    {
         ui->label_errmsg->setText(tr("password error"));
         return;
    }

    WalletModel::UnlockContext ctx(walletmodel, true, true);
    WalletModel::SendCoinsReturn prepareStatus;
    CCoinControl ctrl;

    if (walletmodel->getOptionsModel()->getCoinControlFeatures())
    {
        ctrl = *CoinControlDialog::coinControl;

    }
    else
    {

        ctrl.nConfirmTarget = 1;
    }
     prepareStatus = walletmodel->prepareExitBookkeeping( &ctrl,m_error);
     processSendCoinsReturn(prepareStatus);
    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
         return;

    }
    else
    {
        fNewRecipientAllowed = true;
        Q_EMIT next(true);
    }

}
void TallyOutAccount::clearinfo()
{
    ui->label_errmsg->setText("");
}

/*
Not join the campaign yet ---- 当前钱包并没有加入共识！
Transaction amounts must not be negative ---- 交易中ipc数额不合法（小于0）
Transaction must have at least one recipient --- 收件人为空
The Address which you want to ExitCampaign must have some money!
-- 推出竞选交易的钱包地址需要有余额来支付退出竞选的交易费。
Transaction amount too small ----  微尘交易，拒绝！
Insufficient funds --- 推出竞选交易的钱包地址需要有余额来支付退出竞选的交易费。
Keypool ran out, please call keypoolrefill first --- 密钥池推出，请先激活密钥池。
The transaction amount is too small to send after the fee has been deducted --
微尘交易
Signing transaction failed  -- 交易签名失败
Transaction too large for fee policy  -- 不合法的交易费金额(小于最小的交易费费率费用)
Transaction too large  --  交易尺寸过大。
Transaction has too long of a mempool chain -- 当前交易池没有足够空间放入构造的交易，构建失败。
*/
bool TallyOutAccount::setlabel_errmsg()
{
    if(m_error == "Not join the campaign yet"){
        ui->label_errmsg->setText(tr("Not join the campaign yet"));
    }else if(m_error == "Transaction amounts must not be negative"){
        ui->label_errmsg->setText(tr("Transaction amounts must not be negative"));
    }else if(m_error == "Transaction must have at least one recipient"){
        ui->label_errmsg->setText(tr("Transaction must have at least one recipient"));
    }else if(m_error == "ExitCampaign The Address which you want to ExitCampaign must have some money!"){
        ui->label_errmsg->setText(tr("ExitCampaign The Address which you want to ExitCampaign must have some money!"));
    }else if(m_error == "The Address which you want to ExitCampaign must have some money!"){
        ui->label_errmsg->setText(tr("The Address which you want to ExitCampaign must have some money!"));
    }else if(m_error == "Transaction amount too small"){
        ui->label_errmsg->setText(tr("Transaction amount too small"));
    }else if(m_error == "Insufficient funds"){
        ui->label_errmsg->setText(tr("Insufficient funds"));
    }else if(m_error == "Keypool ran out, please call keypoolrefill first"){
        ui->label_errmsg->setText(tr("Keypool ran out, please call keypoolrefill first"));
    }else if(m_error == "The transaction amount is too small to send after the fee has been deducted"){
        ui->label_errmsg->setText(tr("The transaction amount is too small to send after the fee has been deducted"));
    }else if(m_error == "Signing transaction failed"){
        ui->label_errmsg->setText(tr("Signing transaction failed"));
    } else if(m_error == "Transaction too large for fee policy"){
        ui->label_errmsg->setText(tr("Transaction too large for fee policy"));
    } else if(m_error == "Transaction too large"){
        ui->label_errmsg->setText(tr("Transaction too large"));
    } else if(m_error == "Transaction has too long of a mempool chai"){
        ui->label_errmsg->setText(tr("Transaction has too long of a mempool chai"));
    } else if(m_error == "txn-campaign-EXIT_PUBKEY_NOT_EXIST_IN_LIST"){
        ui->label_errmsg->setText(tr("txn-campaign-EXIT_PUBKEY_NOT_EXIST_IN_LIST"));
    }else{
        return false;
    }
    return true;
}
