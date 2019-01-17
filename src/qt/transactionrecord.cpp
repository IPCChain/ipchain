// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionrecord.h"
#include <QMessageBox>
#include "base58.h"
#include "consensus/consensus.h"
#include "validation.h"
#include "timedata.h"
#include "wallet/wallet.h"
#include <stdint.h>
#include <cmath>
#include <math.h>
#include <boost/foreach.hpp>
#include <QMessageBox>
#include "timedata.h"
#include "util.h"
#include <stdint.h>
#include <string>
#include <QObject>
#include <guiutil.h>
#include "log/log.h"
#include "ipcdialog.h"
#include "ecoindialog.h"

/* Return positive answer if transaction should be shown in list.
 */


QString IntTimeToQStringTime(int inttime)
{
    QDateTime timedate = QDateTime::fromTime_t(inttime);
    QDate dateqdate = timedate.date();
    QString timestr = dateqdate.toString("yyyy/MM/dd");
    return timestr;
}

int TransactionRecord::GetAccuracySymbol(std::string tokensymbol)
{
    uint8_t uback =  wallet_->GetAccuracyBySymbol(tokensymbol);
    QString temp = QString::number(uback,16);
    return temp.toInt();
}
QString TransactionRecord::getAccuracyNumstr(QString name ,QString num)
{
    std::string  stdname = name.toStdString();
    int acc = GetAccuracySymbol(stdname);
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



QString TransactionRecord::getAccuracyNum(int acc ,QString num)
{
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

bool TransactionRecord::showTransaction(const CWalletTx &wtx)
{

    if (wtx.IsCoinBase())
    {
        // Ensures we show generated coins / mined transactions at depth 1
        if (!wtx.IsInMainChain())
        {
            return false;
        }
    }
    return true;
}
QString TransactionRecord::FormatTxStatus(const CWalletTx& wtx)
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
            return QObject::tr("conflicted with a transaction with %1 confirmations").arg(-nDepth);
        }
        else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            return QObject::tr("%1/offline").arg(wtx.GetDepthInMainChain());
            // return tr("%1/offline").arg(nDepth);
        }
        else if (nDepth == 0)
        {
            return QObject::tr("0/unconfirmed, %1").arg((wtx.InMempool() ? QObject::tr("in memory pool") : QObject::tr("not in memory pool"))) + (wtx.isAbandoned() ? ", "+QObject::tr("abandoned") : "");
        }
        else if (nDepth < 8)
        {
            return QObject::tr("%1/unconfirmed").arg(wtx.GetDepthInMainChain());

            //return tr("%1/unconfirmed").arg(nDepth);
        }
        else
        {
            return QObject::tr("%1 confirmations").arg(wtx.GetDepthInMainChain());

            // return tr("%1 confirmations").arg(nDepth);
        }
    }
}
QString TransactionRecord::FormatTxTime(const CWalletTx& wtx)
{
    int64_t nTime = wtx.GetTxTime();
    QString m_time=QDateTime::fromTime_t(nTime).toString("yyyy-MM-dd hh:mm:ss");;


    return m_time;
}

void TransactionRecord::markLastInfo(std::string add,CAmount mount)
{
    QList<TransactionRecord> parts;
    TransactionRecord sub(m_traninfo.nhash, m_traninfo.ntime);
    sub.credit=mount;
    sub.type = TransactionRecord::Recvdeposit;
    sub.address = CBitcoinAddress(add).ToString();

    sub.involvesWatchAddress = true;
    sub.strstatus = m_traninfo.status;
    sub.strtime =m_traninfo.time;
    parts.append(sub);
}
void TransactionRecord::saveInfo(TranInfo traninfo)
{
    m_traninfo.add = traninfo.add;
    m_traninfo.status = traninfo.status;
    m_traninfo.time = traninfo.time;
    m_traninfo.nhash = traninfo.nhash;
    m_traninfo.ntime = traninfo.ntime;
}

QList<TransactionRecord> TransactionRecord::decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx)
{
    wallet_=const_cast<CWallet *>(wallet);
    if( nullptr== wallet_)
    {
        printf("never happende \n\n\n\n");
    }

    QList<TransactionRecord> parts;
    QList<TransactionRecord> m_parts;
    int64_t nTime = wtx.GetTxTime();
    CAmount nCredit = wtx.GetCredit(ISMINE_ALL);
    CAmount nDebit = wtx.GetDebit(ISMINE_ALL);
    CAmount nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    LOG_WRITE(LOG_INFO,"decomposeTransaction txid:",hash.ToString().c_str());
    LOG_WRITE(LOG_INFO,"decomposeTransaction nDebit:",QString::number(nDebit).toStdString().c_str());
    LOG_WRITE(LOG_INFO,"decomposeTransaction nCredit:",QString::number(nCredit).toStdString().c_str());
    bool isTag = false;
    bool isMarked = false;
    std::map<std::string, std::string> mapValue = wtx.mapValue;
    if (wtx.IsCoinBase())
    {
        // mapmarkAdd.clear();
        for(unsigned int i = 0; i < wtx.tx->vout.size(); i++)
        {
            const CTxOut& txout = wtx.tx->vout[i];
            isminetype mine = wallet->IsMine(txout);
            if(mine)
            {
                TransactionRecord sub(hash, nTime);
                CTxDestination address;

                sub.idx = i; // vout index
                sub.credit = txout.nValue;
                sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                sub.strstatus = FormatTxStatus(wtx);
                sub.strtime =FormatTxTime(wtx);
                if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*wallet, address))
                {

                    if(0==txout.txType)
                    {
                        //   sub.type = TransactionRecord::RecvCoin;
                    }
                    else if(1== txout.txType && 4 == txout.devoteLabel.ExtendType)
                    {

                        CAmount markbill;
                        std::string add_ = CBitcoinAddress(address).ToString();

                        sub.involvesWatchAddress = false;
                        sub.credit = wallet->GetDeposit();
                        LOG_WRITE(LOG_INFO,"mark bill0000000",QString::number(sub.credit).toStdString().c_str(),sub.strtime.toStdString().c_str(),add_.c_str());

                        sub.credit = 0;
                        sub.debit = 0;
                        sub.type = TransactionRecord::Recvdeposit;
                        sub.address = CBitcoinAddress(address).ToString();
                        parts.append(sub);

                        LOG_WRITE(LOG_INFO,"mark bill",QString::number(sub.credit).toStdString().c_str(),sub.strtime.toStdString().c_str(),add_.c_str());
                    }
                }
            }
        }

    }
    else
    {
        bool involvesWatchAddress = false;
        isminetype fAllFromMe = ISMINE_SPENDABLE;
        BOOST_FOREACH(const CTxIn& txin, wtx.tx->vin)
        {
            isminetype mine = wallet->IsMine(txin);
            CTxDestination address;
            std::string add;

            ExtractDestination(txin.scriptSig, address);
            add= CBitcoinAddress(address).ToString();

            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllFromMe > mine) fAllFromMe = mine;
        }

        isminetype fAllToMe = ISMINE_SPENDABLE;
        bool m_isAllToMe = true;
        isminetype m_lastfAllToMe = ISMINE_WATCH_UNSOLVABLE;
        int m_allis = 0;
        BOOST_FOREACH(const CTxOut& txout, wtx.tx->vout)
        {
            isminetype mine = wallet->IsMine(txout);

            CTxDestination address;
            std::string add;
            if (ExtractDestination(txout.scriptPubKey, address))
                add= CBitcoinAddress(address).ToString();

            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllToMe > mine) fAllToMe = mine;
            if(fAllFromMe && (wtx.tx->vout.size() >2))
            {
                if(0 == txout.txType)
                {
                    // if((m_lastfAllToMe != ISMINE_WATCH_UNSOLVABLE) && (m_lastfAllToMe != fAllToMe))
                    {
                        m_isAllToMe = false;

                    }
                    m_lastfAllToMe = fAllToMe;
                    m_allis++;
                }

            }

        }
        if((m_allis != wtx.tx->vout.size()) && (m_allis != 0))
        {

            LOG_WRITE(LOG_INFO,"m_isAllToMe = true",QString::number(m_allis).toStdString().c_str());
            m_isAllToMe = true;
        }


        LOG_WRITE(LOG_INFO,"m_isAllToMe = true00000",QString::number(fAllFromMe).toStdString().c_str(),\
                  QString::number(m_isAllToMe).toStdString().c_str(),\
                  QString::number(fAllToMe).toStdString().c_str());

        if (fAllFromMe && fAllToMe && m_isAllToMe)
        {
            LOG_WRITE(LOG_INFO,"fAllFromMe && fAllToMe && m_isAllToMe");
            CAmount nChange = wtx.GetChange();
            bool isSpecileTran = false;

            CAmount nTxFee = nDebit - wtx.tx->GetValueOut();

            for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++)
            {

                const CTxOut& txout = wtx.tx->vout[nOut];
                if(2==txout.txType || 3==txout.txType){

                    ipcdialog::updatalist();
                    ipcdialog::m_bNeedUpdateLater = true;
                    CTxDestination address;
                    std::string add;
                    if (ExtractDestination(txout.scriptPubKey, address))
                        add= CBitcoinAddress(address).ToString();
                    int typenum = txout.ipcLabel.ExtendType;
                    int ipcType;

                    ipcType = typenum;
                    QString AuthType;
                    QString AuthLimit;
                    QString AuthTime;
                    if(2 == txout.txType)
                        AuthType =QObject::tr("ownership");
                    else
                        AuthType = QObject::tr("Use right");
                    if(1 == txout.ipcLabel.reAuthorize)                   //if(1== txout.ipcLabel.uniqueAuthorize)//5
                    {
                        AuthLimit = QObject::tr("can authorization");
                    }
                    else
                    {
                        AuthLimit = QObject::tr("cannot authorization");
                    }

                    uint32_t start = txout.ipcLabel.startTime;
                    uint32_t stop = txout.ipcLabel.stopTime;


                    QString starttime=IntTimeToQStringTime(start);//2
                    QString stoptime = IntTimeToQStringTime(stop);//3
                    if(starttime == "1970/01/01" && stoptime == "1970/01/01")
                    {
                        AuthTime =QObject::tr("forever");
                    }
                    else
                    {
                        if(stoptime == "1970/01/01")
                        {
                            AuthTime = starttime+ "--" +QObject::tr("forever");
                        }
                        else
                        {
                            AuthTime = starttime+ "--" +stoptime;

                        }
                    }
                    QString m_status=FormatTxStatus(wtx);
                    QString m_strtime =FormatTxTime(wtx);
                    CAmount m_fee = nDebit - wtx.tx->GetValueOut();
                    parts.append(TransactionRecord(hash, nTime, TransactionRecord::RecvIPC,add,
                                                   -(nDebit - nChange), nCredit - nChange,txout.ipcLabel.labelTitle,ipcType,AuthType,AuthLimit,AuthTime,m_status,m_strtime,m_fee));
                    parts.last().involvesWatchAddress = involvesWatchAddress;// maybe pass to TransactionRecord as constructor argument
                    isSpecileTran = true;
                    break;

                }
                else if(0==txout.txType)
                {
                    LOG_WRITE(LOG_INFO,"0==txout.txType");

                }
				else if (4 == txout.txType || 5 == txout.txType || TXOUT_ADDTOKEN == txout.txType)
                {
                    ECoinDialog::updatalist();
                    ECoinDialog::m_bNeedUpdateLater = true;
                    const CTxOut& txout = wtx.tx->vout[nOut];
                    std::string y;
                    QString num;
                    if(4  == txout.txType){
                        y =(char*)(txout.tokenRegLabel.TokenSymbol);
                        num=QString::number(txout.tokenRegLabel.totalCount);//444
                        int vacc = txout.tokenRegLabel.accuracy;
                        num = getAccuracyNum(vacc,num);
                       }
					else if (TXOUT_ADDTOKEN == txout.txType){
						y = (char*)(txout.addTokenLabel.TokenSymbol);
						num = QString::number(txout.addTokenLabel.totalCount);//444
						int vacc = txout.addTokenLabel.accuracy;
						num = getAccuracyNum(vacc, num);
					}
                    else{
                        y=(char*)(txout.tokenLabel.TokenSymbol);
                        num=QString::number(txout.tokenLabel.value);//555
                        int vacc = txout.tokenLabel.accuracy;
                        num = getAccuracyNum(vacc,num);
                    }

                    isSpecileTran = true;
                    CTxDestination address;
                    std::string add;
                    if (ExtractDestination(txout.scriptPubKey, address))
                        add= CBitcoinAddress(address).ToString();
                    QString m_status=FormatTxStatus(wtx);
                    QString m_strtime =FormatTxTime(wtx);
                    CAmount m_fee = nDebit - wtx.tx->GetValueOut();
                    parts.append(TransactionRecord(hash, nTime, TransactionRecord::RecveCoin, add,
                                                   -(nDebit - nChange), nCredit - nChange,y,num,m_status,m_strtime,m_fee));
                    parts.last().involvesWatchAddress = involvesWatchAddress;// maybe pass to TransactionRecord as constructor argument
                    break;
                }
                else if(1 == txout.txType)
                {
                    LOG_WRITE(LOG_INFO,"1 == txout.txType");
                    CTxDestination address;
                    std::string add;
                    if(ExtractDestination(txout.scriptPubKey, address))
                        add= CBitcoinAddress(address).ToString();

                    QString m_status=FormatTxStatus(wtx);
                    QString m_strtime =FormatTxTime(wtx);

                    CAmount m_fee = nDebit - wtx.tx->GetValueOut();

                    parts.append(TransactionRecord(hash, nTime, TransactionRecord::Senddeposit,add,
                                                   -(nDebit - nChange), nChange -  nCredit ,m_status,m_strtime,m_fee));//txout.ipcLabel.labelTitle,txout.ipcLabel.ExtendType,

                    parts.last().involvesWatchAddress = involvesWatchAddress;// maybe pass to TransactionRecord as constructor argument

                    isSpecileTran = true;
                    break;
                }

            }
            if(false == isSpecileTran)
            {
                LOG_WRITE(LOG_INFO,"false == isSpecileTran");
                mapAdd.clear();
                for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++)
                {
                    const CTxOut& txout = wtx.tx->vout[nOut];
                    if(0 == txout.txType )//|| 1==txout.txType)
                    {
                        CTxDestination address;
                        std::string add;
                        if(ExtractDestination(txout.scriptPubKey, address))
                        {
                            add= CBitcoinAddress(address).ToString();
                            mapAdd.insert(std::pair<int, std::string>(txout.nValue, add));
                        }
                    }
                }
                for (unsigned int nOut = 0; nOut < (wtx.tx->vout.size()-1); nOut++)
                {
                    const CTxOut& txout = wtx.tx->vout[nOut];
                    if(0 == txout.txType )//|| 1==txout.txType)
                    {
                        CTxDestination address;
                        std::string add;
                        if(ExtractDestination(txout.scriptPubKey, address))
                            add= CBitcoinAddress(address).ToString();
                        if( wtx.tx->vout.size() > 2)
                        {
                            add = add+"...";
                        }

                        QString m_status=FormatTxStatus(wtx);

                        QString m_strtime =FormatTxTime(wtx);

                        CAmount m_fee = nDebit - wtx.tx->GetValueOut();
                        if(nOut == wtx.tx->vout.size()-2)
                        {
                            parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendToSelf,add,
                                                           -(nDebit - nChange), nChange -  nCredit ,m_status,m_strtime,m_fee));//txout.ipcLabel.labelTitle,txout.ipcLabel.ExtendType,

                            parts.last().involvesWatchAddress = involvesWatchAddress;// maybe pass to TransactionRecord as constructor argument
                        }
                        // break;

                    }
                }
            }
        }
        else if (fAllFromMe && !fAllToMe && m_isAllToMe)
        {
            LOG_WRITE(LOG_INFO,"fAllFromMe && !fAllToMe && m_isAllToMe",QString::number(fAllFromMe).toStdString().c_str(),\
                      QString::number(m_isAllToMe).toStdString().c_str(),\
                      QString::number(fAllToMe).toStdString().c_str());
            bool isSpecileTran = false;
            CAmount nTxFee = nDebit - wtx.tx->GetValueOut();
            for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.tx->vout[nOut];

                TransactionRecord sub(hash, nTime);
                sub.idx = nOut;
                sub.involvesWatchAddress = involvesWatchAddress;
                if(wallet->IsMine(txout))
                {
                    continue;
                }

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {
                    if(2==txout.txType|| 3==txout.txType)
                    {
                        ipcdialog::updatalist();
                        ipcdialog::m_bNeedUpdateLater = true;
                        sub.type = TransactionRecord::SendIPC;

                        sub.ipcTitle = txout.ipcLabel.labelTitle;
                        int typenum = txout.ipcLabel.ExtendType;

                        sub.ipcType = typenum;
                        sub.authType = txout.ipcLabel.reAuthorize;
                        sub.authLimit =txout.ipcLabel.uniqueAuthorize ;


                        if(2 == txout.txType)
                            sub.authType =QObject::tr("ownership");
                        else
                            sub.authType = QObject::tr("Use right");
                        if(1 == txout.ipcLabel.reAuthorize)
                        {
                            sub.authLimit = QObject::tr("can authorization");
                        }
                        else
                        {
                            sub.authLimit = QObject::tr("cannot authorization");
                        }

                        uint32_t start = txout.ipcLabel.startTime;
                        uint32_t stop = txout.ipcLabel.stopTime;


                        QString starttime=IntTimeToQStringTime(start);//2
                        QString stoptime = IntTimeToQStringTime(stop);//3
                        if(starttime == "1970/01/01" && stoptime == "1970/01/01")
                        {
                            sub.authTime =QObject::tr("forever");
                        }
                        else
                        {
                            if(stoptime == "1970/01/01")
                            {
                                sub.authTime = starttime+ "--" +QObject::tr("forever");
                            }
                            else
                            {
                                sub.authTime = starttime+ "--" +stoptime;

                            }
                        }

                        sub.address = CBitcoinAddress(address).ToString();
                        sub.strstatus=FormatTxStatus(wtx);
                        sub.strtime =FormatTxTime(wtx);
                        CAmount nValue = txout.nValue;
                        if (nTxFee > 0)
                        {
                            nValue += nTxFee;
                            nTxFee = 0;
                        }
                        sub.debit = -nValue;

                        CAmount m_fee = nDebit - wtx.tx->GetValueOut();
                        sub.TxFee = m_fee;
                        isSpecileTran = true;
                        parts.append(sub);

                        break;

                    }
                    // Sent to Bitcoin Address
                    else if(0==txout.txType)
                    {

                    }
                    else if(1==txout.txType)
                    {

                        break;

                    }
					else if (4 == txout.txType || 5 == txout.txType || TXOUT_ADDTOKEN == txout.txType)
                    {
                        ECoinDialog::updatalist();
                        ECoinDialog::m_bNeedUpdateLater = true;
                        sub.type = TransactionRecord::SendeCoin;
                        const CTxOut& txout = wtx.tx->vout[nOut];
						std::string y = txout.getTokenSymbol();
                        //sub.amount = txout.tokenRegLabel.totalCount;
                        sub.ecoinType = y;
						sub.ecoinNum = QString::number(txout.GetTokenvalue());
                        int vacc = txout.tokenLabel.accuracy;
                        sub.ecoinNum = getAccuracyNum(vacc,sub.ecoinNum);
                        sub.address = CBitcoinAddress(address).ToString();
                        CAmount nValue = txout.nValue;
                        if (nTxFee > 0)
                        {
                            nValue += nTxFee;
                            nTxFee = 0;
                        }
                        sub.debit = -nValue;

                        CAmount m_fee = nDebit - wtx.tx->GetValueOut();
                        sub.TxFee = m_fee;

                        sub.strstatus=FormatTxStatus(wtx);
                        sub.strtime =FormatTxTime(wtx);

                        isSpecileTran = true;
                        parts.append(sub);
                        break;
                    }
                    else
                    {
                    }

                }

            }
            if(false == isSpecileTran)
            {
                LOG_WRITE(LOG_INFO,"false == isSpecileTran");
                CAmount nvalueplus = 0;
                for (unsigned int nOut = 0; nOut < (wtx.tx->vout.size()-1); nOut++)
                {
                    const CTxOut& txout = wtx.tx->vout[nOut];
                    TransactionRecord sub(hash, nTime);
                    sub.idx = nOut;
                    sub.involvesWatchAddress = involvesWatchAddress;
                    


                    if(wallet->IsMine(txout))
                    {
                        // Ignore parts sent to self, as this is usually the change
                        // from a transaction sent back to our own address.
                        continue;
                    }


                    CTxDestination address;
                    if (ExtractDestination(txout.scriptPubKey, address))
                    {

                        if(0==txout.txType)
                        {

                            sub.strstatus=FormatTxStatus(wtx);
                            sub.strtime =FormatTxTime(wtx);
                            sub.type = TransactionRecord::SendCoin;
                            sub.address = CBitcoinAddress(address).ToString();
                            if(wtx.tx->vout.size() >2)
                            {
                                sub.address = sub.address+"...";
                            }
                            CAmount nValue = txout.nValue;
                            nvalueplus += -nValue;
                            sub.debit = nvalueplus;

                            CAmount m_fee = nDebit - wtx.tx->GetValueOut();
                            sub.TxFee = m_fee;

                            if(nOut == wtx.tx->vout.size()-2)
                            {
                                parts.append(sub);
                            }
                            // break;

                        }
                    }

                }
            }
        }
        else if(fAllFromMe && !m_isAllToMe)
        {
            LOG_WRITE(LOG_INFO,"fAllFromMe && !m_isAllToMe",QString::number(fAllFromMe).toStdString().c_str(),\
                      QString::number(m_isAllToMe).toStdString().c_str(),\
                      QString::number(fAllToMe).toStdString().c_str());
            CAmount nChange = wtx.GetChange();
            bool isSpecileTran = false;
            CAmount nvalueplus_ = 0;
            CAmount nTxFee = nDebit - wtx.tx->GetValueOut();

            for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++)
            {

                const CTxOut& txout = wtx.tx->vout[nOut];
                if(2==txout.txType || 3==txout.txType){
                    LOG_WRITE(LOG_INFO,"error date2");
                    break;
                }
                else if(0==txout.txType)
                {
                }
				else if (4 == txout.txType || 5 == txout.txType || TXOUT_ADDTOKEN == txout.txType)
                {
                    LOG_WRITE(LOG_INFO,"error date4");
                    break;
                }
                else if(1 == txout.txType)
                {
                    LOG_WRITE(LOG_INFO,"error date1");
                    break;
                }

            }
            if(false == isSpecileTran)
            {
                LOG_WRITE(LOG_INFO,"false == isSpecileTran");
                for (unsigned int nOut = 0; nOut < (wtx.tx->vout.size()-1); nOut++)
                {
                    const CTxOut& txout = wtx.tx->vout[nOut];

                    if(0 == txout.txType )//|| 1==txout.txType)
                    {
                        CTxDestination address;
                        std::string add;
                        if(ExtractDestination(txout.scriptPubKey, address))
                            add= CBitcoinAddress(address).ToString();
                        if( wtx.tx->vout.size() > 2)
                        {
                            add = add+"...";
                        }

                        QString m_status=FormatTxStatus(wtx);
                        QString m_strtime =FormatTxTime(wtx);
                        CAmount nValue = txout.nValue;
                        nvalueplus_ += -nValue;
                        CAmount m_fee = nDebit - wtx.tx->GetValueOut();

                        if(nOut == wtx.tx->vout.size()-2)
                        {
                            LOG_WRITE(LOG_INFO,"SENDMANY");
                            parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendToSelf,add,
                                                           0, nvalueplus_ ,m_status,m_strtime,m_fee));//txout.ipcLabel.labelTitle,txout.ipcLabel.ExtendType,

                            parts.last().involvesWatchAddress = involvesWatchAddress;// maybe pass to TransactionRecord as constructor argument
                        }
                        // break;

                    }
                }
            }


        }
        else
        {
            LOG_WRITE(LOG_INFO,"m_isAllToMe = true4",QString::number(fAllFromMe).toStdString().c_str(),\
                      QString::number(m_isAllToMe).toStdString().c_str(),\
                      QString::number(fAllToMe).toStdString().c_str());
            CAmount nTxFee = nDebit - wtx.tx->GetValueOut();
            CAmount m_plusmoney = 0;
            std::string m_plusadd = "";
            int m_plusnOut = 0;
            int m_newplusOut = 0;
            int m_isplusnOut = 0;
            int m_notplusnOut = 0;

            int m_newisplusnOut = 0;
            int m_newnotplusnOut = 0;
            for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.tx->vout[nOut];
                TransactionRecord sub(hash, nTime);
                sub.idx = nOut;
                sub.involvesWatchAddress = involvesWatchAddress;
                m_plusnOut++;
                ++m_newplusOut;
                if(!wallet->IsMine(txout))
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    m_notplusnOut++;
                    ++m_newnotplusnOut;

                    if(0 == parts.size() &&(m_notplusnOut == (wtx.tx->vout.size()-m_isplusnOut)) )
                    {
                        LOG_WRITE(LOG_INFO,"new push <<>1-2",QString::number(m_plusnOut).toStdString().c_str(),QString::number(wtx.tx->vout.size()).toStdString().c_str(),QString::number(m_isplusnOut).toStdString().c_str() ,QString::number(m_notplusnOut).toStdString().c_str());

                        parts.append(m_parts);
                        m_parts.clear();
                    }

                    continue;
                }

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {
                    LOG_WRITE(LOG_INFO,"txid:",hash.ToString().c_str()," vout:",\
                               QString::number(nOut+1).toStdString().c_str(),\
                               "/",QString::number(wtx.tx->vout.size()).toStdString().c_str());
                    LOG_WRITE(LOG_INFO,"txout.txType",QString::number(txout.txType).toStdString().c_str(),\
                              "txout.nValue",QString::number(txout.nValue).toStdString().c_str() );

                    if(  2==txout.txType||3==txout.txType)
                    {
                        ipcdialog::updatalist();
                        ipcdialog::m_bNeedUpdateLater = true;

                        sub.type = TransactionRecord::RecvIPC;
                        sub.address = CBitcoinAddress(address).ToString();
                        CAmount nValue = txout.nValue;
                        sub.credit = nValue;
                        std::string title = txout.ipcLabel.labelTitle;
                        sub.ipcTitle=txout.ipcLabel.labelTitle;
                        sub.strstatus=FormatTxStatus(wtx);
                        sub.strtime =FormatTxTime(wtx);
                        int typenum = txout.ipcLabel.ExtendType;
                        sub.ipcType = typenum;
                        sub.authType = txout.ipcLabel.reAuthorize;
                        sub.authLimit =txout.ipcLabel.uniqueAuthorize ;


                        if(2 == txout.txType)
                            sub.authType =QObject::tr("ownership");
                        else
                            sub.authType = QObject::tr("Use right");
                        if(1 == txout.ipcLabel.reAuthorize)
                        {
                            sub.authLimit = QObject::tr("can authorization");
                        }
                        else
                        {
                            sub.authLimit = QObject::tr("cannot authorization");
                        }

                        uint32_t start = txout.ipcLabel.startTime;
                        uint32_t stop = txout.ipcLabel.stopTime;


                        QString starttime=IntTimeToQStringTime(start);//2
                        QString stoptime = IntTimeToQStringTime(stop);//3
                        if(starttime == "1970/01/01" && stoptime == "1970/01/01")
                        {
                            sub.authTime =QObject::tr("forever");
                        }
                        else
                        {
                            if(stoptime == "1970/01/01")
                            {
                                sub.authTime = starttime+ "--" +QObject::tr("forever");
                            }
                            else
                            {
                                sub.authTime = starttime+ "--" +stoptime;

                            }
                        }

                        sub.TxFee = 0;
                        parts.append(sub);
                        break;

                    }
                    // Sent to Bitcoin Address
                    else if(0==txout.txType)
                    {
                        m_isplusnOut++;
                        sub.strtime = FormatTxTime(wtx);
                        sub.strstatus = FormatTxStatus(wtx);
                        sub.type = TransactionRecord::RecvCoin;

                        LOG_WRITE(LOG_INFO,"recv is mine1");

                        m_plusadd = CBitcoinAddress(address).ToString();
                        sub.address=m_plusadd;
                        //if(wtx.tx->vout.size()>2)
                        if(m_isplusnOut > 1)
                        {
                            sub.address +="...";
                        }
                        CAmount nValue = txout.nValue;
                        m_plusmoney += nValue;
                        sub.credit = m_plusmoney;
                        m_parts.clear();
                        m_parts.append(sub);
                        if((m_plusnOut == (wtx.tx->vout.size()-1)))
                        {
                            parts.append(sub);
                            m_parts.clear();
                        }
                        if(wtx.tx->vout.size()==1)
                        {
                            parts.append(sub);
                            m_parts.clear();
                        }
                        LOG_WRITE(LOG_INFO,"msg read",sub.address.c_str(),"num",QString::number(sub.credit).toStdString().c_str());


                        LOG_WRITE(LOG_INFO,"m_plusnOut = 2 && wtx.tx->vout.size()",QString::number(m_plusnOut).toStdString().c_str(),QString::number(wtx.tx->vout.size()).toStdString().c_str());


                        LOG_WRITE(LOG_INFO,"parts.size() = 0 ",QString::number(parts.size()).toStdString().c_str(),QString::number(wtx.tx->vout.size()).toStdString().c_str());

                        if(0== parts.size()&& 2== wtx.tx->vout.size()  && m_plusnOut==wtx.tx->vout.size() )
                        {
                            LOG_WRITE(LOG_INFO,"new push");
                            parts.append(sub);
                            m_parts.clear();
                        }
                        //LOG_WRITE(LOG_INFO,"new push >>>>2",QString::number(m_isplusnOut).toStdString().c_str() );
                        if(0 == parts.size() && wtx.tx->vout.size() > 2 && m_plusnOut==wtx.tx->vout.size() )
                        {
                            LOG_WRITE(LOG_INFO,"new push >2",QString::number(m_isplusnOut).toStdString().c_str() );
                            parts.append(sub);
                            m_parts.clear();
                        }



                        // break;
                    }
                    else if(1==txout.txType)
                    {
                    }
					else if (4 == txout.txType || 5 == txout.txType || TXOUT_ADDTOKEN == txout.txType)
                    {
                        m_isplusnOut++;
                        if(m_isplusnOut > 1)
                        {
                            sub.address +="...";
                        }
                        LOG_WRITE(LOG_INFO,"else if(4==txout.txType || 5==txout.txType)",QString::number(m_isplusnOut).toStdString().c_str() );
                        ECoinDialog::updatalist();
                        ECoinDialog::m_bNeedUpdateLater = true;
                        sub.type = TransactionRecord::RecveCoin;
                        std::string y;
                        int vacc=0;
                        if(4==txout.txType){
                            y=(char*)(txout.tokenRegLabel.TokenSymbol);
                            sub.ecoinNum =QString::number(txout.tokenRegLabel.totalCount);
                            vacc = txout.tokenRegLabel.accuracy;
                        }
						else if (TXOUT_ADDTOKEN == txout.txType){
							y = (char*)(txout.addTokenLabel.TokenSymbol);
							sub.ecoinNum = QString::number(txout.addTokenLabel.currentCount);
							vacc = txout.addTokenLabel.accuracy;
						}
                        else{
                            y=(char*)(txout.tokenLabel.TokenSymbol);
                            sub.ecoinNum =QString::number(txout.tokenLabel.value);
                            vacc = txout.tokenLabel.accuracy;
                        }
                        sub.ecoinType =y;
                        sub.ecoinNum = getAccuracyNum(vacc,sub.ecoinNum);
                        sub.strstatus = FormatTxStatus(wtx);
                        sub.strtime =FormatTxTime(wtx);
                        sub.address = CBitcoinAddress(address).ToString();
                        CAmount nValue = txout.nValue;
                        sub.credit = nValue;
                        sub.TxFee = 0;
                        parts.append(sub);
                        LOG_WRITE(LOG_INFO,"parts.size = ",\
                                  QString::number(parts.size()).toStdString().c_str(), \
                                  "sub.address = ", sub.address.c_str(),\
                                  "sub.ecoinNum = ", sub.ecoinNum.toStdString().c_str());

                        //break;
                    }
                }

                else
                {
                    // m_notplusnOut++;
                    // if(m_notplusnOut == (wtx.tx->vout.size()-m_isplusnOut))
                    {
                        //   parts.append(m_parts);
                    }

                }
            }
        }
    }


    return parts;
}

void TransactionRecord::updateStatus(const CWalletTx &wtx)
{
    AssertLockHeld(cs_main);
    // Determine transaction status

    // Find the block the tx is in
    CBlockIndex* pindex = NULL;
    BlockMap::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
                               (pindex ? pindex->nHeight : std::numeric_limits<int>::max()),
                               (wtx.IsCoinBase() ? 1 : 0),
                               wtx.nTimeReceived,
                               idx);
    status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity() > 0);
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = chainActive.Height();

    if (!CheckFinalTx(wtx))
    {
        if (wtx.tx->nLockTime < LOCKTIME_THRESHOLD)
        {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = wtx.tx->nLockTime - chainActive.Height();
        }
        else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.tx->nLockTime;
        }
    }
    // For generated transactions, determine maturity
    else if(type == TransactionRecord::Generated)
    {
        if (wtx.GetBlocksToMaturity() > 0)
        {
            status.status = TransactionStatus::Immature;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetBlocksToMaturity();

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.status = TransactionStatus::MaturesWarning;
            }
            else
            {
                status.status = TransactionStatus::NotAccepted;
            }
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }
    else
    {
        if (status.depth < 0)
        {
            status.status = TransactionStatus::Conflicted;
        }
        else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            status.status = TransactionStatus::Offline;
        }
        else if (status.depth == 0)
        {
            status.status = TransactionStatus::Unconfirmed;
            if (wtx.isAbandoned())
                status.status = TransactionStatus::Abandoned;
        }
        else if (status.depth < RecommendedNumConfirmations)
        {
            status.status = TransactionStatus::Confirming;
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }

}

bool TransactionRecord::statusUpdateNeeded()
{
    AssertLockHeld(cs_main);
    return status.cur_num_blocks != chainActive.Height();
}

QString TransactionRecord::getTxID() const
{
    return QString::fromStdString(hash.ToString());
}

int TransactionRecord::getOutputIndex() const
{
    return idx;
}
//QString TransactionRecord::getstrStatus()
//int TransactionRecord::getstrStatus()
QString TransactionRecord::getstrStatus()
{
    return strstatus;//QString::fromStdString(strstatus);
}

QString TransactionRecord::getecoinType()
{
    return QString::fromStdString(ecoinType);
}
QString TransactionRecord::getIPCTitle()
{
    return QString::fromStdString(ipcTitle);
}
uint8_t TransactionRecord::getIPCType()
{
    return ipcType;
}

uint8_t TransactionRecord::getType()
{
    return type;
}
