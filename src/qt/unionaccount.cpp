#include "unionaccount.h"
#include "ui_unionaccount.h"
#include <QPixmapCache>
#include "walletmodel.h"
#include "unionaccountcreate.h"
#include "unionaccountgenkey.h"
#include "unionaccountjoin.h"
#include "unionaccounttransaction.h"
#include "unionaccounttrasend.h"
#include "unionaccounttrasign.h"
#include "unionaccounthistory.h"
#include "unionacounthistorydetail.h"
#include "unionacounthistorydetailr.h"
#include "successfultrade.h"
#include "ipcselectaddress.h"
#include "upgradewidget.h"
#include "log/log.h"
#include "wallet/wallet.h"
unionaccount* g_unionaccount = NULL;
bool unionaccount::m_bNeedUpdateLater = false;
unionaccount::unionaccount(QWidget *parent) :
    QWidget(parent),pOldSelectIpcButtons(NULL),
    ui(new Ui::unionaccount)
{
    ui->setupUi(this);

    ui->scrollArea->setFrameShape(QFrame::NoFrame);

    ui->label_num->setText(tr("Unionaccount Total num:") + "0");

    walletStackBranchPage = ui->stackedWidget;

    unionaccountcreatePage = new unionaccountcreate(this);
    walletStackBranchPage->addWidget(unionaccountcreatePage);
    connect(unionaccountcreatePage,SIGNAL(opensuccesscreatePage(QString,QString)),this,SLOT(gotosuccesscreatePage(QString,QString)));

    unionaccountgenkeyPage = new unionaccountgenkey(this);
    walletStackBranchPage->addWidget(unionaccountgenkeyPage);
    connect(unionaccountgenkeyPage,SIGNAL(opensuccessgenkeyPage()),this,SLOT(gotounionaccountgenkeyPage()));

    connect(unionaccountgenkeyPage,SIGNAL(selectaddress()),this,SLOT(gotoUnionAddressPage()));

    unionaccountjoinPage= new unionaccountjoin(this);
    walletStackBranchPage->addWidget(unionaccountjoinPage);

    connect(unionaccountjoinPage,SIGNAL(opensuccessjoinPage()),this,SLOT(gotosuccessjoinPage()));

    SuccessfulTradePage= new SuccessfulTrade(this);
    walletStackBranchPage->addWidget(SuccessfulTradePage);
    unionaccounttransactionPage= new unionaccounttransaction(this);
    connect(unionaccounttransactionPage,SIGNAL(gosendPage(QString)),this,SLOT(gotosendPage(QString)));
    connect(unionaccounttransactionPage,SIGNAL(gosignPage()),this,SLOT(gotosignPage()));

    walletStackBranchPage->addWidget(unionaccounttransactionPage);
    unionaccounttrasendPage= new unionaccounttrasend(this);


    walletStackBranchPage->addWidget(unionaccounttrasendPage);
    connect(unionaccounttrasendPage,SIGNAL(opensendsuccessPage(QString,QString)),this,SLOT(gotosendsuccessPage(QString,QString)));
    unionaccounttrasignPage= new unionaccounttrasign(this);
    walletStackBranchPage->addWidget(unionaccounttrasignPage);
    connect(unionaccounttrasignPage,SIGNAL(opensignsuccessPage()),this,SLOT(gotosignsuccessPage()));

    unionSelectAddressPage = new ipcSelectAddress(this);
    walletStackBranchPage->addWidget(unionSelectAddressPage);




    connect(unionSelectAddressPage,SIGNAL(back(QString)),this,SLOT(gobacktounionaccountgenkeyPage(QString)));

    walletStackBranchPage->setCurrentWidget(unionaccountcreatePage);
    connect(unionaccountjoinPage,SIGNAL(refreshunionaccount()),this,SLOT(updateunionaccountList()));
    connect(unionaccountcreatePage,SIGNAL(refreshunionaccount()),this,SLOT(updateunionaccountList()));

    connect(unionaccounttrasendPage,SIGNAL(backtotraPage()),this,SLOT(gobacktotraPage()));
    connect(unionaccounttrasignPage,SIGNAL(backtotraPage()),this,SLOT(gobacktotraPage()));


    unionaccounthistoryPage = new unionaccounthistory(this);
    walletStackBranchPage->addWidget(unionaccounthistoryPage);


    connect(unionaccounttransactionPage,SIGNAL(jumptohistorypage(CAmount,std::string,std::map<uint256,const CWalletTx*>)),this,SLOT(gotohistorypage(CAmount,std::string,std::map<uint256,const CWalletTx*>)));


    connect(unionaccounthistoryPage,SIGNAL(jumptohistorysend(std::string, CAmount,bool,CAmount,QString,QString,QString)),this,SLOT(gotohistorysend(std::string, CAmount,bool,CAmount,QString,QString,QString)));

    connect(unionaccounthistoryPage,SIGNAL(jumptohistoryrecv(std::string, CAmount,bool,CAmount,QString,QString,QString)),this,SLOT(gotohistoryrecv(std::string, CAmount,bool,CAmount,QString,QString,QString)));



    unionacounthistorydetailPage = new unionacounthistorydetail(this);
    walletStackBranchPage->addWidget(unionacounthistorydetailPage);

    unionacounthistorydetailPageR = new unionacounthistorydetailR(this);
    walletStackBranchPage->addWidget(unionacounthistorydetailPageR);


    connect(unionacounthistorydetailPageR,SIGNAL(unionRPage_back()),this,SLOT(unionRPage_goback()));

    connect(unionacounthistorydetailPage,SIGNAL(unionPage_back()),this,SLOT(unionPage_goback()));

    connect(unionaccounthistoryPage,SIGNAL(backtoTraPage()),this,SLOT(gobacktoTrainfoPage()));


    g_unionaccount = this;

}

unionaccount::~unionaccount()
{
    delete ui;
}
void unionaccount::gobacktoTrainfoPage()
{
       walletStackBranchPage->setCurrentWidget(unionaccounttransactionPage);
}
void unionaccount::unionRPage_goback()
{
  walletStackBranchPage->setCurrentWidget(unionaccounthistoryPage);
}
void unionaccount::unionPage_goback()
{
  walletStackBranchPage->setCurrentWidget(unionaccounthistoryPage);
}
void unionaccount::updataLater()
{
    if(unionaccount::m_bNeedUpdateLater)
    {
        updatalist();
        unionaccount::m_bNeedUpdateLater = false;
    }
}
void unionaccount::gotohistorypage(CAmount num,std::string add,std::map<uint256,const CWalletTx*> s)
{
    unionaccounthistoryPage->setnum(num);
   unionaccounthistoryPage->setAdd(add);

   LOG_WRITE(LOG_INFO,"updateinfoupdateinfo",QString::number(s.size()).toStdString().c_str());
   unionaccounthistoryPage->updateinfo(s);

   walletStackBranchPage->setCurrentWidget(unionaccounthistoryPage);
}
void unionaccount::updatalist()
{
    if(g_unionaccount&&g_unionaccount->model&&g_unionaccount->model->m_bFinishedLoading){

        g_unionaccount->updateunionaccountList();
    }
}
void unionaccount::updateunionaccountList()
{
    if(model)
    {
        std::vector<UnionAddressInfo> addinfo;
        model->getUnionAddressinfo(addinfo);
        ui->label_num->setText(tr("Unionaccount Total num:")+tr(" ")+QString::number(addinfo.size()));
        QWidget * pnewall = new QWidget();
        pvboxlayoutall = new QVBoxLayout();
        pnewall->setLayout(pvboxlayoutall);
        ui->scrollArea->setWidget(pnewall);
        pOldSelectIpcButtons = NULL;
        for (std::vector<UnionAddressInfo>::iterator ii = addinfo.begin(); ii != addinfo.end(); ++ii)
        {
            QString address=QString::fromStdString((*ii).address);
            QString name=QString::fromStdString((*ii).name);
            QString tempname = name;
            if(name.size()>8 ){
                tempname = name.mid(0,8);
                tempname+="...";
            }
            addnewoneaccount(name,address,tempname);
        }
        QSpacerItem* horizontalSpacer = new QSpacerItem(20,40,QSizePolicy::Minimum, QSizePolicy::Expanding);
        pvboxlayoutall->addSpacerItem(horizontalSpacer);
    }
}

int unionaccount::addnewoneaccount(QString name,QString add,QString m_name)
{
    upgradewidget * pnewone = new upgradewidget();


    pnewone->name = name;
    pnewone->m_name = m_name;
    pnewone->add =add;
    pnewone->setMaximumHeight(70);
    pnewone->setMinimumHeight(70);
    pnewone->setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(79,194,186));
    pnewone->setPalette(palette);

    QHBoxLayout * phboxlayout = new QHBoxLayout();
    phboxlayout->setContentsMargins(0,0,0,0);
    pnewone->setLayout(phboxlayout);

    QLabel* pemptylabel = new QLabel("  ");
    phboxlayout->addWidget(pemptylabel);
    QLabel* namelabel = new QLabel(m_name);
    namelabel->setAttribute(Qt::WA_TranslucentBackground);
    QFont ftname;
    ftname.setPointSize(16);
    namelabel->setFont(ftname);
    QPalette paname;
    paname.setColor(QPalette::WindowText,Qt::white);
    namelabel->setPalette(paname);
    phboxlayout->addWidget(namelabel);
    QSpacerItem* horizontalSpacer = new QSpacerItem(40,20,QSizePolicy::Expanding);
    phboxlayout->addSpacerItem(horizontalSpacer);
    QLabel* picendlabel = new QLabel();
    pnewone->setipcselectlabel(picendlabel);




    QPixmapCache::clear();
    QPixmapCache::setCacheLimit(1);


    QPixmap p;
    p.load(":/res/png/triangle.png");
    picendlabel->setPixmap(p);
    phboxlayout->addWidget(picendlabel);
    picendlabel->hide();
    connect(pnewone,SIGNAL(selectinfo(QString,QString)),this,SLOT(gotoUnionAccounttrainfo(QString,QString)));

    connect(pnewone,SIGNAL(selectlabel(QLabel*)),this,SLOT(changelabel(QLabel*)));

    pvboxlayoutall->addWidget(pnewone);
    return 1;

}
void unionaccount::changelabel(QLabel* date)
{
    if(pOldSelectIpcButtons == date)return;
    if(pOldSelectIpcButtons)pOldSelectIpcButtons->hide();
    pOldSelectIpcButtons = date;
}
void unionaccount::gotoUnionAccounttrainfo(QString name,QString add)
{

    std::map<std::string,CAmount> addinfo;
    CAmount num  = 0;
    addinfo.clear();

    addinfo.insert(make_pair(add.toStdString().c_str(), num));
    model->getAccounttrainfo(addinfo);
    std::string m_add = add.toStdString().c_str();
    num = addinfo[m_add];
    std::string m_script = "";
    int sign_num = 0;
    int nRequired = 0;
    model->getScript(m_add,sign_num,nRequired,m_script);
    unionaccounttransactionPage->setinfo(name,add,num,m_script,
                                         QString::number(nRequired),
                                         QString::number(sign_num));
    walletStackBranchPage->setCurrentWidget(unionaccounttransactionPage);
}

void unionaccount::gotohistorysend(std::string add, CAmount fee,bool isSend,CAmount num,QString status,QString strtime,QString txid)
{

    LOG_WRITE(LOG_INFO,"gotohistorysend",QString::number(fee).toStdString().c_str());
    unionacounthistorydetailPage->setinfo(add, fee,isSend,num,status,strtime,txid);
    walletStackBranchPage->setCurrentWidget(unionacounthistorydetailPage);
}
void unionaccount::gotohistoryrecv(std::string add, CAmount fee,bool isSend,CAmount num,QString status,QString strtime,QString txid)
{
    unionacounthistorydetailPageR->setinfo(add, fee,isSend,num,status,strtime,txid);
    walletStackBranchPage->setCurrentWidget(unionacounthistorydetailPageR);
}

void unionaccount::setModel(WalletModel *_model)
{
    this->model = _model;
    unionacounthistorydetailPage->setModel(model);
    unionacounthistorydetailPageR->setModel(model);
    unionaccountcreatePage->setModel(this->model);
    unionaccountgenkeyPage->setModel(this->model);
    unionaccountjoinPage->setModel(this->model);
    unionaccounttransactionPage->setModel(this->model);
    unionaccounttrasendPage->setModel(this->model);
    unionaccounttrasignPage->setModel(this->model);
    unionSelectAddressPage->setWalletModel(this->model);
    unionaccounthistoryPage->setModel(this->model);
    connect(model,SIGNAL(updataLoadingFinished()),this,SLOT(updateunionaccountList()));
    //connect(model,SIGNAL(updataLater()),this,SLOT(updataLater()));
    updateunionaccountList();

}
void unionaccount::on_createuniaccBtn_pressed()
{
    unionaccountcreatePage->setInit();
    walletStackBranchPage->setCurrentWidget(unionaccountcreatePage);
}

void unionaccount::on_joinuniaccBtn_pressed()
{
    unionaccountjoinPage->setInit();
    walletStackBranchPage->setCurrentWidget(unionaccountjoinPage);
}

void unionaccount::on_genkeyBtn_pressed()
{
    unionaccountgenkeyPage->clearinfo();
    walletStackBranchPage->setCurrentWidget(unionaccountgenkeyPage);
}
void unionaccount::gotoUnionAddressPage()
{
    walletStackBranchPage->setCurrentWidget(unionSelectAddressPage);

}

void unionaccount::gobacktounionaccountgenkeyPage(QString address)
{
    if(!address.isEmpty())
        unionaccountgenkeyPage->setaddress(address);
    else
        unionaccountgenkeyPage->clearData();
    walletStackBranchPage->setCurrentWidget(unionaccountgenkeyPage);
}
void unionaccount::gotosuccesscreatePage(QString s1,QString s2)
{
    SuccessfulTradePage->setSuccessText_(s1,s2);
    walletStackBranchPage->setCurrentWidget(SuccessfulTradePage);

}
void unionaccount::gotounionaccountgenkeyPage()
{
    walletStackBranchPage->setCurrentWidget(SuccessfulTradePage);

}
void unionaccount::gotosuccessjoinPage()
{
    walletStackBranchPage->setCurrentWidget(SuccessfulTradePage);

}
void unionaccount::gotosendPage(QString add)
{
    unionaccounttrasendPage->setaddress(add);
    walletStackBranchPage->setCurrentWidget(unionaccounttrasendPage);

}
void unionaccount::gotosignPage()
{
    unionaccounttrasignPage->set0Text();
    string address =   unionaccounttransactionPage->getAddress();
    unionaccounttrasignPage->setSourceAddress(address);
    walletStackBranchPage->setCurrentWidget(unionaccounttrasignPage);
}
void unionaccount::gotosendsuccessPage(QString s1,QString s2)
{
    SuccessfulTradePage->setsendSuccessText_(s1,s2);
    walletStackBranchPage->setCurrentWidget(SuccessfulTradePage);
}
void unionaccount::gotosignsuccessPage()
{
    walletStackBranchPage->setCurrentWidget(SuccessfulTradePage);
}
void unionaccount::gobacktotraPage()
{
    walletStackBranchPage->setCurrentWidget(unionaccounttransactionPage);
}
