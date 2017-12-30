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

//普通交易构造函数
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
	txType = TXOUT_NORMAL;
	coinbaseScript.clear();
}
//普通交易构造函数（仅由CoinBase调用）
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, std::string coinbaseScriptIn)
{
	nValue = nValueIn;
	scriptPubKey = scriptPubKeyIn;
	txType = TXOUT_NORMAL;
	coinbaseScript = coinbaseScriptIn;
}

//申请加入交易输出构造函数
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, DevoteLabel& devoteLableIn)
{
	SetNull();
	nValue = nValueIn;
	txType = TXOUT_CAMPAIGN;
	devoteLabel = devoteLableIn;
	scriptPubKey = scriptPubKeyIn;
}
//申请退出交易输出构造函数
CTxOut::CTxOut(const CAmount& nValueIn, DevoteLabel& devoteLableIn)
{
	SetNull();
	nValue = nValueIn;
	txType = TXOUT_CAMPAIGN;
	devoteLabel = devoteLableIn;
	scriptPubKey = GetScriptForDestination(CBitcoinAddress(Params().system_account_address).Get());
}

//普通处罚交易输出构造函数
//严重处罚交易输出构造函数
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
//严重处罚申请交易构造函数
CTxOut::CTxOut(uint160 campaignPubkeyHash, const std::string evidence)
{
	SetNull();
	nValue = 0;
	txType = TXOUT_CAMPAIGN;
	devoteLabel.ExtendType = (uint8_t)TYPE_CONSENSUS_SEVERE_PUNISHMENT_REQUEST;
	devoteLabel.hash = campaignPubkeyHash;
	scriptPubKey = GetScriptForDestination(CBitcoinAddress(Params().system_account_address).Get());
	LogPrintf("[CTxOut::CTxOut 严重处罚申请] 证据=%s\n", evidence.c_str());
	txLabel = evidence;
}


//退款交易输出构造函数
CTxOut::CTxOut(const CAmount& refund, uint160 campaignPubkeyHash)
{
	SetNull();
	nValue = refund;
	txType = TXOUT_CAMPAIGN;
	devoteLabel.ExtendType = 4;
	devoteLabel.hash = campaignPubkeyHash;
	scriptPubKey = GetScriptForDestination(CKeyID(campaignPubkeyHash));
}

//所有权交易的构造函数
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

//代币登记交易的构造函数
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, TokenRegLabel& labelIn, std::string voutLabel)
{
	SetNull();
	nValue = nValueIn;
	scriptPubKey = scriptPubKeyIn;
	txType = TXOUT_TOKENREG;
	tokenRegLabel = labelIn;
	txLabel = voutLabel;
}

//代币交易的构造函数
CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, TokenLabel& labelIn, std::string voutLabel)
{
	SetNull();
	nValue = nValueIn;
	scriptPubKey = scriptPubKeyIn;
	txType = TXOUT_TOKEN;
	tokenLabel = labelIn;
	txLabel = voutLabel;
}



//获取竞选交易的交易子类型
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

//获取交易类型
uint8_t CTransaction::GetTxType() const
{
	//遍历当前交易的vout，取第一个特殊类型为交易类型返回。如没有特殊类型，返回普通交易类型
	for (uint32_t i = 0; i < vout.size(); i++)
	{
		if (vout[i].txType != 0)
			return vout[i].txType;
	}
	return 0;
}

//获取竞选交易的交易子类型
CampaignType_t CTransaction::GetCampaignType() const
{
	//if (GetTxType() != 1)
	//	return TYPE_CONSENSUS_NULL;

	//遍历全部vout，取第一个竞选类型交易vout的CampaignType返回
	for (uint32_t i = 0; i < vout.size(); i++)
	{
		if (vout[i].txType == 1)
			return vout[i].GetCampaignType();
	}
	return TYPE_CONSENSUS_NULL;
}

//获取竞选者pubkeyHash
uint160 CTransaction::GetCampaignPublickey() const
{
	////如为竞选交易，除CoinBase之外只应有一个输出，返回该输出的hash即可
	//return vout[0].GetCampaignHash();

	//遍历全部vout，取第一个竞选类型交易vout的CampaignType返回
	for (uint32_t i = 0; i < vout.size(); i++)
	{
		if (vout[i].txType == 1)
			return vout[i].devoteLabel.hash;
	}
	return uint160();

}

//获取竞选注册支付押金
CAmount CTransaction::GetRegisterIPC() const
{
	//if (GetTxType() != 1 || vout[0].devoteLabel.ExtendType != 0)
	//	return -1;
	//
	////如为竞选交易加入申请，只应有一个输出，返回该输出的hash即可
	//return vout[0].nValue;

	//遍历全部vout，取第一个竞选类型交易vout的CampaignType返回
	for (uint32_t i = 0; i < vout.size(); i++)
	{
		if (vout[i].txType == 1 && vout[i].devoteLabel.ExtendType == TYPE_CONSENSUS_REGISTER)
			return vout[i].nValue;
	}
	return TYPE_CONSENSUS_NULL;
}

std::string CTransaction::GetCampaignEvidence() const
{
	//遍历全部vout，取第一个竞选类型交易vout的CampaignType返回
	for (uint32_t i = 0; i < vout.size(); i++)
	{
		if (vout[i].txType == 1 && vout[i].devoteLabel.ExtendType == TYPE_CONSENSUS_SEVERE_PUNISHMENT_REQUEST)
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
