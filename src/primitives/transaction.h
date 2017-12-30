// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_TRANSACTION_H
#define BITCOIN_PRIMITIVES_TRANSACTION_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"

static const int SERIALIZE_TRANSACTION_NO_WITNESS = 0x40000000;

static const int WITNESS_SCALE_FACTOR = 4;

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    uint32_t n;

    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, uint32_t nIn) { hash = hashIn; n = nIn; }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(hash);
        READWRITE(n);
    }

    void SetNull() { hash.SetNull(); n = (uint32_t) -1; }
    bool IsNull() const { return (hash.IsNull() && n == (uint32_t) -1); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        int cmp = a.hash.Compare(b.hash);
        return cmp < 0 || (cmp == 0 && a.n < b.n);
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;
    CScriptWitness scriptWitness; //! Only serialized through CTransaction

    /* Setting nSequence to this value for every input in a transaction
     * disables nLockTime. */
    static const uint32_t SEQUENCE_FINAL = 0xffffffff;

    /* Below flags apply in the context of BIP 68*/
    /* If this flag set, CTxIn::nSequence is NOT interpreted as a
     * relative lock-time. */
    static const uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1 << 31);

    /* If CTxIn::nSequence encodes a relative lock-time and this flag
     * is set, the relative lock-time has units of 512 seconds,
     * otherwise it specifies blocks with a granularity of 1. */
    static const uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);

    /* If CTxIn::nSequence encodes a relative lock-time, this mask is
     * applied to extract that lock-time from the sequence field. */
    static const uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;

    /* In order to use the same number of bits to encode roughly the
     * same wall-clock duration, and because blocks are naturally
     * limited to occur every 600s on average, the minimum granularity
     * for time-based relative lock-time is fixed at 512 seconds.
     * Converting from CTxIn::nSequence to seconds is performed by
     * multiplying by 512 = 2^9, or equivalently shifting up by
     * 9 bits. */
    static const int SEQUENCE_LOCKTIME_GRANULARITY = 9;

    CTxIn()
    {
        nSequence = SEQUENCE_FINAL;
    }

    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(prevout);
        READWRITE(*(CScriptBase*)(&scriptSig));
        READWRITE(nSequence);
    }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

enum TransactionOutType_t{
	TXOUT_INVALID = -1,  //��Ч
	TXOUT_NORMAL = 0,   //  ��ͨ�������
	TXOUT_CAMPAIGN = 1,   // ��ѡģ���������
	TXOUT_IPCOWNER = 2,  //  ����Ȩ�������
	TXOUT_IPCAUTHORIZATION = 3,  // ��Ȩ�������
	TXOUT_TOKENREG = 4, // ���ҵǼ��������
	TXOUT_TOKEN = 5 // �����������
};

//IPC����ṹ�е�֪ʶ��Ȩ��ǩ��
class IPCLabel
{
public:
	uint8_t ExtendType; //��չ����
	uint32_t startTime; //��ʼʱ�� 4�ֽ�
	uint32_t stopTime; //��ֹʱ�� 4�ֽ�
	uint8_t reAuthorize; //����Ȩ��־λ
	uint8_t uniqueAuthorize; //Ψһ��Ȩ��־λ
	uint8_t hashLen = 16; //֪������HASHֵ���ȡ�MD5��ʽ�̶�Ϊ16�ֽڼ�128bit
	uint128 hash; //֪������HASHֵ
	std::string labelTitle; //֪�����ݱ���

	IPCLabel() { SetNull(); }

	//�������л����ʱ��������Label����
	uint32_t size() const { return 1 + 4 + 4 + 1 + 1 + 1 + 16 + 1 + labelTitle.size(); };
	ADD_SERIALIZE_METHODS;

	void SetNull() { hash.SetNull(); ExtendType = (uint8_t)-1; }
	bool IsNull() const { return (hash.IsNull() && ExtendType == (uint8_t)-1); }

	 void operator=(const IPCLabel& a)
	{
		ExtendType = a.ExtendType;
		startTime = a.startTime;
		stopTime = a.stopTime;
		reAuthorize = a.reAuthorize;
		uniqueAuthorize = a.uniqueAuthorize;
		hash = a.hash;
		labelTitle = a.labelTitle;
	}

	friend bool operator==(const IPCLabel& a, const IPCLabel& b)
	{
		return (a.ExtendType == b.ExtendType &&
			a.startTime == b.startTime &&
			a.stopTime == b.stopTime &&
			a.reAuthorize == b.reAuthorize &&
			a.uniqueAuthorize == b.uniqueAuthorize &&
			a.hash == b.hash &&
			a.labelTitle == b.labelTitle);
	}


	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(ExtendType);
		READWRITE(startTime);
		READWRITE(stopTime);
		READWRITE(reAuthorize);
		READWRITE(uniqueAuthorize);
		READWRITE(hashLen);
		READWRITE(hash);
		READWRITE(labelTitle);
	}
};

enum CampaignType_t{
	TYPE_CONSENSUS_NULL = -1,  //���Ǿ�ѡ����
	TYPE_CONSENSUS_REGISTER = 0,   //   �������
	TYPE_CONSENSUS_QUITE = 1,   //  �����˳�
	TYPE_CONSENSUS_ORDINARY_PUSNISHMENT = 2,  //  Ordinary punishment��ͨ����
	TYPE_CONSENSUS_SEVERE_PUNISHMENT = 3,  // severe punishment ���ش���
	TYPE_CONSENSUS_RETURN_DEPOSI = 4, // Return the deposi   ����Ѻ��
	TYPE_CONSENSUS_SEVERE_PUNISHMENT_REQUEST = 5 //���ش�������
};

//IPC����ṹ�еľ�ѡ��ǩ��
class DevoteLabel
{
public:
	uint8_t ExtendType; //��չ����
	uint160 hash; //��ѡ��PubKey HASH

	DevoteLabel() { SetNull(); }

	//�������л����ʱ��������Label����
	uint32_t size() const { return 1+sizeof(uint160); };
	ADD_SERIALIZE_METHODS;

	void SetNull() { hash.SetNull(); ExtendType = (uint8_t)-1; }
	bool IsNull() const { return (hash.IsNull() && ExtendType == (uint8_t)-1); }

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(ExtendType);
		READWRITE(hash);
	}

	void operator=(const DevoteLabel& a)
	{
		ExtendType = a.ExtendType;
		hash = a.hash;
	}
};

//IPC����ṹ�еĴ��ҵǼǱ�ǩ��
class TokenRegLabel
{
public:
	uint8_t TokenSymbol[9]; //����Symbol��unique��
	uint64_t value; //��������--?
	uint128 hash; //����HASH��ǩ��MD5
	uint8_t label[17]; //���ұ�ǩ
	uint32_t issueDate; //���ҷ���ʱ�� ʱ��
	uint64_t totalCount; //���ҷ�������
	uint8_t  accuracy;//����
	TokenRegLabel() { SetNull(); }

	//�������л����ʱ��������Label����
	uint32_t size() const { return 8+8+16+16+4+8+1; };

	//����decodeʱ��������ַ���
	std::string getTokenSymbol() const { return std::string((char*)TokenSymbol); };
	std::string getTokenLabel() const { return std::string((char*)label); };

	ADD_SERIALIZE_METHODS;

	void SetNull() { hash.SetNull(); value = 0; memset(TokenSymbol, 0x00, 9); memset(label, 0x00, 17); }
	bool IsNull() const { return (hash.IsNull() && value == 0); }

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(TokenSymbol[0]);
		READWRITE(TokenSymbol[1]);
		READWRITE(TokenSymbol[2]);
		READWRITE(TokenSymbol[3]);
		READWRITE(TokenSymbol[4]);
		READWRITE(TokenSymbol[5]);
		READWRITE(TokenSymbol[6]);
		READWRITE(TokenSymbol[7]);
		READWRITE(value);
		READWRITE(hash);
		READWRITE(label[0]);
		READWRITE(label[1]);
		READWRITE(label[2]);
		READWRITE(label[3]);
		READWRITE(label[4]);
		READWRITE(label[5]);
		READWRITE(label[6]);
		READWRITE(label[7]);
		READWRITE(label[8]);
		READWRITE(label[9]);
		READWRITE(label[10]);
		READWRITE(label[11]);
		READWRITE(label[12]);
		READWRITE(label[13]);
		READWRITE(label[14]);
		READWRITE(label[15]);
		READWRITE(issueDate);
		READWRITE(totalCount);
		READWRITE(accuracy);
	}
};

//IPC����ṹ�еĴ��ұ�ǩ��
class TokenLabel
{
public:
	uint8_t TokenSymbol[9]; //����Symbol��unique��
	uint64_t value; //��������
	uint8_t  accuracy;//����
	TokenLabel() { SetNull(); }

	//�������л����ʱ��������Label����
	uint32_t size() const { return 8 + 8 + 1; };

	//����decodeʱ��������ַ���
	std::string getTokenSymbol() const { return std::string((char*)TokenSymbol); };

	ADD_SERIALIZE_METHODS;

	void SetNull() { value = 0; memset(TokenSymbol, 0x00, 9); }
	bool IsNull() const { return value == 0; }

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(TokenSymbol[0]);
		READWRITE(TokenSymbol[1]);
		READWRITE(TokenSymbol[2]);
		READWRITE(TokenSymbol[3]);
		READWRITE(TokenSymbol[4]);
		READWRITE(TokenSymbol[5]);
		READWRITE(TokenSymbol[6]);
		READWRITE(TokenSymbol[7]);
		READWRITE(value);
		READWRITE(accuracy);
	}

};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
public:
    CAmount nValue;
	uint8_t txType; //�����UTXO���͡�0Ϊ��ͨ���ף�1Ϊ��ѡ���ף�2Ϊ����Ȩ���ͣ�3Ϊ��Ȩ����
	std::string coinbaseScript = "";//���ֶν���CoinBase����ڿ���ʱʹ�ã���Ϊ��ͨ���ס�
	uint8_t labelLen; //֪����ǩ���ȣ����л����ʱʹ�ã�
	IPCLabel ipcLabel; //֪����ǩ����
	DevoteLabel devoteLabel; //��ѡ��ǩ����
	TokenRegLabel tokenRegLabel; //���ҵǼǱ�ǩ����
	TokenLabel tokenLabel; //���ұ�ǩ����
    CScript scriptPubKey;
	uint16_t txLabelLen = 0; //��ע/ת�峤��
	std::string txLabel; //��ע/ת��

    CTxOut()
    {
        SetNull();
    }
	//��ͨ����������캯��
    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn);

	//CoinBase����������캯��
	CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, std::string coinbaseScriptIn);

	//CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, uint8_t txTypeIn);

	//������뽻��������캯��
	CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn,DevoteLabel& devoteLableIn);
	//�����˳�����������캯��
	CTxOut(const CAmount& nValueIn, DevoteLabel& devoteLableIn);

	//��ͨ��������������캯��
	//���ش�������������캯��
	CTxOut(CampaignType_t campaignType, uint160 campaignPubkeyHash);

	//���ش����������빹�캯��
	CTxOut(uint160 campaignPubkeyHash, const std::string evidence);

	//�˿��������캯��
	CTxOut(const CAmount& refund, uint160 campaignPubkeyHash);

	//����Ȩ/��Ȩ��������Ĺ��캯��
	CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, TransactionOutType_t txTypeIn, IPCLabel& lableIn,std::string voutLabel="");

	//���ҵǼǽ�������Ĺ��캯��
	CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, TokenRegLabel& regLableIn, std::string voutLabel = "");

	//���ҽ�������Ĺ��캯��
	CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, TokenLabel& tokenLableIn, std::string voutLabel = "");

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
		
        READWRITE(nValue);
		READWRITE(txType);
		switch (txType)
		{

		case 1://���ʽ��ף����׶��ֶ���ֵ��������������Ϊ0
			labelLen = devoteLabel.size();
			READWRITE(labelLen);
			if (labelLen > 0)
			{
				READWRITE(devoteLabel);
			}
// 			READWRITE(*(CScriptBase*)(&scriptPubKey));
// 			READWRITE(txLabel);
			
			break;

		case 2://����Ȩ����
		case 3://��Ȩ����
			labelLen = ipcLabel.size();
			READWRITE(labelLen);
			if (labelLen > 0)
			{
				READWRITE(ipcLabel);
			}
// 			READWRITE(*(CScriptBase*)(&scriptPubKey));
// 			READWRITE(txLabel);

			break;

		case 4:
			labelLen = tokenRegLabel.size();
			READWRITE(labelLen);
			if (labelLen > 0)
			{
				READWRITE(tokenRegLabel);
			}
// 			READWRITE(*(CScriptBase*)(&scriptPubKey));
// 			READWRITE(txLabel);
			break;

		case 5:
			labelLen = tokenLabel.size();
			READWRITE(labelLen);
			if (labelLen > 0)
			{
				READWRITE(tokenLabel);
			}
// 			READWRITE(*(CScriptBase*)(&scriptPubKey));
// 			READWRITE(txLabel);
			break;

		default:
		case 0://��ͨ���ף������������ݳ��Ⱦ�Ϊ0
			READWRITE(coinbaseScript);
			//labelLen = 0;
			//READWRITE(labelLen);
// 			READWRITE(*(CScriptBase*)(&scriptPubKey));
// 			txLabelLen = 0;
// 			READWRITE(txLabelLen);

			break;
		}
	
		READWRITE(*(CScriptBase*)(&scriptPubKey));
		READWRITE(txLabel);
		txLabelLen = txLabel.size();		//add by xxy 20171014 
    }

    void SetNull()
    {
        nValue = -1;
		txType = TXOUT_INVALID;
		labelLen = 0;
        scriptPubKey.clear();
		txLabel.clear();
		coinbaseScript.clear();
    }

    bool IsNull() const
    {
        return (nValue == -1);
    }

	//��ȡ��ѡ���׵Ľ���������
	CampaignType_t GetCampaignType() const;
	std::string GetCheckBlockContent() const{ return coinbaseScript; };
	uint160 GetCampaignHash() const{ return devoteLabel.hash; };

    CAmount GetDustThreshold(const CFeeRate &minRelayTxFee) const
    {
		//for IPC or Token, we allow the IPC/Token UTXO's value = 0
		if (txType != 0)
			return 0;

        // "Dust" is defined in terms of CTransaction::minRelayTxFee,
        // which has units satoshis-per-kilobyte.
        // If you'd pay more than 1/3 in fees
        // to spend something, then we consider it dust.
        // A typical spendable non-segwit txout is 34 bytes big, and will
        // need a CTxIn of at least 148 bytes to spend:
        // so dust is a spendable txout less than
        // 546*minRelayTxFee/1000 (in satoshis).
        // A typical spendable segwit txout is 31 bytes big, and will
        // need a CTxIn of at least 67 bytes to spend:
        // so dust is a spendable txout less than
        // 294*minRelayTxFee/1000 (in satoshis).
        if (scriptPubKey.IsUnspendable())
            return 0;

        size_t nSize = GetSerializeSize(*this, SER_DISK, 0);
        int witnessversion = 0;
        std::vector<unsigned char> witnessprogram;

        if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            // sum the sizes of the parts of a transaction input
            // with 75% segwit discount applied to the script size.
            nSize += (32 + 4 + 1 + (107 / WITNESS_SCALE_FACTOR) + 4);
        } else {
            nSize += (32 + 4 + 1 + 107 + 4); // the 148 mentioned above
        }

        return 3 * minRelayTxFee.GetFee(nSize);
    }

    bool IsDust(const CFeeRate &minRelayTxFee) const
    {
        return (nValue < GetDustThreshold(minRelayTxFee));
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

struct CMutableTransaction;

/**
 * Basic transaction serialization format:
 * - int32_t nVersion
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - uint32_t nLockTime
 *
 * Extended transaction serialization format:
 * - int32_t nVersion
 * - unsigned char dummy = 0x00
 * - unsigned char flags (!= 0)
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - if (flags & 1):
 *   - CTxWitness wit;
 * - uint32_t nLockTime
 */
template<typename Stream, typename TxType>
inline void UnserializeTransaction(TxType& tx, Stream& s) {
	//����ı��λ�Ǹ�����֤�����������汾���й�ϵ�������ǵĴ��벻��Ҫ��������֤���ݣ����Կ���ֱ��ǿ����Ϊ��Ҫ������֤���ܹ��汾�Ÿ��ĵķ��ա� edit by Millieen 20170729
	//���ں�������Ҫ�ο��Ĵ����ǻ���0.12�汾�ģ�����û�и�����֤����������Ҳ�ùص�������֤��edit by Millieen 20170831
	//const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);
	const bool fAllowWitness = false;

    s >> tx.nVersion;
    unsigned char flags = 0;
    tx.vin.clear();
    tx.vout.clear();
    /* Try to read the vin. In case the dummy is there, this will be read as an empty vector. */
    s >> tx.vin;
    if (tx.vin.size() == 0 && fAllowWitness) {
        /* We read a dummy or an empty vin. */
        s >> flags;
        if (flags != 0) {
            s >> tx.vin;
            s >> tx.vout;
        }
    } else {
        /* We read a non-empty vin. Assume a normal vout follows. */
        s >> tx.vout;
    }
    if ((flags & 1) && fAllowWitness) {
        /* The witness flag is present, and we support witnesses. */
        flags ^= 1;
        for (size_t i = 0; i < tx.vin.size(); i++) {
            s >> tx.vin[i].scriptWitness.stack;
        }
    }
    if (flags) {
        /* Unknown flag in the serialization */
        throw std::ios_base::failure("Unknown transaction optional data");
    }
    s >> tx.nLockTime;
}

template<typename Stream, typename TxType>
inline void SerializeTransaction(const TxType& tx, Stream& s) {	
	//����ı��λ�Ǹ�����֤�����������汾���й�ϵ�������ǵĴ��벻��Ҫ��������֤���ݣ����Կ���ֱ��ǿ����Ϊ��Ҫ������֤���ܹ��汾�Ÿ��ĵķ��ա� edit by Millieen 20170729
	//���ں�������Ҫ�ο��Ĵ����ǻ���0.12�汾�ģ�����û�и�����֤����������Ҳ�ùص�������֤��edit by Millieen 20170831
	//const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);
	const bool fAllowWitness = false;

    s << tx.nVersion;
    unsigned char flags = 0;
    // Consistency check
    if (fAllowWitness) {
        /* Check whether witnesses need to be serialized. */
        if (tx.HasWitness()) {
            flags |= 1;
        }
    }
    if (flags) {
        /* Use extended format in case witnesses are to be serialized. */
        std::vector<CTxIn> vinDummy;
        s << vinDummy;
        s << flags;
    }
    s << tx.vin;
    s << tx.vout;
    if (flags & 1) {
        for (size_t i = 0; i < tx.vin.size(); i++) {
            s << tx.vin[i].scriptWitness.stack;
        }
    }
    s << tx.nLockTime;
}


/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class CTransaction
{
public:
    // Default transaction version.
    static const int32_t CURRENT_VERSION=2;

    // Changing the default transaction version requires a two step process: first
    // adapting relay policy by bumping MAX_STANDARD_VERSION, and then later date
    // bumping the default CURRENT_VERSION at which point both CURRENT_VERSION and
    // MAX_STANDARD_VERSION will be equal.
    static const int32_t MAX_STANDARD_VERSION=2;

    // The local variables are made const to prevent unintended modification
    // without updating the cached hash value. However, CTransaction is not
    // actually immutable; deserialization and assignment are implemented,
    // and bypass the constness. This is safe, as they update the entire
    // structure, including the hash.
    const int32_t nVersion;
    const std::vector<CTxIn> vin;
    const std::vector<CTxOut> vout;
    const uint32_t nLockTime;

private:
    /** Memory only. */
    const uint256 hash;

    uint256 ComputeHash() const;

public:
    /** Construct a CTransaction that qualifies as IsNull() */
    CTransaction();

    /** Convert a CMutableTransaction into a CTransaction. */
    CTransaction(const CMutableTransaction &tx);
    CTransaction(CMutableTransaction &&tx);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }

    /** This deserializing constructor is provided instead of an Unserialize method.
     *  Unserialize is not possible, since it would require overwriting const fields. */
    template <typename Stream>
    CTransaction(deserialize_type, Stream& s) : CTransaction(CMutableTransaction(deserialize, s)) {}

    bool IsNull() const {
        return vin.empty() && vout.empty();
    }

    const uint256& GetHash() const {
        return hash;
    }

    // Compute a hash that includes both transaction and witness data
    uint256 GetWitnessHash() const;

    // Return sum of txouts.
    CAmount GetValueOut() const;
    // GetValueIn() is a method on CCoinsViewCache, because
    // inputs must be known to compute value in.

	//��ȡ��������
	uint8_t GetTxType() const;
	//��ȡ��ѡ���׵Ľ���������
	CampaignType_t GetCampaignType() const;
	//��ȡ��ѡ��pubkeyHash
	uint160 GetCampaignPublickey() const;
	//��ȡ��ѡע��֧��Ѻ��
	CAmount GetRegisterIPC() const;
	//��ȡ�ͷ�����֤��
	std::string GetCampaignEvidence() const;

    // Compute priority, given priority of inputs and (optionally) tx size
    double ComputePriority(double dPriorityInputs, unsigned int nTxSize=0) const;

    // Compute modified tx size for priority calculation (optionally given tx size)
    unsigned int CalculateModifiedSize(unsigned int nTxSize=0) const;

    /**
     * Get the total transaction size in bytes, including witness data.
     * "Total Size" defined in BIP141 and BIP144.
     * @return Total transaction size in bytes
     */
    unsigned int GetTotalSize() const;

    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return a.hash != b.hash;
    }

    std::string ToString() const;

    bool HasWitness() const
    {
        for (size_t i = 0; i < vin.size(); i++) {
            if (!vin[i].scriptWitness.IsNull()) {
                return true;
            }
        }
        return false;
    }
};

/** A mutable version of CTransaction. */
struct CMutableTransaction
{
    int32_t nVersion;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    uint32_t nLockTime;

    CMutableTransaction();
    CMutableTransaction(const CTransaction& tx);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }


    template <typename Stream>
    inline void Unserialize(Stream& s) {
        UnserializeTransaction(*this, s);
    }

    template <typename Stream>
    CMutableTransaction(deserialize_type, Stream& s) {
        Unserialize(s);
    }

    /** Compute the hash of this CMutableTransaction. This is computed on the
     * fly, as opposed to GetHash() in CTransaction, which uses a cached result.
     */
    uint256 GetHash() const;

    friend bool operator==(const CMutableTransaction& a, const CMutableTransaction& b)
    {
        return a.GetHash() == b.GetHash();
    }

    bool HasWitness() const
    {
        for (size_t i = 0; i < vin.size(); i++) {
            if (!vin[i].scriptWitness.IsNull()) {
                return true;
            }
        }
        return false;
    }
};

typedef std::shared_ptr<const CTransaction> CTransactionRef;
static inline CTransactionRef MakeTransactionRef() { return std::make_shared<const CTransaction>(); }
template <typename Tx> static inline CTransactionRef MakeTransactionRef(Tx&& txIn) { return std::make_shared<const CTransaction>(std::forward<Tx>(txIn)); }

/** Compute the weight of a transaction, as defined by BIP 141 */
int64_t GetTransactionWeight(const CTransaction &tx);

#endif // BITCOIN_PRIMITIVES_TRANSACTION_H
