#include "sendcoinsaffrimwidget.h"
#include "forms/ui_sendcoinsaffrimwidget.h"
#include <QMessageBox>
#include "bitcoinunits.h"
#include "coincontroldialog.h"//20170823
#include "optionsmodel.h"
#include "ui_interface.h"//20170823
#include "validation.h" // mempool and minRelayTxFee//20170823
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
        LOG_WRITE(LOG_INFO,"SendCoinsAffrimWidget::showpwd");
        //ui->line_2->show();
        ui->Psdframe->show();
        ui->InputPsd->show();
        ui->lineEditPsd->show();
    }
    else
    {
        LOG_WRITE(LOG_INFO,"SendCoinsAffrimWidget::hidepwd");
        //ui->line_2->hide();
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
        // connect(_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(updateSmartFeeLabel()));
    }
}

void SendCoinsAffrimWidget::setModel(WalletModel *_model)
{

    this->model = _model;

    if(_model && _model->getOptionsModel())
    {
    }
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
        ui->Tiplabel->setText(tr("The total exceeds your balance when the %1 transaction fee is included."));
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
        // msgParams.first = tr("Transaction creation failed!");
        // msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::TransactionCommitFailed:
        //ui->Tiplabel->setText(tr("The transaction was rejected with the following reason: %1");
        ui->Tiplabel->setText(tr("The transaction was rejected with the following reason:sendCoinReturn.reasonCommitFail"));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        // msgParams.first = tr("The transaction was rejected with the following reason: %1").arg(sendCoinsReturn.reasonCommitFailed);
        //  msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::AbsurdFee:
        ui->Tiplabel->setText(tr("A fee higher than %1 is considered an absurdly high fee."));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        // msgParams.first = tr("A fee higher than %1 is considered an absurdly high fee.").arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), maxTxFee));
        break;
    case WalletModel::PaymentRequestExpired:
        ui->Tiplabel->setText(tr("Payment request expired."));
        ui->Tiplabel->setVisible(true);
        ui->Tiplabel->setPalette(pa);
        // msgParams.first = tr("Payment request expired.");
        //  msgParams.second = CClientUIInterface::MSG_ERROR;
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
    model->getCurrentTime("before on_sendButton_pressed");


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


    LOG_WRITE(LOG_INFO,"SendCoinsAffrimWidget::on_sendButton_pressed",s.toStdString().c_str());
    recipient.address = ui->addEdit->text();
    recipient.label = eLabel;//"";//biaoqian
    recipient.amount = doubleamountuipc;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = false;
    recipients.append(recipient);


    //printf("",recipient.address);

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
        ctrl = *CoinControlDialog::coinControl;
    }
    else
    {
        ctrl.nConfirmTarget = 1;//2017-08-24 before =0
    }


    model->getCurrentTime("before prepareNormalTransaction");
    prepareStatus = model->prepareNormalTransaction(currentTransaction, &ctrl);
    model->getCurrentTime("end prepareNormalTransaction");
    processSendCoinsReturn(prepareStatus,
                           BitcoinUnits::formatWithUnit(eTag, currentTransaction.getTransactionFee()));

    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
        return;//2017-09-01
    }

    CAmount txFee = currentTransaction.getTransactionFee();
/*
    // Format confirmation message
    QStringList formatted;
    Q_FOREACH(const SendCoinsRecipient &rcp, currentTransaction.getRecipients())
    {

        QString amount = "<b>" + BitcoinUnits::formatHtmlWithUnit(0, rcp.amount);//20171001
        amount.append("</b>");
        // generate monospace address string
        QString address = "<span style='font-family: monospace;'>" + rcp.address;
        address.append("</span>");

        QString recipientElement;
        QString q = QString("%1").arg(rcp.amount);

        if (!rcp.paymentRequest.IsInitialized()) // normal payment
        {
            if(rcp.label.length() > 0) // label with address
            {
                recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.label));
                recipientElement.append(QString(" (%1)").arg(address));
            }
            else // just address
            {
                recipientElement = tr("%1 to %2").arg(amount, address);
            }
        }
        else if(!rcp.authenticatedMerchant.isEmpty()) // authenticated payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.authenticatedMerchant));
        }
        else // unauthenticated payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, address);
        }

        formatted.append(recipientElement);
    }

    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if(txFee > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(BitcoinUnits::formatHtmlWithUnit(0, txFee));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));

        // append transaction size
        questionString.append(" (" + QString::number((double)currentTransaction.getTransactionSize() / 1000) + " kB)");
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    Q_FOREACH(BitcoinUnits::Unit u, BitcoinUnits::availableUnits())
    {
        alternativeUnits.append(BitcoinUnits::formatHtmlWithUnit(0, totalAmount));
    }

    questionString.append(tr("Total Amount %1")
                          .arg(BitcoinUnits::formatHtmlWithUnit(0, totalAmount)));


    questionString.append(QString("<span style='font-size:10pt;font-weight:normal;'><br />(=%2)</span>")
                          .arg(alternativeUnits.join(" " + tr("or") + "<br />")));
*/
    // now send the prepared transaction
    model->getCurrentTime("before model->sendCoins");
    
    WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
model->getCurrentTime("end model->sendCoins");
    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        time_t startTime = time(NULL);

        ui->addEdit->setText("");
        ui->coinEdit->setText("");
        Q_EMIT goresultpage();

        time_t stopTime = time(NULL);
        long elapsed = stopTime - startTime;
        LOG_WRITE(LOG_INFO,"发送信号用了多久",(QString::number(elapsed)).toStdString().c_str());
    }
    else
    {

    }
    fNewRecipientAllowed = true;
    model->getCurrentTime("finish sendCoins");


}


void SendCoinsAffrimWidget::on_pushButton_pressed()
{
    Q_EMIT gobacktosendpage();
}
