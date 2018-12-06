// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"

#include "hash.h"
#include "base58.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "util.h"

bool b_TestTxLarge = false;
std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10), n);
}

CTxIn::CTxIn(COutPoint prevoutIn, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = COutPoint(hashPrevTx, nOut);
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

std::string CTxIn::ToString() const
{
    std::string str;
    str += "CTxIn(";
    str += prevout.ToString();
    if (prevout.IsNull())
        str += strprintf(", coinbase %s", HexStr(scriptSig));
    else
        str += strprintf(", scriptSig=%s", HexStr(scriptSig).substr(0, 24));
    if (nSequence != SEQUENCE_FINAL)
        str += strprintf(", nSequence=%u", nSequence);
    str += ")";
    return str;
}

//Ordinary transaction constructor
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
	txType = TXOUT_NORMAL;
	coinbaseScript.clear();
	if (b_TestTxLarge)
	{
		txLabel = "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111";
		txLabelLen = txLabel.length();
	}
}
//Ordinary transaction constructor（Just called by CoinBase）
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, std::string coinbaseScriptIn)
{
	nValue = nValueIn;
	scriptPubKey = scriptPubKeyIn;
	txType = TXOUT_NORMAL;
	coinbaseScript = coinbaseScriptIn;
}

//Apply to join the transaction output constructor
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, DevoteLabel& devoteLableIn)
{
	SetNull();
	nValue = nValueIn;
	txType = TXOUT_CAMPAIGN;
	devoteLabel = devoteLableIn;
	scriptPubKey = scriptPubKeyIn;
}
//Apply to exit the transaction output constructor
CTxOut::CTxOut(const CAmount& nValueIn, DevoteLabel& devoteLableIn)
{
	SetNull();
	nValue = nValueIn;
	txType = TXOUT_CAMPAIGN;
	devoteLabel = devoteLableIn;
	scriptPubKey = GetScriptForDestination(CBitcoinAddress(Params().system_account_address).Get());
}

// the normal penalty transaction output constructor
// severe penalty transaction output constructor
CTxOut::CTxOut(CampaignType_t campaignType, uint160 campaignPubkeyHash)
{
	SetNull();
	nValue = 0;
	if (campaignType == TYPE_CONSENSUS_ORDINARY_PUSNISHMENT || campaignType == TYPE_CONSENSUS_SEVERE_PUNISHMENT)
	{
		txType = TXOUT_CAMPAIGN;
		devoteLabel.ExtendType = (uint8_t)campaignType;
		devoteLabel.hash = campaignPubkeyHash;
		scriptPubKey = GetScriptForDestination(CBitcoinAddress(Params().system_account_address).Get());
	}
}
// severe penalties for the application of trade constructors
CTxOut::CTxOut(uint160 campaignPubkeyHash, const std::string evidence)
{
	SetNull();
	nValue = 0;
	txType = TXOUT_CAMPAIGN;
	devoteLabel.ExtendType = (uint8_t)TYPE_CONSENSUS_SEVERE_PUNISHMENT_REQUEST;
	devoteLabel.hash = campaignPubkeyHash;
	scriptPubKey = GetScriptForDestination(CBitcoinAddress(Params().system_account_address).Get());
	LogPrintf("[CTxOut::CTxOut Severe punishment application] evidence=%s\n", evidence.c_str());
	txLabel = evidence;
}


//The refund transaction output constructor
CTxOut::CTxOut(const CAmount& refund, uint160 campaignPubkeyHash)
{
	SetNull();
	nValue = refund;
	txType = TXOUT_CAMPAIGN;
	devoteLabel.ExtendType = 4;
	devoteLabel.hash = campaignPubkeyHash;
	scriptPubKey = GetScriptForDestination(CKeyID(campaignPubkeyHash));
}

//The constructor of the ownership transaction
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, TransactionOutType_t  txTypeIn, IPCLabel& labelIn, std::string  voutLabel)
{
	SetNull();
	nValue = nValueIn;
	scriptPubKey = scriptPubKeyIn;
	txType = txTypeIn;
	labelLen = ipcLabel.size();
	ipcLabel = labelIn;
	txLabel = voutLabel;
}
//Ownership structure
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, TransactionOutType_t txTypeIn, IPCLabel& labelIn, std::string voutLabel, std::string ipcinfo)
{
	SetNull();
	nValue = nValueIn;
	scriptPubKey = scriptPubKeyIn;
	txType = txTypeIn;
	labelLen = ipcLabel.size();
	ipcLabel = labelIn;
	txLabel = voutLabel;
	ipcLabelinfo = ipcinfo;
}
//The construction function of scrip registration transaction
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, TokenRegLabel& labelIn, std::string voutLabel)
{
	SetNull();
	nValue = nValueIn;
	scriptPubKey = scriptPubKeyIn;
	txType = TXOUT_TOKENREG;
	tokenRegLabel = labelIn;
	txLabel = voutLabel;
}

//The constructor of a token trade
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, TokenLabel& labelIn, std::string voutLabel)
{
	SetNull();
	nValue = nValueIn;
	scriptPubKey = scriptPubKeyIn;
	txType = TXOUT_TOKEN;
	tokenLabel = labelIn;
	txLabel = voutLabel;
}



//Get the transaction subtype of the campaign
CampaignType_t CTxOut::GetCampaignType() const
{
	if (txType != 1)
		return TYPE_CONSENSUS_NULL;
	
	return (CampaignType_t)devoteLabel.ExtendType;
}


std::string CTxOut::ToString() const
{
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30));
}

uint64_t CTxOut::GetTokenvalue() const{
    if (txType == TXOUT_TOKENREG)
    {
        return tokenRegLabel.totalCount;
    }
    else if (txType == TXOUT_TOKEN)
    {
        return tokenLabel.value;
    }
    return uint64_t(0);
}

uint8_t CTxOut::getTokenaccuracy() const
{
	if (txType == TXOUT_TOKENREG)
	{
		return tokenRegLabel.accuracy;
	}
	else if (txType == TXOUT_TOKEN)
	{
		return tokenLabel.accuracy;
	}
	return uint8_t(0);
}


CMutableTransaction::CMutableTransaction() : nVersion(CTransaction::CURRENT_VERSION), nLockTime(0) {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : nVersion(tx.nVersion), vin(tx.vin), vout(tx.vout), nLockTime(tx.nLockTime) {}

uint256 CMutableTransaction::GetHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::ComputeHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::GetWitnessHash() const
{
    if (!HasWitness()) {
        return GetHash();
    }
    return SerializeHash(*this, SER_GETHASH, 0);
}

/* For backward compatibility, the hash is initialized to 0. TODO: remove the need for this default constructor entirely. */
CTransaction::CTransaction() : nVersion(CTransaction::CURRENT_VERSION), vin(), vout(), nLockTime(0), hash() {}
CTransaction::CTransaction(const CMutableTransaction &tx) : nVersion(tx.nVersion), vin(tx.vin), vout(tx.vout), nLockTime(tx.nLockTime), hash(ComputeHash()) {}
CTransaction::CTransaction(CMutableTransaction &&tx) : nVersion(tx.nVersion), vin(std::move(tx.vin)), vout(std::move(tx.vout)), nLockTime(tx.nLockTime), hash(ComputeHash()) {}

CAmount CTransaction::GetValueOut() const
{
    CAmount nValueOut = 0;
    for (std::vector<CTxOut>::const_iterator it(vout.begin()); it != vout.end(); ++it)
    {
        nValueOut += it->nValue;
        if (!MoneyRange(it->nValue) || !MoneyRange(nValueOut))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nValueOut;
}

//Acquisition transaction type
uint8_t CTransaction::GetTxType() const
{
	for (uint32_t i = 0; i < vout.size(); i++)
	{
		if (vout[i].txType != TXOUT_NORMAL)
			return vout[i].txType;
	}
	return 0;
}

//Get the transaction subtype of the campaign
CampaignType_t CTransaction::GetCampaignType() const
{

	for (uint32_t i = 0; i < vout.size(); i++)
	{
		if (vout[i].txType == TXOUT_CAMPAIGN)
			return vout[i].GetCampaignType();
	}
	return TYPE_CONSENSUS_NULL;
}

//Get the candidate pubkeyHash
uint160 CTransaction::GetCampaignPublickey() const
{

	for (uint32_t i = 0; i < vout.size(); i++)
	{
		if (vout[i].txType == TXOUT_CAMPAIGN)
			return vout[i].devoteLabel.hash;
	}
	return uint160();

}

//Get a campaign registration payment deposit
CAmount CTransaction::GetRegisterIPC() const
{

	for (uint32_t i = 0; i < vout.size(); i++)
	{
		if (vout[i].txType == TXOUT_CAMPAIGN && vout[i].devoteLabel.ExtendType == TYPE_CONSENSUS_REGISTER)
			return vout[i].nValue;
	}
	return TYPE_CONSENSUS_NULL;
}

std::string CTransaction::GetCampaignEvidence() const
{
	for (uint32_t i = 0; i < vout.size(); i++)
	{
		if (vout[i].txType == TXOUT_CAMPAIGN && vout[i].devoteLabel.ExtendType == TYPE_CONSENSUS_SEVERE_PUNISHMENT_REQUEST)
			return vout[i].txLabel;
	}
	return "";
}


double CTransaction::ComputePriority(double dPriorityInputs, unsigned int nTxSize) const
{
    nTxSize = CalculateModifiedSize(nTxSize);
    if (nTxSize == 0) return 0.0;

    return dPriorityInputs / nTxSize;
}

unsigned int CTransaction::CalculateModifiedSize(unsigned int nTxSize) const
{
    // In order to avoid disincentivizing cleaning up the UTXO set we don't count
    // the constant overhead for each txin and up to 110 bytes of scriptSig (which
    // is enough to cover a compressed pubkey p2sh redemption) for priority.
    // Providing any more cleanup incentive than making additional inputs free would
    // risk encouraging people to create junk outputs to redeem later.
    if (nTxSize == 0)
        nTxSize = (GetTransactionWeight(*this) + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR;
    for (std::vector<CTxIn>::const_iterator it(vin.begin()); it != vin.end(); ++it)
    {
        unsigned int offset = 41U + std::min(110U, (unsigned int)it->scriptSig.size());
        if (nTxSize > offset)
            nTxSize -= offset;
    }
    return nTxSize;
}

unsigned int CTransaction::GetTotalSize() const
{
    return ::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION);
}

std::string CTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, vin.size=%u, vout.size=%u, nLockTime=%u)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        vin.size(),
        vout.size(),
        nLockTime);
    for (unsigned int i = 0; i < vin.size(); i++)
        str += "    " + vin[i].ToString() + "\n";
    for (unsigned int i = 0; i < vin.size(); i++)
        str += "    " + vin[i].scriptWitness.ToString() + "\n";
    for (unsigned int i = 0; i < vout.size(); i++)
        str += "    " + vout[i].ToString() + "\n";
    return str;
}

int64_t GetTransactionWeight(const CTransaction& tx)
{
    return ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR -1) + ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
}
