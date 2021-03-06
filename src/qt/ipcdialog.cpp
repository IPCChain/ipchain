#include "ipcdialog.h"
#include "forms/ui_ipcdialog.h"
#include <QHBoxLayout>
#include "upgradewidget.h"
#include <QLabel>
#include "walletmodel.h"
#include "ipcdetails.h"
#include "ipcregister.h"
#include "ipcregisterinformation.h"
#include "successfultrade.h"
#include "ipcinspectiontag.h"
#include "ipctransfertransaction.h"
#include "ipcauthorizationtransaction.h"
#include "ipcconfirmauthorizationtransaction.h"
#include "ipcconfirmtransfertransaction.h"
#include "ipcselectaddress.h"
#include <QPalette>
#include <QFont>
#include <QMessageBox>
#include <QSpacerItem>
#include <QDateTime>
#include <iostream>
#include <log/log.h>
ipcdialog* g_pipcdialog = NULL;
bool ipcdialog::m_bNeedUpdateLater = false;
ipcdialog::ipcdialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ipcdialog),
    walletModel(NULL),pOldSelectIpcButtons(NULL)
{
    ui->setupUi(this);
    walletStackBranchPage = ui->stackedWidget;

    ipcdetailsPage = new IpcDetails(this);
    walletStackBranchPage->addWidget(ipcdetailsPage);
    connect(ipcdetailsPage,SIGNAL(gotoIpcTransferTransactionPage(QString)),this,SLOT(gotoIpcTransferTransactionPage(QString)));
    connect(ipcdetailsPage,SIGNAL(gotoIpcAuthorizationTransactionPage(QString)),this,SLOT(gotoIpcAuthorizationTransactionPage(QString)));
    IpcRegisterPage = new IpcRegister(this);
    walletStackBranchPage->addWidget(IpcRegisterPage);
    connect(IpcRegisterPage,SIGNAL(next(QStringList)),this,SLOT(gotoIpcRegisterInformationPage(QStringList)));
    connect(IpcRegisterPage,SIGNAL(gotoIpcSelectAddressPage()),this,SLOT(gotoIpcSelectAddressPage()));
    IpcRegisterInformationPage= new IpcRegisterInformation(this);
    walletStackBranchPage->addWidget(IpcRegisterInformationPage);
    connect(IpcRegisterInformationPage,SIGNAL(back()),this,SLOT(gotoBackIpcRegisterPage()));
    connect(IpcRegisterInformationPage,SIGNAL(next(int)),this,SLOT(gotoSuccessfulTradePage(int)));
    SuccessfulTradePage= new SuccessfulTrade(this);
    walletStackBranchPage->addWidget(SuccessfulTradePage);
    IpcInspectionTagPage= new IpcInspectionTag(this);
    walletStackBranchPage->addWidget(IpcInspectionTagPage);
    IpcTransferTransactionPage= new IpcTransferTransaction(this);
    connect(IpcTransferTransactionPage,SIGNAL(back()),this,SLOT(gotoIpcdetailsPage()));
    connect(IpcTransferTransactionPage,SIGNAL(gotoSuccessfultradePage(int)),this,SLOT(gotoSuccessfulTradePage(int)));


    walletStackBranchPage->addWidget(IpcTransferTransactionPage);
    IpcAuthorizationTransactionPage= new IpcAuthorizationTransaction(this);
    connect(IpcAuthorizationTransactionPage,SIGNAL(back()),this,SLOT(gotoIpcdetailsPage()));
    connect(IpcAuthorizationTransactionPage,SIGNAL(gotoSuccessfultradePage(int)),this,SLOT(gotoSuccessfulTradePage(int)));
    walletStackBranchPage->addWidget(IpcAuthorizationTransactionPage);
    ipcSelectAddressPage = new ipcSelectAddress(this);
    connect(ipcSelectAddressPage,SIGNAL(back(QString)),this,SLOT(gotoIpcRegisterPage(QString)));
    walletStackBranchPage->addWidget(ipcSelectAddressPage);


    // 20180201
    connect(IpcAuthorizationTransactionPage,SIGNAL(gotoconfrimauthorizationIpc(QStringList)),this,SLOT(gotoconfrimauthorizationIpcPage(QStringList)));

    ipcconfirmauthorizationtransactionpage = new ipcconfirmauthorizationtransaction(this);
    walletStackBranchPage->addWidget(ipcconfirmauthorizationtransactionpage);


    connect(IpcTransferTransactionPage,SIGNAL(gotoconfrimtransferIpc(QString,QString)),this,SLOT(gotoconfrimtransferIpcPage(QString,QString)));

    ipcconfirmtransfertransactionpage = new ipcconfirmtransfertransaction(this);
    walletStackBranchPage->addWidget(ipcconfirmtransfertransactionpage);




    connect(ipcconfirmtransfertransactionpage,SIGNAL(gotoSuccessfultradePage(int)),this,SLOT(gotoSuccessfulTradePage(int)));

    connect(ipcconfirmauthorizationtransactionpage,SIGNAL(gotoSuccessfultradePage(int)),this,SLOT(gotoSuccessfulTradePage(int)));



    connect(ipcconfirmtransfertransactionpage,SIGNAL(backtoipctransfertransaction()),this,SLOT(backtoipctransfertransactionPage()));

    connect(ipcconfirmauthorizationtransactionpage,SIGNAL(backtoipcauthorization()),this,SLOT(backtoipcauthorizationPage()));
    //20180201

    walletStackBranchPage->setCurrentWidget(IpcRegisterPage);

    connect(this,SIGNAL(openInspectionTagPage()),this,SLOT(gotoIpcInspectionTagPage()));
    connect(this,SIGNAL(openRegisterPage()),this,SLOT(gotoIpcRegisterPage()));
    g_pipcdialog = this;
}

ipcdialog::~ipcdialog()
{
    delete ui;
}

void ipcdialog::backtoipcauthorizationPage()
{
 walletStackBranchPage->setCurrentWidget(IpcAuthorizationTransactionPage);

}
void ipcdialog::backtoipctransfertransactionPage()
{
 walletStackBranchPage->setCurrentWidget(IpcTransferTransactionPage);
}
void ipcdialog::setModel(WalletModel * model)
{
    walletModel = model;
    ipcdetailsPage->setWalletModel(walletModel);
    ipcSelectAddressPage->setWalletModel(walletModel);
    IpcRegisterPage->setModel(walletModel);
    IpcAuthorizationTransactionPage->setModel(walletModel);
    IpcTransferTransactionPage->setModel(walletModel);
    IpcRegisterInformationPage->setModel(walletModel);
    ipcconfirmauthorizationtransactionpage->setModel(walletModel);
    ipcconfirmtransfertransactionpage->setModel(walletModel);


    connect(walletModel,SIGNAL(updataLoadingFinished()),this,SLOT(updateIpcList()));
    connect(walletModel,SIGNAL(updataLater()),this,SLOT(updataLater()));

    updateIpcList();
}
int ipcdialog::addnewoneipcmessage(int pictype,QString name,QString time,int picprogess,int dlgnum,QString txid)
{
    upgradewidget * pnewone = new upgradewidget();
    pnewone->m_dialognum = dlgnum;
    pnewone->m_txid = txid;
    pnewone->setMaximumHeight(70);
    pnewone->setMinimumHeight(70);
    pnewone->setAutoFillBackground(true);
    QPalette palette(pnewone->palette());
    palette.setColor(QPalette::Background, QColor(79,194,186));
    pnewone->setPalette(palette);

    QHBoxLayout * phboxlayout = new QHBoxLayout();
    phboxlayout->setContentsMargins(0,0,0,0);
    pnewone->setLayout(phboxlayout);

    QLabel* pictypelabel = new QLabel();
    pictypelabel->setAutoFillBackground(true);
    phboxlayout->addWidget(pictypelabel);

    QVBoxLayout * pvboxlayout = new QVBoxLayout();
    phboxlayout->addLayout(pvboxlayout);

    std::string tempname = name.toStdString();
    if(name.size()>12 && tempname.length()>12*3){
        QString nametemp = name.mid(0,12);
        nametemp+="...";
        name = nametemp;
    }
    QLabel* namelabel = new QLabel(name);
    namelabel->setAttribute(Qt::WA_TranslucentBackground);
    QFont ftname;
    ftname.setPointSize(16);
    namelabel->setFont(ftname);
    QPalette paname;
    paname.setColor(QPalette::WindowText,Qt::white);
    namelabel->setPalette(paname);
    namelabel->setStyleSheet("font-size : 16px;color: rgb(255,255,255);");
    pvboxlayout->addWidget(namelabel);

    QLabel* timelabel = new QLabel(time);
    timelabel->setAttribute(Qt::WA_TranslucentBackground);
    QFont fttime;
    fttime.setPointSize(12);
    timelabel->setFont(fttime);
    QPalette patime;
    patime.setColor(QPalette::WindowText,Qt::white);
    timelabel->setPalette(patime);
    timelabel->setStyleSheet("font-size : 12px;color: rgb(255,255,255);");
    pvboxlayout->addWidget(timelabel);
    QSpacerItem* horizontalSpacer = new QSpacerItem(40,20,QSizePolicy::Expanding);
    phboxlayout->addSpacerItem(horizontalSpacer);
    QLabel* picendlabel = new QLabel();
    pnewone->setipcselectlabel(picendlabel);
    picendlabel->setPixmap(QPixmap(":/res/png/triangle.png"));
    phboxlayout->addWidget(picendlabel);
    picendlabel->hide();
    connect(pnewone,SIGNAL(dlgnum(int)),this,SLOT(gotoIpcdetailsPage(int)));
    connect(pnewone,SIGNAL(selectlabel(QLabel*)),this,SLOT(changelabel(QLabel*)));

    QString picpath;
    if(pictype == 2)
    {
        picpath = ":/res/png/ipctype/file.png";
    }
    else if(pictype == 6)
    {
        picpath = ":/res/png/ipctype/music.png";
    }
    else if(pictype == 4)
    {
        picpath = ":/res/png/ipctype/news.png";
    }
    else if(pictype == 0)
    {
        picpath = ":/res/png/ipctype/patent.png";
    }
    else if(pictype == 3)
    {
        picpath = ":/res/png/ipctype/picture.png";
    }
    else if(pictype == 7)
    {
        picpath = ":/res/png/ipctype/securitycode.png";
    }
    else if(pictype == 1)
    {
        picpath = ":/res/png/ipctype/trademark.png";
    }
    else if(pictype == 5)
    {
        picpath = ":/res/png/ipctype/video.png";
    }
    else if(pictype == 8)
    {
        LOG_WRITE(LOG_INFO,"pictype == 8");
        picpath = ":/res/png/ipctype/gameprops.png";
    }
    pictypelabel->setFixedSize(40,40);
    pictypelabel->setPixmap(QPixmap(picpath));

    pvboxlayoutall->addWidget(pnewone);
    return 1;

}

void ipcdialog::on_pushButton_register_pressed()
{
    Q_EMIT openRegisterPage();
}

void ipcdialog::on_pushButton_label_pressed()
{
    Q_EMIT openInspectionTagPage();
}

void ipcdialog::showEvent(QShowEvent *)
{

}
void ipcdialog::updateIpcList()
{
    QDateTime time = QDateTime::currentDateTime();
    int timeT = time.toTime_t();
    static int timeOld = 0;
    if(timeOld == timeT)
    {
        timeOld = timeT;
        return;
    }
     LOG_WRITE(LOG_INFO,"ipcdialog updateIpcList");
    timeOld = timeT;

    if(walletModel)
    {
        walletModel->getIpcListCoins(m_ipclist);
        std::cout<<"getIpcListCoins end\n"<<std::endl;
        ui->label_title_2->setText(tr("Ip Total")+tr(" ")+QString::number(m_ipclist.size()));
        QWidget * pnewall = new QWidget();
        pvboxlayoutall = new QVBoxLayout();

        pnewall->setLayout(pvboxlayoutall);
        pOldSelectIpcButtons = NULL;
        ui->scrollArea->setWidget(pnewall);
        for(int i=0;i<m_ipclist.size();i++)
        {
            addnewoneipcmessage(m_ipclist.at(i).at(0).toInt(),m_ipclist.at(i).at(1),m_ipclist.at(i).at(2),1,i,m_ipclist.at(i).at(3));
        }
        QSpacerItem* horizontalSpacer = new QSpacerItem(20,40,QSizePolicy::Minimum, QSizePolicy::Expanding);
        pvboxlayoutall->addSpacerItem(horizontalSpacer);
    }
    LOG_WRITE(LOG_INFO,"ipcdialog updateIpcList finish");
}

void ipcdialog::gotoIpcdetailsPage(QString txid)
{
   walletStackBranchPage->setCurrentWidget(ipcdetailsPage);
}
void ipcdialog::gotoIpcdetailsPage(int index)
{
    if(index>=0)
    {
        LOG_WRITE(LOG_INFO,"gotoIpcdetailsPage index ",QString::number(index).toStdString().c_str());
        ipcdetailsPage->resetinfo(index);
    }
    walletStackBranchPage->setCurrentWidget(ipcdetailsPage);
}
void ipcdialog::gotoIpcRegisterPage(QString address)
{
    if(!address.isEmpty())
        IpcRegisterPage->setaddress(address);
    else
        IpcRegisterPage->clearData();
    walletStackBranchPage->setCurrentWidget(IpcRegisterPage);
}
void ipcdialog::gotoBackIpcRegisterPage()
{
    walletStackBranchPage->setCurrentWidget(IpcRegisterPage);
}

void ipcdialog::gotoIpcRegisterInformationPage(QStringList infos)
{
    IpcRegisterInformationPage->setinfos(infos);
    walletStackBranchPage->setCurrentWidget(IpcRegisterInformationPage);
}
void ipcdialog::gotoconfrimtransferIpcPage(QString name,QString address)
{
    ipcconfirmtransfertransactionpage->setInfo(name,address);
    walletStackBranchPage->setCurrentWidget(ipcconfirmtransfertransactionpage);
}
void ipcdialog::gotoconfrimauthorizationIpcPage(QStringList data1)
{

    walletStackBranchPage->setCurrentWidget(ipcconfirmauthorizationtransactionpage);
    ipcconfirmauthorizationtransactionpage->setInfo(data1);
}
void ipcdialog::gotoSuccessfulTradePage(int type)
{
    walletStackBranchPage->setCurrentWidget(SuccessfulTradePage);
    SuccessfulTradePage->setSuccessText(type);
}

void ipcdialog::gotoIpcInspectionTagPage()
{
    IpcInspectionTagPage->cleardata();
    walletStackBranchPage->setCurrentWidget(IpcInspectionTagPage);
}

void ipcdialog::gotoIpcSelectAddressPage()
{
    walletStackBranchPage->setCurrentWidget(ipcSelectAddressPage);
}

void ipcdialog::gotoIpcTransferTransactionPage(QString s1)
{
    IpcTransferTransactionPage->SetNameDisplay(s1);
    IpcTransferTransactionPage->SetStartTime(ipcdetailsPage->getStartTime());
    walletStackBranchPage->setCurrentWidget(IpcTransferTransactionPage);
}
void ipcdialog::gotoIpcAuthorizationTransactionPage(QString s1)
{
    IpcAuthorizationTransactionPage->SetNameDisplay(s1);
    IpcAuthorizationTransactionPage->SetStartTime(ipcdetailsPage->getStartTime());
    walletStackBranchPage->setCurrentWidget(IpcAuthorizationTransactionPage);
}
void ipcdialog::changelabel(QLabel* date)
{
    if(pOldSelectIpcButtons == date)return;
    if(pOldSelectIpcButtons)pOldSelectIpcButtons->hide();
    pOldSelectIpcButtons = date;
}
void ipcdialog::updatalist()
{
    if(g_pipcdialog&&g_pipcdialog->walletModel&&g_pipcdialog->walletModel->m_bFinishedLoading){
         g_pipcdialog->updateIpcList();
    }
}
void ipcdialog::updataLater()
{
    if(ipcdialog::m_bNeedUpdateLater)
    {
        updatalist();
        ipcdialog::m_bNeedUpdateLater = false;
    }
}
