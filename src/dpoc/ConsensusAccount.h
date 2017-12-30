#ifndef __IPCCHIAN_CONSENSUSACCOUT_H_20170810__
#define __IPCCHIAN_CONSENSUSACCOUT_H_20170810__

#include "../hash.h"
#include "../uint256.h"
#include "../pubkey.h"
#include "amount.h"
#include "DpocInfo.h"

// enum CampaignType_t
// {
// 	TYPE_CAMPAIGN_NULL = -1 ,		//   不是竞选交易
// 	TYPE_CAMPAIGN_REGISTER = 0 ,	//   申请加入
// 	TYPE_CAMPAIGN_QUITE = 1,		//   申请退出
// 
// 	TYPE_CAMPAIGN_ORDINARY_PUSNISHMENT = 2, //  Ordinary punishment普通处罚	
// 	TYPE_CAMPAIGN_SEVERE_PUNISHMENT = 3 ,	// severe punishment 严重处罚
//     TYPE_CAMPAIGN_RETURN_DEPOSI = 4 ,		// Return the deposi   返回押金
// };



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
	//add by xxy
	void setTxhash(const uint256 hash){ txhash = hash; };
	uint256 getTxhash(){
		return txhash;
	};
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

	uint32_t mU32Height;//加入块高度
	uint256 mSortValue;//排序用
	uint160  mPubicKey160hash;//交易竞选者 Public Key HASH
	int64_t  miCredit; //信用值
	int64_t  creditBackup; //黑名单操作会清空信用值，为了回滚，需要保存清空前的数据	
	CAmount mJoinIPC;//加入退出共识的IPC
	//add by xxy
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