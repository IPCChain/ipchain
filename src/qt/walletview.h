// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_WALLETVIEW_H
#define BITCOIN_QT_WALLETVIEW_H

#include "amount.h"

#include <QStackedWidget>

class BitcoinGUI;
class ClientModel;
class OverviewPage;
class PlatformStyle;
class ReceiveCoinsDialog;
class SendCoinsDialog;
class SendCoinsRecipient;
class TransactionView;
class WalletModel;
class AddressBookPage;
class ipcdialog;
class setdialog;
class ECoinDialog;

class SendConfirmationDialog;
class PasswordSettingWidget;
class SendCoinsAffrimWidget;

class InfoWidget;
class sendhistory;
class RecvHistory;
class sendipchistory;
class recvipchistory;
class SendTokenHistory;
class RecvTokenHistory;

class TallyAccount;
class TallyApply;
class TallyClause;
class TallyDscribe;
class TallyOutAccount;
class  ipcSelectAddress;
class AddressTableModel;
class AddBookWidget;
class editadddialog;
class SendResultWidget;
#include <QDateTime>
QT_BEGIN_NAMESPACE
class QModelIndex;
class QProgressDialog;
class Settingwidget;
QT_END_NAMESPACE

/*
  WalletView class. This class represents the view to a single wallet.
  It was added to support multiple wallet functionality. Each wallet gets its own WalletView instance.
  It communicates with both the client and the wallet models to give the user an up-to-date view of the
  current core state.
*/
struct SendInfo{
    QString Coin_;
    QString Add_;
    QString Label_;
    int Status_;

};


class WalletView : public QStackedWidget
{
    Q_OBJECT
    
public:
    explicit WalletView(const PlatformStyle *platformStyle, QWidget *parent);
    ~WalletView();
    void gotopsdsetpage(PasswordSettingWidget  *PasswordSettingPage,int tag);



    void setvalue(SendInfo info);
    SendInfo getvalue();
    
    void setBitcoinGUI(BitcoinGUI *gui);
    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel);
    /** Set the wallet model.
        The wallet model represents a bitcoin wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    void setWalletModel(WalletModel *walletModel);
    
    bool handlePaymentRequest(const SendCoinsRecipient& recipient);
    
    void showOutOfSyncWarning(bool fShow);
    void opensendpage(sendhistory *sendpage);
    void openrecvpage(RecvHistory *recvpage);
    void opensendipcpage(sendipchistory *sendipcpage);
    void openrecvipcpage(recvipchistory *recvipcpage);
    void opensendtokenpage(SendTokenHistory *sendTokenPage);
    void openrecvtokenpage(RecvTokenHistory *recvTokenPage);


    void setIsTallying(bool IsTally);
    bool getIsTallying();

    
private:
    SendInfo sendinfo_;
    ClientModel *clientModel;
    WalletModel *walletModel;
    PasswordSettingWidget* PasswordSettingPage;
    SendCoinsAffrimWidget* SendCoinsAffrimPage;
    AddBookWidget*  AddBookPage ;
    
    OverviewPage *overviewPage;
    QWidget *transactionsPage;

    InfoWidget *infopage;
    ReceiveCoinsDialog *receiveCoinsPage;
    SendCoinsDialog *sendCoinsPage;
    ipcdialog  *ipcdialogPage;
    setdialog  *setdialogPage;
    ECoinDialog  *ecoinDialogPage;

    QWidget *   tallyDialogPage;
    QStackedWidget *tallyDialogPageStack;

    SendResultWidget* sendresultpage ;
    AddressBookPage *usedSendingAddressesPage;
    AddressBookPage *usedReceivingAddressesPage;
    editadddialog *editaddpage;
    TransactionView *transactionView;
    TallyAccount *pTallyAccount;
    TallyApply *pTallyApply;
    TallyClause *pTallyClause;
    TallyDscribe *pTallyDscribe;
    TallyOutAccount *pTallyOutAccount;
    ipcSelectAddress  *pTallySelectAddressPage;
    
    Settingwidget* settingwidgetPage;
    QProgressDialog *progressDialog;
    const PlatformStyle *platformStyle;
    bool m_IsTallying;
    
public Q_SLOTS:
    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "",QString label="");

    void backtoSendCoinsPage();
    /** Switch to ipc page */
    void gotoIpcPage();
    /** Switch to set page */
    void gotoSetPage();
    void gotoResultPage();
    void gotoTallyPage();
    void gotoeCoinPage();
    void gotoClausePage();
    void gotoDscribePage();
    void gotoTallyApplyPage(QString address= "");
    void gotoTallyAccountPage(QString add,CAmount num);
    void gotoTallyOutAccountPage(QString add,CAmount num);
    void gotoBackTallyAccountPage(bool);
    void gotoTallyAddressPage();
    void gotoSendaffrimPage(QString a,QString b,QString label,int tag);
    void gotoSettingPage(QString a,QString b,QString label,int tag);
    void gotoPasswordSetwidgetPage(int tag);
    void gotoSendCoinsAffrimPage();

    void gotoPsdSetSuccessPage();
    void gotoAddBookPage(AddressTableModel *atm,int tag);
    void openAddBookPage();
    void gotoWalletViewPage();
    void gotoeditadddialog(AddressTableModel *atm,int tag);
    void ReceiveData(QList<QString> *inputDataList,int tag);
    void gotoWalletViewPagea(QString e);
    void gotoWalletViewPaga(QString e,QString f);
    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");
    
    /** Show incoming transaction notification for new transactions.
      
        The new items are those between start and end inclusive, under the given parent item.
    */
    void processNewTransaction(const QModelIndex& parent, int start, int /*end*/);
    /** Encrypt the wallet */
    void encryptWallet(bool status);
    /** Backup the wallet */
    void backupWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */
    void unlockWallet();
    
    /** Show used sending addresses */
    void usedSendingAddresses();
    /** Show used receiving addresses */
    void usedReceivingAddresses();
    
    /** Re-emit encryption status signal */
    void updateEncryptionStatus();
    
    /** Show progress dialog e.g. for rescan */
    void showProgress(const QString &title, int nProgress);
    
    /** User has requested more information about the out of sync state */
    void requestedSyncWarningInfo();
    
Q_SIGNALS:
    void openadd();
    /** Signal that we want to show the main window */
    void showNormalIfMinimized();
    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);
    /** Encryption status of wallet changed */
    void encryptionStatusChanged(int status);
    /** HD-Enabled status of wallet changed (only possible during startup) */
    void hdEnabledStatusChanged(int hdEnabled);
    /** Notify that a new transaction appeared */
    void incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label,const QString& ipctitle,const QString& ipctype);
    /** Notify that the out of sync warning icon has been pressed */
    void outOfSyncWarningClicked();
};

#endif // BITCOIN_QT_WALLETVIEW_H
