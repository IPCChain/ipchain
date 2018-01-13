#include "exportdialog.h"
#include "ui_exportdialog.h"
#include <QFileDialog>
#include <QFileInfo>
#include "walletmodel.h"
#include "log/log.h"
#include "intro.h"
#include <QLocale>
exportdialog::exportdialog(QWidget *parent) :
    QWidget(parent),showother(true),walletModel(NULL),
    ui(new Ui::exportdialog)
{
    ui->setupUi(this);
    showother = false;
    showorhideother();
    ui->pushButton_other->hide();
    ui->label_setpassword->hide();
    ui->lineEdit_setpassword->hide();
    ui->label__confirmpassword->hide();
    ui->lineEdit_confirmpassword->hide();
    ui->label_error->setText("");
    ui->pushButton_confirm->hide();
    ui->textEdit_walletpath->setText(QString::fromLocal8Bit(Intro::m_datapath.c_str()));
}

exportdialog::~exportdialog()
{
    delete ui;
}

void exportdialog::on_pushButton_back_pressed()
{
    Q_EMIT back();
}

void exportdialog::on_pushButton_browse_pressed()
{
    QString file_full, file_name, file_path;
    QFileInfo fi;
    file_full = QFileDialog::getSaveFileName(this);
    if(file_full == "")
    {
        ui->label_filename->setText(tr("Unselected"));
        return;
    }
    m_file_full = file_full;
    fi = QFileInfo(file_full);
    file_name = fi.fileName();
    file_path = fi.absolutePath();
    ui->label_filename->setText( file_name);
    on_pushButton_confirm_pressed();
}

void exportdialog::on_pushButton_other_pressed()
{
    showorhideother();
}
string qstr2str(const QString qstr)
{
    QLocale locale;
    if( locale.language() == QLocale::Chinese )
    {
        QByteArray cdata = qstr.toLocal8Bit();
        return string(cdata);
    }else{
        return string(qstr.toStdString());
    }
}
void exportdialog::on_pushButton_confirm_pressed()
{
    if (m_file_full.isEmpty())
    {
        ui->label_error->setText(tr("Please select export address"));
        return;
    }
    if(!walletModel->CheckPassword())
    {
        ui->label_error->setText(tr("password error"));
        return;
    }
    WalletModel::UnlockContext ctx(walletModel, true, true);


    QString source = setDestSourcePath(QString::fromLocal8Bit(Intro::m_datapath.c_str()));
    LOG_WRITE(LOG_INFO,"WalletModel::on_pushButton_confirm_pressed"\
              ,source.toStdString().c_str(),m_file_full.toStdString().c_str());
    if(!walletModel->copyFileToPath(source,m_file_full,1)){
        LOG_WRITE(LOG_INFO,"WalletModel::ExportWalletToFile","export failed");
        ui->label_error->setText(tr("export failed"));
    }
    else {
        LOG_WRITE(LOG_INFO,"WalletModel::ExportWalletToFile","OK!");
        Q_EMIT confirm(1);
    }
}
void exportdialog::showorhideother()
{
    if(showother)
    {
        showother = 0;
        ui->lineEdit_confirmpassword->hide();
        ui->lineEdit_setpassword->hide();
        ui->label_setpassword->hide();
        ui->label__confirmpassword->hide();
    }else{
        showother = 1;
        ui->lineEdit_confirmpassword->show();
        ui->lineEdit_setpassword->show();
        ui->label_setpassword->show();
        ui->label__confirmpassword->show();
    }
}
void exportdialog::clearInfo()
{
    ui->label_error->setText("");
    m_file_full = "";
    ui->label_filename->setText(tr("Unselected"));
}
QString exportdialog::setDestSourcePath(QString path)
{
    QString m_desPath = path;
    m_desPath += "/wallet.dat";
    return m_desPath;
}
