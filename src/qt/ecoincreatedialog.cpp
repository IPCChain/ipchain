#include "ecoincreatedialog.h"
#include "forms/ui_ecoincreatedialog.h"
#include "optionsmodel.h"
#include "ipchainunits.h"
#include "wallet/coincontrol.h"
#include "guiutil.h"
#include <stdint.h>
#include <string>
#include <stdio.h>
#include <QFileDialog>
#include <QFileInfo>
#include "md5thread.h"
#include "log/log.h"
#include <QValidator>
#include "CDateEdit.h"
using namespace std;
using namespace boost;
ecoincreatedialog::ecoincreatedialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ecoincreatedialog)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);
    ui->scrollArea->setFrameShape(QFrame::NoFrame);
    connect( this,SIGNAL(showmd5(QString)),this,SLOT(showmd5Slot(QString)));
    ui->coinlabel->setMaxLength(16);
    ui->coinname->setPlaceholderText(tr("Tokens are not more than 8 characters (without IPC)"));
    ui->picEdit->setPlaceholderText(tr("Token pic"));
    ui->coinmd5->setPlaceholderText(tr("Token hash"));
    ui->coinNum->setPlaceholderText(tr("Token num"));
    ui->coinlabel->setPlaceholderText(tr("Please enter no more than 16 characters (without IPC)"));
    QRegExp regx("[a-zA-Z]+$");
    QValidator *validator = new QRegExpValidator(regx, ui->coinlabel );
    ui->coinname->setValidator( validator );

    QRegExp regxcoinNum("[0-9]+$");
    QValidator *validatorxcoinNum = new QRegExpValidator(regxcoinNum, ui->coinNum );
    ui->coinNum->setValidator( validatorxcoinNum );
    connect(ui->coinNum,SIGNAL(textChanged(const QString&)),this,SLOT(slotTextChanged(const QString&)));
    ui->chooseaddBtn->hide();
    ui->comboBox->addItems(QStringList()<<"0"<<"1"<<"2"<<"3"<<"4"<<"5"<<"6"<<"7"<<"8");
}

ecoincreatedialog::~ecoincreatedialog()
{
    delete ui;
}

void ecoincreatedialog::setinfoclean()
{
    ui->coinname->setText("");
    ui->picEdit->setText("");
    ui->coinmd5->setText("");
    ui->coinNum->setText("");
    ui->coinlabel->setText("");
    ui->TimeEdit->setText("");
    ui->tiplabel->setText("");
    ui->chooseaddBtn->setText(tr("chooseaddress"));
    CDateEdit* pdataedit = (CDateEdit*)ui->TimeEdit;
    if(model)
        pdataedit->setDate(model->getSySQDate());
}

void ecoincreatedialog::setModel(WalletModel *_model)
{
    this->model = _model;
}
void ecoincreatedialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    LOG_WRITE(LOG_INFO,"processSendCoinsReturn",m_sendcoinerror.c_str());
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
void  ecoincreatedialog::setAddress(QString address)
{
    ui->chooseaddBtn->setText(address);
    m_address = address;
}

void ecoincreatedialog::on_CreateeCoinButton_pressed()
{
    m_sendcoinerror = "";
    ui->tiplabel->setText(tr(""));
    QString coinNumText = ui->coinNum->text();
    coinNumText = coinNumText.replace(",","");
    Q_EMIT GetOldAddress();
    if(!model || !model->getOptionsModel())
        return;
    if("" == ui->coinname->text() ||
            "" == ui->coinname->text() ||
            "" == ui->picEdit->text() ||
            "" == ui->coinmd5->text() ||
            "" == coinNumText ||
            "" ==ui->chooseaddBtn->text() ||
            "" == ui->coinlabel->text() ||
            "" == ui->TimeEdit->text())
    {
        ui->tiplabel->setText(tr("input info"));
        return;
    }
    if(0 == coinNumText.toDouble())
    {
        ui->tiplabel->setText(tr("num error"));
        return;
    }
    QString stdlabel = ui->coinlabel->text();
    std::string tempstr =  stdlabel.toStdString();
    if(tempstr.length()>16)
    {
        ui->tiplabel->setText(tr("Token label size too large."));
        return;
    }

    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient;
    bool valid = true;

    if (recipient.paymentRequest.IsInitialized())
        return ;

    recipient.address = ui->chooseaddBtn->text();
    recipient.label = "";
    recipient.amount = 0;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = false;
    recipients.append(recipient);
    fNewRecipientAllowed = false;
    QString add1 = ui->chooseaddBtn->text() ;
    uint64_t amount1 = (uint64_t)coinNumText.toDouble();
    bool was_locked = model->getEncryptionStatus() == WalletModel::Locked;
    if(!model->CheckPassword())
    {
        ui->tiplabel->setText(tr("password error"));
        return ;
    }
    if(amount1<=0||amount1>10000000000)
    {
        ui->tiplabel->setText(tr("coinNum (0,10000000000]"));
        return ;
    }
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    valid = model->getEncryptionStatus() != WalletModel::Locked;
    WalletModel::UnlockContext ctx(model, valid, was_locked);
    if(!ctx.isValid())
    {
        fNewRecipientAllowed = true;
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
    QString timestr = ui->TimeEdit->text();
    int inttimr = strTimeToInt(timestr);
    if(inttimr<=0)
    {
        ui->tiplabel->setText(tr("time error"));
        return ;
    }
    int accuracy = ui->comboBox->currentIndex();
    QStringList data;
    QString coin_name = ui->coinname->text();
    QString coin_md5 =ui->coinmd5->text();
    QString coin_label =  ui->coinlabel->text();
    data<<"\""+coin_name+"\"";
    data<<QString::number(0);
    data<<"\""+coin_md5+"\"";
    data<<"\""+coin_label+"\"";
    data<<QString::number(inttimr);
    data<<QString::number(getpow10(amount1,accuracy));
    data<<QString::number(accuracy);

    prepareStatus = model->prepareeCoinsCreateTransaction(data,add1,currentTransaction, &ctrl);
    m_sendcoinerror =  model->m_sendcoinerror;
    processSendCoinsReturn(prepareStatus,
                           BitcoinUnits::formatWithUnit(eTag, currentTransaction.getTransactionFee()));

    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;

        return;
    }
    CAmount txFee = currentTransaction.getTransactionFee();

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
            if(rcp.label.length() > 0)
            {
                recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.label));
                recipientElement.append(QString(" (%1)").arg(address));
            }
            else
            {
                recipientElement = tr("%1 to %2").arg(amount, address);
            }
        }
        else if(!rcp.authenticatedMerchant.isEmpty())
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

    WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);

    if (sendStatus.status == WalletModel::OK)
    {
        Q_EMIT CreateeCoinSuccess();
    }
    else
    {
        m_sendcoinerror =  model->m_sendcoinerror;
    }
    processSendCoinsReturn(sendStatus);
    fNewRecipientAllowed = true;

}

void ecoincreatedialog::on_chooseaddBtn_pressed()
{
    Q_EMIT selectaddress(3);
}
void ecoincreatedialog::fileToMd5(std::string strmd5)
{
    Q_EMIT showmd5(QString::fromStdString(strmd5));
}
void ecoincreatedialog::showmd5Slot(QString strmd5)
{
    ui->coinmd5->setText(strmd5);
}

void ecoincreatedialog::on_openPicBtn_pressed()
{

    QString file_full, file_name, file_path;
    QFileInfo fi;
    file_full = QFileDialog::getOpenFileName(this,"","/","image Files(*.bmp *.jpg *.png *.jpeg)");
    if(file_full == "")
    {
        return;
    }
    fi = QFileInfo(file_full);
    file_name = fi.fileName();
    file_path = fi.absolutePath();
    ui->picEdit->setText(file_name);

    boost::thread m_ptr(fileToMd5Thread,file_full.toStdString(),string("eCoinCreate"),(void*)this);
    m_ptr.detach();

}
int ecoincreatedialog::strTimeToInt(QString time)
{
    QDate date = QDate::fromString(time,"yyyy-MM-dd");
    QDateTime datetime(date);
    int inttime =   datetime.toTime_t();
    return inttime;
}
uint64_t ecoincreatedialog::getpow10(uint64_t oldnum,int n)
{
    while (n--) {
        oldnum = oldnum*10;
    }
    return oldnum;
}
void ecoincreatedialog::slotTextChanged(const QString & text)
{
    QString textContent = text;
    QString numtexttemp = textContent;
    QString textContenttemp = numtexttemp.replace(",","");
    uint64_t maxLength = 10000000000;
    uint64_t amount1 = (uint64_t)textContenttemp.toDouble();
    if(amount1 > maxLength)
    {
        int position = ui->coinNum->cursorPosition();
        textContent.chop(1);
        ui->coinNum->setText(textContent);
        ui->coinNum->setCursorPosition(position-1);
    }
}
