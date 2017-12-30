// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//TransactionTableModel ---- TransactionNotification-------walletmode


#include "transactiontablemodel.h"

#include "addresstablemodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "transactiondesc.h"
#include "transactionrecord.h"
#include "walletmodel.h"

#include "core_io.h"
#include "validation.h"
#include "sync.h"
#include "uint256.h"
#include "util.h"
#include "wallet/wallet.h"
#include "log/log.h"

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QList>

#include <QSettings>
#include "util.h"
#include <QMutableListIterator>
#include <boost/foreach.hpp>
#include <sys/time.h>
// Amount column is right-aligned it contains numbers
static int column_alignments[] = {
    Qt::AlignLeft|Qt::AlignVCenter, /* status */
    Qt::AlignLeft|Qt::AlignVCenter, /* watchonly */
    Qt::AlignLeft|Qt::AlignVCenter, /* date */
    Qt::AlignLeft|Qt::AlignVCenter, /* type */
    Qt::AlignLeft|Qt::AlignVCenter, /* address */
    Qt::AlignRight|Qt::AlignVCenter /* amount */
};

// Comparison operator for sort/binary search of model tx list
struct TxLessThan
{
    bool operator()(const TransactionRecord &a, const TransactionRecord &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const TransactionRecord &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const TransactionRecord &b) const
    {
        return a < b.hash;
    }
};

// Private implementation
class TransactionTablePriv//20170820
{
public:
    TransactionTablePriv(CWallet *_wallet, TransactionTableModel *_parent) :
        wallet(_wallet),
        parent(_parent)
    {
    }

    CWallet *wallet;
    TransactionTableModel *parent;

    /* Local cache of wallet.
     * As it is in the same order as the CWallet, by definition
     * this is sorted by sha256.
     */
    QList<TransactionRecord> cachedWallet;

    QList<TransactionRecord> sdlist;
    QList<TransactionRecord> rdlist;
    QList<TransactionRecord> mergelist;
    QList<TransactionRecord> insertsdlist;
    QList<TransactionRecord> insertrdlist;
    int insert_idx;
    int insert_idy;
    int merge_idy;
    int insert_nidx;
    int insert_nidy;


    // template<class T>
    void resizeList(QList<TransactionRecord> & list, int newSize) {



        int diff = newSize - list.size();
        TransactionRecord t;
        if (diff > 0) {
        }
        else if(0 == diff)
        {

        }
        else if (diff < 0)
        {
            list.erase(list.begin() , list.begin()-diff);
        }
    }

    static bool compareBarData(const TransactionRecord &barAmount1, const TransactionRecord &barAmount2)
    {
        if (barAmount1.time < barAmount2.time)
        {
            return true;
        }
        return false;
    }

    static bool compareABarData(const TransactionRecord &barAmount1, const TransactionRecord &barAmount2)
    {
        if (barAmount1.time < barAmount2.time)
        {
            return false;
        }
        return true;
    }

    void handlenit()
    {
        qSort(insertsdlist.begin(), insertsdlist.end(), compareBarData);
        qSort(insertrdlist.begin(), insertrdlist.end(), compareBarData);


        QMutableListIterator<TransactionRecord> i(insertrdlist);
        QMutableListIterator<TransactionRecord>  j(insertsdlist);

        int min1 = std::min(insertrdlist.size(),insertsdlist.size())  ;
        //for(int i = 0;i< insertrdlist.size();i++)
        for(int i = 0;i< min1;i++)
        {

            if(insertsdlist.size() != 0 && insertrdlist.size() != 0)
            {
                TransactionRecord a =  insertsdlist.at(i);
                TransactionRecord b = insertrdlist.at(i);

                TransactionRecord c = b;

                b.credit = -a.credit;
                insertrdlist.replace(i,b);
            }
        }

    }



    void relistit()
    {

        qSort(mergelist.begin(), mergelist.end(), compareBarData);

        for(int i = 0;i<mergelist.size()-1;i++)
        {
            if(i+1>mergelist.size())
            {
                break;
            }

            TransactionRecord a =  mergelist.at(i);
            TransactionRecord b = mergelist.at(i+1);

            if(a.type == b.type)
            {
                mergelist.removeAt(i+1);
                i--;
            }
        }

        for(int i = 0;i<mergelist.size();i++)
        {
            if(TransactionRecord::Type::Recvdeposit== mergelist.at(i).type)
            {
                TransactionRecord a = mergelist.at(i);
                a.credit = 0;
                mergelist.replace(i,a);
                rdlist.append(a);
            }
            if(TransactionRecord::Type::Senddeposit== mergelist.at(i).type)
            {
                TransactionRecord b = mergelist.at(i);
                sdlist.append(b);
            }


            LOG_WRITE(LOG_INFO,"mergelist",QString::number(mergelist.at(i).credit).toStdString().c_str(),mergelist.at(i).strtime.toStdString().c_str(),mergelist.at(i).address.c_str());

        }



    }

   /*
    void handleit()
    {
        if(sdlist.size() != 0 )
        {
            qSort(sdlist.begin(), sdlist.end(), compareBarData);
        }
        if( rdlist.size() != 0 )
        {
            qSort(rdlist.begin(), rdlist.end(), compareBarData);
        }

        QMutableListIterator<TransactionRecord> i(rdlist);
        QMutableListIterator<TransactionRecord>  j(sdlist);

        int min_ = std::min(rdlist.size(),sdlist.size());
        // for(int i = 0;i< rdlist.size();i++)
        for(int i = 0;i< min_;i++)
        {
            if(rdlist.size() != 0 && sdlist.size() != 0 )
            {
                TransactionRecord a =  sdlist.at(i);
                TransactionRecord b = rdlist.at(i);

                TransactionRecord c = b;

                b.credit = - a.credit;

                rdlist.replace(i,b);
            }
        }

        if(rdlist.size()!=sdlist.size())
        {
            LOG_WRITE(LOG_INFO,"应该只发生在没有退出mark后",QString::number(sdlist.size()-rdlist.size()).toStdString().c_str());
            TransactionRecord lastrecord = sdlist.at(sdlist.size()-1);
            insertsdlist.insert(insert_nidx,lastrecord);
        }

    }
 */
    void handleit()
    {
        if(sdlist.size() != 0 )
        {
            qSort(sdlist.begin(), sdlist.end(), compareABarData);
        }
        if( rdlist.size() != 0 )
        {
            qSort(rdlist.begin(), rdlist.end(), compareABarData);
        }


        for(int i = 0;i<sdlist.size();i++)
        {
            LOG_WRITE(LOG_INFO,"sdlist",sdlist.at(i).strtime.toStdString().c_str());
        }

        for(int i = 0;i<rdlist.size();i++)
        {
            LOG_WRITE(LOG_INFO,"rdlist",rdlist.at(i).strtime.toStdString().c_str());
        }


        QMutableListIterator<TransactionRecord> i(rdlist);
        QMutableListIterator<TransactionRecord>  j(sdlist);

        int min_ = std::min(rdlist.size(),sdlist.size());
        // for(int i = 0;i< rdlist.size();i++)


        for(int i = 0;i< min_;i++)
        {
            if(rdlist.size() != 0 && sdlist.size() != 0 )
            {
                TransactionRecord a ,b;
                int t=sdlist.size()-rdlist.size();
               if( sdlist.at(0).time > rdlist.at(0).time && t>=1)
               {

                 a =  sdlist.at(i+1);
                 b = rdlist.at(i);

               }
               else
               {
                   a =  sdlist.at(i);
                    b = rdlist.at(i);
               }
                TransactionRecord c = b;

                b.credit = - a.credit;

                rdlist.replace(i,b);
            }
        }



        for(int i = 0;i<rdlist.size();i++)
        {
            LOG_WRITE(LOG_INFO,"afterrdlist",rdlist.at(i).strtime.toStdString().c_str());
        }

        if(rdlist.size() != 0 && sdlist.size() != 0 )
        {
        if(sdlist.at(0).time > rdlist.at(0).time)
       // if(rdlist.size()!=sdlist.size())
        {
            LOG_WRITE(LOG_INFO,"应该只发生在没有退出mark后",QString::number(sdlist.size()-rdlist.size()).toStdString().c_str());
           // TransactionRecord lastrecord = sdlist.at(sdlist.size()-1);
            TransactionRecord lastrecord = sdlist.at(0);

            insertsdlist.insert(insert_nidx,lastrecord);
        }
        }
        else if(rdlist.size() == 0 && sdlist.size() != 0)
        {
            TransactionRecord lastrecord = sdlist.at(0);

            insertsdlist.insert(insert_nidx,lastrecord);
        }

    }

    QString getCurrentTime(QString info)
    {
        QDateTime current_date_time = QDateTime::currentDateTime();
        QString current_date = current_date_time.toString("yyyy-MM-dd");
        QString current_time = current_date_time.toString("hh:mm:ss.zzz ");
        LOG_WRITE(LOG_INFO,info.toStdString().c_str(),current_time.toStdString().c_str());
        return current_time;
    }

    void refreshWallet()//20170822
    {
    getCurrentTime("refreshWallet  start");
        LOG_WRITE(LOG_INFO,"refreshWalletrefreshWalletrefreshWallet");
        qDebug() << "TransactionTablePriv::refreshWallet";
        time_t startTime = time(NULL);


        cachedWallet.clear();
        sdlist.clear();
        rdlist.clear();
        mergelist.clear();
        {
            LOCK2(cs_main, wallet->cs_wallet);
            for(std::map<uint256, CWalletTx>::iterator it = wallet->mapWallet.begin(); it != wallet->mapWallet.end(); ++it)
            {
                if(TransactionRecord::showTransaction(it->second))
                {
                    cachedWallet.append(TransactionRecord::decomposeTransaction(wallet, it->second));
                }
            }

            Q_FOREACH(const TransactionRecord &rec, cachedWallet)
            {
                //  TransactionRecord &rec =const_cast<TransactionRecord *>(cachedWallet.at(i));
                if(TransactionRecord::Type::Senddeposit == rec.type || TransactionRecord::Type::Recvdeposit == rec.type)
                {

                    mergelist.insert(merge_idy, rec);
                    merge_idy += 1;
                }


            }

            for(int i = 0;i<cachedWallet.size();i++)
            {

                TransactionRecord a =  cachedWallet.at(i);

                if( TransactionRecord::Type::Recvdeposit == a.type)
                {

                    a.credit =0;
                    cachedWallet.replace(i,a);

                }

            }


            for(int i = 0;i<mergelist.size();i++)
            {

                TransactionRecord a =  mergelist.at(i);

                if( TransactionRecord::Type::Recvdeposit == a.type)
                {

                    a.credit =0;
                    mergelist.replace(i,a);

                }

            }
            relistit();
            handleit();

            for(int i = 0;i<cachedWallet.size();i++)
            {

                if(TransactionRecord::Type::RecveCoin== cachedWallet.at(i).type || TransactionRecord::Type::RecvIPC== cachedWallet.at(i).type)
                {

                }
                else
                {
                    if(  0== cachedWallet.at(i).credit  &&  0== cachedWallet.at(i).debit )
                    {

                        {
                           cachedWallet.removeAt(i);
                            i--;
                        }
                    }
                    // break;
                }

            }
            for(int i = 0;i<cachedWallet.size();i++)
            {

                // while(true)
                if((cachedWallet.at(i).type == TransactionRecord::Type::RecvIPC) || (cachedWallet.at(i).type  == TransactionRecord::Type::RecveCoin))
                {

                }
                else
                {
                    if(  0== cachedWallet.at(i).credit  &&  0== cachedWallet.at(i).debit )
                    {

                        {
                            cachedWallet.removeAt(i);
                        }
                        // i--;

                    }
                    // break;
                }

            }
            for(int j = 0;j< rdlist.size();j++)
            {

                TransactionRecord aaaa =  rdlist.at(j);
                cachedWallet.append(aaaa);
            }

        }

        for(int i = 0;i<cachedWallet.size();i++)
        {

            if((cachedWallet.at(i).type == TransactionRecord::Type::RecvIPC) || (cachedWallet.at(i).type  == TransactionRecord::Type::RecveCoin))
            {

            }
            else
                // while(true)
            {
                if(  0== cachedWallet.at(i).credit  &&  0== cachedWallet.at(i).debit )
                {

                    {
                        cachedWallet.removeAt(i);
                        i--;
                    }

                }
                // break;
            }

        }

        qSort(cachedWallet.begin(), cachedWallet.end(), compareBarData);
        resizeList(cachedWallet,100);



        time_t stopTime = time(NULL);
          long elapsed = stopTime - startTime;
        LOG_WRITE(LOG_INFO,"刷新钱包用了多久",(QString::number(elapsed)).toStdString().c_str());
   getCurrentTime("refreshWallet  end");
    }

    /* Update our model of the wallet incrementally, to synchronize our model of the wallet
       with that of the core.

       Call with transaction that was added, removed or changed.
     */
    void updateWallet(const uint256 &hash, int status, bool showTransaction)
    {
       // LOG_WRITE(LOG_INFO,"updateWalletupdateWallet");
        qDebug() << "TransactionTablePriv::updateWallet: " + QString::fromStdString(hash.ToString()) + " " + QString::number(status);

        // Find bounds of this transaction in model
        QList<TransactionRecord>::iterator lower = qLowerBound(
                    cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
        QList<TransactionRecord>::iterator upper = qUpperBound(
                    cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
        int lowerIndex = (lower - cachedWallet.begin());
        int upperIndex = (upper - cachedWallet.begin());
        bool inModel = (lower != upper);
        if(status == CT_UPDATED)
        {
            if(showTransaction && !inModel)
                status = CT_NEW; /* Not in model, but want to show, treat as new */
            if(!showTransaction && inModel)
                status = CT_DELETED; /* In model, but want to hide, treat as deleted */
        }

        qDebug() << "    inModel=" + QString::number(inModel) +
                    " Index=" + QString::number(lowerIndex) + "-" + QString::number(upperIndex) +
                    " showTransaction=" + QString::number(showTransaction) + " derivedStatus=" + QString::number(status);


        switch(status)
        {
        case CT_NEW:




            if(inModel)
            {
                qWarning() << "TransactionTablePriv::updateWallet: Warning: Got CT_NEW, but transaction is already in model";
                break;
            }

            if(showTransaction)
            {

                time_t startTime = time(NULL);
                LOCK2(cs_main, wallet->cs_wallet);
                // Find transaction in wallet
                std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(hash);
                if(mi == wallet->mapWallet.end())
                {
                    qWarning() << "TransactionTablePriv::updateWallet: Warning: Got CT_NEW, but transaction is not in wallet";
                    break;//wangdandan201710101
                }
                // Added -- insert at the right position

                QList<TransactionRecord> toInsert;
                toInsert.clear();
                toInsert =TransactionRecord::decomposeTransaction(wallet, mi->second);

                Q_FOREACH( const TransactionRecord &rec , toInsert){
                    bool findFlag = false;

                    BOOST_FOREACH(const TransactionRecord &cacheData, cachedWallet)
                    {
                        //if(cacheData.EqualTo(rec.hash)){
                        if(cacheData.hash == rec.hash){
                            findFlag = true;
                            break;
                        }
                    }
                    if(true == findFlag)
                    {

                        toInsert.clear();
                        break;
                    }
                }
                Q_FOREACH(const TransactionRecord &insertrec, toInsert)
                {
                    //  TransactionRecord &rec =const_cast<TransactionRecord *>(cachedWallet.at(i));
                    if(TransactionRecord::Type::Senddeposit == insertrec.type)
                    {

                        insertsdlist.insert(insert_nidx, insertrec);
                        insert_nidx += 1;

                        LOG_WRITE(LOG_INFO,"应该只能是1,0 ，不然会出问题的insertsdlist.size()",QString::number(insertsdlist.size()).toStdString().c_str(),QString::number(insertrdlist.size()).toStdString().c_str());
                    }

                    if(TransactionRecord::Type::Recvdeposit == insertrec.type)
                    {

                        insertrdlist.insert(insert_nidx, insertrec);
                        insert_nidy += 1;

                        LOG_WRITE(LOG_INFO,"应该只能是1,1 ，不然会出问题的insertrdlist.size()",QString::number(insertrdlist.size()).toStdString().c_str(),QString::number(insertsdlist.size()).toStdString().c_str());
                    }

                }
                if(insertrdlist.size()>0)
                {
                    handlenit();
                    for(int i = 0;i<toInsert.size();i++)
                    {

                        // while(true)
                        if( toInsert.at(i).type ==TransactionRecord::Type::RecveCoin || toInsert.at(i).type ==TransactionRecord::Type::RecvIPC)
                        {
                        }
                        else
                        {
                            if(  0== toInsert.at(i).credit && 0== toInsert.at(i).debit )
                            {
                                LOG_WRITE(LOG_INFO,"应该只删除一条数据");

                                {

                                    toInsert.removeAt(i);
                                    i--;
                                }
                            }
                            // break;
                        }

                    }


                    for(int j = 0;j< insertrdlist.size();j++)
                    {
                        TransactionRecord aaaa =  insertrdlist.at(j);

                        LOG_WRITE(LOG_INFO,"应该只插入一条数据");
                        toInsert.append(aaaa);

                    }


                    //再次除0

                    for(int i = 0;i<toInsert.size();i++)
                    {


                        if( toInsert.at(i).type ==TransactionRecord::Type::RecveCoin || toInsert.at(i).type ==TransactionRecord::Type::RecvIPC)
                        {
                            //break;
                        }
                        else
                            // while(true)
                        {
                            if(  0== toInsert.at(i).credit && 0== toInsert.at(i).debit )
                            {

                                {
                                    LOG_WRITE(LOG_INFO,"nobk2",QString::number(cachedWallet.size()).toStdString().c_str(),QString::number(i).toStdString().c_str());

                                    toInsert.removeAt(i);
                                    i--;
                                }

                            }
                            //  break;
                        }

                    }

                    insertsdlist.clear();

                    insertrdlist.clear();



                }
                //对新插入的数据进行处理

                if(!toInsert.isEmpty()) /* only if something to insert */
                {

                    parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                    int insert_idx = lowerIndex;


                    Q_FOREACH(const TransactionRecord &rec, toInsert)
                    {
                        //
                        if( 0 == rec.credit && 0 == rec.debit)
                        {
                            if(rec.type == TransactionRecord::Type::RecvIPC || rec.type == TransactionRecord::Type::RecveCoin)
                            {
                            }
                            else
                            {
                                LOG_WRITE(LOG_INFO,"插入之前不应该有都为0的数据了吧");

                                break;
                            }
                        }
                        //这块加不加都行
                        int idx = 0;
                        bool findFlag = false;
                        BOOST_FOREACH(const TransactionRecord &cacheData, cachedWallet)
                        {
                            //if(cacheData.EqualTo(rec.hash)){
                            if(cacheData.hash == rec.hash){
                                findFlag = true;
                                break;
                            }
                        }
                        if(true == findFlag)
                        {
                            printf("break \n");
                            break;
                        }
                        ++ idx;

                        //这块加不加都行

                        // cachedWallet.insert(insert_idx, rec);
                        // insert_idx += 1;
                        cachedWallet.append(rec);

                    }
                    parent->endInsertRows();

                    if(cachedWallet.size()>100)
                    {

                        parent->beginResetModel();
                        qSort(cachedWallet.begin(), cachedWallet.end(), compareBarData);

                        resizeList(cachedWallet,100);
                        parent->endResetModel();


                    }
                }

               // time_t stopTime = time(NULL);
               //   long elapsed = stopTime - startTime;
             //   LOG_WRITE(LOG_INFO,"处理新消息用了多久",(QString::number(elapsed)).toStdString().c_str());


            }




            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "TransactionTablePriv::updateWallet: Warning: Got CT_DELETED, but transaction is not in model";
                break;
            }
            // Removed -- remove entire transaction from table
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedWallet.erase(lower, upper);
            parent->endRemoveRows();
            break;
        case CT_UPDATED:
            // Miscellaneous updates -- nothing to do, status update will take care of this, and is only computed for
            // visible transactions.
            break;
        }


    }

    int size()
    {
        return cachedWallet.size();
    }

    TransactionRecord *index(int idx)
    {

        if(idx >= 0 && idx < cachedWallet.size())
        {
            TransactionRecord *rec = &cachedWallet[idx];

            // Get required locks upfront. This avoids the GUI from getting
            // stuck if the core is holding the locks for a longer time - for
            // example, during a wallet rescan.
            //
            // If a status update is needed (blocks came in since last check),
            //  update the status of this transaction from the wallet. Otherwise,
            // simply re-use the cached status.
            TRY_LOCK(cs_main, lockMain);
            if(lockMain)
            {
                TRY_LOCK(wallet->cs_wallet, lockWallet);
                if(lockWallet && rec->statusUpdateNeeded())
                {
                    std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);
                    if(mi != wallet->mapWallet.end())
                    {

                        rec->updateStatus(mi->second);
                    }
                }
            }
            return rec;
        }
        return 0;
    }

    QString describe(TransactionRecord *rec, int unit)
    {
        {
            LOCK2(cs_main, wallet->cs_wallet);
            std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);
            if(mi != wallet->mapWallet.end())
            {
               // return TransactionDesc::toHTML(wallet, mi->second, rec, unit);
            }
        }
        return QString();
    }

    QString getTxHex(TransactionRecord *rec)
    {
        LOCK2(cs_main, wallet->cs_wallet);
        std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);
        if(mi != wallet->mapWallet.end())
        {
            std::string strHex = EncodeHexTx(static_cast<CTransaction>(mi->second));
            return QString::fromStdString(strHex);
        }
        return QString();
    }
};

TransactionTableModel::TransactionTableModel(const PlatformStyle *_platformStyle, CWallet* _wallet, WalletModel *parent):
    QAbstractTableModel(parent),
    wallet(_wallet),
    walletModel(parent),
    priv(new TransactionTablePriv(_wallet, this)),
    fProcessingQueuedTransactions(false),
    platformStyle(_platformStyle)
{
    columns << QString() << QString() << tr("Date") << tr("Type") << tr("Label") << BitcoinUnits::getAmountColumnTitle(walletModel->getOptionsModel()->getDisplayUnit());
    priv->refreshWallet();
    connect(walletModel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    subscribeToCoreSignals();
}

TransactionTableModel::~TransactionTableModel()
{
    unsubscribeFromCoreSignals();
    delete priv;
}

/** Updates the column title to "Amount (DisplayUnit)" and emits headerDataChanged() signal for table headers to react. */
void TransactionTableModel::updateAmountColumnTitle()
{
    columns[Amount] = BitcoinUnits::getAmountColumnTitle(walletModel->getOptionsModel()->getDisplayUnit());
    Q_EMIT headerDataChanged(Qt::Horizontal,Amount,Amount);
}

void TransactionTableModel::updateTransaction(const QString &hash, int status, bool showTransaction)
{
    uint256 updated;
    updated.SetHex(hash.toStdString());
//LOG_WRITE(LOG_INFO,"TransactionTableModel::updateTransactionTransactionTableModel::updateTransaction");
    priv->updateWallet(updated, status, showTransaction);
}

void TransactionTableModel::updateConfirmations()
{
    Q_EMIT dataChanged(index(0, Status), index(priv->size()-1, Status));
    Q_EMIT dataChanged(index(0, ToAddress), index(priv->size()-1, ToAddress));
}

int TransactionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    //LOG_WRITE(LOG_INFO,"行num",QString::number(priv->size()).toStdString().c_str());
    return priv->size();
}

int TransactionTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}
bool TransactionTableModel::IsformatTxStatus(const TransactionRecord *wtx) const
{
    bool m_IsConfirmedRole;

    switch(wtx->status.status)
    {
    case TransactionStatus::OpenUntilBlock:
    {
        m_IsConfirmedRole =false;
    }
        break;
    case TransactionStatus::OpenUntilDate:
    {
        m_IsConfirmedRole =false;
    }
        break;
    case TransactionStatus::Offline:
    {

        m_IsConfirmedRole =false;
    }
        break;
    case TransactionStatus::Unconfirmed:
    {
        m_IsConfirmedRole =false;

    }
        break;
    case TransactionStatus::Abandoned:
    {
        m_IsConfirmedRole =false;

    }
        break;
    case TransactionStatus::Confirming:
    {
        m_IsConfirmedRole =false;
    }
        break;
    case TransactionStatus::Confirmed:
    {

        m_IsConfirmedRole =true;

    }
        break;
    case TransactionStatus::Conflicted:
    {
        m_IsConfirmedRole =false;

    }
        break;
    case TransactionStatus::Immature:
    {
        m_IsConfirmedRole =false;
    }
        break;
    case TransactionStatus::MaturesWarning:
    {
        m_IsConfirmedRole =false;
    }
        break;
    case TransactionStatus::NotAccepted:
    {

        m_IsConfirmedRole =false;
    }
        break;

    }
    return m_IsConfirmedRole;
}


void TransactionTableModel::setCurrencyMap(const QMap<QString, double> &map)
{
    // currencyMap = map;
    //重置模型至原始状态，告诉所有视图，他们数据都无效，强制刷新数据
    // reset();
}




QString TransactionTableModel::FormatTxStatusRecord(const TransactionRecord *wtx) const
{

    QString status;
    if(TransactionStatus::OpenUntilBlock == wtx->status.status)
    {
        status = tr("Open for %n more block(s)","",wtx->status.open_for);
    }
    else if(TransactionStatus::OpenUntilDate ==wtx->status.status )
    {
        status = tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx->status.open_for));

    }
    else if(TransactionStatus::Offline ==wtx->status.status)
    {
        status = tr("%1/offline").arg(wtx->status.depth);

    }
    else
    {
        int nDepth = wtx->status.depth;
        if (nDepth < 0)
        {
            status=QObject::tr("conflicted with a transaction with %1 confirmations").arg(-nDepth);
        }
        //   else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        //  {
        //    return QObject::tr("%1/offline").arg(wtx.GetDepthInMainChain());
        // return tr("%1/offline").arg(nDepth);
        //}
        //   else if (nDepth == 0)
        //   {
        //       return QObject::tr("0/unconfirmed, %1").arg((wtx.InMempool() ? QObject::tr("in memory pool") : QObject::tr("not in memory pool"))) + (wtx.isAbandoned() ? ", "+QObject::tr("abandoned") : "");
        //    }
        else if(0== nDepth)
        {
            status= tr("unconfirmed");
        }
        else if (nDepth < 8)
        {
            status=tr("%1/unconfirmed").arg(nDepth);

            //return tr("%1/unconfirmed").arg(nDepth);
        }
        else
        {
            status=tr("%1 confirmations").arg(nDepth);

            // return tr("%1 confirmations").arg(nDepth);
        }

    }




    return status;
}

















QString TransactionTableModel::formatTxStatus(const TransactionRecord *wtx) const
{
    QString status;

    switch(wtx->status.status)
    {
    case TransactionStatus::OpenUntilBlock:
    {
        status = tr("Open for %n more block(s)","",wtx->status.open_for);


    }
        break;
    case TransactionStatus::OpenUntilDate:
    {
        status = tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx->status.open_for));

    }
        break;
    case TransactionStatus::Offline:
    {
        status = tr("Offline");

    }
        break;
    case TransactionStatus::Unconfirmed:
    {
        status = tr("Unconfirmed");
    }
        break;
    case TransactionStatus::Abandoned:
    {
        status = tr("Abandoned");
    }
        break;
    case TransactionStatus::Confirming:
    {
        status = tr("Confirming (%1 of %2 recommended confirmations)").arg(wtx->status.depth).arg(TransactionRecord::RecommendedNumConfirmations);
    }
        break;
    case TransactionStatus::Confirmed:
    {
        status = tr("Confirmed (%1 confirmations)").arg(wtx->status.depth);
    }
        break;
    case TransactionStatus::Conflicted:
    {
        status = tr("Conflicted");
    }
        break;
    case TransactionStatus::Immature:
    {
        status = tr("Immature (%1 confirmations, will be available after %2)").arg(wtx->status.depth).arg(wtx->status.depth + wtx->status.matures_in);
    }
        break;
    case TransactionStatus::MaturesWarning:
    {
        status = tr("This block was not received by any other nodes and will probably not be accepted!");
    }
        break;
    case TransactionStatus::NotAccepted:
    {
        status = tr("Generated but not accepted");
    }
        break;

    }
    return status;
}

QString TransactionTableModel::formatTxDate(const TransactionRecord *wtx) const
{
    if(wtx->time)
    {
        return GUIUtil::dateTimeStr(wtx->time);
    }
    return QString();
}

/* Look up address in address book, if found return label (address)
   otherwise just return (address)
 */
QString TransactionTableModel::lookupAddress(const std::string &address, bool tooltip) const
{
    QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(address));
    QString description;
    if(!label.isEmpty())
    {
        description += label;
    }
    if(label.isEmpty() || tooltip)
    {
        description += QString(" (") + QString::fromStdString(address) + QString(")");
    }
    return description;
}

QString TransactionTableModel::formatTxType(const TransactionRecord *wtx) const
{
    switch(wtx->type)
    {
    case TransactionRecord::RecvWithAddress:
        return tr("Received with");
    case TransactionRecord::RecvFromOther:
        return tr("Received from");
    case TransactionRecord::SendToAddress:
    case TransactionRecord::SendToOther:
        return tr("Sent to");
    case TransactionRecord::SendToSelf:
        return tr("Payment to yourself");
    case TransactionRecord::Generated:
        return tr("Mined");
    default:
        return QString();
    }
}

QVariant TransactionTableModel::txAddressDecoration(const TransactionRecord *wtx) const
{

    //  bool confirmed = wtx->data(TransactionTableModel::ConfirmedRole).toBool();
    switch(wtx->type)
    {
    case TransactionRecord::Generated:
        // return QIcon(":/icons/tx_mined");
        // return QIcon(":/icons/tx_input");
        return QIcon(":/res/png/tx_input.png");
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::RecvFromOther:
    case TransactionRecord::RecvIPC:
    case TransactionRecord::RecvCoin:
    case TransactionRecord::RecveCoin:
    case TransactionRecord::Recvbookkeep:
    case TransactionRecord::Recvdeposit:
        return QIcon(":/res/png/tx_input.png");
    case TransactionRecord::SendToAddress:
    case TransactionRecord::SendToOther:
    case TransactionRecord::SendToSelf:
    case TransactionRecord::SendIPC:
    case TransactionRecord::SendCoin:
    case TransactionRecord::SendeCoin:
    case TransactionRecord::Sendbookkeep:
    case TransactionRecord::Senddeposit:
    case TransactionRecord::Other:
        return QIcon(":/res/png/tx_output.png");
    default:
        return QIcon(":/res/png/tx_inout.png");
    }
}

QString TransactionTableModel::formatTxToAddress(const TransactionRecord *wtx, bool tooltip) const
{
    QString watchAddress;
    if (tooltip) {
        // Mark transactions involving watch-only addresses by adding " (watch-only)"
        watchAddress = wtx->involvesWatchAddress ? QString(" (") + tr("watch-only") + QString(")") : "";
    }

    switch(wtx->type)
    {
    case TransactionRecord::RecvFromOther:
        return QString::fromStdString(wtx->address) + watchAddress;
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::SendToAddress:
    case TransactionRecord::Generated:
        return lookupAddress(wtx->address, tooltip) + watchAddress;
    case TransactionRecord::SendToOther:
        return QString::fromStdString(wtx->address) + watchAddress;
    case TransactionRecord::SendToSelf:
    case TransactionRecord::SendeCointoself:
    default:
        return tr("(n/a)") + watchAddress;
    }
}

QVariant TransactionTableModel::addressColor(const TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::SendToAddress:
    case TransactionRecord::Generated:
    {
        QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
        if(label.isEmpty())
            return COLOR_BAREADDRESS;
    } break;
    case TransactionRecord::SendToSelf:
        return COLOR_BAREADDRESS;
    default:
        break;
    }
    return QVariant();
}

QString TransactionTableModel::formatTxAmount(const TransactionRecord *wtx, bool showUnconfirmed, BitcoinUnits::SeparatorStyle separators) const
{
    QString str = BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), wtx->credit + wtx->debit, false, separators);
    if(showUnconfirmed)
    {
        if(!wtx->status.countsForBalance)
        {
            str = QString("[") + str + QString("]");
        }
    }
    return QString(str);
}



QString TransactionTableModel::formatLang() const
{
    QSettings settings;
    // Get desired locale (e.g. "de_DE")
    // 1) System default language
    QString lang_territory = QLocale::system().name();
    // 2) Language from QSettings
    QString lang_territory_qsettings = settings.value("language", "").toString();
    if(!lang_territory_qsettings.isEmpty())
        lang_territory = lang_territory_qsettings;
    // 3) -lang command line argument
    lang_territory = QString::fromStdString(GetArg("-lang", lang_territory.toStdString()));


    QLocale locale;
    if( locale.language() == QLocale::English )  //获取系统语言环境
    {
        lang_territory ="English";
    }
    else if( locale.language() == QLocale::Chinese )
    {
        lang_territory ="Chinese";
    }


    return lang_territory;
}


QVariant TransactionTableModel::txStatusDecoration(const TransactionRecord *wtx) const
{
    switch(wtx->status.status)
    {
    case TransactionStatus::OpenUntilBlock:
    case TransactionStatus::OpenUntilDate:
        return COLOR_TX_STATUS_OPENUNTILDATE;
    case TransactionStatus::Offline:
        return COLOR_TX_STATUS_OFFLINE;
    case TransactionStatus::Unconfirmed:
        return QIcon(":/icons/transaction_0");
    case TransactionStatus::Abandoned:
        return QIcon(":/icons/transaction_abandoned");
    case TransactionStatus::Confirming:
        switch(wtx->status.depth)
        {
        case 1: return QIcon(":/icons/transaction_1");
        case 2: return QIcon(":/icons/transaction_2");
        case 3: return QIcon(":/icons/transaction_3");
        case 4: return QIcon(":/icons/transaction_4");
        default: return QIcon(":/icons/transaction_5");
        };
    case TransactionStatus::Confirmed:
        return QIcon(":/icons/transaction_confirmed");
    case TransactionStatus::Conflicted:
        return QIcon(":/icons/transaction_conflicted");
    case TransactionStatus::Immature: {
        int total = wtx->status.depth + wtx->status.matures_in;
        int part = (wtx->status.depth * 4 / total) + 1;
        return QIcon(QString(":/icons/transaction_%1").arg(part));
    }
    case TransactionStatus::MaturesWarning:
    case TransactionStatus::NotAccepted:
        return QIcon(":/icons/transaction_0");
    default:
        return COLOR_BLACK;
    }
}

QVariant TransactionTableModel::txWatchonlyDecoration(const TransactionRecord *wtx) const
{
    if (wtx->involvesWatchAddress)
        return QIcon(":/icons/eye");
    else
        return QVariant();
}

QString TransactionTableModel::formatTooltip(const TransactionRecord *rec) const
{
    QString tooltip = formatTxStatus(rec) + QString("\n") + formatTxType(rec);
    if( TransactionRecord::RecvFromOther == rec->type  ||  TransactionRecord::SendToOther ==rec->type ||
            TransactionRecord::SendToAddress ==  rec->type || TransactionRecord::RecvWithAddress==rec->type  ||
            TransactionRecord::SendIPC==rec->type  ||  TransactionRecord::RecvIPC==rec->type )
    {
        // tooltip += QString(" ") + formatTxToAddress(rec, true);
    }
    return tooltip;
}
///////////////赋值模型每个项的数据

QVariant TransactionTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    TransactionRecord *rec = static_cast<TransactionRecord*>(index.internalPointer());
    switch(role)//2017101220171012
    {
    case RawDecorationRole:
        switch(index.column())
        {
        case Status:
            return txStatusDecoration(rec);
        case Watchonly:
            return txWatchonlyDecoration(rec);
        case ToAddress:
            return txAddressDecoration(rec);
        }
        break;
    case Qt::DecorationRole:
    {
        QIcon icon = qvariant_cast<QIcon>(index.data(RawDecorationRole));
        return platformStyle->TextColorIcon(icon);
    }
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Date:
            return formatTxDate(rec);
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec, false);
        case Amount:
            return formatTxAmount(rec, true, BitcoinUnits::separatorAlways);
        }
        break;
    case Qt::EditRole:
        // Edit role is used for sorting, so return the unformatted values
        switch(index.column())
        {
        case Status:
            return QString::fromStdString(rec->status.sortKey);
        case Date:
            return rec->time;
        case Type:
            return formatTxType(rec);
        case Watchonly:
        {
            return (rec->involvesWatchAddress ? 1 : 0);
            //  return (rec->type == TransactionRecord::SendToSelf ? 1 : 0);//20171001
        }
        case ToAddress:
            return formatTxToAddress(rec, true);
        case Amount:
            return qint64(rec->credit + rec->debit);//20170901
        case overAmount:
            return qint64(rec->debit);

        }
        break;
    case Qt::ToolTipRole://2017101220171012
        return formatTooltip(rec);
    case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
    case Qt::ForegroundRole:
        // Use the "danger" color for abandoned transactions
        if(rec->status.status == TransactionStatus::Abandoned)
        {
            return COLOR_TX_STATUS_DANGER;
        }
        // Non-confirmed (but not immature) as transactions are grey
        if(!rec->status.countsForBalance && rec->status.status != TransactionStatus::Immature)
        {
            return COLOR_UNCONFIRMED;
        }
        if(index.column() == Amount && (rec->credit+rec->debit) < 0)
        {
            return COLOR_NEGATIVE;
        }
        if(index.column() == ToAddress)
        {
            return addressColor(rec);
        }
        break;
    case TypeRole:
        return rec->type;
    case IPCType:
        return rec->ipcType;
    case IPCTitle:
        return QString::fromStdString(rec->ipcTitle);



    case eCoinType:
        return QString::fromStdString(rec->ecoinType);
    case eCoinNum:
        return rec->ecoinNum;
    case DateRole:
        return (QDateTime::fromTime_t(static_cast<uint>(rec->time))).toString("yyyy-MM-dd hh:mm:ss");
    case WatchonlyRole:
        return rec->involvesWatchAddress;
    case WatchonlyDecorationRole:
        return txWatchonlyDecoration(rec);
    case LongDescriptionRole:
        return priv->describe(rec, walletModel->getOptionsModel()->getDisplayUnit());
    case AddressRole:
        return QString::fromStdString(rec->address);
    case LabelRole:
        return walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));
    case LanguageRole:
        return formatLang();

    case feeAmount:
        return qint64(rec->TxFee);


    case AmountRole:
        if( TransactionRecord::Type::Senddeposit== rec->type ||TransactionRecord::Type::SendToSelf== rec->type || TransactionRecord::Type::Recvdeposit== rec->type)
        {
            return qint64(rec->credit) ;
        }
        else if(TransactionRecord::Type::SendCoin== rec->type)
        {
            return qint64(rec->debit);//+ rec->debit);
        }
        else
        {
            return qint64(rec->credit+ rec->debit);
        }

        //不要HTML截取字符串了
    case InfoStatus:
        return FormatTxStatusRecord(rec);
        // return rec->strstatus;

    case InfoTime:
        return rec->strtime;

        //不要HTML截取字符串了

    case AuthTime:
        return rec->authTime;
    case AuthLimit:
        return rec->authLimit;
    case AuthType:
        return rec->authType;
    case overAmount:
        return qint64(rec->debit);
    case TxIDRole:
        return rec->getTxID();
    case TxHashRole:
        return QString::fromStdString(rec->hash.ToString());
    case TxHexRole:
        return priv->getTxHex(rec);
    case TxPlainTextRole:
    {
        QString details;
        QDateTime date = QDateTime::fromTime_t(static_cast<uint>(rec->time));
        QString txLabel = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));

        details.append(date.toString("M/d/yy HH:mm"));
        details.append(" ");
        details.append(formatTxStatus(rec));
        details.append(". ");
        if(!formatTxType(rec).isEmpty()) {
            details.append(formatTxType(rec));
            details.append(" ");
        }
        if(!rec->address.empty()) {
            if(txLabel.isEmpty())
                details.append(tr("(no label)") + " ");
            else {
                details.append("(");
                details.append(txLabel);
                details.append(") ");
            }
            details.append(QString::fromStdString(rec->address));
            details.append(" ");
        }
        details.append(formatTxAmount(rec, false, BitcoinUnits::separatorNever));
        return details;
    }
    case ConfirmedRole:
        return rec->status.countsForBalance;
    case IsConfirmedRole:
        return IsformatTxStatus(rec);



    case FormattedAmountRole:
        // Used for copy/export, so don't include separators
        return formatTxAmount(rec, false, BitcoinUnits::separatorNever);
    case StatusRole:
        return rec->status.status;
    }
    return QVariant();
}

QVariant TransactionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return columns[section];
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return column_alignments[section];
        } else if (role == Qt::ToolTipRole)
        {
            switch(section)
            {
            case Status:
                return tr("Transaction status. Hover over this field to show number of confirmations.");
            case Date:
                return tr("Date and time that the transaction was received.");
            case Type:
                return tr("Type of transaction.");
            case Watchonly:
                return tr("Whether or not a watch-only address is involved in this transaction.");
            case ToAddress:
                return tr("User-defined intent/purpose of the transaction.");
            case Amount:
                return tr("Amount removed from or added to balance.");
            }
        }
    }
    return QVariant();
}
//20171014201710142017101420171014
QModelIndex TransactionTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    TransactionRecord *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    return QModelIndex();
}

void TransactionTableModel::updateDisplayUnit()
{

    // emit dataChanged to update Amount column with the current unit
    updateAmountColumnTitle();

    Q_EMIT dataChanged(index(0, Amount), index(priv->size()-1, Amount));
}

// queue notifications to show a non freezing progress dialog e.g. for rescan
struct TransactionNotification
{
public:
    TransactionNotification() {}
    TransactionNotification(uint256 _hash, ChangeType _status, bool _showTransaction):
        hash(_hash), status(_status), showTransaction(_showTransaction) {}

    void invoke(QObject *ttm)
    {

       // LOG_WRITE(LOG_INFO,"TransactionNotification::invoke");
        QString strHash = QString::fromStdString(hash.GetHex());
        qDebug() << "NotifyTransactionChanged: " + strHash + " status= " + QString::number(status);
        QMetaObject::invokeMethod(ttm, "updateTransaction", Qt::QueuedConnection,
                                  Q_ARG(QString, strHash),
                                  Q_ARG(int, status),
                                  Q_ARG(bool, showTransaction));
    }
private:
    uint256 hash;
    ChangeType status;
    bool showTransaction;
};

static bool fQueueNotifications = false;
static std::vector< TransactionNotification > vQueueNotifications;

static void NotifyTransactionChanged(TransactionTableModel *ttm, CWallet *wallet, const uint256 &hash, ChangeType status)
{
   // LOG_WRITE(LOG_INFO,"NotifyTransactionChangedNotifyTransactionChanged");
    // Find transaction in wallet
    std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(hash);
    // Determine whether to show transaction or not (determine this here so that no relocking is needed in GUI thread)
    bool inWallet = mi != wallet->mapWallet.end();
    bool showTransaction = (inWallet && TransactionRecord::showTransaction(mi->second));

    TransactionNotification notification(hash, status, showTransaction);

    if (fQueueNotifications)
    {
        vQueueNotifications.push_back(notification);
        return;
    }
    notification.invoke(ttm);

}

static void ShowProgress(TransactionTableModel *ttm, const std::string &title, int nProgress)
{
    if (nProgress == 0)
        fQueueNotifications = true;

    if (nProgress == 100)
    {
        fQueueNotifications = false;
        if (vQueueNotifications.size() > 10) // prevent balloon spam, show maximum 10 balloons
            QMetaObject::invokeMethod(ttm, "setProcessingQueuedTransactions", Qt::QueuedConnection, Q_ARG(bool, true));
        for (unsigned int i = 0; i < vQueueNotifications.size(); ++i)
        {
            if (vQueueNotifications.size() - i <= 10)
                QMetaObject::invokeMethod(ttm, "setProcessingQueuedTransactions", Qt::QueuedConnection, Q_ARG(bool, false));

            vQueueNotifications[i].invoke(ttm);
        }
        std::vector<TransactionNotification >().swap(vQueueNotifications); // clear
    }
}

void TransactionTableModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    wallet->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
}

void TransactionTableModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
}
