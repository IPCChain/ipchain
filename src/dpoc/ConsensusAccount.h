#ifndef __IPCCHIAN_CONSENSUSACCOUT_H_20170810__
#define __IPCCHIAN_CONSENSUSACCOUT_H_20170810__

#include "../hash.h"
#include "../uint256.h"
#include "../pubkey.h"
#include "amount.h"
#include "DpocInfo.h"

class CConsensusAccount
{
public:
	CConsensusAccount();
	CConsensusAccount(const uint160& pubkeyhash, CAmount& JoinIPC, const uint256& hash, int64_t credit = 0); //add by xxy 20171026
	CConsensusAccount(const uint160& pubkeyhash, CAmount& JoinIPC,  int64_t credit = 0);
	CConsensusAccount(const uint160& pubkeyhash);
	CConsensusAccount(const CConsensusAccount* rhs);
	~CConsensusAccount();

	CConsensusAccount& operator= (const CConsensusAccount& rhs);

	uint160  getPubicKey160hash();
	uint256  getSortValue();
	void setSortValue(uint256 uSortValue);
	CAmount getJoinIPC();
	void setJoinIPC(CAmount IPC) { mJoinIPC = IPC; };
	void setTxhash(const uint256 hash){ txhash = hash; };
	uint256 getTxhash(){ return txhash; };
	int64_t getCredit() {
		return miCredit;
	};
	void setCredit(int64_t in) {
		miCredit = in;
	};

	uint32_t getHeight() {
		return mU32Height;
	};
	void setHeight(uint32_t in) {
		mU32Height = in;
	};
	int64_t getBackupCredit() { return creditBackup; };
	void backupCredit();
	void revertCredit();
	
	void debugOut();

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(mU32Height);
		READWRITE(mSortValue);
		READWRITE(mPubicKey160hash);
		READWRITE(miCredit);
		READWRITE(creditBackup);
		READWRITE(mJoinIPC);
		READWRITE(txhash);
	}

private:
	uint32_t mU32Height;//Block height
	uint256 mSortValue;
	uint160  mPubicKey160hash;//Public Key HASH
	int64_t  miCredit; //Credit value
	//The blacklist operation empties the credit value, 
	//and in order to roll back, you need to save the unprecedented data	
	int64_t  creditBackup;
	CAmount mJoinIPC;//join and exit consensus IPC
	uint256 txhash; 
};

class CLocalAccount
{
public:
	CLocalAccount();
	CLocalAccount(const CLocalAccount& loaclAccount);
	bool IsNull();
	int Get160Hash(uint160 &hash160);
	int Set160Hash();
private:
	uint160 pubicKey160hash;
};

#endif
