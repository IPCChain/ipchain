#include "successfultrade.h"
#include "forms/ui_successfultrade.h"
#include "log/log.h"
#include <QFileDialog>
#include <QTextStream>
SuccessfulTrade::SuccessfulTrade(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SuccessfulTrade)
{
    ui->setupUi(this);
    ui->pushButton_ok->hide();
}

SuccessfulTrade::~SuccessfulTrade()
{
    delete ui;
}

void SuccessfulTrade::on_pushButton_ok_pressed()
{
    Q_EMIT back();
}
void SuccessfulTrade::setsendSuccessText_(QString s1,QString s2)
{
    ui->label_successtext->setVisible(false);

    if("create successed"==s1)
    {
        ui->label_tip->setText(tr("created successfully,sending signature to friend, waiting for friend to sign signature,successful signature,successful transaction."));
        ui->textEdit->setText(s2);
        ui->btn_import->setVisible(true);
    }
    else
    {
        ui->label_tip->setText(tr("successful signature,successful transaction."));
        ui->textEdit->setText("");
        ui->btn_import->setVisible(false);
    }

    LOG_WRITE(LOG_INFO,"send success",s1.toStdString().c_str(),s2.toStdString().c_str());
}
void SuccessfulTrade::setSuccessText_(QString s1,QString s2)
{
    ui->label_successtext->setVisible(false);
    ui->label_tip->setText(tr("The joint account is created successfully, sending an invitation code to a friend and inviting a friend to the account.For your asset safety, please backup your wallet again!"));
    ui->textEdit->setText(s1);
    ui->btn_import->setVisible(false);

    LOG_WRITE(LOG_INFO,"JOIN KEY",s1.toStdString().c_str());
}
void SuccessfulTrade::setSuccessText(int type)
{
    ui->btn_import->setVisible(false);
    if(type==successtrade)
    {
        ui->label_tip->setVisible(true);
        ui->label_successtext->setText(tr("successful trade"));
    }
    else if(type==successexport)
    {
        ui->label_tip->setVisible(false);
        ui->label_successtext->setText(tr("successful export"));
    }
    else if(type == successrevovery)
    {
        ui->label_tip->setVisible(false);
        ui->label_successtext->setText(tr("successful revovery"));
    }
    else if(type ==successpsdset)
    {
        ui->label_tip->setVisible(false);
        ui->label_successtext->setText(tr("successful psdset"));
    }
    else
    {
        ui->label_tip->setVisible(false);
        ui->label_successtext->setText("");
    }

    ui->textEdit->setText("");

}



void SuccessfulTrade::on_btn_import_pressed()
{

    QString filename = QFileDialog::getSaveFileName(this,tr("Save Text"),"",tr("Text files(*.txt)"));
    if(filename.isEmpty())
    {
        LOG_WRITE(LOG_INFO,"file is empty");
        return;
    }
    else
    {

        if(filename.right(4)!=".txt"){
            filename=filename+".txt";
        }
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            LOG_WRITE(LOG_INFO,"open fail");
            return;
        }
        else
        {
            QString str1, str2, qstr_similarity, sumqstr;
            for(int i = 0; i < 1; ++i)
            {
                str1 = QString::fromStdString(ui->textEdit->document()->toPlainText().toStdString());
                QLocale locale;
                QString timestr;
                if( locale.language() == QLocale::Chinese )
                {
                    sumqstr = str1 ;
                }else{
                   sumqstr =  str1 ;
                }

            }
            QTextStream in(&file);
            in << sumqstr;
            file.close();

        }

    }

}
