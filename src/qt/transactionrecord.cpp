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
//#include <qt4/QtCore/QList>
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

bool TransactionRecord::showTransaction(const CWalletTx &wtx)
{

    //  LOG_WRITE(LOG_INFO,"TransactionRecord::showTransaction");
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


    LOG_WRITE(LOG_INFO,"记录上一条数据",sub.address.c_str(),sub.strstatus.toStdString().c_str(),sub.strtime.toStdString().c_str());

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
//txout.txType   0普通交易 1计帐 2知产登记.3知产授权，4代币登记 5代币交易
QList<TransactionRecord> TransactionRecord::decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx)//20170821
{
    wallet_=const_cast<CWallet *>(wallet);
    if( nullptr== wallet_)
    {
        printf("never happende \n\n\n\n");
    }

    QList<TransactionRecord> parts;
    int64_t nTime = wtx.GetTxTime();
    CAmount nCredit = wtx.GetCredit(ISMINE_ALL);
    CAmount nDebit = wtx.GetDebit(ISMINE_ALL);
    CAmount nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();

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

                        sub.type = TransactionRecord::RecvCoin;

                    }
                    else if(1== txout.txType && 4 == txout.devoteLabel.ExtendType)
                    {
                        CAmount markbill;
                        std::string add_ = CBitcoinAddress(address).ToString();

                        sub.involvesWatchAddress = false;
                        sub.credit = wallet->GetDeposit();

                        if(0==sub.credit)
                        {
                            LOG_WRITE(LOG_INFO,"never happend",QString::number(sub.credit).toStdString().c_str());
                            //  sub.credit = markbill;
                        }


                        sub.type = TransactionRecord::Recvdeposit;
                        sub.address = CBitcoinAddress(address).ToString();
                        parts.append(sub);

                        LOG_WRITE(LOG_INFO,"有一笔记账退款发生",QString::number(sub.credit).toStdString().c_str(),sub.strtime.toStdString().c_str(),add_.c_str());

                        break;
                    }
                    sub.address = CBitcoinAddress(address).ToString();
                }
                if (wtx.IsCoinBase())
                {

                    sub.type = TransactionRecord::Generated;
                }

                // parts.append(sub);

                break;
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
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllFromMe > mine) fAllFromMe = mine;
        }

        isminetype fAllToMe = ISMINE_SPENDABLE;
        BOOST_FOREACH(const CTxOut& txout, wtx.tx->vout)
        {
            isminetype mine = wallet->IsMine(txout);
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllToMe > mine) fAllToMe = mine;
        }

        if (fAllFromMe && fAllToMe)//20170830
        {
            // Payment to self
            CAmount nChange = wtx.GetChange();


            bool isSpecileTran = false;

            //20170827
            CAmount nTxFee = nDebit - wtx.tx->GetValueOut();
            // sub.feeAmount = nTxFee;
            printf("fAllFromMe && fAllToMe \n\n\n\n\n");

            for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++)
            {
                printf("不是普通交易 非找零");
                const CTxOut& txout = wtx.tx->vout[nOut];
                printf("txout txType: %d \r\n",txout.txType);

                if(2==txout.txType || 3==txout.txType){
                    LOG_WRITE(LOG_INFO,"ipcdialog::updatalist0");
                    ipcdialog::updatalist();
                    printf("知产登记\n");
                    CTxDestination address;
                    std::string add;
                    if (ExtractDestination(txout.scriptPubKey, address))
                        add= CBitcoinAddress(address).ToString();
                    printf("add : %s \n\n",add.c_str());
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

                }
                else if(4==txout.txType || 5==txout.txType)
                {
                    ECoinDialog::updatalist();
                    const CTxOut& txout = wtx.tx->vout[nOut];
                    std::string y;
                    QString num;
                    if(4  == txout.txType){

                        printf("代币登记\n");
                        y =(char*)(txout.tokenRegLabel.TokenSymbol);
                        num=QString::number(txout.tokenRegLabel.totalCount);//444
                        num = getAccuracyNumstr(QString::fromStdString(y),num);
                    }
                    else{
                        y=(char*)(txout.tokenLabel.TokenSymbol);
                        num=QString::number(txout.tokenLabel.value);//555
                        num = getAccuracyNumstr(QString::fromStdString(y),num);

                    }

                    isSpecileTran = true;

                    // sub.type = TransactionRecord::SendeCoin;
                    CTxDestination address;
                    std::string add;
                    if (ExtractDestination(txout.scriptPubKey, address))
                        add= CBitcoinAddress(address).ToString();


                    printf("add : %s \n\n",add.c_str());

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
                    CTxDestination address;
                    std::string add;
                    if(ExtractDestination(txout.scriptPubKey, address))
                        add= CBitcoinAddress(address).ToString();

                    QString m_status=FormatTxStatus(wtx);
                    QString m_strtime =FormatTxTime(wtx);
                    printf("自己给自己发送  -(nDebit - nChange), nCredit - nChange \n\n\n\n\n\n",-(nDebit - nChange), nCredit - nChange);
                    LOG_WRITE(LOG_INFO,"有一笔记账发生",hash.ToString().c_str(),m_strtime.toStdString().c_str(),add.c_str());

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
                printf("是普通交易 非找零 \n\n");
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

                for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++)
                {
                    const CTxOut& txout = wtx.tx->vout[nOut];

                    if(0 == txout.txType )//|| 1==txout.txType)
                    {
                        std::string goodadd;
                        std::map<int, std::string>::iterator iter = mapAdd.find( nCredit - nChange );
                        if (iter != mapAdd.end())
                        {
                            goodadd = iter->second;
                        }

                        CTxDestination address;
                        std::string add;
                        if(ExtractDestination(txout.scriptPubKey, address))
                            add= CBitcoinAddress(address).ToString();

                        QString m_status=FormatTxStatus(wtx);

                        QString m_strtime =FormatTxTime(wtx);
                        printf("自己给自己发送  -(nDebit - nChange), nCredit - nChange \n\n\n\n\n\n",-(nDebit - nChange), nCredit - nChange);
                        CAmount m_fee = nDebit - wtx.tx->GetValueOut();


                        parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendToSelf,goodadd,
                                                       -(nDebit - nChange), nChange -  nCredit ,m_status,m_strtime,m_fee));//txout.ipcLabel.labelTitle,txout.ipcLabel.ExtendType,

                        parts.last().involvesWatchAddress = involvesWatchAddress;// maybe pass to TransactionRecord as constructor argument

                        break;

                    }



                }


            }


        }
        else if (fAllFromMe && !fAllToMe)
        {
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
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }
                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {
                    if(2==txout.txType|| 3==txout.txType)//20170830
                    {
                         LOG_WRITE(LOG_INFO,"ipcdialog::updatalist0");
                        ipcdialog::updatalist();
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


                    else if(4==txout.txType || 5==txout.txType)
                    {
                        ECoinDialog::updatalist();
                        sub.type = TransactionRecord::SendeCoin;
                        const CTxOut& txout = wtx.tx->vout[nOut];
                        std::string y =(char*)(txout.tokenLabel.TokenSymbol);
                        //sub.amount = txout.tokenRegLabel.totalCount;
                        sub.ecoinType = y;



                        // int preci =GetAccuracySymbol(y);

                        sub.ecoinNum =QString::number(txout.tokenLabel.value);

                        sub.ecoinNum = getAccuracyNumstr(QString::fromStdString(y),sub.ecoinNum);




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
                for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++)
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
                            CAmount nValue = txout.nValue;

                            sub.debit = -nValue;

                            CAmount m_fee = nDebit - wtx.tx->GetValueOut();
                            sub.TxFee = m_fee;

                            parts.append(sub);
                            break;

                        }
                    }

                }


            }


        }
        else
        {
           // printf("not fAllFromMe \n\n\n\n\n");
            CAmount nTxFee = nDebit - wtx.tx->GetValueOut();
            for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.tx->vout[nOut];
                TransactionRecord sub(hash, nTime);
                sub.idx = nOut;
                sub.involvesWatchAddress = involvesWatchAddress;
                if(!wallet->IsMine(txout))
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {

                    if(2==txout.txType|| 3==txout.txType)//20170830
                    {
                         LOG_WRITE(LOG_INFO,"ipcdialog::updatalist0");
                        ipcdialog::updatalist();

                        sub.type = TransactionRecord::RecvIPC;
                        sub.address = CBitcoinAddress(address).ToString();
                        CAmount nValue = txout.nValue;
                        sub.credit = nValue;
                        std::string title = txout.ipcLabel.labelTitle;
                        sub.ipcTitle=txout.ipcLabel.labelTitle;
                        sub.strstatus=FormatTxStatus(wtx);
                        sub.strtime =FormatTxTime(wtx);
                        //20171013
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


                        parts.append(sub);
                        break;

                    }
                    // Sent to Bitcoin Address
                    else if(0==txout.txType)
                    {

                        sub.strtime = FormatTxTime(wtx);
                        sub.strstatus = FormatTxStatus(wtx);
                        sub.type = TransactionRecord::RecvCoin;
                        sub.address = CBitcoinAddress(address).ToString();
                        CAmount nValue = txout.nValue;
                        sub.credit = nValue;
                        parts.append(sub);

                        break;
                    }
                    else if(1==txout.txType)
                    {

                    }


                    else if(4==txout.txType || 5==txout.txType)
                    {
                        ECoinDialog::updatalist();
                        sub.type = TransactionRecord::RecveCoin;
                        const CTxOut& txout = wtx.tx->vout[nOut];
                        std::string y =(char*)(txout.tokenLabel.TokenSymbol);
                        sub.ecoinType =y;


                        sub.ecoinNum =QString::number(txout.tokenLabel.value);//555
                        sub.ecoinNum = getAccuracyNumstr(QString::fromStdString(y),sub.ecoinNum);

                        sub.strstatus = FormatTxStatus(wtx);
                        sub.strtime =FormatTxTime(wtx);
                        sub.address = CBitcoinAddress(address).ToString();
                        CAmount nValue = txout.nValue;
                        sub.credit = nValue;
                        parts.append(sub);
                        break;
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


    //  printf("插入一条数据后处理状态？ \n\n\n");
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
