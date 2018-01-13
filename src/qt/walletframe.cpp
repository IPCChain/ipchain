// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletframe.h"

#include "ipchaingui.h"
#include "walletview.h"
#include "settingwidget.h"
#include "passwordsettingwidget.h"
#include "sendcoinsaffrimwidget.h"
#include <cstdio>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include "sendresultwidget.h"
#include "addresstablemodel.h"
#include "infowidget.h"
#include "sendhistory.h"
#include "recvhistory.h"
#include "sendipchistory.h"
#include "recvipchistory.h"
#include "sendipchistory.h"
#include "recvtokenhistory.h"
#include "sendtokenhistory.h"
#include "walletpagebuttons.h"
#include "log/log.h"
#include "transactiontablemodel.h"

WalletFrame::WalletFrame(const PlatformStyle *_platformStyle, BitcoinGUI *_gui) :
    QFrame(_gui),
    gui(_gui),
    platformStyle(_platformStyle)
{
    // Leave VBox hook for adding a list view later
    QVBoxLayout *walletFrameLayout = new QVBoxLayout(this);
    setContentsMargins(0,0,0,0);
    walletStack = new QStackedWidget(this);
    walletStack->setContentsMargins(0,0,0,0);
    // walletStack->setStyleSheet("background-color:rgb(255,255,255)");
    QPalette pal(walletStack->palette());
    pal.setColor(QPalette::Background, Qt::white);
    walletStack->setAutoFillBackground(true);
    //walletStack->setPalette(pal);
    walletFrameLayout->setContentsMargins(0,0,0,0);
    walletpagebuttonswidget = new walletpagebuttons(this);
    connect(walletpagebuttonswidget,SIGNAL(walletpressed()),this,SLOT(gotoOverviewPage()));
    connect(walletpagebuttonswidget,SIGNAL(sendpressed()),this,SLOT(gotoSendCoinsPage()));
    connect(walletpagebuttonswidget,SIGNAL(recivepressed()),this,SLOT(gotoReceiveCoinsPage()));
    connect(walletpagebuttonswidget,SIGNAL(ipcpressed()),this,SLOT(gotoIpcPage()));
    connect(walletpagebuttonswidget,SIGNAL(setpressed()),this,SLOT(gotoSetPage()));
    connect(walletpagebuttonswidget,SIGNAL(tallypressed()),this,SLOT(gotoTallyPage()));
    connect(walletpagebuttonswidget,SIGNAL(eipcpressed()),this,SLOT(gotoeCoinPage()));

    QVBoxLayout *walletLayout = new QVBoxLayout(this);
    walletLayout->setContentsMargins(0,0,0,0);
    walletLayout->addWidget(walletpagebuttonswidget);

    pwalletStackwidget = new QWidget(this);
    pwalletStackwidget->setLayout(walletLayout);
    walletStackBranchPage =  new QStackedWidget(this);
    walletStackBranchPage->addWidget(pwalletStackwidget);
    walletLayout->addWidget(walletStack);
    walletStackBranchPage->setCurrentWidget(pwalletStackwidget);

    walletFrameLayout->addWidget(walletStackBranchPage);
    QLabel *noWallet = new QLabel(tr("No wallet has been loaded."));
    noWallet->setAlignment(Qt::AlignCenter);
    walletStack->addWidget(noWallet);


}

WalletFrame::~WalletFrame()
{
    LOG_WRITE(LOG_INFO,"WalletFrame::~WalletFrameWalletFrame::~WalletFrameWalletFrame::~WalletFrame");
    delete sendpage;

    delete recvpage ;

    delete sendipcpage;

    delete recvipcpage;

    delete  sendtokenpage ;
    delete  recvtokenpage;
    if(pollTimer)
    {
        sel =QModelIndex();
        selid = -1;
        pollTimer->stop();
        delete pollTimer;
    }

}
void WalletFrame::headshowchaininfo(int concount,int count,bool header)
{
    if (!header)
    {
        walletpagebuttonswidget->fresh(concount,count);
    }
}


QStackedWidget* WalletFrame::getwaletstack()
{
    return walletStackBranchPage;
}

void WalletFrame::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;
}

bool WalletFrame::addWallet(const QString& name, WalletModel *walletModel)
{
    if (!gui || !clientModel || !walletModel || mapWalletViews.count(name) > 0)
        return false;

    WalletView *walletView = new WalletView(platformStyle, this);
    walletView->setBitcoinGUI(gui);
    walletView->setClientModel(clientModel);
    walletView->setWalletModel(walletModel);
    this->walletmd = walletModel;
    this->clientmd = clientModel;

    walletView->gotoOverviewPage();
    walletStack->addWidget(walletView);
    mapWalletViews[name] = walletView;

    // Ensure a walletView is able to show the main window
    connect(walletView, SIGNAL(showNormalIfMinimized()), gui, SLOT(showNormalIfMinimized()));

    connect(walletView, SIGNAL(outOfSyncWarningClicked()), this, SLOT(outOfSyncWarningClicked()));

    return true;
}

bool WalletFrame::setCurrentWallet(const QString& name)
{
    if (mapWalletViews.count(name) == 0)
        return false;

    WalletView *walletView = mapWalletViews.value(name);
    walletStack->setCurrentWidget(walletView);
    walletView->updateEncryptionStatus();
    return true;
}

bool WalletFrame::removeWallet(const QString &name)
{
    if (mapWalletViews.count(name) == 0)
        return false;

    WalletView *walletView = mapWalletViews.take(name);
    walletStack->removeWidget(walletView);
    return true;
}

void WalletFrame::removeAllWallets()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        walletStack->removeWidget(i.value());
    mapWalletViews.clear();
}

void WalletFrame::gotoOverviewPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoOverviewPage();

}

void WalletFrame::gotoIpcPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoIpcPage();
}

void WalletFrame::gotoSetPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoSetPage();
}
void WalletFrame::gotoTallyPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoTallyPage();
}
void WalletFrame::gotoeCoinPage(){
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoeCoinPage();

}
void WalletFrame::GoToChainInfoPage()
{
    Q_EMIT requestoverwidget();

}

void WalletFrame::StatusInfoUpdate()
{
    if(!clientmd) return;
    if(!walletmd) return;

    if(sel.isValid())// && sel.data().isValid() && !sel.data().isNull())
    {
        if(TAB_sendipc == selid)
        {
            if(sendipcpage)
            {
                QString m_status = sel.data(TransactionTableModel::InfoStatus).toString();

                sendipcpage->updateInfo(m_status);
            }
        }

        else if(TAB_recvipc == selid)//recvipc
        {
            if(recvipcpage)
            {
                QString m_status = sel.data(TransactionTableModel::InfoStatus).toString();

                recvipcpage->updateInfo(m_status);
            }
        }

        else if(TAB_sendtoken == selid)
        {
            if(sendtokenpage)
            {
                QString m_status = sel.data(TransactionTableModel::InfoStatus).toString();

                sendtokenpage->updateInfo(m_status);
            }
        }
        else if(TAB_recvtoken == selid)
        {
            if(recvtokenpage)
            {
                QString m_status = sel.data(TransactionTableModel::InfoStatus).toString();

                recvtokenpage->updateInfo(m_status);
            }
        }
        else if(TAB_send ==selid)
        {
            if(sendpage)
            {
                QString m_status = sel.data(TransactionTableModel::InfoStatus).toString();

                sendpage->updateInfo(m_status);
            }
        }
        else if(TAB_recv ==selid)
        {
            if(recvpage)
            {
                QString m_status = sel.data(TransactionTableModel::InfoStatus).toString();

                recvpage->updateInfo(m_status);
            }
        }
        else if(-1 == selid)
        {


            return;
        }



    }
}
void WalletFrame::showwwDetails(QModelIndex index,quint8 t)
{

    if(pollTimer)
    {
        sel =QModelIndex();
        selid = -1;
        pollTimer->stop();
        delete pollTimer;

    }
    WalletView *walletView = currentWalletView();
    sel  = index;
    selid = t;
    if(TAB_sendipc == t)//sendipc
    {
        QModelIndex selection  = index;
        sendipcpage = new sendipchistory(selection);
        walletView->opensendipcpage(sendipcpage);

    }

    else if(TAB_recvipc == t)//recvipc
    {
        QModelIndex selection  = index;
        recvipcpage = new recvipchistory(selection);
        walletView->openrecvipcpage(recvipcpage);
    }

    else if(TAB_sendtoken == t)
    {
        QModelIndex selection  = index;
        sendtokenpage = new SendTokenHistory(selection);
        walletView->opensendtokenpage(sendtokenpage);
    }
    else if(TAB_recvtoken== t)
    {
        QModelIndex selection  = index;
        recvtokenpage = new RecvTokenHistory(selection);
        walletView->openrecvtokenpage(recvtokenpage);
    }
    else if(TAB_send ==t)
    {
        QModelIndex selection  = index;
        sendpage = new sendhistory(selection);
        walletView->opensendpage(sendpage);
    }
    else if(TAB_recv ==t)
    {
        QModelIndex selection  = index;
        recvpage = new RecvHistory(selection);
        walletView->openrecvpage(recvpage);
    }

    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(StatusInfoUpdate()));
    pollTimer->start(10000);
}
void WalletFrame::gotoReceiveCoinsPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoReceiveCoinsPage();
}

void WalletFrame::gotoSendCoinsPage(QString addr)
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoSendCoinsPage(addr);
}



void WalletFrame::encryptWallet(bool status)
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->encryptWallet(status);
}

void WalletFrame::backupWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->backupWallet();
}

void WalletFrame::unlockWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->unlockWallet();
}
WalletView *WalletFrame::currentWalletView()
{
    return qobject_cast<WalletView*>(walletStack->currentWidget());
}

void WalletFrame::outOfSyncWarningClicked()
{
    Q_EMIT requestedSyncWarningInfo();
}


