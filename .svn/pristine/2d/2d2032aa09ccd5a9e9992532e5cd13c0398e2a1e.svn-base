#include "ipcinspectiontag.h"
#include "ui_ipcinspectiontag.h"
#include "walletmodel.h"
#include <QFileDialog>
#include <QFileInfo>
#include "md5thread.h"
#include "log/log.h"
using namespace std;
using namespace boost;
IpcInspectionTag::IpcInspectionTag(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IpcInspectionTag)
{
    ui->setupUi(this);
    connect( this,SIGNAL(showmd5(QString)),this,SLOT(showmd5Slot(QString)));
    ui->label_answer->setText(tr(""));
    ui->lineEdit_md5->setEnabled(false);
}

IpcInspectionTag::~IpcInspectionTag()
{
    delete ui;
}

void IpcInspectionTag::on_pushButton_back_pressed()
{
    Q_EMIT back();
}

void IpcInspectionTag::on_pushButton_select_pressed()
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
    ui->lineEdit_md5->setText(tr("please wait"));
    boost::thread m_ptr(fileToMd5Thread,file_full.toStdString(),std::string("IpcInspectionTag"),(void*)this);
    m_ptr.detach();
}

void IpcInspectionTag::fileToMd5(string strmd5)
{
    Q_EMIT showmd5(QString::fromStdString(strmd5));
}
void IpcInspectionTag::showmd5Slot(QString strmd5)
{
    ui->lineEdit_md5->setText(strmd5);
}

void IpcInspectionTag::on_pushButton_inspectiontag_pressed()
{
    QString textmd5 = ui->lineEdit_md5->text();
    QString textinput = ui->lineEdit_input->text();
    if(textmd5.isEmpty())
    {
        ui->label_answer->setText(tr("Please upload the electronic tag."));
    }
    else if(textinput.isEmpty())
    {
        ui->label_answer->setText(tr("Please input the electronic tag."));
    }
    else if(textmd5.size()!=32)
    {
        ui->label_answer->setText(tr("Please check the electronic tag."));
    }
    else if(textinput==textmd5)
    {
        ui->label_answer->setText(tr("same"));
    }else
    {
        ui->label_answer->setText(tr("not same"));
    }

}
void IpcInspectionTag::cleardata()
{
    ui->lineEdit_md5->setText("");
    ui->lineEdit_input->setText("");
    ui->label_filename->setText(tr("None"));
    ui->label_answer->setText(tr(""));
}
