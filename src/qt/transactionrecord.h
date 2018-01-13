// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TRANSACTIONRECORD_H
#define BITCOIN_QT_TRANSACTIONRECORD_H

#include "amount.h"
#include "uint256.h"

#include <QList>
#include <QString>
#include <QDateTime>
#include <QObject>
#include <map>
class CWallet;
class CWalletTx;

struct TranInfo
{
    std::string add;
    QString status;
    QString time;
    int64_t ntime ;
    uint256 nhash;
};
struct TranTinfo
{
    int64_t time ;
    CAmount money;
};


/** UI model for transaction status. The transaction status is the part of a transaction that will change over time.
 */
class TransactionStatus
{

public:
    TransactionStatus():
        countsForBalance(false), sortKey(""),
        matures_in(0), status(Offline), depth(0), open_for(0), cur_num_blocks(-1)
    { }

    enum Status {
        Confirmed,          /**< Have 6 or more confirmations (normal tx) or fully mature (mined tx) **/
        /// Normal (sent/received) transactions
        OpenUntilDate,      /**< Transaction not yet final, waiting for date */
        OpenUntilBlock,     /**< Transaction not yet final, waiting for block */
        Offline,            /**< Not sent to any other nodes **/
        Unconfirmed,        /**< Not yet mined into a block **/
        Confirming,         /**< Confirmed, but waiting for the recommended number of confirmations **/
        Conflicted,         /**< Conflicts with other transaction or mempool **/
        Abandoned,          /**< Abandoned from the wallet **/
        /// Generated (mined) transactions
        Immature,           /**< Mined but waiting for maturity */
        MaturesWarning,     /**< Transaction will likely not mature because no nodes have confirmed */
        NotAccepted         /**< Mined but not accepted */
    };

    /// Transaction counts towards available balance
    bool countsForBalance;
    /// Sorting key based on status
    std::string sortKey;

    /** @name Generated (mined) transactions
       @{*/
    int matures_in;
    /**@}*/

    /** @name Reported status
       @{*/
    Status status;
    qint64 depth;
    qint64 open_for; /**< Timestamp if status==OpenUntilDate, otherwise number
                      of additional blocks that need to be mined before
                      finalization */
    /**@}*/

    /** Current number of blocks (to know whether cached status is still valid) */
    int cur_num_blocks;
};
QString IntTimeToQStringTime(int t);
/** UI model for a transaction. A core transaction can be represented by multiple UI transactions if it has
    multiple outputs.
 */
static CWallet *wallet_ =nullptr;
static uint256 hash_ ;
static std::map<int, std::string> mapAdd;
static TranInfo m_traninfo;
static bool m_ismarked = false ;
static int push_num = 0;
static int pull_num =0;
static int pull_num_ =0 ;
class TransactionRecord
{
    //Q_OBJECT
public:
    enum Type
    {
        Other = 0,
        SendCoin,
        Sendbookkeep,
        Generated,
        SendToAddress,
        SendToOther,
        RecvWithAddress,
        RecvFromOther,
        SendToSelf,
        SendIPC,
        RecvIPC,
        SendeCoin,
        RecveCoin,
        Recvbookkeep,
        RecvCoin,
        SendeCointoself,
        Recvdeposit,
        Senddeposit,
    };

    /** Number of confirmation recommended for accepting a transaction */
    static const int RecommendedNumConfirmations = 8;

    TransactionRecord():
        hash(), time(0), type(Other), address(""), debit(0), credit(0), idx(0)
    {

    }

    TransactionRecord(uint256 _hash, qint64 _time):
        hash(_hash), time(_time), type(Other), address(""), debit(0),
        credit(0), idx(0)
    {

    }

    TransactionRecord(uint256 _hash, qint64 _time,
                      Type _type, const std::string &_address,
                      const CAmount& _debit, const CAmount& _credit,const QString &status,const QString &_strtime, const CAmount& _fee):
        hash(_hash), time(_time), type(_type),address(_address), debit(_debit), credit(_credit),
        idx(0),strstatus(status),strtime(_strtime),TxFee(_fee)
    {

    }

    TransactionRecord(uint256 _hash, qint64 _time,
                      Type _type, const std::string &_address,
                      const CAmount& _debit, const CAmount& _credit,const std::string &ecointype,const QString &_ecoinnum,const QString &status ,const QString &_strtime, const CAmount& _fee):
        hash(_hash), time(_time), type(_type),address(_address), debit(_debit), credit(_credit),ecoinType(ecointype),ecoinNum(_ecoinnum),
        idx(0),strstatus(status),strtime(_strtime),TxFee(_fee)
    {

    }


    TransactionRecord(uint256 _hash, qint64 _time,
                      Type _type, const std::string &_address,
                      const CAmount& _debit, const CAmount& _credit,const std::string &ipctitle ,const qint64 &ipctype,const QString &authtype,const QString &authlimit,const QString &authdate,const QString &status ,const QString &_strtime, const CAmount& _fee ):
        hash(_hash), time(_time), type(_type),address(_address), debit(_debit), credit(_credit),ipcTitle(ipctitle),ipcType(ipctype),authLimit(authlimit),authTime(authdate),authType(authtype),
        idx(0),strstatus(status),strtime(_strtime),TxFee(_fee)
    {

    }
    static bool showTransaction(const CWalletTx &wtx);
    static int GetAccuracySymbol(std::string tokensymbol);
    static QString getAccuracyNum(int acc ,QString num);
    static QString getAccuracyNumstr(QString name ,QString num);
    static QList<TransactionRecord> decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx);

    /** @name Immutable transaction attributes
      @{*/
    uint256 hash;
    qint64 time;
    Type type;
    QString strstatus;
    QString strtime;
    std::string address;
    CAmount debit;
    CAmount TxFee;
    CAmount credit;
    int amount;
    QString ecoinNum;
    bool IsConfrimed;
    std::string  ipcTitle;
    std::string ecoinType;
    int ipcType;
    QString authType;
    QString authLimit;
    QString authTime;
    int idx;
    TransactionStatus status;

    /** Whether the transaction was sent/received with a watch-only address */
    bool involvesWatchAddress;

    bool isTag;

    /** Return the unique identifier for this transaction (part) */
    QString getTxID() const;

    /** Return the output index of the subtransaction  */
    int getOutputIndex() const;

    QString getecoinType();

    QString getIPCTitle();

    QString getstrStatus();

    static QString FormatTxStatus(const CWalletTx& wtx);
    static QString FormatTxTime(const CWalletTx& wtx);

    static void saveInfo(TranInfo traninfo);
    static void markLastInfo(std::string add,CAmount mount);
    uint8_t getIPCType();

    uint8_t getType();

    /** Update status from core wallet tx.
     */
    void updateStatus(const CWalletTx &wtx);

    /** Return whether a status update is needed.
     */
    bool statusUpdateNeeded();
    uint256 hash1 ;

};

#endif // BITCOIN_QT_TRANSACTIONRECORD_H
