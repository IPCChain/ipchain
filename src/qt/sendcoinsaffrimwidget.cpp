#include "sendcoinsaffrimwidget.h"
#include "forms/ui_sendcoinsaffrimwidget.h"
#include "ipchainunits.h"
#include "optionsmodel.h"
#include "ui_interface.h"
#include "validation.h"
#include "guiutil.h"
#include "wallet/coincontrol.h"
#include "log/log.h"
#include <QValidator>
#include <QRegExp>
#include <sys/time.h>
SendConfirmationDialog::SendConfirmationDialog(const QString &title, const QString &text, int _secDelay,
                                               QWidget *parent) :
    QMessageBox(QMessageBox::Question, title, text, QMessageBox::Yes | QMessageBox::Cancel, parent), secDelay(_secDelay)
{
    setDefaultButton(QMessageBox::Cancel);
    yesButton = button(QMessageBox::Yes);
    updateYesButton();
    connect(&countDownTimer, SIGNAL(timeout()), this, SLOT(countDown()));

}

int SendConfirmationDialog::exec()
{
    updateYesButton();
    countDownTimer.start(1000);
    return QMessageBox::exec();
}

void SendConfirmationDialog::countDown()
{
    secDelay--;
    updateYesButton();

    if(secDelay <= 0)
    {
        countDownTimer.stop();
    }
}

void SendConfirmationDialog::updateYesButton()
{
    if(secDelay > 0)
    {
        yesButton->setEnabled(false);
        yesButton->setText(tr("Yes") + " (" + QString::number(secDelay) + ")");
    }
    else
    {
        yesButton->setEnabled(true);
        yesButton->setText(tr("Yes"));
    }
}

SendCoinsAffrimWidget::SendCoinsAffrimWidget(bool isLocked,QWidget *parent) :
    QWidget(parent),
    model(0),
    ui(new Ui::SendCoinsAffrimWidget)
{
    ui->setupUi(this);
    ui->coinEdit->setReadOnly(true);
    m_isLocked =isLocked;
    if(isLocked)
    {
        ui->Psdframe->show();
        ui->InputPsd->show();
        ui->lineEditPsd->show();
    }
    else
    {
        ui->Psdframe->hide();
        ui->InputPsd->hide();
        ui->lineEditPsd->hide();
    }
    ui->widget->hide();
}

SendCoinsAffrimWidget::~SendCoinsAffrimWidget()
{
    delete ui;
}
void SendCoinsAffrimWidget::setMessage(QString a,QString b,QString label,int c){
    ui->addEdit->setText(a);
    eLabel = label;
    if(0 == c)
    {
        ui->coinEdit->setText(b+" "+"Ipc");
        eTag =0;
    }
    else if(1 == c)
    {
        ui->coinEdit->setText(b+" "+"mIpc");
        eTag =1;
    }
    else if(2 == c)
    {
        ui->coinEdit->setText(b+" "+"uIpc");
        eTag =2;
    }
    else
    {
        ui->coinEdit->setText(b+" "+"zhi");
        eTag =3;
    }
}

void SendCoinsAffrimWidget::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    if (_clientModel) {
    }
}

void SendCoinsAffrimWidget::setModel(WalletModel *_model)
{
    this->model = _model;
}

void SendCoinsAffrimWidget::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPalette pa;
    pa.setColor(QPalette::WindowText,Qt::black);
    switch(sendCoinsReturn.status)
    {
    case WalletModel::InvalidAddress:

        ui->Tiplabel->setText(tr("The recipient address is not valid. Please recheck."));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        break;
    case WalletModel::InvalidAmount:
        ui->Tiplabel->setText(tr("The amount to pay must be larger than 0."));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        break;
    case WalletModel::AmountExceedsBalance:
        ui->Tiplabel->setText(tr("The amount exceeds your balance."));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        ui->Tiplabel->setText(tr("The total exceeds your balance when the 0.001IPC/KB transaction fee is included."));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        break;
    case WalletModel::DuplicateAddress:
        ui->Tiplabel->setText(tr("Duplicate address found: addresses should only be used once each."));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        //  msgParams.first = tr("Duplicate address found: addresses should only be used once each.");
        break;
    case WalletModel::TransactionCreationFailed:
        ui->Tiplabel->setText(tr("Transaction creation failed!"));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        break;
    case WalletModel::TransactionCommitFailed:
        ui->Tiplabel->setText(tr("The transaction was rejected with the following reason:sendCoinReturn.reasonCommitFail"));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        break;
    case WalletModel::AbsurdFee:
        ui->Tiplabel->setText(tr("A fee higher than %1 is considered an absurdly high fee."));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        break;
    case WalletModel::PaymentRequestExpired:
        ui->Tiplabel->setText(tr("Payment request expired."));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        break;
        // included to prevent a compiler warning.
    case WalletModel::OK:
    default:
        return;
    }
}


void SendCoinsAffrimWidget::on_sendButton_pressed()
{

    if(!model || !model->getOptionsModel())
        return;

    QString a = ui->addEdit->text();
    QString b = ui->coinEdit->text();

    if(NULL == a || NULL == b)
    {
        ui->Tiplabel->setText("error");
        return;
    }
    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient;
    bool valid = true;
    if (recipient.paymentRequest.IsInitialized())
        return ;

    QString t = ui->coinEdit->text();
    int h= t.indexOf(" ");
    QString s = t.mid(0,h);
    double doubleamountuipc =  s.toDouble();
    if(3 == eTag)
    {
        doubleamountuipc=doubleamountuipc;
    }
    else if(2 == eTag)
    {
        doubleamountuipc=doubleamountuipc*100;
    }
    else if(1 == eTag)
    {
        doubleamountuipc=doubleamountuipc*100000;
    }
    else
    {
        doubleamountuipc=doubleamountuipc*100000000;
    }

    recipient.address = ui->addEdit->text();
    recipient.label = eLabel;//"";//biaoqian
    recipient.amount = doubleamountuipc;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = false;
    recipients.append(recipient);
    fNewRecipientAllowed = false;
    bool passwordwrite = model->CheckPassword();
    if(!passwordwrite){
        ui->Tiplabel->setText(tr("The passphrase entered for the wallet decryption was incorrect."));
        fNewRecipientAllowed = true;
    }
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    valid = model->getEncryptionStatus() != WalletModel::Locked;

    WalletModel::UnlockContext ctx(model, valid, true);

    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return;
    }

    // prepare transaction for getting txFee earlier
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;

    // Always use a CCoinControl instance, use the CoinControlDialog instance if CoinControl has been enabled

    CCoinControl ctrl;
    if (model->getOptionsModel()->getCoinControlFeatures())
    {
       // ctrl = *CoinControlDialog::coinControl;
    }
    else
    {
        ctrl.nConfirmTarget = 1;
    }
    prepareStatus = model->prepareNormalTransaction(currentTransaction, &ctrl);
    processSendCoinsReturn(prepareStatus,
                           BitcoinUnits::formatWithUnit(eTag, currentTransaction.getTransactionFee()));

    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
        return;
    }

    CAmount txFee = currentTransaction.getTransactionFee();

    WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        time_t startTime = time(NULL);

        ui->addEdit->setText("");
        ui->coinEdit->setText("");
        Q_EMIT goresultpage();

        time_t stopTime = time(NULL);
        long elapsed = stopTime - startTime;
    }
    fNewRecipientAllowed = true;
}


void SendCoinsAffrimWidget::on_pushButton_pressed()
{
    Q_EMIT gobacktosendpage();
}
