#include "ecoincreatedialog.h"
#include "forms/ui_ecoincreatedialog.h"

#include "coincontroldialog.h"
#include "optionsmodel.h"
#include "bitcoinunits.h"
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
    pdataedit->setDate(QDate::currentDate());

}

void ecoincreatedialog::setModel(WalletModel *_model)
{

    this->model = _model;

    if(_model && _model->getOptionsModel())
    {
    }
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
        ui->tiplabel->setText(tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg));
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

        // ui->tiplabel->setText(tr("The transaction was rejected with the following reason: %1").arg(sendCoinsReturn.reasonCommitFailed));
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
        // included to prevent a compiler warning.
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

    Q_EMIT GetOldAddress();
    //20170908
    if(!model || !model->getOptionsModel())
        return;

    if("" == ui->coinname->text() ||
            "" == ui->coinname->text() ||
            "" == ui->picEdit->text() ||
            "" == ui->coinmd5->text() ||
            "" == ui->coinNum->text() ||
            "" ==ui->chooseaddBtn->text() ||
            "" == ui->coinlabel->text() ||
            "" == ui->TimeEdit->text())
    {
        ui->tiplabel->setText(tr("input info"));
        return;
    }
    QString num = ui->coinNum->text();
    if(0 == num.toDouble())
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

    QString add1 = ui->chooseaddBtn->text() ;//= ui->pushButton_address->text();//ui->addEdit->text();
    uint64_t amount1 = (uint64_t)ui->coinNum->text().toDouble(); ;
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

    //ctx(this, valid, was_locked);
    WalletModel::UnlockContext ctx(model, valid, was_locked);

    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        //return;
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
    QString timestr = ui->TimeEdit->text();
    int inttimr = strTimeToInt(timestr);
    if(inttimr<=0)
    {
        ui->tiplabel->setText(tr("time error"));
        return ;
    }
    int accuracy = ui->comboBox->currentIndex();
    QStringList data;
    QString coin_name = ui->coinname->text();  //"wangdddd";
    QString coin_md5 =ui->coinmd5->text();//"dhadsasaqazxswqwqazxswqazxswqazx";//
    QString coin_label =  ui->coinlabel->text();;//"hhhhh";
    data<<"\""+coin_name+"\"";
    data<<QString::number(0);//备用
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

        //   Q_EMIT CreateeCoinSuccess();
        fNewRecipientAllowed = true;

        //  printf("prepareeCoinsCreateTransaction fail %d",prepareStatus.status);
        return;//2017-09-01
    }
    CAmount txFee = currentTransaction.getTransactionFee();

    // Format confirmation message
    QStringList formatted;
    Q_FOREACH(const SendCoinsRecipient &rcp, currentTransaction.getRecipients())
    {

        QString amount = "<b>" + BitcoinUnits::formatHtmlWithUnit(0, rcp.amount);//20171001

        //   QString  amount = "<b>" + ui->coinEdit->text();
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
        //   questionString.append(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
        questionString.append(BitcoinUnits::formatHtmlWithUnit(0, txFee));
        //  questionString.append(QString::number(txFee)+QString("")+"IPC");
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
        // if(u != model->getOptionsModel()->getDisplayUnit())
        alternativeUnits.append(BitcoinUnits::formatHtmlWithUnit(0, totalAmount));
        //  alternativeUnits.append(BitcoinUnits::formatHtmlWithUnit(eTag, totalAmount));
        //   alternativeUnits.append(QString::number(totalAmount)+QString("")+"IPC");
    }

    //  questionString.append(tr("Total Amount %1")
    //      .arg(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount)));
    questionString.append(tr("Total Amount %1")
                          .arg(BitcoinUnits::formatHtmlWithUnit(0, totalAmount)));
    //  questionString.append(tr("Total Amount %1")
    //                       .arg(QString::number(totalAmount)+QString("")+"IPC"));


    questionString.append(QString("<span style='font-size:10pt;font-weight:normal;'><br />(=%2)</span>")
                          .arg(alternativeUnits.join(" " + tr("or") + "<br />")));

    WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);

    // process sendStatus and on error generate message shown to user



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
    // ui->label_md5->setText(tr("please wait"));

    boost::thread m_ptr(fileToMd5Thread,q2s(file_full),string("eCoinCreate"),(void*)this);
    //m_ptr.join();
    m_ptr.detach();
    // m_ptr->interrupt()

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
