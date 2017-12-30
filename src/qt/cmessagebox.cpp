#include "cmessagebox.h"
#include "forms/ui_cmessagebox.h"

CMessageBox::CMessageBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CMessageBox)
{
    ui->setupUi(this);
    m_answertype = 0;
    this->setFixedSize(this->width(), this->height());
}

CMessageBox::~CMessageBox()
{
    delete ui;
}

void CMessageBox::on_pushButton_ok_pressed()
{
    m_answertype = 1;
    this->accept();
}

void CMessageBox::on_pushButton__cancle_pressed()
{
    m_answertype = 0;
    this->accept();
}
void CMessageBox::setMessage(QString msg)
{
    ui->label_message->setText(msg);
}
void CMessageBox::setMessage(int msg)
{
    if(msg == 1)
    ui->label_message->setText(tr("General punishment.Long term abnormal bookkeeping, please standardize bookkeeping behavior."));
    if(msg == 2)//pwd err 5
    ui->label_message->setText(tr("Password input error more than 5 times, please prohibit input within a day!"));
}
