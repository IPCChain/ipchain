// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletmodel.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "consensus/validation.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "paymentserver.h"
#include "recentrequeststablemodel.h"
#include "transactiontablemodel.h"
#include "cmessagebox.h"
#include "base58.h"
#include "keystore.h"
#include "validation.h"
#include "net.h" // for g_connman
#include "sync.h"
#include "ui_interface.h"
#include "util.h" // for GetBoolArg
#include "wallet/wallet.h"
#include "wallet/walletdb.h" // for BackupWallet
#include "wallet/coincontrol.h"
#include <stdint.h>
#include "log/log.h"
#include <QDebug>
#include <QSet>
#include <QTimer>
#include <QStringList>
#include <boost/foreach.hpp>
#include "walletpassword.h"
#include <QMessageBox>
#include "dpoc/ConsensusAccountPool.h"
#include "json/cJSON.h"
#include <QLocale>
#include "dpoc/TimeService.h"
#include <QSettings>
#include <sys/time.h>
#include<algorithm>
#include <QFileDialog>
#include <QCoreApplication>
#include <iostream>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
boost::mutex mutex_list;
#define LOCK_LIST(mutex) boost::lock_guard<boost::mutex> lock(mutex)

extern bool isfullloaded;
extern CAmount _balance;
WalletModel::WalletModel(const PlatformStyle *platformStyle, CWallet *_wallet, OptionsModel *_optionsModel, QObject *parent) :
    QObject(parent), wallet(_wallet), optionsModel(_optionsModel), addressTableModel(0),
    transactionTableModel(0),
    recentRequestsTableModel(0),
    cachedBalance(0), cachedUnconfirmedBalance(0), cachedImmatureBalance(0),
    cachedEncryptionStatus(Unencrypted),ipcSelectFromList(0),m_bFinishedLoading(false),
    cachedNumBlocks(0),m_WalletModelTransactionIpcRegister(NULL),m_WalletModelTransactioneCoinRegister(NULL),
    m_WalletModelTransactionIpcTransfer(NULL),m_WalletModelTransactioneCoinSend1(NULL),currentTransaction1(NULL)
{
    fHaveWatchOnly = wallet->HaveWatchOnly();
    fForceCheckBalanceChanged = false;

    addressTableModel = new AddressTableModel(wallet, this);
    transactionTableModel = new TransactionTableModel(platformStyle, wallet, this);
    recentRequestsTableModel = new RecentRequestsTableModel(wallet, this);

    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollBalanceChanged()));
    pollTimer->start(MODEL_UPDATE_DELAY);
    subscribeToCoreSignals();
}

WalletModel::~WalletModel()
{
    unsubscribeFromCoreSignals();
}

CAmount WalletModel::getBalance(const CCoinControl *coinControl) const
{
    if (coinControl)
    {
        CAmount nBalance = 0;
        std::vector<COutput> vCoins;
        wallet->AvailableCoins(vCoins, true, coinControl);
        BOOST_FOREACH(const COutput& out, vCoins)
                if(out.fSpendable)
                nBalance += out.tx->tx->vout[out.i].nValue;

        return nBalance;
    }

    return wallet->GetBalance();
}

CAmount WalletModel::getUnconfirmedBalance() const
{
    return wallet->GetUnconfirmedBalance();
}

CAmount WalletModel::getImmatureBalance() const
{
    return wallet->GetImmatureBalance();
}

bool WalletModel::haveWatchOnly() const
{
    return fHaveWatchOnly;
}

CAmount WalletModel::getWatchBalance() const
{
    return wallet->GetWatchOnlyBalance();
}
CAmount WalletModel::GetDetained() const
{
    return wallet->GetDeposit();
}

int WalletModel::GetDepth() const
{
    return wallet->GetDepthofJoinTX();
}
CAmount WalletModel::getWatchUnconfirmedBalance() const
{
    return wallet->GetUnconfirmedWatchOnlyBalance();
}

CAmount WalletModel::getWatchImmatureBalance() const
{
    return wallet->GetImmatureWatchOnlyBalance();
}

void WalletModel::updateStatus()
{
    EncryptionStatus newEncryptionStatus = getEncryptionStatus();

    if(cachedEncryptionStatus != newEncryptionStatus)
        Q_EMIT encryptionStatusChanged(newEncryptionStatus);
}
void WalletModel::pollBalanceChanged()
{
    if(fForceCheckBalanceChanged || chainActive.Height() != cachedNumBlocks)
    {
        fForceCheckBalanceChanged = false;
        cachedNumBlocks = chainActive.Height();
        if(true == isfullloaded)
        {
            checkBalanceChanged();
        }
        if(transactionTableModel)
        {
            transactionTableModel->updateConfirmations();
        }
    }
}

void WalletModel::checkBalanceChanged()
{
    if(!m_bFinishedLoading)
        return;
    CAmount newBalance = getBalance();
    CAmount newUnconfirmedBalance = getUnconfirmedBalance();
    CAmount newImmatureBalance = getImmatureBalance();
    CAmount newDepositBalance = GetDetained();
    CAmount newWatchOnlyBalance = 0;
    CAmount newWatchUnconfBalance = 0;
    CAmount newWatchImmatureBalance = 0;
    if (haveWatchOnly())
    {
        newWatchOnlyBalance = getWatchBalance();
        newWatchUnconfBalance = getWatchUnconfirmedBalance();
        newWatchImmatureBalance = getWatchImmatureBalance();
    }

    if(cachedBalance != newBalance || cachedUnconfirmedBalance != newUnconfirmedBalance || cachedImmatureBalance != newImmatureBalance ||
            cachedWatchOnlyBalance != newWatchOnlyBalance || cachedWatchUnconfBalance != newWatchUnconfBalance || cachedWatchImmatureBalance != newWatchImmatureBalance  ||  cachednewDepositBalance != newDepositBalance)
    {
        cachedBalance = newBalance;
        cachedUnconfirmedBalance = newUnconfirmedBalance;
        cachedImmatureBalance = newImmatureBalance;
        cachedWatchOnlyBalance = newWatchOnlyBalance;
        cachedWatchUnconfBalance = newWatchUnconfBalance;
        cachedWatchImmatureBalance = newWatchImmatureBalance;
        Q_EMIT balanceChanged(newBalance, newUnconfirmedBalance, newImmatureBalance,
                              newWatchOnlyBalance, newWatchUnconfBalance, newWatchImmatureBalance,newDepositBalance);

    }
}

void WalletModel::updateTransaction()
{
    fForceCheckBalanceChanged = true;
    if(m_bFinishedLoading)
        Q_EMIT updateIpcList();
}

void WalletModel::updateAddressBook(const QString &address, const QString &label,
                                    bool isMine, const QString &purpose, int status)
{
    if(addressTableModel)
        addressTableModel->updateEntry(address, label, isMine, purpose, status);
}

void WalletModel::updateWatchOnlyFlag(bool fHaveWatchonly)
{
    fHaveWatchOnly = fHaveWatchonly;
    Q_EMIT notifyWatchonlyChanged(fHaveWatchonly);
}

bool WalletModel::validateAddress(const QString &address)
{
    CBitcoinAddress addressParsed(address.toStdString());
    return addressParsed.IsValid();
}

WalletModel::SendCoinsReturn WalletModel::prepareExitBookkeeping(const CCoinControl *coinControl,std::string &error)
{
    CAmount nFeeRequired = 0;
    int nChangePosRet = -1;
    std::string strFailReason;
    CWalletTx newTx;
    CReserveKey keyChange(wallet);
    bool fCreated = wallet->ExitCampaign( newTx, keyChange,  nFeeRequired,
                                          nChangePosRet, strFailReason, coinControl,true);
    if(!fCreated)
    {
        error = strFailReason;
        LOG_WRITE(LOG_INFO,"ExitCampaign",strFailReason.c_str());
        return TransactionCreationFailed;
    }
    else
    {
        CValidationState state;
        std::cout<<"CommitJoinCampaignTransaction"<<std::endl;
        if(!wallet->CommitTransaction(newTx, keyChange,  g_connman.get(), state))
        {
            std::cout<<"CommitJoinCampaignTransaction faild"<<std::endl;
            ipcSendIsValidState = state.IsValid();
            error = state.GetRejectReason();
            LOG_WRITE(LOG_INFO,"CommitTransaction",(state.GetRejectReason()).c_str());
            return SendCoinsReturn(TransactionCommitFailed, QString::fromStdString(state.GetRejectReason()));
        }
        else
        {
            ipcSendIsValidState = state.IsValid();
            LOG_WRITE(LOG_INFO,"CommitTransaction OK");
        }
    }


    return SendCoinsReturn(OK);
}
WalletModel::SendCoinsReturn WalletModel::ExitBookkeeping(WalletModelTransaction &transaction)
{
    QByteArray transaction_array;
    LOCK2(cs_main, wallet->cs_wallet);
    CWalletTx *newTx = transaction.getTransaction();
    Q_FOREACH(const SendCoinsRecipient &rcp, transaction.getRecipients())
    {
        if (rcp.paymentRequest.IsInitialized())
        {
            // Make sure any payment requests involved are still valid.
            if (PaymentServer::verifyExpired(rcp.paymentRequest.getDetails())) {
                return PaymentRequestExpired;
            }

            std::string key("PaymentRequest");
            std::string value;
            rcp.paymentRequest.SerializeToString(&value);
            newTx->vOrderForm.push_back(make_pair(key, value));
        }
        else if (!rcp.message.isEmpty()) // Message from normal bitcoin:URI (bitcoin:123...?message=example)
            newTx->vOrderForm.push_back(make_pair("Message", rcp.message.toStdString()));
    }

    CReserveKey *keyChange = transaction.getPossibleKeyChange();
    CValidationState state;

    std::cout<<"CommitJoinCampaignTransaction"<<std::endl;
    if(!wallet->CommitTransaction(*newTx, *keyChange,  g_connman.get(), state))
    {
        std::cout<<"CommitJoinCampaignTransaction faild"<<std::endl;
        ipcSendIsValidState = state.IsValid();
        LOG_WRITE(LOG_INFO,"CommitTransaction",(state.GetRejectReason()).c_str());
        return SendCoinsReturn(TransactionCommitFailed, QString::fromStdString(state.GetRejectReason()));
    }
    else
    {
        ipcSendIsValidState = state.IsValid();
        std::cout<<"CommitJoinCampaignTransaction OK"<<std::endl;
    }
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << *newTx->tx;
    transaction_array.append(&(ssTx[0]), ssTx.size());
    Q_FOREACH(const SendCoinsRecipient &rcp, transaction.getRecipients())
    {
        if (!rcp.paymentRequest.IsInitialized())
        {
            std::string strAddress = rcp.address.toStdString();
            CTxDestination dest = CBitcoinAddress(strAddress).Get();
            std::string strLabel = rcp.label.toStdString();
            {
                LOCK(wallet->cs_wallet);
                std::map<CTxDestination, CAddressBookData>::iterator mi = wallet->mapAddressBook.find(dest);
                // Check if we have a new address or an updated label
                if (mi == wallet->mapAddressBook.end())
                {
                    wallet->SetAddressBook(dest, strLabel, "send");
                }
                else if (mi->second.name != strLabel)
                {
                    wallet->SetAddressBook(dest, strLabel, ""); // "" means don't change purpose
                }

            }
        }

        Q_EMIT coinsSent(wallet, rcp, transaction_array);
    }
    checkBalanceChanged(); // update balance immediately, otherwise there could be a short noticeable delay until pollBalanceChanged hits
    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::prepareeCoinsSendCreateTransaction(std::string& Txid,int Index,uint64_t eCount,WalletModelTransaction &transaction,std::string &error, const CCoinControl *coinControl)
{
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if(recipients.empty())
    {
        return OK;
    }

    QSet<QString> setAddress;
    int nAddresses = 0;

    Q_FOREACH(const SendCoinsRecipient &rcp, recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
                CAmount nAmount = out.amount();
                //CRecipient recipient = {scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount};
                CRecipient recipient = {scriptPubKey, 0, rcp.fSubtractFeeFromAmount};

                vecSend.push_back(recipient);
            }
            if (subtotal <= 0)
            {
                return InvalidAmount;
            }
            total += subtotal;
        }
        else
        {   // User-entered bitcoin address / amount:
            if(!validateAddress(rcp.address))
            {
                return InvalidAddress;
            }
            if(rcp.amount < 0)
            {
                return InvalidAmount;
            }
            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
            CRecipient recipient = {scriptPubKey, 0, rcp.fSubtractFeeFromAmount};

            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }
    if(setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    CAmount nBalance = getBalance(coinControl);

    if(total > nBalance)
    {

        return AmountExceedsBalance;
    }

    {
        LOCK2(cs_main, wallet->cs_wallet);

        transaction.newPossibleKeyChange(wallet);

        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;

        CWalletTx *newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();
        std::cout<<Txid<<" "<<eCount<<std::endl;
        LOG_WRITE(LOG_INFO,"CreateTokenTransaction ",\
                  Txid.c_str(),\
                  QString::number(eCount).toStdString().c_str());
        bool fCreated = wallet->CreateTokenTransaction(Txid,eCount, vecSend,
                                                       *newTx,
                                                       *keyChange,
                                                       nFeeRequired,
                                                       nChangePosRet,
                                                       strFailReason,
                                                       coinControl,
                                                       true);

        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
            transaction.reassignAmounts(nChangePosRet);

        if(!fCreated)
        {

            if(fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            error = strFailReason;
            return TransactionCreationFailed;
        }
    }
    return SendCoinsReturn(OK);
}



QString WalletModel::processeCoinsCreateReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn)
{
    QPalette pa;
    pa.setColor(QPalette::WindowText,Qt::black);

    QString msg;

    switch(sendCoinsReturn.status)
    {
    case WalletModel::InvalidAddress:
    {
        msg ="The recipient address is not valid. Please recheck.";
    }
        break;
    case WalletModel::InvalidAmount:
    {
        msg ="The amount to pay must be larger than 0.";
    }
        break;
    case WalletModel::AmountExceedsBalance:
    {
        msg ="The amount exceeds your balance.";
    }
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        //msg = tr("The total exceeds your balance when the 0.001IPC/KB transaction fee is included.").arg(msgArg);
        break;
    case WalletModel::DuplicateAddress:
        msg ="Duplicate address found: addresses should only be used once each.";
        break;
    case WalletModel::TransactionCreationFailed:
        if(m_sendcoinerror!="")
            msg =m_sendcoinerror.c_str();
        else
            msg ="Transaction creation failed!";
        break;
    case WalletModel::TransactionCommitFailed:
    {
        if("bad-Token-tokensymbol-repeat" == sendCoinsReturn.reasonCommitFailed )
        {
            msg ="The transaction was rejected with the following reason: bad-Token-tokensymbol-repeat";

        }
        else if("bad-Token-tokenhash-repeat" == sendCoinsReturn.reasonCommitFailed )
        {
            msg ="The transaction was rejected with the following reason: bad-Token-tokenhash-repeat";

        }
        else if("bad-Token-Multi-inType" == sendCoinsReturn.reasonCommitFailed )
        {
            msg ="The transaction was rejected with the following reason: bad-Token-Multi-inType";

        }
        else if("bad-Token-Reg-issueDate(Regtime)" == sendCoinsReturn.reasonCommitFailed)
        {
            msg =tr("The transaction was rejected with the following reason: bad-Token-Reg-issueDate(Regtime)");

        }
        else if("bad-Token-regtotoken-value-unequal" == sendCoinsReturn.reasonCommitFailed )
        {
            msg =tr("The transaction was rejected with the following reason: bad-Token-regtotoken-value-unequal");
        }
        else if("bad-Token-Label-contain-errvalue"== sendCoinsReturn.reasonCommitFailed)
        {
            msg =tr("The transaction was rejected with the following reason: bad-Token-Label-contain-errvalue");
        }
        else if("bad-Token-Symbol-contain-errvalue"== sendCoinsReturn.reasonCommitFailed)
        {
            msg =tr("The transaction was rejected with the following reason: bad-Token-Symbol-contain-errvalue");
        }
        else if("bad-Token-value-unequal" == sendCoinsReturn.reasonCommitFailed )
        {
            msg =tr("The transaction was rejected with the following reason: bad-Token-value-unequal");
        }
        else if("bad-Token-Multi-outType" == sendCoinsReturn.reasonCommitFailed )
        {
            msg =tr("The transaction was rejected with the following reason: bad-Token-Multi-outType");
        }
        else
        {
            msg =tr("The transaction was rejected with the following reason: %1").arg(sendCoinsReturn.reasonCommitFailed);
        }
    }
        break;
    case WalletModel::AbsurdFee:
        msg =tr("A fee higher is considered an absurdly high fee.");
        break;
    case WalletModel::PaymentRequestExpired:
        msg =tr("Payment request expired.");
        break;
    case WalletModel::OK:
    default:
        return "";
    }
    return msg;
}
uint64_t WalletModel::getpow10(uint64_t oldnum,int n)
{
    while (n--) {
        oldnum = oldnum*10;
    }
    return oldnum;
}
WalletModel::SendCoinsReturn WalletModel::prepareeCoinsCreate(QString coin_name,QString coin_md5,QString coin_label,QString timestr,QString add1,QString &msg,int inttimr,int accuracy,uint64_t amount1)
{

    bool fNewRecipientAllowed;
    bool was_locked = this->getEncryptionStatus() == WalletModel::Locked;
    if(!this->CheckPassword())
    {
        // ui->tiplabel->setText(tr("password error"));
        msg = "password error";
        return WalletModel::PsdErr;
    }
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    bool valid = this->getEncryptionStatus() != WalletModel::Locked;
    WalletModel::UnlockContext ctx(this, valid, was_locked);
    if(!ctx.isValid())
    {
        fNewRecipientAllowed = true;
    }


    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient;


    if (recipient.paymentRequest.IsInitialized())
        return WalletModel::InitErr;

    recipient.address = add1;
    recipient.label = "";
    recipient.amount = 0;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = false;
    recipients.append(recipient);
    fNewRecipientAllowed = false;
    // prepare transaction for getting txFee earlier
    //    WalletModelTransaction currentTransaction(recipients);
    WalletModelTransaction* currentTransaction = new WalletModelTransaction(recipients);

    WalletModel::SendCoinsReturn prepareStatus;
    // Always use a CCoinControl instance, use the CoinControlDialog instance if CoinControl has been enabled
    CCoinControl ctrl;
    if (this->getOptionsModel()->getCoinControlFeatures())
    {
        // ctrl = *CoinControlDialog::coinControl;
    }
    else
    {
        ctrl.nConfirmTarget = 1;
    }

    QStringList data;

    data<<"\""+coin_name+"\"";
    data<<QString::number(0);
    data<<"\""+coin_md5+"\"";
    data<<"\""+coin_label+"\"";
    data<<QString::number(inttimr);
    data<<QString::number(getpow10(amount1,accuracy));
    data<<QString::number(accuracy);

    LOG_WRITE(LOG_INFO,"ecoinregedit",QString::number(getpow10(amount1,accuracy)).toStdString().c_str(),QString::number(amount1).toStdString().c_str(),QString::number(accuracy).toStdString().c_str());

    prepareStatus = this->prepareeCoinsCreateTransaction(data,add1,*currentTransaction, &ctrl);
    m_sendcoinerror =  this->m_sendcoinerror;

    msg =processeCoinsCreateReturn(prepareStatus);
    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;

        return prepareStatus.status;
    }

    else
    {
        if(m_WalletModelTransactioneCoinRegister)
            delete m_WalletModelTransactioneCoinRegister;
        m_WalletModelTransactioneCoinRegister = currentTransaction;

        return WalletModel::OK;
    }

}
WalletModel::SendCoinsReturn WalletModel::prepareeCoinsCreateTransaction(QStringList data,QString add1,WalletModelTransaction &transaction, const CCoinControl *coinControl)
{
    std::string strLabel = strprintf("{\"TokenSymbol\":%u,\"value\":%u,\"hash\":%u,\"label\":%u,\"issueDate\":%u,\"totalCount\":%u,\"txLabel\":\" \",\"accuracy\":%u}",\
                                     data.at(0).toStdString(),\
                                     data.at(1).toStdString(),\
                                     data.at(2).toStdString(),\
                                     data.at(3).toStdString(),\
                                     data.at(4).toStdString(),\
                                     data.at(5).toStdString(),\
                                     data.at(6).toStdString());

    LOG_WRITE(LOG_INFO,"prepareeCoinsCreateTransaction strLabel",\
              strLabel.c_str());
    m_sendcoinerror = "";
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if(recipients.empty())
    {
        return OK;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    Q_FOREACH(const SendCoinsRecipient &rcp, recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
                CAmount nAmount = out.amount();
                nAmount = 0;
                CRecipient recipient = {scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount};
                vecSend.push_back(recipient);
            }
            total += subtotal;
        }
        else
        {   // User-entered bitcoin address / amount:

            if(!validateAddress(rcp.address))
            {

                return InvalidAddress;
            }

            setAddress.insert(rcp.address);
            ++nAddresses;
            CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
            CRecipient recipient = {scriptPubKey, 0, rcp.fSubtractFeeFromAmount};
            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }
    if(setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    CAmount nBalance = getBalance(coinControl);

    if(total > nBalance)
    {
    }
    {
        LOCK2(cs_main, wallet->cs_wallet);
        transaction.newPossibleKeyChange(wallet);
        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;

        CWalletTx *newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();

        QString address = add1;
        CTxDestination strAddress = CBitcoinAddress(address.toStdString()).Get();
        bool fCreated = wallet->CreateTokenRegTransaction(
                    strLabel,
                    vecSend,
                    *newTx,
                    *keyChange,
                    nFeeRequired,
                    nChangePosRet,
                    strFailReason,
                    coinControl,
                    true
                    );
        std::cout<<"CreateTokenRegTransaction txhash: "<<newTx->tx->GetHash().ToString()<<std::endl;
        transaction.setTransactionFee(nFeeRequired);

        if (fSubtractFeeFromAmount && fCreated)
        {
            transaction.reassignAmounts(nChangePosRet);
        }
        if(!fCreated)
        {
            LOG_WRITE(LOG_INFO,"CreateTokenRegTransaction back",\
                      strFailReason.c_str());
            m_sendcoinerror = strFailReason;
            return TransactionCreationFailed;
        }
    }
    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::prepareBookkeepingTransaction(QString add1,CAmount amount1,std::string &error,\
                                                                        WalletModelTransaction &transaction, const CCoinControl *coinControl)
{
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if(recipients.empty())
    {
        return OK;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    Q_FOREACH(const SendCoinsRecipient &rcp, recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;

            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
                CAmount nAmount = out.amount();


                CRecipient recipient = {scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount};
                vecSend.push_back(recipient);
            }
            if (subtotal <= 0)
            {
                return InvalidAmount;
            }
            total += subtotal;
        }
        else
        {   // User-entered bitcoin address / amount:

            if(!validateAddress(rcp.address))
            {
                return InvalidAddress;
            }
            if(rcp.amount <= 0)
            {
                return InvalidAmount;
            }
            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
            CRecipient recipient = {scriptPubKey, rcp.amount, rcp.fSubtractFeeFromAmount};
            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }
    if(setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    CAmount nBalance = getBalance(coinControl);

    if(total > nBalance)
    {
        return AmountExceedsBalance;
    }

    {
        transaction.newPossibleKeyChange(wallet);

        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;

        CWalletTx *newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();

        QString address = add1;
        CTxDestination strAddress = CBitcoinAddress(address.toStdString()).Get();
        bool fCreated = wallet->JoinCampaign(strAddress,
                                             amount1,
                                             *newTx,
                                             *keyChange,
                                             nFeeRequired,
                                             nChangePosRet,
                                             strFailReason,
                                             coinControl,
                                             true);
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
            transaction.reassignAmounts(nChangePosRet);

        if(!fCreated)
        {

            if(fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            error = strFailReason;
            return TransactionCreationFailed;
        }
        if (nFeeRequired > maxTxFee)
            return AbsurdFee;
    }

    return SendCoinsReturn(OK);
}


WalletModel::SendCoinsReturn WalletModel::sendBookkeeping(QString add2,WalletModelTransaction &transaction)
{
    QByteArray transaction_array;
    LOCK2(cs_main, wallet->cs_wallet);
    CWalletTx *newTx = transaction.getTransaction();

    Q_FOREACH(const SendCoinsRecipient &rcp, transaction.getRecipients())
    {
        if (rcp.paymentRequest.IsInitialized())
        {
            // Make sure any payment requests involved are still valid.
            if (PaymentServer::verifyExpired(rcp.paymentRequest.getDetails())) {
                return PaymentRequestExpired;
            }

            // Store PaymentRequests in wtx.vOrderForm in wallet.
            std::string key("PaymentRequest");
            std::string value;
            rcp.paymentRequest.SerializeToString(&value);
            newTx->vOrderForm.push_back(make_pair(key, value));
        }
        else if (!rcp.message.isEmpty()) // Message from normal bitcoin:URI (bitcoin:123...?message=example)
            newTx->vOrderForm.push_back(make_pair("Message", rcp.message.toStdString()));
    }
    CReserveKey *keyChange = transaction.getPossibleKeyChange();
    CValidationState state;
    std::cout<<"CommitJoinCampaignTransaction"<<std::endl;
    QString address = add2;
    CTxDestination curAddress = CBitcoinAddress(address.toStdString()).Get();
    LOG_WRITE(LOG_INFO,"CommitJoinCampaignTransaction begin");

    if(!wallet->CommitJoinCampaignTransaction(curAddress, *newTx, *keyChange, g_connman.get(), state))
    {
        std::cout<<"CommitJoinCampaignTransaction faild"<<std::endl;
        ipcSendIsValidState = state.IsValid();
        LOG_WRITE(LOG_INFO,"CommitJoinCampaignTransaction begin",\
                  state.GetRejectReason().c_str());
        m_sendcoinerror = state.GetRejectReason();
        return SendCoinsReturn(TransactionCommitFailed, QString::fromStdString(state.GetRejectReason()));
    }
    else
    {
        LOG_WRITE(LOG_INFO,"CommitJoinCampaignTransaction end");
        ipcSendIsValidState = state.IsValid();
        std::cout<<"CommitJoinCampaignTransaction OK"<<std::endl;
    }
    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::prepareTransaction(WalletModelTransaction &transaction, const CCoinControl *coinControl)
{
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if(recipients.empty())
    {
        return OK;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    Q_FOREACH(const SendCoinsRecipient &rcp, recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
                CAmount nAmount = out.amount();

                CRecipient recipient = {scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount};
                vecSend.push_back(recipient);
            }
            if (subtotal <= 0)
            {

                return InvalidAmount;
            }
            total += subtotal;
        }
        else
        {   // User-entered bitcoin address / amount:
            if(!validateAddress(rcp.address))
            {
                return InvalidAddress;
            }
            if(rcp.amount <= 0)
            {
                return InvalidAmount;
            }
            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
            CRecipient recipient = {scriptPubKey, rcp.amount, rcp.fSubtractFeeFromAmount};
            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }
    if(setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    CAmount nBalance = getBalance(coinControl);

    if(total > nBalance)
    {
        return AmountExceedsBalance;
    }

    {
        LOCK2(cs_main, wallet->cs_wallet);

        transaction.newPossibleKeyChange(wallet);

        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;

        CWalletTx *newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();
        bool fCreated = wallet->CreateNormalTransaction(vecSend, *newTx, *keyChange, nFeeRequired, nChangePosRet, strFailReason, coinControl);
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
            transaction.reassignAmounts(nChangePosRet);

        if(!fCreated)
        {
            if(fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            Q_EMIT message(tr("Send Coins"), QString::fromStdString(strFailReason),
                           CClientUIInterface::MSG_ERROR);
            return TransactionCreationFailed;
        }

        // reject absurdly high fee. (This can never happen because the
        // wallet caps the fee at maxTxFee. This merely serves as a
        // belt-and-suspenders check)
        if (nFeeRequired > maxTxFee)
            return AbsurdFee;
    }

    return SendCoinsReturn(OK);
}
void WalletModel::startTimerControl()
{
    pollTimer_ = new QTimer(this);
    connect(pollTimer_, SIGNAL(timeout()), this, SLOT(updateTimer()));
    pollTimer_->start(250);

}
void WalletModel::updateTimer()
{
    if(!CDpocInfo::Instance().IsHasLocalAccount())
    {
        if(pollTimer_)
        {
            pollTimer_->stop();
            delete pollTimer_;
        }
        CMessageBox t;
        t.setIsClose(true);
        t.setWalletModel(this);
        t.setMessage(4);

        t.exec();

        if(t.m_answertype == 0)
        {
            return;
        }

    }
    else
    {
    }
}

WalletModel::SendCoinsReturn WalletModel::prepareNormalTransaction(WalletModelTransaction &transaction, const CCoinControl *coinControl)
{
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;
    if (recipients.empty())
    {
        return OK;
    }
    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    Q_FOREACH(const SendCoinsRecipient &rcp, recipients)
    {

        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr + out.script().size());
                CAmount nAmount = out.amount();
                CRecipient recipient = { scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount };
                vecSend.push_back(recipient);
            }
            if (subtotal <= 0)
            {
                return InvalidAmount;
            }
            total += subtotal;
        }
        else
        {   // User-entered bitcoin address / amount:
            if (!validateAddress(rcp.address))
            {
                return InvalidAddress;
            }
            if (rcp.amount <= 0)
            {
                return InvalidAmount;
            }
            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
            CRecipient recipient = { scriptPubKey, rcp.amount, rcp.fSubtractFeeFromAmount };
            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }
    if (setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }
    CAmount nBalance =_balance;

    if (total > nBalance)
    {
        return AmountExceedsBalance;
    }
    {
        LOCK2(cs_main, wallet->cs_wallet);
        transaction.newPossibleKeyChange(wallet);

        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;
        CWalletTx *newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();
        bool fCreated = wallet->CreateNormalTransaction(vecSend, *newTx, *keyChange, nFeeRequired, nChangePosRet, strFailReason, coinControl);
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
            transaction.reassignAmounts(nChangePosRet);
        if (!fCreated)
        {
            LOG_WRITE(LOG_INFO,"CreateNormalTransaction end",\
                      strFailReason.c_str());
            if (!fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            Q_EMIT message(tr("Send Coins"), QString::fromStdString(strFailReason),
                           CClientUIInterface::MSG_ERROR);
            return TransactionCreationFailed;
        }

        // reject absurdly high fee. (This can never happen because the
        // wallet caps the fee at maxTxFee. This merely serves as a
        // belt-and-suspenders check)
        if (nFeeRequired > maxTxFee)
            return AbsurdFee;
    }

    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::prepareIPCAuthorizationTransaction(WalletModelTransaction &transaction,std::string txid,int txi, QStringList data, const CCoinControl *coinControl )
{
    if(data.size()<4)return InvalidAddress;
    std::string strLabel = strprintf("{\"reAuthorize\":%u,\"uniqueAuthorize\":%u,\"startTime\":%u,\"stopTime\":%u}",\
                                     data.at(0).toStdString(),\
                                     data.at(1).toStdString(),\
                                     data.at(2).toStdString(),\
                                     data.at(3).toStdString());


    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if (recipients.empty())
    {
        return OK;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    Q_FOREACH(const SendCoinsRecipient &rcp, recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr + out.script().size());
                CAmount nAmount = out.amount();
                CRecipient recipient = { scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount };
                vecSend.push_back(recipient);
            }
            if (subtotal < 0)
            {
                return InvalidAmount;
            }
            total += subtotal;
        }
        else
        {   // User-entered bitcoin address / amount:
            if (!validateAddress(rcp.address))
            {

                return InvalidAddress;
            }
            if (rcp.amount < 0)
            {
                return InvalidAmount;
            }
            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
            CRecipient recipient = { scriptPubKey, rcp.amount, rcp.fSubtractFeeFromAmount };
            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }
    if (setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    CAmount nBalance = getBalance(coinControl);

    if (total > nBalance)
    {
        return AmountExceedsBalance;
    }

    {
        LOCK2(cs_main, wallet->cs_wallet);

        transaction.newPossibleKeyChange(wallet);

        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;

        CWalletTx *newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();
        LOG_WRITE(LOG_INFO,"CreateIPCAuthorizationTransaction begin",\
                  strLabel.c_str());
        bool fCreated = wallet->CreateIPCAuthorizationTransaction(txid,txi, vecSend,strLabel, *newTx, *keyChange, nFeeRequired, nChangePosRet, strFailReason, coinControl);
        LOG_WRITE(LOG_INFO,"CreateIPCAuthorizationTransaction end",\
                  strFailReason.c_str());
        //end
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
            transaction.reassignAmounts(nChangePosRet);

        if (!fCreated)
        {
            if (!fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }

            LOG_WRITE(LOG_INFO,"CreateIPCAuthorizationTransaction",\
                      strFailReason.c_str());
            m_sendcoinerror = strFailReason;
            return TransactionCreationFailed;
        }

        // reject absurdly high fee. (This can never happen because the
        // wallet caps the fee at maxTxFee. This merely serves as a
        // belt-and-suspenders check)
        if (nFeeRequired > maxTxFee)
            return AbsurdFee;
    }

    return SendCoinsReturn(OK);
}

CAmount WalletModel::getTotal(QList<SendCoinsRecipient>& recipients,QSet<QString>&setAddress,int &nAddresses)
{
    CAmount total = 0;
    return total;
}

WalletModel::SendCoinsReturn WalletModel::prepareIPCRegTransaction(WalletModelTransaction &transaction, uint8_t ExtendType, uint32_t startTime, uint32_t stopTime, uint8_t reAuthorize, uint8_t uniqueAuthorize, std::string hash, std::string labelTitle, std::string txLabel, const CCoinControl *coinControl)
{
    m_sendcoinerror ="";
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if (recipients.empty())
    {std::cout<<"recipients.empty()"<<std::endl;
        return OK;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;
    // Pre-check input data for validity
    Q_FOREACH(const SendCoinsRecipient &rcp, recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr + out.script().size());
                CAmount nAmount = out.amount();
                CRecipient recipient = { scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount };
                vecSend.push_back(recipient);
            }
            if (subtotal < 0)
            {
                std::cout<<"subtotal < 0"<<std::endl;
                return InvalidAmount;
            }
            total += subtotal;
        }
        else
        {   // User-entered bitcoin address / amount:
            if (!validateAddress(rcp.address))
            {

                return InvalidAddress;
            }
            if (rcp.amount < 0)
            {
                std::cout<<"rcp.amount <= 0"<<std::endl;
                return InvalidAmount;
            }
            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
            CRecipient recipient = { scriptPubKey, rcp.amount, rcp.fSubtractFeeFromAmount };
            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }

    if (setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    CAmount nBalance = getBalance(coinControl);
    LOG_WRITE(LOG_INFO,"CreateIPCRegTransaction",\
              "total",QString::number(total).toStdString().c_str(),\
              "nBalance",QString::number(nBalance).toStdString().c_str());
    if (total > nBalance)
    {
        return AmountExceedsBalance;
    }
    {
        LOCK2(cs_main, wallet->cs_wallet);
        transaction.newPossibleKeyChange(wallet);
        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;

        CWalletTx *newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();
        uint8_t	hashlen = hash.length();
        std::cout<<"comin CreateIPCRegTransaction"<<hash.c_str()<<std::endl;

        cJSON *root = NULL;
        root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "ExtendType", (int)ExtendType);
        cJSON_AddNumberToObject(root, "startTime", (int)startTime);
        cJSON_AddNumberToObject(root, "stopTime", (int)stopTime);
        cJSON_AddNumberToObject(root, "reAuthorize", (int)reAuthorize);
        cJSON_AddNumberToObject(root, "uniqueAuthorize", (int)uniqueAuthorize);
        cJSON_AddNumberToObject(root, "hashLen", (int)hashlen);
        cJSON_AddStringToObject(root, "hash", hash.c_str());
        cJSON_AddStringToObject(root, "labelTitle", labelTitle.c_str());
        cJSON_AddStringToObject(root, "txLabel", txLabel.c_str());

        std::string strLabel = cJSON_Print(root);//strprintf("{\"ExtendType\":%u,\"startTime\":%u,\"stopTime\":%u,\"reAuthorize\":%u,\"uniqueAuthorize\":%u,\"hashLen\":%u,\"hash\":\"" + hash + "\"," + "\"labelTitle\":\"" + labelTitle  + "\","+ "\"txLabel\":\"" + txLabel+"\"}", ExtendType, startTime, stopTime, reAuthorize, uniqueAuthorize, hashlen);
        std::cout<<strLabel.c_str()<<std::endl;
        LOG_WRITE(LOG_INFO,"CreateIPCRegTransaction",\
                  strLabel.c_str());
        bool fCreated = wallet->CreateIPCRegTransaction(strLabel,vecSend, *newTx, *keyChange, nFeeRequired, nChangePosRet, strFailReason, coinControl);
        std::cout<<"comout CreateIPCRegTransaction"<<std::endl;
        LOG_WRITE(LOG_INFO,"CreateIPCRegTransaction finish",\
                  strFailReason.c_str());
        //end
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
            transaction.reassignAmounts(nChangePosRet);

        if (!fCreated)
        {
            if (!fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            m_sendcoinerror = strFailReason;
            return TransactionCreationFailed;
        }

        // reject absurdly high fee. (This can never happen because the
        // wallet caps the fee at maxTxFee. This merely serves as a
        // belt-and-suspenders check)
        if (nFeeRequired > maxTxFee)
            return AbsurdFee;
    }

    return SendCoinsReturn(OK);
}

bool WalletModel::preparetransferIpc(QString address,QString &msg)
{
    if(m_ipcCoins.size()<ipcSelectFromList)return false;
    if(!CheckPassword())
    {
        msg = "Password error.";
        return 0;
    }
    WalletModel::UnlockContext ctx2(this, true, true);
    int num = m_ipcCoins.at(ipcSelectFromList).i;
    std::string txid = m_ipcCoins.at(ipcSelectFromList).tx->GetHash().ToString();
    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient;
    recipient.address = address;
    recipient.label =getAddressTableModel()->labelForAddress(address);//"";

    recipient.amount = 0;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = 0;
    recipients.append(recipient);

    LOG_WRITE(LOG_INFO,"transferIpc",address.toStdString().c_str(),txid.c_str());
    WalletModelTransaction* currentTransaction = new WalletModelTransaction(recipients);





    WalletModel::SendCoinsReturn prepareStatus;
    CCoinControl ctrl;
    ctrl.nConfirmTarget = 0;
    std::cout<<"prepareIPCSendTransaction"<<std::endl;
    prepareStatus = prepareIPCSendTransaction(txid,num,*currentTransaction,&ctrl);


    LOG_WRITE(LOG_INFO,"prepareIPCSendTransaction end",QString::number(prepareStatus.status).toStdString().c_str());
    if(prepareStatus.status == WalletModel::OK) {

        if(m_WalletModelTransactionIpcTransfer)
            delete m_WalletModelTransactionIpcTransfer;
        m_WalletModelTransactionIpcTransfer = currentTransaction;
        return 1;
    }
    else if(prepareStatus.status == WalletModel::AmountExceedsBalance)
    {
        msg = tr("AmountExceedsBalance");
        return 0;
    }
    else if(prepareStatus.status == WalletModel::AmountWithFeeExceedsBalance)
    {
        msg = tr("AmountWithFeeExceedsBalance");
        return 0;
    }
    else if(prepareStatus.status == WalletModel::InvalidAddress)
    {
        msg = tr("InvalidAddress");
        return 0;
    }
    else
    {
        std::cout<<QString::number(prepareStatus.status).toStdString().c_str()<<std::endl;
        msg = QString::number(prepareStatus.status);
        return 0;
    }

}
WalletModel::SendCoinsReturn  WalletModel::sendtransferIpc()
{

    WalletModel::SendCoinsReturn status =sendCoins(*m_WalletModelTransactionIpcTransfer);
    return status;

}
bool WalletModel::transferIpc(QString address,QString &msg)
{
    if(m_ipcCoins.size()<ipcSelectFromList)return false;
    if(!CheckPassword())
    {
        msg = "Password error.";
        return 0;
    }
    WalletModel::UnlockContext ctx2(this, true, true);
    int num = m_ipcCoins.at(ipcSelectFromList).i;
    std::string txid = m_ipcCoins.at(ipcSelectFromList).tx->GetHash().ToString();
    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient;
    recipient.address = address;
    recipient.label =getAddressTableModel()->labelForAddress(address);//"";

    recipient.amount = 0;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = 0;
    recipients.append(recipient);

    LOG_WRITE(LOG_INFO,"transferIpc",address.toStdString().c_str(),txid.c_str());
    WalletModelTransaction* currentTransaction = new WalletModelTransaction(recipients);
    if(m_WalletModelTransactionIpcRegister)
        delete m_WalletModelTransactionIpcRegister;
    m_WalletModelTransactionIpcRegister = currentTransaction;
    WalletModel::SendCoinsReturn prepareStatus;
    CCoinControl ctrl;
    ctrl.nConfirmTarget = 0;
    std::cout<<"prepareIPCSendTransaction"<<std::endl;
    prepareStatus = prepareIPCSendTransaction(txid,num,*currentTransaction,&ctrl);

    if(prepareStatus.status == WalletModel::OK) {
        msg = "OK";
        std::cout<<"prepareIPCSendTransaction"<<"OK"<<std::endl;
        if(sendIpcCoins()){
            return 1;
        }
        else{
            msg = QString::fromStdString(m_sendcoinerror);
            return 0;
        }
    }
    else if(prepareStatus.status == WalletModel::AmountExceedsBalance)
    {
        msg = tr("AmountExceedsBalance");
        return 0;
    }
    else if(prepareStatus.status == WalletModel::AmountWithFeeExceedsBalance)
    {
        msg = tr("AmountWithFeeExceedsBalance");
        return 0;
    }
    else if(prepareStatus.status == WalletModel::InvalidAddress)
    {
        msg = tr("InvalidAddress");
        return 0;
    }
    else
    {
        std::cout<<QString::number(prepareStatus.status).toStdString().c_str()<<std::endl;
        msg = QString::number(prepareStatus.status);
        return 0;
    }

}

bool WalletModel::prepareauthorizationIpc(QString address,int license,int exclusive,int intstart,int intend,QString& msg)
{
    m_sendcoinerror = "";
    if(m_ipcCoins.size()<ipcSelectFromList)return false;
    if(!CheckPassword())
    {
        msg = "Password error.";
        LOG_WRITE(LOG_INFO,"authorizationIpc",\
                  msg.toStdString().c_str());
        return 0;
    }
    WalletModel::UnlockContext ctx(this, true, true);
    int num = m_ipcCoins.at(ipcSelectFromList).i;
    std::string txid = m_ipcCoins.at(ipcSelectFromList).tx->GetHash().ToString();
    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient;
    recipient.address = address;
    recipient.label = getAddressTableModel()->labelForAddress(address);//"";
    recipient.amount = 0;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = 0;
    recipients.append(recipient);

    WalletModelTransaction* currentTransaction = new WalletModelTransaction(recipients);
    if(m_WalletModelTransactionIpcRegister){
        LOG_WRITE(LOG_INFO,"authorizationIpc",\
                  "m_WalletModelTransactionIpcRegister");
        delete m_WalletModelTransactionIpcRegister;
    }
    m_WalletModelTransactionIpcRegister = currentTransaction;
    WalletModel::SendCoinsReturn prepareStatus;
    CCoinControl ctrl;
    ctrl.nConfirmTarget = 0;
    std::cout<<"prepareIPCSendTransaction"<<std::endl;
    QStringList data;
    data<<QString::number(license);
    data<<QString::number(exclusive);
    data<<QString::number(intstart);
    data<<QString::number(intend);
    LOG_WRITE(LOG_INFO,"prepareIPCAuthorizationTransaction",\
              address.toStdString().c_str(),\
              QString::number(license).toStdString().c_str(),\
              QString::number(exclusive).toStdString().c_str(),\
              QString::number(intstart).toStdString().c_str(),\
              QString::number(intend).toStdString().c_str());
    prepareStatus = prepareIPCAuthorizationTransaction(*currentTransaction,txid,num,data,&ctrl);
    LOG_WRITE(LOG_INFO,"prepareIPCAuthorizationTransaction end",\
              QString::number(prepareStatus.status).toStdString().c_str());
    if(prepareStatus.status == WalletModel::OK) {
        msg = "OK";
        std::cout<<"prepareIPCSendTransaction"<<"OK"<<std::endl;

        return 1;

    }
    else if(prepareStatus.status == WalletModel::AmountExceedsBalance)
    {
        LOG_WRITE(LOG_INFO,"AmountExceedsBalance");
        msg = tr("AmountExceedsBalance");
        return 0;
    }
    else if(prepareStatus.status == WalletModel::AmountWithFeeExceedsBalance)
    {
        LOG_WRITE(LOG_INFO,"AmountWithFeeExceedsBalance");
        msg = tr("AmountWithFeeExceedsBalance");
        return 0;
    }
    else if(prepareStatus.status == WalletModel::InvalidAddress)
    {
        LOG_WRITE(LOG_INFO,"InvalidAddress");
        msg = "InvalidAddress";
        return 0;
    }
    else
    {
        LOG_WRITE(LOG_INFO,QString::number(prepareStatus.status).toStdString().c_str());
        std::cout<<QString::number(prepareStatus.status).toStdString().c_str()<<std::endl;
        msg = QString::fromStdString(m_sendcoinerror);
        return 0;
    }

}

bool WalletModel::sendauthorizationIpc(QString &msg)
{
    if(sendIpcCoins()){
        return 1;
    }
    else{
        msg = QString::fromStdString(m_sendcoinerror);
        return 0;
    }

}
bool WalletModel::authorizationIpc(QString address,int license,int exclusive,int intstart,int intend,QString& msg)
{
    m_sendcoinerror = "";
    if(m_ipcCoins.size()<ipcSelectFromList)return false;
    if(!CheckPassword())
    {
        msg = "Password error.";
        LOG_WRITE(LOG_INFO,"authorizationIpc",\
                  msg.toStdString().c_str());
        return 0;
    }
    WalletModel::UnlockContext ctx(this, true, true);
    int num = m_ipcCoins.at(ipcSelectFromList).i;
    std::string txid = m_ipcCoins.at(ipcSelectFromList).tx->GetHash().ToString();
    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient;
    recipient.address = address;
    recipient.label = getAddressTableModel()->labelForAddress(address);//"";
    recipient.amount = 0;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = 0;
    recipients.append(recipient);

    WalletModelTransaction* currentTransaction = new WalletModelTransaction(recipients);
    if(m_WalletModelTransactionIpcRegister){
        LOG_WRITE(LOG_INFO,"authorizationIpc",\
                  "m_WalletModelTransactionIpcRegister");
        delete m_WalletModelTransactionIpcRegister;
    }
    m_WalletModelTransactionIpcRegister = currentTransaction;
    WalletModel::SendCoinsReturn prepareStatus;
    CCoinControl ctrl;
    ctrl.nConfirmTarget = 0;
    std::cout<<"prepareIPCSendTransaction"<<std::endl;
    QStringList data;
    data<<QString::number(license);
    data<<QString::number(exclusive);
    data<<QString::number(intstart);
    data<<QString::number(intend);
    LOG_WRITE(LOG_INFO,"prepareIPCAuthorizationTransaction",\
              address.toStdString().c_str(),\
              QString::number(license).toStdString().c_str(),\
              QString::number(exclusive).toStdString().c_str(),\
              QString::number(intstart).toStdString().c_str(),\
              QString::number(intend).toStdString().c_str());
    prepareStatus = prepareIPCAuthorizationTransaction(*currentTransaction,txid,num,data,&ctrl);
    LOG_WRITE(LOG_INFO,"prepareIPCAuthorizationTransaction end",\
              QString::number(prepareStatus.status).toStdString().c_str());
    if(prepareStatus.status == WalletModel::OK) {
        msg = "OK";
        std::cout<<"prepareIPCSendTransaction"<<"OK"<<std::endl;
        if(sendIpcCoins()){
            return 1;
        }
        else{
            msg = QString::fromStdString(m_sendcoinerror);
            return 0;
        }
    }
    else if(prepareStatus.status == WalletModel::AmountExceedsBalance)
    {
        LOG_WRITE(LOG_INFO,"AmountExceedsBalance");
        msg = tr("AmountExceedsBalance");
        return 0;
    }
    else if(prepareStatus.status == WalletModel::AmountWithFeeExceedsBalance)
    {
        LOG_WRITE(LOG_INFO,"AmountWithFeeExceedsBalance");
        msg = tr("AmountWithFeeExceedsBalance");
        return 0;
    }
    else if(prepareStatus.status == WalletModel::InvalidAddress)
    {
        LOG_WRITE(LOG_INFO,"InvalidAddress");
        msg = "InvalidAddress";
        return 0;
    }
    else
    {
        LOG_WRITE(LOG_INFO,QString::number(prepareStatus.status).toStdString().c_str());
        std::cout<<QString::number(prepareStatus.status).toStdString().c_str()<<std::endl;
        msg = QString::fromStdString(m_sendcoinerror);
        return 0;
    }
}

WalletModel::SendCoinsReturn WalletModel::prepareIPCSendTransaction(std::string& IPCSendvinTxid,int Index, WalletModelTransaction &transaction, const CCoinControl *coinControl)
{
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if (recipients.empty())
    {
        return OK;
    }
    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    Q_FOREACH(const SendCoinsRecipient &rcp, recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr + out.script().size());
                CAmount nAmount = out.amount();
                CRecipient recipient = { scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount };
                vecSend.push_back(recipient);
            }
            if (subtotal < 0)
            {
                return InvalidAmount;
            }
            total += subtotal;
        }
        else
        {   // User-entered bitcoin address / amount:
            if (!validateAddress(rcp.address))
            {
                return InvalidAddress;
            }
            if (rcp.amount < 0)
            {
                return InvalidAmount;
            }
            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
            CRecipient recipient = { scriptPubKey, rcp.amount, rcp.fSubtractFeeFromAmount };
            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }
    if (setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }
    CAmount nBalance = getBalance(coinControl);

    if (total > nBalance)
    {

        LOG_WRITE(LOG_INFO,"return AmountExceedsBalance",QString::number(total).toStdString().c_str(),QString::number(nBalance).toStdString().c_str());
        return AmountExceedsBalance;
    }

    {
        LOCK2(cs_main, wallet->cs_wallet);

        transaction.newPossibleKeyChange(wallet);

        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;

        CWalletTx *newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();

        //	COutput IpcSendTxin = IPCSendTxin;
        bool fCreated = wallet->CreateIPCSendTransaction(IPCSendvinTxid, Index, vecSend, *newTx, *keyChange, nFeeRequired, nChangePosRet, strFailReason, coinControl);
        //end
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
            transaction.reassignAmounts(nChangePosRet);
        if (!fCreated)
        {
            if (!fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }

            if(strFailReason == _("Insufficient funds"))
            {
                return AmountExceedsBalance;
            }
            LOG_WRITE(LOG_INFO,"strFailReason",QString::fromStdString(strFailReason).toStdString().c_str(),QString::number(nBalance).toStdString().c_str());

            /*
            Q_EMIT message(tr("Send Coins"), QString::fromStdString(strFailReason),
                           CClientUIInterface::MSG_ERROR);
            */
            return TransactionCreationFailed;
        }

        // reject absurdly high fee. (This can never happen because the
        // wallet caps the fee at maxTxFee. This merely serves as a
        // belt-and-suspenders check)
        if (nFeeRequired > maxTxFee)
            return AbsurdFee;
    }

    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::sendcreateeCoins()
{

    if(m_WalletModelTransactioneCoinRegister)
    {
        WalletModel::SendCoinsReturn prepareStatus = sendCoins(*m_WalletModelTransactioneCoinRegister);
        return prepareStatus;
    }
}

WalletModel::SendCoinsReturn WalletModel::sendecoinaffrim()
{
    WalletModel::SendCoinsReturn sendStatus = sendCoins(*currentTransaction1);
    return sendStatus;
}
WalletModel::SendCoinsReturn WalletModel::prepareecoinaffrim(QString num,QString add,QString name)
{
    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient;
    std::string m_error = "";
    bool valid = true;

    if (recipient.paymentRequest.IsInitialized())
        return InitErr;

    QString strNumEdit = num;
    recipient.address = add;

    QString lab =getAddressTableModel()->labelForAddress(add);
    recipient.label =lab;
    recipient.amount = 0;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = false;
    recipients.append(recipient);
    // fNewRecipientAllowed = false;

    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    if(!CheckPassword())
    {
        //ui->tiplabel->setText(tr("password error"));
        return WalletModel::PsdErr;
    }
    WalletModel::UnlockContext ctx(this, true, true);

    // WalletModelTransaction currentTransaction(recipients);
    //currentTransaction1 = new WalletModelTransaction(currentTransaction);
    currentTransaction1 = new WalletModelTransaction(recipients);

    WalletModel::SendCoinsReturn prepareStatus;

    CCoinControl ctrl;

    if (getOptionsModel()->getCoinControlFeatures())
    {
        // ctrl = *CoinControlDialog::coinControl;
    }
    else
    {
        ctrl.nConfirmTarget = 1;
    }


    std::string Txid = name.toStdString();
    int acc = GetAccuracyBySymbol(Txid);
    double temp10 = 10;
    int pointplace = strNumEdit.indexOf(".");
    printf("pointplace %d \n",pointplace);
    if(pointplace>0)
    {
        int afterpointnum =  strNumEdit.size() - pointplace-1;
        strNumEdit.replace(QString("."), QString(""));
        acc =acc-afterpointnum;
    }
    while (acc>0) {
        strNumEdit+="0";
        acc--;
    }

    stringstream strValue;
    strValue << strNumEdit.toStdString().c_str();
    uint64_t intNumEdit;
    strValue >> intNumEdit;
    LOG_WRITE(LOG_INFO,"SendCoinsAffrimWidget::strNumEdit",\
              strNumEdit.toStdString().c_str(),\
              QString::number(intNumEdit).toStdString().c_str());

    if(intNumEdit<=0){
        // ui->tiplabel->setText(tr("num error"));
        return WalletModel::NumErr;
    }
    LOG_WRITE(LOG_INFO,"prepareeCoinsSendCreateTransaction start");
    prepareStatus = prepareeCoinsSendCreateTransaction(Txid,0,intNumEdit,*currentTransaction1,m_error, &ctrl);
    LOG_WRITE(LOG_INFO,"prepareeCoinsSendCreateTransaction end");
    if(prepareStatus.status != WalletModel::OK) {
        //  fNewRecipientAllowed = true;
        m_sendcoinerror  = m_error;
        return  prepareStatus.status;
    }
    else
    {
        LOG_WRITE(LOG_INFO,"prepareecoinaffrim::currentTransaction1 start");
        if(m_WalletModelTransactioneCoinSend1){
            delete m_WalletModelTransactioneCoinSend1;
            m_WalletModelTransactioneCoinSend1 = NULL;
          }
        m_WalletModelTransactioneCoinSend1 = currentTransaction1;
        LOG_WRITE(LOG_INFO,"prepareecoinaffrim::currentTransaction1 end");
        return WalletModel::OK;
    }



}
WalletModel::SendCoinsReturn WalletModel::sendCoins(WalletModelTransaction &transaction)
{
    QByteArray transaction_array; /* store serialized transaction */
    LOCK2(cs_main, wallet->cs_wallet);
    CWalletTx *newTx = transaction.getTransaction();
    Q_FOREACH(const SendCoinsRecipient &rcp, transaction.getRecipients())
    {
        if (rcp.paymentRequest.IsInitialized())
        {
            // Make sure any payment requests involved are still valid.
            if (PaymentServer::verifyExpired(rcp.paymentRequest.getDetails())) {
                return PaymentRequestExpired;
            }

            // Store PaymentRequests in wtx.vOrderForm in wallet.
            std::string key("PaymentRequest");
            std::string value;
            rcp.paymentRequest.SerializeToString(&value);
            newTx->vOrderForm.push_back(make_pair(key, value));
        }
        else if (!rcp.message.isEmpty()) // Message from normal bitcoin:URI (bitcoin:123...?message=example)
            newTx->vOrderForm.push_back(make_pair("Message", rcp.message.toStdString()));
    }
    CReserveKey *keyChange = transaction.getPossibleKeyChange();
    CValidationState state;

    std::cout<<"CommitTransaction"<<std::endl;
    std::cout<<"tx hash: "<<newTx->tx->GetHash().ToString()<<std::endl;
    if(!wallet->CommitTransaction(*newTx, *keyChange, g_connman.get(), state))
    {
        std::cout<<"CommitTransaction faild!\n"<<std::endl;
        ipcSendIsValidState = state.IsValid();
        m_sendcoinerror = state.GetRejectReason();

        return SendCoinsReturn(TransactionCommitFailed, QString::fromStdString(state.GetRejectReason()));
    }
    else
    {
        ipcSendIsValidState = state.IsValid();
        std::cout<<"CommitTransaction OK"<<std::endl;
    }

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << *newTx->tx;

    transaction_array.append(&(ssTx[0]), ssTx.size());


    Q_FOREACH(const SendCoinsRecipient &rcp, transaction.getRecipients())
    {

        // Don't touch the address book when we have a payment request
        if (!rcp.paymentRequest.IsInitialized())
        {
            std::string strAddress = rcp.address.toStdString();
            CTxDestination dest = CBitcoinAddress(strAddress).Get();
            std::string strLabel = rcp.label.toStdString();
            {
                LOCK(wallet->cs_wallet);

                std::map<CTxDestination, CAddressBookData>::iterator mi = wallet->mapAddressBook.find(dest);

                // Check if we have a new address or an updated label
                if (mi == wallet->mapAddressBook.end())
                {
                    wallet->SetAddressBook(dest, strLabel, "send");
                }
				else if (mi->second.name != strLabel &&  mi->second.purpose != "union")
                {
                    wallet->SetAddressBook(dest, strLabel, ""); // "" means don't change purpose
				}

            }
        }
        Q_EMIT coinsSent(wallet, rcp, transaction_array);
    }
    return SendCoinsReturn(OK);
}
OptionsModel *WalletModel::getOptionsModel()
{
    return optionsModel;
}

AddressTableModel *WalletModel::getAddressTableModel()
{
    return addressTableModel;
}

TransactionTableModel *WalletModel::getTransactionTableModel()
{
    return transactionTableModel;
}

RecentRequestsTableModel *WalletModel::getRecentRequestsTableModel()
{
    return recentRequestsTableModel;
}

WalletModel::EncryptionStatus WalletModel::getEncryptionStatus() const
{

    if(!wallet->IsCrypted())
    {

        return Unencrypted;
    }
    else if(wallet->IsLocked())
    {

        return Locked;
    }
    else
    {

        return Unlocked;
    }
}

bool WalletModel::setWalletEncrypted(bool encrypted, const SecureString &passphrase)
{
    if(encrypted)
    {
        return wallet->EncryptWallet(passphrase);
    }
    else
    {
        return false;
    }
}

bool WalletModel::setWalletLocked(bool locked, const SecureString &passPhrase)
{
    if(locked)
    {
        // Lock
        return wallet->Lock();
    }
    else
    {
        // Unlock
        return wallet->Unlock(passPhrase);
    }
}

bool WalletModel::changePassphrase(const SecureString &oldPass, const SecureString &newPass)
{
    bool retval;
    {
        LOCK(wallet->cs_wallet);
        wallet->Lock(); // Make sure wallet is locked before attempting pass change
        retval = wallet->ChangeWalletPassphrase(oldPass, newPass);
    }
    return retval;
}

bool WalletModel::backupWallet(const QString &filename)
{
    return wallet->BackupWallet(filename.toLocal8Bit().data());
}

// Handlers for core signals
static void NotifyKeyStoreStatusChanged(WalletModel *walletmodel, CCryptoKeyStore *wallet)
{
    qDebug() << "NotifyKeyStoreStatusChanged";
    QMetaObject::invokeMethod(walletmodel, "updateStatus", Qt::QueuedConnection);
}

static void NotifyAddressBookChanged(WalletModel *walletmodel, CWallet *wallet,
                                     const CTxDestination &address, const std::string &label, bool isMine,
                                     const std::string &purpose, ChangeType status)
{
    QString strAddress = QString::fromStdString(CBitcoinAddress(address).ToString());
    QString strLabel = QString::fromStdString(label);
    QString strPurpose = QString::fromStdString(purpose);

    qDebug() << "NotifyAddressBookChanged: " + strAddress + " " + strLabel + " isMine=" + QString::number(isMine) + " purpose=" + strPurpose + " status=" + QString::number(status);
    QMetaObject::invokeMethod(walletmodel, "updateAddressBook", Qt::QueuedConnection,
                              Q_ARG(QString, strAddress),
                              Q_ARG(QString, strLabel),
                              Q_ARG(bool, isMine),
                              Q_ARG(QString, strPurpose),
                              Q_ARG(int, status));
}

static void NotifyTransactionChanged(WalletModel *walletmodel, CWallet *wallet, const uint256 &hash, ChangeType status)
{
    Q_UNUSED(wallet);
    Q_UNUSED(hash);
    Q_UNUSED(status);
    QMetaObject::invokeMethod(walletmodel, "updateTransaction", Qt::QueuedConnection);
}

static void ShowProgress(WalletModel *walletmodel, const std::string &title, int nProgress)
{
    // emits signal "showProgress"
    QMetaObject::invokeMethod(walletmodel, "showProgress", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(title)),
                              Q_ARG(int, nProgress));
}

static void NotifyWatchonlyChanged(WalletModel *walletmodel, bool fHaveWatchonly)
{
    QMetaObject::invokeMethod(walletmodel, "updateWatchOnlyFlag", Qt::QueuedConnection,
                              Q_ARG(bool, fHaveWatchonly));
}

void WalletModel::subscribeToCoreSignals()
{
    // Connect signals to wallet

    wallet->NotifyStatusChanged.connect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.connect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
    wallet->NotifyWatchonlyChanged.connect(boost::bind(NotifyWatchonlyChanged, this, _1));
}

void WalletModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    wallet->NotifyStatusChanged.disconnect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.disconnect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
    wallet->NotifyWatchonlyChanged.disconnect(boost::bind(NotifyWatchonlyChanged, this, _1));
}

// WalletModel::UnlockContext implementation
WalletModel::UnlockContext WalletModel::requestUnlock()
{
    bool was_locked = getEncryptionStatus() == Locked;

    if(was_locked)
    {
        // Request UI to unlock wallet
        Q_EMIT requireUnlock();
    }

    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    bool valid = getEncryptionStatus() != Locked;

    return UnlockContext(this, valid, was_locked);
}

WalletModel::UnlockContext::UnlockContext(WalletModel *_wallet, bool _valid, bool _relock):
    wallet(_wallet),
    valid(_valid),
    relock(_relock)
{
}

WalletModel::UnlockContext::~UnlockContext()
{
    if(valid && relock)
    {
        wallet->setWalletLocked(true);
    }
}

void WalletModel::UnlockContext::CopyFrom(const UnlockContext& rhs)
{
    // Transfer context; old object no longer relocks wallet
    *this = rhs;
    rhs.relock = false;
}

bool WalletModel::getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    return wallet->GetPubKey(address, vchPubKeyOut);
}

bool WalletModel::havePrivKey(const CKeyID &address) const
{
    return wallet->HaveKey(address);
}

bool WalletModel::getPrivKey(const CKeyID &address, CKey& vchPrivKeyOut) const
{
    return wallet->GetKey(address, vchPrivKeyOut);
}

// returns a list of COutputs from COutPoints
void WalletModel::getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<COutput>& vOutputs)
{
    LOCK2(cs_main, wallet->cs_wallet);
    BOOST_FOREACH(const COutPoint& outpoint, vOutpoints)
    {
        if (!wallet->mapWallet.count(outpoint.hash)) continue;
        int nDepth = wallet->mapWallet[outpoint.hash].GetDepthInMainChain();
        if (nDepth < 0) continue;
        COutput out(&wallet->mapWallet[outpoint.hash], outpoint.n, nDepth, true, true);
        vOutputs.push_back(out);
    }
}

bool WalletModel::isSpent(const COutPoint& outpoint) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->IsSpent(outpoint.hash, outpoint.n);
}
void WalletModel::getbookkeepList(std::vector<WalletModel::CBookKeep>& bookkeeplist)
{
    //UniValue results(UniValue::VARR);
    std::vector<std::string> rewardtimelist;
    std::vector<std::string> rewardvaluelist;
    // vector<string,string> bookkeeplist;

    assert(wallet != NULL);
    LOCK2(cs_main, wallet->cs_wallet);
    {
        LOG_WRITE(LOG_INFO,"wallet->GetCurrentRewards");
        wallet->GetCurrentRewards(rewardtimelist, rewardvaluelist);
        LOG_WRITE(LOG_INFO,"wallet->GetCurrentRewards back",\
                  QString::number(rewardtimelist.size()).toStdString().c_str(),\
                  QString::number(rewardvaluelist.size()).toStdString().c_str());
    }
    int listcount = rewardvaluelist.size();
    for (int i = 0; i < listcount; i++)
    {
        CBookKeep m_bookkeep;
        m_bookkeep.time = rewardtimelist.at(i);
        m_bookkeep.award = rewardvaluelist.at(i);
        bookkeeplist.push_back(m_bookkeep);

    }

}

void WalletModel::listCoins(std::map<QString, std::vector<COutput> >& mapCoins) const
{
    std::vector<COutput> vCoins;
    wallet->AvailableCoins(vCoins);

    LOCK2(cs_main, wallet->cs_wallet); // ListLockedCoins, mapWallet
    std::vector<COutPoint> vLockedCoins;
    wallet->ListLockedCoins(vLockedCoins);

    // add locked coins
    BOOST_FOREACH(const COutPoint& outpoint, vLockedCoins)
    {
        if (!wallet->mapWallet.count(outpoint.hash)) continue;
        int nDepth = wallet->mapWallet[outpoint.hash].GetDepthInMainChain();
        if (nDepth < 0) continue;
        COutput out(&wallet->mapWallet[outpoint.hash], outpoint.n, nDepth, true, true);
        if (outpoint.n < out.tx->tx->vout.size() && wallet->IsMine(out.tx->tx->vout[outpoint.n]) == ISMINE_SPENDABLE)
            vCoins.push_back(out);
    }

    BOOST_FOREACH(const COutput& out, vCoins)
    {
        COutput cout = out;

        while (wallet->IsChange(cout.tx->tx->vout[cout.i]) && cout.tx->tx->vin.size() > 0 && wallet->IsMine(cout.tx->tx->vin[0]))
        {
            if (!wallet->mapWallet.count(cout.tx->tx->vin[0].prevout.hash)) break;
            cout = COutput(&wallet->mapWallet[cout.tx->tx->vin[0].prevout.hash], cout.tx->tx->vin[0].prevout.n, 0, true, true);
        }

        CTxDestination address;
        if(!out.fSpendable || !ExtractDestination(cout.tx->tx->vout[cout.i].scriptPubKey, address))
            continue;
        mapCoins[QString::fromStdString(CBitcoinAddress(address).ToString())].push_back(out);
    }
}
bool comp(const COutput &a,const COutput &b)
{
    return a.tx->GetTxTime()>b.tx->GetTxTime();
}
void WalletModel::getIpcListCoins(QVector<QStringList>& ipclist)
{
    ipclist.clear();
    LOG_WRITE(LOG_INFO,"AvailableIPCCoins");
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->AvailableIPCCoins(m_ipcCoins);
    LOG_WRITE(LOG_INFO,"end AvailableIPCCoins",\
              QString::number(m_ipcCoins.size()).toStdString().c_str());
    std::cout<<m_ipcCoins.size()<<std::endl;
    if(m_ipcCoins.size()>0){
        sort(m_ipcCoins.begin(),m_ipcCoins.end(),comp);
    }

    BOOST_FOREACH(const COutput& out, m_ipcCoins)
    {
        QStringList data;
        data<<QString::number(out.GetIPCExtendType());
        data<<QString::fromStdString(out.GetIPCTitle());
        int inttime = out.GetIPCStartTime();//out.tx->GetTxTime();
        data<<IntTimeToQStringTime(inttime);
        data<<QString::fromStdString(out.tx->GetHash().ToString());

        ipclist.push_back(data);
    }

}

void WalletModel::getTokenListCoins(std::map<std::string, uint64_t>& ecoinlist)//QVector<COutput>& ecoinlist)
{
    ecoinlist.clear();
    wallet->ListTokenBalance(ecoinlist);
    LOG_WRITE(LOG_INFO,"ListTokenBalance",\
              QString::number(ecoinlist.size()).toStdString().c_str());

}
QString WalletModel::GetHashBySymbol(QString tokensymbol)
{
   std::string name = tokensymbol.toStdString();
   uint128 hash = wallet->GetHashBySymbol(name);
   return QString::fromStdString(hash.ToString());
}

QString WalletModel::IntTimeToQStringTime(int inttime)
{
    QDateTime timedate = QDateTime::fromTime_t(inttime);
    //timedate = timedate.toUTC();
    QDate dateqdate = timedate.date();
    QString timestr = dateqdate.toString("yyyy-MM-dd");
    return timestr;
}
QString WalletModel::IntTimeToLocalQStringTime(int inttime)
{
    QDateTime timedate = QDateTime::fromTime_t(inttime);
    QDate dateqdate = timedate.date();
    QString timestr = dateqdate.toString("yyyy-MM-dd");
    return timestr;
}
QString WalletModel::IntTimeToQStringTimeForDetilDialog(int inttime)
{
    if(inttime==0)return QString("forever");
    QDateTime timedate = QDateTime::fromTime_t(inttime);
    //timedate = timedate.toUTC();
    QLocale locale;
    QString timestr;
    if( locale.language() == QLocale::Chinese )
    {
        timestr = timedate.toString("yyyy年MM月dd日 hh:mm:ss");
    }else{
        timestr = timedate.toString("yyyy/MM/dd hh:mm:ss");
    }
    return timestr;
}

QStringList WalletModel::getInfoFromIpcList(int index)
{
    QStringList back;
    int num = index;
    if(num>=0&&num<m_ipcCoins.size())
    {
        ipcSelectFromList = num;

        back<<QString::fromStdString(m_ipcCoins.at(num).GetIPCTitle());//0name
        int typenum = m_ipcCoins.at(num).GetIPCExtendType();//1type
        back<<QString::number(typenum);
        int64_t starttime = m_ipcCoins.at(num).GetIPCStartTime();
        back<<IntTimeToQStringTimeForDetilDialog(starttime);//2
        LOG_WRITE(LOG_INFO,"GetIPCStartTime",QString::number(m_ipcCoins.at(num).GetIPCStartTime()).toStdString().c_str());
        back<<IntTimeToQStringTimeForDetilDialog(m_ipcCoins.at(num).GetIPCStopTime());//3
        LOG_WRITE(LOG_INFO,"GetIPCStopTime",QString::number(m_ipcCoins.at(num).GetIPCStopTime()).toStdString().c_str());
        if(m_ipcCoins.at(num).GetType()==2)//4
            back<<QString(tr("ownership"));
        else
            back<<QString(tr("Use right"));
        if(m_ipcCoins.at(num).CanBeAuthorizedToOther())//5
        {
            back<<"can authorization";
        }
        else
        {
            back<<"cannot authorization";
        }
        back<<QString::fromStdString(m_ipcCoins.at(num).GetIPCHash());//6
        back<<QString::fromStdString(m_ipcCoins.at(num).tx->GetHash().ToString());//7
        back<<QString::fromStdString(m_ipcCoins.at(num).GetIPCLabel());//8
        std::cout<<m_ipcCoins.at(num).GetIPCLabel().c_str()<<std::endl;
        // CAmount nCredit = m_ipcCoins.at(num).tx->GetCredit(ISMINE_ALL);
        // CAmount nDebit = m_ipcCoins.at(num).tx->GetDebit(ISMINE_ALL);
        int nNet = m_ipcCoins.at(num).tx->GetDepthInMainChain();//nCredit - nDebit;
        QString queren = QString::number(nNet);
        // QString queren = QString::fromStdString(m_ipcCoins.at(num).Getqueren());//9
        back<<queren.append("+");//9

        if(m_ipcCoins.at(num).CanBeSentToOhter())//10
            back<<QString("1");
        else
            back<<QString("0");
        if(m_ipcCoins.at(num).CanBeAuthorizedToOther())
            back<<QString("1");
        else
            back<<QString("0");


        int paita = m_ipcCoins.at(num).GetIPCUniqAuthorize();
        back<<QString::number(paita);
        int64_t timeoftoken  =  m_ipcCoins.at(num).GetAssetEntryTime();
        if(timeoftoken != 0)
            back<<IntTimeToQStringTimeForDetilDialog(timeoftoken);
        else
            back<<"";

    }
    return back;
}
int WalletModel::GetDepthInMainChain(int iIndex)
{
    if(m_ipcCoins.size()>iIndex){
        int nNet = 0;
        if(m_ipcCoins.at(iIndex).tx)
            nNet = m_ipcCoins.at(iIndex).tx->GetDepthInMainChain();
        return nNet;
    }
    else{
        return -1;
    }
}
QString WalletModel::GetTimeOfTokenInChain(int iIndex)
{
    if(m_ipcCoins.size()>iIndex){
        int64_t timeoftoken  =  m_ipcCoins.at(iIndex).GetAssetEntryTime();
        if(timeoftoken != 0){
            return IntTimeToQStringTimeForDetilDialog(timeoftoken);//13 canusetime   GetTimeOfTokenInChain
        }
        else
            return "";
    }
    else{
        return "";
    }
}
QDate WalletModel::getSySQDate()
{
    int64_t nowtime  =  timeService.GetCurrentTimeSeconds();
    return QDateTime::fromTime_t(nowtime).date();
}

bool WalletModel::CanIPCTransaction(int iIndex)
{
    if(iIndex>=0&&m_ipcCoins.size()>iIndex){
        int64_t timeoftoken  =  m_ipcCoins.at(iIndex).GetAssetEntryTime();
        if(timeoftoken != 0){
            int64_t nowtime = timeService.GetCurrentTimeSeconds();
            if(nowtime>=timeoftoken)
                return true;
        }
    }
    return false;
}

bool WalletModel::isLockedCoin(uint256 hash, unsigned int n) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->IsLockedCoin(hash, n);
}

void WalletModel::lockCoin(COutPoint& output)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->LockCoin(output);
}

void WalletModel::unlockCoin(COutPoint& output)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->UnlockCoin(output);
}

void WalletModel::listLockedCoins(std::vector<COutPoint>& vOutpts)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->ListLockedCoins(vOutpts);
}

void WalletModel::loadReceiveRequests(std::vector<std::string>& vReceiveRequests)
{
    LOCK(wallet->cs_wallet);
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, wallet->mapAddressBook)
            BOOST_FOREACH(const PAIRTYPE(std::string, std::string)& item2, item.second.destdata)
            if (item2.first.size() > 2 && item2.first.substr(0,2) == "rr") // receive request
            vReceiveRequests.push_back(item2.second);
}

bool WalletModel::saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest)
{
    CTxDestination dest = CBitcoinAddress(sAddress).Get();

    std::stringstream ss;
    ss << nId;
    std::string key = "rr" + ss.str(); // "rr" prefix = "receive request" in destdata

    LOCK(wallet->cs_wallet);
    if (sRequest.empty())
        return wallet->EraseDestData(dest, key);
    else
        return wallet->AddDestData(dest, key, sRequest);
}

bool WalletModel::transactionCanBeAbandoned(uint256 hash) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    const CWalletTx *wtx = wallet->GetWalletTx(hash);
    if (!wtx || wtx->isAbandoned() || wtx->GetDepthInMainChain() > 0 || wtx->InMempool())
        return false;
    return true;
}

bool WalletModel::abandonTransaction(uint256 hash) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->AbandonTransaction(hash);
}

bool WalletModel::isWalletEnabled()
{
    return !GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET);
}

bool WalletModel::hdEnabled() const
{
    return wallet->IsHDEnabled();
}

int WalletModel::getDefaultConfirmTarget() const
{
    return nTxConfirmTarget;
}

bool WalletModel::CreateIpcRegister(QString address,QString name ,QString md5,QString brief,int type,QString &msg,int &fee)
{
    if(!CheckPassword())
    {
        msg = "Password error.";
        return 0;
    }
    WalletModel::UnlockContext ctx2(this, true, true);

    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient;

    recipient.address = address;
    recipient.label = "";
    recipient.amount = 0;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = 0;

    recipients.append(recipient);


    WalletModel::UnlockContext ctx(requestUnlock());
    if(!ctx.isValid())
    {
        return 0;
    }

    // prepare transaction for getting txFee earlier
    WalletModelTransaction* currentTransaction = new WalletModelTransaction(recipients);
    if(m_WalletModelTransactionIpcRegister)
        delete m_WalletModelTransactionIpcRegister;
    m_WalletModelTransactionIpcRegister = currentTransaction;
    WalletModel::SendCoinsReturn prepareStatus;

    // Always use a CCoinControl instance, use the CoinControlDialog instance if CoinControl has been enabled
    CCoinControl ctrl;
    ctrl.nConfirmTarget = 0;
    LOG_WRITE(LOG_INFO,"CreateIpcRegister",\
              name.toStdString().c_str(),\
              md5.toStdString().c_str(),\
              brief.toStdString().c_str(),\
              QString::number(type).toStdString().c_str());
    prepareStatus = prepareIPCRegTransaction(*currentTransaction,type,0,0,1,0,md5.toStdString(),name.toStdString(),brief.toStdString(), &ctrl);
    LOG_WRITE(LOG_INFO,"CreateIpcRegister prepareIPCRegTransaction finish",\
              QString::number(prepareStatus.status).toStdString().c_str());
    if(prepareStatus.status == WalletModel::OK) {
        msg = "OK";
        std::cout<<"CreateIpcRegister"<<"OK"<<std::endl;
        return 1;
    }
    else if(prepareStatus.status == WalletModel::AmountExceedsBalance)
    {

        msg = tr("AmountExceedsBalance");
        return 0;
    }
    else if(prepareStatus.status == WalletModel::AmountWithFeeExceedsBalance)
    {
        msg = tr("AmountWithFeeExceedsBalance");
        return 0;
    }
    else if(prepareStatus.status == WalletModel::InvalidAddress)
    {
        msg = tr("InvalidAddress");
        return 0;
    }
    else
    {

        msg = QString::fromStdString(m_sendcoinerror);//QString::number(prepareStatus.status);//tr("Other");
        return 0;
    }

    return 1;
}
bool WalletModel::CreateWalletFromFile(QString filepath)
{
    std::string path = filepath.toStdString();
    std::cout<<path<<std::endl;
    wallet->CreateWalletFromFile(path);
    std::cout<<path<<std::endl;
    return 1;
}


bool WalletModel::sendIpcCoins()
{
    WalletModel::SendCoinsReturn prepareStatus;
    if(m_WalletModelTransactionIpcRegister)
    {
        prepareStatus = sendCoins(*m_WalletModelTransactionIpcRegister);
    }
    if(prepareStatus.status != WalletModel::OK)
    {
        LOG_WRITE(LOG_INFO,"sendCoins",QString::number(prepareStatus.status).toStdString().c_str());
        return 0;
    }
    else
    {
        if(ipcSendIsValidState == 1){
            LOG_WRITE(LOG_INFO,"sendCoins","OK");
            return 1;

        }else{
            LOG_WRITE(LOG_INFO,"ipcSendIsValidState","0");
        }
    }

    return 0;
}
//extern bool LoadWalletFromFile(const std::string filepath);
bool WalletModel::LoadWalletFromFile(QString filepath)
{
    std::string temp = q2s(filepath);
    LOG_WRITE(LOG_INFO,"LoadWalletFromFile ok",temp.c_str());
    printf("begin wallet->LoadWalletFromFile %s \n",temp.c_str());
    return wallet->LoadWalletFromFile(temp);
}
bool WalletModel::ExportWalletToFile(std::string path)
{
    LOG_WRITE(LOG_INFO,"WalletModel::ExportWalletToFile",path.c_str());
    return wallet->ExportWalletToFile(path);
}

bool WalletModel::CheckPassword()
{
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    int64_t timenum = 0;
    int iPasswordErrorNum = 0;
    bool was_locked = (getEncryptionStatus() == WalletModel::Locked)?1:0;
    if(was_locked)
    {
        QSettings settings;

        iPasswordErrorNum = settings.value("PasswordErrorNum").toInt();
        LOG_WRITE(LOG_INFO,"iPasswordErrorNum",QString::number(iPasswordErrorNum).toStdString().c_str());
        timenum =  timeService.GetCurrentTimeSeconds();

        if(iPasswordErrorNum >= 5){

            QString locktime = settings.value("locktime").toString();
            LOG_WRITE(LOG_INFO,"timeService::GetCurrentTimeSeconds",QString::number(timenum).toStdString().c_str(),"locktime",locktime.toStdString().c_str());
            QString strtimenum = QString::number(timenum);
            if(locktime > strtimenum){
                CMessageBox msg;
                msg.setIsClose(false);
                msg.setMessage(2);
                msg.exec();
                return false;
            }else{
                settings.setValue("locktime", QString::number(0));
                iPasswordErrorNum = 0;
            }
        }

        WalletPassword pwddlg;
        pwddlg.setDlgParams();
        pwddlg.exec();
        QString pwd = pwddlg.GetPassword();
        SecureString oldpass;
        oldpass.assign(pwd.toStdString().c_str());
        timenum = timenum + (int64_t)60*60*24;
        if(!setWalletLocked(false, oldpass))
        {
            settings.setValue("PasswordErrorNum", iPasswordErrorNum +1);
            settings.setValue("locktime", QString::number(timenum));
            LOG_WRITE(LOG_INFO,"WalletModel::CheckPassword","Password error.");
            return false;
        }
        else
        {
            settings.setValue("PasswordErrorNum", 0);
            LOG_WRITE(LOG_INFO,"WalletModel::CheckPassword","Password true.");
        }
    }

    return true;
}
bool WalletModel::CheckIsCrypted()
{
    return wallet->IsCrypted();
}
int WalletModel::GetAccuracyBySymbol(std::string tokensymbol)
{
    uint8_t uback =  wallet->GetAccuracyBySymbol(tokensymbol);

    QString temp = QString::number(uback,16);
    LOG_WRITE(LOG_INFO,"GetAccuracyBySymbol",tokensymbol.c_str(),temp.toStdString().c_str());
    return temp.toInt();
}
QString WalletModel::getAccuracyNumstr(QString name ,QString num)
{
    std::string  stdname = name.toStdString();
    // num.chop(GetAccuracyBySymbol(stdname));
    int acc = GetAccuracyBySymbol(stdname);
    if(acc>0){
        if(num.size()>acc){
            num=num.insert(num.size()-acc,".");
        }else{
            QString temp = "0";
            for(int i=0;i<acc-num.size();i++){
                temp += "0";
            }
            temp+=num;
            num=temp.insert(temp.size()-acc,".");
        }
    }
    return num;
}



QString  WalletModel::getDeposit(QString qaddress)
{
    std::string address = qaddress.toStdString();
    uint160 pkhash;
    wallet->address2pkhash(address,pkhash);
    LOG_WRITE(LOG_INFO,"address2pkhash",address.c_str(),pkhash.GetHex().c_str());
    int64_t money = CConsensusAccountPool::Instance().GetCurDepositAdjust(pkhash,chainActive.Tip()->nHeight);
    QString back(QString::number(money));
    LOG_WRITE(LOG_INFO,"GetCurDepositAdjust",pkhash.GetHex().c_str(),back.toStdString().c_str());
    back.chop(8);

    return back;
}

void WalletModel::setUpdataFinished()
{
    static int itimes = 0;
    itimes++;//because we donot konw when 100% it maybe 2 times.So,I set it 3 times chances.
    if(m_bFinishedLoading == false || itimes <=3){
        m_bFinishedLoading = true;
        LOG_WRITE(LOG_INFO,"WalletModel::setUpdataFinished");
        Q_EMIT updataLoadingFinished();
    }else{
        Q_EMIT updataLater();
    }
}

QString WalletModel::GetECoinCanUseNum(QString name)
{
    std::string tokensymbol= name.toStdString();
    uint64_t value;
    wallet->GetSymbolbalance(tokensymbol,value);
    return getAccuracyNumstr(name,QString::number(value));
}
QString WalletModel::GetIssueDateBySymbol(QString name)
{
    std::string namestr = name.toStdString();
    int time = wallet->GetIssueDateBySymbol(namestr);
    return IntTimeToLocalQStringTime(time);
}
bool WalletModel::copyFileToPath(QString sourceDir ,QString toDir, bool coverFileIfExist,bool flushwallet)
{

    toDir.replace("\\","/");
    LOG_WRITE(LOG_INFO,"sourceDir:",sourceDir.toStdString().c_str()," toDir:",toDir.toStdString().c_str());
    if (sourceDir == toDir){
        return true;
    }
    if (!QFile::exists(sourceDir)){
         LOG_WRITE(LOG_INFO,"copyFileToPath false:QFile::exists");
        return false;
    }
    QDir *createfile     = new QDir;
    bool exist = createfile->exists(toDir);
    if(exist&&!coverFileIfExist){
        LOG_WRITE(LOG_INFO,"copyFileToPath false:exist&&!coverFileIfExist");
        return false;
    }
    if (exist){
        if(coverFileIfExist){
            createfile->remove(toDir);
        }
    }
    if(flushwallet){
        LOCK2(cs_main,wallet->cs_wallet);
        LOCK(bitdb.cs_db);
        wallet->flushWalletData();
    }
    if(!QFile::copy(sourceDir, toDir))
    {
        LOG_WRITE(LOG_INFO,"QFile::copy");
        return false;
    }
    return true;
}
QString WalletModel::getCreditValue(QString strDeposit)
{
    uint160 pkhash;
    std::string address = strDeposit.toStdString();
    wallet->address2pkhash(address,pkhash);
    LOG_WRITE(LOG_INFO,"address2pkhash",address.c_str(),pkhash.GetHex().c_str());
    int64_t credit;
    bool finish = CConsensusAccountPool::Instance().getCreditbyPkhash(pkhash,credit);
    if(finish){
        return QString::number(credit);
    }else{
        return "0";
    }
}

bool WalletModel::addtopubkey(std::string& add,std::string& key, std::string& strFailReason)
{

    if(wallet->address2Pubkey(add,key,strFailReason))
    {
        LOG_WRITE(LOG_INFO,"address2Pubkey  return true!!");
        return true;
    }
    else
    {
        return false;
    }

}
bool WalletModel::CreateUnionAddress(unsigned int nRequired, const std::vector<std::string>& strPubkeys, std::string& multiscript, std::string& strmultiaddress,std::string& strFailReason)
{
    if(wallet->CreateMultiAddress( nRequired, strPubkeys, multiscript, strmultiaddress,strFailReason))
    {
        LOG_WRITE(LOG_INFO,"my_num,conf_num---walletmodel",QString::number(nRequired).toStdString().c_str());

        return true;
    }
    else
    {
        return false;
    }

}

void WalletModel::lookuphistory(std::map<uint256,const CWalletTx*>& vCoins,std::string add)
{
  wallet->UnionCWalletTxFromAddress(vCoins, add);
}



void WalletModel::getScript(std::string add,int& sign_num,int &nRequired,std::string& script)
{
  wallet->GetAddressNmembers(add,sign_num,nRequired,script);

  LOG_WRITE(LOG_INFO,"GetAddressNmembers",add.c_str(),script.c_str());
}
bool WalletModel::analyTransaction(const std::string& str,std::string& strFailReason,std::string& address,CAmount& money,int haveSigned,std::string& sourceaddress)
{


    if(wallet->analysisMultiTransaction(str,strFailReason,address,money,haveSigned,sourceaddress))
    {
        LOG_WRITE(LOG_INFO,"analyTransaction sourceaddress:",sourceaddress.c_str());
        return true;
    }
    else
    {
        return false;
    }

}
bool WalletModel::joinunionaccount(std::string name,  std::string& strFailReason ,std::string union_script)
{
    if(!CheckPassword())
    {
        strFailReason = "Password error.";
        LOG_WRITE(LOG_INFO,"joinunionaccount",\
                  strFailReason.c_str());
        return 0;
    }
    WalletModel::UnlockContext ctx(this, true, true);

    LOG_WRITE(LOG_INFO,"walletmodel+AddMultiAddress",union_script.c_str(),name.c_str());
    int num, num1 = 0;
   // CMessageBox *msgbox = new CMessageBox();
   // msgbox->setMessage(6);

   // msgbox->show();

   // QDateTime time1 = QDateTime::currentDateTime();
   // int time1T = time1.toTime_t();
    QCoreApplication::processEvents();
    if(wallet->AddMultiAddress( union_script,strFailReason,num,num1, name))
    {
    //    if(!msgbox->isHidden()){
    //    msgbox->close();
    //    msgbox->deleteLater();
     //   }
        return true;
    }
    else
    {
       // if(!msgbox->isHidden()){
      //  QDateTime time2 = QDateTime::currentDateTime();
      //  int time2T = time2.toTime_t();
       // if(time2T-time1T<=1)sleep(2);
      //  msgbox->close();
      //  msgbox->deleteLater();
      //  }
        return false;
    }
}

void WalletModel::getUnionAddressinfo(std::vector<UnionAddressInfo> &addinfo)
{
    wallet->getUnionAddresses(addinfo);
}
void WalletModel::getAccounttrainfo(std::map<std::string,CAmount> &addinfo)
{
    wallet->AvailableUnionCoins(addinfo);
}

WalletModel::SendCoinsReturn WalletModel::repreparUnionaccountCreate(QString addfrom,QString add,CAmount num,ScriptError &serror,std::string &strtx)
{

    bool fNewRecipientAllowed;
    std::string m_error = "";
    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient;
    bool valid = true;
    if (recipient.paymentRequest.IsInitialized())
        return InitErr;


    recipient.address = add;
    recipient.label = "";//biaoqian
    recipient.amount = num;
    recipient.message = "";
    recipient.fSubtractFeeFromAmount = false;
    recipients.append(recipient);
    fNewRecipientAllowed = false;
    bool passwordwrite = CheckPassword();
    if(!passwordwrite){
         fNewRecipientAllowed = true;
    }



    LOG_WRITE(LOG_INFO,"SendMulMoney",QString::number(recipient.amount).toStdString().c_str());
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    valid = getEncryptionStatus() != WalletModel::Locked;

    WalletModel::UnlockContext ctx(this, valid, true);

    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return PsdErr;
    }

    // prepare transaction for getting txFee earlier
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;

    // Always use a CCoinControl instance, use the CoinControlDialog instance if CoinControl has been enabled

    CCoinControl ctrl;
    if (this->getOptionsModel()->getCoinControlFeatures())
    {
        // ctrl = *CoinControlDialog::coinControl;
    }
    else
    {
        ctrl.nConfirmTarget = 1;
    }

   LOG_WRITE(LOG_INFO,"repreparUnionaccountCreate::prepareUnionaccountCreateTransaction start");

    prepareStatus = this->prepareUnionaccountCreateTransaction(addfrom,currentTransaction, &ctrl,serror,strtx,m_error);
    LOG_WRITE(LOG_INFO,"repreparUnionaccountCreate::prepareUnionaccountCreateTransaction end");

    if(prepareStatus.status != WalletModel::OK) {
        //  fNewRecipientAllowed = true;
        m_sendcoinerror  = m_error;
        LOG_WRITE(LOG_INFO," WalletModel::NOOK",m_sendcoinerror.c_str());
        return  prepareStatus.status;
    }
    else
    {
        LOG_WRITE(LOG_INFO," WalletModel::OK");

        return WalletModel::OK;
    }



}

WalletModel::SendCoinsReturn WalletModel::prepareUnionaccountCreateTransaction(QString add_from,WalletModelTransaction &transaction, const CCoinControl *coinControl,ScriptError &serror,std::string &strtx,std::string &m_error)
{
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;
    if (recipients.empty())
    {
        LOG_WRITE(LOG_INFO,"recipients.empty()");
        return OK;
    }
    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    Q_FOREACH(const SendCoinsRecipient &rcp, recipients)
    {

        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr + out.script().size());
                CAmount nAmount = out.amount();
                CRecipient recipient = { scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount };
                vecSend.push_back(recipient);
            }
            if (subtotal <= 0)
            {
                return InvalidAmount;
            }
            total += subtotal;
        }
        else
        {   // User-entered bitcoin address / amount:
            /*
            if (!validateAddress(rcp.address))
            {
                return InvalidAddress;
            }
            if (rcp.amount <= 0)
            {
                return InvalidAmount;
            }
            */

            CBitcoinAddress address(add_from.toStdString());
            if (!address.IsValid())
            {
                return InvalidAddress;
            }

            if (rcp.amount <= 0)
            {
                return InvalidAmount;
            }


            if (!validateAddress(rcp.address))
            {
                LOG_WRITE(LOG_INFO,"here");
                return InvalidAddress;
            }



            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
            CRecipient recipient = { scriptPubKey, rcp.amount, rcp.fSubtractFeeFromAmount };
            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }
    /*
    if (setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }
    CAmount nBalance =_balance;

    if (total > nBalance)
    {
        return AmountExceedsBalance;
    }

    */
    {
        LOCK2(cs_main, wallet->cs_wallet);
        transaction.newPossibleKeyChange(wallet);

        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;

        std::string addfrom = add_from.toStdString() ;

        CWalletTx *newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();




        bool fCreated = wallet->CreateMultiTransaction(addfrom,vecSend, *newTx, *keyChange, nFeeRequired, nChangePosRet, strFailReason,serror, strtx, coinControl);

        LOG_WRITE(LOG_INFO,"///////CreateMultiTransaction serror",\
                  QString::number(serror).toStdString().c_str());

        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
            transaction.reassignAmounts(nChangePosRet);
        if (!fCreated)
        {
            LOG_WRITE(LOG_INFO,"CreateNormalTransaction end",\
                      strFailReason.c_str());
            /*
            if (!fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            */
         //   Q_EMIT message(tr("Send Coins"), QString::fromStdString(strFailReason),
         //                  CClientUIInterface::MSG_ERROR);
            m_error = strFailReason;
            return TransactionCreationFailed;
        }

        // reject absurdly high fee. (This can never happen because the
        // wallet caps the fee at maxTxFee. This merely serves as a
        // belt-and-suspenders check)
        if (nFeeRequired > maxTxFee)
            return AbsurdFee;

    }
    LOG_WRITE(LOG_INFO,"SendCoinsReturn(OK)");
    return SendCoinsReturn(OK);
}
bool WalletModel::commitUnionaccounttra(const std::string& strtx, std::string& strFailReason, ScriptError& serror, std::string& strtxout,CMutableTransaction& mergedTx)
{

    if(wallet->SignMultiTransaction(strtx, strFailReason,  serror, strtxout,mergedTx))
   // if(wallet->SignMultiTransaction(strtx,strFailReason, serror,strtxout))
    {
        LOG_WRITE(LOG_INFO,"/////SignMultiTransaction",QString::number(serror).toStdString().c_str());
        return true;
    }
    else
    {
        return false;
    }
}
bool WalletModel::commitUnionaccounttra_plus(CMutableTransaction& mergedTx,std::string& error)
{
    if(wallet->CommitMultiTransaction(mergedTx, error))
    {
        LOG_WRITE(LOG_INFO,"MODEL CommitMultiTransaction success",error.c_str());
        return true;
    }
    else
    {
        LOG_WRITE(LOG_INFO,"MODEL CommitMultiTransaction FAIL",error.c_str());
        return false;

    }
}
bool WalletModel::isMyPk(std::string addpk)
{
    return wallet->isMyPk(addpk);
}
bool WalletModel::vinsFindAddress(const std::vector<CTxIn>& vin,std::string address)
{
     return  wallet->vinsFindAddress(vin,address);
}
int64_t WalletModel::GetBlockTime(const CWalletTx& wtx)
{
    return  wallet->GetBlockTime(wtx);
}
QString WalletModel::FormatTxTimeP2SH(const CWalletTx& wtx)
{
    int64_t nTime = GetBlockTime(wtx);
    if(nTime==0){
        nTime = wtx.GetTxTime();
    }
    QString m_time=QDateTime::fromTime_t(nTime).toString("yyyy-MM-dd hh:mm:ss");;
    return m_time;
}
QString WalletModel::FormatTxStatus(const CWalletTx& wtx)
{
    AssertLockHeld(cs_main);
    if (!CheckFinalTx(wtx))
    {
        if (wtx.tx->nLockTime < LOCKTIME_THRESHOLD)
        {
            return QObject::tr("Open for %n more block(s)", "", wtx.tx->nLockTime - chainActive.Height());
        }
        else
        {
            return QObject::tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx.tx->nLockTime));
        }
    }
    else
    {
        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < 0)
             {
            return QObject::tr("conflicted with a transaction with %1").arg(-nDepth);

           // return QObject::tr("conflicted with a transaction with %1 confirmations").arg(-nDepth);
        }
        else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            return QObject::tr("%1 offline").arg(wtx.GetDepthInMainChain());
            // return tr("%1/offline").arg(nDepth);
        }
        else if (nDepth == 0)
        {
            return QObject::tr("0/unconfirmed, %1").arg((wtx.InMempool() ? QObject::tr("in memory pool") : QObject::tr("not in memory pool"))) + (wtx.isAbandoned() ? ", "+QObject::tr("abandoned") : "");
        }
        else if (nDepth < 8)
        {
            return QObject::tr("%1 unconfirmed").arg(wtx.GetDepthInMainChain());

            //return tr("%1/unconfirmed").arg(nDepth);
        }
        else
        {
            return QObject::tr("%1 confirmations").arg(wtx.GetDepthInMainChain());

            // return tr("%1 confirmations").arg(nDepth);
        }
    }
}

bool WalletModel::importprivkeybibipay(std::string strSecret,std::string& strError,std::string strLabel,bool fRescan)
{
    return wallet->importprivkeybibipay(strSecret,strError,strLabel,fRescan);
}
