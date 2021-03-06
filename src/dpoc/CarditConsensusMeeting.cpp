﻿
#include "CarditConsensusMeeting.h"
#include "../net.h"
#include <memory>
#include "../util.h"
#include "../chain.h"
#include "../sync.h"
#include "../validation.h"
#include "../net_processing.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <time.h>
#include <vector>
#include <list>
#include "../net.h"

extern void  generateDPOCForMeetingForPackage(uint160 pubkey160hash, uint32_t nPeriodCount, uint64_t  nPeriodStartTime, uint32_t  nTimePeriod);

CCarditConsensusMeeting::CCarditConsensusMeeting():bInit(true),bHasCompleteNewMeeting(false),nInitPeriodStartTime(0)
                                                          , bCanPackage(true)
                                                          , bPackageing(false), nMeetingStatus(0)
{
	pCurrentMetting.reset();
}

CCarditConsensusMeeting::~CCarditConsensusMeeting()
{
}

void CCarditConsensusMeeting::meeting()
{
	boost::this_thread::interruption_point();

	if (0 == nMeetingStatus || 3 == nMeetingStatus) 
	{
		if (0 == nMeetingStatus) 
		{
			nMeetingStatus = 1;
			init();
		}
		return;
	}
	else if (2 != nMeetingStatus || NULL == pCurrentMetting)
	{
		if (1 == nMeetingStatus  && NULL != pCurrentMetting)
		{
			nMeetingStatus = 2;
		}	
		return;
	}
	
	doMeeting();
}

void CCarditConsensusMeeting::init()
{
	LogPrintf("[CCarditConsensusMeeting::init] begin\n");
	boost::this_thread::interruption_point();

	int readtime = (nInitPeriodStartTime - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME) / 1000;
	
	//Get the list of consensus at the start
	std::list<std::shared_ptr<CConsensusAccount>> consensusList;
	CConsensusAccountPool::Instance().listSnapshotsToTime(consensusList, readtime);

	LogPrintf("[CCarditConsensusMeeting::init]PeriodStartTime--%d--ConsensusList--%d\n", 
		       nInitPeriodStartTime,consensusList.size());

	pCurrentMetting.reset();
	localAccount.Set160Hash();
	
	pCurrentMetting.reset(new CMeetingItem(localAccount, consensusList, nInitPeriodStartTime));
	if (NULL == pCurrentMetting)
	{
		LogPrintf("[CCarditConsensusMeeting::init]Initialization failure，new CMeetingItem failure\n");

		nMeetingStatus = 3;
		return;
	}
	
	pCurrentMetting->sortConsensusList();
	
	//Get my packing position
	initNewMeetingRound();

	bInit = false;

	LogPrintf("[CCarditConsensusMeeting::init] old metting size : {%d}\n", oldMettingsList.size());
	LogPrintf("[CCarditConsensusMeeting::init] end by Initializing the meeting build completion\n");
}

void CCarditConsensusMeeting::doMeeting()
{
	int64_t nNowTime = timeService.GetCurrentTimeMillis();

	//Is it in the consensus list
	if (pCurrentMetting->inConsensusList())
	{
		//Should I pack it now
		if (pCurrentMetting->canPackage(nNowTime))
		{
			LogPrintf("[CCarditConsensusMeeting::doMeeting] canPackage is true. \n");
			doPackage();
		}
	}

	//Whether the meeting is over
	if (pCurrentMetting->canEnd(nNowTime))
	{
		LogPrintf("[CCarditConsensusMeeting::doMeeting] CurrentMetting is canEnd\n");
		newMeetingRound();
	}

	//MilliSleep(50);

	return;
}


void CCarditConsensusMeeting::initNewMeetingRound()
{
	LogPrintf("[CCarditConsensusMeeting::initNewMeetingRound] begin\n");
	pCurrentMetting->startConsensus();

	LogPrintf("[CCarditConsensusMeeting::initNewMeetingRound] end byRound the end, switch to a new round of consensus , startTime {%d} , endTime {%d} , myTime {%d}\n", 
		       pCurrentMetting->getPeriodStartTime(),pCurrentMetting->getPeriodEndTime(),
		       pCurrentMetting->getMyPackageTime());
}

void CCarditConsensusMeeting::doPackage()
{
	LogPrintf("[CCarditConsensusMeeting::doPackage] begin\n");
	
	uint160 hash160;
	localAccount.Get160Hash(hash160);
	try
	{
		generateDPOCForMeetingForPackage(hash160, pCurrentMetting->getPeriodCount(),
			pCurrentMetting->getPeriodStartTime(),
			pCurrentMetting->getMyPackageIndex());
	
		//print out
		CBlockIndex* pBestBlockIndexss = NULL;
		{
			LOCK(cs_main);
			pBestBlockIndexss = chainActive.Tip();
		}
		if (NULL == pBestBlockIndexss)
		{
			LogPrintf("[CCarditConsensusMeeting::doPackage] end by error NULL == pBestBlockIndexss\n");
			return;
		}

		time_t t = time(NULL);
		struct tm* stime = localtime(&t);
		char cTmp[32];
		memset(cTmp, 0, sizeof(cTmp));
		sprintf(cTmp, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + stime->tm_year, 1 + stime->tm_mon,
				stime->tm_mday, stime->tm_hour,
				stime->tm_min, stime->tm_sec);

		if (g_bStdCout)
		{
			std::cout <<"---------------------------------------------------"<< std::endl;
			std::cout << "CCarditConsensusMeeting::doPackage--MeetingStartTime--" << pCurrentMetting->getPeriodStartTime()
					  << "--MeetingEndTime--" << pCurrentMetting->getPeriodEndTime()
					  << "--packageTime--" << pCurrentMetting->getMyPackageTime()
					  << "--packageTimeEnd--" << pCurrentMetting->getMyPackageTimeEnd()
					  << "--packageIndex--" << pCurrentMetting->getMyPackageIndex()
					  << "--packageCount--" << pCurrentMetting->getPeriodCount()
					 // << "---Time----" << cTmp
					  << "-----PackageTime---" << DateTimeStrFormat("%Y-%m-%d %H:%M:%S", pCurrentMetting->getMyPackageTimeEnd() / 1000)
					  <<"---Hight---"<<pBestBlockIndexss->nHeight
					  <<"---Publickey--"<< hash160.GetHex()
					  << std::endl;
			std::cout << "---------------------------------------------------" << std::endl;
		}

		LogPrintf("[CCarditConsensusMeeting::doPackage] end by-- \
				   MeetingStartTime--%d--MeetingEndTime--%d--packageTime--%d--  \
				   packageTimeEnd--%d--packageIndex--%d--packageCount--%d--  \
				   Time--%s--Hight--%d--Publickey--%s\n",
			pCurrentMetting->getPeriodStartTime(), pCurrentMetting->getPeriodEndTime(),
			pCurrentMetting->getMyPackageTime(), pCurrentMetting->getMyPackageTimeEnd(),
			pCurrentMetting->getMyPackageIndex(), pCurrentMetting->getPeriodCount(),
			cTmp, pBestBlockIndexss->nHeight, hash160.GetHex());
	}
	catch (std::exception &e)
	{
		LogPrintf("[CCarditConsensusMeeting::doPackage] end by generateDPOCForMeetingForPackage trow error %s\n", e.what());
		return;
	}
	catch (...)
	{
		LogPrintf("[CCarditConsensusMeeting::doPackage] end by generateDPOCForMeetingForPackage trow error\n");
		return;
	}
}

bool CCarditConsensusMeeting::newMeetingRound()
{
	boost::this_thread::interruption_point();
	LogPrintf("[CCarditConsensusMeeting::newMeetingRound] begin\n");

	std::shared_ptr<CMeetingItem> previousMetting(pCurrentMetting);
	
	{
		writeLock  wtlock(rwmutex);
		oldMettingsList.push_back(previousMetting);
		if (5 < oldMettingsList.size())
		{
			int count = oldMettingsList.size();
			for (int i = 0; i < count - 5; i++)
			{
				oldMettingsList.pop_front();
			}
		}
	}

	CBlockIndex* pBestBlockIndex = NULL;
	{
		LOCK(cs_main);
		pBestBlockIndex = chainActive.Tip();
	}
	if (NULL == pBestBlockIndex)
	{
		LogPrintf("[CCarditConsensusMeeting::newMeetingRound] end by error NULL == pBestBlockIndex\n");
		return false;
	}

	int64_t blockStartTime = pBestBlockIndex->nPeriodStartTime();
	int64_t blockStopTime = pBestBlockIndex->nPeriodStartTime() + pBestBlockIndex->nPeriodCount()*BLOCK_GEN_TIME;
	int64_t newMeetingTime = 0;

	// Reset meeting time
	// The last meeting was not finished
	if ((previousMetting->getPeriodEndTime() < blockStopTime) && (previousMetting->getPeriodEndTime() >= blockStartTime)) {
		//The meeting has not been completed and resumed
		newMeetingTime = blockStartTime;
	}
	else if (previousMetting->getPeriodEndTime() > blockStopTime) {
		//The state of the block synchronization is problematic, and the block synchronization is not completed.
		//(you're in a meeting, waiting to roll back or synchronize, and stop keeping the bookkeeping)
		newMeetingTime = previousMetting->getPeriodEndTime();
	}
	else if (previousMetting->getPeriodEndTime() == blockStopTime) {
		// The meeting is normal
		newMeetingTime = blockStopTime;
	}

	//So we're going to push 10 blocks
	int readtime = (newMeetingTime - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME) / 1000;
	
	//Start a new round of meetings
	pCurrentMetting.reset();
	std::list<std::shared_ptr<CConsensusAccount>> accountlist;
	CConsensusAccountPool::Instance().listSnapshotsToTime(accountlist, readtime);
	localAccount.Set160Hash();
	pCurrentMetting.reset(new CMeetingItem(localAccount, accountlist, newMeetingTime));
	if (NULL == pCurrentMetting)
	{
		LogPrintf("[CCarditConsensusMeeting::newMeetingRound] Create a MeetingItem failure\n");
		return false;
	}

	//The sorting
	pCurrentMetting->sortConsensusList();
	//Get your own packaging start time and end time
	initNewMeetingRound();
	LogPrintf("[CCarditConsensusMeeting::newMeetingRound] end\n");
	return true;	
}

void CCarditConsensusMeeting::GetLocalAccountHash160(uint160 &u160Hash)
{
	localAccount.Get160Hash(u160Hash);
}

void CCarditConsensusMeeting::stopPackageNow() 
{
	bPackageing = false;
}

void CCarditConsensusMeeting::startMeeting()
{
	LogPrintf("[CCarditConsensusMeeting::startMeeting] begin\n");

	try
	{
		{
			if (g_bStdCout)
			{
				std::cout << "[CCarditConsensusMeeting::startMeeting] wait for TimeService begin" << std::endl;
			}
			LogPrintf("[CCarditConsensusMeeting::startMeeting] wait for TimeService begin\n");
			
			while (true)
			{
				boost::this_thread::interruption_point();
				if (timeService.IsHasNetTimeOffset())
				{
					break;
				}
				MilliSleep(2000);
			}
			
			if (g_bStdCout)
			{
				std::cout << "[CCarditConsensusMeeting::startMeeting] wait for TimeService end" << std::endl;
			}
			LogPrintf("[CCarditConsensusMeeting::startMeeting] wait for TimeService end\n");
		}

		while (!CConsensusAccountPool::Instance().InitFinished())
		{
			boost::this_thread::interruption_point();
			MilliSleep(1000);
		}
			

		while (true)
		{
			boost::this_thread::interruption_point();
			
			if (hasCompleteRev())
			{
				meeting();
			}

			MilliSleep(50);
		}
	}
	catch (boost::thread_interrupted & errcod)
	{
		LogPrintf("[CCarditConsensusMeeting::startMeeting] end by boost::thread thrdMiningService Interrupt exception was thrown\n");
	}
	LogPrintf("[CCarditConsensusMeeting::startMeeting] end\n");
}

extern int64_t g_Reboot_Meeting_StartTime;


bool CCarditConsensusMeeting::hasCompleteRev()
{
	if (IsInitialBlockDownload())
	{
		return false;
	}
	if (bInit)
	{
		CBlockIndex* pBestBlockIndex = NULL;
		try
		{	
			{
				LOCK(cs_main);
				pBestBlockIndex = chainActive.Tip();
			}
			if (NULL == pBestBlockIndex)
			{
				return false;
			}
		}
		catch (...)
		{
			LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] end\n");

			return false;
		}

		int64_t nMeetEndTime = (pBestBlockIndex->nPeriodStartTime()) + (pBestBlockIndex->nPeriodCount()*BLOCK_GEN_TIME);
		int64_t nPackageTime = (pBestBlockIndex->nPeriodStartTime()) + (pBestBlockIndex->nTimePeriod()+1)*BLOCK_GEN_TIME;
		int64_t nNowTime = timeService.GetCurrentTimeMillis();
		
		//MilliSleep(50);

		//reboot chain
		int64_t nRebootHeight = GetArg("-blockrebootheight", 0);
		if (0 != nRebootHeight)
		{
			std::cout << "reboot chain nHeight=" << nRebootHeight << std::endl;
			std::string strPublicKey;
			if (CDpocInfo::Instance().GetLocalAccount(strPublicKey))
			{
				if (CConsensusAccountPool::Instance().verifyPkIsTrustNode(strPublicKey))
				{
					if (nRebootHeight == pBestBlockIndex->nHeight+1)
					{
						int32_t p_timeout = 10*60;
						while (g_Reboot_Meeting_StartTime == 0)
						{
							MilliSleep (100);
							p_timeout --;

							if (p_timeout == 0)
							{
								break;
							}
						}

						if (p_timeout)
						{
							nInitPeriodStartTime = g_Reboot_Meeting_StartTime;
						}
						else
						{
							//nInitPeriodStartTime = timeService.GetCurrentTimeMillis();
							nNowTime = (nNowTime-pBestBlockIndex->nPeriodStartTime())/(pBestBlockIndex->nPeriodCount()*BLOCK_GEN_TIME);
							nNowTime *= (pBestBlockIndex->nPeriodCount()*BLOCK_GEN_TIME);
							nNowTime += pBestBlockIndex->nPeriodStartTime();
							nInitPeriodStartTime = nNowTime;
						}
						return true;
					}
				}
			}
		}
		
		//nowTime-The latest block package time < 10s
		if (((nNowTime - nPackageTime) < BLOCK_GEN_TIME) && (nNowTime >= nPackageTime))
		{	
			//nowTime<The latest block Meet End Time
			if (nNowTime < nMeetEndTime)
			{
				//The start time of the current meeting of this block as the start time of the initialization meeting
				//But if you take your own packaging time, you're not packing, and packing judgment is logically involved
				nInitPeriodStartTime = pBestBlockIndex->nPeriodStartTime();
				LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] 0 %d and nowtime= %d end\n",nInitPeriodStartTime,  nNowTime);
				return true;
			}//nowTime>The latest block Meet End Time，but nowTime< The first package time in the next round
			else if ((nNowTime >= nMeetEndTime)&&(nNowTime <= (nMeetEndTime+BLOCK_GEN_TIME)))
			{
				//At the end of the last meeting, as the beginning of the initial meeting
				nInitPeriodStartTime = nMeetEndTime;
				LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] 1 %d and nowtime= %d end\n",nInitPeriodStartTime, nNowTime);
				return true;
			}//nowTime>The latest end of the meeting，The first 10 seconds passed, but not the first one
			else if(nNowTime > (nMeetEndTime + BLOCK_GEN_TIME))
			{
				//At the end of the last meeting, as the beginning of the initial meeting
				nInitPeriodStartTime = nMeetEndTime;
				LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] 2 %d and nowtime= %d end\n",nInitPeriodStartTime, nNowTime);
				return true;
			}

		}
	}

	//The above code is executed only once when it is initialized
	if (!bInit)
	{
		//MilliSleep(50);
		return true;
	}
	
	return false;
}

void CCarditConsensusMeeting::stop()
{
}

bool CCarditConsensusMeeting::isPreMeetingLastBlock(CBlockIndex* pBlockHeader)
{
	if (NULL == pBlockHeader)
	{
		return false;
	}

	if (pBlockHeader->nTimePeriod() == pBlockHeader->nPeriodCount() - 1)
	{
		return true;
	}

	return false;
}

int CCarditConsensusMeeting::getCurrentConsensusInfo(int64_t nPeriodStartTime, int32_t nTimePeriod ,  uint160& myHash160)
{
	readLock  rdlock(rwmutex);

	if ( NULL == pCurrentMetting || pCurrentMetting->isNull() ) {
		return -1;
	}

	if (nPeriodStartTime == pCurrentMetting->getPeriodStartTime())
	{
		if (pCurrentMetting->getOldPublicKey(nTimePeriod, myHash160))
		{
			return 1;
		}
		return 0;
	}
	else {
		LogPrintf("[getCurrentConsensusInfo]  pCurrentMetting getPeriodStartTime== %ld\n", pCurrentMetting->getPeriodStartTime());
	}

	//Previous round
	bool bHavePeriodStartTime = false;
	
	std::list<std::shared_ptr<CMeetingItem>>::iterator iter = oldMettingsList.begin();
	for (;iter!=oldMettingsList.end();++iter)
	{
		LogPrintf("[getCurrentConsensusInfo] getPeriodStartTime== %ld\n", (*iter)->getPeriodStartTime());
		if ((*iter)->getPeriodStartTime() == nPeriodStartTime)
		{
			bHavePeriodStartTime = true;
		}

		if ((*iter)->getPeriodStartTime() == nPeriodStartTime)
		{
			if ((*iter)->getOldPublicKey(nTimePeriod, myHash160))
			{
				return 1;
			}
		}	
	}

	if (bHavePeriodStartTime)
	{
		return 0;
	}
	return -1;
}

int CCarditConsensusMeeting::getLast1RoundMeetingInfo(int64_t curPeriodStartTime,   int64_t& nPeriodStartTime  , int32_t& nPeriodCount)
{
	readLock  rdlock(rwmutex);
	if (oldMettingsList.empty())
	{
		return -1;
	}

	std::list<std::shared_ptr<CMeetingItem>>::iterator iterbegin = oldMettingsList.begin();
	if ((*iterbegin)->getPeriodStartTime() > curPeriodStartTime)
	{
		return -1;
	}

	std::list<std::shared_ptr<CMeetingItem>>::reverse_iterator iter = oldMettingsList.rbegin();
	nPeriodCount= (*iter)->getPeriodCount();
	nPeriodStartTime = (*iter)->getPeriodStartTime();
	return 1;
}

int CCarditConsensusMeeting::getLast2RoundMeetingInfo(int64_t curPeriodStartTime,   int64_t& nPeriodStartTime  , int32_t& nPeriodCount)
{
	readLock  rdlock(rwmutex);
	if (oldMettingsList.size() < 2)
	{
		return -1;
	}

	std::list<std::shared_ptr<CMeetingItem>>::iterator iterbegin = oldMettingsList.begin();
	if ((*iterbegin)->getPeriodStartTime() > curPeriodStartTime)
	{
		return -1;
	}

	std::list<std::shared_ptr<CMeetingItem>>::reverse_iterator iter = oldMettingsList.rbegin();
	++iter;
	nPeriodCount= (*iter)->getPeriodCount();
	nPeriodStartTime = (*iter)->getPeriodStartTime();
	return 1;
}


bool CCarditConsensusMeeting::GetMeetingList(int64_t nStartTime, std::list<std::shared_ptr<CConsensusAccount>> &conList)
{
	LogPrintf("[CCarditConsensusMeeting::GetMeetingList] begin\n");
    static  int64_t tempStartTime = 0;
    static std::list<std::shared_ptr<CConsensusAccount>> tempconList;
    static std::list<std::shared_ptr<CConsensusAccount>> sortconList;
  if(tempStartTime == nStartTime && tempconList.size() == conList.size()&&tempconList.size()!=0){
        std::list<std::shared_ptr<CConsensusAccount>>::iterator itor1 = tempconList.begin();
        std::list<std::shared_ptr<CConsensusAccount>>::iterator itor2 = conList.begin();
        bool getdifference = false;
           while(itor1!=tempconList.end()&&itor2!=conList.end()){
               uint160 pk1 = (*itor1)->getPubicKey160hash();
               uint160 pk2 = (*itor2)->getPubicKey160hash();
               if(pk1 != pk2){
                   getdifference = true;
                       break;
                }
               itor1++;
               itor2++;
           }
           if(!getdifference){
                conList.clear();
                std::copy(sortconList.begin(), sortconList.end(), std::back_inserter(conList));
                LogPrintf("[CCarditConsensusMeeting::GetMeetingList] end temp used conList_size:%d\n",conList.size());
                return true;
           }
    }
    tempStartTime = nStartTime;
    tempconList.clear();
    tempconList = conList;


	CLocalAccount account;
    CMeetingItem meetingItem(account, conList, nStartTime);
	meetingItem.sortConsensusList();
	meetingItem.GetConsensusList(conList);
    sortconList = conList;

    LogPrintf("[CCarditConsensusMeeting::GetMeetingList] endl\n");
	return true;
}

bool CCarditConsensusMeeting::IsCompleteInit()
{
	if (!bInit)
	{
		return true;
	}
	else
	{
		return false;
	}
}
