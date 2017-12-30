
#ifndef MEETING_ITEM_H
#define MEETING_ITEM_H
#include <stdint.h>
#include <list>
#include "ConsensusAccount.h"
#include "../chain.h"
#include <memory>


//#include "CarditConsensusMeeting.h"
//class CCarditConsensusMeeting;

//一轮完整的会议纪要，从就绪，到顺序共识，到结束，都有数据记录
class CMeetingItem
{
public:
	CMeetingItem();
	~CMeetingItem();
	CMeetingItem(CLocalAccount &localAccount, std::list<std::shared_ptr<CConsensusAccount>> &consensusList,int64_t nStartTime);
	CMeetingItem(const CMeetingItem & meetingItem);
	
	//排序这里打乱共识的顺序，运用每个节点都统一的startHeight属性，来重新排序consensusList
	//每次会议时间内，排序顺序都是一样的
	//初始化后，马上调用
	void sortConsensusList();
	
	//开始共识，这时根据队列数量决定结束的区块高度	
	void startConsensus();

	//当前是否轮到我打包了
	bool canPackage(int64_t nNow);

	bool GetHasPackage();
	void SetHasPackage(bool bSet);

	/**
	* 当前是否已满足本轮结束条件，二哥条件二选一满意即可
	* 1、高度达到结束高度
	* 2、本轮时间周期结束
	*/
	bool canEnd(int64_t nNow);
	
	//获取当前轮的时段数量，也就是由多少人参与共识
	int getPeriodCount();
	
	//获取我的共识时段
	int getTimePeriod();

	//判断本次会议纪要是否为空
	bool isNull();

	int64_t getPeriodStartTime();
	void setPeriodStartTime(const int64_t periodStartTime);

	int64_t getPeriodEndTime();
	void setPeriodEndTime(const int64_t periodEndTime);

	int64_t getMyPackageTime();
	void setMyPackageTime(const int64_t myPackageTime);

	int64_t getMyPackageTimeEnd();
	void setMyPackageTimeEnd(const int64_t myPackageTimeEnd);

	int getMyPackageIndex();
	
	//该块是否是该轮共识的最后一个块
	bool isLastBlock(CBlockIndex* pBlockHeader);

	//是否在共识列表中
	bool inConsensusList(uint160 uPublicKeyHash);
	bool inConsensusList();
	uint160 getCurrentMettingAccount();
	bool getOldPublicKey(int nTimePeriod, uint160 &retPublicKey);

	int getConsensusListSize();

	void GetConsensusList(std::list<std::shared_ptr<CConsensusAccount>> &conList);
	
public:
	//我的hash160
	uint160 myHash160;
private:
	int compare(std::shared_ptr<CConsensusAccount> &a1, std::shared_ptr<CConsensusAccount> &a2);
	uint256 getNewHash(const int64_t nTime, std::shared_ptr<CConsensusAccount> &account);

	std::list<std::shared_ptr<CConsensusAccount>> consensusList;

	//共识会议
	CLocalAccount localAccount;
	//本轮的所有就绪消息队列
	
	//本轮对应的开始时间点，单位秒
	int64_t nPeriodStartTime;
	//上一轮的偏移
	int64_t nDiffCount;
	//当前轮结束时间，单位秒
	int64_t nPeriodEndTime;
	//本轮的共识状态
	int nStatus;

	//我是否已经打过包了
	bool bHasPackage;
	//我的hash160
//	uint160 myHash160;
	//我的打包时间
	int64_t nMyPackageTime;
	int64_t nMyPackageTimeEnd;
	//第几个打包（从0开始）
	int nIndex;

	//是否初始化完成
	bool bInit;

	//本人是否在共识列表中
	bool bInConsensusList;
	
};

/*class CompareAccount
{
public:

	bool operator()(CConsensusAccount a1, CConsensusAccount  a2)
	{
	

	}
};*/
 


#endif //MEETING_ITEM_H