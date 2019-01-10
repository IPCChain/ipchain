// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet/wallet.h"

#include "base58.h"
#include "checkpoints.h"
#include "chain.h"
#include "wallet/coincontrol.h"
#include "consensus/consensus.h"
#include "consensus/validation.h"
#include "key.h"
#include "keystore.h"
#include "validation.h"
#include "net.h"
#include "policy/policy.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/sign.h"
#include "timedata.h"
#include "txmempool.h"
#include "util.h"
#include "ui_interface.h"
#include "utilmoneystr.h"
#include "univalue.h"

#include <fstream>
#include <assert.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include "univalue/include/univalue.h"
#include "rpc/server.h"
#include "dpoc/DpocInfo.h"
#include "dpoc/TimeService.h"
#include "dpoc/ConsensusAccountPool.h"
#include "core_io.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace boost::property_tree;
using namespace std;

CWallet* pwalletMain = NULL;
/** Transaction fee set by the user */
CFeeRate payTxFee(DEFAULT_TRANSACTION_FEE*2);
unsigned int nTxConfirmTarget = DEFAULT_TX_CONFIRM_TARGET;
bool bSpendZeroConfChange = DEFAULT_SPEND_ZEROCONF_CHANGE;
bool fSendFreeTransactions = DEFAULT_SEND_FREE_TRANSACTIONS;
bool fWalletRbf = DEFAULT_WALLET_RBF;

const char * DEFAULT_WALLET_DAT = "wallet.dat";
const uint32_t BIP32_HARDENED_KEY_LIMIT = 0x80000000;
#define  UNION_NUMBER  5
#define  SIGNATURE_SIZE 150

/**
 * Fees smaller than this (in satoshi) are considered zero fee (for transaction creation)
 * Override with -mintxfee
 */
CFeeRate CWallet::minTxFee = CFeeRate(DEFAULT_TRANSACTION_MINFEE);
/**
 * If fee estimation does not have enough data to provide estimates, use this fee instead.
 * Has no effect if not using fee estimation
 * Override with -fallbackfee
 */
CFeeRate CWallet::fallbackFee = CFeeRate(DEFAULT_FALLBACK_FEE);

const uint256 CMerkleTx::ABANDON_HASH(uint256S("0000000000000000000000000000000000000000000000000000000000000001"));

/** @defgroup mapWallet
 *
 * @{
 */

struct CompareValueOnly
{
    bool operator()(const pair<CAmount, pair<const CWalletTx*, unsigned int> >& t1,
                    const pair<CAmount, pair<const CWalletTx*, unsigned int> >& t2) const
    {
        return t1.first < t2.first;
    }
};

std::string COutput::ToString() const
{
    return strprintf("COutput(%s, %d, %d) [%s]", tx->GetHash().ToString(), i, nDepth, FormatMoney(tx->tx->vout[i].nValue));
}

bool COutput::CanBeSentToOhter() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return false;
	return (tx->tx->vout[i].txType == TXOUT_IPCOWNER);
}

bool COutput::CanBeAuthorizedToOther() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return false;
	return (tx->tx->vout[i].ipcLabel.reAuthorize == 1);
}

bool COutput::CanBeUniqueAuthorizedToOther() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return false;
	if (tx->tx->vout[i].txType != TXOUT_IPCOWNER)
		return false;
	if (tx->tx->vout[i].ipcLabel.uniqueAuthorize == 0)
		return true;
}

int COutput::GetType() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return -1;
	return tx->tx->vout[i].txType;
}

int COutput::GetIPCExtendType() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return -1;
	return tx->tx->vout[i].ipcLabel.ExtendType;
}

uint32_t COutput::GetIPCStartTime() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return 0;
	return tx->tx->vout[i].ipcLabel.startTime;
}

uint32_t COutput::GetIPCStopTime() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return 0;
	return tx->tx->vout[i].ipcLabel.stopTime;
}

uint8_t  COutput::GetIPCreAuthorize() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return -1;
	return tx->tx->vout[i].ipcLabel.reAuthorize;
}

uint8_t  COutput::GetIPCUniqAuthorize()const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return -1;
	return tx->tx->vout[i].ipcLabel.uniqueAuthorize;
}

std::string COutput::GetIPCHash() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return "";
	return tx->tx->vout[i].ipcLabel.hash.GetHex();
}

std::string COutput::GetIPCTitle() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return "";
	return tx->tx->vout[i].ipcLabel.labelTitle;
}

std::string COutput::GetIPCLabel() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return "";
	return tx->tx->vout[i].txLabel;
}

std::string COutput::GetTokenSymbol() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return "";
	int txtype = GetType();
	if (txtype == TXOUT_TOKENREG)
	{
		return tx->tx->vout[i].tokenRegLabel.getTokenSymbol();
	}
	else if (txtype == TXOUT_TOKEN)
	{
		return tx->tx->vout[i].tokenLabel.getTokenSymbol();
	}
	else if (txtype == TXOUT_ADDTOKEN)
	{
		return tx->tx->vout[i].addTokenLabel.getTokenSymbol();
	}
	return "";
}
uint8_t  COutput::GetTokenAccuracy() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return 10;
	int txtype = GetType();
	if (txtype == TXOUT_TOKENREG)
	{
		return tx->tx->vout[i].tokenRegLabel.accuracy;
	}
	else if (txtype == TXOUT_TOKEN)
	{
		return tx->tx->vout[i].tokenLabel.accuracy;
	}
	else if (txtype == TXOUT_ADDTOKEN)
	{
		return tx->tx->vout[i].addTokenLabel.accuracy;
	}
	return 0;
}

uint64_t COutput::GetTokenvalue() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return 0;
	int txtype = GetType();
	if (txtype == TXOUT_TOKENREG)
	{
		return tx->tx->vout[i].tokenRegLabel.totalCount;
	}
	else if (txtype == TXOUT_TOKEN)
	{
		return tx->tx->vout[i].tokenLabel.value;
	}
	else if (txtype == TXOUT_ADDTOKEN)
	{
		return tx->tx->vout[i].addTokenLabel.currentCount;
	}
	return uint64_t(0);
}
int64_t COutput::GetAssetEntryTime() const{
	if (tx == NULL || tx->tx == NULL || i >= tx->tx->vout.size())
		return 0;
	int ntxtype = GetType();
	std::string stripchash = GetIPCHash();
	int64_t ipcstarttime = tx->tx->vout[i].ipcLabel.startTime;
	if (ntxtype != TXOUT_IPCOWNER && ntxtype != TXOUT_IPCAUTHORIZATION)
	{
		return tx->GetTimeOfTokenInChain()>ipcstarttime ? tx->GetTimeOfTokenInChain() : ipcstarttime;
	}
	if (pwalletMain->mapWallet.count(tx->tx->GetHash()) == 0)
		return 0;
	CWalletTx curtx = pwalletMain->mapWallet[tx->tx->GetHash()];
	CTransactionRef curptx = curtx.tx;
	bool isFindParenttx = false;
	int nindex = 0;
	while (true)
	{
		if (NULL == curptx)
			return 0;

		nindex = 0;
		BOOST_FOREACH(const CTxIn& txin, curptx->vin)
		{
			nindex++;
			isminetype isme= pwalletMain->IsMine(txin);
			if (isme == ISMINE_NO)
			{
				isFindParenttx = true;
				break;
			}
			if (pwalletMain->mapWallet.count(txin.prevout.hash) == 0)
				continue;
			CWalletTx& prev = pwalletMain->mapWallet[txin.prevout.hash];
			if (NULL == prev.tx)
				return 0;
			if (prev.tx->vout[txin.prevout.n].txType == ntxtype && stripchash == prev.tx->vout[txin.prevout.n].ipcLabel.hash.GetHex())
			{
				ipcstarttime = prev.tx->vout[txin.prevout.n].ipcLabel.startTime;
				curtx = prev;
				curptx = curtx.tx;
				break;
			}
			else if (nindex == curptx->vin.size())
			{
				isFindParenttx = true;
				break;
			}
		
			
		}
		if (isFindParenttx)
		{
			return curtx.GetTimeOfTokenInChain()>ipcstarttime ? curtx.GetTimeOfTokenInChain() : ipcstarttime;
		}
	
	}
	
	return 0;
}

const CWalletTx* CWallet::GetWalletTx(const uint256& hash) const
{
    LOCK(cs_wallet);
    std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(hash);
    if (it == mapWallet.end())
        return NULL;
    return &(it->second);
}

CPubKey CWallet::GenerateNewKey()
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    bool fCompressed = CanSupportFeature(FEATURE_COMPRPUBKEY); // default to compressed public keys if we want 0.6.0 wallets

    CKey secret;

    // Create new metadata
    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);

    // use HD key derivation if HD was enabled during wallet creation
    if (IsHDEnabled()) {
        DeriveNewChildKey(metadata, secret);
    } else {
        secret.MakeNewKey(fCompressed);
    }

    // Compressed public keys were introduced in version 0.6.0
    if (fCompressed)
        SetMinVersion(FEATURE_COMPRPUBKEY);

    CPubKey pubkey = secret.GetPubKey();
    assert(secret.VerifyPubKey(pubkey));

    mapKeyMetadata[pubkey.GetID()] = metadata;
    UpdateTimeFirstKey(nCreationTime);

    if (!AddKeyPubKey(secret, pubkey))
        throw std::runtime_error(std::string(__func__) + ": AddKey failed");
    return pubkey;
}

void CWallet::DeriveNewChildKey(CKeyMetadata& metadata, CKey& secret)
{
    // for now we use a fixed keypath scheme of m/0'/0'/k
    CKey key;                      //master key seed (256bit)
    CExtKey masterKey;             //hd master key
    CExtKey accountKey;            //key at m/0'
    CExtKey externalChainChildKey; //key at m/0'/0'
    CExtKey childKey;              //key at m/0'/0'/<n>'

    // try to get the master key
    if (!GetKey(hdChain.masterKeyID, key))
        throw std::runtime_error(std::string(__func__) + ": Master key not found");

    masterKey.SetMaster(key.begin(), key.size());

    // derive m/0'
    // use hardened derivation (child keys >= 0x80000000 are hardened after bip32)
    masterKey.Derive(accountKey, BIP32_HARDENED_KEY_LIMIT);

    // derive m/0'/0'
    accountKey.Derive(externalChainChildKey, BIP32_HARDENED_KEY_LIMIT);

    // derive child key at next index, skip keys already known to the wallet
    do {
        // always derive hardened keys
        // childIndex | BIP32_HARDENED_KEY_LIMIT = derive childIndex in hardened child-index-range
        // example: 1 | BIP32_HARDENED_KEY_LIMIT == 0x80000001 == 2147483649
        externalChainChildKey.Derive(childKey, hdChain.nExternalChainCounter | BIP32_HARDENED_KEY_LIMIT);
        metadata.hdKeypath = "m/0'/0'/" + std::to_string(hdChain.nExternalChainCounter) + "'";
        metadata.hdMasterKeyID = hdChain.masterKeyID;
        // increment childkey index
        hdChain.nExternalChainCounter++;
    } while (HaveKey(childKey.key.GetPubKey().GetID()));
    secret = childKey.key;

    // update the chain model in the database
    if (!CWalletDB(strWalletFile).WriteHDChain(hdChain))
        throw std::runtime_error(std::string(__func__) + ": Writing HD chain model failed");
}

bool CWallet::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey))
        return false;

    // check if we need to remove from watch-only
    CScript script;
    script = GetScriptForDestination(pubkey.GetID());
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);
    script = GetScriptForRawPubKey(pubkey);
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);

    if (!fFileBacked)
        return true;
    if (!IsCrypted()) {
        return CWalletDB(strWalletFile).WriteKey(pubkey,
                                                 secret.GetPrivKey(),
                                                 mapKeyMetadata[pubkey.GetID()]);
    }
    return true;
}

bool CWallet::AddCryptedKey(const CPubKey &vchPubKey,
                            const vector<unsigned char> &vchCryptedSecret)
{
    if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
        return false;
    if (!fFileBacked)
        return true;
    {
        LOCK(cs_wallet);
        if (pwalletdbEncryption)
            return pwalletdbEncryption->WriteCryptedKey(vchPubKey,
                                                        vchCryptedSecret,
                                                        mapKeyMetadata[vchPubKey.GetID()]);
        else
            return CWalletDB(strWalletFile).WriteCryptedKey(vchPubKey,
                                                            vchCryptedSecret,
                                                            mapKeyMetadata[vchPubKey.GetID()]);
    }
    return false;
}

bool CWallet::LoadKeyMetadata(const CTxDestination& keyID, const CKeyMetadata &meta)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    UpdateTimeFirstKey(meta.nCreateTime);
    mapKeyMetadata[keyID] = meta;
    return true;
}

bool CWallet::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret);
}

void CWallet::UpdateTimeFirstKey(int64_t nCreateTime)
{
    AssertLockHeld(cs_wallet);
    if (nCreateTime <= 1) {
        // Cannot determine birthday information, so set the wallet birthday to
        // the beginning of time.
        nTimeFirstKey = 1;
    } else if (!nTimeFirstKey || nCreateTime < nTimeFirstKey) {
        nTimeFirstKey = nCreateTime;
    }
}

bool CWallet::AddCScript(const CScript& redeemScript)
{
    if (!CCryptoKeyStore::AddCScript(redeemScript))
        return false;
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).WriteCScript(Hash160(redeemScript), redeemScript);
}

bool CWallet::LoadCScript(const CScript& redeemScript)
{
    /* A sanity check was added in pull #3843 to avoid adding redeemScripts
     * that never can be redeemed. However, old wallets may still contain
     * these. Do not add them to the wallet and warn. */
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
    {
        std::string strAddr = CBitcoinAddress(CScriptID(redeemScript)).ToString();
        LogPrintf("%s: Warning: This wallet contains a redeemScript of size %i which exceeds maximum size %i thus can never be redeemed. Do not use address %s.\n",
            __func__, redeemScript.size(), MAX_SCRIPT_ELEMENT_SIZE, strAddr);
        return true;
    }

    return CCryptoKeyStore::AddCScript(redeemScript);
}

bool CWallet::AddWatchOnly(const CScript& dest)
{
    if (!CCryptoKeyStore::AddWatchOnly(dest))
        return false;
    const CKeyMetadata& meta = mapKeyMetadata[CScriptID(dest)];
    UpdateTimeFirstKey(meta.nCreateTime);
    NotifyWatchonlyChanged(true);
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).WriteWatchOnly(dest, meta);
}

bool CWallet::AddWatchOnly(const CScript& dest, int64_t nCreateTime)
{
    mapKeyMetadata[CScriptID(dest)].nCreateTime = nCreateTime;
    return AddWatchOnly(dest);
}

bool CWallet::RemoveWatchOnly(const CScript &dest)
{
    AssertLockHeld(cs_wallet);
    if (!CCryptoKeyStore::RemoveWatchOnly(dest))
        return false;
    if (!HaveWatchOnly())
        NotifyWatchonlyChanged(false);
    if (fFileBacked)
        if (!CWalletDB(strWalletFile).EraseWatchOnly(dest))
            return false;

    return true;
}

bool CWallet::LoadWatchOnly(const CScript &dest)
{
    return CCryptoKeyStore::AddWatchOnly(dest);
}

bool CWallet::Unlock(const SecureString& strWalletPassphrase)
{
    CCrypter crypter;
    CKeyingMaterial vMasterKey;

    {
        LOCK(cs_wallet);
        BOOST_FOREACH(const MasterKeyMap::value_type& pMasterKey, mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                continue; // try another master key
			LogPrintf("[crypter.Decrypt]..vMasterKey.size() = %d \n",vMasterKey.size());
			if (CCryptoKeyStore::Unlock(vMasterKey))
			{
				curstrWalletPassphrase = strWalletPassphrase; 
				return true;
			}
               
        }
    }
    return false;
}

bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
{
    bool fWasLocked = IsLocked();

    {
        LOCK(cs_wallet);
        Lock();

        CCrypter crypter;
        CKeyingMaterial vMasterKey;
        BOOST_FOREACH(MasterKeyMap::value_type& pMasterKey, mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                return false;
            if (CCryptoKeyStore::Unlock(vMasterKey))
            {
                int64_t nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = pMasterKey.second.nDeriveIterations * (100 / ((double)(GetTimeMillis() - nStartTime)));

                nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + pMasterKey.second.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

                if (pMasterKey.second.nDeriveIterations < 25000)
                    pMasterKey.second.nDeriveIterations = 25000;

                LogPrintf("Wallet passphrase changed to an nDeriveIterations of %i\n", pMasterKey.second.nDeriveIterations);

                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(vMasterKey, pMasterKey.second.vchCryptedKey))
                    return false;
                CWalletDB(strWalletFile).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                if (fWasLocked)
                    Lock();
				curstrWalletPassphrase = strNewWalletPassphrase; 
                return true;
            }
        }
    }

    return false;
}

void CWallet::SetBestChain(const CBlockLocator& loc)
{
    LOCK(cs_wallet);
    CWalletDB walletdb(strWalletFile);
    walletdb.WriteBestBlock(loc);
}

bool CWallet::SetMinVersion(enum WalletFeature nVersion, CWalletDB* pwalletdbIn, bool fExplicit)
{
    LOCK(cs_wallet); // nWalletVersion
    if (nWalletVersion >= nVersion)
        return true;

    // when doing an explicit upgrade, if we pass the max version permitted, upgrade all the way
    if (fExplicit && nVersion > nWalletMaxVersion)
            nVersion = FEATURE_LATEST;

    nWalletVersion = nVersion;

    if (nVersion > nWalletMaxVersion)
        nWalletMaxVersion = nVersion;

    if (fFileBacked)
    {
        CWalletDB* pwalletdb = pwalletdbIn ? pwalletdbIn : new CWalletDB(strWalletFile);
        if (nWalletVersion > 40000)
            pwalletdb->WriteMinVersion(nWalletVersion);
        if (!pwalletdbIn)
            delete pwalletdb;
    }

    return true;
}

bool CWallet::SetMaxVersion(int nVersion)
{

    LOCK(cs_wallet); // nWalletVersion, nWalletMaxVersion
    // cannot downgrade below current version
    if (nWalletVersion > nVersion)
        return false;

    nWalletMaxVersion = nVersion;

    return true;
}

set<uint256> CWallet::GetConflicts(const uint256& txid) const
{
    set<uint256> result;
    AssertLockHeld(cs_wallet);

    std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(txid);
    if (it == mapWallet.end())
        return result;
    const CWalletTx& wtx = it->second;

    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;

    BOOST_FOREACH(const CTxIn& txin, wtx.tx->vin)
    {
        if (mapTxSpends.count(txin.prevout) <= 1)
            continue;  // No conflict if zero or one spends
        range = mapTxSpends.equal_range(txin.prevout);
        for (TxSpends::const_iterator _it = range.first; _it != range.second; ++_it)
            result.insert(_it->second);
    }
    return result;
}

bool CWallet::HasWalletSpend(const uint256& txid) const
{
    AssertLockHeld(cs_wallet);
    auto iter = mapTxSpends.lower_bound(COutPoint(txid, 0));
    return (iter != mapTxSpends.end() && iter->first.hash == txid);
}

void CWallet::Flush(bool shutdown)
{
    bitdb.Flush(shutdown);
}

bool CWallet::Verify()
{
    if (GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET))
        return true;
    LogPrintf("Using BerkeleyDB version %s\n", DbEnv::version(0, 0, 0));
    std::string walletFile = GetArg("-wallet", DEFAULT_WALLET_DAT);

    LogPrintf("Using wallet %s\n", walletFile);
    uiInterface.InitMessage(_("Verifying wallet..."));

    // Wallet file must be a plain filename without a directory
    if (walletFile != boost::filesystem::basename(walletFile) + boost::filesystem::extension(walletFile))
        return InitError(strprintf(_("Wallet %s resides outside data directory %s"), walletFile, GetDataDir().string()));

    if (!bitdb.Open(GetDataDir()))
    {
        // try moving the database env out of the way
        boost::filesystem::path pathDatabase = GetDataDir() / "database";
        boost::filesystem::path pathDatabaseBak = GetDataDir() / strprintf("database.%d.bak", GetTime());
        try {
            boost::filesystem::rename(pathDatabase, pathDatabaseBak);
            LogPrintf("Moved old %s to %s. Retrying.\n", pathDatabase.string(), pathDatabaseBak.string());
        } catch (const boost::filesystem::filesystem_error&) {
            // failure is ok (well, not really, but it's not worse than what we started with)
        }
        
        // try again
        if (!bitdb.Open(GetDataDir())) {
            // if it still fails, it probably means we can't even create the database env
            return InitError(strprintf(_("Error initializing wallet database environment %s!"), GetDataDir()));
        }
    }
    
    if (GetBoolArg("-salvagewallet", false))
    {
        // Recover readable keypairs:
        if (!CWalletDB::Recover(bitdb, walletFile, true))
            return false;
    }
    
    if (boost::filesystem::exists(GetDataDir() / walletFile))
    {
        CDBEnv::VerifyResult r = bitdb.Verify(walletFile, CWalletDB::Recover);
        if (r == CDBEnv::RECOVER_OK)
        {
            InitWarning(strprintf(_("Warning: Wallet file corrupt, data salvaged!"
                                         " Original %s saved as %s in %s; if"
                                         " your balance or transactions are incorrect you should"
                                         " restore from a backup."),
                walletFile, "wallet.{timestamp}.bak", GetDataDir()));
        }
        if (r == CDBEnv::RECOVER_FAIL){
            LogPrintf("Using wallet %s InitError r == CDBEnv::RECOVER_FAIL\n", walletFile);
         //   return InitError(strprintf(_("%s corrupt, salvage failed"), walletFile));
        }
    }
    
    return true;
}

void CWallet::SyncMetaData(pair<TxSpends::iterator, TxSpends::iterator> range)
{
    // We want all the wallet transactions in range to have the same metadata as
    // the oldest (smallest nOrderPos).
    // So: find smallest nOrderPos:

    int nMinOrderPos = std::numeric_limits<int>::max();
    const CWalletTx* copyFrom = NULL;
    for (TxSpends::iterator it = range.first; it != range.second; ++it)
    {
        const uint256& hash = it->second;
        int n = mapWallet[hash].nOrderPos;
        if (n < nMinOrderPos)
        {
            nMinOrderPos = n;
            copyFrom = &mapWallet[hash];
        }
    }
    // Now copy data from copyFrom to rest:
    for (TxSpends::iterator it = range.first; it != range.second; ++it)
    {
        const uint256& hash = it->second;
        CWalletTx* copyTo = &mapWallet[hash];
        if (copyFrom == copyTo) continue;
        if (!copyFrom->IsEquivalentTo(*copyTo)) continue;
        copyTo->mapValue = copyFrom->mapValue;
        copyTo->vOrderForm = copyFrom->vOrderForm;
        // fTimeReceivedIsTxTime not copied on purpose
        // nTimeReceived not copied on purpose
        copyTo->nTimeSmart = copyFrom->nTimeSmart;
        copyTo->fFromMe = copyFrom->fFromMe;
        copyTo->strFromAccount = copyFrom->strFromAccount;
        // nOrderPos not copied on purpose
        // cached members not copied on purpose
    }
}

/**
 * Outpoint is spent if any non-conflicted transaction
 * spends it:
 */
bool CWallet::IsSpent(const uint256& hash, unsigned int n) const
{

    const COutPoint outpoint(hash, n);
    pair<TxSpends::const_iterator, TxSpends::const_iterator> range;
    range = mapTxSpends.equal_range(outpoint);

	
    for (TxSpends::const_iterator it = range.first; it != range.second; ++it)
    {
        const uint256& wtxid = it->second;
        std::map<uint256, CWalletTx>::const_iterator mit = mapWallet.find(wtxid);
        if (mit != mapWallet.end()) {
            int depth = mit->second.GetDepthInMainChain();
            if (depth > 0  || (depth == 0 && !mit->second.isAbandoned()))
                return true; // Spent
        }
    }
    return false;
}

void CWallet::AddToSpends(const COutPoint& outpoint, const uint256& wtxid)
{
	mapTxSpends.insert(make_pair(outpoint, wtxid));

    pair<TxSpends::iterator, TxSpends::iterator> range;
    range = mapTxSpends.equal_range(outpoint);
    SyncMetaData(range);
}


void CWallet::AddToSpends(const uint256& wtxid)
{

    assert(mapWallet.count(wtxid));
    CWalletTx& thisTx = mapWallet[wtxid];
    if (thisTx.IsCoinBase()) // Coinbases don't spend anything!
        return;
    BOOST_FOREACH(const CTxIn& txin, thisTx.tx->vin)
        AddToSpends(txin.prevout, wtxid);
}

void CWallet::RemoveFromSpends(const COutPoint& outpoint, const uint256& wtxid)
{
	mapTxSpends.erase(outpoint);
}


void CWallet::RemoveFromSpends(const uint256& wtxid)
{
	int count = mapWallet.count(wtxid);
	assert(count);
	CWalletTx& thisTx = mapWallet[wtxid];
	if (thisTx.IsCoinBase()) // Coinbases don't spend anything!
		return;

	BOOST_FOREACH(const CTxIn& txin, thisTx.tx->vin)
		RemoveFromSpends(txin.prevout, wtxid);
}


bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
{
    if (IsCrypted())
        return false;
	LogPrintf("[EncryptWallet]:in ......................\n");
    CKeyingMaterial vMasterKey;

    vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    GetStrongRandBytes(&vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey;

    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    GetStrongRandBytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = 2500000 / ((double)(GetTimeMillis() - nStartTime));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

    if (kMasterKey.nDeriveIterations < 25000)
        kMasterKey.nDeriveIterations = 25000;

    LogPrintf("Encrypting Wallet with an nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey))
        return false;
	LogPrintf("[EncryptWallet]:fFileBacked = %d \n",fFileBacked);
    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        if (fFileBacked)
        {
            assert(!pwalletdbEncryption);
            pwalletdbEncryption = new CWalletDB(strWalletFile);
            if (!pwalletdbEncryption->TxnBegin()) {
                delete pwalletdbEncryption;
                pwalletdbEncryption = NULL;
                return false;
            }
            pwalletdbEncryption->WriteMasterKey(nMasterKeyMaxID, kMasterKey);
        }

        if (!EncryptKeys(vMasterKey))
        {
            if (fFileBacked) {
                pwalletdbEncryption->TxnAbort();
                delete pwalletdbEncryption;
            }
            // We now probably have half of our keys encrypted in memory, and half not...
            // die and let the user reload the unencrypted wallet.
            assert(false);
        }

        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, pwalletdbEncryption, true);

        if (fFileBacked)
        {
            if (!pwalletdbEncryption->TxnCommit()) {
                delete pwalletdbEncryption;
                // We now have keys encrypted in memory, but not on disk...
                // die to avoid confusion and let the user reload the unencrypted wallet.
                assert(false);
            }

            delete pwalletdbEncryption;
            pwalletdbEncryption = NULL;
        }

        Lock();
        Unlock(strWalletPassphrase);

        // if we are using HD, replace the HD master key (seed) with a new one
        if (IsHDEnabled()) {
            CKey key;
            CPubKey masterPubKey = GenerateNewHDMasterKey();
            if (!SetHDMasterKey(masterPubKey))
                return false;
        }

        NewKeyPool();
        Lock();

        // Need to completely rewrite the wallet file; if we don't, bdb might keep
        // bits of the unencrypted private key in slack space in the database file.
        CDB::Rewrite(strWalletFile);

    }
    NotifyStatusChanged(this);
    return true;
}

DBErrors CWallet::ReorderTransactions()
{
    LOCK(cs_wallet);
    CWalletDB walletdb(strWalletFile);

    // Old wallets didn't have any defined order for transactions
    // Probably a bad idea to change the output of this

    // First: get all CWalletTx and CAccountingEntry into a sorted-by-time multimap.
    typedef pair<CWalletTx*, CAccountingEntry*> TxPair;
    typedef multimap<int64_t, TxPair > TxItems;
    TxItems txByTime;

    for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        CWalletTx* wtx = &((*it).second);
        txByTime.insert(make_pair(wtx->nTimeReceived, TxPair(wtx, (CAccountingEntry*)0)));
    }
    list<CAccountingEntry> acentries;
    walletdb.ListAccountCreditDebit("", acentries);
    BOOST_FOREACH(CAccountingEntry& entry, acentries)
    {
        txByTime.insert(make_pair(entry.nTime, TxPair((CWalletTx*)0, &entry)));
    }

    nOrderPosNext = 0;
    std::vector<int64_t> nOrderPosOffsets;
    for (TxItems::iterator it = txByTime.begin(); it != txByTime.end(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;
        CAccountingEntry *const pacentry = (*it).second.second;
        int64_t& nOrderPos = (pwtx != 0) ? pwtx->nOrderPos : pacentry->nOrderPos;

        if (nOrderPos == -1)
        {
            nOrderPos = nOrderPosNext++;
            nOrderPosOffsets.push_back(nOrderPos);

            if (pwtx)
            {
                if (!walletdb.WriteTx(*pwtx))
                    return DB_LOAD_FAIL;
            }
            else
                if (!walletdb.WriteAccountingEntry(pacentry->nEntryNo, *pacentry))
                    return DB_LOAD_FAIL;
        }
        else
        {
            int64_t nOrderPosOff = 0;
            BOOST_FOREACH(const int64_t& nOffsetStart, nOrderPosOffsets)
            {
                if (nOrderPos >= nOffsetStart)
                    ++nOrderPosOff;
            }
            nOrderPos += nOrderPosOff;
            nOrderPosNext = std::max(nOrderPosNext, nOrderPos + 1);

            if (!nOrderPosOff)
                continue;

            // Since we're changing the order, write it back
            if (pwtx)
            {
                if (!walletdb.WriteTx(*pwtx))
                    return DB_LOAD_FAIL;
            }
            else
                if (!walletdb.WriteAccountingEntry(pacentry->nEntryNo, *pacentry))
                    return DB_LOAD_FAIL;
        }
    }
    walletdb.WriteOrderPosNext(nOrderPosNext);

    return DB_LOAD_OK;
}

int64_t CWallet::IncOrderPosNext(CWalletDB *pwalletdb)
{
    AssertLockHeld(cs_wallet); // nOrderPosNext
    int64_t nRet = nOrderPosNext++;
    if (pwalletdb) {
        pwalletdb->WriteOrderPosNext(nOrderPosNext);
    } else {
        CWalletDB(strWalletFile).WriteOrderPosNext(nOrderPosNext);
    }
    return nRet;
}

bool CWallet::AccountMove(std::string strFrom, std::string strTo, CAmount nAmount, std::string strComment)
{
    CWalletDB walletdb(strWalletFile);
    if (!walletdb.TxnBegin())
        return false;

    int64_t nNow = GetAdjustedTime();

    // Debit
    CAccountingEntry debit;
    debit.nOrderPos = IncOrderPosNext(&walletdb);
    debit.strAccount = strFrom;
    debit.nCreditDebit = -nAmount;
    debit.nTime = nNow;
    debit.strOtherAccount = strTo;
    debit.strComment = strComment;
    AddAccountingEntry(debit, &walletdb);

    // Credit
    CAccountingEntry credit;
    credit.nOrderPos = IncOrderPosNext(&walletdb);
    credit.strAccount = strTo;
    credit.nCreditDebit = nAmount;
    credit.nTime = nNow;
    credit.strOtherAccount = strFrom;
    credit.strComment = strComment;
    AddAccountingEntry(credit, &walletdb);

    if (!walletdb.TxnCommit())
        return false;

    return true;
}

bool CWallet::GetAccountPubkey(CPubKey &pubKey, std::string strAccount, bool bForceNew)
{
    CWalletDB walletdb(strWalletFile);

    CAccount account;
    walletdb.ReadAccount(strAccount, account);

    if (!bForceNew) {
        if (!account.vchPubKey.IsValid())
            bForceNew = true;
        else {
            // Check if the current key has been used
            CScript scriptPubKey = GetScriptForDestination(account.vchPubKey.GetID());
            for (map<uint256, CWalletTx>::iterator it = mapWallet.begin();
                 it != mapWallet.end() && account.vchPubKey.IsValid();
                 ++it)
                BOOST_FOREACH(const CTxOut& txout, (*it).second.tx->vout)
                    if (txout.scriptPubKey == scriptPubKey) {
                        bForceNew = true;
                        break;
                    }
        }
    }

    // Generate a new key
    if (bForceNew) {
        if (!GetKeyFromPool(account.vchPubKey))
            return false;

        SetAddressBook(account.vchPubKey.GetID(), strAccount, "receive");
        walletdb.WriteAccount(strAccount, account);
    }

    pubKey = account.vchPubKey;

    return true;
}

void CWallet::MarkDirty()
{
    {
        LOCK(cs_wallet);
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
            item.second.MarkDirty();
    }
}

bool CWallet::MarkReplaced(const uint256& originalHash, const uint256& newHash)
{
    LOCK(cs_wallet);

    auto mi = mapWallet.find(originalHash);

    // There is a bug if MarkReplaced is not called on an existing wallet transaction.
    assert(mi != mapWallet.end());

    CWalletTx& wtx = (*mi).second;

    // Ensure for now that we're not overwriting data
    assert(wtx.mapValue.count("replaced_by_txid") == 0);

    wtx.mapValue["replaced_by_txid"] = newHash.ToString();

    CWalletDB walletdb(strWalletFile, "r+");

    bool success = true;
    if (!walletdb.WriteTx(wtx)) {
        LogPrintf("%s: Updating walletdb tx %s failed", __func__, wtx.GetHash().ToString());
        success = false;
    }

    NotifyTransactionChanged(this, originalHash, CT_UPDATED);

    return success;
}

bool CWallet::AddToWallet(const CWalletTx& wtxIn, bool fFlushOnClose)
{
    LOCK(cs_wallet);

    CWalletDB walletdb(strWalletFile, "r+", fFlushOnClose);

    uint256 hash = wtxIn.GetHash();

    // Inserts only if not already there, returns tx inserted or tx found
    pair<map<uint256, CWalletTx>::iterator, bool> ret = mapWallet.insert(make_pair(hash, wtxIn));
    CWalletTx& wtx = (*ret.first).second;
    wtx.BindWallet(this);
    bool fInsertedNew = ret.second;
    if (fInsertedNew)
    {
        wtx.nTimeReceived = GetAdjustedTime();
        wtx.nOrderPos = IncOrderPosNext(&walletdb);
        wtxOrdered.insert(make_pair(wtx.nOrderPos, TxPair(&wtx, (CAccountingEntry*)0)));

        wtx.nTimeSmart = wtx.nTimeReceived;
        if (!wtxIn.hashUnset())
        {
            if (mapBlockIndex.count(wtxIn.hashBlock))
            {
                int64_t latestNow = wtx.nTimeReceived;
                int64_t latestEntry = 0;
                {
                    // Tolerate times up to the last timestamp in the wallet not more than 5 minutes into the future
                    int64_t latestTolerated = latestNow + 300;
                    const TxItems & txOrdered = wtxOrdered;
                    for (TxItems::const_reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
                    {
                        CWalletTx *const pwtx = (*it).second.first;
                        if (pwtx == &wtx)
                            continue;
                        CAccountingEntry *const pacentry = (*it).second.second;
                        int64_t nSmartTime;
                        if (pwtx)
                        {
                            nSmartTime = pwtx->nTimeSmart;
                            if (!nSmartTime)
                                nSmartTime = pwtx->nTimeReceived;
                        }
                        else
                            nSmartTime = pacentry->nTime;
                        if (nSmartTime <= latestTolerated)
                        {
                            latestEntry = nSmartTime;
                            if (nSmartTime > latestNow)
                                latestNow = nSmartTime;
                            break;
                        }
                    }
                }

                int64_t blocktime = mapBlockIndex[wtxIn.hashBlock]->GetBlockTime();
                wtx.nTimeSmart = std::max(latestEntry, std::min(blocktime, latestNow));
            }
            else
                LogPrintf("AddToWallet(): found %s in block %s not in index\n",
                         wtxIn.GetHash().ToString(),
                         wtxIn.hashBlock.ToString());
        }
        AddToSpends(hash);
    }

    bool fUpdated = false;
    if (!fInsertedNew)
	{

        // Merge
        if (!wtxIn.hashUnset() && wtxIn.hashBlock != wtx.hashBlock)
        {
            wtx.hashBlock = wtxIn.hashBlock;
            fUpdated = true;
        }
        // If no longer abandoned, update
        if (wtxIn.hashBlock.IsNull() && wtx.isAbandoned())
        {
            wtx.hashBlock = wtxIn.hashBlock;
            fUpdated = true;
        }
        if (wtxIn.nIndex != -1 && (wtxIn.nIndex != wtx.nIndex))
        {
            wtx.nIndex = wtxIn.nIndex;
            fUpdated = true;
        }
        if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
        {
            wtx.fFromMe = wtxIn.fFromMe;
            fUpdated = true;
        }
    }

    //// debug print
    LogPrintf("AddToWallet %s  %s%s\n", wtxIn.GetHash().ToString(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

    // Write to disk
    if (fInsertedNew || fUpdated)
	if (!walletdb.WriteTx(wtx))
	{
		return false;
	}
            

    // Break debit/credit balance caches:
    wtx.MarkDirty();

    // Notify UI of new or updated transaction
    NotifyTransactionChanged(this, hash, fInsertedNew ? CT_NEW : CT_UPDATED);

    // notify an external script when a wallet transaction comes in or is updated
    std::string strCmd = GetArg("-walletnotify", "");

    if ( !strCmd.empty())
    {
        boost::replace_all(strCmd, "%s", wtxIn.GetHash().GetHex());
        boost::thread t(runCommand, strCmd); // thread runs free
    }

    return true;
}

bool CWallet::RemoveFromWallet(const CWalletTx& wtxIn, bool fFlushOnClose)
{
	LOCK(cs_wallet);

	CWalletDB walletdb(strWalletFile, "r+", fFlushOnClose);

	uint256 hash = wtxIn.GetHash();

	RemoveFromSpends(hash);  

	bool fUpdated = false;
	//// debug print
	LogPrintf("RemoveFromWallet %s  %s\n", wtxIn.GetHash().ToString(), "removed");

	int ret = mapWallet.erase(hash);

	// Write to disk
	if (!walletdb.EraseTx(hash))
		return false;	

	// Notify UI of new or updated transaction
	NotifyTransactionChanged(this, hash, CT_DELETED);

	// notify an external script when a wallet transaction comes in or is updated
	std::string strCmd = GetArg("-walletnotify", "");

	if (!strCmd.empty())
	{
		boost::replace_all(strCmd, "%s", wtxIn.GetHash().GetHex());
		boost::thread t(runCommand, strCmd); // thread runs free
	}

	return true;

}
bool CWallet::LoadToWallet(const CWalletTx& wtxIn)
{
    uint256 hash = wtxIn.GetHash();

    mapWallet[hash] = wtxIn;
    CWalletTx& wtx = mapWallet[hash];
    wtx.BindWallet(this);
    wtxOrdered.insert(make_pair(wtx.nOrderPos, TxPair(&wtx, (CAccountingEntry*)0)));
    AddToSpends(hash);
    BOOST_FOREACH(const CTxIn& txin, wtx.tx->vin) {
        if (mapWallet.count(txin.prevout.hash)) {
            CWalletTx& prevtx = mapWallet[txin.prevout.hash];
            if (prevtx.nIndex == -1 && !prevtx.hashUnset()) {
                MarkConflicted(prevtx.hashBlock, wtx.GetHash());
            }
        }
    }

    return true;
}

/**
 * Add a transaction to the wallet, or update it.  pIndex and posInBlock should
 * be set when the transaction was known to be included in a block.  When
 * posInBlock = SYNC_TRANSACTION_NOT_IN_BLOCK (-1) , then wallet state is not
 * updated in AddToWallet, but notifications happen and cached balances are
 * marked dirty.
 * If fUpdate is true, existing transactions will be updated.
 * TODO: One exception to this is that the abandoned state is cleared under the
 * assumption that any further notification of a transaction that was considered
 * abandoned is an indication that it is not safe to be considered abandoned.
 * Abandoned state should probably be more carefuly tracked via different
 * posInBlock signals or by checking mempool presence when necessary.
 */
bool CWallet::AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlockIndex* pIndex, int posInBlock, bool fUpdate)
{
    {
        AssertLockHeld(cs_wallet);

        if (posInBlock != -1) {
            BOOST_FOREACH(const CTxIn& txin, tx.vin) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(txin.prevout);
                while (range.first != range.second) {
                    if (range.first->second != tx.GetHash()) {
                        LogPrintf("Transaction %s (in block %s) conflicts with wallet transaction %s (both spend %s:%i)\n", tx.GetHash().ToString(), pIndex->GetBlockHash().ToString(), range.first->second.ToString(), range.first->first.hash.ToString(), range.first->first.n);
                        MarkConflicted(pIndex->GetBlockHash(), range.first->second);
                    }
                    range.first++;
                }
            }
        }

        bool fExisted = mapWallet.count(tx.GetHash()) != 0;
		if (fExisted && !fUpdate)
		{
			std::cout << "fExisted :" << fExisted << " ,fUpdate : " << fUpdate << std::endl;
			return false;
		}
        if (fExisted || IsMine(tx) || IsFromMe(tx) || IsFromMe(tx,1)||checkMyUnion(tx))
        {		
            CWalletTx wtx(this, MakeTransactionRef(tx));

            // Get merkle branch if transaction was found in a block
            if (posInBlock != -1)
                wtx.SetMerkleBranch(pIndex, posInBlock);
            return AddToWallet(wtx, false);
        }
    }
    return false;
}


bool CWallet::checkMyUnion(const CTransaction& tx)
{


    BOOST_FOREACH(const CTxOut& txout, tx.vout) {
        CTxDestination utxoaddress;
        txnouttype typeRet;
        vector<CTxDestination> prevdestes;
        int nRequiredRet;
        bool fValidAddress = ExtractDestinations(txout.scriptPubKey, typeRet, prevdestes, nRequiredRet);
        if(fValidAddress)
        {
            BOOST_FOREACH(CTxDestination &prevdest, prevdestes)
            {
                CBitcoinAddress add(prevdest);
                std::string address = CBitcoinAddress(prevdest).ToString();
                    if(address.size()>5&&(address[0]=='2'||address[0]=='3'))
                          if(mapAddressBook.find(prevdest) != mapAddressBook.end())
                          {
                            std::cout<<"add true"<<std::endl;
                            return true;
                          }
            }
        }
    }
    if(vinsFindAddress(tx.vin,""))
        return true;
    return false;

}
bool CWallet::AbandonTransaction(const uint256& hashTx)
{
    LOCK2(cs_main, cs_wallet);

    CWalletDB walletdb(strWalletFile, "r+");

    std::set<uint256> todo;
    std::set<uint256> done;

    // Can't mark abandoned if confirmed or in mempool
    assert(mapWallet.count(hashTx));
    CWalletTx& origtx = mapWallet[hashTx];
    if (origtx.GetDepthInMainChain() > 0 || origtx.InMempool()) {
        return false;
    }

    todo.insert(hashTx);

    while (!todo.empty()) {
        uint256 now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        assert(mapWallet.count(now));
        CWalletTx& wtx = mapWallet[now];
        int currentconfirm = wtx.GetDepthInMainChain();
        // If the orig tx was not in block, none of its spends can be
        assert(currentconfirm <= 0);
        // if (currentconfirm < 0) {Tx and spends are already conflicted, no need to abandon}
        if (currentconfirm == 0 && !wtx.isAbandoned()) {
            // If the orig tx was not in block/mempool, none of its spends can be in mempool
            assert(!wtx.InMempool());
            wtx.nIndex = -1;
            wtx.setAbandoned();
            wtx.MarkDirty();
            walletdb.WriteTx(wtx);
            NotifyTransactionChanged(this, wtx.GetHash(), CT_UPDATED);
            // Iterate over all its outputs, and mark transactions in the wallet that spend them abandoned too
            TxSpends::const_iterator iter = mapTxSpends.lower_bound(COutPoint(hashTx, 0));
            while (iter != mapTxSpends.end() && iter->first.hash == now) {
                if (!done.count(iter->second)) {
                    todo.insert(iter->second);
                }
                iter++;
            }
            // If a transaction changes 'conflicted' state, that changes the balance
            // available of the outputs it spends. So force those to be recomputed
            BOOST_FOREACH(const CTxIn& txin, wtx.tx->vin)
            {
                if (mapWallet.count(txin.prevout.hash))
                    mapWallet[txin.prevout.hash].MarkDirty();
            }
        }
    }

    return true;
}

void CWallet::MarkConflicted(const uint256& hashBlock, const uint256& hashTx)
{
    LOCK2(cs_main, cs_wallet);

    int conflictconfirms = 0;
    if (mapBlockIndex.count(hashBlock)) {
        CBlockIndex* pindex = mapBlockIndex[hashBlock];
        if (chainActive.Contains(pindex)) {
            conflictconfirms = -(chainActive.Height() - pindex->nHeight + 1);
        }
    }
    // If number of conflict confirms cannot be determined, this means
    // that the block is still unknown or not yet part of the main chain,
    // for example when loading the wallet during a reindex. Do nothing in that
    // case.
    if (conflictconfirms >= 0)
        return;

    // Do not flush the wallet here for performance reasons
    CWalletDB walletdb(strWalletFile, "r+", false);

    std::set<uint256> todo;
    std::set<uint256> done;

    todo.insert(hashTx);

    while (!todo.empty()) {
        uint256 now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        assert(mapWallet.count(now));
        CWalletTx& wtx = mapWallet[now];
        int currentconfirm = wtx.GetDepthInMainChain();
        if (conflictconfirms < currentconfirm) {
            // Block is 'more conflicted' than current confirm; update.
            // Mark transaction as conflicted with this block.
            wtx.nIndex = -1;
            wtx.hashBlock = hashBlock;
            wtx.MarkDirty();
            walletdb.WriteTx(wtx);
            // Iterate over all its outputs, and mark transactions in the wallet that spend them conflicted too
            TxSpends::const_iterator iter = mapTxSpends.lower_bound(COutPoint(now, 0));
            while (iter != mapTxSpends.end() && iter->first.hash == now) {
                 if (!done.count(iter->second)) {
                     todo.insert(iter->second);
                 }
                 iter++;
            }
            // If a transaction changes 'conflicted' state, that changes the balance
            // available of the outputs it spends. So force those to be recomputed
            BOOST_FOREACH(const CTxIn& txin, wtx.tx->vin)
            {
                if (mapWallet.count(txin.prevout.hash))
                    mapWallet[txin.prevout.hash].MarkDirty();
            }
        }
    }
}

void CWallet::SyncTransaction(const CTransaction& tx, const CBlockIndex *pindex, int posInBlock)
{
    LOCK2(cs_main, cs_wallet);

    if (!AddToWalletIfInvolvingMe(tx, pindex, posInBlock, true))
        return; // Not one of ours

    // If a transaction changes 'conflicted' state, that changes the balance
    // available of the outputs it spends. So force those to be
    // recomputed, also:
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        if (mapWallet.count(txin.prevout.hash))
            mapWallet[txin.prevout.hash].MarkDirty();
    }
}


isminetype CWallet::IsMine(const CTxIn &txin) const
{
    {
        LOCK(cs_wallet);
        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.tx->vout.size())
                return IsMine(prev.tx->vout[txin.prevout.n]);
        }
    }
    return ISMINE_NO;
}



// Note that this function doesn't distinguish between a 0-valued input,
// and a not-"is mine" (according to the filter) input.
CAmount CWallet::GetDebit(const CTxIn &txin, const isminefilter& filter) const
{
    {
        LOCK(cs_wallet);
        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.tx->vout.size())
                if (IsMine(prev.tx->vout[txin.prevout.n]) & filter)
                    return prev.tx->vout[txin.prevout.n].nValue;
        }
    }
    return 0;
}

isminetype CWallet::IsMine(const CTxOut& txout) const
{
    return ::IsMine(*this, txout.scriptPubKey);
}



CAmount CWallet::GetCredit(const CTxOut& txout, const isminefilter& filter) const
{
    if (!MoneyRange(txout.nValue))
        throw std::runtime_error(std::string(__func__) + ": value out of range");
    return ((IsMine(txout) & filter) ? txout.nValue : 0);
}

bool CWallet::IsChange(const CTxOut& txout) const
{
    // TODO: fix handling of 'change' outputs. The assumption is that any
    // payment to a script that is ours, but is not in the address book
    // is change. That assumption is likely to break when we implement multisignature
    // wallets that return change back into a multi-signature-protected address;
    // a better way of identifying which outputs are 'the send' and which are
    // 'the change' will need to be implemented (maybe extend CWalletTx to remember
    // which output, if any, was change).
    if (::IsMine(*this, txout.scriptPubKey))
    {
        CTxDestination address;
        if (!ExtractDestination(txout.scriptPubKey, address))
            return true;

        LOCK(cs_wallet);
        if (!mapAddressBook.count(address))
            return true;
    }
    return false;
}


CAmount CWallet::GetChange(const CTxOut& txout) const
{
    if (!MoneyRange(txout.nValue))
        throw std::runtime_error(std::string(__func__) + ": value out of range");
    return (IsChange(txout) ? txout.nValue : 0);
}

bool CWallet::IsMine(const CTransaction& tx) const
{
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
        if (IsMine(txout))
            return true;
    return false;
}



bool CWallet::IsFromMe(const CTransaction& tx) const
{
    return (GetDebit(tx, ISMINE_ALL) > 0);
}

bool CWallet::IsFromMe(const CTransaction& tx ,int type) const
{
	BOOST_FOREACH(const CTxIn& txin, tx.vin)
	if (IsMine(txin))
		return true;
	return false;
}

CAmount CWallet::GetDebit(const CTransaction& tx, const isminefilter& filter) const
{
    CAmount nDebit = 0;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        nDebit += GetDebit(txin, filter);
        if (!MoneyRange(nDebit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nDebit;
}

bool CWallet::IsAllFromMe(const CTransaction& tx, const isminefilter& filter) const
{
    LOCK(cs_wallet);

    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        auto mi = mapWallet.find(txin.prevout.hash);
        if (mi == mapWallet.end())
            return false; // any unknown inputs can't be from us

        const CWalletTx& prev = (*mi).second;

        if (txin.prevout.n >= prev.tx->vout.size())
            return false; // invalid input!

        if (!(IsMine(prev.tx->vout[txin.prevout.n]) & filter))
            return false;
    }
    return true;
}

CAmount CWallet::GetCredit(const CTransaction& tx, const isminefilter& filter) const
{
    CAmount nCredit = 0;
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        nCredit += GetCredit(txout, filter);
        if (!MoneyRange(nCredit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nCredit;
}

CAmount CWallet::GetChange(const CTransaction& tx) const
{
	uint8_t txtp = tx.GetTxType();
	CAmount nChange = 0;
	if (tx.IsCoinBase())
		return nChange;
	if (txtp != TXOUT_NORMAL)
	{
		BOOST_FOREACH(const CTxOut& txout, tx.vout)
		{
			if (txout.txType!=TXOUT_NORMAL)
				continue;
            nChange = txout.nValue;//GetChange(txout);
			if (!MoneyRange(nChange))
				throw std::runtime_error(std::string(__func__) + ": value out of range");
		}
	}
	else
	{
		int index = -2;
		if (!GetChangeIndex(tx, index))
		{
			return nChange;
		}
		else{
			nChange = tx.vout[index].nValue;
			if (!MoneyRange(nChange))
				throw std::runtime_error(std::string(__func__) + ": value out of range");
		}
		
	}
	

    return nChange;
}

CPubKey CWallet::GenerateNewHDMasterKey()
{
    CKey key;
    key.MakeNewKey(true);

    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);

    // calculate the pubkey
    CPubKey pubkey = key.GetPubKey();
    assert(key.VerifyPubKey(pubkey));

    // set the hd keypath to "m" -> Master, refers the masterkeyid to itself
    metadata.hdKeypath     = "m";
    metadata.hdMasterKeyID = pubkey.GetID();

    {
        LOCK(cs_wallet);

        // mem store the metadata
        mapKeyMetadata[pubkey.GetID()] = metadata;

        // write the key&metadata to the database
        if (!AddKeyPubKey(key, pubkey))
            throw std::runtime_error(std::string(__func__) + ": AddKeyPubKey failed");
    }

    return pubkey;
}

bool CWallet::SetHDMasterKey(const CPubKey& pubkey)
{
    LOCK(cs_wallet);

    // ensure this wallet.dat can only be opened by clients supporting HD
    SetMinVersion(FEATURE_HD);

    // store the keyid (hash160) together with
    // the child index counter in the database
    // as a hdchain object
    CHDChain newHdChain;
    newHdChain.masterKeyID = pubkey.GetID();
    SetHDChain(newHdChain, false);

    return true;
}

bool CWallet::SetHDChain(const CHDChain& chain, bool memonly)
{
    LOCK(cs_wallet);
    if (!memonly && !CWalletDB(strWalletFile).WriteHDChain(chain))
        throw runtime_error(std::string(__func__) + ": writing chain failed");

    hdChain = chain;
    return true;
}

bool CWallet::IsHDEnabled()
{
    return !hdChain.masterKeyID.IsNull();
}

int64_t CWalletTx::GetTxTime() const
{
    int64_t n = nTimeSmart;
    return n ? n : nTimeReceived;
}

int CWalletTx::GetRequestCount() const
{
    // Returns -1 if it wasn't being tracked
    int nRequests = -1;
    {
        LOCK(pwallet->cs_wallet);
        if (IsCoinBase())
        {
            // Generated block
            if (!hashUnset())
            {
                map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                if (mi != pwallet->mapRequestCount.end())
                    nRequests = (*mi).second;
            }
        }
        else
        {
            // Did anyone request this transaction?
            map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(GetHash());
            if (mi != pwallet->mapRequestCount.end())
            {
                nRequests = (*mi).second;

                // How about the block it's in?
                if (nRequests == 0 && !hashUnset())
                {
                    map<uint256, int>::const_iterator _mi = pwallet->mapRequestCount.find(hashBlock);
                    if (_mi != pwallet->mapRequestCount.end())
                        nRequests = (*_mi).second;
                    else
                        nRequests = 1; // If it's in someone else's block it must have got out
                }
            }
        }
    }
    return nRequests;
}

void CWalletTx::GetAmounts2(list<COutputEntry>& listReceived,
                           list<COutputEntry>& listSent, CAmount& nFee, string& strSentAccount, const isminefilter& filter) const
{
    nFee = 0;
    listReceived.clear();
    listSent.clear();
    strSentAccount = strFromAccount;

    // Compute fee:
    CAmount nDebit = GetDebit(filter);
    if (nDebit > 0) // debit>0 means we signed/sent this transaction
    {
        CAmount nValueOut = tx->GetValueOut();
        nFee = nDebit - nValueOut;
    }
	int nindex = -2; 
	bool isFindtxChange = false;
	if (pwallet->GetChangeIndex(*tx, nindex))
	{
		isFindtxChange = true;
	}

	uint8_t txtp = tx->GetTxType(); //Get the type of transaction
	//The change in ipchain trading last Vout
    // Sent/received.
	unsigned int nSize = tx->vout.size();
	if (tx->IsCoinBase())
		nSize = 1;
    for (unsigned int i = 0; i < nSize; ++i)
    {
        const CTxOut& txout = tx->vout[i];
		uint8_t txtype = txout.txType;
		if (txtype != TXOUT_NORMAL || txtype!=txtp) 
			continue;
        isminetype fIsMine = pwallet->IsMine(txout);
        // Only need to handle txouts if AT LEAST one of these is true:
        //   1) they debit from us (sent)
        //   2) the output is to us (received)
        if (nDebit > 0)
        {
            // Don't report 'change' txouts
			if (pwallet->IsChange(txout) ) 
                continue;
			if (isFindtxChange && nindex == i)
				continue;
        }
        else if (!(fIsMine & filter))
            continue;

        // In either case, we need to get the destination address
        CTxDestination address;

        if (!ExtractDestination(txout.scriptPubKey, address) && !txout.scriptPubKey.IsUnspendable())
        {
            LogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
                     this->GetHash().ToString());
            address = CNoDestination();
        }

        COutputEntry output = {address, txout.nValue, (int)i};

        // If we are debited by the transaction, add the output as a "sent" entry
        if (nDebit > 0)
            listSent.push_back(output);

        // If we are receiving the output, add it as a "received" entry
		if ((fIsMine & filter))
            listReceived.push_back(output);
    }

}

void CWalletTx::GetAmountsIP(int IPtype, list<COutputEntryIP>& listReceived,
	list<COutputEntryIP>& listSent, CAmount& nFee, string& strSentAccount, const isminefilter& filter) const
{
	nFee = 0;
	listReceived.clear();
	listSent.clear();
	strSentAccount = strFromAccount;

	// Compute fee:
	CAmount nDebit = GetDebit(filter);
	if (nDebit > 0) // debit>0 means we signed/sent this transaction
	{
		CAmount nValueOut = tx->GetValueOut();
		nFee = nDebit - nValueOut;
	}
	uint8_t nIpDebit = pwallet->GetDebitOfIp(*tx, filter);

	int nindex = -2;
	bool isFindtxChange = false;
	if (pwallet->GetChangeIndex(*tx, nindex))
	{
		isFindtxChange = true;
	}
	uint8_t txtp = tx->GetTxType(); //Get the type of transaction
	//The change in ipchain trading last Vout
	// Sent/received.
	unsigned int nSize = tx->vout.size();
	for (unsigned int i = 0; i < nSize; ++i)
	{
		const CTxOut& txout = tx->vout[i];
		uint8_t txtype = txout.txType;
		uint8_t ipExtendType = txout.ipcLabel.ExtendType;
		if (txtype != txtp )
			continue;
		if (txtype != TXOUT_IPCOWNER && txtype != TXOUT_IPCAUTHORIZATION)
			continue;
		if (IPtype != -1 && IPtype != ipExtendType)
			continue;
		isminetype fIsMine = pwallet->IsMine(txout);
		// Only need to handle txouts if AT LEAST one of these is true:
		//   1) they debit from us (sent)
		//   2) the output is to us (received)
		if (nDebit > 0)
		{
			// Don't report 'change' txouts
			if (pwallet->IsChange(txout))
				continue;
			if (isFindtxChange && nindex == i)
				continue;
		}
		else if (!(fIsMine & filter))
			continue;
		// In either case, we need to get the destination address
		CTxDestination address;

		if (!ExtractDestination(txout.scriptPubKey, address) && !txout.scriptPubKey.IsUnspendable())
		{
			LogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
				this->GetHash().ToString());
			address = CNoDestination();
		}
		uint8_t iptype = ipExtendType;
		std::string iptitle = txout.ipcLabel.labelTitle;
		std::string iphash = txout.ipcLabel.hash.GetHex();
		COutputEntryIP output = { address, txout.nValue, (int)i, txtype, iptype, iptitle, iphash };
		// If we are debited by the transaction, add the output as a "sent" entry
		if (nIpDebit > 0 )
			listSent.push_back(output);

		// If we are receiving the output, add it as a "received" entry
		if ((fIsMine & filter))
			listReceived.push_back(output);

	}

}

void CWalletTx::GetAmountsToken(const std::string& strtokensymbol, list<COutputEntryToken>& listReceived,
	list<COutputEntryToken>& listSent, CAmount& nFee, string& strSentAccount, const isminefilter& filter) const
{
	nFee = 0;
	listReceived.clear();
	listSent.clear();
	strSentAccount = strFromAccount;

	// Compute fee:
	CAmount nDebit = GetDebit(filter);
	if (nDebit > 0) // debit>0 means we signed/sent this transaction
	{
		CAmount nValueOut = tx->GetValueOut();
		nFee = nDebit - nValueOut;
	}
	uint64_t ntokenDebit = pwallet->GetDebitOfToken(*tx,filter);
	int nindex = -2;
	bool isFindtxChange = false;
	if (pwallet->GetChangeIndex(*tx, nindex))
	{
		isFindtxChange = true;
	}
	uint8_t txtp = tx->GetTxType(); //Get the type of transaction
	//The change in ipchain trading last Vout
	// Sent/received.
	unsigned int nSize = tx->vout.size();
	for (unsigned int i = 0; i < nSize; ++i)
	{
		const CTxOut& txout = tx->vout[i];
		uint8_t txtype = txout.txType;
		if (txtype != txtp)
			continue;
		if (txtype != TXOUT_TOKENREG && txtype != TXOUT_TOKEN && txtype != TXOUT_ADDTOKEN)
			continue;
		isminetype fIsMine = pwallet->IsMine(txout);
		// Only need to handle txouts if AT LEAST one of these is true:
		//   1) they debit from us (sent)
		//   2) the output is to us (received)
		if (nDebit > 0)
        {
			// Don't report 'change' txouts
			if (pwallet->IsChange(txout))
				continue;
			if (isFindtxChange && nindex == i)
				continue;
		}
        else if (!(fIsMine & filter))
			continue;
		// In either case, we need to get the destination address
		CTxDestination address;

		if (!ExtractDestination(txout.scriptPubKey, address) && !txout.scriptPubKey.IsUnspendable())
		{
			LogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
				this->GetHash().ToString());
			address = CNoDestination();
		}
		std::string strsymbol = "";
		uint8_t  accuracy = -1;
		uint64_t tokenvalue = 0;
		if (txtype == TXOUT_TOKENREG)
		{
			strsymbol = txout.tokenRegLabel.getTokenSymbol();
			accuracy = txout.tokenRegLabel.accuracy;
			tokenvalue = txout.tokenRegLabel.totalCount;
		}
		else if (txtype == TXOUT_TOKEN){
			strsymbol = txout.tokenLabel.getTokenSymbol();
			accuracy = txout.tokenLabel.accuracy;
			tokenvalue = txout.tokenLabel.value;
		}
		else if (txtype == TXOUT_ADDTOKEN)
		{
			strsymbol = txout.addTokenLabel.getTokenSymbol();
			accuracy = txout.addTokenLabel.accuracy;
			tokenvalue = txout.addTokenLabel.currentCount;
		}
		if (strsymbol != strtokensymbol)
			continue;
		COutputEntryToken output = { address, txout.nValue, (int)i, txtype, strsymbol ,accuracy,tokenvalue};
		// If we are debited by the transaction, add the output as a "sent" entry
		if (ntokenDebit > 0)
			listSent.push_back(output);

		// If we are receiving the output, add it as a "received" entry
		if ((fIsMine & filter))
			listReceived.push_back(output);

	}

}


void CWalletTx::GetAmountsTokenForUnion(const std::string& strtokensymbol, list<COutputEntryToken>& listReceived,
    list<COutputEntryToken>& listSent, CAmount& nFee, string& strSentAccount, const isminefilter& filter,std::vector<UnionAddressInfo>&unionaddresses) const
{
    nFee = 0;
    listReceived.clear();
    listSent.clear();
    strSentAccount = strFromAccount;

    // Compute fee:
    CAmount nDebit = GetDebitForUnion(filter,unionaddresses);
    if (nDebit > 0) // debit>0 means we signed/sent this transaction
    {
        CAmount nValueOut = tx->GetValueOut();
        nFee = nDebit - nValueOut;
    }
    uint64_t ntokenDebit = pwallet->GetDebitOfTokenForUnion(*tx,filter,unionaddresses);
    int nindex = -2;
    bool isFindtxChange = false;
    if (pwallet->GetChangeIndexTokenUnion(*tx, nindex))
    {

        LogPrintf("GetChangeIndexTokenUnion %d\n",nindex);
        isFindtxChange = true;
    }
    const CTransaction& txCT = *tx;
    std::string txid =  txCT.GetHash().ToString();
    LogPrintf("txid %s\n",txid);
    uint8_t txtp = tx->GetTxType(); //Get the type of transaction
    //The change in ipchain trading last Vout
    // Sent/received.
    unsigned int nSize = tx->vout.size();
    for (unsigned int i = 0; i < nSize; ++i)
    {
        const CTxOut& txout = tx->vout[i];
        uint8_t txtype = txout.txType;
        if (txtype != txtp)
            continue;
		if (txtype != TXOUT_TOKENREG && txtype != TXOUT_TOKEN && txtype != TXOUT_ADDTOKEN)
            continue;
        isminetype fIsMine = pwallet->IsMine(txout);
        // Only need to handle txouts if AT LEAST one of these is true:
        //   1) they debit from us (sent)
        //   2) the output is to us (received)
        if (nDebit > 0)
        {
            // Don't report 'change' txouts
            if (pwallet->IsChange(txout))
                continue;

        }
        // In either case, we need to get the destination address
        CTxDestination address;
        if (!ExtractDestination(txout.scriptPubKey, address) && !txout.scriptPubKey.IsUnspendable())
        {
            LogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
                this->GetHash().ToString());
            address = CNoDestination();
            continue;
        }
        bool findunionaddress = false;

        std::string strsymbol = "";
        uint8_t  accuracy = -1;
        uint64_t tokenvalue = 0;
        if (txtype == TXOUT_TOKENREG)
        {
            strsymbol = txout.tokenRegLabel.getTokenSymbol();
            accuracy = txout.tokenRegLabel.accuracy;
            tokenvalue = txout.tokenRegLabel.totalCount;
        }
        else if (txtype == TXOUT_TOKEN){
            strsymbol = txout.tokenLabel.getTokenSymbol();
            accuracy = txout.tokenLabel.accuracy;
            tokenvalue = txout.tokenLabel.value;
        }
		else if (txtype == TXOUT_ADDTOKEN)
		{
			strsymbol = txout.addTokenLabel.getTokenSymbol();
			accuracy = txout.addTokenLabel.accuracy;
			tokenvalue = txout.addTokenLabel.currentCount;
		}
        if (strsymbol != strtokensymbol)
            continue;

		if (isFindtxChange && nindex < i && (txtype == TXOUT_TOKENREG || txtype == TXOUT_TOKEN || txtype == TXOUT_ADDTOKEN))
            continue;

        COutputEntryToken output = { address, txout.nValue, (int)i, txtype, strsymbol ,accuracy,tokenvalue};
        // If we are debited by the transaction, add the output as a "sent" entry
        if (ntokenDebit > 0)
            listSent.push_back(output);
        BOOST_FOREACH(UnionAddressInfo& info,unionaddresses){
                if(info.address == CBitcoinAddress(address).ToString()){
                    findunionaddress = true;
                }
            }
        if(findunionaddress)
        listReceived.push_back(output);
    }

}

void CWalletTx::GetAmounts(list<COutputEntry>& listReceived,
	list<COutputEntry>& listSent, CAmount& nFee, string& strSentAccount, const isminefilter& filter) const
{
	nFee = 0;
	listReceived.clear();
	listSent.clear();
	strSentAccount = strFromAccount;

	// Compute fee:
	CAmount nDebit = GetDebit(filter);
	if (nDebit > 0) // debit>0 means we signed/sent this transaction
	{
		CAmount nValueOut = tx->GetValueOut();
		nFee = nDebit - nValueOut;
	}

	// Sent/received.
	for (unsigned int i = 0; i < tx->vout.size(); ++i)
	{
		const CTxOut& txout = tx->vout[i];
		if (txout.nValue<=0)
			continue;
		isminetype fIsMine = pwallet->IsMine(txout);
		// Only need to handle txouts if AT LEAST one of these is true:
		//   1) they debit from us (sent)
		//   2) the output is to us (received)
		if (nDebit > 0)
		{
			// Don't report 'change' txouts
			if (pwallet->IsChange(txout))
				continue;
		}
		else if (!(fIsMine & filter))
			continue;

		// In either case, we need to get the destination address
		CTxDestination address;

		if (!ExtractDestination(txout.scriptPubKey, address) && !txout.scriptPubKey.IsUnspendable())
		{
			LogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
				this->GetHash().ToString());
			address = CNoDestination();
		}

		COutputEntry output = { address, txout.nValue, (int)i };

		// If we are debited by the transaction, add the output as a "sent" entry
		if (nDebit > 0)
			listSent.push_back(output);

		// If we are receiving the output, add it as a "received" entry
		if ((fIsMine & filter))
			listReceived.push_back(output);
	}

}

void CWalletTx::GetAmountsIPForGetitx(list<COutputEntryIP>& listReceived,
	list<COutputEntryIP>& listSent, CAmount& nFee, string& strSentAccount, const isminefilter& filter) const
{
	nFee = 0;
	listReceived.clear();
	listSent.clear();
	strSentAccount = strFromAccount;

	// Compute fee:
	CAmount nDebit = GetDebit(filter);
	if (nDebit > 0) // debit>0 means we signed/sent this transaction
	{
		CAmount nValueOut = tx->GetValueOut();
		nFee = nDebit - nValueOut;
	}
	uint8_t nIpDebit = pwallet->GetDebitOfIp(*tx, filter);

	uint8_t txtp = tx->GetTxType(); //Get the type of transaction
	//The change in ipchain trading last Vout
	// Sent/received.
	unsigned int nSize = tx->vout.size();
	for (unsigned int i = 0; i < nSize; ++i)
	{
		const CTxOut& txout = tx->vout[i];
		uint8_t txtype = txout.txType;
		uint8_t ipExtendType = txout.ipcLabel.ExtendType;

		if (txtype != TXOUT_IPCOWNER && txtype != TXOUT_IPCAUTHORIZATION)
			continue;
		isminetype fIsMine = pwallet->IsMine(txout);
		// Only need to handle txouts if AT LEAST one of these is true:
		//   1) they debit from us (sent)
		//   2) the output is to us (received)
		if (nDebit > 0)
		{
			// Don't report 'change' txouts
			if (pwallet->IsChange(txout))
				continue;
		}
		else if (!(fIsMine & filter))
			continue;
		// In either case, we need to get the destination address
		CTxDestination address;

		if (!ExtractDestination(txout.scriptPubKey, address) && !txout.scriptPubKey.IsUnspendable())
		{
			LogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
				this->GetHash().ToString());
			address = CNoDestination();
		}
		uint8_t iptype = ipExtendType;
		std::string iptitle = txout.ipcLabel.labelTitle;
		std::string iphash = txout.ipcLabel.hash.GetHex();
		COutputEntryIP output = { address, txout.nValue, (int)i, txtype, iptype, iptitle, iphash };
		// If we are debited by the transaction, add the output as a "sent" entry
		if (nIpDebit > 0 )
			listSent.push_back(output);

		// If we are receiving the output, add it as a "received" entry
		if ((fIsMine & filter))
			listReceived.push_back(output);

	}

}



void CWalletTx::GetAmountsTokenForGetttx(list<COutputEntryToken>& listReceived,
	list<COutputEntryToken>& listSent, CAmount& nFee, string& strSentAccount, const isminefilter& filter) const
{
	nFee = 0;
	listReceived.clear();
	listSent.clear();
	strSentAccount = strFromAccount;

	// Compute fee:
	CAmount nDebit = GetDebit(filter);
	if (nDebit > 0) // debit>0 means we signed/sent this transaction
	{
		CAmount nValueOut = tx->GetValueOut();
		nFee = nDebit - nValueOut;
	}
	uint64_t ntokenDebit = pwallet->GetDebitOfToken(*tx, filter);
	// Sent/received.
	for (unsigned int i = 0; i < tx->vout.size(); ++i)
	{
		const CTxOut& txout = tx->vout[i];
		uint8_t txtype = txout.txType;
		if (txtype != TXOUT_TOKENREG && txtype != TXOUT_TOKEN && txtype != TXOUT_ADDTOKEN)
			continue;
		isminetype fIsMine = pwallet->IsMine(txout);
		// Only need to handle txouts if AT LEAST one of these is true:
		//   1) they debit from us (sent)
		//   2) the output is to us (received)
		if (nDebit > 0)
		{
			// Don't report 'change' txouts
			if (pwallet->IsChange(txout))
				continue;
		}
		else if (!(fIsMine & filter))
			continue;

		// In either case, we need to get the destination address
		CTxDestination address;

		if (!ExtractDestination(txout.scriptPubKey, address) && !txout.scriptPubKey.IsUnspendable())
		{
			LogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
				this->GetHash().ToString());
			address = CNoDestination();
		}

		std::string strsymbol = "";
		uint8_t  accuracy = -1;
		uint64_t tokenvalue = 0;
		if (txtype == TXOUT_TOKENREG)
		{
			strsymbol = txout.tokenRegLabel.getTokenSymbol();
			accuracy = txout.tokenRegLabel.accuracy;
			tokenvalue = txout.tokenRegLabel.totalCount;
		}
		else if (txtype == TXOUT_TOKEN){
			strsymbol = txout.tokenLabel.getTokenSymbol();
			accuracy = txout.tokenLabel.accuracy;
			tokenvalue = txout.tokenLabel.value;
		}
		else if (txtype == TXOUT_ADDTOKEN)
		{
			strsymbol = txout.addTokenLabel.getTokenSymbol();
			accuracy = txout.addTokenLabel.accuracy;
			tokenvalue = txout.addTokenLabel.currentCount;
		}
		
		COutputEntryToken output = { address, txout.nValue, (int)i, txtype, strsymbol, accuracy, tokenvalue };

		// If we are debited by the transaction, add the output as a "sent" entry
		if (ntokenDebit > 0)
			listSent.push_back(output);

		// If we are receiving the output, add it as a "received" entry
		if ((fIsMine & filter))
			listReceived.push_back(output);
	}

}

void CWalletTx::GetAccountAmounts(const string& strAccount, CAmount& nReceived,
                                  CAmount& nSent, CAmount& nFee, const isminefilter& filter) const
{
    nReceived = nSent = nFee = 0;

    CAmount allFee;
    string strSentAccount;
    list<COutputEntry> listReceived;
    list<COutputEntry> listSent;
    GetAmounts(listReceived, listSent, allFee, strSentAccount, filter);

    if (strAccount == strSentAccount)
    {
        BOOST_FOREACH(const COutputEntry& s, listSent)
            nSent += s.amount;
        nFee = allFee;
    }
    {
        LOCK(pwallet->cs_wallet);
        BOOST_FOREACH(const COutputEntry& r, listReceived)
        {
            if (pwallet->mapAddressBook.count(r.destination))
            {
                map<CTxDestination, CAddressBookData>::const_iterator mi = pwallet->mapAddressBook.find(r.destination);
                if (mi != pwallet->mapAddressBook.end() && (*mi).second.name == strAccount)
                    nReceived += r.amount;
            }
            else if (strAccount.empty())
            {
                nReceived += r.amount;
            }
        }
    }
}

/**
 * Scan the block chain (starting in pindexStart) for transactions
 * from or to us. If fUpdate is true, found transactions that already
 * exist in the wallet will be updated.
 *
 * Returns pointer to the first block in the last contiguous range that was
 * successfully scanned.
 *
 */
CBlockIndex* CWallet::ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate,bool bIsP2SH)
{
    LogPrintf("ScanForWalletTransactions begin\n.");
    CBlockIndex* ret = nullptr;
    int64_t nNow = GetTime();
    const CChainParams& chainParams = Params();

    CBlockIndex* pindex = pindexStart;
    {
        if(bIsP2SH){
            // no need to read and scan block, if block was created before
            // our wallet birthday (as adjusted for block time variability)
            while (pindex && nTimeFirstKey && (pindex->GetBlockTime() < (nTimeFirstKey - 7200)))
                pindex = chainActive.Next(pindex);

            ShowProgress(_("Rescanning..."), 0); // show rescan progress in GUI as dialog or on splashscreen, if -rescan on startup
            double dProgressStart = GuessVerificationProgress(chainParams.TxData(), pindex);
            double dProgressTip = GuessVerificationProgress(chainParams.TxData(), chainActive.Tip());
            while (pindex)
            {
                if (pindex->nHeight % 100 == 0 && dProgressTip - dProgressStart > 0.0){
                        ShowProgress(_("Rescanning..."), std::max(1, std::min(99, (int)((GuessVerificationProgress(chainParams.TxData(), pindex) - dProgressStart) / (dProgressTip - dProgressStart) * 100))));
                }
                CBlock block;
                if (ReadBlockFromDisk(block, pindex, Params().GetConsensus())) {
                    for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
                        AddToWalletIfInvolvingMe(*block.vtx[posInBlock], pindex, posInBlock, fUpdate);
                    }
                    if (!ret) {
                        ret = pindex;
                    }
                } else {
                    ret = nullptr;
                }
                pindex = chainActive.Next(pindex);
                    if (GetTime() >= nNow + 60) {
                        nNow = GetTime();
                        LogPrintf("Still rescanning. At block %d. Progress=%f\n", pindex->nHeight, GuessVerificationProgress(chainParams.TxData(), pindex));
                    }

            }
            ShowProgress(_("Rescanning..."), 100); // hide progress dialog in GUI

        }else{
            LOCK2(cs_main, cs_wallet);

            // no need to read and scan block, if block was created before
            // our wallet birthday (as adjusted for block time variability)
            while (pindex && nTimeFirstKey && (pindex->GetBlockTime() < (nTimeFirstKey - 7200)))
                pindex = chainActive.Next(pindex);

            ShowProgress(_("Rescanning..."), 0); // show rescan progress in GUI as dialog or on splashscreen, if -rescan on startup
            double dProgressStart = GuessVerificationProgress(chainParams.TxData(), pindex);
            double dProgressTip = GuessVerificationProgress(chainParams.TxData(), chainActive.Tip());
            while (pindex)
            {
                if (pindex->nHeight % 100 == 0 && dProgressTip - dProgressStart > 0.0){
                        ShowProgress(_("Rescanning..."), std::max(1, std::min(99, (int)((GuessVerificationProgress(chainParams.TxData(), pindex) - dProgressStart) / (dProgressTip - dProgressStart) * 100))));
                }
                CBlock block;
                if (ReadBlockFromDisk(block, pindex, Params().GetConsensus())) {
                    for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
                        AddToWalletIfInvolvingMe(*block.vtx[posInBlock], pindex, posInBlock, fUpdate);
                    }
                    if (!ret) {
                        ret = pindex;
                    }
                } else {
                    ret = nullptr;
                }
                pindex = chainActive.Next(pindex);
                    if (GetTime() >= nNow + 60) {
                        nNow = GetTime();
                        LogPrintf("Still rescanning. At block %d. Progress=%f\n", pindex->nHeight, GuessVerificationProgress(chainParams.TxData(), pindex));
                    }
            }
            ShowProgress(_("Rescanning..."), 100); // hide progress dialog in GUI
        }

    }
    LogPrintf("ScanForWalletTransactions end. \n");
    return ret;
}

void CWallet::ReacceptWalletTransactions()
{
    // If transactions aren't being broadcasted, don't let them into local mempool either
    if (!fBroadcastTransactions)
        return;
    LOCK2(cs_main, cs_wallet);
    std::map<int64_t, CWalletTx*> mapSorted;

    // Sort pending wallet transactions based on their initial wallet insertion order
    BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
    {
        const uint256& wtxid = item.first;
        CWalletTx& wtx = item.second;
        assert(wtx.GetHash() == wtxid);

        int nDepth = wtx.GetDepthInMainChain();

        if (!wtx.IsCoinBase() && (nDepth == 0 && !wtx.isAbandoned())) {
            mapSorted.insert(std::make_pair(wtx.nOrderPos, &wtx));
        }
    }
	LogPrintf("mapSorted.size() = %d \n",mapSorted.size());
    // Try to add wallet transactions to memory pool
    BOOST_FOREACH(PAIRTYPE(const int64_t, CWalletTx*)& item, mapSorted)
    {
        CWalletTx& wtx = *(item.second);

        LOCK(mempool.cs);
        CValidationState state;
        wtx.AcceptToMemoryPool(maxTxFee, state);
    }
	LogPrintf("Done[mapSorted] \n");
}

bool CWalletTx::RelayWalletTransaction(CConnman* connman)
{
    //assert(pwallet->GetBroadcastTransactions());
    if(!pwallet->GetBroadcastTransactions()){
        LogPrintf("pwallet->GetBroadcastTransactions() false \n");
        std::cout<<"pwallet->GetBroadcastTransactions() false"<<std::endl;
    }
    if (!IsCoinBase() && !isAbandoned() && GetDepthInMainChain() == 0)
    {
        CValidationState state;
        /* GetDepthInMainChain already catches known conflicts. */
        if (InMempool() || AcceptToMemoryPool(maxTxFee, state)) {
            LogPrintf("Relaying wtx %s\n", GetHash().ToString());
            if (connman) {
                CInv inv(MSG_TX, GetHash());
                connman->ForEachNode([&inv](CNode* pnode)
                {
                    pnode->PushInventory(inv);
                });
                return true;
            }
        }
    }
    return false;
}

set<uint256> CWalletTx::GetConflicts() const
{
    set<uint256> result;
    if (pwallet != NULL)
    {
        uint256 myHash = GetHash();
        result = pwallet->GetConflicts(myHash);
        result.erase(myHash);
    }
    return result;
}

CAmount CWalletTx::GetDebit(const isminefilter& filter) const
{
    if (tx->vin.empty())
        return 0;

    CAmount debit = 0;
    if(filter & ISMINE_SPENDABLE)
    {
        if (fDebitCached)
            debit += nDebitCached;
        else
        {
            nDebitCached = pwallet->GetDebit(*this, ISMINE_SPENDABLE);
            fDebitCached = true;
            debit += nDebitCached;
        }
    }
    if(filter & ISMINE_WATCH_ONLY)
    {
        if(fWatchDebitCached)
            debit += nWatchDebitCached;
        else
        {
            nWatchDebitCached = pwallet->GetDebit(*this, ISMINE_WATCH_ONLY);
            fWatchDebitCached = true;
            debit += nWatchDebitCached;
        }
    }
    return debit;
}

CAmount CWalletTx::GetDebitForUnion(const isminefilter& filter,std::vector<UnionAddressInfo>&unionaddresses) const
{
    if (tx->vin.empty())
        return 0;

    CAmount debit = 0;
    if(filter & ISMINE_SPENDABLE)
    {
        if (fDebitCached)
            debit += nDebitCached;
        else
        {
            nDebitCached = pwallet->GetDebit(*this, ISMINE_SPENDABLE);
            fDebitCached = true;
            debit += nDebitCached;
        }
    }
    if(filter & ISMINE_WATCH_ONLY)
    {
        if(fWatchDebitCached)
            debit += nWatchDebitCached;
        else
        {
            nWatchDebitCached = pwallet->GetDebit(*this, ISMINE_WATCH_ONLY);
            fWatchDebitCached = true;
            debit += nWatchDebitCached;
        }
    }
    return debit;
}
CAmount CWalletTx::GetCredit(const isminefilter& filter) const
{
    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (IsCoinBase() && GetBlocksToMaturity() > 0)
        return 0;

    CAmount credit = 0;
    if (filter & ISMINE_SPENDABLE)
    {
        // GetBalance can assume transactions in mapWallet won't change
        if (fCreditCached)
            credit += nCreditCached;
        else
        {
            nCreditCached = pwallet->GetCredit(*this, ISMINE_SPENDABLE);
            fCreditCached = true;
            credit += nCreditCached;
        }
    }
    if (filter & ISMINE_WATCH_ONLY)
    {
        if (fWatchCreditCached)
            credit += nWatchCreditCached;
        else
        {
            nWatchCreditCached = pwallet->GetCredit(*this, ISMINE_WATCH_ONLY);
            fWatchCreditCached = true;
            credit += nWatchCreditCached;
        }
    }
    return credit;
}

CAmount CWalletTx::GetImmatureCredit(bool fUseCache) const
{
    if (IsCoinBase() && GetBlocksToMaturity() > 0 && IsInMainChain())
    {
        if (fUseCache && fImmatureCreditCached)
            return nImmatureCreditCached;
        nImmatureCreditCached = pwallet->GetCredit(*this, ISMINE_SPENDABLE);
        fImmatureCreditCached = true;
        return nImmatureCreditCached;
    }

    return 0;
}

CAmount CWalletTx::GetAvailableCredit(bool fUseCache) const
{
    if (pwallet == 0)
        return 0;
	bool reAvailableCreditCached = false;
    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (IsCoinBase() && GetBlocksToMaturity() > 0)
        return 0;

    if (fUseCache && fAvailableCreditCached )
        return nAvailableCreditCached;

    CAmount nCredit = 0;
    uint256 hashTx = GetHash();
    for (unsigned int i = 0; i < tx->vout.size(); i++)
    {
        if (!pwallet->IsSpent(hashTx, i))
        {
	             const CTxOut &txout = tx->vout[i];
			//Remove the COINS that have been locked in. add by xxy 20171030
			if (txout.txType == TXOUT_CAMPAIGN &&
				txout.devoteLabel.ExtendType == TYPE_CONSENSUS_REGISTER)
			{
				reAvailableCreditCached = true; //The utxo that applies to join the type in tx will need to be re-checked next time, and the state will change!
				if (!CConsensusAccountPool::Instance().IsAviableUTXO(hashTx))
				{
						continue;
				}
				
			}
			//end 
            nCredit += pwallet->GetCredit(txout, ISMINE_SPENDABLE);
            if (!MoneyRange(nCredit))
                throw std::runtime_error("CWalletTx::GetAvailableCredit() : value out of range");
        }
    }

    nAvailableCreditCached = nCredit;
    fAvailableCreditCached = true;
	//add by xxy
	if (reAvailableCreditCached)
		fAvailableCreditCached = false;
	//end
    return nCredit;
}

//add by xxy 20171219
CAmount CWalletTx::GetLockCredit(bool fUseCache) const
{
	if (pwallet == 0)
		return 0;
	// Must wait until coinbase is safely deep enough in the chain before valuing it
	if (IsCoinBase() && GetBlocksToMaturity() > 0)
		return 0;

	if (fUseCache && fAvailableCreditCached)
		return 0; 

	CAmount nCredit = 0;
	uint256 hashTx = GetHash();
	for (unsigned int i = 0; i < tx->vout.size(); i++)
	{
		if (!pwallet->IsSpent(hashTx, i))
		{
			const CTxOut &txout = tx->vout[i];
			if (txout.txType == TXOUT_CAMPAIGN &&
				txout.devoteLabel.ExtendType == TYPE_CONSENSUS_REGISTER)
			{
				if (!CConsensusAccountPool::Instance().IsAviableUTXO(hashTx))
				{
					nCredit += pwallet->GetCredit(txout, ISMINE_SPENDABLE);
					if (!MoneyRange(nCredit))
						throw std::runtime_error("CWalletTx::GetAvailableCredit() : value out of range");
					continue;
				}
				
			}
			//end 
			
			
		}
	}
	fAvailableCreditCached = false;

	return nCredit;
}
//end 
CAmount CWalletTx::GetImmatureWatchOnlyCredit(const bool& fUseCache) const
{
    if (IsCoinBase() && GetBlocksToMaturity() > 0 && IsInMainChain())
    {
        if (fUseCache && fImmatureWatchCreditCached)
            return nImmatureWatchCreditCached;
        nImmatureWatchCreditCached = pwallet->GetCredit(*this, ISMINE_WATCH_ONLY);
        fImmatureWatchCreditCached = true;
        return nImmatureWatchCreditCached;
    }

    return 0;
}

CAmount CWalletTx::GetAvailableWatchOnlyCredit(const bool& fUseCache) const
{
    if (pwallet == 0)
        return 0;

    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (IsCoinBase() && GetBlocksToMaturity() > 0)
        return 0;

    if (fUseCache && fAvailableWatchCreditCached)
        return nAvailableWatchCreditCached;

    CAmount nCredit = 0;
    for (unsigned int i = 0; i < tx->vout.size(); i++)
    {
        if (!pwallet->IsSpent(GetHash(), i))
        {
            const CTxOut &txout = tx->vout[i];
            nCredit += pwallet->GetCredit(txout, ISMINE_WATCH_ONLY);
            if (!MoneyRange(nCredit))
                throw std::runtime_error("CWalletTx::GetAvailableCredit() : value out of range");
        }
    }

    nAvailableWatchCreditCached = nCredit;
    fAvailableWatchCreditCached = true;
    return nCredit;
}

CAmount CWalletTx::GetChange() const
{
    if (fChangeCached)
        return nChangeCached;
    nChangeCached = pwallet->GetChange(*this);
    fChangeCached = true;
    return nChangeCached;
}

bool CWalletTx::InMempool() const
{
    LOCK(mempool.cs);
    if (mempool.exists(GetHash())) {
        return true;
    }
    return false;
}

bool CWalletTx::IsTrusted() const
{

    int nDepth = GetDepthInMainChain();
	if (nDepth >= 1)
       return true;
    if (nDepth < 0)
        return false;
    if (!bSpendZeroConfChange || !IsFromMe(ISMINE_ALL)) // using wtx's cached debit
        return false;

    // Don't trust unconfirmed transactions from us unless they are in the mempool.
    if (!InMempool())
        return false;

    // Trusted if all inputs are from us and are in the mempool:
    BOOST_FOREACH(const CTxIn& txin, tx->vin)
    {
        // Transactions not sent by us: not trusted
        const CWalletTx* parent = pwallet->GetWalletTx(txin.prevout.hash);
        if (parent == NULL)
            return false;
        const CTxOut& parentOut = parent->tx->vout[txin.prevout.n];
        if (pwallet->IsMine(parentOut) != ISMINE_SPENDABLE)
            return false;
    }
    return true;
}

bool CWalletTx::IsEquivalentTo(const CWalletTx& _tx) const
{
        CMutableTransaction tx1 = *this->tx;
        CMutableTransaction tx2 = *_tx.tx;
        for (unsigned int i = 0; i < tx1.vin.size(); i++) tx1.vin[i].scriptSig = CScript();
        for (unsigned int i = 0; i < tx2.vin.size(); i++) tx2.vin[i].scriptSig = CScript();
        return CTransaction(tx1) == CTransaction(tx2);
}


std::vector<uint256> CWallet::ResendWalletTransactionsBefore(int64_t nTime, CConnman* connman)
{
    std::vector<uint256> result;

    LOCK(cs_wallet);
    // Sort them in chronological order
    multimap<unsigned int, CWalletTx*> mapSorted;
    BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
    {
        CWalletTx& wtx = item.second;
        // Don't rebroadcast if newer than nTime:
        if (wtx.nTimeReceived > nTime)
            continue;
        mapSorted.insert(make_pair(wtx.nTimeReceived, &wtx));
    }
    BOOST_FOREACH(PAIRTYPE(const unsigned int, CWalletTx*)& item, mapSorted)
    {
        CWalletTx& wtx = *item.second;
        if (wtx.RelayWalletTransaction(connman))
            result.push_back(wtx.GetHash());
    }
    return result;
}

void CWallet::ResendWalletTransactions(int64_t nBestBlockTime, CConnman* connman)
{
    // Do this infrequently and randomly to avoid giving away
    // that these are our transactions.
    if (GetTime() < nNextResend || !fBroadcastTransactions)
        return;
    bool fFirst = (nNextResend == 0);
    nNextResend = GetTime() + GetRand(30 * 60);
    if (fFirst)
        return;

    // Only do it if there's been a new block since last time
    if (nBestBlockTime < nLastResend)
        return;
    nLastResend = GetTime();

    // Rebroadcast unconfirmed txes older than 5 minutes before the last
    // block was found:
    std::vector<uint256> relayed = ResendWalletTransactionsBefore(nBestBlockTime-5*60, connman);
    if (!relayed.empty())
        LogPrintf("%s: rebroadcast %u unconfirmed transactions\n", __func__, relayed.size());
}








CAmount CWallet::GetDeposit() const
{
	CAmount nTotal = 0;
	std::string strPublicKey;
	uint160 pkhash;

	{
		LOCK2(cs_main, cs_wallet);
		for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
		{
			const CWalletTx* pcoin = &(*it).second;
			if (pcoin->IsTrusted())
				nTotal += pcoin->GetLockCredit();

		}
	}

	return nTotal;
}
int   CWallet::GetDepthofJoinTX()
{
	int  depth = 0;
	std::string strPublicKey;
	uint256 hash;
	uint160 pkhash;
	if (CDpocInfo::Instance().GetLocalAccount(strPublicKey))
	{
		pkhash.SetHex(strPublicKey);
		hash = CConsensusAccountPool::Instance().GetTXhashBypkhash(pkhash);
		if (hash.IsNull())
			return depth;
		if (mapWallet.count(hash)>0)
		{
			const CWalletTx* pcoin = &mapWallet[hash];
			return pcoin->GetDepthInMainChain();
		}
		
	}
	return depth;

}
CAmount CWallet::GetBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
			if (pcoin->GetDepthInMainChain() >= nTxConfirmTarget && pcoin->IsTrusted())
				nTotal += pcoin->GetAvailableCredit();
               
        }
    }

    return nTotal;
}
CAmount CWallet::GetUnconfirmedBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
       //     if (!pcoin->IsTrusted() && pcoin->GetDepthInMainChain() == 0 && pcoin->InMempool())
			if (pcoin->GetDepthInMainChain() < nTxConfirmTarget || (!pcoin->IsTrusted() && pcoin->GetDepthInMainChain() == 0 && pcoin->InMempool()))
                nTotal += pcoin->GetAvailableCredit();
        }
    }
    return nTotal;
}

CAmount CWallet::GetImmatureBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            nTotal += pcoin->GetImmatureCredit();
        }
    }
    return nTotal;
}

CAmount CWallet::GetWatchOnlyBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsTrusted())
                nTotal += pcoin->GetAvailableWatchOnlyCredit();
        }
    }

    return nTotal;
}

CAmount CWallet::GetUnconfirmedWatchOnlyBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (!pcoin->IsTrusted() && pcoin->GetDepthInMainChain() == 0 && pcoin->InMempool())
                nTotal += pcoin->GetAvailableWatchOnlyCredit();
        }
    }
    return nTotal;
}

CAmount CWallet::GetImmatureWatchOnlyBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            nTotal += pcoin->GetImmatureWatchOnlyCredit();
        }
    }
    return nTotal;
}

void CWallet::AvailableCoins(vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue) const
{
    vCoins.clear();

    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const uint256& wtxid = it->first;
            const CWalletTx* pcoin = &(*it).second;

//             if (!CheckIPCFinalTx(*pcoin))
//                 continue;

            if (fOnlyConfirmed && !pcoin->IsTrusted())
                continue;

            if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < 0)
                continue;

            // We should not consider coins which aren't at least in our mempool
            // It's possible for these to be conflicted via ancestors which we may never be able to detect
            if (nDepth == 0 && !pcoin->InMempool())
                continue;

            // We should not consider coins from transactions that are replacing
            // other transactions.
            //
            // Example: There is a transaction A which is replaced by bumpfee
            // transaction B. In this case, we want to prevent creation of
            // a transaction B' which spends an output of B.
            //
            // Reason: If transaction A were initially confirmed, transactions B
            // and B' would no longer be valid, so the user would have to create
            // a new transaction C to replace B'. However, in the case of a
            // one-block reorg, transactions B' and C might BOTH be accepted,
            // when the user only wanted one of them. Specifically, there could
            // be a 1-block reorg away from the chain where transactions A and C
            // were accepted to another chain where B, B', and C were all
            // accepted.
            if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaces_txid")) {
                continue;
            }

            // Similarly, we should not consider coins from transactions that
            // have been replaced. In the example above, we would want to prevent
            // creation of a transaction A' spending an output of A, because if
            // transaction B were initially confirmed, conflicting with A and
            // A', we wouldn't want to the user to create a transaction D
            // intending to replace A', but potentially resulting in a scenario
            // where A, A', and D could all be accepted (instead of just B and
            // D, or just A and A' like the user would want).
            if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaced_by_txid")) {
                continue;
            }

            for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++) {
                isminetype mine = IsMine(pcoin->tx->vout[i]);
                if (!(IsSpent(wtxid, i)) && mine != ISMINE_NO &&
                    !IsLockedCoin((*it).first, i) && (pcoin->tx->vout[i].nValue > 0 || fIncludeZeroValue) &&
                    (!coinControl || !coinControl->HasSelected() || coinControl->fAllowOtherInputs || coinControl->IsSelected(COutPoint((*it).first, i))))
                        vCoins.push_back(COutput(pcoin, i, nDepth,
                                                 ((mine & ISMINE_SPENDABLE) != ISMINE_NO) ||
                                                  (coinControl && coinControl->fAllowWatchOnly && (mine & ISMINE_WATCH_SOLVABLE) != ISMINE_NO),
                                                 (mine & (ISMINE_SPENDABLE | ISMINE_WATCH_SOLVABLE)) != ISMINE_NO));
            }
        }
    }
}
//add by xxy
void CWallet::AvailableNormalCoins(vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue) const
{
	vCoins.clear();
	{
		LOCK2(cs_main, cs_wallet);
		for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
		{
			const uint256& wtxid = it->first;
			const CWalletTx* pcoin = &(*it).second;


			if (fOnlyConfirmed && !pcoin->IsTrusted())
				continue;

			if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
				continue;

			int nDepth = pcoin->GetDepthInMainChain();

			if (nDepth < nTxConfirmTarget)
				continue;
			// We should not consider coins which aren't at least in our mempool
			// It's possible for these to be conflicted via ancestors which we may never be able to detect
			if (nDepth == 0 && !pcoin->InMempool())
				continue;

			// We should not consider coins from transactions that are replacing
			// other transactions.
			//
			// Example: There is a transaction A which is replaced by bumpfee
			// transaction B. In this case, we want to prevent creation of
			// a transaction B' which spends an output of B.
			//
			// Reason: If transaction A were initially confirmed, transactions B
			// and B' would no longer be valid, so the user would have to create
			// a new transaction C to replace B'. However, in the case of a
			// one-block reorg, transactions B' and C might BOTH be accepted,
			// when the user only wanted one of them. Specifically, there could
			// be a 1-block reorg away from the chain where transactions A and C
			// were accepted to another chain where B, B', and C were all
			// accepted.
			if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaces_txid")) {
				continue;
			}

			// Similarly, we should not consider coins from transactions that
			// have been replaced. In the example above, we would want to prevent
			// creation of a transaction A' spending an output of A, because if
			// transaction B were initially confirmed, conflicting with A and
			// A', we wouldn't want to the user to create a transaction D
			// intending to replace A', but potentially resulting in a scenario
			// where A, A', and D could all be accepted (instead of just B and
			// D, or just A and A' like the user would want).
			if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaced_by_txid")) {
				continue;
			}

			for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++) {
				isminetype mine = IsMine(pcoin->tx->vout[i]);
				if (!(IsSpent(wtxid, i)) && mine != ISMINE_NO &&
					!IsLockedCoin((*it).first, i) && (pcoin->tx->vout[i].nValue > 0 || fIncludeZeroValue) &&
					(!coinControl || !coinControl->HasSelected() || coinControl->fAllowOtherInputs || coinControl->IsSelected(COutPoint((*it).first, i)))&&
					(pcoin->tx->vout[i].txType == TXOUT_NORMAL || (pcoin->tx->vout[i].txType == TXOUT_CAMPAIGN && pcoin->tx->vout[i].devoteLabel.ExtendType == TYPE_CONSENSUS_REGISTER && 
					CConsensusAccountPool::Instance().IsAviableUTXO(pcoin->tx->GetHash())))) 
					vCoins.push_back(COutput(pcoin, i, nDepth,
					((mine & ISMINE_SPENDABLE) != ISMINE_NO) ||
					(coinControl && coinControl->fAllowWatchOnly && (mine & ISMINE_WATCH_SOLVABLE) != ISMINE_NO),
					(mine & (ISMINE_SPENDABLE | ISMINE_WATCH_SOLVABLE)) != ISMINE_NO));
			}
		}
	}
	
}
void CWallet::AvailableIPCCoins(vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue) const
{
	vCoins.clear();

	{
		LOCK2(cs_main, cs_wallet);
		for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
		{
			const uint256& wtxid = it->first;
			
			const CWalletTx* pcoin = &(*it).second;


			if (fOnlyConfirmed && !pcoin->IsTrusted())
				continue;

			if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
				continue;

			int nDepth = pcoin->GetDepthInMainChain();
			if (nDepth < 0)
				continue;

			// We should not consider coins which aren't at least in our mempool
			// It's possible for these to be conflicted via ancestors which we may never be able to detect
			if (nDepth == 0 && !pcoin->InMempool())
				continue;

			// We should not consider coins from transactions that are replacing
			// other transactions.
			//
			// Example: There is a transaction A which is replaced by bumpfee
			// transaction B. In this case, we want to prevent creation of
			// a transaction B' which spends an output of B.
			//
			// Reason: If transaction A were initially confirmed, transactions B
			// and B' would no longer be valid, so the user would have to create
			// a new transaction C to replace B'. However, in the case of a
			// one-block reorg, transactions B' and C might BOTH be accepted,
			// when the user only wanted one of them. Specifically, there could
			// be a 1-block reorg away from the chain where transactions A and C
			// were accepted to another chain where B, B', and C were all
			// accepted.
			if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaces_txid")) {
				continue;
			}

			// Similarly, we should not consider coins from transactions that
			// have been replaced. In the example above, we would want to prevent
			// creation of a transaction A' spending an output of A, because if
			// transaction B were initially confirmed, conflicting with A and
			// A', we wouldn't want to the user to create a transaction D
			// intending to replace A', but potentially resulting in a scenario
			// where A, A', and D could all be accepted (instead of just B and
			// D, or just A and A' like the user would want).
			if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaced_by_txid")) {
				continue;
			}

			for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++) {

				isminetype mine = IsMine(pcoin->tx->vout[i]);
				if (!(IsSpent(wtxid, i)) && mine != ISMINE_NO &&
					!IsLockedCoin((*it).first, i) && (pcoin->tx->vout[i].nValue >= 0 || fIncludeZeroValue) &&
					(!coinControl || !coinControl->HasSelected() || coinControl->fAllowOtherInputs || coinControl->IsSelected(COutPoint((*it).first, i)))&&
					(pcoin->tx->vout[i].txType == TXOUT_IPCOWNER || pcoin->tx->vout[i].txType == TXOUT_IPCAUTHORIZATION))
					vCoins.push_back(COutput(pcoin, i, nDepth,
					((mine & ISMINE_SPENDABLE) != ISMINE_NO) ||
					(coinControl && coinControl->fAllowWatchOnly && (mine & ISMINE_WATCH_SOLVABLE) != ISMINE_NO),
					(mine & (ISMINE_SPENDABLE | ISMINE_WATCH_SOLVABLE)) != ISMINE_NO));
			}
		}
	}
}
void  CWallet::ListTokenBalance(std::map<std::string, uint64_t>& TokenList)
{
	TokenList.clear();
	UpdateTokenBalanceList();
	std::map<std::string, uint64_t>::iterator it;
	for (it = TokenValueMap.begin(); it != TokenValueMap.end();it++)
	{
		TokenList.insert(std::make_pair(it->first,it->second));
	}
	return;
}
uint8_t CWallet::GetAccuracyBySymbol(std::string& tokensymbol)
{
	if (tokenDataMap.count(tokensymbol) == 0)
		return 10;
	return tokenDataMap[tokensymbol].getAccuracy();
}
uint32_t CWallet::GetIssueDateBySymbol(std::string& tokensymbol)
{
	if (tokenDataMap.count(tokensymbol) == 0)
		return 0;
	return tokenDataMap[tokensymbol].getIssueDate();
}
uint128 CWallet::GetHashBySymbol(std::string& tokensymbol)
{
    uint128 temp;
    if (tokenDataMap.count(tokensymbol) == 0)
        return temp;
    return tokenDataMap[tokensymbol].getHash();
}


bool CWallet::GetSymbolbalance(std::string& tokensymbol, uint64_t& value,std::string unionaddress)
{
	value = 0;
	std::vector<COutput> vAvailableTokenCoins;
	const CCoinControl *coinControl = NULL;
    if(unionaddress == "")
        AvailableTokenCoins(vAvailableTokenCoins, true, coinControl);
    else
        AvailableUnionCoinsCOutput(unionaddress,vAvailableTokenCoins, true, coinControl,true,true);
	uint8_t txtype = -1;
	std::string strsymbol = "";
	uint64_t nvalue = 0;
	bool isFind = false;
	for (const auto& coin : vAvailableTokenCoins)
	{
		if (coin.tx->GetDepthInMainChain()<nTxConfirmTarget) //Not enough for eight confirmed filters
			continue;
		const CTxOut& coinvout = coin.tx->tx->vout[coin.i];
		if (unionaddress == "" && !(IsMine(coinvout)&ISMINE_SPENDABLE))
			continue;
		txtype = coin.tx->tx->vout[coin.i].txType;
		
		
		if (txtype == 4)
		{
			strsymbol = coin.tx->tx->vout[coin.i].tokenRegLabel.getTokenSymbol();
			nvalue = coin.tx->tx->vout[coin.i].tokenRegLabel.totalCount;
		}
		else if (txtype == TXOUT_ADDTOKEN)
		{
			strsymbol = coin.tx->tx->vout[coin.i].addTokenLabel.getTokenSymbol();
			nvalue = coin.tx->tx->vout[coin.i].addTokenLabel.totalCount;
		}
		else if (txtype == 5)
		{
			strsymbol = coin.tx->tx->vout[coin.i].tokenLabel.getTokenSymbol();
			nvalue = coin.tx->tx->vout[coin.i].tokenLabel.value;
		}
		if (strsymbol == tokensymbol) //Symbol symbol is consistent
		{
			isFind = true;
			value += nvalue;
			if (txtype == 4)   //There could be only one token registered
				return isFind;
		}
		if (tokensymbol != strsymbol)
			continue;
	}

	return isFind;
}

bool CWallet::IsHaveTheTokensymbol(std::string&tokensymbol)
{
	std::string str = "";
	for (unsigned int i = 0; i < TokensymbolList.size(); i++)
	{
		str = TokensymbolList[i];
		if (tokensymbol == str)
		{
			return true;
		}
	}
	return false;
}

void CWallet::getListTokenBuildByMe(tokenInfoMap& tokeninfo)
{
    LogPrintf("getListTokenBuildByMe begin \n");
    tokeninfo.clear();
    LOCK2(cs_main, cs_wallet);
    for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const uint256& wtxid = it->first;

        const CWalletTx* pcoin = &(*it).second;
        BOOST_FOREACH(const auto& vout,pcoin->tx->vout){
			if ((vout.txType == TXOUT_TOKENREG || vout.txType == TXOUT_ADDTOKEN) && IsMine(vout) && tokeninfo.count(vout.getTokenSymbol()) == 0){//no  TXOUT_ADDTOKEN
                    TokenByMeInfo info;
					uint8_t  accuracy;
					if (vout.txType == TXOUT_TOKENREG)
						accuracy = vout.tokenRegLabel.accuracy;
					else if (vout.txType == TXOUT_ADDTOKEN)
						accuracy = vout.addTokenLabel.accuracy;
                    info.accuracy =   accuracy ;
                    info.confirmation = pcoin->GetDepthInMainChain();
                    info.txid = pcoin->GetHash().ToString();
                    CScript script =  vout.scriptPubKey;
                    txnouttype typeRet;
                    vector<CTxDestination> prevdestes;
                    std::vector<std::string> addresses;
                    int nRequiredRet;
                    bool fValidAddress = ExtractDestinations(script, typeRet, prevdestes, nRequiredRet);
                    if(fValidAddress)
                    {
                        BOOST_FOREACH(CTxDestination &prevdest, prevdestes)
                        {
                            CBitcoinAddress add(prevdest);
                            std::string multaddress = CBitcoinAddress(prevdest).ToString();
                            addresses.push_back(multaddress);
                        }
                    }
                    info.addresses = addresses;
                    tokeninfo.insert(make_pair(vout.getTokenSymbol(),info));
                    LogPrintf("getListTokenBuildByMe tokensymbol: %s \n",vout.getTokenSymbol());
            }

        }
    }
    LogPrintf("getListTokenBuildByMe end \n");
}

void CWallet::UpdateTokenBalanceList( )
{
	
	std::vector<COutput> vAvailableTokenCoins;
	std::string strsymbol = "";
	std::string cursymbol = "";
	uint8_t txtype = -1;
	uint64_t balance = 0;
	uint64_t nvalue = 0;
	const CCoinControl *coinControl = NULL;
	AvailableTokenCoins(vAvailableTokenCoins,true, coinControl);
	for (const auto& coin : vAvailableTokenCoins)
	{
		txtype = coin.tx->tx->vout[coin.i].txType;
		if (txtype == 4)
		{
			strsymbol = coin.tx->tx->vout[coin.i].tokenRegLabel.getTokenSymbol();
			nvalue = coin.tx->tx->vout[coin.i].tokenRegLabel.totalCount;
		}
		else if (txtype == TXOUT_ADDTOKEN)
		{
			strsymbol = coin.tx->tx->vout[coin.i].addTokenLabel.getTokenSymbol();
			nvalue = coin.tx->tx->vout[coin.i].addTokenLabel.currentCount;
		}
		else if (txtype == 5)
		{
			strsymbol = coin.tx->tx->vout[coin.i].tokenLabel.getTokenSymbol();
			nvalue = coin.tx->tx->vout[coin.i].tokenLabel.value;
		}
		if (!IsHaveTheTokensymbol(strsymbol))	//If not, add it
		{ 
			TokensymbolList.push_back(strsymbol);
		}
			
		
	}
	TokenValueMap.clear();
	if (TokensymbolList.size())
	{
		for (unsigned int i = 0; i < TokensymbolList.size(); i++)
		{
			cursymbol = TokensymbolList[i];
			balance = 0; //For a symbol, the balance is 0
			for (const auto& coin : vAvailableTokenCoins)
			{
				txtype = coin.tx->tx->vout[coin.i].txType;
				if (txtype == 4)
				{
					strsymbol = coin.tx->tx->vout[coin.i].tokenRegLabel.getTokenSymbol();
					nvalue = coin.tx->tx->vout[coin.i].tokenRegLabel.totalCount;
				}
				else if (txtype == TXOUT_ADDTOKEN)
				{
					strsymbol = coin.tx->tx->vout[coin.i].addTokenLabel.getTokenSymbol();
					nvalue = coin.tx->tx->vout[coin.i].addTokenLabel.currentCount;
				}
				else if (txtype == 5)
				{
					strsymbol = coin.tx->tx->vout[coin.i].tokenLabel.getTokenSymbol();
					nvalue = coin.tx->tx->vout[coin.i].tokenLabel.value;
				}
				if (cursymbol != strsymbol)
				continue;
				balance += nvalue;
			}
			TokenValueMap.insert(std::make_pair(cursymbol, balance));
		}
	}
	return;
}
void CWallet::AvailableTokenCoins(vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue) const
{
	vCoins.clear();

	{
		LOCK2(cs_main, cs_wallet);
		for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
		{
			const uint256& wtxid = it->first;
			const CWalletTx* pcoin = &(*it).second;


			if (fOnlyConfirmed && !pcoin->IsTrusted())
				continue;

			if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
				continue;

			int nDepth = pcoin->GetDepthInMainChain();
			if (nDepth < 0)
				continue;

			// We should not consider coins which aren't at least in our mempool
			// It's possible for these to be conflicted via ancestors which we may never be able to detect
			if (nDepth == 0 && !pcoin->InMempool())
				continue;

			// We should not consider coins from transactions that are replacing
			// other transactions.
			//
			// Example: There is a transaction A which is replaced by bumpfee
			// transaction B. In this case, we want to prevent creation of
			// a transaction B' which spends an output of B.
			//
			// Reason: If transaction A were initially confirmed, transactions B
			// and B' would no longer be valid, so the user would have to create
			// a new transaction C to replace B'. However, in the case of a
			// one-block reorg, transactions B' and C might BOTH be accepted,
			// when the user only wanted one of them. Specifically, there could
			// be a 1-block reorg away from the chain where transactions A and C
			// were accepted to another chain where B, B', and C were all
			// accepted.
			if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaces_txid")) {
				continue;
			}

			// Similarly, we should not consider coins from transactions that
			// have been replaced. In the example above, we would want to prevent
			// creation of a transaction A' spending an output of A, because if
			// transaction B were initially confirmed, conflicting with A and
			// A', we wouldn't want to the user to create a transaction D
			// intending to replace A', but potentially resulting in a scenario
			// where A, A', and D could all be accepted (instead of just B and
			// D, or just A and A' like the user would want).
			if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaced_by_txid")) {
				continue;
			}

			for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++) {
				isminetype mine = IsMine(pcoin->tx->vout[i]);
				if (!(IsSpent(wtxid, i)) && mine != ISMINE_NO &&
					!IsLockedCoin((*it).first, i) && (pcoin->tx->vout[i].nValue >= 0 || fIncludeZeroValue) &&
					(!coinControl || !coinControl->HasSelected() || coinControl->fAllowOtherInputs || coinControl->IsSelected(COutPoint((*it).first, i)))&&
					(pcoin->tx->vout[i].txType == TXOUT_TOKEN || pcoin->tx->vout[i].txType == TXOUT_TOKENREG || pcoin->tx->vout[i].txType == TXOUT_ADDTOKEN))//
					vCoins.push_back(COutput(pcoin, i, nDepth,
					((mine & ISMINE_SPENDABLE) != ISMINE_NO) ||
					(coinControl && coinControl->fAllowWatchOnly && (mine & ISMINE_WATCH_SOLVABLE) != ISMINE_NO),
					(mine & (ISMINE_SPENDABLE | ISMINE_WATCH_SOLVABLE)) != ISMINE_NO));
			}
		}
	}
}
//end
static void ApproximateBestSubset(vector<pair<CAmount, pair<const CWalletTx*,unsigned int> > >vValue, const CAmount& nTotalLower, const CAmount& nTargetValue,
                                  vector<char>& vfBest, CAmount& nBest, int iterations = 1000)
{
    vector<char> vfIncluded;

    vfBest.assign(vValue.size(), true);
    nBest = nTotalLower;

    FastRandomContext insecure_rand;

    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(vValue.size(), false);
        CAmount nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (unsigned int i = 0; i < vValue.size(); i++)
            {
                //The solver here uses a randomized algorithm,
                //the randomness serves no real security purpose but is just
                //needed to prevent degenerate behavior and it is important
                //that the rng is fast. We do not use a constant random sequence,
                //because there may be some privacy improvement by making
                //the selection random.
                if (nPass == 0 ? insecure_rand.rand32()&1 : !vfIncluded[i])
                {
                    nTotal += vValue[i].first;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= vValue[i].first;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}

bool CWallet::SelectCoinsMinConf(const CAmount& nTargetValue, const int nConfMine, const int nConfTheirs, const uint64_t nMaxAncestors, vector<COutput> vCoins,
                                 set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    // List of values less than target
    pair<CAmount, pair<const CWalletTx*,unsigned int> > coinLowestLarger;
    coinLowestLarger.first = std::numeric_limits<CAmount>::max();
    coinLowestLarger.second.first = NULL;
    vector<pair<CAmount, pair<const CWalletTx*,unsigned int> > > vValue;
    CAmount nTotalLower = 0;

    random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);

    BOOST_FOREACH(const COutput &output, vCoins)
    {
        if (!output.fSpendable)
            continue;

        const CWalletTx *pcoin = output.tx;

        if (output.nDepth < (pcoin->IsFromMe(ISMINE_ALL) ? nConfMine : nConfTheirs))
            continue;

        if (!mempool.TransactionWithinChainLimit(pcoin->GetHash(), nMaxAncestors))
            continue;

        int i = output.i;
        CAmount n = pcoin->tx->vout[i].nValue;

        pair<CAmount,pair<const CWalletTx*,unsigned int> > coin = make_pair(n,make_pair(pcoin, i));

        if (n == nTargetValue)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
            return true;
        }
        else if (n < nTargetValue + MIN_CHANGE)
        {
            vValue.push_back(coin);
            nTotalLower += n;
        }
        else if (n < coinLowestLarger.first)
        {
            coinLowestLarger = coin;
        }
    }

    if (nTotalLower == nTargetValue)
    {
        for (unsigned int i = 0; i < vValue.size(); ++i)
        {
            setCoinsRet.insert(vValue[i].second);
            nValueRet += vValue[i].first;
        }
        return true;
    }

    if (nTotalLower < nTargetValue)
    {
        if (coinLowestLarger.second.first == NULL)
            return false;
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
        return true;
    }

    // Solve subset sum by stochastic approximation
    std::sort(vValue.begin(), vValue.end(), CompareValueOnly());
    std::reverse(vValue.begin(), vValue.end());
    vector<char> vfBest;
    CAmount nBest;

    ApproximateBestSubset(vValue, nTotalLower, nTargetValue, vfBest, nBest);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + MIN_CHANGE)
        ApproximateBestSubset(vValue, nTotalLower, nTargetValue + MIN_CHANGE, vfBest, nBest);

    // If we have a bigger coin and (either the stochastic approximation didn't find a good solution,
    //                                   or the next bigger coin is closer), return the bigger coin
    if (coinLowestLarger.second.first &&
        ((nBest != nTargetValue && nBest < nTargetValue + MIN_CHANGE) || coinLowestLarger.first <= nBest))
    {
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
    }
    else {
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
            {
                setCoinsRet.insert(vValue[i].second);
                nValueRet += vValue[i].first;
            }

        LogPrint("selectcoins", "SelectCoins() best subset: ");
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
                LogPrint("selectcoins", "%s ", FormatMoney(vValue[i].first));
        LogPrint("selectcoins", "total %s\n", FormatMoney(nBest));
    }

    return true;
}

bool CWallet::SelectCoins(const vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl* coinControl) const
{
    vector<COutput> vCoins(vAvailableCoins);

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coinControl && coinControl->HasSelected() && !coinControl->fAllowOtherInputs)
    {
        BOOST_FOREACH(const COutput& out, vCoins)
        {
            if (!out.fSpendable)
                 continue;
            nValueRet += out.tx->tx->vout[out.i].nValue;
            setCoinsRet.insert(make_pair(out.tx, out.i));
        }
        return (nValueRet >= nTargetValue);
    }

    // calculate value from preset inputs and store them
    set<pair<const CWalletTx*, uint32_t> > setPresetCoins;
    CAmount nValueFromPresetInputs = 0;

    std::vector<COutPoint> vPresetInputs;
    if (coinControl)
        coinControl->ListSelected(vPresetInputs);
    BOOST_FOREACH(const COutPoint& outpoint, vPresetInputs)
    {
        map<uint256, CWalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
        if (it != mapWallet.end())
        {
            const CWalletTx* pcoin = &it->second;
            // Clearly invalid input, fail
            if (pcoin->tx->vout.size() <= outpoint.n)
                return false;
            nValueFromPresetInputs += pcoin->tx->vout[outpoint.n].nValue;
            setPresetCoins.insert(make_pair(pcoin, outpoint.n));
        } else
            return false; // TODO: Allow non-wallet inputs
    }

    // remove preset inputs from vCoins
    for (vector<COutput>::iterator it = vCoins.begin(); it != vCoins.end() && coinControl && coinControl->HasSelected();)
    {
        if (setPresetCoins.count(make_pair(it->tx, it->i)))
            it = vCoins.erase(it);
        else
            ++it;
    }

    size_t nMaxChainLength = std::min(GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT), GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT));
    bool fRejectLongChains = GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    bool res = nTargetValue <= nValueFromPresetInputs ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 6, 0, vCoins, setCoinsRet, nValueRet) ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 1, 0, vCoins, setCoinsRet, nValueRet) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, 2, vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, std::min((size_t)4, nMaxChainLength/3), vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, nMaxChainLength/2, vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, nMaxChainLength, vCoins, setCoinsRet, nValueRet)) ||
        (bSpendZeroConfChange && !fRejectLongChains && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, std::numeric_limits<uint64_t>::max(), vCoins, setCoinsRet, nValueRet));

    // because SelectCoinsMinConf clears the setCoinsRet, we now add the possible inputs to the coinset
    setCoinsRet.insert(setPresetCoins.begin(), setPresetCoins.end());

    // add preset inputs to the total value selected
    nValueRet += nValueFromPresetInputs;

    return res;
}
//add by xxy
bool CWallet::SelectNormalCoins(const vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, set<pair<const CWalletTx*, unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl* coinControl) const
{
	vector<COutput> vCoins(vAvailableCoins);
	// coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
	if (coinControl && coinControl->HasSelected() && !coinControl->fAllowOtherInputs)
	{
		BOOST_FOREACH(const COutput& out, vCoins)
		{
			if (!out.fSpendable)
				continue;
			nValueRet += out.tx->tx->vout[out.i].nValue;
			setCoinsRet.insert(make_pair(out.tx, out.i));
		}
		return (nValueRet >= nTargetValue);
	}

	// calculate value from preset inputs and store them
	set<pair<const CWalletTx*, uint32_t> > setPresetCoins;
	CAmount nValueFromPresetInputs = 0;

	std::vector<COutPoint> vPresetInputs;
	
	if (coinControl)
		coinControl->ListSelected(vPresetInputs);
	BOOST_FOREACH(const COutPoint& outpoint, vPresetInputs)
	{
		map<uint256, CWalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
		if (it != mapWallet.end())
		{
			const CWalletTx* pcoin = &it->second;
			// Clearly invalid input, fail
			if (pcoin->tx->vout.size() <= outpoint.n)
				return false;
			nValueFromPresetInputs += pcoin->tx->vout[outpoint.n].nValue;
			setPresetCoins.insert(make_pair(pcoin, outpoint.n));
		}
		else
			return false; // TODO: Allow non-wallet inputs
	}

	// remove preset inputs from vCoins
	
	for (vector<COutput>::iterator it = vCoins.begin(); it != vCoins.end() && coinControl && coinControl->HasSelected();)
	{
		if (setPresetCoins.count(make_pair(it->tx, it->i)))
			it = vCoins.erase(it);
		else
			++it;
	}

	size_t nMaxChainLength = std::min(GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT), GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT));
	bool fRejectLongChains = GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

	bool res = nTargetValue <= nValueFromPresetInputs ||
		SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 6, 0, vCoins, setCoinsRet, nValueRet) ||
		SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 1, 0, vCoins, setCoinsRet, nValueRet) ||
		(bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, 2, vCoins, setCoinsRet, nValueRet)) ||
		(bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, std::min((size_t)4, nMaxChainLength / 3), vCoins, setCoinsRet, nValueRet)) ||
		(bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, nMaxChainLength / 2, vCoins, setCoinsRet, nValueRet)) ||
		(bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, nMaxChainLength, vCoins, setCoinsRet, nValueRet)) ||
		(bSpendZeroConfChange && !fRejectLongChains && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, std::numeric_limits<uint64_t>::max(), vCoins, setCoinsRet, nValueRet));

	// because SelectCoinsMinConf clears the setCoinsRet, we now add the possible inputs to the coinset
	setCoinsRet.insert(setPresetCoins.begin(), setPresetCoins.end());

	// add preset inputs to the total value selected
	nValueRet += nValueFromPresetInputs;

	return res;
}

bool CWallet::SelectTokenCoins(const std::vector<COutput>& vAvailableCoins, std::set<std::pair<const CWalletTx*, unsigned int> >& setCoinsRet, std::string & symbol, uint64_t& nTokenValue, const CCoinControl *coinControl) const
{
	vector<COutput> vCoins(vAvailableCoins);
	uint8_t txtype = -1;
	uint64_t tempValue = 0;

	//Go through it and see if you can find only one tokencoins to trade and find and return
	tempValue = nTokenValue;
	BOOST_FOREACH(const COutput& out, vCoins)
	{
		if (!out.fSpendable)
			continue;
		//Eight confirmations can be spent
		if (out.nDepth < nTxConfirmTarget)
			continue;
		//
		txtype = out.tx->tx->vout[out.i].txType;
		if (txtype == 4)
		{
			if (symbol != out.tx->tx->vout[out.i].tokenRegLabel.getTokenSymbol())
				continue;
			if (out.tx->tx->vout[out.i].tokenRegLabel.totalCount >= nTokenValue)
			{
				setCoinsRet.insert(make_pair(out.tx, out.i));
				return true;
			}
			return false;
		}
		else if (txtype == TXOUT_ADDTOKEN)
		{
			if (symbol != out.tx->tx->vout[out.i].addTokenLabel.getTokenSymbol())
				continue;
			if (out.tx->tx->vout[out.i].addTokenLabel.currentCount >= nTokenValue)
			{
				setCoinsRet.insert(make_pair(out.tx, out.i));
				return true;
			}
			return false;
		}
		if (txtype == 5)
		{
			if (symbol != out.tx->tx->vout[out.i].tokenLabel.getTokenSymbol())
				continue;
			if (out.tx->tx->vout[out.i].tokenLabel.value >= nTokenValue)
			{
				//clear before
				setCoinsRet.clear();
				setCoinsRet.insert(make_pair(out.tx, out.i));
				return true;
			}
			else if (out.tx->tx->vout[out.i].tokenLabel.value < tempValue)
			{
				setCoinsRet.insert(make_pair(out.tx, out.i));
				tempValue = tempValue - out.tx->tx->vout[out.i].tokenLabel.value;
			}
			else
			{
				setCoinsRet.insert(make_pair(out.tx, out.i));
				return true;
			}
		}
	}

	return false;

}



bool CWallet::SelectUnionTokenCoins(const std::vector<COutput>& vAvailableCoins, std::set<std::pair<const CWalletTx*, unsigned int> >& setCoinsRet, std::string & symbol, uint64_t& nTokenValue, const CCoinControl *coinControl) const
{
    vector<COutput> vCoins(vAvailableCoins);
    uint8_t txtype = -1;
    uint64_t tempValue = 0;

    //Go through it and see if you can find only one tokencoins to trade and find and return
    tempValue = nTokenValue;
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        //if (!out.fSpendable)
        //	continue;
        //Eight confirmations can be spent
        if (out.nDepth < nTxConfirmTarget)
            continue;
        //
        txtype = out.tx->tx->vout[out.i].txType;
        if (txtype == 4)
        {
            if (symbol != out.tx->tx->vout[out.i].tokenRegLabel.getTokenSymbol())
                continue;
            if (out.tx->tx->vout[out.i].tokenRegLabel.totalCount >= nTokenValue)
            {
                setCoinsRet.insert(make_pair(out.tx, out.i));
                return true;
            }
            return false;
        }
        if (txtype == 5)
        {
            if (symbol != out.tx->tx->vout[out.i].tokenLabel.getTokenSymbol())
                continue;
            if (out.tx->tx->vout[out.i].tokenLabel.value >= nTokenValue)
            {
                //clear before
                setCoinsRet.clear();
                setCoinsRet.insert(make_pair(out.tx, out.i));
                return true;
            }
            else if (out.tx->tx->vout[out.i].tokenLabel.value < tempValue)
            {
                setCoinsRet.insert(make_pair(out.tx, out.i));
                tempValue = tempValue - out.tx->tx->vout[out.i].tokenLabel.value;
            }
            else
            {
                setCoinsRet.insert(make_pair(out.tx, out.i));
                return true;
            }
        }
    }

    return false;

}



bool CWallet::SelectUnionTokenCoinsFromLimit(const std::vector<COutput>& vAvailableCoins,\
       std::set<std::pair<const CWalletTx*, unsigned int> >& setCoinsRet,\
    std::string & symbol, uint64_t& nTokenValue, const CCoinControl *coinControl,\
     int maxvinsize) const
{
    vector<COutput> vCoins(vAvailableCoins);
    uint8_t txtype = -1;
    int64_t tempValue = 0;

    //Go through it and see if you can find only one tokencoins to trade and find and return
    tempValue = nTokenValue;
    for(int index = 0;index < vCoins.size();index++)
    {
        if(maxvinsize>0&&index>=maxvinsize){
            break;
        }
        LogPrintf("SelectUnionTokenCoinsFromLimit index:%d maxvinsize:%d\n",index,maxvinsize);
        const COutput& out = vCoins.at(index);
        if (out.nDepth < nTxConfirmTarget)
            continue;
        //
        txtype = out.tx->tx->vout[out.i].txType;
        if (txtype == 4)
        {
            if (symbol != out.tx->tx->vout[out.i].tokenRegLabel.getTokenSymbol())
                continue;
            if (out.tx->tx->vout[out.i].tokenRegLabel.totalCount >= nTokenValue)
            {
                setCoinsRet.insert(make_pair(out.tx, out.i));

                return true;
            }
            return false;
        }
		else if (txtype == TXOUT_ADDTOKEN)
		{
			if (symbol != out.tx->tx->vout[out.i].addTokenLabel.getTokenSymbol())
				continue;
			if (out.tx->tx->vout[out.i].addTokenLabel.currentCount >= nTokenValue)
			{
				setCoinsRet.insert(make_pair(out.tx, out.i));

				return true;
			}
			return false;
		}
        if (txtype == 5)
        {
            if (symbol != out.tx->tx->vout[out.i].tokenLabel.getTokenSymbol())
                continue;
            LogPrintf("SelectUnionTokenCoinsFromLimit tempValue:%d value:%d\n",tempValue,out.tx->tx->vout[out.i].tokenLabel.value);

                setCoinsRet.insert(make_pair(out.tx, out.i));
                tempValue = tempValue - out.tx->tx->vout[out.i].tokenLabel.value;
                if(maxvinsize==0&&tempValue<=0){
                    break;
                }
        }
    }
    if(tempValue<=0){
        return true;
    }
    return false;

}

//end
bool CWallet::FundTransaction(CMutableTransaction& tx, CAmount& nFeeRet, bool overrideEstimatedFeeRate, const CFeeRate& specificFeeRate, int& nChangePosInOut, std::string& strFailReason, bool includeWatching, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, bool keepReserveKey, const CTxDestination& destChange)
{
    vector<CRecipient> vecSend;

    // Turn the txout set into a CRecipient vector
    for (size_t idx = 0; idx < tx.vout.size(); idx++)
    {
        const CTxOut& txOut = tx.vout[idx];
        CRecipient recipient = {txOut.scriptPubKey, txOut.nValue, setSubtractFeeFromOutputs.count(idx) == 1};
        vecSend.push_back(recipient);
    }

    CCoinControl coinControl;
    coinControl.destChange = destChange;
    coinControl.fAllowOtherInputs = true;
    coinControl.fAllowWatchOnly = includeWatching;
    coinControl.fOverrideFeeRate = overrideEstimatedFeeRate;
    coinControl.nFeeRate = specificFeeRate;

    BOOST_FOREACH(const CTxIn& txin, tx.vin)
        coinControl.Select(txin.prevout);

    CReserveKey reservekey(this);
    CWalletTx wtx;
    if (!CreateTransaction(vecSend, wtx, reservekey, nFeeRet, nChangePosInOut, strFailReason, &coinControl, false))
        return false;

    if (nChangePosInOut != -1)
        tx.vout.insert(tx.vout.begin() + nChangePosInOut, wtx.tx->vout[nChangePosInOut]);

    // Copy output sizes from new transaction; they may have had the fee subtracted from them
    for (unsigned int idx = 0; idx < tx.vout.size(); idx++)
        tx.vout[idx].nValue = wtx.tx->vout[idx].nValue;

    // Add new txins (keeping original txin scriptSig/order)
    BOOST_FOREACH(const CTxIn& txin, wtx.tx->vin)
    {
        if (!coinControl.IsSelected(txin.prevout))
        {
            tx.vin.push_back(txin);

            if (lockUnspents)
            {
              LOCK2(cs_main, cs_wallet);
              LockCoin(txin.prevout);
            }
        }
    }

    // optionally keep the change output key
    if (keepReserveKey)
        reservekey.KeepKey();

    return true;
}
bool  CWallet::address2pkhash(std::string& address, uint160& pkhash)
{
	//uint160 pkhash = 0;
	CBitcoinAddress Caddress(address);
	if (!Caddress.IsValid())
		return false;
	const CTxDestination &desaddress = Caddress.Get();
	CKeyID id = *get<CKeyID>(&desaddress);
	pkhash = id;
	return true;
}
bool CWallet::address2Pubkey(std::string& address, std::string& strPubkey, std::string& strFailReason)
{
	LogPrintf("[address2Pubkey] address : %s \n", address);
	CBitcoinAddress Caddress(address);
	if (!Caddress.IsValid())
	{
		strFailReason = _("address is valid!");
		return false;
	}
	
	if (Caddress.IsScript())
	{
		strFailReason = _("address can't be Script!");
		return false;
	}
	CPubKey vchPubKey;
	const CTxDestination &desaddress = Caddress.Get();
	CKeyID id = *get<CKeyID>(&desaddress);
	if (pwalletMain && pwalletMain->GetPubKey(id, vchPubKey))
	{
		strPubkey = HexStr(vchPubKey);
		return true;
	}
	else
	{
		std::cout << "[address2Pubkey] false!" << std::endl;
		strFailReason = _("GetPubKey faild!");
	}
	return false;
}

bool CWallet::JoinCampaign(const CTxDestination &address, CAmount deposi, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
	std::string& strFailReason, const CCoinControl *coinControl, bool sign)

{

	CAmount nValue = 0;
	nFeeRet = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;

	//First verify that the bottom layer is already in charge, and if the bottom is in the bookkeeping, it does not create a transaction
	std::string currentAccountHex;

	CKeyID id = *get<CKeyID>(&address);
	uint160 devoterhash = id;
	if (CConsensusAccountPool::Instance().verifyPkInCandidateListByIndex(chainActive.Tip(), id))
	{
		std::string curaddress = CBitcoinAddress(CKeyID(devoterhash)).ToString();
		strFailReason = "Address " + curaddress + " already join the campaign before,can't join now";
		return false;
	}
	if (CDpocInfo::Instance().GetLocalAccount(currentAccountHex))
	{
		uint160 devoterhash;
		devoterhash.SetHex(currentAccountHex);
		std::string curaddress = CBitcoinAddress(CKeyID(devoterhash)).ToString();
		strFailReason = "Address " + curaddress + " already join the campaign";
		return false;
	}
	

	//Construct system payee - change to destination address
	//CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(Params().system_account_address).Get());
	CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(address).Get());
	vector<CRecipient> vecSend;
	CRecipient recipient = { scriptPubKey, deposi, false};
	vecSend.push_back(recipient);

	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);

	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	{
		set<pair<const CWalletTx*, unsigned int> > setCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;
				bool fFirst = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					//Construct a txout for trading
					DevoteLabel joinLabel;
					joinLabel.ExtendType = TYPE_CONSENSUS_REGISTER;
				
					joinLabel.hash = id;

					CTxOut txout(recipient.nAmount, recipient.scriptPubKey , joinLabel);

					if (recipient.fSubtractFeeFromAmount)
					{
						txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

						if (fFirst) // first receiver pays the remainder not divisible by output count
						{
							fFirst = false;
							txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
						}
					}

					if (txout.IsDust(dustRelayFee))
					{
						if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
						{
							if (txout.nValue < 0)
								strFailReason = _("The transaction amount is too small to pay the fee");
							else
								strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
						}
						else
							strFailReason = _("Transaction amount too small");
						return false;
					}
					txNew.vout.push_back(txout);
					//Add an output that supports exit bookkeeping 1 ipc
					CAmount	nNewoutValue = 1*COIN;
					CTxOut newout(nNewoutValue, recipient.scriptPubKey);
					txNew.vout.push_back(newout);
					nValueToSelect += nNewoutValue;
				}

				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind; 
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at random position:
							//nChangePosInOut = GetRandInt(txNew.vout.size() + 1);
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}

				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}

		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
		}

		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}

bool CWallet::ExitCampaign(CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
	std::string& strFailReason, const CCoinControl *coinControl, bool sign)
{
	CAmount nValue = 0;
	nFeeRet = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;

	//Get the pubkey that is currently in charge at the bottom and return directly if it is empty
	std::string currentCampaignAccountPubkeyStr;
	int ret = CDpocInfo::Instance().GetLocalAccount(currentCampaignAccountPubkeyStr);
	if (ret < 0)
	{
		std::cout << "Not join the campaign yet\n";
		strFailReason = _("Not join the campaign yet");
		return false;
	}
	CKeyID id;
	id.SetHex(currentCampaignAccountPubkeyStr);
	CTxDestination address(id);

	//Construct system payee
	CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(Params().system_account_address).Get());
	vector<CRecipient> vecSend;
	CRecipient recipient = { scriptPubKey, 0, false };
	vecSend.push_back(recipient);

	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);

	{
		set<pair<const CWalletTx*, unsigned int> > setCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> tmpAvailableCoins;
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(tmpAvailableCoins, true, coinControl);

			//Filtering is only the source UTXO that specifies the exit address
			BOOST_FOREACH(const COutput& out, tmpAvailableCoins) {

				CTxDestination utxoaddress;
				const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
				bool fValidAddress = ExtractDestination(scriptPubKey, utxoaddress);

				//if (setAddress.size() && (!fValidAddress || !setAddress.count(address)))
				if (!fValidAddress)
					continue;
				if (utxoaddress != address)
					continue;

				vAvailableCoins.push_back(out);
			}
			if (vAvailableCoins.size() == 0)
			{
				strFailReason = _("The Address which you want to ExitCampaign must have some money!");
				return false;
			}
			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;
				bool fFirst = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					//Construct a txout from the transaction
					DevoteLabel exitLabel;
					exitLabel.ExtendType = TYPE_CONSENSUS_QUITE;
					CKeyID id = *get<CKeyID>(&address);
					exitLabel.hash = id;

					printf("pubkey hash to exit:%s\n", exitLabel.hash.GetHex().c_str());

					CTxOut txout(recipient.nAmount, exitLabel);

					if (recipient.fSubtractFeeFromAmount)
					{
						txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

						if (fFirst) // first receiver pays the remainder not divisible by output count
						{
							fFirst = false;
							txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
						}
					}

					if (txout.IsDust(dustRelayFee))
					{
						if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
						{
							if (txout.nValue < 0)
								strFailReason = _("The transaction amount is too small to pay the fee");
							else
								strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
						}
						else
							strFailReason = _("Transaction amount too small");
						return false;
					}
					txNew.vout.push_back(txout);
				}

				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind; 
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}

				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}

		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
		}

		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}

bool CWallet::PunishRequest(const CTxDestination &address, const std::string evdience, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
	std::string& strFailReason, const CCoinControl *coinControl, bool sign)
{
	CAmount nValue = 0;
	nFeeRet = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;


	//Construct system payee
	CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(Params().system_account_address).Get());
	vector<CRecipient> vecSend;
	CRecipient recipient = { scriptPubKey, 0, false };
	vecSend.push_back(recipient);

	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);

	{
		set<pair<const CWalletTx*, unsigned int> > setCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);
			
			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;
				bool fFirst = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					//Construct the penalty application transaction
					CKeyID id = *get<CKeyID>(&address);
					CTxOut txout(id, evdience);

					if (recipient.fSubtractFeeFromAmount)
					{
						txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

						if (fFirst) // first receiver pays the remainder not divisible by output count
						{
							fFirst = false;
							txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
						}
					}

					if (txout.IsDust(dustRelayFee))
					{
						if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
						{
							if (txout.nValue < 0)
								strFailReason = _("The transaction amount is too small to pay the fee");
							else
								strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
						}
						else
							strFailReason = _("Transaction amount too small");
						return false;
					}
					txNew.vout.push_back(txout);
				}

				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				for (const auto& pcoin : setCoins)
				{
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
						std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}

				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}

		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
		}

		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}

bool CWallet::CreateTransaction(const vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet,
       int& nChangePosInOut, std::string& strFailReason, const CCoinControl* coinControl, bool sign)
{
	nFeeRet = 0;
    CAmount nValue = 0;
    int nChangePosRequest = nChangePosInOut;
    unsigned int nSubtractFeeFromAmount = 0;
    for (const auto& recipient : vecSend)
    {
        if (nValue < 0 || recipient.nAmount < 0)
        {
            strFailReason = _("Transaction amounts must not be negative");
            return false;
        }
        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount)
            nSubtractFeeFromAmount++;
    }
    if (vecSend.empty())
    {
        strFailReason = _("Transaction must have at least one recipient");
        return false;
    }

    wtxNew.fTimeReceivedIsTxTime = true;
    wtxNew.BindWallet(this);
    CMutableTransaction txNew;

    // Discourage fee sniping.
    //
    // For a large miner the value of the transactions in the best block and
    // the mempool can exceed the cost of deliberately attempting to mine two
    // blocks to orphan the current best block. By setting nLockTime such that
    // only the next block can include the transaction, we discourage this
    // practice as the height restricted and limited blocksize gives miners
    // considering fee sniping fewer options for pulling off this attack.
    //
    // A simple way to think about this is from the wallet's point of view we
    // always want the blockchain to move forward. By setting nLockTime this
    // way we're basically making the statement that we only want this
    // transaction to appear in the next block; we don't want to potentially
    // encourage reorgs by allowing transactions to appear at lower heights
    // than the next block in forks of the best chain.
    //
    // Of course, the subsidy is high enough, and transaction volume low
    // enough, that fee sniping isn't a problem yet, but by implementing a fix
    // now we ensure code won't be written that makes assumptions about
    // nLockTime that preclude a fix later.
    txNew.nLockTime = chainActive.Height();

    // Secondly occasionally randomly pick a nLockTime even further back, so
    // that transactions that are delayed after signing for whatever reason,
    // e.g. high-latency mix networks and some CoinJoin implementations, have
    // better privacy.
    if (GetRandInt(10) == 0)
        txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

    assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
    assert(txNew.nLockTime < LOCKTIME_THRESHOLD);

    {
        set<pair<const CWalletTx*,unsigned int> > setCoins;
        LOCK2(cs_main, cs_wallet);
        {
            std::vector<COutput> vAvailableCoins;
            AvailableNormalCoins(vAvailableCoins, true, coinControl);

            nFeeRet = 0;
            // Start with no fee and loop until there is enough fee
            while (true)
            {
                nChangePosInOut = nChangePosRequest;
                txNew.vin.clear();
                txNew.vout.clear();
                wtxNew.fFromMe = true;
                bool fFirst = true;

                CAmount nValueToSelect = nValue;
                if (nSubtractFeeFromAmount == 0)
                    nValueToSelect += nFeeRet;
                double dPriority = 0;
                // vouts to the payees
                for (const auto& recipient : vecSend)
                {
                    CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

                    if (recipient.fSubtractFeeFromAmount)
                    {
                        txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

                        if (fFirst) // first receiver pays the remainder not divisible by output count
                        {
                            fFirst = false;
                            txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
                        }
                    }

                    if (txout.IsDust(dustRelayFee))
                    {
                        if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
                        {
                            if (txout.nValue < 0)
                                strFailReason = _("The transaction amount is too small to pay the fee");
                            else
                                strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                        }
                        else
                            strFailReason = _("Transaction amount too small");
                        return false;
                    }
                    txNew.vout.push_back(txout);
                }

                // Choose coins to use
                CAmount nValueIn = 0;
                setCoins.clear();
                if (!SelectCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
                {
                    strFailReason = _("Insufficient funds");
                    return false;
                }
                for (const auto& pcoin : setCoins)
                {
                    CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
                    //The coin age after the next block (depth+1) is used instead of the current,
                    //reflecting an assumption the user would accept a bit more delay for
                    //a chance at a free transaction.
                    //But mempool inputs might still be in the mempool, so their age stays 0
                    int age = pcoin.first->GetDepthInMainChain();
                    assert(age >= 0);
                    if (age != 0)
                        age += 1;
                    dPriority += (double)nCredit * age;
                }

                const CAmount nChange = nValueIn - nValueToSelect;
                if (nChange > 0)
                {
                    // Fill a vout to ourself
                    // TODO: pass in scriptChange instead of reservekey so
                    // change transaction isn't always pay-to-ipchain-address
                    CScript scriptChange;

                    // coin control: send change to custom address
                    if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
                        scriptChange = GetScriptForDestination(coinControl->destChange);

                    // no coin control: send change to newly generated address
                    else
                    {
                        // Note: We use a new key here to keep it from being obvious which side is the change.
                        //  The drawback is that by not reusing a previous key, the change may be lost if a
                        //  backup is restored, if the backup doesn't have the new private key for the change.
                        //  If we reused the old key, it would be possible to add code to look for and
                        //  rediscover unknown transactions that were written with keys of ours to recover
                        //  post-backup change.

                        // Reserve a new key pair from key pool
                        CPubKey vchPubKey;
                        bool ret;
                        ret = reservekey.GetReservedKey(vchPubKey);
                        if (!ret)
                        {
                            strFailReason = _("Keypool ran out, please call keypoolrefill first");
                            return false;
                        }

                        scriptChange = GetScriptForDestination(vchPubKey.GetID());
                    }

                    CTxOut newTxOut(nChange, scriptChange);

                    // We do not move dust-change to fees, because the sender would end up paying more than requested.
                    // This would be against the purpose of the all-inclusive feature.
                    // So instead we raise the change and deduct from the recipient.
                    if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
                    {
                        CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
                        newTxOut.nValue += nDust; // raise change until no more dust
                        for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
                        {
                            if (vecSend[i].fSubtractFeeFromAmount)
                            {
                                txNew.vout[i].nValue -= nDust;
                                if (txNew.vout[i].IsDust(dustRelayFee))
                                {
                                    strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                                    return false;
                                }
                                break;
                            }
                        }
                    }

                    // Never create dust outputs; if we would, just
                    // add the dust to the fee.
                    if (newTxOut.IsDust(dustRelayFee))
                    {
                        nChangePosInOut = -1;
                        nFeeRet += nChange;
                        reservekey.ReturnKey();
                    }
                    else
                    {
                        if (nChangePosInOut == -1)
                        {
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
                        }
                        else if ((unsigned int)nChangePosInOut > txNew.vout.size())
                        {
                            strFailReason = _("Change index out of range");
                            return false;
                        }

                        vector<CTxOut>::iterator position = txNew.vout.begin()+nChangePosInOut;
                        txNew.vout.insert(position, newTxOut);
                    }
                }
                else
                    reservekey.ReturnKey();

                // Fill vin
                //
                // Note how the sequence number is set to non-maxint so that
                // the nLockTime set above actually works.
                //
                // BIP125 defines opt-in RBF as any nSequence < maxint-1, so
                // we use the highest possible value in that range (maxint-2)
                // to avoid conflicting with other possible uses of nSequence,
                // and in the spirit of "smallest possible change from prior
                // behavior."
                for (const auto& coin : setCoins)
                    txNew.vin.push_back(CTxIn(coin.first->GetHash(),coin.second,CScript(),
                                              std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

                // Fill in dummy signatures for fee calculation.
                if (!DummySignTx(txNew, setCoins)) {
                    strFailReason = _("Signing transaction failed");
                    return false;
                }

                unsigned int nBytes = GetVirtualTransactionSize(txNew);

                CTransaction txNewConst(txNew);
                dPriority = txNewConst.ComputePriority(dPriority, nBytes);

                // Remove scriptSigs to eliminate the fee calculation dummy signatures
                for (auto& vin : txNew.vin) {
                    vin.scriptSig = CScript();
                    vin.scriptWitness.SetNull();
                }

                // Allow to override the default confirmation target over the CoinControl instance
                int currentConfirmationTarget = nTxConfirmTarget;
                if (coinControl && coinControl->nConfirmTarget > 0)
                    currentConfirmationTarget = coinControl->nConfirmTarget;

                // Can we complete this as a free transaction?
                if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
                {
                    // Not enough fee: enough priority?
                    double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
                    // Require at least hard-coded AllowFree.
                    if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
                        break;
                }

                CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
                if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
                    nFeeNeeded = coinControl->nMinimumTotalFee;
                }
                if (coinControl && coinControl->fOverrideFeeRate)
                    nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

                // If we made it here and we aren't even able to meet the relay fee on the next pass, give up
                // because we must be at the maximum allowed fee.
                if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
                {
                    strFailReason = _("Transaction too large for fee policy");
                    return false;
                }

                if (nFeeRet >= nFeeNeeded) {
                    // Reduce fee to only the needed amount if we have change
                    // output to increase.  This prevents potential overpayment
                    // in fees if the coins selected to meet nFeeNeeded result
                    // in a transaction that requires less fee than the prior
                    // iteration.
                    // TODO: The case where nSubtractFeeFromAmount > 0 remains
                    // to be addressed because it requires returning the fee to
                    // the payees and not the change output.
                    // TODO: The case where there is no change output remains
                    // to be addressed so we avoid creating too small an output.
                    if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                        CAmount extraFeePaid = nFeeRet - nFeeNeeded;
                        vector<CTxOut>::iterator change_position = txNew.vout.begin()+nChangePosInOut;
                        change_position->nValue += extraFeePaid;
                        nFeeRet -= extraFeePaid;
                    }
                    break; // Done, enough fee included.
                }

                // Try to reduce change to include necessary fee
                if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                    CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
                    vector<CTxOut>::iterator change_position = txNew.vout.begin()+nChangePosInOut;
                    // Only reduce change if remaining amount is still a large enough output.
                    if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
                        change_position->nValue -= additionalFeeNeeded;
                        nFeeRet += additionalFeeNeeded;
                        break; // Done, able to increase fee from change
                    }
                }

                // Include more fee and try again.
                nFeeRet = nFeeNeeded;
                continue;
            }
        }

        if (sign)
        {
            CTransaction txNewConst(txNew);
            int nIn = 0;
            for (const auto& coin : setCoins)
            {
                const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
                SignatureData sigdata;

                if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
                {
                    strFailReason = _("Signing transaction failed");
                    return false;
                } else {
                    UpdateTransaction(txNew, nIn, sigdata);
                }

                nIn++;
            }
        }

        // Embed the constructed transaction data in wtxNew.
        wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

        // Limit size
        if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
        {
            strFailReason = _("Transaction too large");
            return false;
        }
    }

    if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits
        LockPoints lp;
        CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
        CTxMemPool::setEntries setAncestors;
        size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
        size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT)*1000;
        size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
        size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT)*1000;
        std::string errString;
        if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
            strFailReason = _("Transaction has too long of a mempool chain");
            return false;
        }
    }
    return true;
}
//add by xxy
bool CWallet::CreateNormalTransaction(const vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet,
	int& nChangePosInOut, std::string& strFailReason, const CCoinControl* coinControl, bool sign)
{
	CAmount nValue = 0;
	nFeeRet = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE*2);	
	{
		set<pair<const CWalletTx*, unsigned int> > setCoins;
	
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;
				bool fFirst = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

					if (recipient.fSubtractFeeFromAmount)
					{
						txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

						if (fFirst) // first receiver pays the remainder not divisible by output count
						{
							fFirst = false;
							txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
						}
					}

					if (txout.IsDust(dustRelayFee))
					{
						if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
						{
							if (txout.nValue < 0)
								strFailReason = _("The transaction amount is too small to pay the fee");
							else
								strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
						}
						else
							strFailReason = _("Transaction amount too small");
						return false;
					}
					txNew.vout.push_back(txout);
				}

				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl)) 
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind;	
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}

				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}

		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
                    LogPrintf("ProduceSignature : false \n");
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
		}

		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}
//StrReglabel: know the IPCLabel of the register; VecSend: recipient information
bool CWallet::CreateIPCRegTransaction(std::string& strReglabel, const vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet,
	int& nChangePosInOut, std::string& strFailReason, const CCoinControl* coinControl, bool sign)
{
	nFeeRet = 0;
	CAmount nValue = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	if (strReglabel.size() == 0)
	{
		strFailReason = _("IPCRegTransaction must have IPCLabel");
		return false;
	}
	UniValue JsonIpclabel;
	bool isNUll = JsonIpclabel.read(strReglabel);
	IPCLabel label;
	UniValue o = find_value(JsonIpclabel, "ExtendType");
	if (o.isNull() || o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's ExtendType err");
		return false;
	}
	label.ExtendType = (uint8_t)o.get_int();
	if (label.ExtendType < 0 )
	{
		strFailReason = _("The IPCLabel's ExtendType err");
		return false;
	}
	label.startTime = timeService.GetCurrentTimeSeconds();
	label.stopTime = 0;										
	label.reAuthorize = 1;	
	label.uniqueAuthorize = 0;
	o = find_value(JsonIpclabel, "hashLen");
	if (o.isNull() || o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's hashLen err");
		return false;
	}
	label.hashLen = (uint8_t)o.get_int();

	o = find_value(JsonIpclabel, "hash");
	if (o.isNull() || o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's hash err");
		return false;
	}
	std::string ipchash = o.get_str();
	if (ipchash.length() != 32)
	{
		strFailReason = _("this ipchash length is Wrongful ");
		return false;
	}
	label.hash.SetHex(ipchash);

	o = find_value(JsonIpclabel, "labelTitle");
	if (o.isNull() || o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's labelTitle err");
		return false;
	}
	label.labelTitle = o.get_str();
	o = find_value(JsonIpclabel, "txLabel");
	if (o.isNull() || o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's txLabel err");
		return false;
	}

	std::string TxLabel = o.get_str();
	{
		set<pair<const CWalletTx*, unsigned int> > setCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl); 

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;
				//bool fFirst = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{

		
					CTxOut txout(recipient.nAmount, recipient.scriptPubKey, TXOUT_IPCOWNER, label, TxLabel); 

					txNew.vout.push_back(txout);
				}

				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind;  
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}

				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}

		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
		}

		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}
//Knowledge transfer transaction  txid: transaction id for input; Index: the corresponding Index in the pre output; VecSend: recipient information
bool CWallet::CreateIPCSendTransactionWithTxlabel(std::string& txid, int Index, const vector<CRecipient>& vecSend, std::string& strtxlabel, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet,
	int& nChangePosInOut,  std::string& strFailReason, const CCoinControl* coinControl, bool sign)
{
	nFeeRet = 0;
	CAmount nValue = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	UniValue uTxid = UniValue(txid);
	uint256 hash = ParseHashV(uTxid, "parameter 1");
	if (hash.IsNull())
	{
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid Ipchain txid");
	}
	CTransactionRef tx;
	uint256 hashBlock;
	if (!GetTransaction(hash, tx, Params().GetConsensus(), hashBlock, true))
		throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string(fTxIndex ? "No such mempool or blockchain transaction"
		: "No such mempool transaction. Use -txindex to enable blockchain transaction queries") +
		". Use gettransaction for wallet transactions.");
	
	IPCLabel ipcSendLabel;
	ipcSendLabel = tx->vout[Index].ipcLabel; 
	ipcSendLabel.startTime = timeService.GetCurrentTimeSeconds();
	if (strtxlabel.length() >TXLABLE_MAX_LENGTH -1 )
	{
		throw JSONRPCError(RPC_INVALID_PARAMETER, "The length of txlabel should not be more than 511.");
	}
	std::string TxLabel = strtxlabel;

	{
		//Check the incoming txid index for your unspent knowledge
		std::vector<COutput> vAvailableIPCCoins;
		AvailableIPCCoins(vAvailableIPCCoins, true, coinControl);
		bool isFind = false;
		std::string cointxid = "";
		BOOST_FOREACH(const COutput& coin, vAvailableIPCCoins)
		{
			cointxid = coin.tx->tx->GetHash().GetHex();
			if (cointxid == txid && coin.i == Index && coin.tx->GetDepthInMainChain() >= nTxConfirmTarget)
				isFind = true;
		}
		if (!isFind)
		{
			strFailReason = _("This coins you can't spent now!");
			return false;
		}
			
		set<pair<const CWalletTx*, unsigned int> > setCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;
			//	bool fFirst = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					CTxOut txout(recipient.nAmount, recipient.scriptPubKey, TXOUT_IPCOWNER, ipcSendLabel, TxLabel);

					txNew.vout.push_back(txout);
				}
				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind;  
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));
				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}
 				txNew.vin.push_back(CTxIn(tx->GetHash(), Index, CScript(),
 				std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));
					
				//Add fake signature, calculate service charge
				const CScript& scriptPubKey = tx->vout[Index].scriptPubKey;
				SignatureData sigdata;
				int iNewVinsize = txNew.vin.size();
				if (!ProduceSignature(DummySignatureCreator(this), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing  Addvin of transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, iNewVinsize - 1, sigdata);
				}
			
				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}

		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
			//The added ipc type input also needs to be signed
			int itxNewVinsize = txNew.vin.size();

			const CScript& scriptPubKey = tx->vout[Index].scriptPubKey;
			SignatureData sigdata;

			if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, itxNewVinsize - 1, tx->vout[Index].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
			{
				strFailReason = _("Signing transaction failed");
				return false;
			}
			else {
				UpdateTransaction(txNew, itxNewVinsize-1, sigdata);
			}

		}

		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}

//Knowledge transfer transaction  txid: transaction id for input; Index: the corresponding Index in the pre output; VecSend: recipient information
bool CWallet::CreateIPCSendTransaction(std::string& txid, int Index, const vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet,
	int& nChangePosInOut, std::string& strFailReason, const CCoinControl* coinControl, bool sign)
{
	nFeeRet = 0;
	CAmount nValue = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	UniValue uTxid = UniValue(txid);
	uint256 hash = ParseHashV(uTxid, "parameter 1");
	if (hash.IsNull())
	{
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid Ipchain txid");
	}
	CTransactionRef tx;
	uint256 hashBlock;
	if (!GetTransaction(hash, tx, Params().GetConsensus(), hashBlock, true))
		throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string(fTxIndex ? "No such mempool or blockchain transaction"
		: "No such mempool transaction. Use -txindex to enable blockchain transaction queries") +
		". Use gettransaction for wallet transactions.");

	IPCLabel ipcSendLabel;
	ipcSendLabel = tx->vout[Index].ipcLabel;
	ipcSendLabel.startTime = timeService.GetCurrentTimeSeconds();
	std::string TxLabel = "";

	{
		//Check the incoming txid index for your unspent knowledge
		std::vector<COutput> vAvailableIPCCoins;
		AvailableIPCCoins(vAvailableIPCCoins, true, coinControl);
		bool isFind = false;
		std::string cointxid = "";
		BOOST_FOREACH(const COutput& coin, vAvailableIPCCoins)
		{
			cointxid = coin.tx->tx->GetHash().GetHex();
			if (cointxid == txid && coin.i == Index && coin.tx->GetDepthInMainChain() >= nTxConfirmTarget)
				isFind = true;
		}
		if (!isFind)
		{
			strFailReason = _("This coins you can't spent now!");
			return false;
		}

		set<pair<const CWalletTx*, unsigned int> > setCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;
				//	bool fFirst = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					CTxOut txout(recipient.nAmount, recipient.scriptPubKey, TXOUT_IPCOWNER, ipcSendLabel, TxLabel);

					txNew.vout.push_back(txout);
				}
				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind;
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));
				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}
				txNew.vin.push_back(CTxIn(tx->GetHash(), Index, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				//Add fake signature, calculate service charge
				const CScript& scriptPubKey = tx->vout[Index].scriptPubKey;
				SignatureData sigdata;
				int iNewVinsize = txNew.vin.size();
				if (!ProduceSignature(DummySignatureCreator(this), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing  Addvin of transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, iNewVinsize - 1, sigdata);
				}

				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}

		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
			//The added ipc type input also needs to be signed
			int itxNewVinsize = txNew.vin.size();

			const CScript& scriptPubKey = tx->vout[Index].scriptPubKey;
			SignatureData sigdata;

			if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, itxNewVinsize - 1, tx->vout[Index].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
			{
				strFailReason = _("Signing transaction failed");
				return false;
			}
			else {
				UpdateTransaction(txNew, itxNewVinsize - 1, sigdata);
			}

		}

		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}

//To authorize and re-authorize transactions; Txid: the transaction id of the input; Index: the corresponding Index in the pre output; VecSend: recipient information; IPCAuthorizeLabel: IP licensed or reauthorized IPClabel
bool CWallet::CreateIPCAuthorizationTransactionWithTxlabel(std::string& txid, int Index, const vector<CRecipient>& vecSend, std::string& IPCAuthorizeLabel, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet,
	int& nChangePosInOut, std::string& strFailReason, const CCoinControl* coinControl, bool sign)
{
	nFeeRet = 0;
	CAmount nValue = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	UniValue uTxid = UniValue(txid);
	uint256 hash = ParseHashV(uTxid, "parameter 1");
	if (hash.IsNull())
	{
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid Ipchain txid");
	}
	CTransactionRef tx;
	uint256 hashBlock;
	if (!GetTransaction(hash, tx, Params().GetConsensus(), hashBlock, true))
		throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string(fTxIndex ? "No such mempool or blockchain transaction"
		: "No such mempool transaction. Use -txindex to enable blockchain transaction queries") +
		". Use gettransaction for wallet transactions.");

	if (IPCAuthorizeLabel.size() == 0)
	{
		strFailReason = _("IPCAuthorizeTransaction must have IPCLabel");
		return false;
	}
	UniValue JsonIpclabel;
	bool isRightRead = JsonIpclabel.read(IPCAuthorizeLabel);
	IPCLabel label;

	UniValue o = find_value(JsonIpclabel, "startTime");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's startTime err");
		return false;
	}
	label.startTime = o.get_int32();
	o = find_value(JsonIpclabel, "stopTime");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's stopTime err");
		return false;
	}
	label.stopTime = o.get_int32();
	o = find_value(JsonIpclabel, "reAuthorize");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's reAuthorize err");
		return false;
	}
	label.reAuthorize = (uint8_t)o.get_int();

	o = find_value(JsonIpclabel, "txLabel");
	if (o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's txLabel err");
		return false;
	}
	
// 	o = find_value(JsonIpclabel, "uniqueAuthorize");
// 	if (o.isNull() || o.type() != UniValue::VNUM)
// 	{
// 		strFailReason = _("The IPCLabel's uniqueAuthorize err");
// 		return false;
// 	}
/*	label.uniqueAuthorize = (uint8_t)o.get_int();*/
	label.uniqueAuthorize = 0;
	bool IsUniqueAuthorize = (bool)(label.uniqueAuthorize);
	label.ExtendType = tx->vout[Index].ipcLabel.ExtendType;
	label.hashLen = tx->vout[Index].ipcLabel.hashLen;
	label.hash = tx->vout[Index].ipcLabel.hash;
	label.labelTitle = tx->vout[Index].ipcLabel.labelTitle;
	std::string TxLabel = o.get_str();

	{
		//Check the incoming txid index for your unspent knowledge
		std::vector<COutput> vAvailableIPCCoins;
		AvailableIPCCoins(vAvailableIPCCoins, true, coinControl);
		bool isFind = false;
		std::string cointxid = "";
		BOOST_FOREACH(const COutput& coin, vAvailableIPCCoins)
		{
			cointxid = coin.tx->tx->GetHash().GetHex();
			if (cointxid == txid && coin.i == Index&& coin.tx->GetDepthInMainChain() >= nTxConfirmTarget)
				isFind = true;
		}
		if (!isFind)
		{
			strFailReason = _("This coins you have spent already!");
			return false;
		}

		set<pair<const CWalletTx*, unsigned int> > setCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;
				//	bool fFirst = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					CTxOut txout(recipient.nAmount, recipient.scriptPubKey, TXOUT_IPCAUTHORIZATION, label, TxLabel);

					txNew.vout.push_back(txout);
					//Add the output of the change of ownership

					CTxOut txinpretxout;

					if (!IsUniqueAuthorize)		//Non-exclusive authorization
					{
						txinpretxout = tx->vout[Index];
					}
					else
					{
						txinpretxout = tx->vout[Index];
						txinpretxout.txLabel = "";
					}
					txNew.vout.push_back(txinpretxout);

				}

				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind;
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				//end*****************************************************************************************************************
				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}
				//Join an input
				txNew.vin.push_back(CTxIn(tx->GetHash(), Index, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));
				//Add fake signature, calculate service charge
				const CScript& scriptPubKey = tx->vout[Index].scriptPubKey;
				SignatureData sigdata;
				int iNewVinsize = txNew.vin.size();
				if (!ProduceSignature(DummySignatureCreator(this), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing  Addvin of transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, iNewVinsize - 1, sigdata);
				}

				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}

		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
			//The added ipc type input also needs to be signed
			int itxNewVinsize = txNew.vin.size();

			const CScript& scriptPubKey = tx->vout[Index].scriptPubKey;
			SignatureData sigdata;

			if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, itxNewVinsize - 1, tx->vout[Index].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
			{
				strFailReason = _("Signing transaction failed");
				return false;
			}
			else {
				UpdateTransaction(txNew, itxNewVinsize - 1, sigdata);
			}
			nIn++;
		}

		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}

//To authorize and re-authorize transactions; Txid: the transaction id of the input; Index: the corresponding Index in the pre output; VecSend: recipient information; IPCAuthorizeLabel: IP licensed or reauthorized IPClabel
bool CWallet::CreateIPCAuthorizationTransaction(std::string& txid, int Index, const vector<CRecipient>& vecSend, std::string& IPCAuthorizeLabel, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet,
	int& nChangePosInOut, std::string& strFailReason, const CCoinControl* coinControl, bool sign)
{
	nFeeRet = 0;
	CAmount nValue = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	UniValue uTxid = UniValue(txid);
	uint256 hash = ParseHashV(uTxid, "parameter 1");
	if (hash.IsNull())
	{
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid Ipchain txid");
	}
	CTransactionRef tx;
	uint256 hashBlock;
	if (!GetTransaction(hash, tx, Params().GetConsensus(), hashBlock, true))
		throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string(fTxIndex ? "No such mempool or blockchain transaction"
		: "No such mempool transaction. Use -txindex to enable blockchain transaction queries") +
		". Use gettransaction for wallet transactions.");

	if (IPCAuthorizeLabel.size() == 0)
	{
		strFailReason = _("IPCAuthorizeTransaction must have IPCLabel");
		return false;
	}
	UniValue JsonIpclabel;
	bool isRightRead = JsonIpclabel.read(IPCAuthorizeLabel);
	IPCLabel label;

	UniValue o = find_value(JsonIpclabel, "startTime");
	if (o.isNull()||o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's startTime err");
		return false;
	}
	label.startTime = o.get_int32();
	o = find_value(JsonIpclabel, "stopTime");
	if (o.isNull()||o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's stopTime err");
		return false;
	}
	label.stopTime = o.get_int32();
	o = find_value(JsonIpclabel, "reAuthorize");
	if (o.isNull()|| o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's reAuthorize err");
		return false;
	}
	label.reAuthorize = (uint8_t)o.get_int();
	
	o = find_value(JsonIpclabel, "uniqueAuthorize");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's uniqueAuthorize err");
		return false;
	}
	label.uniqueAuthorize = (uint8_t)o.get_int(); 
	label.uniqueAuthorize = 0;
	bool IsUniqueAuthorize = (bool)(label.uniqueAuthorize);
	label.ExtendType = tx->vout[Index].ipcLabel.ExtendType;
	label.hashLen = tx->vout[Index].ipcLabel.hashLen;
	label.hash = tx->vout[Index].ipcLabel.hash;
	label.labelTitle = tx->vout[Index].ipcLabel.labelTitle;
	std::string TxLabel = "";

	{
		//Check the incoming txid index for your unspent knowledge
		std::vector<COutput> vAvailableIPCCoins;
		AvailableIPCCoins(vAvailableIPCCoins, true, coinControl);
		bool isFind = false;
		std::string cointxid = "";
		BOOST_FOREACH(const COutput& coin, vAvailableIPCCoins)
		{
			cointxid = coin.tx->tx->GetHash().GetHex();
			if (cointxid == txid && coin.i == Index&& coin.tx->GetDepthInMainChain() >= nTxConfirmTarget)
				isFind = true;
		}
		if (!isFind)
		{
			strFailReason = _("This coins you have spent already!");
			return false;
		}

		set<pair<const CWalletTx*, unsigned int> > setCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;
			//	bool fFirst = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					CTxOut txout(recipient.nAmount, recipient.scriptPubKey, TXOUT_IPCAUTHORIZATION, label, TxLabel);

					txNew.vout.push_back(txout);
					//Add the output of the change of ownership
					
					CTxOut txinpretxout ;
				
					if (!IsUniqueAuthorize)		//Non-exclusive authorization
					{
						txinpretxout = tx->vout[Index];
					}
					else
					{ 
						txinpretxout = tx->vout[Index];
						txinpretxout.txLabel = "";
					}
					txNew.vout.push_back(txinpretxout);

				}
		
				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind; 
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));
				
				//end*****************************************************************************************************************
				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}
				//Join an input
				txNew.vin.push_back(CTxIn(tx->GetHash(), Index, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));
				//Add fake signature, calculate service charge
				const CScript& scriptPubKey = tx->vout[Index].scriptPubKey;
				SignatureData sigdata;
				int iNewVinsize = txNew.vin.size();
				if(!ProduceSignature(DummySignatureCreator(this), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing  Addvin of transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, iNewVinsize-1, sigdata);
				}

				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}

		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
			//The added ipc type input also needs to be signed
			int itxNewVinsize = txNew.vin.size();

			const CScript& scriptPubKey = tx->vout[Index].scriptPubKey;
			SignatureData sigdata;

			if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, itxNewVinsize - 1, tx->vout[Index].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
			{
				strFailReason = _("Signing transaction failed");
				return false;
			}
			else {
				UpdateTransaction(txNew, itxNewVinsize - 1, sigdata);
			}
			 nIn++;
		}
		
		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}
//To create a token registration transaction; StrReglabel: TokenRegLabel; VecSend: recipient information
bool CWallet::CreateTokenRegTransaction(std::string& strReglabel, const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
	std::string& strFailReason, const CCoinControl *coinControl , bool sign )
{
	nFeeRet = 0;
	CAmount nValue = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
		break;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	if (strReglabel.size() == 0)
	{
		strFailReason = _("IPCRegTransaction must have IPCTokenLabel");
		return false;
	}
	UniValue JsonIpclabel;
	bool isNUll = JsonIpclabel.read(strReglabel);

	TokenRegLabel label;
	UniValue o = find_value(JsonIpclabel, "TokenSymbol");
	if (o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's TokenSymbol err");
		return false;
	}
	std::string strTokenSymbol = o.get_str();
	if (strTokenSymbol == "")
	{
		strFailReason = _(" IPCTokenLabel's TokenSymbol is NULL");
		return false;
	}
	if (strTokenSymbol.length()>8)
	{
		strFailReason = _(" IPCTokenLabel's TokenSymbol is longer than 8");
		return false;
	}
	strncpy((char*)(label.TokenSymbol), strTokenSymbol.c_str(), strTokenSymbol.length());
	o = find_value(JsonIpclabel, "value");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's value err");
		return false;
	}
	label.value = o.get_int64();
	o = find_value(JsonIpclabel, "hash");
	if ( o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's hash err");
		return false;
	}
	label.hash.SetHex(o.get_str());
	o = find_value(JsonIpclabel, "label");
	if (o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's label err");
		return false;
	}
	std::string strlabel = o.get_str();
	if (strlabel.length() > 16)
	{
		strFailReason = _(" IPCTokenLabel's Label is longer than 16");
		return false;
	}
	strncpy((char*)(label.label), strlabel.c_str(), strlabel.length());
	o = find_value(JsonIpclabel, "issueDate");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's issueDate err");
		return false;
	}
	label.issueDate = o.get_int32();
	o = find_value(JsonIpclabel, "totalCount");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's totalCount err");
		return false;
	}
	label.totalCount = o.get_int64();
	o = find_value(JsonIpclabel, "accuracy");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's accuracy err");
		return false;
	}
	label.accuracy = o.get_int();
	o = find_value(JsonIpclabel, "txLabel");
	if ( o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's txLabel err");
		return false;
	}
	std::string TxLabel = o.get_str();
	CAmount nDefalutFee = 100 * COIN;
	nValue += nDefalutFee;

	{
		set<pair<const CWalletTx*, unsigned int> > setCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					CTxOut txout(recipient.nAmount, recipient.scriptPubKey,  label, TxLabel); 
					txNew.vout.push_back(txout);
					break;
				}

				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind;  
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}

				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}
		
		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
		}

		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}
//To create a token registration transaction; StrReglabel: TokenRegLabel; VecSend: recipient information
bool CWallet::CreateAddTokenRegTransactionByStrReglabel(std::string& strReglabel, const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
	std::string& strFailReason, const CCoinControl *coinControl, bool sign)
{
	nFeeRet = 0;
	CAmount nValue = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
		break;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	if (strReglabel.size() == 0)
	{
		strFailReason = _("IPCRegTransaction must have IPCTokenLabel");
		return false;
	}
	UniValue JsonIpclabel;
	bool isNUll = JsonIpclabel.read(strReglabel);
	if (!isNUll)
	{
		strFailReason = _("IPCRegTransaction must have strReglabel.");
		return false;
	}
	AddTokenLabel label;
	UniValue o = find_value(JsonIpclabel, "TokenSymbol");
	if (o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's TokenSymbol err");
		return false;
	}
	std::string strTokenSymbol = o.get_str();
	if (strTokenSymbol == "")
	{
		strFailReason = _(" IPCTokenLabel's TokenSymbol is NULL");
		return false;
	}
	if (strTokenSymbol.length() > 8)
	{
		strFailReason = _(" IPCTokenLabel's TokenSymbol is longer than 8");
		return false;
	}
	strncpy((char*)(label.TokenSymbol), strTokenSymbol.c_str(), strTokenSymbol.length());
	o = find_value(JsonIpclabel, "accuracy");
	if (o.isNull() || o.type() != UniValue::VNUM){
		strFailReason = _("The IPCLabel's accuracy err");
		return false;
	}
	label.accuracy = o.get_int();
	o = find_value(JsonIpclabel, "addmode");
	if (o.isNull() || o.type() != UniValue::VNUM || (o.get_int() != 0 && o.get_int()!=1)){
		strFailReason = _("The IPCLabel's accuracy err");
		return false;
	}
	label.addmode = o.get_int();
	//label.value = o.get_int64();
	o = find_value(JsonIpclabel, "hash");
	if (o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's hash err");
		return false;
	}
	label.hash.SetHex(o.get_str());
	o = find_value(JsonIpclabel, "label");
	if (o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's label err");
		return false;
	}
	std::string strlabel = o.get_str();
	if (strlabel.length() > 16)
	{
		strFailReason = _(" IPCTokenLabel's Label is longer than 16");
		return false;
	}
	strncpy((char*)(label.label), strlabel.c_str(), strlabel.length());
	o = find_value(JsonIpclabel, "issueDate");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The IPCLabel's issueDate err");
		return false;
	}
	label.issueDate = o.get_int32();
	o = find_value(JsonIpclabel, "totalCount");
	if (o.isNull() || o.type() != UniValue::VNUM || o.get_int64() <= 0 || o.get_int64()>pow(10, 18))
	{
		strFailReason = _("The IPCLabel's totalCount err");
		return false;
	}
	label.totalCount = o.get_int64()* pow(10, (int)label.accuracy);
	if (label.totalCount <= 0 || label.totalCount > pow(10, 18))
	{
		strFailReason = _("The IPCLabel's totalCount err");
		return false;
	}
	o = find_value(JsonIpclabel, "txLabel");
	if (o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The IPCLabel's txLabel err");
		return false;
	}
	std::string TxLabel = o.get_str();
	o = find_value(JsonIpclabel, "currentCount");
	if (o.isNull() || o.type() != UniValue::VARR){
		strFailReason = _("The IPCLabel's currentCount err");
		return false;
	}
	int64_t total = label.totalCount ;
	std::map<int, int> currentcountmap;
	for (unsigned int idx = 0; idx < o.size(); idx++) {
		UniValue pos = o[idx].get_obj();
		if (pos.exists("height") == 0){
			strFailReason = _("Invalid parameter, duplicated position:height");
			return false;
		}
		if (pos.exists("count") == 0){
			strFailReason = _("Invalid parameter, duplicated position:count");
			return false;
		}
		int64_t nAmount = TCoinsFromValue(find_value(pos, "count"), label.accuracy);
		if (nAmount <= 0){
			strFailReason = _("Invalid token amount for send");
			return false;
		}
		UniValue z = find_value(pos, "height");
		if (z.isNull() || z.type() != UniValue::VNUM){
			strFailReason = _("The IPCLabel's height err");
			return false;
		}

		total -= nAmount;
		if (currentcountmap.count(z.get_int64())!=0){
			strFailReason = _("The IPCLabel's height err");
			return false;
		}
		currentcountmap.insert(make_pair(z.get_int64(), nAmount));
	}
	if (o.size() != currentcountmap.size() && currentcountmap.size() < 1 || (total != 0 && label.addmode==0)){
		strFailReason = _("The IPCLabel's currentcount size err");
		return false;
	}
	CAmount nDefalutFee = 100 * COIN;
	nValue += nDefalutFee;

	{
		set<pair<const CWalletTx*, unsigned int> > setCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear(); 
				txNew.vout.clear();
				wtxNew.fFromMe = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				if (vecSend.size() != 1){
					strFailReason = _("The vecSend's size err");
					return false;
				}
					
				for (auto i = currentcountmap.begin(); i!=currentcountmap.end(); i++){
					AddTokenLabel labeltemp = label;
					labeltemp.currentCount = i->second;
					labeltemp.height = i->first;
					CTxOut txout(vecSend[0].nAmount, vecSend[0].scriptPubKey, labeltemp, TxLabel);
					txNew.vout.push_back(txout);
				}

				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind;
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}

				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}

		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
		}

		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}
//To create a token trade; Tokensymbol: token symbol; TokenValue: token trading details; VecSend: recipient information
bool CWallet::CreateTokenTransaction(std::string& tokensymbol, uint64_t TokenValue, const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
	std::string& strFailReason, const CCoinControl *coinControl, bool sign)
{
	nFeeRet = 0;
	CAmount nValue = 0;
	uint64_t  curTokenBalance = 0;

	GetSymbolbalance(tokensymbol, curTokenBalance);

	if (TokenValue > curTokenBalance)
	{
		strFailReason = _("The Tokenvalue is too big,you have not enough Tokencoins.");
		return false;
	}

	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
		break;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	std::string txLabel = "";
	TokenLabel tokenlabel;
	memcpy((char*)(tokenlabel.TokenSymbol), (char*)(tokensymbol.c_str()), sizeof(tokenlabel.TokenSymbol));
	tokenlabel.value = TokenValue;
	if (tokenDataMap.count(tokenlabel.getTokenSymbol()) == 0)
	{
		strFailReason = _("Can't found 'accuracy' of the Token");
		return false;
	}
	tokenlabel.accuracy = tokenDataMap[tokenlabel.getTokenSymbol()].getAccuracy();
	{

		set<pair<const CWalletTx*, unsigned int> > setCoins;
		set<pair<const CWalletTx*, unsigned int> > setTokenCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);
			std::vector<COutput> vAvailableTokenCoins;
			AvailableTokenCoins(vAvailableTokenCoins, true, coinControl);

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					CTxOut txout(recipient.nAmount, recipient.scriptPubKey, tokenlabel,txLabel);
					txNew.vout.push_back(txout);
					break;										
				}

				// Choose coins to use  
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind;  
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}
				//Join an input
				//Find the appropriate utxo input in the current sybmol balance
				int icurinsize = txNew.vin.size();
				setTokenCoins.clear();
				if (!SelectTokenCoins(vAvailableTokenCoins, setTokenCoins, tokensymbol, TokenValue))
				{
					strFailReason = _("Can't select enough TokenCoins!");
					return false;
				}
				bool getVinscriptPubKey = false;
				uint64_t TotalvalueVin = 0;
				CScript newscriptPubKey;
				for (const auto& coin : setTokenCoins)
				{
					if (coin.first->tx->vout[coin.second].txType == 4)
						TotalvalueVin += coin.first->tx->vout[coin.second].tokenRegLabel.totalCount;
					else if (coin.first->tx->vout[coin.second].txType == TXOUT_ADDTOKEN)
						TotalvalueVin += coin.first->tx->vout[coin.second].addTokenLabel.currentCount;
					else
						TotalvalueVin += coin.first->tx->vout[coin.second].tokenLabel.value;
						
					if (!getVinscriptPubKey)
					{
						newscriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
						getVinscriptPubKey = true;
					}
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
						std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));
					
				}	
				if (!DummySignTx(txNew, setTokenCoins, icurinsize)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}
				
				//After finding enough input, you need to compute the add token change
				TokenLabel stokenlabel;
				for (const auto& recipient : vecSend)
				{
					memcpy((char*)(stokenlabel.TokenSymbol), (char*)(tokensymbol.c_str()), sizeof(stokenlabel.TokenSymbol));
					stokenlabel.value = TotalvalueVin - TokenValue;
					stokenlabel.accuracy = tokenlabel.accuracy;

					if (stokenlabel.value > 0)
					{
						CTxOut txsout(recipient.nAmount, newscriptPubKey, stokenlabel, txLabel);
						int  ntokenchangepos = txNew.vout.size() - 1;
						vector<CTxOut>::iterator tposition = txNew.vout.begin() + ntokenchangepos;
						if (ntokenchangepos > 0)
							txNew.vout.insert(tposition, txsout);
						else
							txNew.vout.push_back(txsout);
					}
					break;
				}
			
				
				unsigned int nBytes = GetVirtualTransactionSize(txNew);
				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}
		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
			//The added Token type input also needs to be signed
			for (const auto& coin : setTokenCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
		}
		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}

bool  CWallet::CreateTokenTransactionM(std::string& tokensymbol, const std::vector<CRecipientToken>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
	std::string& strFailReason, const CCoinControl *coinControl, bool sign)
{
	nFeeRet = 0;
	CAmount nValue = 0;
	uint64_t  curTokenBalance = 0;
	uint64_t TokenValue = 0;
	GetSymbolbalance(tokensymbol, curTokenBalance);

	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0 || TokenValue < 0 ||recipient.nvalue < 0)
		{
			strFailReason = _("Transaction amounts/tokenvalue must not be negative");
			return false;
		}
		nValue += recipient.nAmount;
		TokenValue += recipient.nvalue;
		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
	
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}
	if (TokenValue > curTokenBalance)
	{
		strFailReason = _("The Tokenvalue is too big,you have not enough Tokencoins.");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	std::string txLabel = "";
	TokenLabel tokenlabel;
	memcpy((char*)(tokenlabel.TokenSymbol), (char*)(tokensymbol.c_str()), sizeof(tokenlabel.TokenSymbol));
	tokenlabel.value = 0;
	if (tokenDataMap.count(tokenlabel.getTokenSymbol()) == 0)
	{
		strFailReason = _("Can't found 'accuracy' of the Token");
		return false;
	}
	tokenlabel.accuracy = tokenDataMap[tokenlabel.getTokenSymbol()].getAccuracy();
	{

		set<pair<const CWalletTx*, unsigned int> > setCoins;
		set<pair<const CWalletTx*, unsigned int> > setTokenCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);
			std::vector<COutput> vAvailableTokenCoins;
			AvailableTokenCoins(vAvailableTokenCoins, true, coinControl);

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					TokenLabel nowtokenlabel = tokenlabel;
					nowtokenlabel.value = recipient.nvalue;
					CTxOut txout(recipient.nAmount, recipient.scriptPubKey, nowtokenlabel, txLabel);
					txNew.vout.push_back(txout);
				}
	
				// Choose coins to use  
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind;
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}
				//Join an input
				//Find the appropriate utxo input in the current sybmol balance
				int icurinsize = txNew.vin.size();
				setTokenCoins.clear();
				if (!SelectTokenCoins(vAvailableTokenCoins, setTokenCoins, tokensymbol, TokenValue))
				{
					strFailReason = _("Can't select enough TokenCoins!");
					return false;
				}
				bool getVinscriptPubKey = false;
				uint64_t TotalvalueVin = 0;
				CScript newscriptPubKey;
				for (const auto& coin : setTokenCoins)
				{
					if (coin.first->tx->vout[coin.second].txType == 4)
						TotalvalueVin += coin.first->tx->vout[coin.second].tokenRegLabel.totalCount;
					else if (coin.first->tx->vout[coin.second].txType == TXOUT_ADDTOKEN)
						TotalvalueVin += coin.first->tx->vout[coin.second].addTokenLabel.currentCount;
					else
						TotalvalueVin += coin.first->tx->vout[coin.second].tokenLabel.value;

					if (!getVinscriptPubKey)
					{
						newscriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
						getVinscriptPubKey = true;
					}
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
						std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				}
				if (!DummySignTx(txNew, setTokenCoins, icurinsize)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}

				//After finding enough input, you need to compute the add token change
				TokenLabel stokenlabel;
				
				{
					memcpy((char*)(stokenlabel.TokenSymbol), (char*)(tokensymbol.c_str()), sizeof(stokenlabel.TokenSymbol));
					stokenlabel.value = TotalvalueVin - TokenValue;
					stokenlabel.accuracy = tokenlabel.accuracy;

					if (stokenlabel.value > 0)
					{
						CAmount nstokenamount = 0;
						CTxOut txsout(nstokenamount, newscriptPubKey, stokenlabel, txLabel);
						int  ntokenchangepos = txNew.vout.size() - 1;
						vector<CTxOut>::iterator tposition = txNew.vout.begin() + ntokenchangepos;
						if (ntokenchangepos > 0)
							txNew.vout.insert(tposition, txsout);
						else
							txNew.vout.push_back(txsout);
					}
					
				}


				unsigned int nBytes = GetVirtualTransactionSize(txNew);
				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}
		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
			//The added Token type input also needs to be signed
			for (const auto& coin : setTokenCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
		}
		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}
int64_t getTokeNmunberFromCOutput(const COutput &out)
{
    int txtype = out.tx->tx->vout[out.i].txType;
    std::string symbol;
    if (txtype == 4)
    {
        if (symbol == out.tx->tx->vout[out.i].tokenRegLabel.getTokenSymbol())
            return 0;
        else
            return out.tx->tx->vout[out.i].tokenRegLabel.totalCount;
    }
	if (txtype == TXOUT_ADDTOKEN)
	{
		if (symbol == out.tx->tx->vout[out.i].addTokenLabel.getTokenSymbol())
			return 0;
		else
			return out.tx->tx->vout[out.i].addTokenLabel.currentCount;
	}
    if (txtype == 5)
    {
        if (symbol == out.tx->tx->vout[out.i].tokenLabel.getTokenSymbol())
            return 0;
        else
            return  out.tx->tx->vout[out.i].tokenLabel.value;
    }
	if (txtype == TXOUT_ADDTOKEN)
	{
		if (symbol == out.tx->tx->vout[out.i].addTokenLabel.getTokenSymbol())
			return 0;
		else
			return out.tx->tx->vout[out.i].addTokenLabel.currentCount;
	}
}

bool uniontokencoinscomp(const COutput &outa, const COutput &outb){
     return getTokeNmunberFromCOutput(outa) > getTokeNmunberFromCOutput(outb);
 }
bool  CWallet::UnionCreateTokenTransactionM(std::string unionaddress,std::string& tokensymbol, const std::vector<CRecipientToken>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,\
    std::string& strFailReason,std::string& strtx, const CCoinControl *coinControl, bool sign,\
    int maxvinsize,int minvinsize,int minconfirmation)
{
    nFeeRet = 0;
    CAmount nValue = 0;
    uint64_t  curTokenBalance = 0;
    uint64_t TokenValue = 0;
    GetSymbolbalance(tokensymbol, curTokenBalance,unionaddress);

    int nChangePosRequest = nChangePosInOut;
    unsigned int nSubtractFeeFromAmount = 0;
    for (const auto& recipient : vecSend)
    {
        if (nValue < 0 || recipient.nAmount < 0 || TokenValue < 0 ||recipient.nvalue < 0)
        {
            strFailReason = _("Transaction amounts/tokenvalue must not be negative");
            return false;
        }
        nValue += recipient.nAmount;
        TokenValue += recipient.nvalue;
        if (recipient.fSubtractFeeFromAmount)
            nSubtractFeeFromAmount++;

    }
    if (vecSend.empty())
    {
        strFailReason = _("Transaction must have at least one recipient");
        return false;
    }
    if (TokenValue > curTokenBalance)
    {
        strFailReason = _("The Tokenvalue is too big,you have not enough Tokencoins.");
        return false;
    }

    wtxNew.fTimeReceivedIsTxTime = true;
    wtxNew.BindWallet(this);
    CMutableTransaction txNew;

    // Discourage fee sniping.
    //
    // For a large miner the value of the transactions in the best block and
    // the mempool can exceed the cost of deliberately attempting to mine two
    // blocks to orphan the current best block. By setting nLockTime such that
    // only the next block can include the transaction, we discourage this
    // practice as the height restricted and limited blocksize gives miners
    // considering fee sniping fewer options for pulling off this attack.
    //
    // A simple way to think about this is from the wallet's point of view we
    // always want the blockchain to move forward. By setting nLockTime this
    // way we're basically making the statement that we only want this
    // transaction to appear in the next block; we don't want to potentially
    // encourage reorgs by allowing transactions to appear at lower heights
    // than the next block in forks of the best chain.
    //
    // Of course, the subsidy is high enough, and transaction volume low
    // enough, that fee sniping isn't a problem yet, but by implementing a fix
    // now we ensure code won't be written that makes assumptions about
    // nLockTime that preclude a fix later.
    txNew.nLockTime = chainActive.Height();

    // Secondly occasionally randomly pick a nLockTime even further back, so
    // that transactions that are delayed after signing for whatever reason,
    // e.g. high-latency mix networks and some CoinJoin implementations, have
    // better privacy.
    if (GetRandInt(10) == 0)
        txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

    assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
    assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
    payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
    std::string txLabel = "";
    TokenLabel tokenlabel;
    memcpy((char*)(tokenlabel.TokenSymbol), (char*)(tokensymbol.c_str()), sizeof(tokenlabel.TokenSymbol));
    tokenlabel.value = 0;
    if (tokenDataMap.count(tokenlabel.getTokenSymbol()) == 0)
    {
        strFailReason = _("Can't found 'accuracy' of the Token");
        return false;
    }
	tokenlabel.accuracy = tokenDataMap[tokenlabel.getTokenSymbol()].getAccuracy();
    {

        set<pair<const CWalletTx*, unsigned int> > setCoins;
        set<pair<const CWalletTx*, unsigned int> > setTokenCoins;
        LOCK2(cs_main, cs_wallet);
        {
            std::vector<COutput> vAvailableCoins;
            AvailableUnionCoinsCOutput(unionaddress,vAvailableCoins, true, coinControl,false);
            std::vector<COutput> vAvailableTokenCoins;
            AvailableUnionCoinsCOutput(unionaddress,vAvailableTokenCoins, true, coinControl,true,true);
            LogPrintf("vAvailableTokenCoins num1:%d \n",vAvailableTokenCoins.size());
            auto iter = vAvailableTokenCoins.begin();
               while (iter != vAvailableTokenCoins.end()) {
                   std::string tempname = iter->tx->tx->vout[iter->i].getTokenSymbol();
                   LogPrintf("tempname:%s tokensymbol:%s\n",tempname,tokensymbol );
                   if(tokensymbol != tempname){
                       iter = vAvailableTokenCoins.erase(iter);
                   }
                   else if (minconfirmation>8&&iter->tx->GetDepthInMainChain() < minconfirmation) {
                       iter = vAvailableTokenCoins.erase(iter);
                   }
                   else {
                       ++iter;
                   }
               }
            sort(vAvailableTokenCoins.begin(),vAvailableTokenCoins.end(),uniontokencoinscomp);
            LogPrintf("vAvailableTokenCoins num2:%d \n",vAvailableTokenCoins.size());
            nFeeRet = 0;
            // Start with no fee and loop until there is enough fee
            while (true)
            {
                nChangePosInOut = nChangePosRequest;
                txNew.vin.clear();
                txNew.vout.clear();
                wtxNew.fFromMe = true;

                CAmount nValueToSelect = nValue;
                if (nSubtractFeeFromAmount == 0)
                    nValueToSelect += nFeeRet;
                double dPriority = 0;
                // vouts to the payees
                for (const auto& recipient : vecSend)
                {
                    TokenLabel nowtokenlabel = tokenlabel;
                    nowtokenlabel.value = recipient.nvalue;
                    CTxOut txout(recipient.nAmount, recipient.scriptPubKey, nowtokenlabel, recipient.crosslabel);
                    txNew.vout.push_back(txout);
                }

                // Choose coins to use
                CAmount nValueIn = 0;
                setCoins.clear();
                if (!SelectUnionCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
                {
                    strFailReason = _("Insufficient funds");
                    return false;
                }
                bool bFindChangScp = false;
                CScript scriptChangeFind;
                for (const auto& pcoin : setCoins)
                {
                    if (!bFindChangScp)
                    {
                        scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
                        bFindChangScp = true;
                    }
                    CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
                    //The coin age after the next block (depth+1) is used instead of the current,
                    //reflecting an assumption the user would accept a bit more delay for
                    //a chance at a free transaction.
                    //But mempool inputs might still be in the mempool, so their age stays 0
                    int age = pcoin.first->GetDepthInMainChain();
                    assert(age >= 0);
                    if (age != 0)
                        age += 1;
                    dPriority += (double)nCredit * age;
                }

                const CAmount nChange = nValueIn - nValueToSelect;
                if (nChange > 0)
                {
                    // Fill a vout to ourself
                    // TODO: pass in scriptChange instead of reservekey so
                    // change transaction isn't always pay-to-ipchain-address
                    CScript scriptChange;

                    // coin control: send change to custom address
                    if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
                        scriptChange = GetScriptForDestination(coinControl->destChange);

                    // no coin control: send change to newly generated address
                    else
                    {
                        // Note: We use a new key here to keep it from being obvious which side is the change.
                        //  The drawback is that by not reusing a previous key, the change may be lost if a
                        //  backup is restored, if the backup doesn't have the new private key for the change.
                        //  If we reused the old key, it would be possible to add code to look for and
                        //  rediscover unknown transactions that were written with keys of ours to recover
                        //  post-backup change.

                        // Reserve a new key pair from key pool
                        CPubKey vchPubKey;
                        bool ret;
                        ret = reservekey.GetReservedKey(vchPubKey);
                        if (!ret)
                        {
                            strFailReason = _("Keypool ran out, please call keypoolrefill first");
                            return false;
                        }

                        scriptChange = GetScriptForDestination(vchPubKey.GetID());
                    }

                    // Take the first input script as the destination output used for the coin change address
                    if (bFindChangScp)
                        scriptChange = scriptChangeFind;
                    CTxOut newTxOut(nChange, scriptChange);

                    // We do not move dust-change to fees, because the sender would end up paying more than requested.
                    // This would be against the purpose of the all-inclusive feature.
                    // So instead we raise the change and deduct from the recipient.
                    if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
                    {
                        CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
                        newTxOut.nValue += nDust; // raise change until no more dust
                        for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
                        {
                            if (vecSend[i].fSubtractFeeFromAmount)
                            {
                                txNew.vout[i].nValue -= nDust;
                                if (txNew.vout[i].IsDust(dustRelayFee))
                                {
                                    strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                                    return false;
                                }
                                break;
                            }
                        }
                    }

                    // Never create dust outputs; if we would, just
                    // add the dust to the fee.
                    if (newTxOut.IsDust(dustRelayFee))
                    {
                        nChangePosInOut = -1;
                        nFeeRet += nChange;
                        reservekey.ReturnKey();
                    }
                    else
                    {
                        if (nChangePosInOut == -1)
                        {
                            // Insert change txn at end position:
                            nChangePosInOut = txNew.vout.size();
                        }
                        else if ((unsigned int)nChangePosInOut > txNew.vout.size())
                        {
                            strFailReason = _("Change index out of range");
                            return false;
                        }

                        vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
                        txNew.vout.insert(position, newTxOut);
                    }
                }
                else
                    reservekey.ReturnKey();

                // Fill vin
                //
                // Note how the sequence number is set to non-maxint so that
                // the nLockTime set above actually works.
                //
                // BIP125 defines opt-in RBF as any nSequence < maxint-1, so
                // we use the highest possible value in that range (maxint-2)
                // to avoid conflicting with other possible uses of nSequence,
                // and in the spirit of "smallest possible change from prior
                // behavior."
                for (const auto& coin : setCoins)
                    txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
                    std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

                // Fill in dummy signatures for fee calculation.
                //if (!DummySignTx(txNew, setCoins)) {
                //    strFailReason = _("Signing transaction failed");
                //    return false;
                //}
                //Join an input
                //Find the appropriate utxo input in the current sybmol balance
                int icurinsize = txNew.vin.size();
                setTokenCoins.clear();
                bool bSelectUnionToken = false;
                if(maxvinsize>0)
                    bSelectUnionToken = SelectUnionTokenCoinsFromLimit(vAvailableTokenCoins, setTokenCoins, tokensymbol, TokenValue,NULL,maxvinsize);
                else
                    bSelectUnionToken = SelectUnionTokenCoins(vAvailableTokenCoins, setTokenCoins, tokensymbol, TokenValue);
                if (!bSelectUnionToken)
                {
                    strFailReason = _("Can't select enough TokenCoins!");
                    return false;
                }
                LogPrintf("SelectUnionTokenCoinsFromLimit vAvailableTokenCoins:%d setTokenCoins:%d\n" ,vAvailableTokenCoins.size(),setTokenCoins.size());
                if(minvinsize>0&&setTokenCoins.size()<minvinsize)
                {
                    strFailReason = _("Can't select minvinsize TokenCoins!");
                    return false;
                }
                bool getVinscriptPubKey = false;
                uint64_t TotalvalueVin = 0;
                CScript newscriptPubKey;
                for (const auto& coin : setTokenCoins)
                {
                    if (coin.first->tx->vout[coin.second].txType == 4)
                        TotalvalueVin += coin.first->tx->vout[coin.second].tokenRegLabel.totalCount;
					else if (coin.first->tx->vout[coin.second].txType == TXOUT_ADDTOKEN)
						TotalvalueVin += coin.first->tx->vout[coin.second].addTokenLabel.currentCount;
                    else
                        TotalvalueVin += coin.first->tx->vout[coin.second].tokenLabel.value;

                    if (!getVinscriptPubKey)
                    {
                        newscriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
                        getVinscriptPubKey = true;
                    }
                    txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
                        std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

                }
               // if (!DummySignTx(txNew, setTokenCoins, icurinsize)) {
                //    strFailReason = _("Signing transaction failed");
               //     return false;
               // }

                //After finding enough input, you need to compute the add token change
                TokenLabel stokenlabel;

                {
                    memcpy((char*)(stokenlabel.TokenSymbol), (char*)(tokensymbol.c_str()), sizeof(stokenlabel.TokenSymbol));
                    stokenlabel.value = TotalvalueVin - TokenValue;
                    stokenlabel.accuracy = tokenlabel.accuracy;

                    if (stokenlabel.value > 0)
                    {
                        CAmount nstokenamount = 0;
                        CTxOut txsout(nstokenamount, newscriptPubKey, stokenlabel, txLabel);
                        int  ntokenchangepos = txNew.vout.size() - 1;
                        vector<CTxOut>::iterator tposition = txNew.vout.begin() + ntokenchangepos;
                        if (ntokenchangepos > 0)
                            txNew.vout.insert(tposition, txsout);
                        else
                            txNew.vout.push_back(txsout);
                    }

                }


                unsigned int nBytes = GetVirtualTransactionSize(txNew);

                int nmembers = 0;
                int nRequired = 0;
                std::string strmultiscript;
                if(GetAddressNmembers(unionaddress,nmembers,nRequired,strmultiscript)&&nmembers>0){
                    nBytes+=((SIGNATURE_SIZE*(nmembers-1)+235)*(setCoins.size()+setTokenCoins.size()));
                }else
                    nBytes+=((SIGNATURE_SIZE*(UNION_NUMBER-1)+235)*(setCoins.size()+setTokenCoins.size()));
                CTransaction txNewConst(txNew);
                dPriority = txNewConst.ComputePriority(dPriority, nBytes);

                // Remove scriptSigs to eliminate the fee calculation dummy signatures
                for (auto& vin : txNew.vin) {
                    vin.scriptSig = CScript();
                    vin.scriptWitness.SetNull();
                }

                // Allow to override the default confirmation target over the CoinControl instance
                int currentConfirmationTarget = nTxConfirmTarget;
                if (coinControl && coinControl->nConfirmTarget > 0)
                    currentConfirmationTarget = coinControl->nConfirmTarget;

                // Can we complete this as a free transaction?
                if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
                {
                    // Not enough fee: enough priority?
                    double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
                    // Require at least hard-coded AllowFree.
                    if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
                        break;
                }

                CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
                if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
                    nFeeNeeded = coinControl->nMinimumTotalFee;
                }
                if (coinControl && coinControl->fOverrideFeeRate)
                    nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

                // If we made it here and we aren't even able to meet the relay fee on the next pass, give up
                // because we must be at the maximum allowed fee.
                if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
                {
                    strFailReason = _("Transaction too large for fee policy");
                    return false;
                }

                if (nFeeRet >= nFeeNeeded) {
                    // Reduce fee to only the needed amount if we have change
                    // output to increase.  This prevents potential overpayment
                    // in fees if the coins selected to meet nFeeNeeded result
                    // in a transaction that requires less fee than the prior
                    // iteration.
                    // TODO: The case where nSubtractFeeFromAmount > 0 remains
                    // to be addressed because it requires returning the fee to
                    // the payees and not the change output.
                    // TODO: The case where there is no change output remains
                    // to be addressed so we avoid creating too small an output.
                    if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                        CAmount extraFeePaid = nFeeRet - nFeeNeeded;
                        vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
                        change_position->nValue += extraFeePaid;
                        nFeeRet -= extraFeePaid;
                    }
                    break; // Done, enough fee included.
                }

                // Try to reduce change to include necessary fee
                if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                    CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
                    vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
                    // Only reduce change if remaining amount is still a large enough output.
                    if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
                        change_position->nValue -= additionalFeeNeeded;
                        nFeeRet += additionalFeeNeeded;
                        break; // Done, able to increase fee from change
                    }
                }

                // Include more fee and try again.
                nFeeRet = nFeeNeeded;
                continue;
            }
        }
        if (sign)
        {
            CTransaction txNewConst(txNew);
            int nIn = 0;
            for (const auto& coin : setCoins)
            {
                const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
                SignatureData sigdata;

                ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata);
                UpdateTransaction(txNew, nIn, sigdata);

                nIn++;
            }
            //The added Token type input also needs to be signed
            for (const auto& coin : setTokenCoins)
            {
                const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
                SignatureData sigdata;

                ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata);
                UpdateTransaction(txNew, nIn, sigdata);


                nIn++;
            }
        }
        strtx = EncodeHexTx(txNew);
        // Embed the constructed transaction data in wtxNew.
        wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

        // Limit size
        if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
        {
            strFailReason = _("Transaction too large");
            return false;
        }
    }

    if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits
        LockPoints lp;
        CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
        CTxMemPool::setEntries setAncestors;
        size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
        size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
        size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
        size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
        std::string errString;
        if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
            strFailReason = _("Transaction has too long of a mempool chain");
            return false;
        }
    }
    return true;
}


//Submit a bid for the election
bool CWallet::CommitJoinCampaignTransaction(const CTxDestination &address, CWalletTx& wtxNew, CReserveKey& reservekey, CConnman* connman, CValidationState& state)
{
	{
		LOCK2(cs_main, cs_wallet);
		LogPrintf("CommitTransaction:\n%s", wtxNew.tx->ToString());


		if (fBroadcastTransactions)
		{
			// Broadcast
			if (!wtxNew.AcceptToMemoryPool(maxTxFee, state)) {
				LogPrintf("CommitTransaction(): Transaction cannot be broadcast immediately, %s\n", state.GetRejectReason());
				// TODO: if we expect the failure to be long term or permanent, instead delete wtx from the wallet and return failure.
				return false;
			}
			else {

				{
					// Take key pair from key pool so it won't be used again
					reservekey.KeepKey();

					// Add tx to wallet, because if it has change it's also ours,
					// otherwise just for transaction history.
					AddToWallet(wtxNew);

					// Notify that old coins are spent
					BOOST_FOREACH(const CTxIn& txin, wtxNew.tx->vin)
					{
						CWalletTx &coin = mapWallet[txin.prevout.hash];
						coin.BindWallet(this);
						NotifyTransactionChanged(this, coin.GetHash(), CT_UPDATED);
					}
				}

				// Track how many getdata requests our transaction gets
				mapRequestCount[wtxNew.GetHash()] = 0;

				wtxNew.RelayWalletTransaction(connman);
			}
		}
	}

	//After the transaction is successfully sent, the underlying system interface is invoked to set the public key
	CKeyID id = *get<CKeyID>(&address);
	std::string sethexstring = id.GetHex();
	int nRet = CDpocInfo::Instance().SetLocalAccount(sethexstring);
	if (0 == nRet)
	{
		CDpocInfo::Instance().setJoinCampaign(true);
		CDpocInfo::Instance().setLocalAccoutVar(sethexstring);
	}
	
	LogPrintf("[CWallet::CommitJoinCampaignTransaction] Sets the underlying public key return value %d\n", nRet);
	
	//Set the consensus status to be normal 0
	std::string strStatus("0");
	bool bRet = CDpocInfo::Instance().SetConsensusStatus(strStatus, sethexstring);
	LogPrintf("[CWallet::CommitJoinCampaignTransaction] Set the consensus return value %d\n", bRet);

	return true;
}

//Gets the current node bookkeeping reward
bool CWallet::GetCurrentRewards(std::vector<std::string>& timelist, std::vector<std::string>& valuelist)
{
	timelist.clear();
	valuelist.clear();
	std::vector<BlockInfo> listInfo;
	int nSum;
	bool ret = CDpocInfo::Instance().GetBlockInfo(listInfo, nSum);
	if (!ret) return ret;

	BOOST_FOREACH(const BlockInfo &info, listInfo)
	{
		timelist.push_back(info.strTime);
		valuelist.push_back(info.strFee);
	}
	return true;
}

//end 
/**
 * Call after CreateTransaction unless you want to abort
 */
bool CWallet::CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey, CConnman* connman, CValidationState& state)
{
    {
        LOCK2(cs_main, cs_wallet);
        LogPrintf("CommitTransaction: %s\n", wtxNew.GetHash().GetHex());


        if (fBroadcastTransactions)
        {
            // Broadcast
            if (!wtxNew.AcceptToMemoryPool(maxTxFee, state)) {
                LogPrintf("CommitTransaction(): Transaction cannot be broadcast immediately, %s\n", state.GetRejectReason());
                // TODO: if we expect the failure to be long term or permanent, instead delete wtx from the wallet and return failure.

				return false;
            } else {
					
					{
						// Take key pair from key pool so it won't be used again
						reservekey.KeepKey();

						// Add tx to wallet, because if it has change it's also ours,
						// otherwise just for transaction history.
						AddToWallet(wtxNew);

						// Notify that old coins are spent
						BOOST_FOREACH(const CTxIn& txin, wtxNew.tx->vin)
						{
							CWalletTx &coin = mapWallet[txin.prevout.hash];
							coin.BindWallet(this);
							NotifyTransactionChanged(this, coin.GetHash(), CT_UPDATED);
						}
					}

				// Track how many getdata requests our transaction gets
				mapRequestCount[wtxNew.GetHash()] = 0;

                wtxNew.RelayWalletTransaction(connman);
            }
        }
    }
    return true;
}

void CWallet::ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& entries) {
    CWalletDB walletdb(strWalletFile);
    return walletdb.ListAccountCreditDebit(strAccount, entries);
}

bool CWallet::AddAccountingEntry(const CAccountingEntry& acentry)
{
    CWalletDB walletdb(strWalletFile);

    return AddAccountingEntry(acentry, &walletdb);
}

bool CWallet::AddAccountingEntry(const CAccountingEntry& acentry, CWalletDB *pwalletdb)
{
    if (!pwalletdb->WriteAccountingEntry_Backend(acentry))
        return false;

    laccentries.push_back(acentry);
    CAccountingEntry & entry = laccentries.back();
    wtxOrdered.insert(make_pair(entry.nOrderPos, TxPair((CWalletTx*)0, &entry)));

    return true;
}

CAmount CWallet::GetRequiredFee(unsigned int nTxBytes)
{
    return std::max(minTxFee.GetFee(nTxBytes), ::minRelayTxFee.GetFee(nTxBytes));
}

CAmount CWallet::GetMinimumFee(unsigned int nTxBytes, unsigned int nConfirmTarget, const CTxMemPool& pool)
{
    // payTxFee is the user-set global for desired feerate
    return GetMinimumFee(nTxBytes, nConfirmTarget, pool, payTxFee.GetFee(nTxBytes));
}

CAmount CWallet::GetMinimumFee(unsigned int nTxBytes, unsigned int nConfirmTarget, const CTxMemPool& pool, CAmount targetFee)
{
    CAmount nFeeNeeded = targetFee;
    // User didn't set: use -txconfirmtarget to estimate...
    if (nFeeNeeded == 0) {
        int estimateFoundTarget = nConfirmTarget;
        nFeeNeeded = pool.estimateSmartFee(nConfirmTarget, &estimateFoundTarget).GetFee(nTxBytes);
        // ... unless we don't have enough mempool data for estimatefee, then use fallbackFee
        if (nFeeNeeded == 0)
            nFeeNeeded = fallbackFee.GetFee(nTxBytes);
    }
    // prevent user from paying a fee below minRelayTxFee or minTxFee
    nFeeNeeded = std::max(nFeeNeeded, GetRequiredFee(nTxBytes));
    // But always obey the maximum
    if (nFeeNeeded > maxTxFee)
        nFeeNeeded = maxTxFee;
    return nFeeNeeded;
}




DBErrors CWallet::LoadWallet(bool& fFirstRunRet)
{
    if (!fFileBacked)
        return DB_LOAD_OK;
    fFirstRunRet = false;
    DBErrors nLoadWalletRet = CWalletDB(strWalletFile,"cr+").LoadWallet(this);
    if (nLoadWalletRet == DB_NEED_REWRITE)
    {
        if (CDB::Rewrite(strWalletFile, "\x04pool"))
        {
            LOCK(cs_wallet);
            setKeyPool.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nLoadWalletRet != DB_LOAD_OK)
        return nLoadWalletRet;
    fFirstRunRet = !vchDefaultKey.IsValid();

    uiInterface.LoadWallet(this);

    return DB_LOAD_OK;
}

DBErrors CWallet::ZapSelectTx(vector<uint256>& vHashIn, vector<uint256>& vHashOut)
{
    if (!fFileBacked)
        return DB_LOAD_OK;
    DBErrors nZapSelectTxRet = CWalletDB(strWalletFile,"cr+").ZapSelectTx(this, vHashIn, vHashOut);
    if (nZapSelectTxRet == DB_NEED_REWRITE)
    {
        if (CDB::Rewrite(strWalletFile, "\x04pool"))
        {
            LOCK(cs_wallet);
            setKeyPool.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nZapSelectTxRet != DB_LOAD_OK)
        return nZapSelectTxRet;

    MarkDirty();

    return DB_LOAD_OK;

}

DBErrors CWallet::ZapWalletTx(std::vector<CWalletTx>& vWtx)
{
    if (!fFileBacked)
        return DB_LOAD_OK;
    DBErrors nZapWalletTxRet = CWalletDB(strWalletFile,"cr+").ZapWalletTx(this, vWtx);
    if (nZapWalletTxRet == DB_NEED_REWRITE)
    {
        if (CDB::Rewrite(strWalletFile, "\x04pool"))
        {
            LOCK(cs_wallet);
            setKeyPool.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nZapWalletTxRet != DB_LOAD_OK)
        return nZapWalletTxRet;

    return DB_LOAD_OK;
}

bool CWallet::FindAddressBook(const CTxDestination& address, std::string type)
{
    LOCK(cs_wallet);
    auto itor = mapAddressBook.find(address);
    if(itor != mapAddressBook.end())
    {
        if((itor->second).purpose == type)
        {
            return true;
        }
    }
    return false;
}

bool CWallet::SetAddressBook(const CTxDestination& address, const string& strName, const string& strPurpose)
{
    bool fUpdated = false;
    {
        LOCK(cs_wallet); // mapAddressBook
        std::map<CTxDestination, CAddressBookData>::iterator mi = mapAddressBook.find(address);
        fUpdated = mi != mapAddressBook.end();
        mapAddressBook[address].name = strName;
        if (!strPurpose.empty()) /* update purpose only if requested */
            mapAddressBook[address].purpose = strPurpose;
    }
    NotifyAddressBookChanged(this, address, strName, ::IsMine(*this, address) != ISMINE_NO,
                             strPurpose, (fUpdated ? CT_UPDATED : CT_NEW) );
    if (!fFileBacked)
        return false;
    if (!strPurpose.empty() && !CWalletDB(strWalletFile).WritePurpose(CBitcoinAddress(address).ToString(), strPurpose))
        return false;
    return CWalletDB(strWalletFile).WriteName(CBitcoinAddress(address).ToString(), strName);
}

bool CWallet::DelAddressBook(const CTxDestination& address)
{
    {
        LOCK(cs_wallet); // mapAddressBook

        if(fFileBacked)
        {
            // Delete destdata tuples associated with address
            std::string strAddress = CBitcoinAddress(address).ToString();
            BOOST_FOREACH(const PAIRTYPE(string, string) &item, mapAddressBook[address].destdata)
            {
                CWalletDB(strWalletFile).EraseDestData(strAddress, item.first);
            }
        }
        mapAddressBook.erase(address);
    }

    NotifyAddressBookChanged(this, address, "", ::IsMine(*this, address) != ISMINE_NO, "", CT_DELETED);

    if (!fFileBacked)
        return false;
    CWalletDB(strWalletFile).ErasePurpose(CBitcoinAddress(address).ToString());
    return CWalletDB(strWalletFile).EraseName(CBitcoinAddress(address).ToString());
}

bool CWallet::SetDefaultKey(const CPubKey &vchPubKey)
{
    if (fFileBacked)
    {
        if (!CWalletDB(strWalletFile).WriteDefaultKey(vchPubKey))
            return false;
    }
    vchDefaultKey = vchPubKey;
    return true;
}

/**
 * Mark old keypool keys as used,
 * and generate all new keys 
 */
bool CWallet::NewKeyPool()
{
    {
        LOCK(cs_wallet);
        CWalletDB walletdb(strWalletFile);
        BOOST_FOREACH(int64_t nIndex, setKeyPool)
            walletdb.ErasePool(nIndex);
        setKeyPool.clear();

        if (IsLocked())
            return false;

        int64_t nKeys = max(GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t)0);
        for (int i = 0; i < nKeys; i++)
        {
            int64_t nIndex = i+1;
            walletdb.WritePool(nIndex, CKeyPool(GenerateNewKey()));
            setKeyPool.insert(nIndex);
        }
        LogPrintf("CWallet::NewKeyPool wrote %d new keys\n", nKeys);
    }
    return true;
}

bool CWallet::TopUpKeyPool(unsigned int kpSize)
{
    {
        LOCK(cs_wallet);

        if (IsLocked())
            return false;

        CWalletDB walletdb(strWalletFile);

        // Top up key pool
        unsigned int nTargetSize;
        if (kpSize > 0)
            nTargetSize = kpSize;
        else
            nTargetSize = max(GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t) 0);

        while (setKeyPool.size() < (nTargetSize + 1))
        {
            int64_t nEnd = 1;
            if (!setKeyPool.empty())
                nEnd = *(--setKeyPool.end()) + 1;
            if (!walletdb.WritePool(nEnd, CKeyPool(GenerateNewKey())))
                throw runtime_error(std::string(__func__) + ": writing generated key failed");
            setKeyPool.insert(nEnd);
            LogPrintf("keypool added key %d, size=%u\n", nEnd, setKeyPool.size());
        }
    }
    return true;
}

void CWallet::ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool)
{
    nIndex = -1;
    keypool.vchPubKey = CPubKey();
    {
        LOCK(cs_wallet);

        if (!IsLocked())
            TopUpKeyPool();

        // Get the oldest key
        if(setKeyPool.empty())
            return;

        CWalletDB walletdb(strWalletFile);

        nIndex = *(setKeyPool.begin());
        setKeyPool.erase(setKeyPool.begin());
        if (!walletdb.ReadPool(nIndex, keypool))
            throw runtime_error(std::string(__func__) + ": read failed");
        if (!HaveKey(keypool.vchPubKey.GetID()))
            throw runtime_error(std::string(__func__) + ": unknown key in key pool");
        assert(keypool.vchPubKey.IsValid());
        LogPrintf("keypool reserve %d\n", nIndex);
    }
}

void CWallet::KeepKey(int64_t nIndex)
{
    // Remove from key pool
    if (fFileBacked)
    {
        CWalletDB walletdb(strWalletFile);
        walletdb.ErasePool(nIndex);
    }
    LogPrintf("keypool keep %d\n", nIndex);
}

void CWallet::ReturnKey(int64_t nIndex)
{
    // Return to key pool
    {
        LOCK(cs_wallet);
        setKeyPool.insert(nIndex);
    }
    LogPrintf("keypool return %d\n", nIndex);
}

bool CWallet::GetKeyFromPool(CPubKey& result)
{
    int64_t nIndex = 0;
    CKeyPool keypool;
    {
        LOCK(cs_wallet);
        ReserveKeyFromKeyPool(nIndex, keypool);
        if (nIndex == -1)
        {
            if (IsLocked()) return false;
            result = GenerateNewKey();
            return true;
        }
        KeepKey(nIndex);
        result = keypool.vchPubKey;
    }
    return true;
}

int64_t CWallet::GetOldestKeyPoolTime()
{
    LOCK(cs_wallet);

    // if the keypool is empty, return <NOW>
    if (setKeyPool.empty())
        return GetTime();

    // load oldest key from keypool, get time and return
    CKeyPool keypool;
    CWalletDB walletdb(strWalletFile);
    int64_t nIndex = *(setKeyPool.begin());
    if (!walletdb.ReadPool(nIndex, keypool))
        throw runtime_error(std::string(__func__) + ": read oldest key in keypool failed");
    assert(keypool.vchPubKey.IsValid());
    return keypool.nTime;
}

std::map<CTxDestination, CAmount> CWallet::GetAddressBalances()
{
    map<CTxDestination, CAmount> balances;

    {
        LOCK(cs_wallet);
        BOOST_FOREACH(PAIRTYPE(uint256, CWalletTx) walletEntry, mapWallet)
        {
            CWalletTx *pcoin = &walletEntry.second;

            if (!pcoin->IsTrusted())
                continue;

            if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < (pcoin->IsFromMe(ISMINE_ALL) ? 0 : 1))
                continue;

            for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++)
            {
                CTxDestination addr;
                if (!IsMine(pcoin->tx->vout[i]))
                    continue;
                if(!ExtractDestination(pcoin->tx->vout[i].scriptPubKey, addr))
                    continue;

                CAmount n = IsSpent(walletEntry.first, i) ? 0 : pcoin->tx->vout[i].nValue;

                if (!balances.count(addr))
                    balances[addr] = 0;
                balances[addr] += n;
            }
        }
    }

    return balances;
}

set< set<CTxDestination> > CWallet::GetAddressGroupings()
{
    AssertLockHeld(cs_wallet); // mapWallet
    set< set<CTxDestination> > groupings;
    set<CTxDestination> grouping;

    BOOST_FOREACH(PAIRTYPE(uint256, CWalletTx) walletEntry, mapWallet)
    {
        CWalletTx *pcoin = &walletEntry.second;

        if (pcoin->tx->vin.size() > 0)
        {
            bool any_mine = false;
            // group all input addresses with each other
            BOOST_FOREACH(CTxIn txin, pcoin->tx->vin)
            {
                CTxDestination address;
                if(!IsMine(txin)) /* If this input isn't mine, ignore it */
                    continue;
                if(!ExtractDestination(mapWallet[txin.prevout.hash].tx->vout[txin.prevout.n].scriptPubKey, address))
                    continue;
                grouping.insert(address);
                any_mine = true;
            }

            // group change with input addresses
            if (any_mine)
            {
               BOOST_FOREACH(CTxOut txout, pcoin->tx->vout)
                   if (IsChange(txout))
                   {
                       CTxDestination txoutAddr;
                       if(!ExtractDestination(txout.scriptPubKey, txoutAddr))
                           continue;
                       grouping.insert(txoutAddr);
                   }
            }
            if (grouping.size() > 0)
            {
                groupings.insert(grouping);
                grouping.clear();
            }
        }

        // group lone addrs by themselves
        for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++)
            if (IsMine(pcoin->tx->vout[i]))
            {
                CTxDestination address;
                if(!ExtractDestination(pcoin->tx->vout[i].scriptPubKey, address))
                    continue;
                grouping.insert(address);
                groupings.insert(grouping);
                grouping.clear();
            }
    }

    set< set<CTxDestination>* > uniqueGroupings; // a set of pointers to groups of addresses
    map< CTxDestination, set<CTxDestination>* > setmap;  // map addresses to the unique group containing it
    BOOST_FOREACH(set<CTxDestination> _grouping, groupings)
    {
        // make a set of all the groups hit by this new group
        set< set<CTxDestination>* > hits;
        map< CTxDestination, set<CTxDestination>* >::iterator it;
        BOOST_FOREACH(CTxDestination address, _grouping)
            if ((it = setmap.find(address)) != setmap.end())
                hits.insert((*it).second);

        // merge all hit groups into a new single group and delete old groups
        set<CTxDestination>* merged = new set<CTxDestination>(_grouping);
        BOOST_FOREACH(set<CTxDestination>* hit, hits)
        {
            merged->insert(hit->begin(), hit->end());
            uniqueGroupings.erase(hit);
            delete hit;
        }
        uniqueGroupings.insert(merged);

        // update setmap
        BOOST_FOREACH(CTxDestination element, *merged)
            setmap[element] = merged;
    }

    set< set<CTxDestination> > ret;
    BOOST_FOREACH(set<CTxDestination>* uniqueGrouping, uniqueGroupings)
    {
        ret.insert(*uniqueGrouping);
        delete uniqueGrouping;
    }

    return ret;
}

CAmount CWallet::GetAccountBalance(const std::string& strAccount, int nMinDepth, const isminefilter& filter)
{
    CWalletDB walletdb(strWalletFile);
    return GetAccountBalance(walletdb, strAccount, nMinDepth, filter);
}

CAmount CWallet::GetAccountBalance(CWalletDB& walletdb, const std::string& strAccount, int nMinDepth, const isminefilter& filter)
{
    CAmount nBalance = 0;

    // Tally wallet transactions
    for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
		if (!CheckIPCFinalTx(wtx) || wtx.GetBlocksToMaturity() > 0 || wtx.GetDepthInMainChain() < 0)
            continue;

        CAmount nReceived, nSent, nFee;
        wtx.GetAccountAmounts(strAccount, nReceived, nSent, nFee, filter);

        if (nReceived != 0 && wtx.GetDepthInMainChain() >= nMinDepth)
            nBalance += nReceived;
        nBalance -= nSent + nFee;
    }
    // Tally internal accounting entries
    nBalance += walletdb.GetAccountCreditDebit(strAccount);

    return nBalance;
}

std::set<CTxDestination> CWallet::GetAccountAddresses(const std::string& strAccount) const
{
    LOCK(cs_wallet);
    set<CTxDestination> result;
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, mapAddressBook)
    {
        const CTxDestination& address = item.first;
        const string& strName = item.second.name;
        if (strName == strAccount)
            result.insert(address);
    }
    return result;
}

bool CReserveKey::GetReservedKey(CPubKey& pubkey)
{
    if (nIndex == -1)
    {
        CKeyPool keypool;
        pwallet->ReserveKeyFromKeyPool(nIndex, keypool);
        if (nIndex != -1)
            vchPubKey = keypool.vchPubKey;
        else {
            return false;
        }
    }
    assert(vchPubKey.IsValid());
    pubkey = vchPubKey;
    return true;
}

void CReserveKey::KeepKey()
{
    if (nIndex != -1)
        pwallet->KeepKey(nIndex);
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CReserveKey::ReturnKey()
{
    if (nIndex != -1)
        pwallet->ReturnKey(nIndex);
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CWallet::GetAllReserveKeys(set<CKeyID>& setAddress) const
{
    setAddress.clear();

    CWalletDB walletdb(strWalletFile);

    LOCK2(cs_main, cs_wallet);
    BOOST_FOREACH(const int64_t& id, setKeyPool)
    {
        CKeyPool keypool;
        if (!walletdb.ReadPool(id, keypool))
            throw runtime_error(std::string(__func__) + ": read failed");
        assert(keypool.vchPubKey.IsValid());
        CKeyID keyID = keypool.vchPubKey.GetID();
        if (!HaveKey(keyID))
            throw runtime_error(std::string(__func__) + ": unknown key in key pool");
        setAddress.insert(keyID);
    }
}

void CWallet::UpdatedTransaction(const uint256 &hashTx)
{
    {
        LOCK(cs_wallet);
        // Only notify UI if this transaction is in this wallet
        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(hashTx);
        if (mi != mapWallet.end())
            NotifyTransactionChanged(this, hashTx, CT_UPDATED);
    }
}

void CWallet::GetScriptForMining(boost::shared_ptr<CReserveScript> &script)
{
    boost::shared_ptr<CReserveKey> rKey(new CReserveKey(this));
    CPubKey pubkey;
    if (!rKey->GetReservedKey(pubkey))
        return;

    script = rKey;
    script->reserveScript = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;
}

void CWallet::LockCoin(const COutPoint& output)
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    setLockedCoins.insert(output);
}

void CWallet::UnlockCoin(const COutPoint& output)
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    setLockedCoins.erase(output);
}

void CWallet::UnlockAllCoins()
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    setLockedCoins.clear();
}

bool CWallet::IsLockedCoin(uint256 hash, unsigned int n) const
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    COutPoint outpt(hash, n);

    return (setLockedCoins.count(outpt) > 0);
}

void CWallet::ListLockedCoins(std::vector<COutPoint>& vOutpts)
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    for (std::set<COutPoint>::iterator it = setLockedCoins.begin();
         it != setLockedCoins.end(); it++) {
        COutPoint outpt = (*it);
        vOutpts.push_back(outpt);
    }
}

/** @} */ // end of Actions

class CAffectedKeysVisitor : public boost::static_visitor<void> {
private:
    const CKeyStore &keystore;
    std::vector<CKeyID> &vKeys;

public:
    CAffectedKeysVisitor(const CKeyStore &keystoreIn, std::vector<CKeyID> &vKeysIn) : keystore(keystoreIn), vKeys(vKeysIn) {}

    void Process(const CScript &script) {
        txnouttype type;
        std::vector<CTxDestination> vDest;
        int nRequired;
        if (ExtractDestinations(script, type, vDest, nRequired)) {
            BOOST_FOREACH(const CTxDestination &dest, vDest)
                boost::apply_visitor(*this, dest);
        }
    }

    void operator()(const CKeyID &keyId) {
        if (keystore.HaveKey(keyId))
            vKeys.push_back(keyId);
    }

    void operator()(const CScriptID &scriptId) {
        CScript script;
        if (keystore.GetCScript(scriptId, script))
            Process(script);
    }

    void operator()(const CNoDestination &none) {}
};

void CWallet::GetKeyBirthTimes(std::map<CTxDestination, int64_t> &mapKeyBirth) const {
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    mapKeyBirth.clear();

    // get birth times for keys with metadata
    for (const auto& entry : mapKeyMetadata) {
        if (entry.second.nCreateTime) {
            mapKeyBirth[entry.first] = entry.second.nCreateTime;
        }
    }

    // map in which we'll infer heights of other keys
    CBlockIndex *pindexMax = chainActive[std::max(0, chainActive.Height() - 144)]; // the tip can be reorganized; use a 144-block safety margin
    std::map<CKeyID, CBlockIndex*> mapKeyFirstBlock;
    std::set<CKeyID> setKeys;
    GetKeys(setKeys);
    BOOST_FOREACH(const CKeyID &keyid, setKeys) {
        if (mapKeyBirth.count(keyid) == 0)
            mapKeyFirstBlock[keyid] = pindexMax;
    }
    setKeys.clear();

    // if there are no such keys, we're done
    if (mapKeyFirstBlock.empty())
        return;

    // find first block that affects those keys, if there are any left
    std::vector<CKeyID> vAffected;
    for (std::map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); it++) {
        // iterate over all wallet transactions...
        const CWalletTx &wtx = (*it).second;
        BlockMap::const_iterator blit = mapBlockIndex.find(wtx.hashBlock);
        if (blit != mapBlockIndex.end() && chainActive.Contains(blit->second)) {
            // ... which are already in a block
            int nHeight = blit->second->nHeight;
            BOOST_FOREACH(const CTxOut &txout, wtx.tx->vout) {
                // iterate over all their outputs
                CAffectedKeysVisitor(*this, vAffected).Process(txout.scriptPubKey);
                BOOST_FOREACH(const CKeyID &keyid, vAffected) {
                    // ... and all their affected keys
                    std::map<CKeyID, CBlockIndex*>::iterator rit = mapKeyFirstBlock.find(keyid);
                    if (rit != mapKeyFirstBlock.end() && nHeight < rit->second->nHeight)
                        rit->second = blit->second;
                }
                vAffected.clear();
            }
        }
    }

    // Extract block timestamps for those keys
    for (std::map<CKeyID, CBlockIndex*>::const_iterator it = mapKeyFirstBlock.begin(); it != mapKeyFirstBlock.end(); it++)
        mapKeyBirth[it->first] = it->second->GetBlockTime() - 7200; // block times can be 2h off
}

bool CWallet::AddDestData(const CTxDestination &dest, const std::string &key, const std::string &value)
{
    if (boost::get<CNoDestination>(&dest))
        return false;

    mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).WriteDestData(CBitcoinAddress(dest).ToString(), key, value);
}

bool CWallet::EraseDestData(const CTxDestination &dest, const std::string &key)
{
    if (!mapAddressBook[dest].destdata.erase(key))
        return false;
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).EraseDestData(CBitcoinAddress(dest).ToString(), key);
}

bool CWallet::LoadDestData(const CTxDestination &dest, const std::string &key, const std::string &value)
{
    mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
    return true;
}

bool CWallet::GetDestData(const CTxDestination &dest, const std::string &key, std::string *value) const
{
    std::map<CTxDestination, CAddressBookData>::const_iterator i = mapAddressBook.find(dest);
    if(i != mapAddressBook.end())
    {
        CAddressBookData::StringMap::const_iterator j = i->second.destdata.find(key);
        if(j != i->second.destdata.end())
        {
            if(value)
                *value = j->second;
            return true;
        }
    }
    return false;
}

std::string CWallet::GetWalletHelpString(bool showDebug)
{
    std::string strUsage = HelpMessageGroup(_("Wallet options:"));
    strUsage += HelpMessageOpt("-disablewallet", _("Do not load the wallet and disable wallet RPC calls"));
    strUsage += HelpMessageOpt("-keypool=<n>", strprintf(_("Set key pool size to <n> (default: %u)"), DEFAULT_KEYPOOL_SIZE));
    strUsage += HelpMessageOpt("-fallbackfee=<amt>", strprintf(_("A fee rate (in %s/kB) that will be used when fee estimation has insufficient data (default: %s)"),
                                                               CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)));
    strUsage += HelpMessageOpt("-mintxfee=<amt>", strprintf(_("Fees (in %s/kB) smaller than this are considered zero fee for transaction creation (default: %s)"),
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)));
    strUsage += HelpMessageOpt("-paytxfee=<amt>", strprintf(_("Fee (in %s/kB) to add to transactions you send (default: %s)"),
                                                            CURRENCY_UNIT, FormatMoney(payTxFee.GetFeePerK())));
    strUsage += HelpMessageOpt("-rescan", _("Rescan the block chain for missing wallet transactions on startup"));
    strUsage += HelpMessageOpt("-salvagewallet", _("Attempt to recover private keys from a corrupt wallet on startup"));
    if (showDebug)
        strUsage += HelpMessageOpt("-sendfreetransactions", strprintf(_("Send transactions as zero-fee transactions if possible (default: %u)"), DEFAULT_SEND_FREE_TRANSACTIONS));
    strUsage += HelpMessageOpt("-spendzeroconfchange", strprintf(_("Spend unconfirmed change when sending transactions (default: %u)"), DEFAULT_SPEND_ZEROCONF_CHANGE));
    strUsage += HelpMessageOpt("-txconfirmtarget=<n>", strprintf(_("If paytxfee is not set, include enough fee so transactions begin confirmation on average within n blocks (default: %u)"), DEFAULT_TX_CONFIRM_TARGET));
    strUsage += HelpMessageOpt("-usehd", _("Use hierarchical deterministic key generation (HD) after BIP32. Only has effect during wallet creation/first start") + " " + strprintf(_("(default: %u)"), DEFAULT_USE_HD_WALLET));
    strUsage += HelpMessageOpt("-walletrbf", strprintf(_("Send transactions with full-RBF opt-in enabled (default: %u)"), DEFAULT_WALLET_RBF));
    strUsage += HelpMessageOpt("-upgradewallet", _("Upgrade wallet to latest format on startup"));
    strUsage += HelpMessageOpt("-wallet=<file>", _("Specify wallet file (within data directory)") + " " + strprintf(_("(default: %s)"), DEFAULT_WALLET_DAT));
    strUsage += HelpMessageOpt("-walletbroadcast", _("Make the wallet broadcast transactions") + " " + strprintf(_("(default: %u)"), DEFAULT_WALLETBROADCAST));
    strUsage += HelpMessageOpt("-walletnotify=<cmd>", _("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)"));
    strUsage += HelpMessageOpt("-zapwallettxes=<mode>", _("Delete all wallet transactions and only recover those parts of the blockchain through -rescan on startup") +
                               " " + _("(1 = keep tx meta data e.g. account owner and payment request information, 2 = drop tx meta data)"));

    if (showDebug)
    {
        strUsage += HelpMessageGroup(_("Wallet debugging/testing options:"));

        strUsage += HelpMessageOpt("-dblogsize=<n>", strprintf("Flush wallet database activity from memory to disk log every <n> megabytes (default: %u)", DEFAULT_WALLET_DBLOGSIZE));
        strUsage += HelpMessageOpt("-flushwallet", strprintf("Run a thread to flush wallet periodically (default: %u)", DEFAULT_FLUSHWALLET));
        strUsage += HelpMessageOpt("-privdb", strprintf("Sets the DB_PRIVATE flag in the wallet db environment (default: %u)", DEFAULT_WALLET_PRIVDB));
        strUsage += HelpMessageOpt("-walletrejectlongchains", strprintf(_("Wallet will not create transactions that violate mempool chain limits (default: %u)"), DEFAULT_WALLET_REJECT_LONG_CHAINS));
    }

    return strUsage;
}

extern std::string DecodeDumpString(const std::string &str);
extern int64_t DecodeDumpTime(const std::string &str);

bool CWallet::LoadWalletFromFile(const std::string filepath){

	LOCK2(cs_main, pwalletMain->cs_wallet);

	if (pwalletMain->IsLocked())
	{
		printf("Error: Please enter the wallet passphrase with walletpassphrase first.");
		return false;
	}

	std::ifstream file;
	file.open(filepath, std::ios::in | std::ios::ate);
	if (!file.is_open())
	{
		printf("Cannot open wallet dump file");
		return false;
	}

	int64_t nTimeBegin = chainActive.Tip()->GetBlockTime();

	bool fGood = true;

	int64_t nFilesize = std::max((int64_t)1, (int64_t)file.tellg());
	file.seekg(0, file.beg);

	pwalletMain->ShowProgress(_("Importing..."), 0); // show progress dialog in GUI
	while (file.good()) {
		pwalletMain->ShowProgress("", std::max(1, std::min(99, (int)(((double)file.tellg() / (double)nFilesize) * 100))));
		std::string line;
		std::getline(file, line);
		if (line.empty() || line[0] == '#')
			continue;

		std::vector<std::string> vstr;
		boost::split(vstr, line, boost::is_any_of(" "));
		if (vstr.size() < 2)
			continue;
		CBitcoinSecret vchSecret;
		if (!vchSecret.SetString(vstr[0]))
			continue;
		CKey key = vchSecret.GetKey();
		CPubKey pubkey = key.GetPubKey();
		assert(key.VerifyPubKey(pubkey));
		CKeyID keyid = pubkey.GetID();
		if (pwalletMain->HaveKey(keyid)) {
			LogPrintf("Skipping import of %s (key already present)\n", CBitcoinAddress(keyid).ToString());
			continue;
		}
		int64_t nTime = DecodeDumpTime(vstr[1]);
		std::string strLabel;
		bool fLabel = true;
		for (unsigned int nStr = 2; nStr < vstr.size(); nStr++) {
			if (boost::algorithm::starts_with(vstr[nStr], "#"))
				break;
			if (vstr[nStr] == "change=1")
				fLabel = false;
			if (vstr[nStr] == "reserve=1")
				fLabel = false;
			if (boost::algorithm::starts_with(vstr[nStr], "label=")) {
				strLabel = DecodeDumpString(vstr[nStr].substr(6));
				fLabel = true;
			}
		}
		LogPrintf("Importing %s...\n", CBitcoinAddress(keyid).ToString());
		if (!pwalletMain->AddKeyPubKey(key, pubkey)) {
			fGood = false;
			continue;
		}
		pwalletMain->mapKeyMetadata[keyid].nCreateTime = nTime;
		if (fLabel)
			pwalletMain->SetAddressBook(keyid, strLabel, "receive");
		nTimeBegin = std::min(nTimeBegin, nTime);
	}
	file.close();
	pwalletMain->ShowProgress("", 100); // hide progress dialog in GUI

	CBlockIndex *pindex = chainActive.Tip();
	while (pindex && pindex->pprev && pindex->GetBlockTime() > nTimeBegin - 7200)
		pindex = pindex->pprev;

	pwalletMain->UpdateTimeFirstKey(nTimeBegin);

	LogPrintf("Rescanning last %i blocks\n", chainActive.Height() - pindex->nHeight + 1);
	pwalletMain->ScanForWalletTransactions(pindex);
	pwalletMain->MarkDirty();

	if (!fGood)
	{
		printf("Error adding some keys to wallet");
		return false;
	}

	return true;
}

extern std::string EncodeDumpString(const std::string &str);
extern std::string  EncodeDumpTime(int64_t nTime);
bool CWallet::ExportWalletToFile(const std::string filepath){

	LOCK2(cs_main, pwalletMain->cs_wallet);

//	EnsureWalletIsUnlocked();
	if (pwalletMain->IsLocked())
	{
		printf("Error: Please enter the wallet passphrase with walletpassphrase first.");
		return false;
	}
	ofstream file;
	file.open(filepath.c_str());
	if (!file.is_open())
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

	std::map<CTxDestination, int64_t> mapKeyBirth;
	std::set<CKeyID> setKeyPool;
	pwalletMain->GetKeyBirthTimes(mapKeyBirth);
	pwalletMain->GetAllReserveKeys(setKeyPool);

	// sort time/key pairs
	std::vector<std::pair<int64_t, CKeyID> > vKeyBirth;
	for (const auto& entry : mapKeyBirth) {
		if (const CKeyID* keyID = boost::get<CKeyID>(&entry.first)) { // set and test
			vKeyBirth.push_back(std::make_pair(entry.second, *keyID));
		}
	}
	mapKeyBirth.clear();
	std::sort(vKeyBirth.begin(), vKeyBirth.end());

	// produce output
	file << strprintf("# Wallet dump created by Ipchain %s\n", CLIENT_BUILD);
	file << strprintf("# * Created on %s\n", EncodeDumpTime(GetTime()));
	file << strprintf("# * Best block at time of backup was %i (%s),\n", chainActive.Height(), chainActive.Tip()->GetBlockHash().ToString());
	file << strprintf("#   mined on %s\n", EncodeDumpTime(chainActive.Tip()->GetBlockTime()));
	file << "\n";

	// add the base58check encoded extended master if the wallet uses HD 
	CKeyID masterKeyID = pwalletMain->GetHDChain().masterKeyID;
	if (!masterKeyID.IsNull())
	{
		CKey key;
		if (pwalletMain->GetKey(masterKeyID, key))
		{
			CExtKey masterKey;
			masterKey.SetMaster(key.begin(), key.size());

			CBitcoinExtKey b58extkey;
			b58extkey.SetKey(masterKey);

			file << "# extended private masterkey: " << b58extkey.ToString() << "\n\n";
		}
	}
	for (std::vector<std::pair<int64_t, CKeyID> >::const_iterator it = vKeyBirth.begin(); it != vKeyBirth.end(); it++) {
		const CKeyID &keyid = it->second;
		std::string strTime = EncodeDumpTime(it->first);
		std::string strAddr = CBitcoinAddress(keyid).ToString();
		CKey key;
		if (pwalletMain->GetKey(keyid, key)) {
			file << strprintf("%s %s ", CBitcoinSecret(key).ToString(), strTime);
			if (pwalletMain->mapAddressBook.count(keyid)) {
				file << strprintf("label=%s", EncodeDumpString(pwalletMain->mapAddressBook[keyid].name));
			}
			else if (keyid == masterKeyID) {
				file << "hdmaster=1";
			}
			else if (setKeyPool.count(keyid)) {
				file << "reserve=1";
			}
			else if (pwalletMain->mapKeyMetadata[keyid].hdKeypath == "m") {
				file << "inactivehdmaster=1";
			}
			else {
				file << "change=1";
			}
			file << strprintf(" # addr=%s%s\n", strAddr, (pwalletMain->mapKeyMetadata[keyid].hdKeypath.size() > 0 ? " hdkeypath=" + pwalletMain->mapKeyMetadata[keyid].hdKeypath : ""));
		}
	}
	file << "\n";
	file << "# End of dump\n";
	file.close();
	return true;
}

CWallet* CWallet::CreateWalletFromFile(const std::string walletFile)
{
    // needed to restore wallet transaction meta data after -zapwallettxes
    std::vector<CWalletTx> vWtx;

    if (GetBoolArg("-zapwallettxes", false)) {
        uiInterface.InitMessage(_("Zapping all transactions from wallet..."));

        CWallet *tempWallet = new CWallet(walletFile);
        DBErrors nZapWalletRet = tempWallet->ZapWalletTx(vWtx);
        if (nZapWalletRet != DB_LOAD_OK) {
            InitError(strprintf(_("Error loading %s: Wallet corrupted"), walletFile));
            return NULL;
        }

        delete tempWallet;
        tempWallet = NULL;
    }

    uiInterface.InitMessage(_("Loading wallet..."));

    int64_t nStart = GetTimeMillis();
    bool fFirstRun = true;
    CWallet *walletInstance = new CWallet(walletFile);
    DBErrors nLoadWalletRet = walletInstance->LoadWallet(fFirstRun);
    if (nLoadWalletRet != DB_LOAD_OK)
    {
        if (nLoadWalletRet == DB_CORRUPT) {
            InitError(strprintf(_("Error loading %s: Wallet corrupted"), walletFile));
            return NULL;
        }
        else if (nLoadWalletRet == DB_NONCRITICAL_ERROR)
        {
            InitWarning(strprintf(_("Error reading %s! All keys read correctly, but transaction data"
                                         " or address book entries might be missing or incorrect."),
                walletFile));
        }
        else if (nLoadWalletRet == DB_TOO_NEW) {
            InitError(strprintf(_("Error loading %s: Wallet requires newer version of %s"), walletFile, _(PACKAGE_NAME)));
            return NULL;
        }
        else if (nLoadWalletRet == DB_NEED_REWRITE)
        {
            InitError(strprintf(_("Wallet needed to be rewritten: restart %s to complete"), _(PACKAGE_NAME)));
            return NULL;
        }
        else {
            InitError(strprintf(_("Error loading %s"), walletFile));
            return NULL;
        }
    }

    if (GetBoolArg("-upgradewallet", fFirstRun))
    {
        int nMaxVersion = GetArg("-upgradewallet", 0);
        if (nMaxVersion == 0) // the -upgradewallet without argument case
        {
            LogPrintf("Performing wallet upgrade to %i\n", FEATURE_LATEST);
            nMaxVersion = CLIENT_VERSION;
            walletInstance->SetMinVersion(FEATURE_LATEST); // permanently upgrade the wallet immediately
        }
        else
            LogPrintf("Allowing wallet upgrade up to %i\n", nMaxVersion);
        if (nMaxVersion < walletInstance->GetVersion())
        {
            InitError(_("Cannot downgrade wallet"));
            return NULL;
        }
        walletInstance->SetMaxVersion(nMaxVersion);
    }

    if (fFirstRun)
    {
        // Create new keyUser and set as default key
        if (GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET) && !walletInstance->IsHDEnabled()) {
            // generate a new master key
            CPubKey masterPubKey = walletInstance->GenerateNewHDMasterKey();
            if (!walletInstance->SetHDMasterKey(masterPubKey))
                throw std::runtime_error(std::string(__func__) + ": Storing master key failed");
        }
        CPubKey newDefaultKey;
        if (walletInstance->GetKeyFromPool(newDefaultKey)) {
            walletInstance->SetDefaultKey(newDefaultKey);
            if (!walletInstance->SetAddressBook(walletInstance->vchDefaultKey.GetID(), "", "receive")) {
                InitError(_("Cannot write default address") += "\n");
                return NULL;
            }
        }

        walletInstance->SetBestChain(chainActive.GetLocator());
    }
    else if (IsArgSet("-usehd")) {
        bool useHD = GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET);
        if (walletInstance->IsHDEnabled() && !useHD) {
            InitError(strprintf(_("Error loading %s: You can't disable HD on a already existing HD wallet"), walletFile));
            return NULL;
        }
        if (!walletInstance->IsHDEnabled() && useHD) {
            InitError(strprintf(_("Error loading %s: You can't enable HD on a already existing non-HD wallet"), walletFile));
            return NULL;
        }
    }

    LogPrintf(" wallet      %15dms\n", GetTimeMillis() - nStart);

    RegisterValidationInterface(walletInstance);

    CBlockIndex *pindexRescan = chainActive.Tip();
    if (GetBoolArg("-rescan", false))
        pindexRescan = chainActive.Genesis();
    else
    {
        CWalletDB walletdb(walletFile);
        CBlockLocator locator;
        if (walletdb.ReadBestBlock(locator))
            pindexRescan = FindForkInGlobalIndex(chainActive, locator);
        else
            pindexRescan = chainActive.Genesis();
    }
    if (chainActive.Tip() && chainActive.Tip() != pindexRescan)
    {
        //We can't rescan beyond non-pruned blocks, stop and throw an error
        //this might happen if a user uses a old wallet within a pruned node
        // or if he ran -disablewallet for a longer time, then decided to re-enable
        if (fPruneMode)
        {
            CBlockIndex *block = chainActive.Tip();
            while (block && block->pprev && (block->pprev->nStatus()& BLOCK_HAVE_DATA) && block->pprev->nTx() > 0 && pindexRescan != block)
                block = block->pprev;

            if (pindexRescan != block) {
                InitError(_("Prune: last wallet synchronisation goes beyond pruned data. You need to -reindex (download the whole blockchain again in case of pruned node)"));
                return NULL;
            }
        }

        uiInterface.InitMessage(_("Rescanning..."));
        LogPrintf("Rescanning last %i blocks (from block %i)...\n", chainActive.Height() - pindexRescan->nHeight, pindexRescan->nHeight);
        nStart = GetTimeMillis();
        walletInstance->ScanForWalletTransactions(pindexRescan, true);
        LogPrintf(" rescan      %15dms\n", GetTimeMillis() - nStart);
        walletInstance->SetBestChain(chainActive.GetLocator());
        CWalletDB::IncrementUpdateCounter();

        // Restore wallet transaction metadata after -zapwallettxes=1
        if (GetBoolArg("-zapwallettxes", false) && GetArg("-zapwallettxes", "1") != "2")
        {
            CWalletDB walletdb(walletFile);

            BOOST_FOREACH(const CWalletTx& wtxOld, vWtx)
            {
                uint256 hash = wtxOld.GetHash();
                std::map<uint256, CWalletTx>::iterator mi = walletInstance->mapWallet.find(hash);
                if (mi != walletInstance->mapWallet.end())
                {
                    const CWalletTx* copyFrom = &wtxOld;
                    CWalletTx* copyTo = &mi->second;
                    copyTo->mapValue = copyFrom->mapValue;
                    copyTo->vOrderForm = copyFrom->vOrderForm;
                    copyTo->nTimeReceived = copyFrom->nTimeReceived;
                    copyTo->nTimeSmart = copyFrom->nTimeSmart;
                    copyTo->fFromMe = copyFrom->fFromMe;
                    copyTo->strFromAccount = copyFrom->strFromAccount;
                    copyTo->nOrderPos = copyFrom->nOrderPos;
                    walletdb.WriteTx(*copyTo);
                }
            }
        }
    }
    walletInstance->SetBroadcastTransactions(GetBoolArg("-walletbroadcast", DEFAULT_WALLETBROADCAST));

    {
        LOCK(walletInstance->cs_wallet);
        LogPrintf("setKeyPool.size() = %u\n",      walletInstance->GetKeyPoolSize());
        LogPrintf("mapWallet.size() = %u\n",       walletInstance->mapWallet.size());
        LogPrintf("mapAddressBook.size() = %u\n",  walletInstance->mapAddressBook.size());
    }

    return walletInstance;
}

bool CWallet::InitLoadWallet()
{
    if (GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        pwalletMain = NULL;
        LogPrintf("Wallet disabled!\n");
        return true;
    }

    std::string walletFile = GetArg("-wallet", DEFAULT_WALLET_DAT);

    CWallet * const pwallet = CreateWalletFromFile(walletFile);
    if (!pwallet) {
        return false;
    }
    pwalletMain = pwallet;

    return true;
}

std::atomic<bool> CWallet::fFlushThreadRunning(false);

void CWallet::postInitProcess(boost::thread_group& threadGroup)
{
    // Add wallet transactions that aren't already in a block to mempool
    // Do this here as mempool requires genesis block to be loaded
    ReacceptWalletTransactions();

    // Run a thread to flush wallet periodically
    if (!CWallet::fFlushThreadRunning.exchange(true)) {
        threadGroup.create_thread(ThreadFlushWalletDB);
    }
}

bool CWallet::ParameterInteraction()
{
    if (GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET))
        return true;

    if (GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY) && SoftSetBoolArg("-walletbroadcast", false)) {
        LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -walletbroadcast=0\n", __func__);
    }

    if (GetBoolArg("-salvagewallet", false) && SoftSetBoolArg("-rescan", true)) {
        // Rewrite just private keys: rescan to find transactions
        LogPrintf("%s: parameter interaction: -salvagewallet=1 -> setting -rescan=1\n", __func__);
    }

    // -zapwallettx implies a rescan
    if (GetBoolArg("-zapwallettxes", false) && SoftSetBoolArg("-rescan", true)) {
        LogPrintf("%s: parameter interaction: -zapwallettxes=<mode> -> setting -rescan=1\n", __func__);
    }

    if (GetBoolArg("-sysperms", false))
        return InitError("-sysperms is not allowed in combination with enabled wallet functionality");
    if (GetArg("-prune", 0) && GetBoolArg("-rescan", false))
        return InitError(_("Rescans are not possible in pruned mode. You will need to use -reindex which will download the whole blockchain again."));

    if (::minRelayTxFee.GetFeePerK() > HIGH_TX_FEE_PER_KB)
        InitWarning(AmountHighWarn("-minrelaytxfee") + " " +
                    _("The wallet will avoid paying less than the minimum relay fee."));

    if (IsArgSet("-mintxfee"))
    {
        CAmount n = 0;
        if (!ParseMoney(GetArg("-mintxfee", ""), n) || 0 == n)
            return InitError(AmountErrMsg("mintxfee", GetArg("-mintxfee", "")));
        if (n > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-mintxfee") + " " +
                        _("This is the minimum transaction fee you pay on every transaction."));
        CWallet::minTxFee = CFeeRate(n);
    }
    if (IsArgSet("-fallbackfee"))
    {
        CAmount nFeePerK = 0;
        if (!ParseMoney(GetArg("-fallbackfee", ""), nFeePerK))
            return InitError(strprintf(_("Invalid amount for -fallbackfee=<amount>: '%s'"), GetArg("-fallbackfee", "")));
        if (nFeePerK > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-fallbackfee") + " " +
                        _("This is the transaction fee you may pay when fee estimates are not available."));
        CWallet::fallbackFee = CFeeRate(nFeePerK);
    }
    if (IsArgSet("-paytxfee"))
    {
        CAmount nFeePerK = 0;
        if (!ParseMoney(GetArg("-paytxfee", ""), nFeePerK))
            return InitError(AmountErrMsg("paytxfee", GetArg("-paytxfee", "")));
        if (nFeePerK > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-paytxfee") + " " +
                        _("This is the transaction fee you will pay if you send a transaction."));

        payTxFee = CFeeRate(nFeePerK, 1000);
        if (payTxFee < ::minRelayTxFee)
        {
            return InitError(strprintf(_("Invalid amount for -paytxfee=<amount>: '%s' (must be at least %s)"),
                                       GetArg("-paytxfee", ""), ::minRelayTxFee.ToString()));
        }
    }
    if (IsArgSet("-maxtxfee"))
    {
        CAmount nMaxFee = 0;
        if (!ParseMoney(GetArg("-maxtxfee", ""), nMaxFee))
            return InitError(AmountErrMsg("maxtxfee", GetArg("-maxtxfee", "")));
        if (nMaxFee > HIGH_MAX_TX_FEE)
            InitWarning(_("-maxtxfee is set very high! Fees this large could be paid on a single transaction."));
        maxTxFee = nMaxFee;
        if (CFeeRate(maxTxFee, 1000) < ::minRelayTxFee)
        {
            return InitError(strprintf(_("Invalid amount for -maxtxfee=<amount>: '%s' (must be at least the minrelay fee of %s to prevent stuck transactions)"),
                                       GetArg("-maxtxfee", ""), ::minRelayTxFee.ToString()));
        }
    }
    nTxConfirmTarget = GetArg("-txconfirmtarget", DEFAULT_TX_CONFIRM_TARGET);
    bSpendZeroConfChange = GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);
    fSendFreeTransactions = GetBoolArg("-sendfreetransactions", DEFAULT_SEND_FREE_TRANSACTIONS);
    fWalletRbf = GetBoolArg("-walletrbf", DEFAULT_WALLET_RBF);

    if (fSendFreeTransactions && GetArg("-limitfreerelay", DEFAULT_LIMITFREERELAY) <= 0)
        return InitError("Creation of free transactions with their relay disabled is not supported.");

    return true;
}

bool CWallet::BackupWallet(const std::string& strDest)
{
    if (!fFileBacked)
        return false;
    while (true)
    {
        {
            LOCK(bitdb.cs_db);
            if (!bitdb.mapFileUseCount.count(strWalletFile) || bitdb.mapFileUseCount[strWalletFile] == 0)
            {
                // Flush log data to the dat file
                bitdb.CloseDb(strWalletFile);
                bitdb.CheckpointLSN(strWalletFile);
                bitdb.mapFileUseCount.erase(strWalletFile);

                // Copy wallet file
                boost::filesystem::path pathSrc = GetDataDir() / strWalletFile;
                boost::filesystem::path pathDest(strDest);
                if (boost::filesystem::is_directory(pathDest))
                    pathDest /= strWalletFile;

                try {
#if BOOST_VERSION >= 104000
                    boost::filesystem::copy_file(pathSrc, pathDest, boost::filesystem::copy_option::overwrite_if_exists);
#else
                    boost::filesystem::copy_file(pathSrc, pathDest);
#endif
                    LogPrintf("copied %s to %s\n", strWalletFile, pathDest.string());
                    return true;
                } catch (const boost::filesystem::filesystem_error& e) {
                    LogPrintf("error copying %s to %s - %s\n", strWalletFile, pathDest.string(), e.what());
                    return false;
                }
            }
        }
        MilliSleep(100);
    }
    return false;
}

bool CWallet::GetChangeIndex(const CTransaction& tx, int& index) const
{
	
	uint32_t nSize = tx.vout.size();
	if (nSize == 1 || tx.IsCoinBase())
	{
		index = -1;
		return true;
	}
	uint8_t txtp = tx.GetTxType();
	CTransactionRef prevTx;
	uint256 hashBlock;
	unsigned int preindex = 0;
	for (auto tvin:	tx.vin)
	{
		if (!GetTransaction(tvin.prevout.hash, prevTx, Params().GetConsensus(), hashBlock, true))
			return false;
		uint8_t ttype = prevTx->vout[tvin.prevout.n].txType;
		preindex = tvin.prevout.n;
		if (txtp == TXOUT_NORMAL)
			break;
		else if (txtp == TXOUT_TOKENREG || txtp == TXOUT_IPCOWNER)
		{
			index = -1;
			return true;
		}
		else if (txtp == TXOUT_IPCAUTHORIZATION && (ttype == TXOUT_IPCOWNER || ttype == TXOUT_IPCAUTHORIZATION))
			break;
		else if (txtp == TXOUT_TOKEN && (ttype == TXOUT_TOKEN || ttype == TXOUT_TOKENREG || ttype == TXOUT_ADDTOKEN))
			break;
		
	}
	
	const CTxOut& prev = prevTx->vout[preindex];
	for (int i = nSize-1; i >=0 ;i--)
	{
		if (tx.vout[i].txType != txtp)
			continue;
		if (tx.vout[i].scriptPubKey == prev.scriptPubKey)
		{
			index = i;
			return true;
		}
		else
		{
			index = -1;
			return false;
		}
	}
	return false;
}

bool CWallet::GetChangeIndexTokenUnion(const CTransaction& tx, int& index) const
{

    uint32_t nSize = tx.vout.size();
    if (nSize == 1 || tx.IsCoinBase())
    {
        index = -1;
        return true;
    }
    uint8_t txtp = tx.GetTxType();
    CTransactionRef prevTx;
    uint256 hashBlock;
    unsigned int preindex = 0;
    uint64_t tokentotal;
    for (auto tvin:	tx.vin)
    {
        if (!GetTransaction(tvin.prevout.hash, prevTx, Params().GetConsensus(), hashBlock, true))
            return false;
        uint8_t ttype = prevTx->vout[tvin.prevout.n].txType;
        preindex = tvin.prevout.n;
        if (txtp == TXOUT_NORMAL)
            break;
		else if (txtp == TXOUT_TOKENREG || txtp == TXOUT_IPCOWNER || txtp == TXOUT_ADDTOKEN )
        {
            index = -1;
            return true;
        }
        else if (txtp == TXOUT_IPCAUTHORIZATION && (ttype == TXOUT_IPCOWNER || ttype == TXOUT_IPCAUTHORIZATION))
            break;
        else if (txtp == TXOUT_TOKEN && (ttype == TXOUT_TOKEN || ttype == TXOUT_TOKENREG)){
            tokentotal += prevTx->vout[tvin.prevout.n].GetTokenvalue();
            break;
        }

    }
    const CTxOut& prev = prevTx->vout[preindex];
    for (int i = 0; i <=nSize-1 ;i++)
    {
        if (tx.vout[i].txType != txtp)
            continue;
        tokentotal-= tx.vout[i].GetTokenvalue();
        if (tokentotal<=0)
        {
            index = i;
            return true;
        }
        else
        {
            index = -1;
        }
    }
    return false;
}

uint64_t CWallet::GetDebitOfToken(const CTransaction& tx, const isminefilter& filter) const
{
	if (tx.vin.empty())
		return 0;
	uint64_t debit = 0;
	for (auto txvin : tx.vin)
	{

		{
			LOCK(cs_wallet);
			map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txvin.prevout.hash);
			if (mi != mapWallet.end())
			{
				const CWalletTx& prev = (*mi).second;
				if (txvin.prevout.n < prev.tx->vout.size())
                if (IsMine(prev.tx->vout[txvin.prevout.n]) & filter)
				{
					if (prev.tx->vout[txvin.prevout.n].txType == 4)
					{
						debit += prev.tx->vout[txvin.prevout.n].tokenRegLabel.totalCount;
						return debit;
					}
					else if (prev.tx->vout[txvin.prevout.n].txType == TXOUT_ADDTOKEN)
					{
						debit += prev.tx->vout[txvin.prevout.n].addTokenLabel.currentCount;
					}
					else if (prev.tx->vout[txvin.prevout.n].txType == 5)
						debit += prev.tx->vout[txvin.prevout.n].tokenLabel.value;
				}
			}
		}
	}
	return debit;
}

uint64_t CWallet::GetDebitOfTokenForUnion(const CTransaction& tx, const isminefilter& filter,std::vector<UnionAddressInfo>&unionaddresses) const
{
    if (tx.vin.empty())
        return 0;
    uint64_t debit = 0;
    for (auto txvin : tx.vin)
    {

        {
            LOCK(cs_wallet);
            map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txvin.prevout.hash);
            if (mi != mapWallet.end())
            {
                const CWalletTx& prev = (*mi).second;
                if (txvin.prevout.n >= prev.tx->vout.size()) continue;

                CTxDestination address;
                const CTxOut & txout = prev.tx->vout[txvin.prevout.n];
                if (!ExtractDestination(txout.scriptPubKey, address) && !txout.scriptPubKey.IsUnspendable())
                {
                    LogPrintf("CWalletTx::GetDebitOfTokenForUnion: Unknown transaction type found, txid %s\n",
                        tx.GetHash().ToString());
                    address = CNoDestination();
                    continue;
                }
                bool findaddress = false;
                std::string straddress = CBitcoinAddress(address).ToString();
                BOOST_FOREACH(UnionAddressInfo&info,unionaddresses){
                    if(info.address == straddress)
                        findaddress = true;
                }

                if (findaddress)
                {
                    if (prev.tx->vout[txvin.prevout.n].txType == 4)
                    {
                        debit += prev.tx->vout[txvin.prevout.n].tokenRegLabel.totalCount;
                        return debit;
                    }
					else  if (prev.tx->vout[txvin.prevout.n].txType == TXOUT_ADDTOKEN)
					{
						debit += prev.tx->vout[txvin.prevout.n].addTokenLabel.currentCount;
					}
                    else if (prev.tx->vout[txvin.prevout.n].txType == 5)
                        debit += prev.tx->vout[txvin.prevout.n].tokenLabel.value;
                }
            }
        }
    }
    return debit;
}

bool CWallet::CreateTokenTransactionForCross(std::string& tokensymbol, const std::string& strtxLabel, uint64_t TokenValue, const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut, std::string& strFailReason, const CCoinControl *coinControl /*= NULL*/, bool sign /*= true*/)
{
	nFeeRet = 0;
	CAmount nValue = 0;
	uint64_t  curTokenBalance = 0;

	GetSymbolbalance(tokensymbol, curTokenBalance);

	if (TokenValue > curTokenBalance)
	{
		strFailReason = _("The Tokenvalue is too big,you have not enough Tokencoins.");
		return false;
	}

	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	for (const auto& recipient : vecSend)
	{
		if (nValue < 0 || recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;

		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
		break;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	std::string txLabel = "";
	TokenLabel tokenlabel;
	memcpy((char*)(tokenlabel.TokenSymbol), (char*)(tokensymbol.c_str()), sizeof(tokenlabel.TokenSymbol));
	tokenlabel.value = TokenValue;
	if (tokenDataMap.count(tokenlabel.getTokenSymbol()) == 0)
	{
		strFailReason = _("Can't found 'accuracy' of the Token");
		return false;
	}
	tokenlabel.accuracy = tokenDataMap[tokenlabel.getTokenSymbol()].getAccuracy();
	{

		set<pair<const CWalletTx*, unsigned int> > setCoins;
		set<pair<const CWalletTx*, unsigned int> > setTokenCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);
			std::vector<COutput> vAvailableTokenCoins;
			AvailableTokenCoins(vAvailableTokenCoins, true, coinControl);

			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					CTxOut txout(recipient.nAmount, recipient.scriptPubKey, tokenlabel, strtxLabel);
					txNew.vout.push_back(txout);
					break;
				}

				// Choose coins to use  
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind;
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}
				//Join an input
				//Find the appropriate utxo input in the current sybmol balance
				int icurinsize = txNew.vin.size();
				setTokenCoins.clear();
				if (!SelectTokenCoins(vAvailableTokenCoins, setTokenCoins, tokensymbol, TokenValue))
				{
					strFailReason = _("Can't select enough TokenCoins!");
					return false;
				}

				bool getVinscriptPubKey = false;
				uint64_t TotalvalueVin = 0;
				CScript newscriptPubKey;
				for (const auto& coin : setTokenCoins)
				{
					if (coin.first->tx->vout[coin.second].txType == 4)
						TotalvalueVin += coin.first->tx->vout[coin.second].tokenRegLabel.totalCount;
					else if (coin.first->tx->vout[coin.second].txType == TXOUT_ADDTOKEN)
						TotalvalueVin += coin.first->tx->vout[coin.second].addTokenLabel.currentCount;
					else
						TotalvalueVin += coin.first->tx->vout[coin.second].tokenLabel.value;

					if (!getVinscriptPubKey)
					{
						newscriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
						getVinscriptPubKey = true;
					}
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
						std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				}
				if (!DummySignTx(txNew, setTokenCoins, icurinsize)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}

				//After finding enough input, you need to compute the add token change
				TokenLabel stokenlabel;
				for (const auto& recipient : vecSend)
				{
					memcpy((char*)(stokenlabel.TokenSymbol), (char*)(tokensymbol.c_str()), sizeof(stokenlabel.TokenSymbol));
					stokenlabel.value = TotalvalueVin - TokenValue;
					stokenlabel.accuracy = tokenlabel.accuracy;

					if (stokenlabel.value > 0)
					{
						CTxOut txsout(recipient.nAmount, newscriptPubKey, stokenlabel, txLabel);
						int  ntokenchangepos = txNew.vout.size() - 1;
						vector<CTxOut>::iterator tposition = txNew.vout.begin() + ntokenchangepos;
						if (ntokenchangepos > 0)
							txNew.vout.insert(tposition, txsout);
						else
							txNew.vout.push_back(txsout);
					}
					break;
				}


				unsigned int nBytes = GetVirtualTransactionSize(txNew);
				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}
		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
			//The added Token type input also needs to be signed
			for (const auto& coin : setTokenCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
		}
		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}


bool CWallet::CreateMultiAddress(unsigned int nRequired, const std::vector<std::string>& strPubkeys, std::string& multiscript, std::string& strmultiaddress, std::string& strFailReason)
{
    LogPrintf("CreateMultiAddress begin\n");
	unsigned int nPubkeys = strPubkeys.size();
    if (nRequired < 1 || nPubkeys < nRequired || nPubkeys > UNION_NUMBER)
	{
		strFailReason = _("nRequired or strPubkeys size is valid!");
        LogPrintf("CreateMultiAddress nRequired or strPubkeys size is valid!\n");
		return false;
	}
    stringstream stream;
    ptree pt;
    pt.put("nRequired",nRequired);
    ptree info;
    info.put("nPubkeys",nPubkeys);
    ptree members;
	std::vector<CPubKey> pubkeys;
	for (unsigned int i = 0; i < nPubkeys; i++)
	{
		const std::string& ks = strPubkeys[i];
		if (IsHex(ks))
		{
			CPubKey vchPubKey(ParseHex(ks));
            if (!vchPubKey.IsFullyValid()){
                strFailReason = _("Pubkey is valid!");
                LogPrintf("CreateMultiAddress Pubkey is valid! IsFullyValid false\n");
				return false;
            }
            pubkeys.push_back(vchPubKey);
            ptree member;
            member.put("member",ks);
            members.push_back(make_pair("",member));
		}
		else
		{
            strFailReason = _("Pubkey is not valid!");
            LogPrintf("CreateMultiAddress Pubkey is not valid!\n");
			return false;
		}
	}
	CScript result = GetScriptForMultisig(nRequired, pubkeys);
	if (result.size() > MAX_SCRIPT_ELEMENT_SIZE)
	{
		strFailReason = _("CScript size too large!");
        LogPrintf("CreateMultiAddress CScript size too large!\n");
		return false;
	}
    info.put_child("members",members);
    pt.push_back(make_pair("all_member",info));
    write_json(stream,pt);
    multiscript = EncodeBase64(stream.str());
    LogPrintf("CreateMultiAddress createstr; %s\n", multiscript);
	CScript inner = result;
	CScriptID innerID(inner);
	strmultiaddress = CBitcoinAddress(innerID).ToString();
    std::cout<<"strmultiaddress: "<<strmultiaddress<<std::endl;
    LogPrintf("CreateMultiAddress strmultiaddress; %s\n", strmultiaddress);
	return true;
}

int64_t GetCurrentStamp64()
{
  boost::posix_time::ptime epoch(boost::gregorian::date(1970, boost::gregorian::Jan, 1));
  boost::posix_time::time_duration time_from_epoch =
  boost::posix_time::second_clock::universal_time() - epoch;
  return time_from_epoch.total_seconds();
}


bool CWallet::AddMultiAddress( std::string strmultiscript, std::string& strFailReason,int &nRequired,\
                               int &nmembers, std::string strAccount,bool brpc,bool bRelatedToMe )
{
	bool bCanAdd = false;
    LogPrintf("AddMultiAddress1: %s\n", strmultiscript);
    std::string strmultiscripttemp = DecodeBase64(strmultiscript);
    nRequired = 0;
    std::vector<CPubKey> pubkeys;
    try
    {
        ptree pt;
        stringstream stream(strmultiscripttemp);
        read_json(stream,pt);
        nRequired = pt.get<int>("nRequired");
        ptree info = pt.get_child("all_member");
        ptree members = info.get_child("members");
        nmembers = members.size();
         BOOST_FOREACH(ptree::value_type &v,members)
        {
             std::string ks = v.second.get<string>("member");
             if (IsHex(ks))
             {
                 CPubKey vchPubKey(ParseHex(ks));
				 CKeyID vchAddress = vchPubKey.GetID();
				 if (pwalletMain->HaveKey(vchAddress))
					 bCanAdd = true;
                 pubkeys.push_back(vchPubKey);
             }
        }
    }
     catch(ptree_error pt)
    {
        strFailReason = pt.what();
        return false;
    }
    if (!bCanAdd&&bRelatedToMe)
	{
		strFailReason = _("MultiAdd is not yours!");
		return false;
	}
	
    CScript script = GetScriptForMultisig(nRequired, pubkeys);
	CScriptID innerID(script);
    CBitcoinAddress add(innerID);
    LogPrintf("AddMultiAddress nRequired:%d nmembers:%d\n", nRequired,nmembers);
    LogPrintf("AddMultiAddress address: %s\n", add.ToString());
    if(FindAddressBook(innerID, "union"))
    {
		strFailReason = _("Address duplication!");
       return false;
    }
	std::cout << "[AddMultiAddress] strAccount = " << strAccount <<std::endl;
    if (pwalletMain->AddCScript(script) && pwalletMain->SetAddressBook(innerID, strAccount, "union")){

        try
        {
            ptree pt;
            boost::filesystem::path pathDebug = GetDataDir() / "unionaddress.ini";


            FILE* m_file = fopen(pathDebug.string().c_str(), "a+");
            fclose(m_file);

            ini_parser::read_ini(pathDebug.string(), pt);
            std::string address = add.ToString();
            pt.put<std::string>(address+".strmultiscript",strmultiscript);
            pt.put<int>(address+".nRequired",nRequired);
            pt.put<int>(address+".nmembers",nmembers);
            pt.put<int>(address+".time", GetCurrentStamp64());
            boost::property_tree::ini_parser::write_ini(pathDebug.string(),pt);
        }
        catch(std::exception e)
        {
            LogPrintf("AddMultiAddress %s\n",e.what());
            return false;
        }
        if(brpc){
            ScanForWalletTransactions(chainActive.Genesis(), true,false);
        }else {
             boost::function<void (CBlockIndex*,bool,bool)> memberFunctionWrapper( \
                    boost::bind(&CWallet::ScanForWalletTransactions, this, _1,_2,_3));

             boost::thread thrd( boost::bind(memberFunctionWrapper,\
                                        chainActive.Genesis(),true ,true) ) ;
             thrd.detach();
        }


        return true;
    }
	else{
		strFailReason = _("AddMultiAddress failed!");
		return false;
	}
}

bool CWallet::getunionaddressfrominvcode( std::string strmultiscript, std::string& strFailReason,int &nRequired,int &nmembers, std::string &unionaddress )
{
    bool bCanAdd = false;
    LogPrintf("getunionaddressfrominvcode: %s\n", strmultiscript);
    std::string strmultiscripttemp = DecodeBase64(strmultiscript);
    nRequired = 0;
    std::vector<CPubKey> pubkeys;
    try
    {
        ptree pt;
        stringstream stream(strmultiscripttemp);
        read_json(stream,pt);
        nRequired = pt.get<int>("nRequired");
        ptree info = pt.get_child("all_member");
        ptree members = info.get_child("members");
        nmembers = members.size();
         BOOST_FOREACH(ptree::value_type &v,members)
        {
             std::string ks = v.second.get<string>("member");
             if (IsHex(ks))
             {
                 CPubKey vchPubKey(ParseHex(ks));
                 CKeyID vchAddress = vchPubKey.GetID();
                 if (pwalletMain->HaveKey(vchAddress))
                     bCanAdd = true;
                 pubkeys.push_back(vchPubKey);
             }
        }
    }
     catch(ptree_error pt)
    {
        strFailReason = pt.what();
        return false;
    }
    if (!bCanAdd)
    {
        strFailReason = _("address is not yours!");
        return false;
    }

    CScript script = GetScriptForMultisig(nRequired, pubkeys);
    CScriptID innerID(script);
    CBitcoinAddress add(innerID);
    LogPrintf("AddMultiAddress nRequired:%d nmembers:%d\n", nRequired,nmembers);
    LogPrintf("AddMultiAddress address: %s\n", add.ToString());
    unionaddress =  add.ToString();
    return true;
}

void CWallet::UnionCWalletTxFromAddress(std::map<uint256, const CWalletTx*>& vCoins,std::string address,int txtype,uint64_t minblock ,uint64_t maxblock ,std::string tokenname)
{
        if(address=="")return;
        bool fOnlyConfirmed=true;
        const CCoinControl *coinControl = NULL;
        bool fIncludeZeroValue=false;
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const uint256& wtxid = it->first;
            const CWalletTx* pcoin = &(*it).second;
            if (fOnlyConfirmed && !pcoin->IsTrusted())
                continue;
            if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
                continue;
            int nDepth = pcoin->GetDepthInMainChain();
// 			if (nDepth < nTxConfirmTarget)
//                 continue;
            if (nDepth == 0 && !pcoin->InMempool())
                continue;
            if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaces_txid")) {
                continue;
            }
            if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaced_by_txid")) {
                continue;
            }

            if(txtype == 0){
                if(pcoin->tx->GetTxType()!=0)continue;
            }else if(txtype == 5||txtype == 4){
                if(pcoin->tx->GetTxType()!=4&&pcoin->tx->GetTxType()!=5)continue;
            }


            int voutsize = pcoin->tx->vout.size();
            if((minblock>0||maxblock>0)&&!pcoin->hashUnset()){
               uint256 hashblock = pcoin->hashBlock;
               if(mapBlockIndex.count(hashblock)){
                     CBlockIndex* pblockindex = mapBlockIndex[hashblock];
                     if(pblockindex->nHeight<minblock||(maxblock>0&&pblockindex->nHeight>maxblock))
                            continue;
               }
            }
            for (unsigned int i = 0; i < voutsize; i++)
            {
                if(((txtype==4||txtype==5)&&\
                        pcoin->tx->vout[i].getTokenSymbol()!=""&&((tokenname!=""&&\
                        pcoin->tx->vout[i].getTokenSymbol()==tokenname)||tokenname==""))||txtype==0){
                    if(voutFindAddress(pcoin->tx->vout[i],address)){
                        vCoins.insert(make_pair(it->first,&(*it).second));
                        break;
                    }
                    if(voutsize == 1){
                        if(vinsFindAddress(pcoin->tx->vin,address)){
                            vCoins.insert(make_pair(it->first,&(*it).second));
                            break;
                        }
                    }
                }
          }
      }
  LogPrintf("UnionCWalletTxFromAddress address = %s size=%d\n",address,vCoins.size());
  BOOST_FOREACH(auto &coin, vCoins)
  {
      string txid = coin.first.ToString();
      LogPrintf("UnionCWalletTxFromAddress txid = %s \n",txid);
  }

}

bool CWallet::SearchCrossTxid(std::string crosstxid,std::string& txid,std::string tokensymbol)
{
    if(crosstxid=="")return false;
    bool fOnlyConfirmed=true;
    const CCoinControl *coinControl = NULL;
    bool fIncludeZeroValue=true;
    LOCK2(cs_main, cs_wallet);
    LogPrintf("SearchCrossTxid crosstxid：%s  tokensymbol:%s",crosstxid,tokensymbol);
    for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const uint256& wtxid = it->first;
        const CWalletTx* pcoin = &(*it).second;
        if (fOnlyConfirmed && !pcoin->IsTrusted()){
            continue;
        }
        if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0){
            continue;
        }
        int nDepth = pcoin->GetDepthInMainChain();
// 			if (nDepth < nTxConfirmTarget)
//                 continue;
        if (nDepth == 0 && !pcoin->InMempool()){
            continue;
        }
        if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaces_txid")) {
            continue;
        }
        if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaced_by_txid")) {
            continue;
        }
        uint8_t TxType = pcoin->tx->GetTxType();
        if(TxType!=4&&TxType!=5)continue;
        int voutsize = pcoin->tx->vout.size();
        for (unsigned int i = 0; i < voutsize; i++)
        {
            CTxOut zenmezuo;
            std::string TokenSymboltemp =  pcoin->tx->vout[i].getTokenSymbol();
            if(tokensymbol!=""&&TokenSymboltemp!=""&&tokensymbol==TokenSymboltemp){
                std::string txLabel =  pcoin->tx->vout[i].txLabel;
                //std::string::size_type postion = txLabel.find(crosstxid);
                //if(postion != std::string::npos){
                if(txLabel==crosstxid){
                    txid = wtxid.ToString();
                    return true;
                }
            }
      }
  }
    return false;
}
bool CWallet::vinsFindAddress(const std::vector<CTxIn>& vin,std::string address)
{
    CTransactionRef prevTx;
    uint256 hashBlock;
    for (unsigned int i = 0; i < vin.size(); i++)
    {
        if (!GetTransaction(vin[i].prevout.hash, prevTx, Params().GetConsensus(), hashBlock, true))
        {
            continue;
        }
        const CTxOut& prev = prevTx->vout[ vin[i].prevout.n];
        if(voutFindAddress(prev,address)){
            return true;
        }
    }
    return false;

}

bool CWallet::voutFindAddress(const CTxOut& vout,std::string address)
{
    CScript script =  vout.scriptPubKey;
    txnouttype typeRet;
    vector<CTxDestination> prevdestes;
    int nRequiredRet;
    bool fValidAddress = ExtractDestinations(script, typeRet, prevdestes, nRequiredRet);
    if(fValidAddress)
    {
        BOOST_FOREACH(CTxDestination &prevdest, prevdestes)
        {
            CBitcoinAddress add(prevdest);
            std::string multaddress = CBitcoinAddress(prevdest).ToString();
            if(address == ""){
                if(multaddress.size()>5&&(multaddress[0]=='2'||multaddress[0]=='3')&&
                 mapAddressBook.find(prevdest) != mapAddressBook.end())
                      {
                        return true;
                      }
            }
            else{
                if(multaddress==address){
                            return true;
                }
            }
        }
    }
    return false;
}

int64_t CWallet::GetBlockTime(const CWalletTx& wtxIn)
{
     int64_t blocktime = 0;
     if(mapBlockIndex.count(wtxIn.hashBlock))
      blocktime = mapBlockIndex[wtxIn.hashBlock]->GetBlockTime();
     return blocktime;
}

void CWallet::AvailableUnionCoins(std::map<std::string,CAmount>& moneynums,bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue) const
{
    typedef std::map<std::string,CAmount> map_t;
    BOOST_FOREACH( map_t::value_type &i, moneynums )
        i.second = 0;

        LOCK2(cs_main, cs_wallet);
        CBitcoinAddress add;
        txnouttype typeRet;
        vector<CTxDestination> prevdestes(10);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const uint256& wtxid = it->first;
            const CWalletTx* pcoin = &(*it).second;

            if (fOnlyConfirmed && !pcoin->IsTrusted())
                continue;

            if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
			if (nDepth < nTxConfirmTarget)
                continue;
			
            // We should not consider coins which aren't at least in our mempool
            // It's possible for these to be conflicted via ancestors which we may never be able to detect
            if (nDepth == 0 && !pcoin->InMempool())
                continue;

            // We should not consider coins from transactions that are replacing
            // other transactions.
            //
            // Example: There is a transaction A which is replaced by bumpfee
            // transaction B. In this case, we want to prevent creation of
            // a transaction B' which spends an output of B.
            //
            // Reason: If transaction A were initially confirmed, transactions B
            // and B' would no longer be valid, so the user would have to create
            // a new transaction C to replace B'. However, in the case of a
            // one-block reorg, transactions B' and C might BOTH be accepted,
            // when the user only wanted one of them. Specifically, there could
            // be a 1-block reorg away from the chain where transactions A and C
            // were accepted to another chain where B, B', and C were all
            // accepted.
            if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaces_txid")) {
                continue;
            }
            // Similarly, we should not consider coins from transactions that
            // have been replaced. In the example above, we would want to prevent
            // creation of a transaction A' spending an output of A, because if
            // transaction B were initially confirmed, conflicting with A and
            // A', we wouldn't want to the user to create a transaction D
            // intending to replace A', but potentially resulting in a scenario
            // where A, A', and D could all be accepted (instead of just B and
            // D, or just A and A' like the user would want).
            if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaced_by_txid")) {
                continue;
            }
         
            for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++)
            {
                const CScript& script= pcoin->tx->vout[i].scriptPubKey;
                prevdestes.clear();
                int nRequiredRet;
                bool fValidAddress = ExtractDestinations(script, typeRet, prevdestes, nRequiredRet);
                if(!fValidAddress){
                    LogPrintf("AvailableUnionCoins !fValidAddress txid:%s  voutindex:%d \n",wtxid.ToString(),i);
                    continue;
                }
                bool getunion = false;
                std::string unionstraddress;

                BOOST_FOREACH(CTxDestination &prevdest, prevdestes)
                {
                    add.Set(prevdest);
                    std::string straddress=add.ToString();
                    if(straddress.size()>5&&(straddress.at(0)== '2'||straddress.at(0)=='3')
                         &&(mapAddressBook.count(prevdest) != 0))
                    {
                      getunion = true;
                      unionstraddress = straddress;
                      break;
                    }
                }
                if(!getunion)continue;
                map_t::iterator itor = moneynums.find(unionstraddress);
                if(itor != moneynums.end()&&!(IsSpent(wtxid, i)) &&
                        !IsLockedCoin((*it).first, i) && (pcoin->tx->vout[i].nValue > 0 || fIncludeZeroValue) &&
                        (!coinControl || !coinControl->HasSelected() || coinControl->fAllowOtherInputs || coinControl->IsSelected(COutPoint((*it).first, i))))
                    {     
                        itor->second += pcoin->tx->vout[i].nValue;

          
                    }
        }
    }
        BOOST_FOREACH( map_t::value_type &i, moneynums )
            LogPrintf("AvailableUnionCoins address: %s  money:%d \r\n",i.first,i.second);

}
bool SortByTime( const UnionAddressInfo &v1, const UnionAddressInfo &v2)
{
    return v1.time > v2.time;
}
void CWallet::getUnionAddresses(std::vector<UnionAddressInfo>& addressbook)
{
    LOCK2(cs_main, cs_wallet);
    addressbook.clear();
    typedef const std::map<CTxDestination, CAddressBookData>::value_type const_pair;
    BOOST_FOREACH(const_pair& node,mapAddressBook)
    {
        if(node.second.purpose == "union")
         {
             UnionAddressInfo info;
             std::string address = CBitcoinAddress(node.first).ToString();
             info.address = address;
             info.name = node.second.name;
             info.time = getUnionAddressLocalTime(address);
             addressbook.push_back(info);
             LogPrintf("[getUnionAddresses ] dizhi : %s ---- zhanghu:%s \n",address,node.second.name);
         }
    }
    std::sort(addressbook.begin(),addressbook.end(),SortByTime);
}
int CWallet::getUnionAddressLocalTime(std::string address)
{
    int time = 0;
    try
    {
        ptree pt;
        boost::filesystem::path pathDebug = GetDataDir() / "unionaddress.ini";
        ini_parser::read_ini(pathDebug.string(), pt);
        if(!pt.empty()){
            time = pt.get<int>(address+".time");
        }
    }
    catch(std::exception e)
    {
        LogPrintf("GetAddressNmembers %s\n",e.what());
        return 0;
    }
    catch(ini_parser_error& e)
    {
        LogPrintf("GetAddressNmembers %s\n",e.what());
        return 0;
    }
    LogPrintf("GetAddressNmembers address:%s time:%d \n",address,time);
    return time;
}

bool CWallet::isMyPk(std::string pk)
{
    CPubKey vchPubKey(ParseHex(pk));
    if (!vchPubKey.IsFullyValid())
        return false;
    CKeyID csid = vchPubKey.GetID();
    if(HaveKey(csid))//(mapAddressBook.find(csid) != mapAddressBook.end())
      {
        LogPrintf("isMyPk : %s  true\r\n",pk);
        return true;
      }
    LogPrintf("isMyPk : %s  false\r\n",pk);
    return false;

}
bool CWallet::CreateMultiTransaction(std::string& straddressfrom, const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut, std::string& strFailReason, ScriptError& serror, std::string& strtx, const CCoinControl *coinControl /*= NULL*/, bool sign /*= true*/)
{
	CAmount nValue = 0;
	nFeeRet = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;
	CBitcoinAddress address(straddressfrom);
	if (!address.IsValid())
	{
		strFailReason = _("Invalid Ipchain address");
		return false;
	} 

	if (vecSend.size() > 1)
	{
		strFailReason = _("CRecipient size can't larger than 1 ");
		return false;
	}
	for (const auto& recipient : vecSend)
	{
		if ( recipient.nAmount < 0)
		{
			strFailReason = _("Transaction amounts must not be negative");
			return false;
		}
		nValue += recipient.nAmount;
		
		if (recipient.fSubtractFeeFromAmount)
			nSubtractFeeFromAmount++;
		break;
	}
	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}

	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	// Discourage fee sniping.
	//
	// For a large miner the value of the transactions in the best block and
	// the mempool can exceed the cost of deliberately attempting to mine two
	// blocks to orphan the current best block. By setting nLockTime such that
	// only the next block can include the transaction, we discourage this
	// practice as the height restricted and limited blocksize gives miners
	// considering fee sniping fewer options for pulling off this attack.
	//
	// A simple way to think about this is from the wallet's point of view we
	// always want the blockchain to move forward. By setting nLockTime this
	// way we're basically making the statement that we only want this
	// transaction to appear in the next block; we don't want to potentially
	// encourage reorgs by allowing transactions to appear at lower heights
	// than the next block in forks of the best chain.
	//
	// Of course, the subsidy is high enough, and transaction volume low
	// enough, that fee sniping isn't a problem yet, but by implementing a fix
	// now we ensure code won't be written that makes assumptions about
	// nLockTime that preclude a fix later.
	txNew.nLockTime = chainActive.Height();

	// Secondly occasionally randomly pick a nLockTime even further back, so
	// that transactions that are delayed after signing for whatever reason,
	// e.g. high-latency mix networks and some CoinJoin implementations, have
	// better privacy.
	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	{
		set<pair<const CWalletTx*, unsigned int> > setCoins;

		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
            AvailableUnionCoinsCOutput(straddressfrom,vAvailableCoins, true, coinControl);

			std::cout << "vAvailableCoins .size() =" <<vAvailableCoins.size() << std::endl;
			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;
				bool fFirst = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

					if (recipient.fSubtractFeeFromAmount)
					{
						txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

						if (fFirst) // first receiver pays the remainder not divisible by output count
						{
							fFirst = false;
							txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
						}
					}

					if (txout.IsDust(dustRelayFee))
					{
						if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
						{
							if (txout.nValue < 0)
								strFailReason = _("The transaction amount is too small to pay the fee");
							else
								strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
						}
						else
							strFailReason = _("Transaction amount too small");
						return false;
					}
					txNew.vout.push_back(txout);
					break;
				}

				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				std::cout << "nValueToSelect =" << nValueToSelect << std::endl;
				if (!SelectUnionCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					std::cout << "SelectUnionCoins false" << std::endl;
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-bitcoin-address
					CScript scriptChange;

					// Take the first input script as the destination output used for the coin change address
					if (!bFindChangScp)
					{
						strFailReason = _("scriptChange no funds");
						return false;
					}
                    scriptChange = scriptChangeFind;
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
						for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
						{
							if (vecSend[i].fSubtractFeeFromAmount)
							{
								txNew.vout[i].nValue -= nDust;
								if (txNew.vout[i].IsDust(dustRelayFee))
								{
									strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
									return false;
								}
								break;
							}
						}
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();



				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.

                //if (!DummySignTx(txNew, setCoins)) {
                //	strFailReason = _("DummySignTx Signing transaction failed");
                //	return false;
                //}

                unsigned int nBytes = GetVirtualTransactionSize(txNew);
                int nmembers = 0;
                int nRequired = 0;
                std::string strmultiscript;
                if(GetAddressNmembers(straddressfrom,nmembers,nRequired,strmultiscript)&&nmembers>0){
                    nBytes+=((SIGNATURE_SIZE*(nmembers-1)+235)*setCoins.size());
                }else
                    nBytes+=((SIGNATURE_SIZE*(UNION_NUMBER-1)+235)*setCoins.size());
                CTransaction txNewConst(txNew);
                dPriority = txNewConst.ComputePriority(dPriority, nBytes);

                // Remove scriptSigs to eliminate the fee calculation dummy signatures
                for (auto& vin : txNew.vin) {
                    vin.scriptSig = CScript();
                    vin.scriptWitness.SetNull();
                }

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}
		serror = SCRIPT_ERR_OK;
		const CKeyStore& keystore = *pwalletMain;
		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				const CAmount& camount = coin.first->tx->vout[coin.second].nValue;
				SignatureData sigdata;
				int nhashtype = SIGHASH_ALL;
				ProduceSignature(MutableTransactionSignatureCreator(&keystore, &txNew, nIn, camount, nhashtype), scriptPubKey, sigdata);
	
				UpdateTransaction(txNew, nIn, sigdata);
				ScriptError sserror = SCRIPT_ERR_OK;
				if (!VerifyScript(txNewConst.vin[nIn].scriptSig, scriptPubKey, &txNewConst.vin[nIn].scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txNewConst, nIn, camount), &sserror))
				{
					std::cout << "VerifyScript false" << std::endl;
					serror = sserror;
				}

				nIn++;
			}
		}
		strtx = EncodeHexTx(txNew);
		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
        LogPrintf("CreateMultiTransaction return true\n");
	return true;
}

bool CWallet::P2SHProduceSignatureWithoutNetwork(CMutableTransaction& mergedTx,ScriptError& serror, std::string& strFailReason,bool onlyMySign,std::vector<withoutnetwork_inputs>& inputs)
{
    LOCK2(cs_main, cs_wallet);
    CTransaction txNewConst(mergedTx);
    const CKeyStore& keystore = *pwalletMain;
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++)
    {
        LogPrintf("P2SHProduceSignatureWithoutNetwork mergedTx vin -%d\n",i);
        vector<unsigned char> pkData(ParseHex(inputs[i].scriptPubKey));
        CScript prevPubKey(pkData.begin(),pkData.end());
        if (!&prevPubKey) {
            LogPrintf("P2SHProduceSignatureWithoutNetwork prevPubKey false -%d\n",i);
            continue;
        }
        SignatureData sigdata;
        int nhashtype = SIGHASH_ALL;
        if(!ProduceSignature(MutableTransactionSignatureCreator(&keystore, &mergedTx, i, inputs[i].nAmount, nhashtype), prevPubKey, sigdata)){
            LogPrintf("P2SHProduceSignatureWithoutNetwork ProduceSignature false -%d\n",i);
        }
        if(sigdata.scriptSig.size()==0){
            LogPrintf("P2SHProduceSignatureWithoutNetwork IsPayToScriptHash false -%d\n",i);
            strFailReason = _("sign failed.");
            return false;
        }
        if(!onlyMySign)
            sigdata = CombineSignatures(prevPubKey, TransactionSignatureChecker(&txNewConst, i, inputs[i].nAmount), sigdata, DataFromTransaction(mergedTx, i));
        UpdateTransaction(mergedTx, i, sigdata);
        ScriptError sserror = SCRIPT_ERR_OK;
    }
    return true;
}
bool CWallet::P2SHProduceSignature(CMutableTransaction& mergedTx,ScriptError& serror, std::string& strFailReason,bool onlyMySign)
{
    LOCK2(cs_main, cs_wallet);
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view
        BOOST_FOREACH(const CTxIn& txin, mergedTx.vin) {
            const uint256& prevHash = txin.prevout.hash;
            CCoins coins;
            view.AccessCoins(prevHash); // this is certainly allowed to fail
        }
        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }
    CTransaction txNewConst(mergedTx);
    const CKeyStore& keystore = *pwalletMain;
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++)
    {
        LogPrintf("P2SHProduceSignature mergedTx vin -%d\n",i);
        CTxIn& txin = mergedTx.vin[i];
        const CCoins* coins = view.AccessCoins(txin.prevout.hash);
        if (coins == NULL || !coins->IsAvailable(txin.prevout.n)) {
            LogPrintf("P2SHProduceSignature coins->IsAvailable false -%d\n",i);
            strFailReason = _("Tx have spented coin.");
            return false;
        }
        const CScript& prevPubKey = coins->vout[txin.prevout.n].scriptPubKey;
        if (!&prevPubKey) {
            LogPrintf("P2SHProduceSignature prevPubKey false -%d\n",i);
            continue;
        }
        const CAmount& amount = coins->vout[txin.prevout.n].nValue;
        SignatureData sigdata;
        int nhashtype = SIGHASH_ALL;
        if(!ProduceSignature(MutableTransactionSignatureCreator(&keystore, &mergedTx, i, amount, nhashtype), prevPubKey, sigdata)){
            LogPrintf("P2SHProduceSignature ProduceSignature false -%d\n",i);
        }
        if(sigdata.scriptSig.size()==0){
            LogPrintf("P2SHProduceSignature IsPayToScriptHash false -%d\n",i);
            strFailReason = _("sign failed.");
            return false;
        }
        if(!onlyMySign)
            sigdata = CombineSignatures(prevPubKey, TransactionSignatureChecker(&txNewConst, i, amount), sigdata, DataFromTransaction(mergedTx, i));
        UpdateTransaction(mergedTx, i, sigdata);
        ScriptError sserror = SCRIPT_ERR_OK;
        if (!VerifyScript(txin.scriptSig, prevPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txNewConst, i, amount), &sserror)) {
            serror = sserror;
        }
    }
    return true;
}
bool CWallet::analysisMultiTransaction(const std::string& strtx,std::string& strFailReason,\
        std::string& address,CAmount& money,int haveSigned,std::string& sourceaddress)
{
    CMutableTransaction mergedTx;
    if (!DecodeHexTx(mergedTx, strtx))
    {
        LogPrintf("analysisMultiTransaction DecodeHexTx  failed\n");
        strFailReason = _("DecodeHexTx  failed");
        return false;
    }
    if(mergedTx.vout.size()>0){
        if(mergedTx.vout.size()>2){
            strFailReason = _("vout.size  too large");
            LogPrintf("vout.size: %d\n",mergedTx.vout.size());
        }
        auto itor = mergedTx.vout.begin();
        money = itor->nValue;
        vector<CTxDestination> prevdestes;
        txnouttype typeRet;
        int nRequiredRet;
        bool fValidAddress = ExtractDestinations(itor->scriptPubKey, typeRet, prevdestes, nRequiredRet);
        if(fValidAddress&&prevdestes.size()>0){
            CBitcoinAddress bitaddress(prevdestes[0]);
            address = bitaddress.ToString();
        }
        if(mergedTx.vin.size()>0){
            auto itor = mergedTx.vin.begin();
            std::string asmstr = ScriptToAsmStr(itor->scriptSig,1);
            int begin=-1;
            while((begin=asmstr.find("[ALL]",begin+1))!=string::npos)
            {
                haveSigned++;
            }
            LOCK(mempool.cs);
            CTransactionRef prevTx;
            uint256 hashBlock;
            CAmount totalvintvalues = 0;
            if (GetTransaction(itor->prevout.hash, prevTx,Params().GetConsensus(), hashBlock, true))
            {
                CScript scriptPubKey = prevTx->vout[itor->prevout.n].scriptPubKey;
                bool fValidAddress = ExtractDestinations(scriptPubKey, typeRet, prevdestes, nRequiredRet);
                if(fValidAddress&&prevdestes.size()>0){
                    CBitcoinAddress bitaddress(prevdestes[0]);
                    sourceaddress = bitaddress.ToString();
                }

            }
        }
        LogPrintf("address:%s money:%d haveSigned:%d sourceaddress:%s\n",address,money,haveSigned,sourceaddress);
        return true;
    }else{
        strFailReason = _("vout.size  failed");
        return false;
    }
}
bool CWallet::GetAddressNmembers(std::string address,int &nmembers,int &nRequired,string &strmultiscript)
{
    try
    {
        ptree pt;
        boost::filesystem::path pathDebug = GetDataDir() / "unionaddress.ini";
        ini_parser::read_ini(pathDebug.string(), pt);
       // std::cout<<pathDebug.string()<<address<<std::endl;
        if(!pt.empty()){
        strmultiscript = pt.get<std::string>(address+".strmultiscript");
        nmembers = pt.get<int>(address+".nmembers");
        nRequired = pt.get<int>(address+".nRequired");
        }
    }
    catch(std::exception e)
    {
        LogPrintf("GetAddressNmembers %s\n",e.what());
        return false;
    }
    catch(ini_parser_error& e)
    {
        LogPrintf("GetAddressNmembers %s\n",e.what());
        return false;
    }
    LogPrintf("GetAddressNmembers address:%s nmembers:%d strmultiscript:%s\n",address,nmembers,strmultiscript);
    return true;
}
bool CWallet::getUnionTxVinInfo(std::string& strtx,std::string& strError,std::string& info)
{
    CMutableTransaction mergedTx;
    LogPrintf("getUnionTxVinInfo strtx  %s\n",strtx);
    if (!DecodeHexTx(mergedTx, strtx))
    {
        LogPrintf("getUnionTxVinInfo DecodeHexTx  failed\n");
        strError = _("DecodeHexTx  failed");
        return false;
    }
    LOCK2(cs_main, cs_wallet);
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view
        BOOST_FOREACH(const CTxIn& txin, mergedTx.vin) {
            const uint256& prevHash = txin.prevout.hash;
            CCoins coins;
            view.AccessCoins(prevHash); // this is certainly allowed to fail
        }
        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }
    LogPrintf("getUnionTxVinInfo mergedTx.vin.size() %d\n",mergedTx.vin.size());
    const CKeyStore& keystore = *pwalletMain;
    UniValue results(UniValue::VARR);
    try{
        for (unsigned int i = 0; i < mergedTx.vin.size(); i++)
        {
            LogPrintf("getUnionTxVinInfo vin i=%d\n",i);
            CTxIn& txin = mergedTx.vin[i];
            const CCoins* coins = view.AccessCoins(txin.prevout.hash);
            LogPrintf("gtxin.prevout.hash %s\n",txin.prevout.hash.ToString());
            if(!coins){

                   strError = _("view.AccessCoins  failed");
                   return false;
            }
            std::string txid = txin.prevout.hash.ToString();
            if(txin.prevout.n>=coins->vout.size()){
                LogPrintf("txin.prevout.n size: %d coins->vout.size() %d\n",txin.prevout.n,coins->vout.size());
                strError = _("txin.prevout.n>=coins->vout.size  failed");
                return false;
            }
            if(!coins->IsAvailable(txin.prevout.n)){
                LogPrintf("coins->IsAvailable(txin.prevout.n) %d\n",txin.prevout.n);
                strError = _("coins->IsAvailable  failed");
                return false;
            }
            const CAmount& amount = coins->vout[txin.prevout.n].nValue;
            const CScript& scriptPubKey = coins->vout[txin.prevout.n].scriptPubKey;

            UniValue entry(UniValue::VOBJ);
            if(!scriptPubKey.empty())
            entry.push_back(Pair("scriptPubKey", HexStr(scriptPubKey.begin(), scriptPubKey.end())));
            entry.push_back(Pair("amount", ValueFromAmount(coins->vout[txin.prevout.n].nValue)));
            entry.push_back(Pair("txid", txid));
            entry.push_back(Pair("vout", txin.prevout.n));
            results.push_back(entry);
        }
        info = results.write();
    }
    catch(...){
        LogPrintf("getUnionTxVinInfo catch\n");
    }

    return true;
}

bool CWallet::SignMultiTransaction(const std::string& strtx, std::string& strFailReason, ScriptError& serror, std::string& strtxout,CMutableTransaction& mergedTx,bool isrpc,bool onlyMySign)
{
    std::string strtxtemp = strtx;
	if (!DecodeHexTx(mergedTx, strtx))
	{
        LogPrintf("SignMultiTransaction DecodeHexTx  failed\n");
		strFailReason = _("DecodeHexTx  failed");
		return false;
	}
    unsigned int nBytes1 = GetVirtualTransactionSize(mergedTx);
    LogPrintf("P2SHProduceSignature nBytes size :%d\n",nBytes1);
    LogPrintf("P2SHProduceSignature strtx =%s\n",strtx);
    CAmount nfee = pwalletMain->GetMinimumFee(nBytes1, nTxConfirmTarget, mempool) * 2;
    LogPrintf("P2SHProduceSignature nfee =%d\n",nfee);
	serror = SCRIPT_ERR_OK;
	CBasicKeyStore tempKeystore;
    CMutableTransaction temp(mergedTx);
    if(!P2SHProduceSignature(mergedTx, serror,strFailReason,onlyMySign)){
        return false;
    }
	if (serror == SCRIPT_ERR_OK)
	{
        strtxout = EncodeHexTx(temp);
        LogPrintf("P2SHProduceSignature signhex1 :%s\n",strtxout);
        LogPrintf("P2SHProduceSignature serror == SCRIPT_ERR_OK\n");
        if(isrpc){
            if(CommitMultiTransaction(mergedTx,strFailReason))
               strtxout = mergedTx.GetHash().ToString();
            else
               return false;
        }
        CWalletTx wtxNew(pwalletMain,MakeTransactionRef(mergedTx));
        strtxout = wtxNew.GetHash().ToString();
        if(onlyMySign)
            strtxout = EncodeHexTx(mergedTx);
		return true;
	}
    else{
        std::cout << "serror != SCRIPT_ERR_OK"<<serror << std::endl;
        LogPrintf("SignMultiTransaction serror != SCRIPT_ERR_OK: %s\n",serror);
    }
	strtxout = EncodeHexTx(mergedTx);
    if(strtxtemp == strtxout){
        LogPrintf("P2SHProduceSignature sign failed.\n");
        strFailReason = _("sign failed.");
        return false;
    }
	return true;
}

bool CWallet::SignMultiTransactionWithoutNetwork(const std::string& strtx, std::string& strFailReason,std::string& strtxout,std::vector<withoutnetwork_inputs>& inputs,bool onlyMySign)
{
    CMutableTransaction mergedTx;
    std::string strtxtemp = strtx;
    if (!DecodeHexTx(mergedTx, strtx))
    {
        LogPrintf("SignMultiTransactionWithoutNetwork DecodeHexTx  failed\n");
        strFailReason = _("DecodeHexTx  failed");
        return false;
    }
    if(inputs.size()!=mergedTx.vin.size()){
        strFailReason = _("inputs size false.");
        return false;
    }


    ScriptError serror = SCRIPT_ERR_OK;
    CBasicKeyStore tempKeystore;
    CMutableTransaction temp(mergedTx);
    if(!P2SHProduceSignatureWithoutNetwork(mergedTx, serror,strFailReason,onlyMySign,inputs)){
        return false;
    }
    strtxout = EncodeHexTx(mergedTx);
    if(strtxtemp == strtxout){
        LogPrintf("SignMultiTransactionWithoutNetwork sign failed.\n");
        strFailReason = _("sign failed.");
        return false;
    }
    return true;
}
bool CWallet::getSignatureDataFromTx(std::string txstring,std::string& strFailReason,SignatureData& signdata,unsigned int nIn)
{
    CMutableTransaction mergedTx;
    if (!DecodeHexTx(mergedTx, txstring))
    {
        LogPrintf("getSignatureDataFromTx DecodeHexTx  failed\n");
        strFailReason = _("DecodeHexTx  failed");
        return false;
    }
    signdata = DataFromTransaction(mergedTx,nIn);
    return true;
}
bool CWallet::P2SHIntegratedSignature(CMutableTransaction& mergedTx,std::string& strFailReason,ScriptError& serror,vector<std::string> &signtxs)
{
    LOCK2(cs_main, cs_wallet);
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view
        BOOST_FOREACH(const CTxIn& txin, mergedTx.vin) {
            const uint256& prevHash = txin.prevout.hash;
            CCoins coins;
            view.AccessCoins(prevHash); // this is certainly allowed to fail
        }
        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }
    CTransaction txNewConst(mergedTx);
    const CKeyStore& keystore = *pwalletMain;
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++)
    {
        LogPrintf("P2SHProduceSignature mergedTx vin -%d\n",i);
        CTxIn& txin = mergedTx.vin[i];
        const CCoins* coins = view.AccessCoins(txin.prevout.hash);
        if (coins == NULL || !coins->IsAvailable(txin.prevout.n)) {
            LogPrintf("P2SHProduceSignature coins->IsAvailable false -%d\n",i);
            strFailReason = _("Tx have spented coin.");
            return false;
        }
        const CScript& prevPubKey = coins->vout[txin.prevout.n].scriptPubKey;
        if (!&prevPubKey) {
            LogPrintf("P2SHProduceSignature prevPubKey false -%d\n",i);
            strFailReason = _("prevPubKey false");
            return false;
        }
        const CAmount& amount = coins->vout[txin.prevout.n].nValue;
        SignatureData sigdata;
        int nhashtype = SIGHASH_ALL;
        for(int isign = 0;isign<signtxs.size();isign++){
            std::string sign = signtxs.at(isign);
            SignatureData signdataonly;
            if(getSignatureDataFromTx(sign,strFailReason,signdataonly,i)){
             sigdata = CombineSignatures(prevPubKey, TransactionSignatureChecker(&txNewConst, i, amount), signdataonly, sigdata);
             UpdateTransaction(mergedTx, i, sigdata);
            }else{
                LogPrintf("SignUnionTxWithSigns getSignatureDataFromTx  failed %s %s txvin:%d\n",sign,strFailReason,i);
                strFailReason = strFailReason + "  keys_index:" +  boost::lexical_cast<std::string>(isign);
                return false;
            }
        }

        ScriptError sserror = SCRIPT_ERR_OK;
        if (!VerifyScript(txin.scriptSig, prevPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txNewConst, i, amount), &sserror)) {
            serror = sserror;
        }
    }
    return true;
}



bool CWallet::P2SHCheckSignature(CMutableTransaction& mergedTx,std::string& strFailReason,ScriptError& serror)
{
    LOCK2(cs_main, cs_wallet);
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view
        BOOST_FOREACH(const CTxIn& txin, mergedTx.vin) {
            const uint256& prevHash = txin.prevout.hash;
            CCoins coins;
            view.AccessCoins(prevHash); // this is certainly allowed to fail
        }
        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }
    CTransaction txNewConst(mergedTx);
    const CKeyStore& keystore = *pwalletMain;
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++)
    {
        LogPrintf("P2SHCheckSignature mergedTx vin -%d\n",i);
        CTxIn& txin = mergedTx.vin[i];
        const CCoins* coins = view.AccessCoins(txin.prevout.hash);
        if (coins == NULL || !coins->IsAvailable(txin.prevout.n)) {
            LogPrintf("P2SHCheckSignature coins->IsAvailable false -%d\n",i);
            strFailReason = _("Tx have spented coin.");
            return false;
        }
        const CScript& prevPubKey = coins->vout[txin.prevout.n].scriptPubKey;
        if (!&prevPubKey) {
            LogPrintf("P2SHCheckSignature prevPubKey false -%d\n",i);
            strFailReason = _("P2SHCheckSignature prevPubKey false.");
            return false;
            continue;
        }
        const CAmount& amount = coins->vout[txin.prevout.n].nValue;
        int nhashtype = SIGHASH_ALL;
        ScriptError sserror = SCRIPT_ERR_OK;
        if (VerifyScript(txin.scriptSig, prevPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txNewConst, i, amount), &sserror)) {
           }
        else
        {
            serror = sserror;
            LogPrintf("P2SHCheckSignature VerifyScript serror -%d\n",serror);


             typedef std::vector<unsigned char> valtype;
             SignatureData signdataonly;
             signdataonly = DataFromTransaction(mergedTx,i);


             TransactionSignatureChecker checker=  TransactionSignatureChecker(&txNewConst, i, amount);
             txnouttype txType;
             vector<vector<unsigned char> > vSolutions;

             // Combine all the signatures we've got:
             set<valtype> allsigs;
             std::vector<valtype> script;
             EvalScript(script, signdataonly.scriptSig, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SIGVERSION_BASE);
             if(script.empty()||script.back().empty()){
                 strFailReason = _("script empty.");
                 return false;
             }
             valtype spk = script.back();
             CScript pubKey2(spk.begin(), spk.end());
             Solver(pubKey2, txType, vSolutions);
             LogPrintf("vSolutions.size() %d \n",vSolutions.size());
             BOOST_FOREACH(const valtype& v,script)
             {
                 if (!v.empty())
                     allsigs.insert(v);
             }
             // Build a map of pubkey -> signature by matching sigs to pubkeys:
             //assert(vSolutions.size() > 1);
             if(vSolutions.size() <= 1){
                 LogPrintf("vSolutions.size() <= 1 %d \n",txType);
                 strFailReason = _("vSolutions.size() <= 1.");
                 return false;
             }
             unsigned int nSigsRequired = vSolutions.front()[0];
             unsigned int nPubKeys = vSolutions.size()-2;
             map<valtype, valtype> sigs;
             BOOST_FOREACH(const valtype& sig, allsigs)
             {
                 bool find=false;
                 for (unsigned int i = 0; i < nPubKeys; i++)
                 {
                     const valtype& pubkey = vSolutions[i+1];
                     if (sigs.count(pubkey)){
                         find = true;
                         continue; // Already got a sig for this pubkey
                     }
                     if (checker.CheckSig(sig, pubkey, pubKey2, SIGVERSION_BASE))
                     {
                         sigs[pubkey] = sig;
                         find = true;
                         break;
                     }
                 }
                 if(!find){
                     LogPrintf("P2SHCheckSignature CheckSig false \n");
                     strFailReason = _("CheckSig false.");
                     return false;
                 }
             }
        }
    }
    return true;
}



bool CWallet::uniontxcommit(const std::string strtx, std::string& strFailReason, std::string& strtxout)
{
    CMutableTransaction mergedTx;
    std::string strtxtemp = strtx;
    if (!DecodeHexTx(mergedTx, strtx))
    {
        LogPrintf("uniontxcommit DecodeHexTx  failed\n");
        strFailReason = _("DecodeHexTx  failed");
        return false;
    }
    unsigned int nBytes1 = GetVirtualTransactionSize(mergedTx);
    LogPrintf("uniontxcommit nBytes size :%d\n",nBytes1);
    LogPrintf("uniontxcommit strtx =%s\n",strtx);
    CAmount nfee = pwalletMain->GetMinimumFee(nBytes1, nTxConfirmTarget, mempool) * 2;
    LogPrintf("uniontxcommit nfee =%d\n",nfee);
    if(CommitMultiTransaction(mergedTx,strFailReason))
       strtxout = mergedTx.GetHash().ToString();
    else
       return false;

}
bool CWallet::SignUnionTxWithSigns(const std::string& strtx, std::string& strFailReason, ScriptError& serror, std::string& strtxout,CMutableTransaction& mergedTx,vector<std::string> &signtxs,bool checkscripterr)
{
    if (!DecodeHexTx(mergedTx, strtx))
    {
        LogPrintf("SignUnionTxWithSigns DecodeHexTx  failed\n");
        strFailReason = _("DecodeHexTx  failed");
        return false;
    }
    unsigned int nBytes1 = GetVirtualTransactionSize(mergedTx);
    LogPrintf("SignUnionTxWithSigns nBytes size :%d\n",nBytes1);
    LogPrintf("SignUnionTxWithSigns strtx =%s\n",strtx);
    CAmount nfee = pwalletMain->GetMinimumFee(nBytes1, nTxConfirmTarget, mempool) * 2;
    LogPrintf("SignUnionTxWithSigns nfee =%d\n",nfee);
    serror = SCRIPT_ERR_OK;
    CBasicKeyStore tempKeystore;
    CMutableTransaction temp(mergedTx);
    if(!P2SHIntegratedSignature(mergedTx,strFailReason ,serror,signtxs)){
         LogPrintf("SignUnionTxWithSigns P2SHIntegratedSignature false\n");
         return false;
    }
    if (serror == SCRIPT_ERR_OK||!checkscripterr)
    {
        strtxout = EncodeHexTx(temp);
        LogPrintf("SignUnionTxWithSigns signhex1 :%s\n",strtxout);
        LogPrintf("SignUnionTxWithSigns serror == SCRIPT_ERR_OK\n");
        strtxout = EncodeHexTx(mergedTx);
        return true;
    }
    else{
        std::cout << "serror != SCRIPT_ERR_OK"<<serror << std::endl;
        LogPrintf("SignUnionTxWithSigns serror != SCRIPT_ERR_OK: %s\n",serror);
        strFailReason = _("Tx verification  failed");
        return false;
    }
    return true;
}


bool CWallet::checkUnionSign(const std::string& strtx, std::string& strFailReason, ScriptError& serror, std::string& strtxout,CMutableTransaction& mergedTx)
{
    if (!DecodeHexTx(mergedTx, strtx))
    {
        LogPrintf("checkUnionSign DecodeHexTx  failed\n");
        strFailReason = _("DecodeHexTx  failed");
        return false;
    }
    unsigned int nBytes1 = GetVirtualTransactionSize(mergedTx);
    LogPrintf("checkUnionSign nBytes size :%d\n",nBytes1);
    LogPrintf("checkUnionSign strtx =%s\n",strtx);
    CAmount nfee = pwalletMain->GetMinimumFee(nBytes1, nTxConfirmTarget, mempool) * 2;
    LogPrintf("checkUnionSign nfee =%d\n",nfee);
    serror = SCRIPT_ERR_OK;
    CBasicKeyStore tempKeystore;
    CMutableTransaction temp(mergedTx);
    if(!P2SHCheckSignature(mergedTx,strFailReason ,serror)){
         LogPrintf("checkUnionSign P2SHIntegratedSignature false\n");
         return false;
    }else{
        if (serror == SCRIPT_ERR_OK)
        {
            strtxout = EncodeHexTx(temp);
            LogPrintf("checkUnionSign signhex1 :%s\n",strtxout);
            LogPrintf("checkUnionSign serror == SCRIPT_ERR_OK\n");
            strtxout = "true";//EncodeHexTx(mergedTx);
            return true;
        }
        else{
            std::cout << "serror != SCRIPT_ERR_OK"<<serror << std::endl;
            LogPrintf("checkUnionSign serror != SCRIPT_ERR_OK: %s\n",serror);
            //strFailReason = _("Tx verification  failed");
            strtxout = "false";
            return true;
        }
    }

    return true;
}



bool CWallet::CommitMultiTransaction(CMutableTransaction& mergedTx,std::string& error)
{
    CReserveKey reservekey(pwalletMain);
    CWalletTx wtxNew(pwalletMain,MakeTransactionRef(mergedTx));
    CValidationState state;
    if (!pwalletMain->CommitTransaction(wtxNew, reservekey, g_connman.get(), state))
    {
        LogPrintf("SignMultiTransaction CommitTransaction  failed:%s\n",state.GetRejectReason());
        std::cout << state.GetRejectReason() << std::endl;
        error = state.GetRejectReason();
        return false;
    }
    LogPrintf("SignMultiTransaction CommitTransaction  true\n");
    return true;

}

void CWallet::AvailableUnionCoinsCOutput(std::string& straddress, std::vector<COutput>& vCoins, bool fOnlyConfirmed /*= true*/, const CCoinControl *coinControl /*= NULL*/,\
                                   bool fIncludeZeroValue /*= false*/,bool isToken /*= false*/) const
{
    LogPrintf("AvailableUnionCoinsCOutput straddress: %s\n",straddress);
    CBitcoinAddress address(straddress);
    if (!address.IsValid())
    {
        LogPrintf("AvailableUnionCoinsCOutput address.IsValid false.\n");
        return;
    }
    CScript strunionaddress = GetScriptForDestination(address.Get());

	vCoins.clear();
	{
		LOCK2(cs_main, cs_wallet);
		for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
		{
			const uint256& wtxid = it->first;
			const CWalletTx* pcoin = &(*it).second;


			if (fOnlyConfirmed && !pcoin->IsTrusted())
				continue;

			if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
				continue;

			int nDepth = pcoin->GetDepthInMainChain();

			if (nDepth < nTxConfirmTarget)
				continue;
			// We should not consider coins which aren't at least in our mempool
			// It's possible for these to be conflicted via ancestors which we may never be able to detect
			if (nDepth == 0 && !pcoin->InMempool())
				continue;

			// We should not consider coins from transactions that are replacing
			// other transactions.
			//
			// Example: There is a transaction A which is replaced by bumpfee
			// transaction B. In this case, we want to prevent creation of
			// a transaction B' which spends an output of B.
			//
			// Reason: If transaction A were initially confirmed, transactions B
			// and B' would no longer be valid, so the user would have to create
			// a new transaction C to replace B'. However, in the case of a
			// one-block reorg, transactions B' and C might BOTH be accepted,
			// when the user only wanted one of them. Specifically, there could
			// be a 1-block reorg away from the chain where transactions A and C
			// were accepted to another chain where B, B', and C were all
			// accepted.
			if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaces_txid")) {
				continue;
			}

			// Similarly, we should not consider coins from transactions that
			// have been replaced. In the example above, we would want to prevent
			// creation of a transaction A' spending an output of A, because if
			// transaction B were initially confirmed, conflicting with A and
			// A', we wouldn't want to the user to create a transaction D
			// intending to replace A', but potentially resulting in a scenario
			// where A, A', and D could all be accepted (instead of just B and
			// D, or just A and A' like the user would want).
			if (nDepth == 0 && fOnlyConfirmed && pcoin->mapValue.count("replaced_by_txid")) {
				continue;
			}

			for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++) {
				isminetype mine = IsMine(pcoin->tx->vout[i]);
				const CScript& multicscript = pcoin->tx->vout[i].scriptPubKey;
				if (strunionaddress != multicscript)
					continue;
				if (!(IsSpent(wtxid, i)) &&
					!IsLockedCoin((*it).first, i) && (pcoin->tx->vout[i].nValue > 0 || fIncludeZeroValue) &&
					(!coinControl || !coinControl->HasSelected() || coinControl->fAllowOtherInputs || coinControl->IsSelected(COutPoint((*it).first, i))) &&
					((!isToken && pcoin->tx->vout[i].txType == TXOUT_NORMAL) || (isToken && (pcoin->tx->vout[i].txType == TXOUT_TOKENREG || pcoin->tx->vout[i].txType == TXOUT_TOKEN || pcoin->tx->vout[i].txType == TXOUT_ADDTOKEN)))){

					vCoins.push_back(COutput(pcoin, i, nDepth,
					((mine & ISMINE_SPENDABLE) != ISMINE_NO) ||
					(coinControl && coinControl->fAllowWatchOnly && (mine & ISMINE_WATCH_SOLVABLE) != ISMINE_NO),
					(mine & (ISMINE_SPENDABLE | ISMINE_WATCH_SOLVABLE)) != ISMINE_NO));
                }
			}
		}
	}
    LogPrintf("AvailableUnionCoinsCOutput vCoins.size: %d\n",vCoins.size());
}

bool CWallet::SelectUnionCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<std::pair<const CWalletTx*, unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl *coinControl /*= NULL*/) const
{
	vector<COutput> vCoins(vAvailableCoins);
	uint8_t txtype = -1;
	nValueRet = 0;

	//Go through it and see if you can find only one tokencoins to trade and find and return
	BOOST_FOREACH(const COutput& out, vCoins)
	{
		const CWalletTx *pcoin = out.tx;
// 		if (!out.fSpendable)
// 		{
// 			std::cout << "out.fSpendable =" << out.fSpendable << std::endl;
// 			continue;
// 		}
			
		//Eight confirmations can be spent
		if (out.nDepth < nTxConfirmTarget)
			continue;
		txtype = pcoin->tx->vout[out.i].txType;
		if (txtype != TXOUT_NORMAL)
			continue;
		int i = out.i;
		CAmount nCoinValue = pcoin->tx->vout[out.i].nValue;

		pair<const CWalletTx*, unsigned int> coin = make_pair(pcoin, i);
		if (nCoinValue >= nTargetValue)
		{
			setCoinsRet.clear();
			setCoinsRet.insert(coin);
			nValueRet = 0;
			nValueRet += nCoinValue;
			return true;
		}
		else if (nCoinValue < nTargetValue)
		{
			setCoinsRet.insert(coin);
			nValueRet += nCoinValue;
			if (nValueRet >= nTargetValue)
				return true;
		}
	}

	return false;
}

bool CWallet::ChangeMultiTxFee(CMutableTransaction& tx, unsigned int bytes)
{

    CAmount nfee = pwalletMain->GetMinimumFee(bytes, nTxConfirmTarget, mempool) * 2;
	CTransaction ctx(tx);
	CTransactionRef prevTx;
	uint256 hashBlock;
	CAmount totalvintvalues = 0;
	unsigned int vinnums = ctx.vin.size();
	unsigned int voutnums = ctx.vout.size();
	for (unsigned int i = 0; i < vinnums; i++)
	{
		if (!GetTransaction(tx.vin[i].prevout.hash, prevTx, Params().GetConsensus(), hashBlock, true))
		{
			return false;
		}
		totalvintvalues += prevTx->vout[tx.vin[i].prevout.n].nValue;
	}
	for (unsigned j = 0; j <voutnums - 1; j++)
	{
		totalvintvalues -= ctx.vout[j].nValue;
	}       
	CAmount newAmount = totalvintvalues - nfee;
    LogPrintf("ChangeMultiTxFee newAmount:  %s,\
     totalvintvalues: %s,\
      nfee: %s,nTxConfirmTarget: %s \
    \n",newAmount,totalvintvalues,nfee,nTxConfirmTarget);

    std::cout << "newAmount " << newAmount<<" totalvintvalues: " \
        <<totalvintvalues<<" nfee: "<<nfee << \
       " nTxConfirmTarget:"<<nTxConfirmTarget<< std::endl;
	unsigned int nChangeindex = voutnums - 1;
	tx.vout[nChangeindex].nValue = newAmount;

	return true;
}

bool CWallet::DecodeStrInvCode(std::string strmultiscript, std::vector<std::string>& pubkeys, int& nRequired, int& nmembers)
{

	pubkeys.clear();
	std::string strmultiscripttemp = DecodeBase64(strmultiscript);
	nRequired = 0;
	nmembers = 0;
	try
	{
		ptree pt;
		stringstream stream(strmultiscripttemp);
		read_json(stream, pt);
		nRequired = pt.get<int>("nRequired");
		ptree info = pt.get_child("all_member");
		ptree members = info.get_child("members");
		nmembers = members.size();
		BOOST_FOREACH(ptree::value_type &v, members)
		{
			std::string ks = v.second.get<string>("member");
		
			pubkeys.push_back(ks);
		}
	}
	catch (ptree_error pt)
	{
		return false;
	}
	return true;
}

void CWallet::flushWalletData()
{
    if (!bitdb.mapFileUseCount.count(strWalletFile) || bitdb.mapFileUseCount[strWalletFile] == 0)
    {
        // Flush log data to the dat file
        bitdb.CloseDb(strWalletFile);
        bitdb.CheckpointLSN(strWalletFile);
        bitdb.mapFileUseCount.erase(strWalletFile);
    }
}

uint8_t CWallet::GetDebitOfIp(const CTransaction& tx, const isminefilter& filter) const
{
	if (tx.vin.empty())
		return 0;
	uint8_t debit = 0;
	for (auto txvin : tx.vin)
	{

		{
			LOCK(cs_wallet);
			map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txvin.prevout.hash);
			if (mi != mapWallet.end())
			{
				const CWalletTx& prev = (*mi).second;
				if (txvin.prevout.n < prev.tx->vout.size())
				if (IsMine(prev.tx->vout[txvin.prevout.n]) & filter)
				{
					if (prev.tx->vout[txvin.prevout.n].txType == TXOUT_IPCOWNER || prev.tx->vout[txvin.prevout.n].txType == TXOUT_IPCAUTHORIZATION)
					{
						return prev.tx->vout[txvin.prevout.n].txType;
					}
				
				}
			}
		}
	}
	return debit;
}
bool CWallet::regaddressfirst(std::vector<COutput> &vAvailableCoins, const std::vector<CRecipientaddtoken>&vecSend){
	if (vecSend.empty()){
		return false;
	}
	int needswap = 0;
	bool findaddress = false;
	for (int i = 0; i < vAvailableCoins.size();i++)
	{
		const CTxOut& txout = vAvailableCoins[i].tx->tx->vout[vAvailableCoins[i].i];
		CScript script = txout.scriptPubKey;
		txnouttype typeRet;
		std::vector<CTxDestination> prevdestes;
		int nRequiredRet;
		CScript address;
		bool fValidAddress = ExtractDestinations(script, typeRet, prevdestes, nRequiredRet);
		if (fValidAddress){
			BOOST_FOREACH(CTxDestination &prevdest, prevdestes){
				CBitcoinAddress add(prevdest);
				address = GetScriptForDestination(add.Get());
				break;
			}
		}
		if (vecSend[0].scriptPubKey == address){
			findaddress = true;
			if (needswap < vAvailableCoins.size() && needswap < i){
				swap(vAvailableCoins[needswap], vAvailableCoins[i]);
				needswap++;
			}
		}
	}
	return findaddress;
}

bool CWallet::CreateAddTokenRegTransaction(std::string& strReglabel, const std::vector<CRecipientaddtoken>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut, std::string& strFailReason, const CCoinControl *coinControl /*= NULL*/, bool sign /*= true*/)
{
	nFeeRet = 0;
	CAmount nValue = 0;
	int nChangePosRequest = nChangePosInOut;
	unsigned int nSubtractFeeFromAmount = 0;

	if (vecSend.empty())
	{
		strFailReason = _("Transaction must have at least one recipient");
		return false;
	}
	wtxNew.fTimeReceivedIsTxTime = true;
	wtxNew.BindWallet(this);
	CMutableTransaction txNew;

	txNew.nLockTime = chainActive.Height();

	if (GetRandInt(10) == 0)
		txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

	assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
	assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
	payTxFee = CFeeRate(DEFAULT_TRANSACTION_FEE * 2);
	if (strReglabel.size() == 0)
	{
		strFailReason = _("IPCRegTransaction must have IPCTokenLabel");
		return false;
	}
	UniValue JsonIpclabel;
	bool isNUll = JsonIpclabel.read(strReglabel);

	AddTokenLabel label;
	label.version = 1;

	UniValue o = find_value(JsonIpclabel, "TokenSymbol");
	if (o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The TokenIPCLabel's TokenSymbol err");
		return false;
	}
	std::string strTokenSymbol = o.get_str();
	if (strTokenSymbol == "")
	{
		strFailReason = _(" TokenLabel's TokenSymbol is NULL");
		return false;
	}
	if (strTokenSymbol.length() > 8)
	{
		strFailReason = _(" IPCTokenLabel's TokenSymbol is longer than 8");
		return false;
	}
	strncpy((char*)(label.TokenSymbol), strTokenSymbol.c_str(), strTokenSymbol.length());

	o = find_value(JsonIpclabel, "automode");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The TokenLabel's automode error");
		return false;
	}
	label.addmode = o.get_int();
	if (label.addmode != 0 && label.addmode != 1)
	{
		strFailReason = _("The TokenLabel's automode error,no 0 or 1");
		return false;
	}
	o = find_value(JsonIpclabel, "hash");
	if (o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The TokenLabel's hash err");
		return false;
	}
	label.hash.SetHex(o.get_str());
	o = find_value(JsonIpclabel, "label");
	if (o.isNull() || o.type() != UniValue::VSTR)
	{
		strFailReason = _("The TokenLabel's label err");
		return false;
	}
	std::string strlabel = o.get_str();
	if (strlabel.length() > 16)
	{
		strFailReason = _(" TokenLabel's Label is longer than 16");
		return false;
	}
	strncpy((char*)(label.label), strlabel.c_str(), strlabel.length());
	o = find_value(JsonIpclabel, "issueDate");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The TokenLabel's issueDate err");
		return false;
	}
	label.issueDate = o.get_int32();
	o = find_value(JsonIpclabel, "totalCount");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The TokenLabel's totalCount err");
		return false;
	}
	label.totalCount = o.get_int64();
	if (label.totalCount < 0 )
	{
		strFailReason = _("The TokenLabel's totalCount err,can't smaller than 0.");
		return false;
	}
	o = find_value(JsonIpclabel, "accuracy");
	if (o.isNull() || o.type() != UniValue::VNUM)
	{
		strFailReason = _("The TokenLabel's accuracy err");
		return false;
	}
	label.accuracy = o.get_int();
	
	
	CAmount nDefalutFee = 100 * COIN;
	nValue += nDefalutFee;

	{
		set<pair<const CWalletTx*, unsigned int> > setCoins;
		LOCK2(cs_main, cs_wallet);
		{
			std::vector<COutput> vAvailableCoins;
			AvailableNormalCoins(vAvailableCoins, true, coinControl);
			if (!regaddressfirst(vAvailableCoins, vecSend)){
				strFailReason = _("Regaddress error or insufficient funds");
				return false;
			}
			nFeeRet = 0;
			// Start with no fee and loop until there is enough fee
			while (true)
			{
				nChangePosInOut = nChangePosRequest;
				txNew.vin.clear();
				txNew.vout.clear();
				wtxNew.fFromMe = true;

				CAmount nValueToSelect = nValue;
				if (nSubtractFeeFromAmount == 0)
					nValueToSelect += nFeeRet;
				double dPriority = 0;
				// vouts to the payees
				for (const auto& recipient : vecSend)
				{
					AddTokenLabel  outlabel = label;
					outlabel.height = recipient.height;
					outlabel.currentCount = recipient.tokenvalue;
					outlabel.extendinfo = recipient.extendinfo;
					CTxOut txout(CAmount(0), recipient.scriptPubKey, outlabel, recipient.txLabel);
					txNew.vout.push_back(txout);
					std::cout << "totalCount :"<<txout.addTokenLabel.totalCount<< std::endl;
					std::cout << "tokenvalue :" << txout.addTokenLabel.currentCount << std::endl;
				}

				// Choose coins to use
				CAmount nValueIn = 0;
				setCoins.clear();
				if (!SelectNormalCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
				{
					strFailReason = _("Insufficient funds");
					return false;
				}
				bool bFindChangScp = false;
				CScript scriptChangeFind;
				for (const auto& pcoin : setCoins)
				{
					if (!bFindChangScp)
					{
						scriptChangeFind = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
						bFindChangScp = true;
					}
					CAmount nCredit = pcoin.first->tx->vout[pcoin.second].nValue;
					//The coin age after the next block (depth+1) is used instead of the current,
					//reflecting an assumption the user would accept a bit more delay for
					//a chance at a free transaction.
					//But mempool inputs might still be in the mempool, so their age stays 0
					int age = pcoin.first->GetDepthInMainChain();
					assert(age >= 0);
					if (age != 0)
						age += 1;
					dPriority += (double)nCredit * age;
				}

				const CAmount nChange = nValueIn - nValueToSelect;
				if (nChange > 0)
				{
					// Fill a vout to ourself
					// TODO: pass in scriptChange instead of reservekey so
					// change transaction isn't always pay-to-ipchain-address
					CScript scriptChange;

					// coin control: send change to custom address
					if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
						scriptChange = GetScriptForDestination(coinControl->destChange);

					// no coin control: send change to newly generated address
					else
					{
						// Note: We use a new key here to keep it from being obvious which side is the change.
						//  The drawback is that by not reusing a previous key, the change may be lost if a
						//  backup is restored, if the backup doesn't have the new private key for the change.
						//  If we reused the old key, it would be possible to add code to look for and
						//  rediscover unknown transactions that were written with keys of ours to recover
						//  post-backup change.

						// Reserve a new key pair from key pool
						CPubKey vchPubKey;
						bool ret;
						ret = reservekey.GetReservedKey(vchPubKey);
						if (!ret)
						{
							strFailReason = _("Keypool ran out, please call keypoolrefill first");
							return false;
						}

						scriptChange = GetScriptForDestination(vchPubKey.GetID());
					}

					// Take the first input script as the destination output used for the coin change address
					if (bFindChangScp)
						scriptChange = scriptChangeFind;
					CTxOut newTxOut(nChange, scriptChange);

					// We do not move dust-change to fees, because the sender would end up paying more than requested.
					// This would be against the purpose of the all-inclusive feature.
					// So instead we raise the change and deduct from the recipient.
					if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(dustRelayFee))
					{
						CAmount nDust = newTxOut.GetDustThreshold(dustRelayFee) - newTxOut.nValue;
						newTxOut.nValue += nDust; // raise change until no more dust
					}

					// Never create dust outputs; if we would, just
					// add the dust to the fee.
					if (newTxOut.IsDust(dustRelayFee))
					{
						nChangePosInOut = -1;
						nFeeRet += nChange;
						reservekey.ReturnKey();
					}
					else
					{
						if (nChangePosInOut == -1)
						{
							// Insert change txn at end position:
							nChangePosInOut = txNew.vout.size();
						}
						else if ((unsigned int)nChangePosInOut > txNew.vout.size())
						{
							strFailReason = _("Change index out of range");
							return false;
						}

						vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
						txNew.vout.insert(position, newTxOut);
					}
				}
				else
					reservekey.ReturnKey();

				// Fill vin
				//
				// Note how the sequence number is set to non-maxint so that
				// the nLockTime set above actually works.
				//
				// BIP125 defines opt-in RBF as any nSequence < maxint-1, so
				// we use the highest possible value in that range (maxint-2)
				// to avoid conflicting with other possible uses of nSequence,
				// and in the spirit of "smallest possible change from prior
				// behavior."
				for (const auto& coin : setCoins)
					txNew.vin.push_back(CTxIn(coin.first->GetHash(), coin.second, CScript(),
					std::numeric_limits<unsigned int>::max() - (fWalletRbf ? 2 : 1)));

				// Fill in dummy signatures for fee calculation.
				if (!DummySignTx(txNew, setCoins)) {
					strFailReason = _("Signing transaction failed");
					return false;
				}

				unsigned int nBytes = GetVirtualTransactionSize(txNew);

				CTransaction txNewConst(txNew);
				dPriority = txNewConst.ComputePriority(dPriority, nBytes);

				// Remove scriptSigs to eliminate the fee calculation dummy signatures
				for (auto& vin : txNew.vin) {
					vin.scriptSig = CScript();
					vin.scriptWitness.SetNull();
				}

				// Allow to override the default confirmation target over the CoinControl instance
				int currentConfirmationTarget = nTxConfirmTarget;
				if (coinControl && coinControl->nConfirmTarget > 0)
					currentConfirmationTarget = coinControl->nConfirmTarget;

				// Can we complete this as a free transaction?
				if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
				{
					// Not enough fee: enough priority?
					double dPriorityNeeded = mempool.estimateSmartPriority(currentConfirmationTarget);
					// Require at least hard-coded AllowFree.
					if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
						break;
				}

				CAmount nFeeNeeded = GetMinimumFee(nBytes, currentConfirmationTarget, mempool);
				if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
					nFeeNeeded = coinControl->nMinimumTotalFee;
				}
				if (coinControl && coinControl->fOverrideFeeRate)
					nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

				// If we made it here and we aren't even able to meet the relay fee on the next pass, give up
				// because we must be at the maximum allowed fee.
				if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
				{
					strFailReason = _("Transaction too large for fee policy");
					return false;
				}

				if (nFeeRet >= nFeeNeeded) {
					// Reduce fee to only the needed amount if we have change
					// output to increase.  This prevents potential overpayment
					// in fees if the coins selected to meet nFeeNeeded result
					// in a transaction that requires less fee than the prior
					// iteration.
					// TODO: The case where nSubtractFeeFromAmount > 0 remains
					// to be addressed because it requires returning the fee to
					// the payees and not the change output.
					// TODO: The case where there is no change output remains
					// to be addressed so we avoid creating too small an output.
					if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
						CAmount extraFeePaid = nFeeRet - nFeeNeeded;
						vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
						change_position->nValue += extraFeePaid;
						nFeeRet -= extraFeePaid;
					}
					break; // Done, enough fee included.
				}

				// Try to reduce change to include necessary fee
				if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
					CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
					vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
					// Only reduce change if remaining amount is still a large enough output.
					if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
						change_position->nValue -= additionalFeeNeeded;
						nFeeRet += additionalFeeNeeded;
						break; // Done, able to increase fee from change
					}
				}

				// Include more fee and try again.
				nFeeRet = nFeeNeeded;
				continue;
			}
		}

		if (sign)
		{
			CTransaction txNewConst(txNew);
			int nIn = 0;
			for (const auto& coin : setCoins)
			{
				const CScript& scriptPubKey = coin.first->tx->vout[coin.second].scriptPubKey;
				SignatureData sigdata;

				if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->tx->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata))
				{
					strFailReason = _("Signing transaction failed");
					return false;
				}
				else {
					UpdateTransaction(txNew, nIn, sigdata);
				}

				nIn++;
			}
		}

		// Embed the constructed transaction data in wtxNew.
		wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));

		// Limit size
		if (GetTransactionWeight(wtxNew) >= MAX_STANDARD_TX_WEIGHT)
		{
			strFailReason = _("Transaction too large");
			return false;
		}
	}

	if (GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
		// Lastly, ensure this tx will pass the mempool's chain limits
		LockPoints lp;
		CTxMemPoolEntry entry(wtxNew.tx, 0, 0, 0, 0, 0, false, 0, lp);
		CTxMemPool::setEntries setAncestors;
		size_t nLimitAncestors = GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
		size_t nLimitAncestorSize = GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
		size_t nLimitDescendants = GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
		size_t nLimitDescendantSize = GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
		std::string errString;
		if (!mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
			strFailReason = _("Transaction has too long of a mempool chain");
			return false;
		}
	}
	return true;
}

CKeyPool::CKeyPool()
{
    nTime = GetTime();
}

CKeyPool::CKeyPool(const CPubKey& vchPubKeyIn)
{
    nTime = GetTime();
    vchPubKey = vchPubKeyIn;
}

CWalletKey::CWalletKey(int64_t nExpires)
{
    nTimeCreated = (nExpires ? GetTime() : 0);
    nTimeExpires = nExpires;
}

void CMerkleTx::SetMerkleBranch(const CBlockIndex* pindex, int posInBlock)
{
    // Update the tx's hashBlock
    hashBlock = pindex->GetBlockHash();

    // set the position of the transaction in the block
    nIndex = posInBlock;
}

int CMerkleTx::GetDepthInMainChain(const CBlockIndex* &pindexRet) const
{
    if (hashUnset())
        return 0;

    AssertLockHeld(cs_main);

    // Find the block it claims to be in
    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    pindexRet = pindex;
    return ((nIndex == -1) ? (-1) : 1) * (chainActive.Height() - pindex->nHeight + 1);
}

int64_t CMerkleTx::GetTimeOfTokenInChain() const
{
	/*const CBlockIndex* &pindexRet;*/
	if (hashUnset())
		return 0;

	AssertLockHeld(cs_main);

	// Find the block it claims to be in
	BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
	if (mi == mapBlockIndex.end())
		return 0;
	CBlockIndex* pindex = (*mi).second;
	if (!pindex || !chainActive.Contains(pindex))
		return 0;

	return pindex->GetBlockTime();
}

int CMerkleTx::GetBlocksToMaturity() const	
{
    if (!IsCoinBase()) 
        return 0;
    return max(0, (COINBASE_MATURITY+1) - GetDepthInMainChain());
}


bool CMerkleTx::AcceptToMemoryPool(const CAmount& nAbsurdFee, CValidationState& state)
{
	return ::AcceptToMemoryPool(mempool, state, tx, false, NULL, NULL, false, nAbsurdFee); //The fourth parameter is modified to false
}
