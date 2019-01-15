// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLET_H
#define BITCOIN_WALLET_WALLET_H

#include "amount.h"
#include "streams.h"
#include "tinyformat.h"
#include "ui_interface.h"
#include "utilstrencodings.h"
#include "validationinterface.h"
#include "script/ismine.h"
#include "script/sign.h"
#include "wallet/crypter.h"
#include "wallet/walletdb.h"
#include "wallet/rpcwallet.h"

#include <algorithm>
#include <atomic>
#include <map>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>


extern CWallet* pwalletMain;

/**
 * Settings
 */
extern CFeeRate payTxFee;
extern unsigned int nTxConfirmTarget;
extern bool bSpendZeroConfChange;
extern bool fSendFreeTransactions;
extern bool fWalletRbf;

static const unsigned int DEFAULT_KEYPOOL_SIZE = 100;
//! -paytxfee default
static const CAmount DEFAULT_TRANSACTION_FEE = 50000; 
//! -fallbackfee default
static const CAmount DEFAULT_FALLBACK_FEE = 20000;
//! -mintxfee default
static const CAmount DEFAULT_TRANSACTION_MINFEE = 1000;
//! minimum recommended increment for BIP 125 replacement txs
static const CAmount WALLET_INCREMENTAL_RELAY_FEE = 5000;
//! target minimum change amount
static const CAmount MIN_CHANGE = CENT;
//! final minimum change amount after paying for fees
static const CAmount MIN_FINAL_CHANGE = MIN_CHANGE/2;
//! Default for -spendzeroconfchange
static const bool DEFAULT_SPEND_ZEROCONF_CHANGE = true;
//! Default for -sendfreetransactions
static const bool DEFAULT_SEND_FREE_TRANSACTIONS = false;
//! Default for -walletrejectlongchains
static const bool DEFAULT_WALLET_REJECT_LONG_CHAINS = false;
//! -txconfirmtarget default
static const unsigned int DEFAULT_TX_CONFIRM_TARGET = 8;
//! -walletrbf default
static const bool DEFAULT_WALLET_RBF = false;
//! Largest (in bytes) free transaction we're willing to create
static const unsigned int MAX_FREE_TRANSACTION_CREATE_SIZE = 1000;
static const bool DEFAULT_WALLETBROADCAST = true;
static const bool DEFAULT_DISABLE_WALLET = false;
//! if set, all keys will be derived by using BIP32
static const bool DEFAULT_USE_HD_WALLET = true;

extern const char * DEFAULT_WALLET_DAT;

class CBlockIndex;
class CCoinControl;
class COutput;
class CReserveKey;
class CScript;
class CTxMemPool;
class CWalletTx;
/** (client) version numbers for particular wallet features */
enum WalletFeature
{
    FEATURE_BASE = 10500, // the earliest version new wallets supports (only useful for getinfo's clientversion output)

    FEATURE_WALLETCRYPT = 40000, // wallet encryption 
    FEATURE_COMPRPUBKEY = 60000, // compressed public keys

    FEATURE_HD = 130000, // Hierarchical key derivation after BIP32 (HD Wallet)
    FEATURE_LATEST = FEATURE_COMPRPUBKEY // HD is optional, use FEATURE_COMPRPUBKEY as latest version 
};



/** A key pool entry*/
class CKeyPool
{
public:
    int64_t nTime;
    CPubKey vchPubKey;

    CKeyPool();
    CKeyPool(const CPubKey& vchPubKeyIn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(nTime);
        READWRITE(vchPubKey);
    }
};

/** Address book data */
class CAddressBookData
{
public:
    std::string name;
    std::string purpose;

    CAddressBookData()
    {
        purpose = "unknown";
    }

    typedef std::map<std::string, std::string> StringMap;
    StringMap destdata;
};

struct UnionAddressInfo{
    std::string name;
    std::string address;
    int time;
};

struct CRecipient 
{
    CScript scriptPubKey;
    CAmount nAmount;
    bool fSubtractFeeFromAmount; 
};

struct CRecipientaddtoken
{
	CScript  scriptPubKey;
	uint32_t height;
	uint64_t tokenvalue;
	std::string  extendinfo;
	std::string  txLabel;
};

struct CRecipientToken
{
	CScript scriptPubKey;
	CAmount nAmount;
	uint64_t nvalue;
	bool fSubtractFeeFromAmount;
    std::string crosslabel;
};

typedef std::map<std::string, std::string> mapValue_t;


static inline void ReadOrderPos(int64_t& nOrderPos, mapValue_t& mapValue)
{
    if (!mapValue.count("n"))
    {
        nOrderPos = -1; // TODO: calculate elsewhere
        return;
    }
    nOrderPos = atoi64(mapValue["n"].c_str());
}


static inline void WriteOrderPos(const int64_t& nOrderPos, mapValue_t& mapValue)
{
    if (nOrderPos == -1)
        return;
    mapValue["n"] = i64tostr(nOrderPos);
}

struct COutputEntry
{
    CTxDestination destination;
    CAmount amount;
    int vout;
};
struct COutputEntryToken
{
	CTxDestination destination;
	CAmount amount;
	int vout;
	uint8_t txType;
	std::string strsymbol;
	uint8_t  accuracy;
	uint64_t value;
};
struct COutputEntryIP
{
	CTxDestination destination;
	CAmount amount;
	int vout;
	uint8_t txType;
	uint8_t ExtendType;   
	std::string iptitle;
	std::string iphash;
};
/** A transaction with a merkle branch linking it to the block chain. */
class CMerkleTx
{
private:
  /** Constant used in hashBlock to indicate tx has been abandoned **/
    static const uint256 ABANDON_HASH;

public:
    CTransactionRef tx;
    uint256 hashBlock;

    /* An nIndex == -1 means that hashBlock (in nonzero) refers to the earliest
     * block in the chain we know this or any in-wallet dependency conflicts
     * with. Older clients interpret nIndex == -1 as unconfirmed for backward
     * compatibility.
     */
    int nIndex;

    CMerkleTx()
    {
        SetTx(MakeTransactionRef());
        Init();
    }

    CMerkleTx(CTransactionRef arg)
    {
        SetTx(std::move(arg));
        Init();
    }

    /** Helper conversion operator to allow passing CMerkleTx where CTransaction is expected.
     *  TODO: adapt callers and remove this operator. */
    operator const CTransaction&() const { return *tx; }

    void Init()
    {
        hashBlock = uint256();
        nIndex = -1;
    }

    void SetTx(CTransactionRef arg)
    {
        tx = std::move(arg);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        std::vector<uint256> vMerkleBranch; // For compatibility with older versions.
        READWRITE(tx);
        READWRITE(hashBlock);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
    }

    void SetMerkleBranch(const CBlockIndex* pIndex, int posInBlock);

    /**
     * Return depth of transaction in blockchain:
     * <0  : conflicts with a transaction this deep in the blockchain
     *  0  : in memory pool, waiting to be included in a block
     * >=1 : this many blocks deep in the main chain
     */
    int GetDepthInMainChain(const CBlockIndex* &pindexRet) const;
    int GetDepthInMainChain() const { const CBlockIndex *pindexRet; return GetDepthInMainChain(pindexRet); }
    bool IsInMainChain() const { const CBlockIndex *pindexRet; return GetDepthInMainChain(pindexRet) > 0; }
    int GetBlocksToMaturity() const;
    /** Pass this transaction to the mempool. Fails if absolute fee exceeds absurd fee. */
    bool AcceptToMemoryPool(const CAmount& nAbsurdFee, CValidationState& state);
    bool hashUnset() const { return (hashBlock.IsNull() || hashBlock == ABANDON_HASH); }
    bool isAbandoned() const { return (hashBlock == ABANDON_HASH); }
    void setAbandoned() { hashBlock = ABANDON_HASH; }

	int64_t GetTimeOfTokenInChain()const; //The time to get the block
    const uint256& GetHash() const { return tx->GetHash(); }
    bool IsCoinBase() const { return tx->IsCoinBase(); }
};

/** 
 * A transaction with a bunch of additional info that only the owner cares about.
 * It includes any unrecorded transactions needed to link it back to the block chain.
 */
class CWalletTx : public CMerkleTx
{
private:
    const CWallet* pwallet;

public:
    mapValue_t mapValue;
    std::vector<std::pair<std::string, std::string> > vOrderForm;
    unsigned int fTimeReceivedIsTxTime;
    unsigned int nTimeReceived; //!< time received by this node
    unsigned int nTimeSmart;
    /**
     * From me flag is set to 1 for transactions that were created by the wallet
     * on this bitcoin node, and set to 0 for transactions that were created
     * externally and came in through the network or sendrawtransaction RPC.
     */
    char fFromMe;
    std::string strFromAccount;
    int64_t nOrderPos; //!< position in ordered transaction list

    // memory only
    mutable bool fDebitCached;
    mutable bool fCreditCached;
    mutable bool fImmatureCreditCached;
    mutable bool fAvailableCreditCached;
    mutable bool fWatchDebitCached;
    mutable bool fWatchCreditCached;
    mutable bool fImmatureWatchCreditCached;
    mutable bool fAvailableWatchCreditCached;
    mutable bool fChangeCached;
    mutable CAmount nDebitCached;
    mutable CAmount nCreditCached;
    mutable CAmount nImmatureCreditCached;
    mutable CAmount nAvailableCreditCached;
    mutable CAmount nWatchDebitCached;
    mutable CAmount nWatchCreditCached;
    mutable CAmount nImmatureWatchCreditCached;
    mutable CAmount nAvailableWatchCreditCached;
    mutable CAmount nChangeCached;

    CWalletTx()
    {
        Init(NULL);
    }

    CWalletTx(const CWallet* pwalletIn, CTransactionRef arg) : CMerkleTx(std::move(arg))
    {
        Init(pwalletIn);
    }

    void Init(const CWallet* pwalletIn)
    {
        pwallet = pwalletIn;
        mapValue.clear();
        vOrderForm.clear();
        fTimeReceivedIsTxTime = false;
        nTimeReceived = 0;
        nTimeSmart = 0;
        fFromMe = false;
        strFromAccount.clear();
        fDebitCached = false;
        fCreditCached = false;
        fImmatureCreditCached = false;
        fAvailableCreditCached = false;
        fWatchDebitCached = false;
        fWatchCreditCached = false;
        fImmatureWatchCreditCached = false;
        fAvailableWatchCreditCached = false;
        fChangeCached = false;
        nDebitCached = 0;
        nCreditCached = 0;
        nImmatureCreditCached = 0;
        nAvailableCreditCached = 0;
        nWatchDebitCached = 0;
        nWatchCreditCached = 0;
        nAvailableWatchCreditCached = 0;
        nImmatureWatchCreditCached = 0;
        nChangeCached = 0;
        nOrderPos = -1;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        if (ser_action.ForRead())
            Init(NULL);
        char fSpent = false;

        if (!ser_action.ForRead())
        {
            mapValue["fromaccount"] = strFromAccount;

            WriteOrderPos(nOrderPos, mapValue);

            if (nTimeSmart)
                mapValue["timesmart"] = strprintf("%u", nTimeSmart);
        }

        READWRITE(*(CMerkleTx*)this);
        std::vector<CMerkleTx> vUnused; //!< Used to be vtxPrev
        READWRITE(vUnused);
        READWRITE(mapValue);
        READWRITE(vOrderForm);
        READWRITE(fTimeReceivedIsTxTime);
        READWRITE(nTimeReceived);
        READWRITE(fFromMe);
        READWRITE(fSpent);

        if (ser_action.ForRead())
        {
            strFromAccount = mapValue["fromaccount"];

            ReadOrderPos(nOrderPos, mapValue);

            nTimeSmart = mapValue.count("timesmart") ? (unsigned int)atoi64(mapValue["timesmart"]) : 0;
        }

        mapValue.erase("fromaccount");
        mapValue.erase("version");
        mapValue.erase("spent");
        mapValue.erase("n");
        mapValue.erase("timesmart");
    }

    //! make sure balances are recalculated
    void MarkDirty()
    {
        fCreditCached = false;
        fAvailableCreditCached = false;
        fImmatureCreditCached = false;
        fWatchDebitCached = false;
        fWatchCreditCached = false;
        fAvailableWatchCreditCached = false;
        fImmatureWatchCreditCached = false;
        fDebitCached = false;
        fChangeCached = false;
    }

    void BindWallet(CWallet *pwalletIn)
    {
        pwallet = pwalletIn;
        MarkDirty();
    }

    //! filter decides which addresses will count towards the debit
    CAmount GetDebit(const isminefilter& filter) const;
    CAmount GetDebitForUnion(const isminefilter& filter,std::vector<UnionAddressInfo>&unionaddresses) const;
    CAmount GetCredit(const isminefilter& filter) const;
    CAmount GetImmatureCredit(bool fUseCache=true) const;
    CAmount GetAvailableCredit(bool fUseCache=true) const;
	//add by  xxy Calculate lock deposit
	CAmount GetLockCredit(bool fUseCache = true) const;
	//end
    CAmount GetImmatureWatchOnlyCredit(const bool& fUseCache=true) const;
    CAmount GetAvailableWatchOnlyCredit(const bool& fUseCache=true) const;
    CAmount GetChange() const;

    void GetAmounts(std::list<COutputEntry>& listReceived,
                    std::list<COutputEntry>& listSent, CAmount& nFee, std::string& strSentAccount, const isminefilter& filter) const;//IPC No filter for change
	void GetAmounts2(std::list<COutputEntry>& listReceived,
		std::list<COutputEntry>& listSent, CAmount& nFee, std::string& strSentAccount, const isminefilter& filter) const;//IPC filter for change
	void GetAmountsToken(const std::string& strtokensymbol, std::list<COutputEntryToken>& listReceived,
		std::list<COutputEntryToken>& listSent, CAmount& nFee, std::string& strSentAccount, const isminefilter& filter) const;//Token Filter change
	void GetAmountsIP(int IPtype, std::list<COutputEntryIP>& listReceived,
		std::list<COutputEntryIP>& listSent, CAmount& nFee, std::string& strSentAccount, const isminefilter& filter) const;//IP Filter change
	void GetAmountsIPForGetitx(std::list<COutputEntryIP>& listReceived,
		std::list<COutputEntryIP>& listSent, CAmount& nFee, std::string& strSentAccount, const isminefilter& filter) const;//IP no Filter for change
	void GetAmountsTokenForGetttx(std::list<COutputEntryToken>& listReceived,
		std::list<COutputEntryToken>& listSent, CAmount& nFee, std::string& strSentAccount, const isminefilter& filter) const;//Token no filter for change
    void GetAccountAmounts(const std::string& strAccount, CAmount& nReceived,
                           CAmount& nSent, CAmount& nFee, const isminefilter& filter) const;
    void GetAmountsTokenForUnion(const std::string& strtokensymbol, std::list<COutputEntryToken>& listReceived,
        std::list<COutputEntryToken>& listSent, CAmount& nFee, std::string& strSentAccount, const isminefilter& filter,std::vector<UnionAddressInfo>&unionaddresses) const;

    bool IsFromMe(const isminefilter& filter) const
    {
        return (GetDebit(filter) > 0);
    }

    // True if only scriptSigs are different
    bool IsEquivalentTo(const CWalletTx& tx) const;

    bool InMempool() const;
    bool IsTrusted() const;

    int64_t GetTxTime() const;
    int GetRequestCount() const;

    bool RelayWalletTransaction(CConnman* connman);

    std::set<uint256> GetConflicts() const;
};




class COutput
{
public:
    const CWalletTx *tx;
    int i;
    int nDepth;
    bool fSpendable;
    bool fSolvable;

    COutput(const CWalletTx *txIn, int iIn, int nDepthIn, bool fSpendableIn, bool fSolvableIn)
    {
        tx = txIn; i = iIn; nDepth = nDepthIn; fSpendable = fSpendableIn; fSolvable = fSolvableIn;
    }


	bool CanBeSentToOhter() const;
	bool CanBeAuthorizedToOther() const;
	bool CanBeUniqueAuthorizedToOther() const;

	int GetType() const;
	int GetIPCExtendType() const;
	uint32_t GetIPCStartTime() const;
	uint32_t GetIPCStopTime() const;
	uint8_t GetIPCreAuthorize() const;
	uint8_t GetIPCUniqAuthorize() const;
	std::string GetIPCHash() const;
	std::string GetIPCTitle() const;
	std::string GetIPCLabel() const;

	std::string GetTokenSymbol() const;
	uint8_t GetTokenAccuracy() const;
	uint64_t GetTokenvalue() const;

	int64_t GetAssetEntryTime() const;//Get the asset entry time
    std::string ToString() const;
	
};




/** Private key that includes an expiration date in case it never gets used. */
class CWalletKey
{
public:
    CPrivKey vchPrivKey;
    int64_t nTimeCreated;
    int64_t nTimeExpires;  
    std::string strComment;
    //! todo: add something to note what created it (user, getnewaddress, change)
    //!   maybe should have a map<string, string> property map

    CWalletKey(int64_t nExpires=0);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vchPrivKey);
        READWRITE(nTimeCreated);
        READWRITE(nTimeExpires);
        READWRITE(LIMITED_STRING(strComment, 65536));
    }
};

struct TokenByMeInfo{
    int accuracy;
    int confirmation;
    std::string txid;
    std::vector<std::string> addresses;
};
struct withoutnetwork_inputs{
    std::string scriptPubKey;
    int64_t nAmount;
    int64_t vout;
    std::string txid;
};
typedef std::map<std::string,TokenByMeInfo> tokenInfoMap;
/**
 * Internal transfers.
 * Database key is acentry<account><counter>.
 */
class CAccountingEntry
{
public:
    std::string strAccount;
    CAmount nCreditDebit;
    int64_t nTime;
    std::string strOtherAccount;
    std::string strComment;
    mapValue_t mapValue;
    int64_t nOrderPos; //!< position in ordered transaction list
    uint64_t nEntryNo;

    CAccountingEntry()
    {
        SetNull();
    }

    void SetNull()
    {
        nCreditDebit = 0;
        nTime = 0;
        strAccount.clear();
        strOtherAccount.clear();
        strComment.clear();
        nOrderPos = -1;
        nEntryNo = 0;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        //! Note: strAccount is serialized as part of the key, not here.
        READWRITE(nCreditDebit);
        READWRITE(nTime);
        READWRITE(LIMITED_STRING(strOtherAccount, 65536));

        if (!ser_action.ForRead())
        {
            WriteOrderPos(nOrderPos, mapValue);

            if (!(mapValue.empty() && _ssExtra.empty()))
            {
                CDataStream ss(s.GetType(), s.GetVersion());
                ss.insert(ss.begin(), '\0');
                ss << mapValue;
                ss.insert(ss.end(), _ssExtra.begin(), _ssExtra.end());
                strComment.append(ss.str());
            }
        }

        READWRITE(LIMITED_STRING(strComment, 65536));

        size_t nSepPos = strComment.find("\0", 0, 1);
        if (ser_action.ForRead())
        {
            mapValue.clear();
            if (std::string::npos != nSepPos)
            {
                CDataStream ss(std::vector<char>(strComment.begin() + nSepPos + 1, strComment.end()), s.GetType(), s.GetVersion());
                ss >> mapValue;
                _ssExtra = std::vector<char>(ss.begin(), ss.end());
            }
            ReadOrderPos(nOrderPos, mapValue);
        }
        if (std::string::npos != nSepPos)
            strComment.erase(nSepPos);

        mapValue.erase("n");
    }

private:
    std::vector<char> _ssExtra;
};


/** 
 * A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
 * and provides the ability to create new transactions.
 *
 */
class CWallet : public CCryptoKeyStore, public CValidationInterface
{
private:
    static std::atomic<bool> fFlushThreadRunning;

    /**
     * Select a set of coins such that nValueRet >= nTargetValue and at least
     * all coins from coinControl are selected; Never select unconfirmed coins
     * if they are not ours
     */
    bool SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl *coinControl = NULL) const;

	//add by xxy 
	bool SelectNormalCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<std::pair<const CWalletTx*, unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl *coinControl = NULL) const;
	bool SelectTokenCoins(const std::vector<COutput>& vAvailableCoins, std::set<std::pair<const CWalletTx*, unsigned int> >& setCoinsRet,std::string & symbol,uint64_t& nTokenValue, const CCoinControl *coinControl = NULL) const;

	//end
    CWalletDB *pwalletdbEncryption;

    //! the current wallet version: clients below this version are not able to load the wallet
    int nWalletVersion;

    //! the maximum wallet format version: memory-only variable that specifies to what version this wallet may be upgraded
    int nWalletMaxVersion;

    int64_t nNextResend;
    int64_t nLastResend;
    bool fBroadcastTransactions;

    /**
     * Used to keep track of spent outpoints, and
     * detect and report conflicts (double-spends or
     * mutated transactions where the mutant gets mined).
     */
    typedef std::multimap<COutPoint, uint256> TxSpends;
    TxSpends mapTxSpends;
    void AddToSpends(const COutPoint& outpoint, const uint256& wtxid);
    void AddToSpends(const uint256& wtxid);

	void RemoveFromSpends(const COutPoint& outpoint, const uint256& wtxid);
	void RemoveFromSpends(const uint256& wtxid);

    /* Mark a transaction (and its in-wallet descendants) as conflicting with a particular block. */
    void MarkConflicted(const uint256& hashBlock, const uint256& hashTx);

    void SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator>);

    /* the HD chain data model (external chain counters) */
    CHDChain hdChain;

    bool fFileBacked;

    std::set<int64_t> setKeyPool;

    int64_t nTimeFirstKey;

    /**
     * Private version of AddWatchOnly method which does not accept a
     * timestamp, and which will reset the wallet's nTimeFirstKey value to 1 if
     * the watch key did not previously have a timestamp associated with it.
     * Because this is an inherited virtual method, it is accessible despite
     * being marked private, but it is marked private anyway to encourage use
     * of the other AddWatchOnly which accepts a timestamp and sets
     * nTimeFirstKey more intelligently for more efficient rescans.
     */
    bool AddWatchOnly(const CScript& dest) override;

public:
    /*
     * Main wallet lock.
     * This lock protects all the fields added by CWallet
     *   except for:
     *      fFileBacked (immutable after instantiation)
     *      strWalletFile (immutable after instantiation)
     */
    mutable CCriticalSection cs_wallet;

    const std::string strWalletFile;

    void LoadKeyPool(int nIndex, const CKeyPool &keypool)
    {
        setKeyPool.insert(nIndex);

        // If no metadata exists yet, create a default with the pool key's
        // creation time. Note that this may be overwritten by actually
        // stored metadata for that key later, which is fine.
        CKeyID keyid = keypool.vchPubKey.GetID();
        if (mapKeyMetadata.count(keyid) == 0)
            mapKeyMetadata[keyid] = CKeyMetadata(keypool.nTime);
    }

    // Map from Key ID (for regular keys) or Script ID (for watch-only keys) to
    // key metadata.
    std::map<CTxDestination, CKeyMetadata> mapKeyMetadata;

    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID;

    CWallet()
    {
        SetNull();
    }

    CWallet(const std::string& strWalletFileIn) : strWalletFile(strWalletFileIn)
    {
        SetNull();
        fFileBacked = true;
    }

    ~CWallet()
    {
        delete pwalletdbEncryption;
        pwalletdbEncryption = NULL;
    }

    void SetNull()
    {
        nWalletVersion = FEATURE_BASE;
        nWalletMaxVersion = FEATURE_BASE;
        fFileBacked = false;
        nMasterKeyMaxID = 0;
        pwalletdbEncryption = NULL;
        nOrderPosNext = 0;
        nNextResend = 0;
        nLastResend = 0;
        nTimeFirstKey = 0;
        fBroadcastTransactions = false;
    }

    std::map<uint256, CWalletTx> mapWallet; 
    std::list<CAccountingEntry> laccentries;

    typedef std::pair<CWalletTx*, CAccountingEntry*> TxPair;
    typedef std::multimap<int64_t, TxPair > TxItems;
    TxItems wtxOrdered;

    int64_t nOrderPosNext;
    std::map<uint256, int> mapRequestCount;

    std::map<CTxDestination, CAddressBookData> mapAddressBook;

    CPubKey vchDefaultKey;

    std::set<COutPoint> setLockedCoins;

    const CWalletTx* GetWalletTx(const uint256& hash) const;

	std::map <std::string, uint64_t> TokenValueMap; //The corresponding balance of various tokens - return list to the wallet
	std::vector<std::string> TokensymbolList;			//A collection of tokens in the wallet
	SecureString curstrWalletPassphrase;				//Current wallet password
    //! check whether we are allowed to upgrade (or already support) to the named feature
    bool CanSupportFeature(enum WalletFeature wf) { AssertLockHeld(cs_wallet); return nWalletMaxVersion >= wf; }

    /**
     * populate vCoins with vector of available COutputs.
     */
    void AvailableCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed=true, const CCoinControl *coinControl = NULL, bool fIncludeZeroValue=false) const;

	//add by xxy 
	void AvailableNormalCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed = true, const CCoinControl *coinControl = NULL, bool fIncludeZeroValue = false) const;
	void AvailableIPCCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed = true, const CCoinControl *coinControl = NULL, bool fIncludeZeroValue = false) const;
	void AvailableTokenCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed = true, const CCoinControl *coinControl = NULL, bool fIncludeZeroValue = false) const;
	bool checkVoutAddTokenCanSpend(const CTxOut& vout)const;
    bool GetSymbolbalance(std::string& tokensymbol, uint64_t& value,std::string unionaddress = ""/*, uint64_t nconfirmed = DEFAULT_TX_CONFIRM_TARGET*/);
	void ListTokenBalance(std::map<std::string, uint64_t>& TokenList);
	uint8_t GetAccuracyBySymbol(std::string& tokensymbol);
	uint32_t GetIssueDateBySymbol(std::string& tokensymbol);
    uint128 GetHashBySymbol(std::string& tokensymbol);
	void UpdateTokenBalanceList();
	bool IsHaveTheTokensymbol(std::string&tokensymbol);
	
    /**
     * Shuffle and select coins until nTargetValue is reached while avoiding
     * small change; This method is stochastic for some inputs and upon
     * completion the coin set and corresponding actual target value is
     * assembled
     */
    bool SelectCoinsMinConf(const CAmount& nTargetValue, int nConfMine, int nConfTheirs, uint64_t nMaxAncestors, std::vector<COutput> vCoins, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet) const;

    bool IsSpent(const uint256& hash, unsigned int n) const;

    bool IsLockedCoin(uint256 hash, unsigned int n) const;
    void LockCoin(const COutPoint& output);
    void UnlockCoin(const COutPoint& output);
    void UnlockAllCoins();
    void ListLockedCoins(std::vector<COutPoint>& vOutpts);

    /**
     * keystore implementation
     * Generate a new key
     */
    CPubKey GenerateNewKey();
    void DeriveNewChildKey(CKeyMetadata& metadata, CKey& secret);
    //! Adds a key to the store, and saves it to disk.
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey) override;
    //! Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKey(const CKey& key, const CPubKey &pubkey) { return CCryptoKeyStore::AddKeyPubKey(key, pubkey); }
    //! Load metadata (used by LoadWallet)
    bool LoadKeyMetadata(const CTxDestination& pubKey, const CKeyMetadata &metadata);

    bool LoadMinVersion(int nVersion) { AssertLockHeld(cs_wallet); nWalletVersion = nVersion; nWalletMaxVersion = std::max(nWalletMaxVersion, nVersion); return true; }
    void UpdateTimeFirstKey(int64_t nCreateTime);

    //! Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret) override;
    //! Adds an encrypted key to the store, without saving it to disk (used by LoadWallet)
    bool LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool AddCScript(const CScript& redeemScript) override;
    bool LoadCScript(const CScript& redeemScript);

    //! Adds a destination data tuple to the store, and saves it to disk
    bool AddDestData(const CTxDestination &dest, const std::string &key, const std::string &value);
    //! Erases a destination data tuple in the store and on disk
    bool EraseDestData(const CTxDestination &dest, const std::string &key);
    //! Adds a destination data tuple to the store, without saving it to disk
    bool LoadDestData(const CTxDestination &dest, const std::string &key, const std::string &value);
    //! Look up a destination data tuple in the store, return true if found false otherwise
    bool GetDestData(const CTxDestination &dest, const std::string &key, std::string *value) const;

    //! Adds a watch-only address to the store, and saves it to disk.
    bool AddWatchOnly(const CScript& dest, int64_t nCreateTime);
    bool RemoveWatchOnly(const CScript &dest) override;
    //! Adds a watch-only address to the store, without saving it to disk (used by LoadWallet)
    bool LoadWatchOnly(const CScript &dest);

    bool Unlock(const SecureString& strWalletPassphrase);
    bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);
    bool EncryptWallet(const SecureString& strWalletPassphrase); 

    void GetKeyBirthTimes(std::map<CTxDestination, int64_t> &mapKeyBirth) const;
	
    /** 
     * Increment the next transaction order id
     * @return next transaction order id
     */
    int64_t IncOrderPosNext(CWalletDB *pwalletdb = NULL);
    DBErrors ReorderTransactions();
    bool AccountMove(std::string strFrom, std::string strTo, CAmount nAmount, std::string strComment = "");
    bool GetAccountPubkey(CPubKey &pubKey, std::string strAccount, bool bForceNew = false);

    void MarkDirty();
	bool AddToWallet(const CWalletTx& wtxIn, bool fFlushOnClose = true);
	bool RemoveFromWallet(const CWalletTx& wtxIn, bool fFlushOnClose = true);
    bool LoadToWallet(const CWalletTx& wtxIn);
    void SyncTransaction(const CTransaction& tx, const CBlockIndex *pindex, int posInBlock) override;
    bool AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlockIndex* pIndex, int posInBlock, bool fUpdate);
    CBlockIndex* ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate = false,bool bIsP2SH = false);
    void ReacceptWalletTransactions();
    void ResendWalletTransactions(int64_t nBestBlockTime, CConnman* connman) override;
    std::vector<uint256> ResendWalletTransactionsBefore(int64_t nTime, CConnman* connman);
    CAmount GetBalance() const;
	//
	CAmount GetDeposit()const;
	int  GetDepthofJoinTX();
	//
    CAmount GetUnconfirmedBalance() const;
    CAmount GetImmatureBalance() const;
    CAmount GetWatchOnlyBalance() const;
    CAmount GetUnconfirmedWatchOnlyBalance() const;
    CAmount GetImmatureWatchOnlyBalance() const;

    /**
     * Insert additional inputs into the transaction by
     * calling CreateTransaction();
     */
    bool FundTransaction(CMutableTransaction& tx, CAmount& nFeeRet, bool overrideEstimatedFeeRate, const CFeeRate& specificFeeRate, int& nChangePosInOut, std::string& strFailReason, bool includeWatching, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, bool keepReserveKey = true, const CTxDestination& destChange = CNoDestination());

    /**
     * Create a new transaction paying the recipients with a set of coins
     * selected by SelectCoins(); Also create the change output, when needed
     * @note passing nChangePosInOut as -1 will result in setting a random position
     */
    bool CreateTransaction(const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
                           std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
	//add by xxy 
	//Create ordinary transactions
	bool CreateNormalTransaction(const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
	//Create a knowledge register transaction
	bool CreateIPCRegTransaction(std::string& strReglabel, const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason,  const CCoinControl *coinControl = NULL, bool sign = true);
	//Create knowledge transfer transactions
	bool CreateIPCSendTransaction(std::string& txid,int Index, const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
	//Create an informed authorization transaction
	bool CreateIPCAuthorizationTransaction(std::string& txid, int Index, const std::vector<CRecipient>& vecSend, std::string& IPCAuthorizeLabel, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);

	//Create knowledge transfer transactions
	bool CreateIPCSendTransactionWithTxlabel(std::string& txid, int Index, const std::vector<CRecipient>& vecSend, std::string& strtxlabel,CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
	//Create an informed authorization transaction
	bool CreateIPCAuthorizationTransactionWithTxlabel(std::string& txid, int Index, const std::vector<CRecipient>& vecSend, std::string& IPCAuthorizeLabel, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);

	//Create token reg trade
	bool CreateTokenRegTransaction(std::string& strReglabel, const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);

	bool CreateAddTokenRegTransactionByStrReglabel(std::string& strReglabel, const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
	bool regaddressfirst(std::vector<COutput> &vAvailableCoins, const std::vector<CRecipientaddtoken>&);
	bool setCoinswithaddress(std::vector<COutput> &vAvailableCoins,std::string address, std::set<std::pair<const CWalletTx*, unsigned int> >& setCoins,CAmount& nValueIn);
	bool CreateAddTokenRegTransaction(std::string& strReglabel, const std::vector<CRecipientaddtoken>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut, std::string& strFailReason, std::string address, const CCoinControl *coinControl = NULL, bool sign = true);
	//Create a token trade -- Support only for one address transaction.
	bool CreateTokenTransaction(std::string& tokensymbol, uint64_t TokenValue, const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
	bool CreateTokenTransactionM(std::string& tokensymbol, const std::vector<CRecipientToken>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
	bool CreateTokenTransactionForCross(std::string& tokensymbol, const std::string& strtxLabel, uint64_t TokenValue, const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
	
	//end
	bool address2pkhash(std::string& address,uint160& pkhash);
	bool address2Pubkey(std::string& address, std::string& strPubkey, std::string& strFailReason);
	bool CreateMultiAddress(unsigned int nRequired, const std::vector<std::string>& strPubkeys, std::string& multiscript, std::string& strmultiaddress, std::string& strFailReason);
	bool CreateMultiTransaction(std::string& straddressfrom ,const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason,  ScriptError& serror ,std::string& strtx, const CCoinControl *coinControl = NULL, bool sign = true);

	bool ChangeMultiTxFee(CMutableTransaction& tx,unsigned int bytes);
	//Join and exit the consensus trading interface
	bool JoinCampaign(const CTxDestination &address, CAmount deposi, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
	bool ExitCampaign(CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
	bool PunishRequest(const CTxDestination &address, const std::string evdience, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
		std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);

	//The interface of the consensus class is submitted, and the addition and exit interface of the dpoc layer is invoked in this interface
	bool CommitJoinCampaignTransaction(const CTxDestination &address, CWalletTx& wtxNew, CReserveKey& reservekey, CConnman* connman, CValidationState& state);
	//Get the consensus reward list
	bool GetCurrentRewards(std::vector<std::string>& timelist, std::vector<std::string>& valuelist);

    bool CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey, CConnman* connman, CValidationState& state);

    void ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& entries);
    bool AddAccountingEntry(const CAccountingEntry&);
    bool AddAccountingEntry(const CAccountingEntry&, CWalletDB *pwalletdb);
    template <typename ContainerType>
    bool DummySignTx(CMutableTransaction &txNew, const ContainerType &coins,int nseek = 0);
    static CFeeRate minTxFee;
    static CFeeRate fallbackFee;
    /**
     * Estimate the minimum fee considering user set parameters
     * and the required fee
     */
    static CAmount GetMinimumFee(unsigned int nTxBytes, unsigned int nConfirmTarget, const CTxMemPool& pool);
    /**
     * Estimate the minimum fee considering required fee and targetFee or if 0
     * then fee estimation for nConfirmTarget
     */
    static CAmount GetMinimumFee(unsigned int nTxBytes, unsigned int nConfirmTarget, const CTxMemPool& pool, CAmount targetFee);
    /**
     * Return the minimum required fee taking into account the
     * floating relay fee and user set minimum transaction fee
     */
    static CAmount GetRequiredFee(unsigned int nTxBytes);

    bool NewKeyPool();
    bool TopUpKeyPool(unsigned int kpSize = 0);
    void ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool);
    void KeepKey(int64_t nIndex);
    void ReturnKey(int64_t nIndex);
    bool GetKeyFromPool(CPubKey &key);
    int64_t GetOldestKeyPoolTime();
    void GetAllReserveKeys(std::set<CKeyID>& setAddress) const;

    std::set< std::set<CTxDestination> > GetAddressGroupings();
    std::map<CTxDestination, CAmount> GetAddressBalances();

    CAmount GetAccountBalance(const std::string& strAccount, int nMinDepth, const isminefilter& filter);
    CAmount GetAccountBalance(CWalletDB& walletdb, const std::string& strAccount, int nMinDepth, const isminefilter& filter);
    std::set<CTxDestination> GetAccountAddresses(const std::string& strAccount) const;

    isminetype IsMine(const CTxIn& txin) const;

    /**
     * Returns amount of debit if the input matches the
     * filter, otherwise returns 0
     */
    CAmount GetDebit(const CTxIn& txin, const isminefilter& filter) const;
    isminetype IsMine(const CTxOut& txout) const;
    CAmount GetCredit(const CTxOut& txout, const isminefilter& filter) const;
    bool IsChange(const CTxOut& txout) const;
    CAmount GetChange(const CTxOut& txout) const;
    bool IsMine(const CTransaction& tx) const;

    /** should probably be renamed to IsRelevantToMe */
    bool IsFromMe(const CTransaction& tx) const;
	bool IsFromMe(const CTransaction& tx, int type) const;
    CAmount GetDebit(const CTransaction& tx, const isminefilter& filter) const;
	uint8_t GetDebitOfIp(const CTransaction& tx, const isminefilter& filter)const;
	uint64_t GetDebitOfToken(const CTransaction& tx, const isminefilter& filter) const;
    /** Returns whether all of the inputs match the filter */
    bool IsAllFromMe(const CTransaction& tx, const isminefilter& filter) const;
    CAmount GetCredit(const CTransaction& tx, const isminefilter& filter) const;
    CAmount GetChange(const CTransaction& tx) const;
	bool GetChangeIndex(const CTransaction& tx,int& index)const; //-1:no change
    void SetBestChain(const CBlockLocator& loc) override;

    DBErrors LoadWallet(bool& fFirstRunRet);
    DBErrors ZapWalletTx(std::vector<CWalletTx>& vWtx);
    DBErrors ZapSelectTx(std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut);

    bool SetAddressBook(const CTxDestination& address, const std::string& strName, const std::string& purpose);

    bool DelAddressBook(const CTxDestination& address);

    void UpdatedTransaction(const uint256 &hashTx) override;

    void Inventory(const uint256 &hash) override
    {
        {
            LOCK(cs_wallet);
            std::map<uint256, int>::iterator mi = mapRequestCount.find(hash);
            if (mi != mapRequestCount.end())
                (*mi).second++;
        }
    }

    void GetScriptForMining(boost::shared_ptr<CReserveScript> &script) override;
    void ResetRequestCount(const uint256 &hash) override
    {
        LOCK(cs_wallet);
        mapRequestCount[hash] = 0;
    };
    
    unsigned int GetKeyPoolSize()
    {
        AssertLockHeld(cs_wallet); // setKeyPool
        return setKeyPool.size();
    }

    bool SetDefaultKey(const CPubKey &vchPubKey);

    //! signify that a particular wallet feature is now used. this may change nWalletVersion and nWalletMaxVersion if those are lower
    bool SetMinVersion(enum WalletFeature, CWalletDB* pwalletdbIn = NULL, bool fExplicit = false);

    //! change which version we're allowed to upgrade to (note that this does not immediately imply upgrading to that format)
    bool SetMaxVersion(int nVersion);

    //! get the current wallet format (the oldest client version guaranteed to understand this wallet)
    int GetVersion() { LOCK(cs_wallet); return nWalletVersion; }

    //! Get wallet transactions that conflict with given transaction (spend same outputs)
    std::set<uint256> GetConflicts(const uint256& txid) const;

    //! Check if a given transaction has any of its outputs spent by another transaction in the wallet
    bool HasWalletSpend(const uint256& txid) const;

    //! Flush wallet (bitdb flush)
    void Flush(bool shutdown=false);

    //! Verify the wallet database and perform salvage if required
    static bool Verify();
    
    /** 
     * Address book entry changed.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void (CWallet *wallet, const CTxDestination
            &address, const std::string &label, bool isMine,
            const std::string &purpose,
            ChangeType status)> NotifyAddressBookChanged;

    /** 
     * Wallet transaction added, removed or updated.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void (CWallet *wallet, const uint256 &hashTx,
            ChangeType status)> NotifyTransactionChanged;

    /** Show progress e.g. for rescan */
    boost::signals2::signal<void (const std::string &title, int nProgress)> ShowProgress;

    /** Watch-only address added */
    boost::signals2::signal<void (bool fHaveWatchOnly)> NotifyWatchonlyChanged;

    /** Inquire whether this wallet broadcasts transactions. */
    bool GetBroadcastTransactions() const { return fBroadcastTransactions; }
    /** Set whether this wallet broadcasts transactions. */
    void SetBroadcastTransactions(bool broadcast) { fBroadcastTransactions = broadcast; }

    /* Mark a transaction (and it in-wallet descendants) as abandoned so its inputs may be respent. */
    bool AbandonTransaction(const uint256& hashTx);

    /** Mark a transaction as replaced by another transaction (e.g., BIP 125). */
    bool MarkReplaced(const uint256& originalHash, const uint256& newHash);

    /* Returns the wallets help message */
    static std::string GetWalletHelpString(bool showDebug);

    /* Initializes the wallet, returns a new CWallet instance or a null pointer in case of an error */
    static CWallet* CreateWalletFromFile(const std::string walletFile);
    static bool InitLoadWallet();

	static bool LoadWalletFromFile(const std::string filepath);
	static bool ExportWalletToFile(const std::string filepath);

    /**
     * Wallet post-init setup
     * Gives the wallet a chance to register repetitive tasks and complete post-init tasks
     */
    void postInitProcess(boost::thread_group& threadGroup);

    /* Wallets parameter interaction */
    static bool ParameterInteraction();

    bool BackupWallet(const std::string& strDest);

    /* Set the HD chain model (chain child index counters) */
    bool SetHDChain(const CHDChain& chain, bool memonly);
    const CHDChain& GetHDChain() { return hdChain; }

    /* Returns true if HD is enabled */
    bool IsHDEnabled();

    /* Generates a new HD master key (will not be activated) */
    CPubKey GenerateNewHDMasterKey();
    
    /* Set the current HD master key (will reset the chain child index counters) */
    bool SetHDMasterKey(const CPubKey& key);
    void getListTokenBuildByMe(tokenInfoMap &tokeninfo);

    void flushWalletData();
    //ly p2sh

    bool GetAddressNmembers(std::string address,int &nmembers,int &nRequired,std::string &strmultiscript);
    bool AddMultiAddress(std::string strmultiscript, std::string& strFailReason,\
                         int &nRequired,int &nmembers, std::string strAccount = "",\
                         bool brpc = false,bool bRelatedToMe = true);
	bool DecodeStrInvCode(std::string strmultiscript, std::vector<std::string>& pubkeys, int& nRequired, int& nmembers);
    bool FindAddressBook(const CTxDestination& address, std::string type);
    void UnionCWalletTxFromAddress(std::map<uint256, const CWalletTx*>& vCoins,std::string address,int txtype = 0,\
                                   uint64_t minblock = 0,uint64_t maxblock = 0,std::string tokenname = "");
    bool analysisMultiTransaction(const std::string& strtx,std::string& strFailReason,\
            std::string& address,CAmount& money,int haveSigned,std::string& sourceaddress);
    bool CommitMultiTransaction(CMutableTransaction& mergedTx,std::string& error);
    bool SignMultiTransaction(const std::string& strtx, std::string& strFailReason, ScriptError& serror,std::string& strtxout,CMutableTransaction& mergedTx,bool isrpc=false,bool onlyMySign=false);
    bool SignMultiTransactionWithoutNetwork(const std::string& strtx, std::string& strFailReason,std::string& out,std::vector<withoutnetwork_inputs>& inputs,bool onlyMySign);
    bool P2SHProduceSignature(CMutableTransaction& mergedTx,ScriptError& serror,std::string& strFailReason,bool onlyMySign=false);

    bool P2SHProduceSignatureWithoutNetwork(CMutableTransaction& mergedTx,ScriptError& serror, std::string& strFailReason,bool onlyMySign ,std::vector<withoutnetwork_inputs>&  vCoins);
    bool checkMyUnion(const CTransaction& tx);
    void getUnionAddresses(std::vector<UnionAddressInfo>& addressbook);
    int  getUnionAddressLocalTime(std::string address);
    void AvailableUnionCoins(std::map<std::string,CAmount>& moneynums, bool fOnlyConfirmed=true, const CCoinControl *coinControl = NULL, bool fIncludeZeroValue=false) const;
    bool isMyPk(std::string);
    bool voutFindAddress(const CTxOut& vout,std::string address);
    bool vinsFindAddress(const std::vector<CTxIn>& vin,std::string address);
    int64_t GetBlockTime(const CWalletTx& wtxIn);
    void AvailableUnionCoinsCOutput(std::string & strunionaddress, std::vector<COutput>& vCoins, \
                              bool fOnlyConfirmed = true, const CCoinControl *coinControl = NULL,\
                              bool fIncludeZeroValue = false ,bool isToken = false) const;
    bool UnionCreateTokenTransactionM(std::string unionaddress,std::string& tokensymbol, const std::vector<CRecipientToken>& vecSend, CWalletTx& wtxNew,\
                                      CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,\
                                      std::string& strFailReason,std::string& strtx, const CCoinControl *coinControl = NULL, bool sign = true,\
                                      int maxvinsize = 0,int minvinsize = 0,int minconfirmation = 8);
    bool SelectUnionCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<std::pair<const CWalletTx*, unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl *coinControl = NULL) const;
    bool SelectUnionTokenCoins(const std::vector<COutput>& vAvailableCoins, std::set<std::pair<const CWalletTx*, unsigned int> >& setCoinsRet,std::string & symbol,uint64_t& nTokenValue, const CCoinControl *coinControl = NULL) const;
    bool SignUnionTxWithSigns(const std::string& strtx, std::string& strFailReason, ScriptError& serror, std::string& strtxout,CMutableTransaction& mergedTx,std::vector<std::string>& signtxs,bool checkscripterr = true);
    bool getSignatureDataFromTx(std::string txstring,std::string& strFailReason,SignatureData& signdata,unsigned int nIn);
    bool P2SHIntegratedSignature(CMutableTransaction& mergedTx,std::string& strFailReason,ScriptError& serror,std::vector<std::string> &signtxs);
    bool SearchCrossTxid(std::string crosstxid,std::string& txid,std::string TokenSymbol);
    bool uniontxcommit(const std::string strtx, std::string& strFailReason, std::string& strtxout);
    uint64_t GetDebitOfTokenForUnion(const CTransaction& tx, const isminefilter& filter,std::vector<UnionAddressInfo>&unionaddresses) const;
    bool GetChangeIndexTokenUnion(const CTransaction& tx,int& index)const; //-1:no change
    bool getunionaddressfrominvcode( std::string strmultiscript, std::string& strFailReason,int &nRequired,int &nmembers, std::string &unionaddress );
    bool checkUnionSign(const std::string& strtx, std::string& strFailReason, ScriptError& serror, std::string& strtxout,CMutableTransaction& mergedTx);
    bool P2SHCheckSignature(CMutableTransaction& mergedTx,std::string& strFailReason,ScriptError& serror);
    bool SelectUnionTokenCoinsFromLimit(const std::vector<COutput>& vAvailableCoins, std::set<std::pair<const CWalletTx*, unsigned int> >& setCoinsRet,std::string & symbol,uint64_t& nTokenValue,\
            const CCoinControl *coinControl = NULL,int maxvinsize = 0) const;
    bool getUnionTxVinInfo(std::string& strtx,std::string& strError,std::string& info);


};

/** A key allocated from the key pool. */
class CReserveKey : public CReserveScript
{
protected:
    CWallet* pwallet;
    int64_t nIndex;
    CPubKey vchPubKey;
public:
    CReserveKey(CWallet* pwalletIn)
    {
        nIndex = -1;
        pwallet = pwalletIn;
    }

    ~CReserveKey()
    {
        ReturnKey();
    }

    void ReturnKey();
    bool GetReservedKey(CPubKey &pubkey);
    void KeepKey();
    void KeepScript() { KeepKey(); }
};


/** 
 * Account information.
 * Stored in wallet with key "acc"+string account name.
 */
class CAccount
{
public:
    CPubKey vchPubKey;

    CAccount()
    {
        SetNull();
    }

    void SetNull()
    {
        vchPubKey = CPubKey();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vchPubKey);
    }
};

// Helper for producing a bunch of max-sized low-S signatures (eg 72 bytes)
// ContainerType is meant to hold pair<CWalletTx *, int>, and be iterable
// so that each entry corresponds to each vIn, in order.
template <typename ContainerType>
bool CWallet::DummySignTx(CMutableTransaction &txNew, const ContainerType &coins,int nseek)
{
    // Fill in dummy signatures for fee calculation.
	int nIn = 0;
	if (nseek > 0)
		nIn = nseek;
    for (const auto& coin : coins)
    {
        const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
        SignatureData sigdata;
		bool isp2sh = scriptPubKey.IsPayToScriptHash();
		//std::cout << "IsPayToScriptHash() --- " << isp2sh << std::endl;
        if (!ProduceSignature(DummySignatureCreator(this), scriptPubKey, sigdata))
        {
            return false;
        } else {
            UpdateTransaction(txNew, nIn, sigdata);
        }

        nIn++;
    }
    return true;
}

#endif // BITCOIN_WALLET_WALLET_H
