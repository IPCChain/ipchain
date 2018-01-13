#include "setdialog.h"
#include "ui_setdialog.h"
#include "exportdialog.h"
#include "setrecovery.h"
#include "successfultrade.h"
#include "passwordsettingwidget.h"
#include "addbookwidget.h"
#include "setmessageauthenticationtab.h"
#include "setmessagesignature.h"

extern QString version;
setdialog::setdialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::setdialog)
{
    ui->setupUi(this);
    walletStackBranchPage = ui->stackedWidget;

    SuccessfulTradePage= new SuccessfulTrade(this);
    walletStackBranchPage->addWidget(SuccessfulTradePage);

    exportdialogPage = new exportdialog(this);
    walletStackBranchPage->addWidget(exportdialogPage);
    connect(exportdialogPage,SIGNAL(confirm(int)),this,SLOT(gotoSuccessfulTradePageSlot(int)));

    SetRecoveryPage = new SetRecovery(this);
    connect(SetRecoveryPage,SIGNAL(success(int)),this,SLOT(gotoSuccessfulTradePageSlot(int)));
    walletStackBranchPage->addWidget(SetRecoveryPage);
    SetMessageAuthenticationTabPage = new SetMessageAuthenticationTab(this);
    SetMessageSignaturePage = new SetMessageSignature(this);
    walletStackBranchPage->addWidget(SetMessageAuthenticationTabPage);
    walletStackBranchPage->addWidget(SetMessageSignaturePage);

    connect(this,SIGNAL(gotoMessageAuthenticationPage()),this,SLOT(gotoMessageAuthenticationPageSlot()));
    connect(this,SIGNAL(gotoSetRecoveryPage()),this,SLOT(gotoSetRecoveryPageSlot()));
    connect(this,SIGNAL(openexportdialog()),this,SLOT(gotoExportPageSlot()));
    setButtonPic(NULL);
    ui->label_6->setText(version);

    ui->pushButton_addressbook->setFocus();
    ui->pushButton_addressbook->setDefault(true);
    ui->pushButton_addressbook->setAutoDefault(true);
    ui->frame_3->hide();
}

setdialog::~setdialog()
{
    delete ui;
}
void setdialog::setModel(WalletModel *_model)
{
    this->model = _model;
    exportdialogPage->setWalletModel(model);
    SetRecoveryPage->setWalletModel(model);
    SetMessageAuthenticationTabPage->setModel(model);
    SetMessageSignaturePage->setModel(model);
}
void setdialog::on_pushButton_export_pressed()
{
    setButtonPic(ui->pushButton_pic_export);
    Q_EMIT openexportdialog();
}

void setdialog::on_pushButton_recovery_pressed()
{
    setButtonPic(ui->pushButton_pic_recovery);
    Q_EMIT gotoSetRecoveryPage();
}

void setdialog::on_pushButton_msgauthentication_pressed()
{
    setButtonPic(ui->pushButton_pic_auth);
    Q_EMIT gotoMessageAuthenticationPage();
}

void setdialog::on_pushButton_addressbook_pressed()
{
    setButtonPic(ui->pushButton_pic_addressbook);
    Q_EMIT openAddBookPagewidget(model->getAddressTableModel(),5);
}

void setdialog::on_pushButton_setpwd_pressed()
{
    setButtonPic(ui->pushButton_pic_setpwd);
    Q_EMIT openPasswordSetwidget(5);
}

void setdialog::setAddpageshow(AddBookWidget *Addbookpage)
{
    setButtonPic(ui->pushButton_pic_addressbook);
    walletStackBranchPage->addWidget(Addbookpage);
    walletStackBranchPage->setCurrentWidget(Addbookpage);
}
void setdialog::setPsdshow(PasswordSettingWidget  *PasswordSettingPage)
{
    walletStackBranchPage->addWidget(PasswordSettingPage);
    walletStackBranchPage->setCurrentWidget(PasswordSettingPage);
}
void setdialog::gotoExportPageSlot()
{
    exportdialogPage->clearInfo();
    walletStackBranchPage->setCurrentWidget(exportdialogPage);
}

void setdialog::gotoSetRecoveryPageSlot()
{
    walletStackBranchPage->setCurrentWidget(SetRecoveryPage);
}

void setdialog::gotoMessageAuthenticationPageSlot()
{
    gotoSetMessageAuthenticationTabPage();
}
void setdialog::setPsdSetSuccess()
{
    SuccessfulTradePage->setSuccessText(3);
    walletStackBranchPage->setCurrentWidget(SuccessfulTradePage);
}
void setdialog::gotoSuccessfulTradePageSlot(int type)
{
    SuccessfulTradePage->setSuccessText(type);
    walletStackBranchPage->setCurrentWidget(SuccessfulTradePage);

}
void setdialog::setButtonPic(QPushButton* btn)
{
    if(btn == ui->pushButton_pic_addressbook)
    {
        ui->pushButton_pic_addressbook->show();
    }
    else
    {
        ui->pushButton_pic_addressbook->hide();
    }
    if(btn == ui->pushButton_pic_export)
    {
        ui->pushButton_pic_export->show();
    }
    else
    {
        ui->pushButton_pic_export->hide();
    }
    if(btn == ui->pushButton_pic_recovery)
    {
        ui->pushButton_pic_recovery->show();
    }
    else
    {
        ui->pushButton_pic_recovery->hide();
    }
    if(btn == ui->pushButton_pic_setpwd)
    {
        ui->pushButton_pic_setpwd->show();
    }
    else
    {
        ui->pushButton_pic_setpwd->hide();
    }
    if(btn == ui->pushButton_pic_auth)
    {
        ui->pushButton_pic_auth->show();
    }
    else
    {
        ui->pushButton_pic_auth->hide();
    }
    if(btn == ui->pushButton_pic_verification)
    {
        ui->pushButton_pic_verification->show();
    }
    else
    {
        ui->pushButton_pic_verification->hide();
    }
}

void setdialog::on_pushButton_aboutwallet_pressed()
{

}

void setdialog::on_pushButton_verification_pressed()
{
    gotoSetMessageSignaturePage();
}
void setdialog::gotoSetMessageAuthenticationTabPage()
{
    setButtonPic(ui->pushButton_pic_auth);
    walletStackBranchPage->setCurrentWidget(SetMessageSignaturePage);
}

void setdialog::gotoSetMessageSignaturePage()
{
    setButtonPic(ui->pushButton_pic_verification);
    walletStackBranchPage->setCurrentWidget(SetMessageAuthenticationTabPage);
}
