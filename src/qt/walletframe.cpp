// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletframe.h"

#include "bitcoingui.h"
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

#include "transactiondescdialog.h"
#include "feewidget.h"
#include "sendresultwidget.h"
#include "addbookwidget.h"
#include "addresstablemodel.h"
#include "addressbookpage.h"
#include "editadddialog.h"
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


    //20170904
    walletLayout->addWidget(walletpagebuttonswidget);

    //20170904
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
}
void WalletFrame::headshowchaininfo(int concount,int count,bool header)
{
    if (!header)//20171011
    {
    walletpagebuttonswidget->fresh(concount,count);
    }
}

/*
void WalletFrame::showchaininfo(int count, const QDateTime& blockDate, double nVerificationProgress, bool header)
{

    InfoWidget * singleton1 = InfoWidget::GetInstance();
    singleton1->fresh(count, blockDate,  nVerificationProgress,header);

}
*/
QStackedWidget* WalletFrame::getwaletstack()
{
    return walletStackBranchPage;
}
void WalletFrame::ReceiveData(QList<QString> *inputDataList,int tag)
{
    walletStackBranchPage->setCurrentWidget(AddBookPage);
    AddBookPage->AddEditedRow(inputDataList->at(0),inputDataList->at(1));

}
void WalletFrame::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;
}

bool WalletFrame::addWallet(const QString& name, WalletModel *walletModel)//20170823
{
    if (!gui || !clientModel || !walletModel || mapWalletViews.count(name) > 0)
        return false;

    WalletView *walletView = new WalletView(platformStyle, this);
    walletView->setBitcoinGUI(gui);
    walletView->setClientModel(clientModel);
    walletView->setWalletModel(walletModel);
    walletView->showOutOfSyncWarning(bOutOfSync);


    /* TODO we should goto the currently selected page once dynamically adding wallets is supported */

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

bool WalletFrame::handlePaymentRequest(const SendCoinsRecipient &recipient)
{
    WalletView *walletView = currentWalletView();
    if (!walletView)
        return false;

    return walletView->handlePaymentRequest(recipient);
}

void WalletFrame::showOutOfSyncWarning(bool fShow)
{
    bOutOfSync = fShow;
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->showOutOfSyncWarning(fShow);
}

void WalletFrame::gotoOverviewPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoOverviewPage();
    gotoWalletViewPage();
}

void WalletFrame::gotoHistoryPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoHistoryPage();
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

void WalletFrame::gotoWalletViewPage()
{
    walletStackBranchPage->setCurrentWidget(pwalletStackwidget);
}


void WalletFrame::showDetails(QModelIndexList index)
{
    // QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    QModelIndexList selection  = index;
    if(!selection.isEmpty())
    {
        TransactionDescDialog *dlg = new TransactionDescDialog(selection.at(0));
        connect(dlg,SIGNAL(backMain()),this,SLOT(gotoWalletViewPage()));
        walletStackBranchPage->addWidget(dlg);
        walletStackBranchPage->setCurrentWidget(dlg);

    }

}

void WalletFrame::GoToChainInfoPage()
{
    Q_EMIT requestoverwidget();

}
void WalletFrame::showwwDetails(QModelIndex index,quint8 t)
{
    //sendipc=1,recvipc=2,sendbookkeep=3,recvbookkeep=4
    //sendecoin=5,recvecoin=6sendcoin=7,recvcoin=8
    WalletView *walletView = currentWalletView();

    if(1 == t)//sendipc
    {
        QModelIndex selection  = index;
        sendipcpage = new sendipchistory(selection);
        connect(sendipcpage,SIGNAL(backMain()),this,SLOT(gotoWalletViewPage()));
        walletView->opensendipcpage(sendipcpage);

    }

    else if(2 == t)//recvipc
    {
        QModelIndex selection  = index;
        recvipcpage = new recvipchistory(selection);
        connect(recvipcpage,SIGNAL(backMain()),this,SLOT(gotoWalletViewPage()));
        walletView->openrecvipcpage(recvipcpage);
    }

    else if(5 == t)
    {
        QModelIndex selection  = index;
        sendtokenpage = new SendTokenHistory(selection);
        connect(sendtokenpage,SIGNAL(backMain()),this,SLOT(gotoWalletViewPage()));
        walletView->opensendtokenpage(sendtokenpage);
    }
    else if(6 == t)
    {
        QModelIndex selection  = index;
        recvtokenpage = new RecvTokenHistory(selection);
        connect(recvtokenpage,SIGNAL(backMain()),this,SLOT(gotoWalletViewPage()));
        walletView->openrecvtokenpage(recvtokenpage);
    }
    else if(7 ==t)
    {
        QModelIndex selection  = index;
        sendpage = new sendhistory(selection);
        walletView->opensendpage(sendpage);
    }
    else if(8 ==t)
    {
        QModelIndex selection  = index;
        recvpage = new RecvHistory(selection);
        connect(recvpage,SIGNAL(backMain()),this,SLOT(gotoWalletViewPage()));
        walletView->openrecvpage(recvpage);
    }

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

void WalletFrame::gotoSignMessageTab(QString addr)
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->gotoSignMessageTab(addr);
}

void WalletFrame::gotoVerifyMessageTab(QString addr)
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->gotoVerifyMessageTab(addr);
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

void WalletFrame::changePassphrase()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->changePassphrase();
}

void WalletFrame::unlockWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->unlockWallet();
}

void WalletFrame::usedSendingAddresses()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->usedSendingAddresses();
}

void WalletFrame::usedReceivingAddresses()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->usedReceivingAddresses();
}

WalletView *WalletFrame::currentWalletView()
{
    return qobject_cast<WalletView*>(walletStack->currentWidget());
}

void WalletFrame::outOfSyncWarningClicked()
{
    Q_EMIT requestedSyncWarningInfo();
}


