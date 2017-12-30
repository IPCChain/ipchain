#include "setmessageauthentication.h"
#include "ui_setmessageauthentication.h"
#include "setmessageauthenticationtab.h"
#include "setmessagesignature.h"
#include "titlestyle.h"
#include "walletmodel.h"
SetMessageAuthentication::SetMessageAuthentication(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SetMessageAuthentication),walletModel(NULL)
{
    ui->setupUi(this);
    SetMessageAuthenticationTabPage = new SetMessageAuthenticationTab(this);
    SetMessageSignaturePage = new SetMessageSignature(this);
    ui->stackedWidget->addWidget(SetMessageAuthenticationTabPage);
    ui->stackedWidget->addWidget(SetMessageSignaturePage);
    ui->stackedWidget->setCurrentWidget(SetMessageSignaturePage);
   // setTitleStyle(ui->widget_title,ui->label_title,ui->pushButton_back);
    ui->label_title->setText(tr("message signature"));
}

SetMessageAuthentication::~SetMessageAuthentication()
{
    delete ui;
}

void SetMessageAuthentication::on_pushButton_back_pressed()
{
    Q_EMIT back();
}

void SetMessageAuthentication::on_pushButton__msgauthentication_pressed()
{
    ui->label_title->setText(tr("message authentication"));
    ui->stackedWidget->setCurrentWidget(SetMessageAuthenticationTabPage);
}

void SetMessageAuthentication::on_pushButton_msgsignature_pressed()
{
    ui->label_title->setText(tr("message signature"));
    ui->stackedWidget->setCurrentWidget(SetMessageSignaturePage);
}

void SetMessageAuthentication::setModel(WalletModel * model)
{
   walletModel = model;
   SetMessageAuthenticationTabPage->setModel(model);
   SetMessageSignaturePage->setModel(model);
}
