#include "ecoinconfirmcreatedialog.h"
#include "ui_ecoinconfirmcreatedialog.h"
#include "walletmodel.h"
#include "log/log.h"

ecoinconfirmcreatedialog::ecoinconfirmcreatedialog(QWidget *parent):
    QWidget(parent),walletModel(NULL),
    ui(new Ui::ecoinconfirmcreatedialog)
{
    ui->setupUi(this);
    ui->tiplabel->setText("");
}

ecoinconfirmcreatedialog::~ecoinconfirmcreatedialog()
{
    delete ui;
}
void ecoinconfirmcreatedialog::setModel(WalletModel *_model)
{
    this->walletModel = _model;
}
void ecoinconfirmcreatedialog::settrainfo(QStringList data)
{
    ui->tokenname->setText(data.at(0).toStdString().c_str());
    ui->tokenhash->setText(data.at(1).toStdString().c_str());
    ui->tokennum->setText(data.at(2).toStdString().c_str());
    ui->tokenaccuracy->setText(data.at(3).toStdString().c_str());
    ui->tokenlabel->setText(data.at(4).toStdString().c_str());
    ui->tokentime->setText(data.at(5).toStdString().c_str());
    ui->tiplabel->setText("");

}

void ecoinconfirmcreatedialog::processSendCoinsReturn_(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
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
        ui->tiplabel->setText(tr("The total exceeds your balance when the 0.001IPC/KB transaction fee is included.").arg(msgArg));
        break;
    case WalletModel::DuplicateAddress:
        ui->tiplabel->setText(tr("Duplicate address found: addresses should only be used once each."));
        break;
    case WalletModel::TransactionCreationFailed:
        if(m_sendcoinerror!="")
            ui->tiplabel->setText(m_sendcoinerror.c_str());
        else
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
        else if("txn-already-in-mempool" == sendCoinsReturn.reasonCommitFailed)
        {
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: txn-already-in-mempool"));
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

void ecoinconfirmcreatedialog::on_pushButton_createeCoin_pressed()
{
    WalletModel::SendCoinsReturn sendStatus = walletModel->sendcreateeCoins();
    if (sendStatus.status == WalletModel::OK)
    {
        Q_EMIT CreateeCoinSuccess();
    }
    else
    {
        m_sendcoinerror =  walletModel->m_sendcoinerror;
    }
    processSendCoinsReturn_(sendStatus);
    // fNewRecipientAllowed = true;
}

void ecoinconfirmcreatedialog::on_pushButton_pressed()
{
    Q_EMIT backtoecoincreate();
}
