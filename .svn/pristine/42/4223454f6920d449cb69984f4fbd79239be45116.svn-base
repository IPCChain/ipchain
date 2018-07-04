#include "unionaccounttrasign.h"
#include "ui_unionaccounttrasign.h"
#include "log/log.h"
#include <QDesktopWidget>
#include "util.h"
#include "ipchainunits.h"
#ifdef ENABLE_WALLET
#include "walletmodel.h"
#endif
unionaccounttrasign::unionaccounttrasign(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::unionaccounttrasign)
{
    ui->setupUi(this);
}

unionaccounttrasign::~unionaccounttrasign()
{
    delete ui;
}
void unionaccounttrasign::set0Text()
{
    ui->label_newsignaturestring->hide();
    ui->textEdit_newsignaturestring->hide();
    ui->textEdit->clear();
    ui->signBtn->show();
    ui->label_err->setText("");
    ui->label_tip->setText("");
    ui->label_add->setVisible(false);
    ui->label_num->setVisible(false);
    ui->m_address->setVisible(false);
    ui->m_num->setVisible(false);
    ui->m_address->setText("");
    ui->m_num->setText("");
}
void unionaccounttrasign::setModel(WalletModel *_model)
{
    this->model = _model;
}
void unionaccounttrasign::setMsgDlgPlace(CMessageBox* pmsgdlg)
{
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect applicationRect = desktopWidget->screenGeometry();
    QRect rectgeometry = QApplication::desktop()->geometry();
    QPoint xy = mapToGlobal(mapToParent(mapToParent(this->pos())));
    pmsgdlg->move(applicationRect.x()+1000/2-524/2+xy.x(),applicationRect.y()+660/2-287/2+xy.y());


}
void unionaccounttrasign::on_signBtn_pressed()
{
    ui->label_err->setText("");
    ui->label_tip->setText("");
    if(!getSignInfo()){
        return;
    }
    std::string strtx = ui->textEdit->document()->toPlainText().toStdString().c_str();
    if("" == strtx)
    {
        ui->label_err->setText(tr("input info"));
        return;
    }
    if(!model->CheckPassword())
    {
        ui->label_err->setText(tr("Password error."));
        LOG_WRITE(LOG_INFO,"on_signBtn_pressed Password error.");
        return;
    }
    WalletModel::UnlockContext ctx(model, true, true);
    std::string strFailReason;
    ScriptError serror;
    std::string strtxout;
    CMutableTransaction mergedTx;
    if(model->commitUnionaccounttra( strtx, strFailReason,serror,strtxout,mergedTx))
    {
        if(serror == SCRIPT_ERR_OK)
        {
            CMessageBox msg;
            setMsgDlgPlace(&msg);
            msg.setIsClose(false);
            msg.setMessage(5);
            msg.exec();
            if(0 == msg.m_issend)
            {
                LOG_WRITE(LOG_INFO,"return");
                //return;
            }
            else
            {
                if(model->commitUnionaccounttra_plus(mergedTx,strFailReason))
                {
                    ui->label_err->setText(tr("success"));
                    ui->label_tip->setText(tr("txid")+(" ")+ QString::fromStdString(strtxout));
                    LOG_WRITE(LOG_INFO,"SIGN sign commitUnionaccounttra OK ",strFailReason.c_str());
                    // Q_EMIT opensignsuccessPage();
                    ui->signBtn->hide();
                }
                else
                {
                    LOG_WRITE(LOG_INFO,"commitUnionaccounttra_plus FAILED",strFailReason.c_str());
                    if("bad-txns-inputs-spent"==strFailReason)
                        ui->label_err->setText(tr("sign error")+(" ")+tr("bad-txns-inputs-spent"));
                    else if("txn-already-in-mempool"==strFailReason)
                        ui->label_err->setText(tr("sign error")+(" ")+tr("txn-already-in-mempool"));
                    else if("txn-already-known"==strFailReason)
                        ui->label_err->setText(tr("sign error")+(" ")+tr("txn-already-known"));
                    else
                        ui->label_err->setText(tr("sign error")+(" ")+QString::fromStdString(strFailReason));
                }
            }
        }
        else
        {
            ui->label_err->setText(tr("sign successful send the newly signature to next friend."));
            ui->textEdit_newsignaturestring->setText(QString::fromStdString(strtxout));
            ui->label_newsignaturestring->show();
            ui->textEdit_newsignaturestring->show();
            ui->signBtn->hide();
            //ui->textEdit->setText(tr("sign string")+ QString::fromStdString(strtxout));
            LOG_WRITE(LOG_INFO,"SIGN commitUnionaccounttra error");
        }
    }
    else
    {
        ui->label_err->setText(tr("sign error"));
    }

}



void unionaccounttrasign::on_backbtn_pressed()
{
    Q_EMIT backtotraPage();
}

void unionaccounttrasign::on_btn_sign_pressed()
{
    getSignInfo();
}
bool unionaccounttrasign::getSignInfo()
{
    ui->label_err->setText(tr(""));
    std::string strFailReason = "";
    std::string address = "";
    CAmount money = 0;
    int haveSigned =0;
    std::string sourceaddress = "";
    std::string strtx = ui->textEdit->document()->toPlainText().toStdString().c_str();

    if(model->analyTransaction(strtx,strFailReason,address,money,haveSigned,sourceaddress))
    {
        ui->label_add->setVisible(true);
        ui->label_num->setVisible(true);
        ui->m_address->setVisible(true);
        ui->m_num->setVisible(true);
        ui->m_address->setText(QString::fromStdString(address));
        ui->m_num->setText(BitcoinUnits::formatWithUnit(0, money, false, BitcoinUnits::separatorAlways));
        LOG_WRITE(LOG_INFO,"getSignInfo sourceaddress:",sourceaddress.c_str(),\
                  " m_strSourceaddress:",m_strSourceaddress.c_str());
       if(sourceaddress!=""&&m_strSourceaddress!=""&&m_strSourceaddress!=sourceaddress){
           ui->label_err->setText(tr("Account mismatch."));
           return false;
       }
        return true;
    }
    else
    {
        if("DecodeHexTx failed" == strFailReason)
        {
        ui->label_err->setText(tr("DecodeHexTx failed"));
        }
        else if("vout.size too larger" == strFailReason)
        {
        ui->label_err->setText(tr("vout.size too larger"));
        }
        else if("vout.size failed" == strFailReason)
        {
        ui->label_err->setText(tr("vout.size failed"));
        }
        else
        {
        ui->label_err->setText(tr("resolve failed"));
        }
        return false;
    }
}

