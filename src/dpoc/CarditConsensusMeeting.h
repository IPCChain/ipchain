

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

	void meeting();
	void GetLocalAccountHash160(uint160 &u160Hash);
	void startMeeting();
	//stop meeting
	void stop();
	//Have you received all the blocks
	bool hasCompleteRev();
	bool isPreMeetingLastBlock(CBlockIndex* pBlockHeader);

	int getCurrentConsensusInfo(int64_t nPeriodStartTime, int32_t nTimePeriod ,  uint160& myHash160);
	int getLast1RoundMeetingInfo(int64_t curPeriodStartTime,   int64_t& oPeriodStartTime , int32_t& oPeriodCount);
	int getLast2RoundMeetingInfo(int64_t curPeriodStartTime,   int64_t& nPeriodStartTime  , int32_t& nPeriodCount);
	bool GetMeetingList(int64_t nStartTime, std::list<std::shared_ptr<CConsensusAccount>> &consensusList);
	bool IsCompleteInit();

private:
	void init();
	void doMeeting();
	void initNewMeetingRound();
	void doPackage();	
	bool newMeetingRound();
	void stopPackageNow();

private:
	//Whether to initialize
	bool bInit;
	//The meeting has been held, waiting to be packed
	bool bHasCompleteNewMeeting;
	//Initial meeting start time
	int64_t nInitPeriodStartTime;
	//Whether to allow packaging
	bool bCanPackage;
	//Is it packing
	bool bPackageing;
	//0=Wait initialization，1=In the initialization，
	//2=Initialization success，3=Initialization failure
	int nMeetingStatus;
	//Previous rounds of consensus information
	std::list<std::shared_ptr<CMeetingItem>> oldMettingsList;
	//Current account in consensus
	CLocalAccount localAccount;
	
	boost::thread thrdMiningService;
	std::shared_ptr<CMeetingItem> pCurrentMetting;

	boost::shared_mutex  rwmutex;
	typedef boost::shared_lock<boost::shared_mutex> readLock;
	typedef boost::unique_lock<boost::shared_mutex> writeLock;
};
#endif // CARDIT_CONSENSUS_MEETING_H
