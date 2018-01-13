#include "ecoinsendaffrimdialog.h"
#include "forms/ui_ecoinsendaffrimdialog.h"
#include "addresstablemodel.h"
#include "optionsmodel.h"
#include "ipchainunits.h"
#include "wallet/coincontrol.h"
#include "guiutil.h"
#include <QRegExp>
#include "wallet/wallet.h"
#include <QMessageBox>
#include "log/log.h"
#include<math.h>
#include <stdlib.h>
#include <sstream>

ecoinsendaffrimdialog::ecoinsendaffrimdialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ecoinsendaffrimdialog)
{
    ui->setupUi(this);
}
void ecoinsendaffrimdialog::showEvent(QShowEvent * p)
{
    ui->addEdit->setText("");
    ui->numEdit->setText("");
    ui->tiplabel->setText("");
    ui->addEdit->setFocusPolicy(Qt::StrongFocus);
    ui->addEdit->setFocus(Qt::OtherFocusReason);
    ui->numEdit->clearFocus();
}
ecoinsendaffrimdialog::~ecoinsendaffrimdialog()
{
    delete ui;
}
void ecoinsendaffrimdialog::setModel(WalletModel *_model)
{
    this->model = _model;
}
void ecoinsendaffrimdialog::setMsg(QString name,QString num)
{

    m_name = name;
    m_num = num;
    std::string strname = name.toStdString();
    int acc = model->GetAccuracyBySymbol(strname);
    QString exp("10000000000|([0-9]{0,11}[\.][0-9]{0,");
    exp+=(QString::number(acc));
    exp = exp + ("})");
    QRegExp double_rx10000(exp);
    QValidator *validator = new QRegExpValidator(double_rx10000, ui->numEdit );
    ui->numEdit->setValidator( validator );
    ui->tokenlabel->setText(name);
    ui->addEdit->setText("");
    ui->numEdit->setText("");
    ui->tiplabel->setText("");
    ui->numEdit->clearFocus();

}

void ecoinsendaffrimdialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPalette pa;
    pa.setColor(QPalette::WindowText,Qt::black);

    LOG_WRITE(LOG_INFO,"processSendCoinsReturn","sendCoinsReturn.status",\
              QString::number(sendCoinsReturn.status).toStdString().c_str(),\
              "m_error",m_error.c_str());

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
        ui->tiplabel->setText(tr("The total exceeds your balance."));
        break;
    case WalletModel::DuplicateAddress:
        ui->tiplabel->setText(tr("Duplicate address found: addresses should only be used once each."));
        break;
    case WalletModel::TransactionCreationFailed:
        if(m_error == "The tokenvalue is too big,you have not enough tokencoins.")
            ui->tiplabel->setText(tr("The tokenvalue is too big,you have not enough tokencoins."));
        else if(m_error == "The Tokenvalue is too big,you have not enough Tokencoins.")
            ui->tiplabel->setText(tr("The Tokenvalue is too big,you have not enough Tokencoins."));
        else
            ui->tiplabel->setText(tr("Transaction creation failed!") + " " + m_error.c_str());
        break;
    case WalletModel::TransactionCommitFailed:
        if(m_error == "Token-reg-starttime-is-up-yet")
            ui->tiplabel->setText(tr("Token-reg-starttime-is-up-yet"));
        else
            ui->tiplabel->setText(tr("The transaction was rejected with the following reason: %1").arg(sendCoinsReturn.reasonCommitFailed));
        break;
    case WalletModel::AbsurdFee:
        ui->tiplabel->setText(tr("A fee higher"));
        break;
    case WalletModel::PaymentRequestExpired:
        ui->tiplabel->setText(tr("Payment request expired."));
        break;
    case WalletModel::OK:
    default:
        return;
    }
}


void ecoinsendaffrimdialog::on_sendecoinButton_pressed()
{
    m_error = "";
    if(!model || !model->getOptionsModel())
        return;
    if("" == ui->addEdit->text() ||
            "" == ui->numEdit->text())
    {
        ui->tiplabel->setText(tr("input info"));
        return;
    }
    QString num = ui->numEdit->text();
    QString pointword = ".";
    if(num.indexOf(pointword) == num.size()-1){
        ui->tiplabel->setText(tr("Please enter the correct amount"));
        return;
    }


    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient;
    bool valid = true;

    if (recipient.paymentRequest.IsInitialized())
        return ;


    QString strNumEdit = ui->numEdit->text();
    recipient.address = ui->addEdit->text();
    QString add = ui->addEdit->text();
    QString lab =this->model->getAddressTableModel()->labelForAddress(add);
    recipient.label =lab;
    recipient.amount = 0;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = false;
    recipients.append(recipient);
    fNewRecipientAllowed = false;

    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    if(!model->CheckPassword())
    {
        ui->tiplabel->setText(tr("password error"));
        return ;
    }
    WalletModel::UnlockContext ctx(model, true, true);

    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;

    CCoinControl ctrl;

    if (model->getOptionsModel()->getCoinControlFeatures())
    {
       // ctrl = *CoinControlDialog::coinControl;
    }
    else
    {
        ctrl.nConfirmTarget = 1;
    }

    double val=strNumEdit.toDouble();

    printf("%.8lf \n",val);

    std::string Txid = m_name.toStdString();
    int acc = model->GetAccuracyBySymbol(Txid);
    double temp10 = 10;
    int pointplace = strNumEdit.indexOf(".");
    printf("pointplace %d \n",pointplace);
    if(pointplace>0)
    {
        int afterpointnum =  strNumEdit.size() - pointplace-1;
        printf("%s \n",strNumEdit.toStdString().c_str());
        strNumEdit.replace(QString("."), QString(""));
        printf("%s \n",strNumEdit.toStdString().c_str());
        printf("intacc  %d  %d %d intacc \n",pointplace,afterpointnum,acc);
        acc =acc-afterpointnum;
    }
    while (acc>0) {
        strNumEdit+="0";
        acc--;
    }
    printf("%ld intedit \n",atol(strNumEdit.toStdString().c_str()));

    stringstream strValue;
    strValue << strNumEdit.toStdString().c_str();
    uint64_t intNumEdit;// =  strNumEdit.toInt64();//atol(strNumEdit.toStdString().c_str());//strNumEdit.toLonglong();//static_cast<long>(val);  //(uint64_t)val;
    strValue >> intNumEdit;
    std::cout<<"prepareeCoinsSendCreateTransaction "<<Txid<<" "<<intNumEdit<<" "<<val<<std::endl;
    LOG_WRITE(LOG_INFO,"SendCoinsAffrimWidget::strNumEdit",\
              strNumEdit.toStdString().c_str(),\
              QString::number(intNumEdit).toStdString().c_str());

    if(intNumEdit<=0){
        ui->tiplabel->setText(tr("num error"));
        return ;
    }

    prepareStatus = model->prepareeCoinsSendCreateTransaction(Txid,0,intNumEdit,currentTransaction,m_error, &ctrl);
    processSendCoinsReturn(prepareStatus,
                           BitcoinUnits::formatWithUnit(eTag, currentTransaction.getTransactionFee()));

    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
        return;
    }

    CAmount txFee = currentTransaction.getTransactionFee();
    CWalletTx *newTx = currentTransaction.getTransaction();

    // Format confirmation message
    QStringList formatted;
    Q_FOREACH(const SendCoinsRecipient &rcp, currentTransaction.getRecipients())
    {

        QString amount = "<b>" + BitcoinUnits::formatHtmlWithUnit(0, rcp.amount);

        amount.append("</b>");
        QString address = "<span style='font-family: monospace;'>" + rcp.address;
        address.append("</span>");
        QString recipientElement;
        QString q = QString("%1").arg(rcp.amount);
        if (!rcp.paymentRequest.IsInitialized())
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
        else
        {
            recipientElement = tr("%1 to %2").arg(amount, address);
        }

        formatted.append(recipientElement);
    }

    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if(txFee > 0)
    {
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(BitcoinUnits::formatHtmlWithUnit(0, txFee));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));
        questionString.append(" (" + QString::number((double)currentTransaction.getTransactionSize() / 1000) + " kB)");
    }
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

    WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
    m_error = model->m_sendcoinerror;
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        ui->addEdit->setText("");
        Q_EMIT SendeCoinSuccess(m_name);
    }
    fNewRecipientAllowed = true;

}
