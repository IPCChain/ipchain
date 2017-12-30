
#ifndef DPOC_MINING_H
#define DPOC_MINING_H
#include <boost/thread/thread.hpp>
#include "CarditConsensusMeeting.h"
#include<memory>


class CDpocMining
{
	public:
		~CDpocMining();
		
		void start();
		void stop();
		void reStart();

		//µ¥ÀýÄ£Ê½
		static  CDpocMining&  Instance();
		int getCurrentConsensusInfo(int64_t nPeriodStartTime, int32_t nTimePeriod ,  uint160& myHash160);
		int getLast1RoundMeetingInfo(int64_t curPeriodStartTime,   int64_t& oPeriodStartTime , int32_t& oPeriodCount);
		int getLast2RoundMeetingInfo(int64_t curPeriodStartTime,   int64_t& nPeriodStartTime , int32_t& nPeriodCount);
		bool GetMeetingList(int64_t nStartTime, std::list<std::shared_ptr<CConsensusAccount>> &conList);
		bool IsCompleteInit();
private:
	    CDpocMining();
		std::shared_ptr<CCarditConsensusMeeting> pConsensusMeeting;
	   
	    static boost::thread thrdMiningService;

	    static void CreateInstance();
	    static CDpocMining* _instance;
	    static std::once_flag init_flag;
};



#endif//DPOC_MINING_H
