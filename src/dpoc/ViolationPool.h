
#ifndef  __IPCCHIAN_CVIOLATIONPOLL_H_20170906__
#define  __IPCCHIAN_CVIOLATIONPOLL_H_20170906__

#include <list>
#include <mutex>
#include <deque>
#include <set>

#include "ConsensusAccount.h"
#include "MiniMeetingInfo.h"

#include "primitives/transaction.h"
#include "primitives/block.h"

#include "univalue.h"

class CViolationPool
{
public:

	// 检测到严重违规行为，添加Chan
	bool addSeriouViolationToCach(std::shared_ptr<CMiniMeetingInfo> pMeetingInfo);

	// 是否在严重违规的队列中。
	bool containInSeriouList(uint160 &publickeyhash);
	bool containInTimeoutList(uint160 &publickeyhash);

	bool  AddInSeriouList(uint160 &publickeyhash);
	bool  AddInTimeoutList(uint160 &publickeyhash);

	bool  RemoveTimeoutList(uint160 &publickeyhash);
	bool  RemoveSeriouList(uint160 &publickeyhash);

	//检查是否在超时违规中。 打包时候调用
	bool checkTimeoutMeeting(CBlock* pblock);


	//添加 各种违规 到 CoinBase中
	bool addViolationToCoinBase(CMutableTransaction &coinbaseTx , std::shared_ptr<CMiniMeetingInfo> ptrMiniMeetingInfo);
	static  CViolationPool&  Instance();

	void debugStr(UniValue &debugout);
private:
	

private:

	CViolationPool();
	~CViolationPool();

	static void CreateInstance();
	static CViolationPool* _instance;
	static std::once_flag init_flag;

private:
	std::mutex violationPoolMutex;
	// 存放已经加载 共识账号列表
	std::list<std::unique_ptr<CConsensusAccount>> m_listSeriouViolation;

	// 存放已经加载 共识账号列表
	std::list<std::unique_ptr<CConsensusAccount>> m_listTimeoutViolation;

	// 存放暂时检测到缓严重违规， 加入coinbase 后，存放列表中
	std::deque<std::shared_ptr<CMiniMeetingInfo>> m_cacheSeriouViolation;



	std::set<uint160>  m_TimeoutListResult;
};



#endif