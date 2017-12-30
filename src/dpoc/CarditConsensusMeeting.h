

#ifndef CARDIT_CONSENSUS_MEETING_H
#define CARDIT_CONSENSUS_MEETING_H

#include "MeetingItem.h"
#include "TimeService.h"
#include <list>
#include <boost/shared_ptr.hpp>
#include <memory>
#include <boost/thread/thread.hpp>
#include "ConsensusAccountPool.h"
#include "ConsensusAccount.h"


class CCarditConsensusMeeting
{
public:
	CCarditConsensusMeeting();
	~CCarditConsensusMeeting();

	//共识会议
	void meeting();

	//得到本地账号的Hash160
	void GetLocalAccountHash160(uint160 &u160Hash);

	//线程
	void startMeeting();
	void stop();
	bool hasCompleteRev();
	bool isPreMeetingLastBlock(CBlockIndex* pBlockHeader);

	int getCurrentConsensusInfo(int64_t nPeriodStartTime, int32_t nTimePeriod ,  uint160& myHash160);
	int getLast1RoundMeetingInfo(int64_t curPeriodStartTime,   int64_t& oPeriodStartTime , int32_t& oPeriodCount);
	int getLast2RoundMeetingInfo(int64_t curPeriodStartTime,   int64_t& nPeriodStartTime  , int32_t& nPeriodCount);
	bool GetMeetingList(int64_t nStartTime, std::list<std::shared_ptr<CConsensusAccount>> &consensusList);
	bool IsCompleteInit();
private:
	//初始化，拉取当前的共识状态
	void init();
	//打包数据
	void doMeeting();
	//初始化新一轮共识
	void initNewMeetingRound();
	//真实打包
	void doPackage();
	//开始新的一轮共识
	bool newMeetingRound();
	//std::list<CConsensusAccount>& analysisConsensusSnapshots(int64_t);

	//立刻停止打包
	void stopPackageNow();

	/*
	* 监控节点的情况，如果发现所有连接都断开了，可能是本地断网掉线了，这时如果在共识列表中，应该立即停止，避免单机运行下去，就和网络分叉了
	* 停止之后也会继续监控状态，如果恢复，则继续（如果我还在共识列表中的话）
	*/
	void startPeerMonitor();

private:
	bool bInit;
	//已经开完会，等待打包
	bool bHasCompleteNewMeeting;
	int64_t nInitPeriodStartTime;
	//static bool bRunning;
	//共识轮数
	int nMeetingRound;
	int nCount;
	//是否允许打包
	bool bCanPackage;
	//是否正在打包中
	bool bPackageing;
	//共识调度器状态，0等待初始化，1初始化中，2初始化成功，共识中，3初始化失败
	int nMeetingStatus;
	//当前轮共识信息
	
	//时间服务器
	//CTimeService timeService;
	//上几轮的共识信息
	std::list<std::shared_ptr<CMeetingItem>> oldMettingsList;

	//当前正在共识的账户(本人)
	//本次会议账号
	CLocalAccount localAccount;
	//本次会议账号
	//uint160 uMyCurrentMettingAccount;

	//共识池
	//CConsensusAccountPool::Instance()
	//static CConsensusAccountPool consensusPool;
	
	/** 会议状态 **/
	/** 等待就绪 **/
	const int MEETING_STATUS_WAIT_READY;
	/** 就绪完成，等待开始 **/
	const int MEETING_STATUS_WAIT_BEGIN;
	/** 共识中 **/
	const int MEETING_STATUS_CONSENSUS;
	/** 共识中，接受下一轮选举 **/
	const int MEETING_STATUS_CONSENSUS_WAIT_NEXT;

	boost::thread thrdMiningService;
	std::shared_ptr<CMeetingItem> pCurrentMetting;

	boost::shared_mutex  rwmutex;
	typedef boost::shared_lock<boost::shared_mutex> readLock;
	typedef boost::unique_lock<boost::shared_mutex> writeLock;
};



#endif // CARDIT_CONSENSUS_MEETING_H
