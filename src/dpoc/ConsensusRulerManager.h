
#ifndef  _IPCCHAIN_CONSENSUSRULERMANAGER_H_201708291527_
#define  _IPCCHAIN_CONSENSUSRULERMANAGER_H_201708291527_

#include <deque>
#include <memory.h>
#include "mutex"

#include "uint256.h"
#include "primitives/block.h"
#include "pubkey.h"
#include "bloom.h"
#include "MiniMeetingInfo.h"

struct CRulerItem {
	uint160  pubkeyhash;
	//该时段共识的人数
	int32_t  nPeriodCount;
	//本轮共识开始的时间点，秒
	int64_t  nPeriodStartTime;
	// 时段，一轮共识中的第几个时间段，可以验证的共识的人
	int32_t  nTimePeriod;
	int32_t  nTimers;
};



class CConsensuRulerManager
{
public:

	/************************************************************************/
	/* 下面是旧的方法                                                         */
	/************************************************************************/

	// 分析收到新快
	bool analysisRecvNewBlock(const std::shared_ptr<const CBlock> pblock);

	//添加惩罚 ，返款 ,到CoinBase 
	bool  addRulersToCoinbase(CBlock* pblock ,CMutableTransaction &coinbaseTx, std::shared_ptr<CMiniMeetingInfo> ptrMiniMeetingInfo);

	//单例模式
	static  CConsensuRulerManager&  Instance();

	// 分析收到块的CoinBase取得，Publickey ，以及签名字段
	bool getPublicKeyFromBlock(const CBlock *pblock, CPubKey& outPubkey, std::vector<unsigned char>& outRecvSign);
private:
	
	//添加前面到 CoinBase中
	bool addSignToCoinBase(CMutableTransaction &coinbaseTx, uint160* pPubKeyHash);

	// 检查  在统一会议时段收到重报 ，严重惩罚
	bool  checkRepeadBlock(const std::shared_ptr<const CBlock> pblock, uint160& pubkeyHash);

	// 普通的区块检查 
	bool  checkNewBlock(const std::shared_ptr<const CBlock> pblock, uint160 &pubkeyHash);
private:
	CConsensuRulerManager();
	~CConsensuRulerManager();

	std::unique_ptr<CBloomFilter>  pBloomFilter;

	std::mutex rulerMutex;
	static void CreateInstance();
	static CConsensuRulerManager* _instance;
	static std::once_flag init_flag;

	std::deque<std::unique_ptr<CRulerItem>> mQueueRecvItem;
};




#endif // !
