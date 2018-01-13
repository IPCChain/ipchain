
#include "DpocMining.h"

CDpocMining*  CDpocMining::_instance = NULL;
std::once_flag CDpocMining::init_flag;
boost::thread CDpocMining::thrdMiningService;

CDpocMining::CDpocMining()
{

}

CDpocMining::~CDpocMining()
{
	
}

void CDpocMining::CreateInstance() {
	static CDpocMining instance;
	CDpocMining::_instance = &instance;
}

CDpocMining&  CDpocMining::Instance()
{
	std::call_once(CDpocMining::init_flag, CDpocMining::CreateInstance);
	return *CDpocMining::_instance;
}

void CDpocMining::start()
{
	pConsensusMeeting.reset(new CCarditConsensusMeeting());
	thrdMiningService = boost::thread(&TraceThread<std::function<void()>>, "meet", std::function<void()>(std::bind(&CCarditConsensusMeeting::startMeeting, pConsensusMeeting)));
}

void CDpocMining::stop()
{
	thrdMiningService.interrupt();
	thrdMiningService.timed_join(boost::posix_time::seconds(2));
}
void CDpocMining::reStart()
{
	thrdMiningService.interrupt();
	thrdMiningService.timed_join(boost::posix_time::seconds(2));
	MilliSleep(200);

	pConsensusMeeting.reset();
	pConsensusMeeting.reset(new CCarditConsensusMeeting());
	thrdMiningService = boost::thread(&TraceThread<std::function<void()>>, "meet", std::function<void()>(std::bind(&CCarditConsensusMeeting::startMeeting, pConsensusMeeting)));
}
int CDpocMining::getCurrentConsensusInfo(int64_t nPeriodStartTime, int32_t nTimePeriod ,  uint160& myHash160) 
{
	return pConsensusMeeting->getCurrentConsensusInfo(nPeriodStartTime, nTimePeriod,myHash160);
}
int CDpocMining::getLast1RoundMeetingInfo(int64_t curPeriodStartTime,   int64_t& nPeriodStartTime , int32_t& nPeriodCount)
{
	return pConsensusMeeting->getLast1RoundMeetingInfo(curPeriodStartTime,   nPeriodStartTime , nPeriodCount);
}
int CDpocMining::getLast2RoundMeetingInfo(int64_t curPeriodStartTime,   int64_t& nPeriodStartTime , int32_t& nPeriodCount)
{
	return pConsensusMeeting->getLast2RoundMeetingInfo(curPeriodStartTime,   nPeriodStartTime , nPeriodCount);
}

bool CDpocMining::GetMeetingList(int64_t nStartTime, std::list<std::shared_ptr<CConsensusAccount>> &conList)
{
	return pConsensusMeeting->GetMeetingList(nStartTime, conList);
}

bool CDpocMining::IsCompleteInit()
{
	return pConsensusMeeting->IsCompleteInit();
}
