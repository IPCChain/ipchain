#include "unionaccounthistory.h"
#include "forms/ui_unionaccounthistory.h"
#include "log/log.h"
#include "base58.h"
#include "validation.h"
#include "timedata.h"
#include <guiutil.h>
#include <QDateTime>
#include "upgradewidget.h"
#include <QHBoxLayout>
#include <QPalette>
#include <QFont>
#include <QSpacerItem>
#include <QPainter>
#include "ipchainunits.h"
#include "walletmodel.h"
unionaccounthistory::unionaccounthistory(QWidget *parent) :
    QWidget(parent),m_pwalletmodel(NULL),
    ui(new Ui::unionaccounthistory)
{
    ui->setupUi(this);
}

unionaccounthistory::~unionaccounthistory()
{
    delete ui;
}

void unionaccounthistory::setAdd(std::string add)
{
    m_add = add;
}
void unionaccounthistory::setnum(CAmount num)
{
    m_num = num;
    LOG_WRITE(LOG_INFO,"setnum ",QString::number(m_num).toStdString().c_str());

}
std::string  unionaccounthistory::getAdd()
{
    return m_add;
}
CAmount unionaccounthistory::getnum()
{
    return m_num ;
}
bool comp(const tra_info &a,const tra_info &b)
{
    return a.strtime>b.strtime;
}
void unionaccounthistory::updateinfo(std::map<uint256,const CWalletTx*> s)
{
    m_tra.clear();
    std::vector<tra_info> m_tra_plus;
    m_tra_plus.clear();

    LOG_WRITE(LOG_INFO,"updateinfoupdateinfo--------detaildetail",QString::number(s.size()).toStdString().c_str());

    if(s.size() > 0 )
    {
        std::map<uint256,const CWalletTx*>::iterator it;

        it = s.begin();

        while(it != s.end())
        {
            uint256 txid = it->first;
          //  LOG_WRITE(LOG_INFO,"tra detail",(const char*)(char*)(unsigned char*)&txid);
            const CWalletTx* wtx =it->second;

            LOG_WRITE(LOG_INFO,"tra detailtwice",it->second->GetHash().ToString().c_str());
            bool isSend = false;
            bool isRecv = false;
            if(m_pwalletmodel&&m_pwalletmodel->vinsFindAddress(wtx->tx->vin,m_add))
            {
                isSend = true;
            }
            else
            {
                isRecv = true;
            }

            LOG_WRITE(LOG_INFO,"TEST---IsSend,IsRecv",QString::number(isSend).toStdString().c_str(),QString::number(isRecv).toStdString().c_str());


            if (isSend&&!isRecv)
            {
                CAmount num = 0;
                CTxDestination address;
                std::string add;
                for (unsigned int nOut = 0; nOut < wtx->tx->vout.size(); nOut++)
                {
                    const CTxOut& txout = wtx->tx->vout[nOut];

                    if(nOut < wtx->tx->vout.size()-1||wtx->tx->vout.size()==1)
                    {

                    if (ExtractDestination(txout.scriptPubKey, address))
                        add= CBitcoinAddress(address).ToString();
                    }
                    uint256 hash = wtx->GetHash();

                    QString m_status=m_pwalletmodel->FormatTxStatus(*wtx);
                    QString m_strtime =m_pwalletmodel->FormatTxTimeP2SH(*wtx);

                    CAmount m_fee = Getfeebytxid(static_cast<CTransaction>(*wtx));

                     if(nOut < wtx->tx->vout.size()-1||wtx->tx->vout.size()==1)
                     {
                     num+= txout.nValue;
                     }
                     LOG_WRITE(LOG_INFO,"Total Num",QString::number(num).toStdString().c_str());

                    if(0==txout.txType && (nOut == wtx->tx->vout.size()-1||wtx->tx->vout.size()==1))
                    {
                        LOG_WRITE(LOG_INFO,"num +++ ",QString::number(txout.nValue).toStdString().c_str(),QString::number(m_num).toStdString().c_str());

                        tra_info m_trainfo;
                        m_trainfo.add = add;
                        m_trainfo.fee= m_fee;
                        m_trainfo.isSend = true;
                        m_trainfo.num =num ;
                        m_trainfo.strtime = m_strtime;
                        m_trainfo.status = m_status;
                        m_trainfo.txid = QString::fromStdString(txid.ToString());
                        LOG_WRITE(LOG_INFO,"txid ",hash.ToString().c_str(),\
                                  "starttime ",m_strtime.toStdString().c_str(),\
                                  QString::number((*wtx).nTimeSmart).toStdString().c_str(),\
                                  QString::number((*wtx).nTimeReceived).toStdString().c_str());
                        LOG_WRITE(LOG_INFO,"IsSend",QString::number(m_trainfo.isSend).toStdString().c_str());

                        m_tra.push_back(m_trainfo);
                    }

                }
            }

            if(isRecv && ! isSend)
            {
                for (unsigned int nOut = 0; nOut < wtx->tx->vout.size()-1; nOut++)
                {
                    const CTxOut& txout = wtx->tx->vout[nOut];

                    CTxDestination address;
                    std::string add;
                    if (ExtractDestination(txout.scriptPubKey, address))
                        add= CBitcoinAddress(address).ToString();

                    int64_t nTime = wtx->GetTxTime();
                    CAmount num = 0;
                    num +=txout.nValue;
                    uint256 hash = wtx->GetHash();
                    QString m_status=m_pwalletmodel->FormatTxStatus(*wtx);
                    QString m_strtime =m_pwalletmodel->FormatTxTimeP2SH(*wtx);


                    if(nOut == wtx->tx->vout.size()-2)
                    {
                        tra_info m_trainfo;
                        m_trainfo.add = add;
                        m_trainfo.fee= 0;
                        m_trainfo.isSend = false;
                        m_trainfo.num =num;
                        m_trainfo.strtime = m_strtime;
                        m_trainfo.status = m_status;
                        m_trainfo.txid = QString::fromStdString(txid.ToString());

                        LOG_WRITE(LOG_INFO,"ISRECV",QString::number(m_trainfo.isSend).toStdString().c_str());

                        m_tra.push_back(m_trainfo);
                    }
                }
                if(wtx->tx->vout.size()==1){
                    const CTxOut& txout = wtx->tx->vout[0];
                    CTxDestination address;
                    std::string add;
                    if (ExtractDestination(txout.scriptPubKey, address))
                        add= CBitcoinAddress(address).ToString();
                    int64_t nTime = wtx->GetTxTime();
                    CAmount num = 0;
                    num +=txout.nValue;
                    uint256 hash = wtx->GetHash();
                    QString m_status=m_pwalletmodel->FormatTxStatus(*wtx);
                    QString m_strtime =m_pwalletmodel->FormatTxTimeP2SH(*wtx);
                    tra_info m_trainfo;
                    m_trainfo.add = add;
                    m_trainfo.fee= 0;
                    m_trainfo.isSend = false;
                    m_trainfo.num =num;
                    m_trainfo.strtime = m_strtime;
                    m_trainfo.status = m_status;
                    m_trainfo.txid = QString::fromStdString(txid.ToString());
                    LOG_WRITE(LOG_INFO,"txid ",hash.ToString().c_str(),\
                              "starttime ",m_strtime.toStdString().c_str(),\
                              QString::number((*wtx).nTimeSmart).toStdString().c_str(),\
                              QString::number((*wtx).nTimeReceived).toStdString().c_str());
                    LOG_WRITE(LOG_INFO,"ISRECV",QString::number(m_trainfo.isSend).toStdString().c_str());
                    m_tra.push_back(m_trainfo);
                }

            }
            it ++;

        }


    }

    //m_tra sort

     if(m_tra.size() >0)
     {
         for(int i = 0;i<m_tra.size();i++)
         {
           m_tra_plus.push_back(m_tra.at(i));
         }
     }
     for(int i = 0;i<m_tra_plus.size();i++)
     {
       LOG_WRITE(LOG_INFO,"befort sort",m_tra_plus.at(i).strtime.toStdString().c_str());
     }
     sort(m_tra_plus.begin(),m_tra_plus.end(),comp);
     if(m_tra.size() >0)
     {
         for(int i = 0;i<m_tra_plus.size();i++)
         {
           LOG_WRITE(LOG_INFO,"m_tra strtimr",m_tra_plus.at(i).strtime.toStdString().c_str());
         }
     }


    QWidget * pnewall = new QWidget();
    pvboxlayoutall = new QVBoxLayout();

    pnewall->setLayout(pvboxlayoutall);
    ui->scrollArea->setWidget(pnewall);

    LOG_WRITE(LOG_INFO,"m_tra size",QString::number(m_tra.size()).toStdString().c_str());
    m_tra.clear();
    if(m_tra_plus.size() > 0 )
    {
        for(int i = 0;i<m_tra_plus.size();i++)
        {


            LOG_WRITE(LOG_INFO,"addinfo issend true or flase ==",QString::number(m_tra_plus.at(i).isSend).toStdString().c_str());

            addinfo(m_tra_plus.at(i).add,m_tra_plus.at(i).fee,
                    m_tra_plus.at(i).isSend,
                    m_tra_plus.at(i).num,
                    m_tra_plus.at(i).status,
                    m_tra_plus.at(i).strtime,
                    m_tra_plus.at(i).txid);
            addline();
        }
    }
    QSpacerItem* horizontalSpacer = new QSpacerItem(20,40,QSizePolicy::Minimum, QSizePolicy::Expanding);
    pvboxlayoutall->addSpacerItem(horizontalSpacer);
}
void unionaccounthistory::addline()
{
    upgradewidget * pnewone = new upgradewidget();
    pnewone->m_isunion = true;
    pnewone->setMaximumHeight(17);
    pnewone->setMinimumHeight(17);
    pnewone->setAutoFillBackground(true);
    QPalette palette(pnewone->palette());
    palette.setColor(QPalette::Background, QColor(0,0,0));
    pnewone->setPalette(palette);

    QHBoxLayout * phboxlayout = new QHBoxLayout();
    phboxlayout->setContentsMargins(0,0,0,0);

    pnewone->setLayout(phboxlayout);


    QLabel* namelabel = new QLabel("______________________________________________________________________");
    namelabel->setAttribute(Qt::WA_TranslucentBackground);
    QFont ftname;
    ftname.setPointSize(15);
    namelabel->setFont(ftname);
    QPalette paname;
    paname.setColor(QPalette::WindowText,Qt::gray);
    namelabel->setPalette(paname);
    namelabel->setStyleSheet("font-size : 15px;color: rgb(192,192,192);");
    phboxlayout->addWidget(namelabel);

    pvboxlayoutall->addWidget(pnewone);

}
void unionaccounthistory::addinfo(std::string add, CAmount fee,bool isSend,CAmount num,
                                  QString status,QString strtime,QString txid)
{


    LOG_WRITE(LOG_INFO,"addinfo",add.c_str(),QString::number(fee).toStdString().c_str(),QString::number(num).toStdString().c_str(),status.toStdString().c_str(),strtime.toStdString().c_str());
    upgradewidget * pnewone = new upgradewidget();

    pnewone->setunioninfo( add, fee,isSend, num,status, strtime);
    connect(pnewone,SIGNAL(lookinfo(std::string,bool,QString,QString,CAmount,CAmount,QString)),this,SLOT(lookupinfodetail(std::string,bool,QString,QString,CAmount,CAmount,QString)));

    pnewone->m_isunion = true;
    pnewone->m_txid = txid;
    pnewone->setMaximumHeight(70);
    pnewone->setMinimumHeight(70);
    pnewone->setAutoFillBackground(true);
    QPalette palette(pnewone->palette());
    palette.setColor(QPalette::Background, QColor(0,0,0));
    pnewone->setPalette(palette);

    QHBoxLayout * phboxlayout = new QHBoxLayout();
    phboxlayout->setContentsMargins(0,0,0,0);






    pnewone->setLayout(phboxlayout);



    QLabel* pictypelabel = new QLabel();
    pictypelabel->setAutoFillBackground(true);
    phboxlayout->addWidget(pictypelabel);

    QLabel* tag;
    if(isSend)
    {
        tag = new QLabel(tr("isSend"));
    }
    else
    {
        tag = new QLabel(tr("isRecv"));
    }
    tag->setAutoFillBackground(true);
    phboxlayout->addWidget(tag);
    QSpacerItem* horizontalSpacer = new QSpacerItem(40,20,QSizePolicy::Expanding);
    phboxlayout->addSpacerItem(horizontalSpacer);



    QVBoxLayout * pvboxlayout = new QVBoxLayout();
    phboxlayout->addLayout(pvboxlayout);


   // QLabel* namelabel = new QLabel(BitcoinUnits::formatWithUnit2(0, num, false, BitcoinUnits::separatorAlways));


    QLabel* namelabel;
    if(isSend)
    {
        namelabel= new QLabel("-" + BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, num));
    }
    else
    {
        namelabel= new QLabel("+" + BitcoinUnits::formatWithUnit(BitcoinUnit::IPC, num));
    }



    namelabel->setAttribute(Qt::WA_TranslucentBackground);
    QFont ftname;
    ftname.setPointSize(16);
    namelabel->setFont(ftname);
    QPalette paname;
    paname.setColor(QPalette::WindowText,Qt::white);
    namelabel->setPalette(paname);
    namelabel->setStyleSheet("font-size : 16px;color: rgb(0,0,0);");
    pvboxlayout->addWidget(namelabel);

    QLabel* timelabel = new QLabel(strtime);
    timelabel->setAttribute(Qt::WA_TranslucentBackground);
    QFont fttime;
    fttime.setPointSize(12);
    timelabel->setFont(fttime);
    QPalette patime;
    patime.setColor(QPalette::WindowText,Qt::white);
    timelabel->setPalette(patime);
    timelabel->setStyleSheet("font-size : 12px;color: rgb(0,0,0);");
    pvboxlayout->addWidget(timelabel);

    QString picpath;

    if(isSend)
    {
        picpath = ":/res/png/sendico.png";
    }
    else
    {
        picpath = ":/res/png/recvicon.png";
    }
    pictypelabel->setFixedSize(41,41);
    pictypelabel->setPixmap(QPixmap(picpath));


    pvboxlayoutall->addWidget(pnewone);





}
void unionaccounthistory::lookupinfodetail(std::string add,bool isSend,QString status,QString strtime,CAmount s1,CAmount s2,QString txid)
{
    LOG_WRITE(LOG_INFO,"lookupinfodetail");

    if(isSend)
    {
        Q_EMIT jumptohistorysend(add, s1,isSend,s2,status,strtime,txid);
    }
    else
    {
        Q_EMIT jumptohistoryrecv(add, s1,isSend,s2,status,strtime,txid);
    }

}
void unionaccounthistory::on_btn_back_pressed()
{
    Q_EMIT backtoTraPage();
}
void unionaccounthistory::setModel(WalletModel *_model)
{
    this->m_pwalletmodel = _model;
}
