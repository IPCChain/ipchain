#include "ipcconfirmauthorizationtransaction.h"
#include "ui_ipcconfirmauthorizationtransaction.h"
#include "log/log.h"
ipcconfirmauthorizationtransaction::ipcconfirmauthorizationtransaction(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ipcconfirmauthorizationtransaction)
{
    ui->setupUi(this);
}

ipcconfirmauthorizationtransaction::~ipcconfirmauthorizationtransaction()
{
    delete ui;
}

void ipcconfirmauthorizationtransaction::setModel(WalletModel *_model)
{
    this->walletModel = _model;
}
void ipcconfirmauthorizationtransaction::setInfo(QStringList data1)
{

    ui->label_errmsg->setText("");
    ui->ipcname->setText(data1.at(0).toStdString().c_str());
    ui->ipcaddress->setText(data1.at(1).toStdString().c_str());

    QString str =data1.at(2).toStdString().c_str();
    if("0" == str)
    {
     ui->authtype->setText(tr("Final authority (not allowed to re authorize)"));
    }
    else
    {
     ui->authtype->setText(tr("Allow re authorization"));
    }
    ui->authto->setText(tr("General authorization"));

    QString strbegin;
    QString begin_ =data1.at(4).toStdString().c_str();
    QDateTime datetime = QDateTime::fromTime_t(begin_.toInt());
    strbegin = datetime.toString("yyyy-MM-dd");


    QString strend;
    QString end_ =data1.at(5).toStdString().c_str();
    QDateTime datetime1 = QDateTime::fromTime_t(end_.toInt());
    strend = datetime1.toString("yyyy-MM-dd");

    ui->ipctime->setText(strbegin+"--"+strend);

}
void ipcconfirmauthorizationtransaction::on_pushButton_authIPC_pressed()
{
    QString msg;
    if (walletModel->sendauthorizationIpc(msg))
    {
       // Q_EMIT CreateeCoinSuccess();
         Q_EMIT gotoSuccessfultradePage(0);
    }
    else
    {
      // m_sendcoinerror =  walletModel->m_sendcoinerror;
        if(msg == "AmountExceedsBalance")
            ui->label_errmsg->setText(tr("AmountExceedsBalance"));
        else if(msg == "AmountWithFeeExceedsBalance")
            ui->label_errmsg->setText(tr("AmountWithFeeExceedsBalance"));
        else if(msg == "InvalidAddress")
            ui->label_errmsg->setText(tr("InvalidAddress"));
        else if(msg == "AmountExceedsBalance")
            ui->label_errmsg->setText(tr("AmountExceedsBalance"));
        else if(msg == "Password error.")
            ui->label_errmsg->setText(tr("Password error."));
        else if(msg == "bad-IPC-timecheck-error")
            ui->label_errmsg->setText(tr("bad-IPC-timecheck-error"));
        else if(msg == "bad-IPC-uniqueAuthor-timecheck-error")
            ui->label_errmsg->setText(tr("bad-IPC-uniqueAuthor-timecheck-error"));
        else if(msg == "bad-IPC-normalAuthor-timecheck-error")
            ui->label_errmsg->setText(tr("bad-IPC-normalAuthor-timecheck-error"));
        else if(msg == "bad-IPC-reAuthor-timecheck-error")
            ui->label_errmsg->setText(tr("bad-IPC-reAuthor-timecheck-error"));
        else if(msg == "IPC-owner-starttime-is-up-yet")
            ui->label_errmsg->setText(tr("IPC-owner-starttime-is-up-yet"));//+ tr(" ")+m_strStringTime);
        else if(msg == "you can't uniqueauthorize,you have an authorize yet")
           ui->label_errmsg->setText(tr("you can't uniqueauthorize,you have an authorize yet"));
        else if(msg == "You can't uniqueAuthorize ,you hava an Authorize yet!")
           ui->label_errmsg->setText(tr("you can't uniqueauthorize,you have an authorize yet"));
        else if(msg == "bad-txns-inputs-spent")
            ui->label_errmsg->setText(tr("bad-txns-inputs-spent"));
        else if(msg == "IPC-Author-starttime-is-up-yet")
            ui->label_errmsg->setText(tr("IPC-Author-starttime-is-up-yet"));
        else
        {
            ui->label_errmsg->setText(msg);
        }
    }
  //  processSendCoinsReturn_(sendStatus);
    // fNewRecipientAllowed = true;
}

void ipcconfirmauthorizationtransaction::on_pushButton_pressed()
{
     Q_EMIT backtoipcauthorization();
}
