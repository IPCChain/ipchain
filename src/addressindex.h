#ifndef BITCOIN_ADDRESSINDEX_H
#define BITCOIN_ADDRESSINDEX_H

#include "uint256.h"
#include "amount.h"
#include "script/script.h"
#include "primitives/transaction.h"

struct CAddressUnspentKey {
	unsigned int type;
	uint160 hashBytes;
	uint8_t txType;  // txtype 0 1 --- 0   ,4 5 -- 1   ,2 3 ----2
	uint256 txhash;
	size_t index;
	

	size_t GetSerializeSize() const {
		return 58;
	}
	template<typename Stream>
	void Serialize(Stream& s) const {
		ser_writedata8(s, type);
		hashBytes.Serialize(s);
		ser_writedata8(s, txType);
		txhash.Serialize(s);
		ser_writedata32(s, index);
	}
	template<typename Stream>
	void Unserialize(Stream& s) {
		type = ser_readdata8(s);
		hashBytes.Unserialize(s);
		txType = ser_readdata8(s);
		txhash.Unserialize(s);
		index = ser_readdata32(s);
	}

	CAddressUnspentKey(unsigned int addressType, uint160 addressHash, uint8_t txtype, uint256 txid, size_t indexValue) {
		type = addressType;
		hashBytes = addressHash;
		txType = txtype;
		txhash = txid;
		index = indexValue;
	}

	CAddressUnspentKey() {
		SetNull();
	}

	void SetNull() {
		type = 0;
		hashBytes.SetNull();
		txType = 0;
		txhash.SetNull();
		index = 0;
	}
};

struct CAddressUnspentValue {
	int blockHeight;
	CTxOut unspentout;

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(blockHeight);
		unspentout.SerializationOp(s, ser_action);
// 		READWRITE(unspentout.nValue);
// 		READWRITE(unspentout.txType);
// 		switch (unspentout.txType)
// 		{
// 
// 		case 1:
// 			unspentout.labelLen = unspentout.devoteLabel.size();
// 			READWRITE(unspentout.labelLen);
// 			if (unspentout.labelLen > 0)
// 			{
// 				READWRITE(unspentout.devoteLabel);
// 			}
// 
// 			break;
// 
// 		case 2:
// 		case 3:
// 			unspentout.labelLen = unspentout.ipcLabel.size();
// 			READWRITE(unspentout.labelLen);
// 			if (unspentout.labelLen > 0)
// 			{
// 				READWRITE(unspentout.ipcLabel);
// 			}
// 
// 			break;
// 
// 		case 4:
// 			unspentout.labelLen = unspentout.tokenRegLabel.size();
// 			READWRITE(unspentout.labelLen);
// 			if (unspentout.labelLen > 0)
// 			{
// 				READWRITE(unspentout.tokenRegLabel);
// 			}
// 
// 			break;
// 
// 		case 5:
// 			unspentout.labelLen = unspentout.tokenLabel.size();
// 			READWRITE(unspentout.labelLen);
// 			if (unspentout.labelLen > 0)
// 			{
// 				READWRITE(unspentout.tokenLabel);
// 			}
// 
// 			break;
// 		case 0:
// 			READWRITE(unspentout.coinbaseScript);
// 			break;
// 		default:
// 			return;
// 
// 		}
// 
// 		READWRITE(*(CScriptBase*)(&(unspentout.scriptPubKey)));
// 		READWRITE(unspentout.txLabel);
// 		unspentout.txLabelLen = unspentout.txLabel.size();
	}

	CAddressUnspentValue(int height,const CTxOut& out) {
		blockHeight = height;
		unspentout = out;
	}

	CAddressUnspentValue() {
		unspentout.SetNull();
	}



	bool IsNull() const {
		return unspentout.IsNull();
	}
};
struct CAddressIndexIteratorKey {
	unsigned int type;
	uint160 hashBytes;
	uint8_t txType;

	size_t GetSerializeSize() const {
		return 22;
	}
	template<typename Stream>
	void Serialize(Stream& s) const {
		ser_writedata8(s, type);
		hashBytes.Serialize(s);
		ser_writedata8(s, txType);
	}
	template<typename Stream>
	void Unserialize(Stream& s) {
		type = ser_readdata8(s);
		hashBytes.Unserialize(s);
		txType = ser_readdata8(s);
	}

	CAddressIndexIteratorKey(unsigned int addressType, uint160 addressHash,uint8_t txtype) {
		type = addressType;
		hashBytes = addressHash;
		txType = txtype;
	}

	CAddressIndexIteratorKey() {
		SetNull();
	}

	void SetNull() {
		type = 0;
		hashBytes.SetNull();
		txType = 0;
	}
};


#endif // BITCOIN_ADDRESSINDEX_H