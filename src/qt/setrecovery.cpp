#include "setrecovery.h"
#include "ui_setrecovery.h"
#include <QFileInfo>
#include <QFileDialog>
#include "walletmodel.h"
#include "log/log.h"
#include "intro.h"
#include <QMessageBox>
QString g_CreateWalletFromPath;
SetRecovery::SetRecovery(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SetRecovery),walletModel(NULL)
{
    ui->setupUi(this);
    ui->lineEdit_pwd->hide();
    ui->label_pwd->hide();
    ui->frame->hide();
    ui->label_error->setText(tr(""));
}

SetRecovery::~SetRecovery()
{
    delete ui;
}

void SetRecovery::on_pushButton_back_pressed()
{
    Q_EMIT back();
}

void SetRecovery::on_pushButton_select_pressed()
{
    ui->label_error->setText(tr(""));


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
    m_file_full = file_full;
    ui->label_filename->setText(file_name);

}

void SetRecovery::on_pushButton_import_pressed()
{
    if(m_file_full.isEmpty())return;
    std::cout<<m_file_full.toStdString()<<" "<<m_desPath.toStdString()<<std::endl;
    if(!copyFileToPath(m_file_full,m_desPath,1))
    {
        QLocale locale;
        if( locale.language() != QLocale::Chinese )
        {
            QMessageBox::information(NULL, "IPC", "recovery failed.", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        }
        else{
            QMessageBox::information(NULL, "IPC", "恢复失败.", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        }
    }else{

        g_CreateWalletFromPath = m_file_full;

        Q_EMIT next();
    }


}
void SetRecovery::setWalletModel(WalletModel * model)
{
    walletModel = model;

}

bool SetRecovery::copyFileToPath(QString sourceDir ,QString toDir, bool coverFileIfExist)
{
    toDir.replace("\\","/");
    if (sourceDir == toDir){
        printf("sourceDir == toDir\n");
        return true;
    }
    if (!QFile::exists(sourceDir)){
        printf("exists\n");
        return false;
    }
    QDir *createfile     = new QDir;
    bool exist = createfile->exists(toDir);
    if (exist){
        if(coverFileIfExist){
            createfile->remove(toDir);
        }
    }
    printf("%s %s \n",sourceDir.toStdString().c_str(),toDir.toStdString().c_str());
    if(!QFile::copy(sourceDir, toDir))
    {
        printf("false\n");
        return false;
    }
    return true;
}

bool SetRecovery::isDirExist(QString fullPath)
{
    QDir dir(fullPath);
    if(dir.exists())
    {
        return true;
    }
    else
    {
        bool ok = dir.mkdir(fullPath);
        return ok;
    }
}

void SetRecovery::setDestSourcePath(QString path)
{
    m_desPath = path;
    isDirExist(m_desPath);
    if("IPChain" != m_desPath.right(7))
        m_desPath += "/IPChain";
    isDirExist(m_desPath);

    if(Intro::m_clienttype == "test"){
        m_desPath+="/testnet3";
        isDirExist(m_desPath);
    }
    m_desPath += "/wallet.dat";

}
