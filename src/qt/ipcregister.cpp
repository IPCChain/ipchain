#include "ipcregister.h"
#include "ui_ipcregister.h"
#include <QStackedLayout>
#include <QFileInfo>
#include "titlestyle.h"
#include <boost/thread.hpp>
#include <QFileDialog>
#include <openssl/md5.h>
#include <iostream>
#include <cstdio>
#include <iomanip>
#include <stdlib.h>
#include <QMessageBox>
#include "walletmodel.h"
#include "ipcinspectiontag.h"
#include "md5thread.h"
#include <QTextEdit>
#include "log/log.h"

using namespace std;
using namespace boost;
IpcRegister::IpcRegister(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IpcRegister)
{
    ui->setupUi(this);
    connect( this,SIGNAL(showmd5(QString)),this,SLOT(showmd5Slot(QString)));
    ui->label_md5->setText(" ");
    connect(ui->textEdit_brief,SIGNAL(textChanged()),this,SLOT(slotTextChanged()));
    ui->pushButton_back->hide();
    ui->comboBox_type->addItems(QStringList()<<tr("patent")<<tr("trademark")<<tr("Electronic document")<<tr("Photo")<<tr("Journalism")<<tr("video")<<tr("audio frequency")<<tr("security code"));

    ui->label_ipcfree->hide();
    ui->lineEdit_ipcfee->hide();
    ui->label_tishi->hide();
    ui->label_errmsg->setText("");
}

IpcRegister::~IpcRegister()
{
    delete ui;
}

void IpcRegister::on_pushButton_back_pressed()
{
    Q_EMIT back();
}
void IpcRegister::fileToMd5(std::string strmd5)
{
    Q_EMIT showmd5(QString::fromStdString(strmd5));
}
void IpcRegister::showmd5Slot(QString strmd5)
{
    ui->label_md5->setText(strmd5);
}

void IpcRegister::on_pushButton_next_pressed()
{
    QString name = ui->lineEdit_name->text();
    int type = ui->comboBox_type->currentIndex();
    QString brief = ui->textEdit_brief->toPlainText();
    QString md5string = ui->label_md5->text();
    if(!walletModel)
    {
        LOG_WRITE(LOG_INFO,"IpcRegister::on_pushButton_next_pressed walletModel err");
        return;
    }
    if(name.size()==0)
    {
        ui->label_errmsg->setText(tr("please input ipcname"));
        return;
    }
    if(md5string.length()!=32)
    {
        ui->label_errmsg->setText(tr("please set Electronic tag"));
        return;
    }
    if(m_address.isEmpty())
    {
        ui->label_errmsg->setText(tr("InvalidAddress"));
        return;
    }
    std::string srdbrief = q2s(brief);
    if(srdbrief.length()>128*3||brief.size()>128)
    {
        std::cout<<"brief too large "<<srdbrief.length()<<srdbrief<<std::endl;
        ui->label_errmsg->setText(tr("brief too large"));
        return;
    }


    QString msg;
    int fee;
    if(walletModel&&walletModel->CreateIpcRegister(m_address,name,md5string,brief,type,msg,fee))
    {
        QStringList infos;
        infos<<name<<QString::number(type)<<md5string<<brief;
        Q_EMIT next(infos);
        ui->label_errmsg->setText(" ");
    }
    else
    {
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
        else
            ui->label_errmsg->setText(tr("Other")+tr(" ")+msg);
    }

}


void IpcRegister::on_pushButton_browse_pressed()
{
    QString file_full, file_name, file_path;
    QFileInfo fi;
    file_full = QFileDialog::getOpenFileName(this);
    if(file_full == "")
    {
        return;
    }
    fi = QFileInfo(file_full);
    file_name = fi.fileName();
    file_path = fi.absolutePath();
    ui->label_filename->setText(file_name);
    ui->label_md5->setText(tr("please wait"));

    boost::thread m_ptr(fileToMd5Thread,q2s(file_full),string("IpcRegister"),(void*)this);
    m_ptr.detach();

    LOG_WRITE(LOG_INFO,"on_pushButton_browse_pressed");


}
void IpcRegister::showEvent(QShowEvent *)
{

}

void IpcRegister::setaddress(QString address)
{
   m_address = address;
   ui->pushButton_selectaddress->setText(address);
}

void IpcRegister::on_pushButton_selectaddress_pressed()
{
    Q_EMIT gotoIpcSelectAddressPage();
}
void IpcRegister::slotTextChanged()
{
    QString textContent = ui->textEdit_brief->toPlainText();
        int length = textContent.count();
        int maxLength = 128; // 最大字符数
        if(length > maxLength)
        {
            int position = ui->textEdit_brief->textCursor().position();
            QTextCursor textCursor = ui->textEdit_brief->textCursor();
            textContent.remove(position - (length - maxLength), length - maxLength);
            ui->textEdit_brief->setText(textContent);
            textCursor.setPosition(position - (length - maxLength));
            ui->textEdit_brief->setTextCursor(textCursor);
        }
}
void IpcRegister::clearData()
{
    ui->lineEdit_name->setText("");
    ui->label_filename->setText(tr("None"));
    ui->label_md5->setText("");
    ui->textEdit_brief->setText("");
    ui->label_errmsg->setText("");
    ui->pushButton_selectaddress->setText(tr("select IP registration address"));
    m_address = "";
}
