#include "ipcconfirmtransfertransaction.h"
#include "ui_ipcconfirmtransfertransaction.h"

ipcconfirmtransfertransaction::ipcconfirmtransfertransaction(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ipcconfirmtransfertransaction)
{
    ui->setupUi(this);
}

ipcconfirmtransfertransaction::~ipcconfirmtransfertransaction()
{
    delete ui;
}
void ipcconfirmtransfertransaction::setInfo(QString name,QString address)
{
    ui->ipcname->setText(name);
    ui->address->setText(address);
    ui->tiplabel->setText("");
}
void ipcconfirmtransfertransaction::setModel(WalletModel *_model)
{
    this->walletModel = _model;
}
void ipcconfirmtransfertransaction::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn)
{
    QPalette pa;
    pa.setColor(QPalette::WindowText,Qt::black);

    switch(sendCoinsReturn.status)
    {
    case WalletModel::InvalidAddress:
        ui->tiplabel->setText(tr("The recipient address is not valid. Please recheck."));
        break;
    case WalletModel::InvalidAmount:
        ui->tiplabel->setText(tr("The amount to pay must be larger than 0."));
        break;
    case WalletModel::AmountExceedsBalance:
        ui->tiplabel->setText(tr("The amount exceeds your balance."));
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        ui->tiplabel->setText(tr("The total exceeds your balance when the 0.001IPC/KB transaction fee is included."));
        break;
    case WalletModel::DuplicateAddress:
        ui->tiplabel->setText(tr("Duplicate address found: addresses should only be used once each."));
        break;
    case WalletModel::TransactionCreationFailed:
     // if(m_sendcoinerror!="")
      //     ui->tiplabel->setText(m_sendcoinerror.c_str());
    //    else
            ui->tiplabel->setText(tr("Transaction creation failed!"));
        break;
    case WalletModel::TransactionCommitFailed:
    {
        if("bad-Token-tokensymbol-repeat" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: bad-Token-tokensymbol-repeat"));

        }
        else if("bad-Token-tokenhash-repeat" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: bad-Token-tokenhash-repeat"));

        }
        else if("bad-Token-Multi-inType" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: bad-Token-Multi-inType"));

        }
        else if("bad-Token-Reg-issueDate(Regtime)" == sendCoinsReturn.reasonCommitFailed)
        {
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: bad-Token-Reg-issueDate(Regtime)"));

        }
        else if("bad-Token-regtotoken-value-unequal" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: bad-Token-regtotoken-value-unequal"));
        }
        else if("bad-Token-Label-contain-errvalue"== sendCoinsReturn.reasonCommitFailed)
        {
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: bad-Token-Label-contain-errvalue"));
        }
        else if("bad-Token-Symbol-contain-errvalue"== sendCoinsReturn.reasonCommitFailed)
        {
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: bad-Token-Symbol-contain-errvalue"));
        }
        else if("bad-Token-value-unequal" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: bad-Token-value-unequal"));
        }
        else if("bad-Token-Multi-outType" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: bad-Token-Multi-outType"));
        }
        else
        {
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: %1").arg(sendCoinsReturn.reasonCommitFailed));
        }
    }
        break;
    case WalletModel::AbsurdFee:
        ui->tiplabel->setText(tr("A fee higher is considered an absurdly high fee."));
        break;
    case WalletModel::PaymentRequestExpired:
        ui->tiplabel->setText(tr("Payment request expired."));
        break;
    case WalletModel::OK:
    default:
        return;
    }
}

void ipcconfirmtransfertransaction::on_pushButton_confirmtransferipc_pressed()
{
    QString msg;
    WalletModel::SendCoinsReturn Status = walletModel->sendtransferIpc();
    if(WalletModel::OK == Status.status)
    {
      Q_EMIT gotoSuccessfultradePage(0);
    }
    else
    {
     // m_sendcoinerror =  walletModel->m_sendcoinerror;
    }
    processSendCoinsReturn(Status);
}

void ipcconfirmtransfertransaction::on_pushButton_back_pressed()
{
    Q_EMIT backtoipctransfertransaction();
}
