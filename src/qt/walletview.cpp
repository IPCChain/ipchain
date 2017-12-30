// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletview.h"

#include "addressbookpage.h"
#include "askpassphrasedialog.h"
#include "AskPassphrase.h"
#include "bitcoingui.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "overviewpage.h"
#include "platformstyle.h"
#include "receivecoinsdialog.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "transactiontablemodel.h"
#include "transactionview.h"
#include "walletmodel.h"
#include "settingwidget.h"
#include "ui_interface.h"
#include "ipcdialog.h"
#include "setdialog.h"
#include "infowidget.h"
#include "sendhistory.h"
#include "recvhistory.h"
#include "sendipchistory.h"
#include "recvipchistory.h"
#include "sendresultwidget.h"
#include "sendtokenhistory.h"
#include "recvtokenhistory.h"

#include "passwordsettingwidget.h"
#include "sendcoinsaffrimwidget.h"
#include  "addresstablemodel.h"
#include "addbookwidget.h"
#include "editadddialog.h"
#include "ecoindialog.h"
#include "intro.h"

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include "tallyaccount.h"
#include "tallyapply.h"
#include "tallyclause.h"
#include "tallydscribe.h"
#include "tallyoutaccount.h"
#include "ipcselectaddress.h"

#include "dpoc/DpocInfo.h"
#include "dpoc/DpocMining.h"
#include "log/log.h"

WalletView::WalletView(const PlatformStyle *_platformStyle, QWidget *parent):
    QStackedWidget(parent),
    clientModel(0),
    walletModel(0),
    platformStyle(_platformStyle),
    m_IsTallying(false)
{
    // Create tabs
    overviewPage = new OverviewPage(platformStyle);

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    QHBoxLayout *hbox_buttons = new QHBoxLayout();


    transactionView = new TransactionView(platformStyle, this);
    vbox->addWidget(transactionView);


    QPushButton *exportButton = new QPushButton(tr("&Export"), this);
    exportButton->setToolTip(tr("Export the data in the current tab to a file"));
    if (platformStyle->getImagesOnButtons()) {
        exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/export"));
    }
    hbox_buttons->addStretch();
    hbox_buttons->addWidget(exportButton);
    vbox->addLayout(hbox_buttons);
    transactionsPage->setLayout(vbox);

    receiveCoinsPage = new ReceiveCoinsDialog(platformStyle);
    sendCoinsPage = new SendCoinsDialog(platformStyle);
    pTallySelectAddressPage = new ipcSelectAddress(this);
    ipcdialogPage = new ipcdialog();
    setdialogPage = new setdialog();
    tallyDialogPage = new QWidget();
    tallyDialogPageStack = new QStackedWidget();


    infopage = new InfoWidget();

    QVBoxLayout *walletFrameLayout = new QVBoxLayout();
    walletFrameLayout->setContentsMargins(0,0,0,0);
    walletFrameLayout->setContentsMargins(0,0,0,0);
    tallyDialogPage->setLayout(walletFrameLayout);
    walletFrameLayout->addWidget(tallyDialogPageStack);

    pTallyAccount = new TallyAccount(this);
    pTallyApply = new TallyApply(this);
    pTallyClause = new TallyClause(this);
    pTallyDscribe = new TallyDscribe(this);
    pTallyOutAccount = new TallyOutAccount(this);
    tallyDialogPageStack->addWidget(pTallyAccount);
    tallyDialogPageStack->addWidget(pTallyApply);
    tallyDialogPageStack->addWidget(pTallyClause);
    tallyDialogPageStack->addWidget(pTallyOutAccount);
    tallyDialogPageStack->addWidget(pTallyDscribe);
    tallyDialogPageStack->addWidget(pTallySelectAddressPage);
    tallyDialogPageStack->setCurrentWidget(pTallyDscribe);
    connect(pTallyDscribe,SIGNAL(next()),this,SLOT(gotoClausePage()));
    connect(pTallyClause,SIGNAL(next()),this,SLOT(gotoTallyApplyPage()));
    connect(pTallyClause,SIGNAL(back()),this,SLOT(gotoDscribePage()));
    connect(pTallyApply,SIGNAL(next(QString,CAmount)),this,SLOT(gotoTallyAccountPage(QString,CAmount)));
    connect(pTallyApply,SIGNAL(selectaddress()),this,SLOT(gotoTallyAddressPage()));
    connect(pTallyAccount,SIGNAL(next(QString,CAmount)),this,SLOT(gotoTallyOutAccountPage(QString,CAmount)));
    connect(pTallyOutAccount,SIGNAL(next(bool)),this,SLOT(gotoBackTallyAccountPage(bool)));
    connect(pTallySelectAddressPage,SIGNAL(back(QString)),this,SLOT(gotoTallyApplyPage(QString)));

    ecoinDialogPage =  new ECoinDialog(this);

    usedSendingAddressesPage = new AddressBookPage(platformStyle, AddressBookPage::ForEditing, AddressBookPage::SendingTab, this);
    usedReceivingAddressesPage = new AddressBookPage(platformStyle, AddressBookPage::ForEditing, AddressBookPage::ReceivingTab, this);
    addWidget(overviewPage);
    addWidget(transactionsPage);
    addWidget(receiveCoinsPage);
    addWidget(sendCoinsPage);
    addWidget(ipcdialogPage);
    addWidget(setdialogPage);
    addWidget(tallyDialogPage);
    addWidget(ecoinDialogPage);

    //20171001
    settingwidgetPage= new Settingwidget(this);

    addWidget(settingwidgetPage);
    connect(settingwidgetPage,SIGNAL(back()),this,SLOT(gotoWalletViewPage()));
    connect(settingwidgetPage,SIGNAL(openPasswordSetwidget(int)),this,SLOT(gotoPasswordSetwidgetPage(int)));
    connect(settingwidgetPage,SIGNAL(openSendCoinsAffrimwidget()),this,SLOT(gotoSendCoinsAffrimPage()));



   // Clicking on a transaction on the overview pre-selects the transaction on the transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), parent, SLOT(focusTransaction(QModelIndex)));
    connect(overviewPage, SIGNAL(transactionnnClicked(QModelIndex,quint8)), parent, SLOT(showwwDetails(QModelIndex,quint8)));
    connect(overviewPage, SIGNAL(outOfSyncWarningClicked()), this, SLOT(requestedSyncWarningInfo()));
    connect(overviewPage, SIGNAL(chaininfoClicked()), parent, SLOT(GoToChainInfoPage()));
    connect(overviewPage, SIGNAL(openTransactionDescPage(QModelIndexList)),parent, SLOT(showDetails(QModelIndexList)));
    connect(sendCoinsPage,SIGNAL(openSettingwidget(QString,QString,QString,int)),this,SLOT(gotoSettingPage(QString,QString,QString,int)));

    connect(sendCoinsPage,SIGNAL(openSendAffrimwidget(QString,QString,QString,int)),this,SLOT(gotoSendaffrimPage(QString,QString,QString,int)));

    connect(sendCoinsPage,SIGNAL(openAddBookPagewidget(AddressTableModel*,int)),this,SLOT(gotoAddBookPage(AddressTableModel*,int)));
    connect(setdialogPage,SIGNAL(openAddBookPagewidget(AddressTableModel*,int)),this,SLOT(gotoAddBookPage(AddressTableModel*,int)));
    connect(setdialogPage,SIGNAL(openPasswordSetwidget(int)),this,SLOT(gotoPasswordSetwidgetPage(int)));
    connect(this,SIGNAL(openadd()),this,SLOT(openAddBookPage()));

    connect(exportButton, SIGNAL(clicked()), transactionView, SLOT(exportClicked()));
    connect(sendCoinsPage, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
    // Pass through messages from transactionView
    connect(transactionView, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));


}

WalletView::~WalletView()
{
}

void WalletView::openAddBookPage()
{
    AddBookPage = new AddBookWidget(this);

    AddBookPage->settag(5);
    setdialogPage->setAddpageshow(AddBookPage);

    AddBookPage->setModel(walletModel->getAddressTableModel());
    connect(AddBookPage,SIGNAL(openeditadddialog(AddressTableModel*,int)),this,SLOT(gotoeditadddialog(AddressTableModel*,int)));
    connect(AddBookPage,SIGNAL(backSend()),this,SLOT(gotoWalletViewPage()));
    connect(AddBookPage,SIGNAL(selectaddressok(QString)),this,SLOT(gotoWalletViewPagea(QString)));
    connect(AddBookPage,SIGNAL(selectaddressyes(QString,QString)),this,SLOT(gotoWalletViewPaga(QString,QString)));


}
void WalletView::gotoAddBookPage(AddressTableModel *atm,int tag)
{
    AddBookPage = new AddBookWidget(this);

    if(5== tag)
    {
        AddBookPage->settag(5);
        setdialogPage->setAddpageshow(AddBookPage);
    }
    else
    {
        AddBookPage->settag(2);
        addWidget(AddBookPage);
        setCurrentWidget(AddBookPage);

    }

    AddBookPage->setModel(atm);
    connect(AddBookPage,SIGNAL(openeditadddialog(AddressTableModel*,int)),this,SLOT(gotoeditadddialog(AddressTableModel*,int)));
    connect(AddBookPage,SIGNAL(backSend()),this,SLOT(gotoWalletViewPage()));
    connect(AddBookPage,SIGNAL(selectaddressok(QString)),this,SLOT(gotoWalletViewPagea(QString)));
    connect(AddBookPage,SIGNAL(selectaddressyes(QString,QString)),this,SLOT(gotoWalletViewPaga(QString,QString)));

}

void WalletView::gotoeditadddialog(AddressTableModel *atm,int tag)
{

    editaddpage = new editadddialog();
    editaddpage->setModel(atm);

    //20170809
    QString newAddressToSelect;
    newAddressToSelect = editaddpage->getAddress();

    //20170809
    connect(editaddpage,SIGNAL(sendDataList(QList<QString>*,int)),this,SLOT(ReceiveData(QList<QString>*,int)));

    if(2 == tag)
    {
        editaddpage->settag(2);
        addWidget(editaddpage);
        setCurrentWidget(editaddpage);
    }
    else
    {
        editaddpage->settag(5);
        setdialogPage->setAddeditpageshow(editaddpage);
    }

}

void WalletView::ReceiveData(QList<QString> *inputDataList,int tag)
{

    AddBookPage->AddEditedRow(inputDataList->at(0),inputDataList->at(1));
    if(2==tag)
    {
        setCurrentWidget(AddBookPage);

    }
    else
    {
        setdialogPage->backAddpageshow(AddBookPage);
    }


}
void WalletView::gotoWalletViewPage()
{

    // walletStackBranchPage->setCurrentWidget(pwalletStackwidget);
    sendCoinsPage->clearInfo();
    setCurrentWidget(sendCoinsPage);
}

void WalletView::gotoWalletViewPagea(QString e)
{

    if(5 == AddBookPage->gettag())
    {
        return;
    }

    gotoSendCoinsPage(e);

}
void WalletView::gotoWalletViewPaga(QString a,QString b)
{

    if(5 == AddBookPage->gettag())
    {
        return;
    }

    gotoSendCoinsPage(a,b);

}
void WalletView::gotoPasswordSetwidgetPage(int tag)
{
    bool was_locked = this->walletModel->CheckIsCrypted();
    PasswordSettingWidget::Mode status ;
    if(was_locked)
    {
        status = PasswordSettingWidget::ChangePass;
    }
    else
    {
        status =PasswordSettingWidget::Encrypt;
    }
    PasswordSettingPage = new PasswordSettingWidget(status,this);
    // walletStackBranchPage->addWidget(PasswordSettingPage);
    connect(PasswordSettingPage,SIGNAL(back()),this,SLOT(gotoWalletViewPage()));
    connect(PasswordSettingPage,SIGNAL(openSendCoinsAffrimwidget()),this,SLOT(gotoSendCoinsAffrimPage()));
    connect(PasswordSettingPage,SIGNAL(ChangePasswordSuccess()),this,SLOT(gotoPsdSetSuccessPage()));


    // walletStackBranchPage->setCurrentWidget(PasswordSettingPage);
    PasswordSettingPage->setMode(status);
    PasswordSettingPage->setModel(this->walletModel);//walletmd);//20170824 before
    this->gotopsdsetpage(PasswordSettingPage,tag);

}


void WalletView::gotoSendaffrimPage(QString a,QString b,QString label,int c)
{
    bool was_locked = this->walletModel->getEncryptionStatus() == WalletModel::Locked;

    SendCoinsAffrimPage = new SendCoinsAffrimWidget(was_locked,this);
    SendCoinsAffrimPage->setModel(this->walletModel);
    SendCoinsAffrimPage->setClientModel(this->clientModel);//20170823
    //walletStackBranchPage->addWidget(SendCoinsAffrimPage);
    connect(SendCoinsAffrimPage,SIGNAL(back()),this,SLOT(gotoWalletViewPage()));
    connect(SendCoinsAffrimPage,SIGNAL(goresultpage()),this,SLOT(gotoResultPage()),Qt::DirectConnection);
    connect(SendCoinsAffrimPage,SIGNAL(gobacktosendpage()),this,SLOT(backtoSendCoinsPage()));

    addWidget(SendCoinsAffrimPage);
    setCurrentWidget(SendCoinsAffrimPage);
    QString add = a;
    QString coin = b;
    int tag = c;
    SendCoinsAffrimPage->setMessage(add,coin,label,tag);

}
void WalletView::gotoSendCoinsAffrimPage()
{

    //  bool was_locked = this->walletModel->getEncryptionStatus() == WalletModel::Unencrypted;
    bool was_locked = this->walletModel->getEncryptionStatus() == WalletModel::Locked;

    SendCoinsAffrimPage = new SendCoinsAffrimWidget(was_locked,this);
    SendCoinsAffrimPage->setModel(this->walletModel);
    SendCoinsAffrimPage->setClientModel(this->clientModel);//20170823
    connect(SendCoinsAffrimPage,SIGNAL(back()),this,SLOT(gotoWalletViewPage()));
    connect(SendCoinsAffrimPage,SIGNAL(goresultpage()),this,SLOT(gotoResultPage()),Qt::DirectConnection);
    connect(SendCoinsAffrimPage,SIGNAL(gobacktosendpage()),this,SLOT(backtoSendCoinsPage()));

    addWidget(SendCoinsAffrimPage);
    setCurrentWidget(SendCoinsAffrimPage);
    QString a = sendinfo_.Add_;
    QString b = sendinfo_.Coin_;
    QString label = sendinfo_.Label_;
    int c = sendinfo_.Status_;
    SendCoinsAffrimPage->setMessage(a,b,label,c);

}
void WalletView::gotoPsdSetSuccessPage()
{

    setdialogPage->setPsdSetSuccess();

}
void WalletView::gotoSettingPage(QString a,QString b,QString label,int tag)
{
    SendInfo T;
    T.Add_ = a;
    T.Coin_ = b,
            T.Label_ = label;
    T.Status_ = tag;
    setvalue(T);
    setCurrentWidget(settingwidgetPage);
}

void WalletView::setvalue(SendInfo a)
{
    sendinfo_.Coin_ = a.Coin_;
    sendinfo_.Add_ = a.Add_;
    sendinfo_.Label_ = a.Label_;
    sendinfo_.Status_ = a.Status_;

}
SendInfo WalletView::getvalue()
{
    return sendinfo_;
}
void WalletView::setBitcoinGUI(BitcoinGUI *gui)
{
    if (gui)
    {
        // Clicking on a transaction on the overview page simply sends you to transaction history page
        connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), gui, SLOT(gotoHistoryPage()));

        // Receive and report messages
        connect(this, SIGNAL(message(QString,QString,unsigned int)), gui, SLOT(message(QString,QString,unsigned int)));

        // Pass through encryption status changed signals
        connect(this, SIGNAL(encryptionStatusChanged(int)), gui, SLOT(setEncryptionStatus(int)));

        // Pass through transaction notifications
      //  connect(this, SIGNAL(incomingTransaction(QString,int,CAmount,QString,QString,QString,QString,QString)), gui, SLOT(incomingTransaction(QString,int,CAmount,QString,QString,QString,QString,QString)));

        // Connect HD enabled state signal
        connect(this, SIGNAL(hdEnabledStatusChanged(int)), gui, SLOT(setHDStatus(int)));
    }
}

void WalletView::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    overviewPage->setClientModel(_clientModel);
    sendCoinsPage->setClientModel(_clientModel);
    infopage->setClientModel(_clientModel);
}

void WalletView::setWalletModel(WalletModel *_walletModel)
{
    this->walletModel = _walletModel;

    // Put transaction list in tabs
    transactionView->setModel(_walletModel);
    overviewPage->setWalletModel(_walletModel);
    receiveCoinsPage->setModel(_walletModel);
    sendCoinsPage->setModel(_walletModel);
    usedReceivingAddressesPage->setModel(_walletModel->getAddressTableModel());
    usedSendingAddressesPage->setModel(_walletModel->getAddressTableModel());
    ipcdialogPage->setModel(_walletModel);
    setdialogPage->setModel(_walletModel);
    pTallySelectAddressPage->setWalletModel(_walletModel);
    pTallyApply->setModel(_walletModel);
    pTallyAccount->setModel(_walletModel);
    pTallyOutAccount->setModel(_walletModel);

    ecoinDialogPage->setModel(_walletModel);
    if (_walletModel)
    {
        // Receive and pass through messages from wallet model
        connect(_walletModel, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

        // Handle changes in encryption status
        connect(_walletModel, SIGNAL(encryptionStatusChanged(int)), this, SIGNAL(encryptionStatusChanged(int)));
        updateEncryptionStatus();

        // update HD status
        Q_EMIT hdEnabledStatusChanged(_walletModel->hdEnabled());

        // Balloon pop-up for new transaction
        connect(_walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(processNewTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(_walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));
        //201708123
        // Show progress dialog
        connect(_walletModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));
    }
}

void WalletView::processNewTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if (!walletModel || !clientModel || clientModel->inInitialBlockDownload())
        return;

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    if (!ttm || ttm->processingQueuedTransactions())
        return;

    QString date = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();
    QModelIndex index = ttm->index(start, 0, parent);
    QString address = ttm->data(index, TransactionTableModel::AddressRole).toString();
    QString label = ttm->data(index, TransactionTableModel::LabelRole).toString();

    QString ipctitle = ttm->index(start,TransactionTableModel::IPCTitle,parent).data().toString();
    QString ipctype = ttm->index(start,TransactionTableModel::IPCType,parent).data().toString();

   // Q_EMIT incomingTransaction(date, walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address, label,ipctitle,ipctype);
    //    Q_EMIT incomingTransaction(date, walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address, label);
}


void WalletView::opensendpage(sendhistory *sendpage)
{
    overviewPage->addsendpage(sendpage);
}


void WalletView::openrecvpage(RecvHistory *recvpage)
{
    overviewPage->addrecv(recvpage);
}
void WalletView::opensendipcpage(sendipchistory *sendipcpage)
{
    overviewPage->addsendipcpage(sendipcpage);

}
void WalletView::openrecvipcpage(recvipchistory *recvipcpage)
{
    overviewPage->addrecvipcpage(recvipcpage);
}


void WalletView::opensendtokenpage(SendTokenHistory *sendTokenPage)
{
    overviewPage->addsendtokenpage(sendTokenPage);
}
void WalletView::openrecvtokenpage(RecvTokenHistory *recvTokenPage)
{
    overviewPage->addrecvtokenpage(recvTokenPage);
}

void WalletView::gotoOverviewPage()
{
    setCurrentWidget(overviewPage);
}

void WalletView::gotoHistoryPage()
{
    setCurrentWidget(transactionsPage);
}

void WalletView::gotoReceiveCoinsPage()
{
    setCurrentWidget(receiveCoinsPage);
}

void WalletView::gotopsdsetpage(PasswordSettingWidget  *PasswordSettingPage,int tag)
{
    if(5==tag)
    {
        PasswordSettingPage->setTag(5);
        setdialogPage->setPsdshow(PasswordSettingPage);
    }
    else
    {
        PasswordSettingPage->setTag(2);
        addWidget(PasswordSettingPage);
        setCurrentWidget(PasswordSettingPage);

    }
}
void WalletView::gotoResultPage()
{
    sendresultpage =new SendResultWidget(this);
    addWidget(sendresultpage);
    connect(sendresultpage,SIGNAL(backmain()),this,SLOT(gotoWalletViewPage()));

    setCurrentWidget(sendresultpage);

}

void WalletView::gotoIpcPage()
{

    setCurrentWidget(ipcdialogPage);
}

void WalletView::gotoSetPage()
{
    setCurrentWidget(setdialogPage);

    Q_EMIT openadd();
}
void WalletView::gotoTallyPage()
{

    if(!CDpocInfo::Instance().IsHasLocalAccount())//jizhang ing
    {
        LOG_WRITE(LOG_INFO,"CDpocInfo::Instance().IsHasLocalAccount false ");
        tallyDialogPageStack->setCurrentWidget(pTallyDscribe);
    }
    else
    {
        LOG_WRITE(LOG_INFO,"CDpocInfo::Instance().IsHasLocalAccount true ");
        pTallyAccount->resettime();
        tallyDialogPageStack->setCurrentWidget(pTallyAccount);
    }
    setCurrentWidget(tallyDialogPage);
}
void WalletView::gotoeCoinPage(){
    setCurrentWidget(ecoinDialogPage);

}

void WalletView::backtoSendCoinsPage()
{
    setCurrentWidget(sendCoinsPage);

    //  sendCoinsPage->clearInfo();

}
void WalletView::gotoSendCoinsPage(QString addr,QString label)
{

    setCurrentWidget(sendCoinsPage);

    if (!addr.isEmpty())
    {
        sendCoinsPage->setAddress(addr,label);
    }
    else
    {

        sendCoinsPage->clearInfo();
    }
}

void WalletView::gotoSignMessageTab(QString addr)
{
    // calls show() in showTab_SM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_SM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void WalletView::gotoVerifyMessageTab(QString addr)
{
    // calls show() in showTab_VM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_VM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

bool WalletView::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    // return sendCoinsPage->handlePaymentRequest(recipient);
}

void WalletView::showOutOfSyncWarning(bool fShow)
{
   // overviewPage->showOutOfSyncWarning(fShow);
}

void WalletView::updateEncryptionStatus()
{
    Q_EMIT encryptionStatusChanged(walletModel->getEncryptionStatus());
}

void WalletView::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt : AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    updateEncryptionStatus();
}

void WalletView::backupWallet()
{
    QString filename = GUIUtil::getSaveFileName(this,
                                                tr("Backup Wallet"), QString(),
                                                tr("Wallet Data (*.dat)"), NULL);

    if (filename.isEmpty())
        return;

    if (!walletModel->backupWallet(filename)) {
        Q_EMIT message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
                       CClientUIInterface::MSG_ERROR);
    }
    else {
        Q_EMIT message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
                       CClientUIInterface::MSG_INFORMATION);
    }
}

void WalletView::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void WalletView::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        SecureString oldpass;
        oldpass.assign("1");

        if(!walletModel->setWalletLocked(false, oldpass))
        {
            QMessageBox::critical(this, tr("Wallet unlock failed"),
                                  tr("The passphrase entered for the wallet decryption was incorrect."));
        }
        else
        {
            // QDialog::accept(); // Success
        }

    }
}

void WalletView::usedSendingAddresses()
{
    if(!walletModel)
        return;

    usedSendingAddressesPage->show();
    usedSendingAddressesPage->raise();
    usedSendingAddressesPage->activateWindow();
}

void WalletView::usedReceivingAddresses()
{
    if(!walletModel)
        return;

    usedReceivingAddressesPage->show();
    usedReceivingAddressesPage->raise();
    usedReceivingAddressesPage->activateWindow();
}

void WalletView::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(title, "", 0, 100);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    }
    else if (nProgress == 100)
    {
        if (progressDialog)
        {
            progressDialog->close();
            progressDialog->deleteLater();
        }
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);
}

void WalletView::requestedSyncWarningInfo()
{
    Q_EMIT outOfSyncWarningClicked();
}
void WalletView::gotoClausePage()
{
    tallyDialogPageStack->setCurrentWidget(pTallyClause);
}
void WalletView::gotoDscribePage()
{
    tallyDialogPageStack->setCurrentWidget(pTallyDscribe);
}

void WalletView::gotoTallyApplyPage(QString address)
{
    pTallyApply->resetinfo();
    if(!address.isEmpty()){
        pTallyApply->setAddress(address);
    }
    tallyDialogPageStack->setCurrentWidget(pTallyApply);
}
void WalletView::gotoTallyAccountPage(QString add,CAmount num)
{
    tallyDialogPageStack->setCurrentWidget(pTallyAccount);
    WalletModel::keepupaccountInfo info;
    info.Add_ =add;
    info.Coin_ = num;

    pTallyAccount->setinfo(info);
}
void WalletView::setIsTallying(bool IsTally)
{
    m_IsTallying= IsTally;
}
bool WalletView::getIsTallying()
{
    return m_IsTallying;
}
void WalletView::gotoTallyOutAccountPage(QString add,CAmount num)
{
    tallyDialogPageStack->setCurrentWidget(pTallyOutAccount);
    WalletModel::keepupaccountInfo accinfo;
    accinfo.Add_ =add;
    accinfo.Coin_ = num;
    pTallyOutAccount->setinfo(accinfo);
}
void WalletView::gotoBackTallyAccountPage(bool isok)
{
    if(isok)
        pTallyAccount->setfinishinfo();
    tallyDialogPageStack->setCurrentWidget(pTallyAccount);
}
void WalletView::gotoTallyAddressPage()
{
    tallyDialogPageStack->setCurrentWidget(pTallySelectAddressPage);
}
