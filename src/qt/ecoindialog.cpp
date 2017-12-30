#include "ecoindialog.h"
#include "forms/ui_ecoindialog.h"
#include "ecoincreatedialog.h"
#include "ecoinsenddialog.h"
#include "ecoinsendaffrimdialog.h"
#include "wallet/coincontrol.h"
#include "ecoinselectaddress.h"
#include "ecoinsendresultdialog.h"
//#include "adddetail.h"
#include "optionsmodel.h"
#include "log/log.h"
#include <boost/foreach.hpp>
#include <QFont>
#include <QDebug>
#include "addbookwidget.h"
#include <QSpacerItem>
#include "ecoinaddressdialog.h"
#include <QSettings>
#include <QDateTime>
#include <QPixmapCache>
QSettings settings;
ECoinDialog* g_pECoinDialog = NULL;
ECoinDialog::ECoinDialog(QWidget *parent) :
    QWidget(parent),model(NULL),pOldSelectIpcButtons(NULL),
    ui(new Ui::ecoindialog)
{
    ui->setupUi(this);
    ecoincreateDialogPage = new ecoincreatedialog(this);
    eCoinSenddialogPage =new eCoinSenddialog(this);
    eCoinSendaffrimdialogPage = new ecoinsendaffrimdialog(this);
    pCoinSelectAddressPage = new eCoinSelectAddress(this);
    eCoinSendresultDialogPage = new eCoinSendresultDialog(this);
    m_pEcoinAddressDialog = new EcoinAddressDialog(this);
    ui->stackedWidget->addWidget(ecoincreateDialogPage);
    ui->stackedWidget->addWidget(eCoinSenddialogPage);
    ui->stackedWidget->addWidget(eCoinSendaffrimdialogPage);
    ui->stackedWidget->addWidget(pCoinSelectAddressPage);
    ui->stackedWidget->addWidget(eCoinSendresultDialogPage);
    ui->stackedWidget->addWidget(m_pEcoinAddressDialog);
    ui->stackedWidget->setCurrentWidget(ecoincreateDialogPage);


    connect(m_pEcoinAddressDialog,SIGNAL(back()),this,SLOT(on_createEcoinBtn_pressed()));
    connect(pCoinSelectAddressPage,SIGNAL(back0(QString)),this,SLOT(gotoeCoinCreatePage(QString)));
    connect(pCoinSelectAddressPage,SIGNAL(back1(QString)),this,SLOT(gotoeCoinAddressPage(QString)));
    connect(ecoincreateDialogPage,SIGNAL(selectaddress(int)),this,SLOT(gotoeCoinSelectAddressPage(int)));
    connect(ecoincreateDialogPage,SIGNAL(GetOldAddress()),this,SLOT(GetOldAddress()));
    connect(ecoincreateDialogPage,SIGNAL(CreateeCoinSuccess()),this,SLOT(gotoCreateeCoinResultPage()));
    connect(eCoinSendaffrimdialogPage,SIGNAL(SendeCoinSuccess()),this,SLOT(gotoSendeCoinResultPage()));
    connect(eCoinSendresultDialogPage,SIGNAL(GoSendeCoinSuccess()),this,SLOT(goback()));//gotoSendeCoinPage()));
    connect(eCoinSenddialogPage,SIGNAL(gotoSendAffrimDialog(QString,QString)),this,SLOT(gotoSendAffrimDialogPage(QString,QString)));

    QString add = settings.value("ecoinaddress").toString();
    if(!add.isEmpty())
    {
        m_address = add;
    }
    ui->chooseadd->hide();
    ui->numlabel->hide();
    g_pECoinDialog = this;
}

ECoinDialog::~ECoinDialog()
{
    delete ui;
}


void ECoinDialog::updateDisplayUnit()
{
    QDateTime time = QDateTime::currentDateTime();
    int timeT = time.toTime_t();
    static int timeOld = 0;
    if(timeOld == timeT)
    {
        timeOld = timeT;
        //LOG_WRITE(LOG_INFO,"ECoinDialogList",\
                  QString::number(timeOld).toStdString().c_str(),\
                  QString::number(timeT).toStdString().c_str());
        return;
    }
     LOG_WRITE(LOG_INFO,"ECoinDialogList");
    timeOld = timeT;

    if(model && model->getOptionsModel())
    {
            pOldSelectIpcButtons = NULL;
            model->getTokenListCoins(m_eCoinlist);//

            QWidget * pnewall = new QWidget();
            pvboxlayoutall = new QVBoxLayout();
            pnewall->setLayout(pvboxlayoutall);
            //ui->numlabel->setText(QString::number(m_eCoinlist.size()));
            ui->scrollArea->setWidget(pnewall);

            BOOST_FOREACH( map_t::value_type &i, m_eCoinlist )
            {
                std::cout<<"updateDisplayUnit "<<i.first.c_str()<<" m_eCoinlist "<<QString::number(i.second).toStdString().c_str()<<std::endl;
                addnewoneipcmessage(QString::fromStdString(i.first));
            }
            QSpacerItem* horizontalSpacer = new QSpacerItem(20,40,QSizePolicy::Minimum, QSizePolicy::Expanding);
            pvboxlayoutall->addSpacerItem(horizontalSpacer);
    }

   LOG_WRITE(LOG_INFO,"ECoinDialog::updateDisplayUnit finish");
}


void ECoinDialog::showEvent(QShowEvent *)
{
}

int ECoinDialog::addnewoneipcmessage(QString name)
{
    upgradewidget * pnewone = new upgradewidget();
    pnewone->m_txid = name;
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

    QLabel* namelabel = new QLabel(name);
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
 //   picendlabel->setPixmap(QPixmap(":/res/png/triangle.png"));
    phboxlayout->addWidget(picendlabel);

    picendlabel->hide();

    connect(pnewone,SIGNAL(dlgtxid(QString)),this,SLOT(gotoSendeCoinPage(QString)) );
    connect(pnewone,SIGNAL(selectlabel(QLabel*)),this,SLOT(changelabel(QLabel*)));

    pvboxlayoutall->addWidget(pnewone);

    return 1;

}

void ECoinDialog::setModel(WalletModel *_model)
{

    this->model = _model;
    connect(model,SIGNAL(updataLoadingFinished()),this,SLOT(updateDisplayUnit()));
    //connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    //connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
    ecoincreateDialogPage->setModel(this->model);
    pCoinSelectAddressPage->setWalletModel(this->model);
    eCoinSendaffrimdialogPage->setModel(this->model);
    updateDisplayUnit();
}

void ECoinDialog::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    updateDisplayUnit();

}

void ECoinDialog::gotoCreateeCoinResultPage()
{
    ui->stackedWidget->setCurrentWidget(eCoinSendresultDialogPage);
}
void ECoinDialog::goback()
{
    ecoincreateDialogPage->setinfoclean();
    ui->stackedWidget->setCurrentWidget(ecoincreateDialogPage);
}

void ECoinDialog::gotoSendeCoinResultPage()
{
    ui->stackedWidget->setCurrentWidget(eCoinSendresultDialogPage);
}

void ECoinDialog::gotoSendAffrimDialogPage(QString name,QString num)
{
    eCoinSendaffrimdialogPage->setMsg( name, num);
    ui->stackedWidget->setCurrentWidget(eCoinSendaffrimdialogPage);
}
void ECoinDialog::gotoSendeCoinPage(QString name)
{
    map_t::iterator itor = m_eCoinlist.find(name.toStdString());
    if(itor!=m_eCoinlist.end())
    {
        //std::string strname = name.toStdString();
        //int acc = model->GetAccuracyBySymbol(strname);
        //QString num = QString::number(itor->second);
        //if(acc>0)
       // num=num.insert(num.size()-acc,".");
        QString numold = QString::number(itor->second);
        QString num =model->getAccuracyNumstr(name,numold);
        eCoinSenddialogPage->setMsg(name,num,m_address);
        ui->stackedWidget->setCurrentWidget(eCoinSenddialogPage);
    }

}
void ECoinDialog::gotoeCoinCreatePage(QString address)
{
    if(!address.isEmpty()){
        //if(m_address.isEmpty())
          //  m_address =address;
        ecoincreateDialogPage->setAddress(address);
    }
    ui->stackedWidget->setCurrentWidget(ecoincreateDialogPage);
}

void ECoinDialog::gotoeCoinAddressPage(QString address)
{
    m_address = address;
    settings.setValue("ecoinaddress", m_address);

    m_pEcoinAddressDialog->setAddress(address);
    ui->stackedWidget->setCurrentWidget(m_pEcoinAddressDialog);
}

void ECoinDialog::gotoeCoinSelectAddressPage(int e)
{
    pCoinSelectAddressPage->setClientType(0);
    ui->stackedWidget->setCurrentWidget(pCoinSelectAddressPage);
}

void ECoinDialog::on_createEcoinBtn_pressed()
{
    ecoincreateDialogPage->setinfoclean();
    ui->stackedWidget->setCurrentWidget(ecoincreateDialogPage);
}

void ECoinDialog::gobacktoePage()
{
    ecoincreateDialogPage->setinfoclean();
    ui->stackedWidget->setCurrentWidget(ecoincreateDialogPage);
}
void ECoinDialog::gotoAddEditPage()
{
    /*
    AdddetailPage = new AddDetail();
    AdddetailPage->setModel(model);
    ui->stackedWidget->addWidget(AdddetailPage);
    ui->stackedWidget->setCurrentWidget(AdddetailPage);
    connect(AdddetailPage, SIGNAL(back()), this, SLOT(gobacktoePage()));
*/
}

void ECoinDialog::on_chooseadd_pressed()
{
    pCoinSelectAddressPage->setClientType(1);
    ui->stackedWidget->setCurrentWidget(pCoinSelectAddressPage);
    //connect(pCoinSelectAddressPage, SIGNAL(ecoinpageAddEdit()), this, SLOT(gotoAddEditPage()));
}

void ECoinDialog::changelabel(QLabel* date)
{
    if(pOldSelectIpcButtons)pOldSelectIpcButtons->hide();
    pOldSelectIpcButtons = date;
}
void ECoinDialog::GetOldAddress()
{
    ecoincreateDialogPage->setAddress(pCoinSelectAddressPage->getOldAddress());
}
void ECoinDialog::updatalist()
{
    // LOG_WRITE(LOG_INFO,"ECoinDialog::updatalist");
    if(g_pECoinDialog&&g_pECoinDialog->model&&g_pECoinDialog->model->m_bFinishedLoading){
         g_pECoinDialog->updateDisplayUnit();

    }
}
