// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_VALIDATION_H
#define BITCOIN_CONSENSUS_VALIDATION_H

#include <string>
#include <map>
#include "uint256.h"
#include "serialize.h"
#include "streams.h"
#include "clientversion.h"

/** "reject" message codes */
static const unsigned char REJECT_MALFORMED = 0x01;
static const unsigned char REJECT_INVALID = 0x10;
static const unsigned char REJECT_OBSOLETE = 0x11;
static const unsigned char REJECT_DUPLICATE = 0x12;
static const unsigned char REJECT_NONSTANDARD = 0x40;
static const unsigned char REJECT_DUST = 0x41;
static const unsigned char REJECT_INSUFFICIENTFEE = 0x42;
static const unsigned char REJECT_CHECKPOINT = 0x43;

/** Capture information about block/transaction validation */
class CValidationState {
private:
    enum mode_state {
        MODE_VALID,   //!< everything ok
        MODE_INVALID, //!< network rule violation (DoS value may be set)
        MODE_ERROR,   //!< run-time error
    } mode;
    int nDoS;
    std::string strRejectReason;
    unsigned int chRejectCode;
    bool corruptionPossible;
    std::string strDebugMessage;
public:
    CValidationState() : mode(MODE_VALID), nDoS(0), chRejectCode(0), corruptionPossible(false) {}
    bool DoS(int level, bool ret = false,
             unsigned int chRejectCodeIn=0, const std::string &strRejectReasonIn="",
             bool corruptionIn=false,
             const std::string &strDebugMessageIn="") {
        chRejectCode = chRejectCodeIn;
        strRejectReason = strRejectReasonIn;
        corruptionPossible = corruptionIn;
        strDebugMessage = strDebugMessageIn;
        if (mode == MODE_ERROR)
            return ret;
        nDoS += level;
        mode = MODE_INVALID;
        return ret;
    }
    bool Invalid(bool ret = false,
                 unsigned int _chRejectCode=0, const std::string &_strRejectReason="",
                 const std::string &_strDebugMessage="") {
        return DoS(0, ret, _chRejectCode, _strRejectReason, false, _strDebugMessage);
    }
    bool Error(const std::string& strRejectReasonIn) {
        if (mode == MODE_VALID)
            strRejectReason = strRejectReasonIn;
        mode = MODE_ERROR;
        return false;
    }
    bool IsValid() const {
        return mode == MODE_VALID;
    }
    bool IsInvalid() const {
        return mode == MODE_INVALID;
    }
    bool IsError() const {
        return mode == MODE_ERROR;
    }
    bool IsInvalid(int &nDoSOut) const {
        if (IsInvalid()) {
            nDoSOut = nDoS;
            return true;
        }
        return false;
    }
    bool CorruptionPossible() const {
        return corruptionPossible;
    }
    void SetCorruptionPossible() {
        corruptionPossible = true;
    }
    unsigned int GetRejectCode() const { return chRejectCode; }
    std::string GetRejectReason() const { return strRejectReason; }
    std::string GetDebugMessage() const { return strDebugMessage; }
};

//IPC global MAP is used to determine whether the unique field is unique. 
//The keyword is the value of the unique field, uint256 is txid, int is temporary or permanent marker.
// Transactions that are not registered in a block are temporary, 
//may be cleared, and transactions registered in a block are permanent
class IPCCheckMaps
{
public:
	IPCCheckMaps(){};
	~IPCCheckMaps(){};

	void clear()
	{
		TokenSymbolMap.clear();
		TokenHashMap.clear();
		IPCHashMap.clear();
	};

	//Related unique value of token trade
	std::map<std::string, std::pair<uint256, int>> TokenSymbolMap;
	std::map<uint128, std::pair<uint256, int>> TokenHashMap;
	//Knowledge of transaction related unique value
	std::map<uint128, std::pair<uint256, int>> IPCHashMap;
	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(TokenSymbolMap);
		READWRITE(TokenHashMap);
		READWRITE(IPCHashMap);
	}
};
class CKSizeClass
{
public:
	CKSizeClass(){};
	~CKSizeClass(){};
	void setNull()
	{
		uTSymbolMapSize = 0;
		uTHashMapSize = 0;
		uIHashMapSize = 0;
	}

	uint32_t uTSymbolMapSize;  //Tokensymbol Map.size()
	uint32_t uTHashMapSize;		//TokenHashMap.size()
	uint32_t uIHashMapSize;		//IPCHashMap.size()

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(uTSymbolMapSize);
		READWRITE(uTHashMapSize);
		READWRITE(uIHashMapSize);
	}

	uint32_t getTSymbolMapSize() { return uTSymbolMapSize; };
	uint32_t getTHashMapSize() { return uTHashMapSize; };
	uint32_t getIHashMapSize() { return uIHashMapSize; };
	void setTSymbolMapSize(uint32_t & uNum) { uTSymbolMapSize = uNum; };
	void setTHashMapSize(uint32_t & uNum) {  uTHashMapSize = uNum; };
	void setIHashMapSize(uint32_t & uNum) { uIHashMapSize = uNum; };
};



class CKTSMapDataClass
{
public:
	CKTSMapDataClass(){};
	~CKTSMapDataClass(){};

	void setNull()
	{
		tokensymbol = "";
		txid.SetNull();
		bstate = 0;
	}


	std::string tokensymbol;
	uint256 txid;
	int  bstate;

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(tokensymbol);
		READWRITE(txid);
		READWRITE(bstate);
	}

	std::string gettokensymbol() { return tokensymbol; };
	uint256 gettxid() { return txid; };
	int getbstate() { return bstate; };

};

class CKHashMapDataClass
{
public:
	CKHashMapDataClass(){};
	~CKHashMapDataClass(){};

	void setNull()
	{
		hash.SetNull();
		txid.SetNull();
		bstate = 0;
	}

	uint128 hash;
	uint256 txid;
	int  bstate;

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(hash);
		READWRITE(txid);
		READWRITE(bstate);
	}

	uint128 gethash() { return hash; };
	uint256 gettxid() { return txid; };
	int getbstate() { return bstate; };

};


uint64_t static getSize(CKTSMapDataClass &ssdata)
{
	CDataStream sstream(SER_DISK, CLIENT_VERSION);
	sstream << ssdata;

	return sstream.size();
}
uint64_t static getSize(CKHashMapDataClass &ssdata)
{
	CDataStream sstream(SER_DISK, CLIENT_VERSION);
	sstream << ssdata;

	return sstream.size();
}

#endif // BITCOIN_CONSENSUS_VALIDATION_H
