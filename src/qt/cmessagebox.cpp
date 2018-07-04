#include "cmessagebox.h"
#include "forms/ui_cmessagebox.h"
#include "dpoc/DpocInfo.h"
#include "log/stateinfo.h"
#include "walletmodel.h"
#include "log/log.h"
#include "init.h"
#include <QTimer>
CMessageBox::CMessageBox(QWidget *parent) :
    QDialog(parent),m_isShutDown(false),
    ui(new Ui::CMessageBox)
{
    ui->setupUi(this);

    Qt::WindowFlags flags=Qt::Dialog;
    flags |=Qt::WindowCloseButtonHint;
    this->setWindowFlags(flags);



    m_answertype = 0;
    m_issend = 0;
    m_msg = 0;
    this->setFixedSize(524, 287);
    this->resize(524, 287);
    if( true == getIsClose())
    {
        try{
            if(!CDpocInfo::Instance().IsHasLocalAccount())
            {
                ui->label_message->setText(tr("Confirm exit?"));
            }
            else
            {
                ui->label_message->setText(tr("Is Bookkeeping,Confirm exit?"));
                QPalette pa;
                pa.setColor(QPalette::WindowText,Qt::red);
                ui->label_message->setPalette(pa);
                ui->pushButton_ok->setText(tr("forced return"));
                ui->pushButton__cancle->setText(tr("wait for exit"));
            }



        }
        catch(...){
        }
    }
}

CMessageBox::~CMessageBox()
{
    delete ui;
}
void CMessageBox::setCacelVisible(bool isVisible)
{
    if(!isVisible)
    {
        ui->pushButton__cancle->setVisible(false);

        ui->horizontalLayout->removeWidget(ui->pushButton__cancle);
        ui->pushButton_ok->setGeometry(ui->widget->width()/2,ui->pushButton_ok->y(),205,44);
    }

}
void CMessageBox::on_pushButton_ok_pressed()
{
    m_answertype = 1;
    this->accept();
    if(4 == m_msg){
        StartShutdown();
    }
    else if(5 == m_msg){
       m_issend = 1;
    }
}
void CMessageBox::setWalletModel(WalletModel *model_)
{
    this->model=model_;
}
void CMessageBox::on_pushButton__cancle_pressed()
{
    if( true == getIsClose())
    {
        try{
            if(CDpocInfo::Instance().IsHasLocalAccount())
            {
                /*
                pollTimer = new QTimer(this);
                connect(pollTimer, SIGNAL(timeout()), this, SLOT(updateTimer()));
                pollTimer->start(250);
                */

                model->startTimerControl();



                // killTimer(m_nTimerID);
                // m_nTimerID = this->startTimer(100);

                // timerEvent(NULL);
            }
        }
        catch(...){
        }
    }
    m_answertype = 0;
    m_issend = 0;
    this->accept();
}


void CMessageBox::updateTimer()
{




}


void CMessageBox::setMessage(QString msg)
{
    ui->label_message->setText(msg);
}
void CMessageBox::setMessage(int msg)
{
    if(msg == 1)
        ui->label_message->setText(tr("General punishment.Long term abnormal bookkeeping, please standardize bookkeeping behavior."));
    else if(msg == 2)
        ui->label_message->setText(tr("Password input error more than 5 times, please prohibit input within a day!"));
    else if(msg == 3)
        ui->label_message->setText(tr("Date Updating,please waiting..."));
    else if(msg == 4)
    {
        m_msg = 4;
        ui->label_message->setText(tr("You have quit the bookkeeping ,do you close your wallet"));
    }
    else if(msg == 5)
    {
        m_msg = 5;
        ui->pushButton_ok->setText(tr("OK"));
        ui->pushButton__cancle->setText(tr("cancle"));
        ui->label_message->setText(tr("Reach the confirmation number, whether the transaction is sent or not"));
    }
    else if(msg == 6)
    {
        m_msg = 6;
        ui->label_message->setText(tr("The data is being loaded. Please wait a moment..."));
        ui->pushButton_ok->hide();
        ui->pushButton__cancle->hide();
    }
    else if(msg == 7)
    {
        m_msg = 7;
        ui->label_message->setText(tr("For your asset safety, please backup your wallet again!"));
        ui->pushButton_ok->show();
        ui->pushButton__cancle->show();
    }

}
