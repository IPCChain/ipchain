// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"
#include "hash.h"
#include "pubkey.h"
#include "key.h"

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // header
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;

	// POW 所使用的  add  by chksong  2017/8/3
    uint32_t nBits;
    uint32_t nNonce;

	// DPOS 共识所用  add by cksong 2017/8/3
	//该时段共识的人数
	uint32_t  nPeriodCount;
	//本轮共识开始的时间点，秒
	uint64_t  nPeriodStartTime;
	// 时段，一轮共识中的第几个时间段，可以验证的共识的人
	uint32_t  nTimePeriod;

    CBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);

        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);

		// add by song  DPOS 算法。
		READWRITE(nPeriodCount);
		READWRITE(nPeriodStartTime);
		READWRITE(nTimePeriod);
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;

		// add by song 2017/8/3
		nPeriodCount = 0; 
		nPeriodStartTime = 0;
		nTimePeriod = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;

		// add by song 2017/8/3  for DPOS 
		block.nPeriodCount		= nPeriodCount;
		block.nPeriodStartTime  = nPeriodStartTime;
		block.nTimePeriod       = nTimePeriod;

        return block;
    }

    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    CBlockLocator(const std::vector<uint256>& vHaveIn)
    {
        vHave = vHaveIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};


class CVote
{
public:

	bool operator < (const CVote &x) const
	{
		return (this->GetHash() < x.GetHash());
	}

	typedef enum {Propose=0, Precommit=1, Commit=2} VoteType;

	int                         type; 
	uint256                     block_hash;
	uint160                     owner_hash;

	int64_t                     nPeriodStartTime;
	int32_t                     nTimePeriod;

	CPubKey                     vchPubKeyOut;
   std::vector<unsigned char>  vchSig;


	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action)
	{
		READWRITE(type);
		READWRITE(block_hash);
		READWRITE(owner_hash);
		READWRITE(nPeriodStartTime);
		READWRITE(nTimePeriod);
		READWRITE(vchPubKeyOut);
		READWRITE(vchSig);
	}

	uint256 GetHash () const
	{
		return SerializeHash(*this);
	}

	bool Sign (CKey &pPrivKey)
	{
		CHash256 hashoperator;
      uint256 hash;

		hashoperator.Write ((unsigned char*)&type, sizeof(type));
		hashoperator.Write ((unsigned char*)block_hash.begin(), block_hash.size());
		hashoperator.Write ((unsigned char*)owner_hash.begin(), owner_hash.size());
		hashoperator.Write ((unsigned char*)&nPeriodStartTime, sizeof(nPeriodStartTime));
		hashoperator.Write ((unsigned char*)&nTimePeriod, sizeof(nTimePeriod));
		hashoperator.Write ((unsigned char*)vchPubKeyOut.begin(), vchPubKeyOut.size());
      hashoperator.Finalize(hash.begin());

		return pPrivKey.Sign(hash, vchSig);
	}

	bool SignVerify ()
	{
		CHash256 hashoperator;
      uint256 hash;

		hashoperator.Write ((unsigned char*)&type, sizeof(type));
		hashoperator.Write ((unsigned char*)block_hash.begin(), block_hash.size());
		hashoperator.Write ((unsigned char*)owner_hash.begin(), owner_hash.size());
		hashoperator.Write ((unsigned char*)&nPeriodStartTime, sizeof(nPeriodStartTime));
		hashoperator.Write ((unsigned char*)&nTimePeriod, sizeof(nTimePeriod));
		hashoperator.Write ((unsigned char*)vchPubKeyOut.begin(), vchPubKeyOut.size());
      hashoperator.Finalize(hash.begin());

		return vchPubKeyOut.Verify (hash, vchSig);

	}

};


/** Compute the consensus-critical block weight (see BIP 141). */
int64_t GetBlockWeight(const CBlock& tx);

#endif // BITCOIN_PRIMITIVES_BLOCK_H
