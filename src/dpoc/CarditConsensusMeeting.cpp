

#include "CarditConsensusMeeting.h"
#include "../net.h"
#include <memory>
#include "../util.h"
#include "../chain.h"
#include "../sync.h"
#include "../validation.h"

#include "../net_processing.h"

#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/date_time.hpp>
#include <time.h>
#include <vector>
#include <list>
#include "../net.h"

//区块生成间隔时间，单位豪秒
//#define BLOCK_GEN__MILLISECOND_TIME (BLOCK_GEN_TIME * 1000)

extern void  generateDPOCForMeetingForPackage(uint160 pubkey160hash, uint32_t nPeriodCount, uint64_t  nPeriodStartTime, uint32_t  nTimePeriod);

CCarditConsensusMeeting::CCarditConsensusMeeting():bInit(true),bHasCompleteNewMeeting(false),nInitPeriodStartTime(0),nMeetingRound(0)
                                                          ,nCount(0), bCanPackage(true)
                                                          , bPackageing(false), nMeetingStatus(0)
                                                          , MEETING_STATUS_WAIT_READY(1)
                                                          , MEETING_STATUS_WAIT_BEGIN(2)
	                                                      , MEETING_STATUS_CONSENSUS(3)
	                                                      , MEETING_STATUS_CONSENSUS_WAIT_NEXT(4)
{
	pCurrentMetting.reset();

	//timeService.start();
}

CCarditConsensusMeeting::~CCarditConsensusMeeting()
{
	//timeService.stop();
}

void CCarditConsensusMeeting::meeting()
{
	nCount++;

	//共识调度器状态，0等待初始化，1初始化中，2初始化成功，共识中，3初始化失败
	//等待初始化或初始化失败，则进行初始化
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

//得到会议，得到list，得到我的开会时间
void CCarditConsensusMeeting::init()
{
	LogPrintf("[CCarditConsensusMeeting::init] begin\n");
	//单位为秒
	int readtime = (nInitPeriodStartTime - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME) / 1000;
	
	//获取开始时的共识列表
	//利用当前的快照倒推，新加入和退出的节点以交易的形式出现，遍历交易，添加或删除记账节点
	
	std::list<std::shared_ptr<CConsensusAccount>> consensusList;
	CConsensusAccountPool::Instance().listSnapshotsToTime(consensusList, readtime);

	LogPrintf("[CCarditConsensusMeeting::init]初始化Init开始初始时间nPeriodStartTime--%d--ConsensusList--%d\n", 
		       nInitPeriodStartTime,consensusList.size());

	pCurrentMetting.reset();
	localAccount.Set160Hash();
	//pCurrentMetting.reset(new CMeetingItem(localAccount, consensusList, nPeriodStartTime));
	pCurrentMetting.reset(new CMeetingItem(localAccount, consensusList, nInitPeriodStartTime));
	if (NULL == pCurrentMetting)
	{
		LogPrintf("[CCarditConsensusMeeting::init]初始化失败，new CMeetingItem失败\n");

		nMeetingStatus = 3;
		return;
	}
	
	pCurrentMetting->sortConsensusList();
	
	//得到我的打包位置
	initNewMeetingRound();

	bInit = false;

	LogPrintf("[CCarditConsensusMeeting::init] old metting size : {%d}\n", oldMettingsList.size());
	LogPrintf("[CCarditConsensusMeeting::init] end by 初始化会议构建完成\n");
}

void CCarditConsensusMeeting::doMeeting()
{
	int64_t nNowTime = timeService.GetCurrentTimeMillis();

	//在共识列表中
	if (pCurrentMetting->inConsensusList())
	{
		//本轮会议在共识列表中当前轮到我打包，打包数据
		if (pCurrentMetting->canPackage(nNowTime))
		{
			LogPrintf("[CCarditConsensusMeeting::doMeeting] canPackage is true. \n");
			doPackage();
		}
	}

	//不在共识列表中，不具有打包功能
	if (pCurrentMetting->canEnd(nNowTime))
	{
		LogPrintf("[CCarditConsensusMeeting::doMeeting] CurrentMetting is canEnd\n");
		newMeetingRound();
	}

	MilliSleep(50);

	return;
}


void CCarditConsensusMeeting::initNewMeetingRound()
{
	LogPrintf("[CCarditConsensusMeeting::initNewMeetingRound] begin\n");
	pCurrentMetting->startConsensus();
	++nMeetingRound;
	//得到当前会议的账号
	//uMyCurrentMettingAccount = pCurrentMetting->getCurrentMettingAccount();

	LogPrintf("[CCarditConsensusMeeting::initNewMeetingRound] end by一轮结束，切换新一轮共识 , 开始时间 {%d} , 结束时间 {%d} , 我的时间 {%d}\n", 
		       pCurrentMetting->getPeriodStartTime(),pCurrentMetting->getPeriodEndTime(),
		       pCurrentMetting->getMyPackageTime());
}

void CCarditConsensusMeeting::doPackage()
{
	LogPrintf("[CCarditConsensusMeeting::doPackage] begin\n");
	//打包函数
	uint160 hash160;
	localAccount.Get160Hash(hash160);
	try
	{
		generateDPOCForMeetingForPackage(hash160, pCurrentMetting->getPeriodCount(),
			pCurrentMetting->getPeriodStartTime(),
			pCurrentMetting->getMyPackageIndex());
	
		//设置本轮已打包标志为真
		//pCurrentMetting->SetHasPackage(true);
	
		//打印输出
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

		//pCurrentMetting->SetHasPackage(true);
		return;
	}
	catch (...)
	{
		LogPrintf("[CCarditConsensusMeeting::doPackage] end by generateDPOCForMeetingForPackage trow error\n");

		//pCurrentMetting->SetHasPackage(true);
		return;
	}
}

bool CCarditConsensusMeeting::newMeetingRound()
{
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
	//判断是不是接受到最新的50秒之前的块
	//int64_t nNowTime = timeService.GetCurrentTimeMillis();
	//int64_t nPackageTime = (pBestBlockIndex->nPeriodStartTime) + ((pBestBlockIndex->nTimePeriod + 1)*BLOCK_GEN_TIME);
	
	/*if(nNowTime - nPackageTime > 5* BLOCK_GEN_TIME)
	{
		//return false;
	}*/

	int64_t blockStartTime = pBestBlockIndex->nPeriodStartTime;
	//moddify by xxy 20171128 以前结束时间错误修复
	int64_t blockStopTime = pBestBlockIndex->nPeriodStartTime + pBestBlockIndex->nPeriodCount*BLOCK_GEN_TIME;
	int64_t newMeetingTime = 0;
	// 重置会议时间
	// 上一轮会议还没开完
	if ((previousMetting->getPeriodEndTime() < blockStopTime) && (previousMetting->getPeriodEndTime() >= blockStartTime)) {
		//本轮会议还没有开完，重新开会
		newMeetingTime = blockStartTime;
	}
	else if (previousMetting->getPeriodEndTime() > blockStopTime) {
		//区块同步的状态有问题，还未完成区块同步;(自己开会,等待回滚或同步，此时停止记账)
		newMeetingTime = previousMetting->getPeriodEndTime();
	}
	else if (previousMetting->getPeriodEndTime() == blockStopTime) {
		// 会议正常
		newMeetingTime = blockStopTime;
	}
	//单位为秒,往前推10个块
	int readtime = (newMeetingTime - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME) / 1000;

	//单位为秒,往前推10个块
	//int readtime = (pCurrentMetting->getPeriodEndTime() - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME) / 1000;
	
	//开始新一轮会议
	pCurrentMetting.reset();
	std::list<std::shared_ptr<CConsensusAccount>> accountlist;
	CConsensusAccountPool::Instance().listSnapshotsToTime(accountlist, readtime);
	localAccount.Set160Hash();
	//pCurrentMetting.reset(new CMeetingItem(localAccount, accountlist, previousMetting->getPeriodEndTime()));
	pCurrentMetting.reset(new CMeetingItem(localAccount, accountlist, newMeetingTime));
	if (NULL == pCurrentMetting)
	{
		LogPrintf("[CCarditConsensusMeeting::newMeetingRound]创建MeetingItem失败\n");
		return false;
	}

	//排序
	pCurrentMetting->sortConsensusList();
	//得到自己的打包开始时间和结束时间
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

void CCarditConsensusMeeting::startPeerMonitor() 
{
	LogPrintf("[CCarditConsensusMeeting::startPeerMonitor] begin\n");
	//初始化是允许打包的，当连接的节点为0时，则不继续打包，否则就本地分叉了
	bCanPackage = true;
	
	int nNodeCout = 0; 
	{
	     LOCK(cs_main);
	     nNodeCout = (int)g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL);
	}

	//监控节点变化
	if (0 == nNodeCout) 
	{
		//判断当前是否正在共识中
		if ((NULL != pCurrentMetting) && (-1 != pCurrentMetting->getTimePeriod()))
		{
			bCanPackage = false;
		}
	}
	else 
	{
		//重新上线，继续共识
		bCanPackage = true;	
	}
	LogPrintf("[CCarditConsensusMeeting::startPeerMonitor] end\n");
}

void CCarditConsensusMeeting::startMeeting()
{
	LogPrintf("[CCarditConsensusMeeting::startMeeting] begin\n");

	try
	{
		{
			std::cout << "[CCarditConsensusMeeting::startMeeting] wait for TimeService begin" << std::endl;
			LogPrintf("[CCarditConsensusMeeting::startMeeting] wait for TimeService begin\n");
			//等待时间服务器启动
			//boost::unique_lock<boost::mutex> lockTime(mutexTime);
			//cvTime.wait(lockTime, []{ return timeService.IsHasNetTimeOffset();});
			//cvTime.wait(lockTime, []{ return true; });
			while (true)
			{
				if (timeService.IsHasNetTimeOffset())
				{
					break;
				}
				MilliSleep(2000);
			}
			//std::cout << "xxxxxxxxxxxx" << timeService.GetCurrentTimeMicros() << std::endl;
			std::cout << "[CCarditConsensusMeeting::startMeeting] wait for TimeService end" << std::endl;
			LogPrintf("[CCarditConsensusMeeting::startMeeting] wait for TimeService end\n");
		}

		while (!CConsensusAccountPool::Instance().InitFinished())
			MilliSleep(1000);

		//std::cout << "[CCarditConsensusMeeting::startMeeting] 11111" << std::endl;
		//LogPrintf("[CCarditConsensusMeeting::startMeeting] 111111\n");
		while (true)
		{
			boost::this_thread::interruption_point();

			//接收完所有块，才能开会
			if (hasCompleteRev())
			{
				meeting();
			}
			else
			{
				MilliSleep(100);
			}

		}
	}
	catch (boost::thread_interrupted & errcod)
	{
		LogPrintf("[CCarditConsensusMeeting::startMeeting] end by boost::thread thrdMiningService Interrupt exception was thrown\n");
	}
	LogPrintf("[CCarditConsensusMeeting::startMeeting] end\n");
}

bool CCarditConsensusMeeting::hasCompleteRev()
{
    try{
		//LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] ********** end\n");
	//LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] begin\n");
	if (IsInitialBlockDownload())
	{
		return false;
	}
	if (bInit)
	{
		//LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] 2+++++++++++++22222 end\n");
		//std::cout << "+++++++++++++22222" << std::endl;
		CBlockIndex* pBestBlockIndex = NULL;
		{
			LOCK(cs_main);
			pBestBlockIndex = chainActive.Tip();
		}
		if (NULL == pBestBlockIndex)
		{
			return false;
		}
		//LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] 2+++++++++++++111111 %d end\n", pBestBlockIndex->nHeight);
		//std::cout << "+++++++++++++1111" << std::endl;
		//当前块的会议结束时间
		int64_t nMeetEndTime = (pBestBlockIndex->nPeriodStartTime) + (pBestBlockIndex->nPeriodCount*BLOCK_GEN_TIME);
		int64_t nPackageTime = (pBestBlockIndex->nPeriodStartTime) + (pBestBlockIndex->nTimePeriod+1)*BLOCK_GEN_TIME;
		int64_t nNowTime = timeService.GetCurrentTimeMillis();
		//std::cout << "nNowTime---" << nNowTime << "----nPackageTime---"
		//	      << nPackageTime << "nNowTime - nPackageTime" << nNowTime - nPackageTime << std::endl;
		
		MilliSleep(50);
		/*if (pBestBlockIndex->nHeight == 221365)
		{
			//std::cout << "+++++++++++++33333" << std::endl;
			//LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] 2+++++++++++++333333 end\n");
			nInitPeriodStartTime = timeService.GetCurrentTimeMillis();
			return true;
		}*/
		
		//modify by Millieen ,以前是if ((nNowTime - nPackageTime) < BLOCK_GEN_TIME)
		if ((nNowTime - nPackageTime) < BLOCK_GEN_TIME && nNowTime >= nPackageTime)//当前时间-最新块的打包时间<10秒
		{
			//std::cout << "--nNowTime---" << nNowTime << "----nMeetEndTime---" << nMeetEndTime 
			//	      << "nNowTime - nMeetEndTime++++++"<< nNowTime-nMeetEndTime << std::endl;
			
			//当前时间<最新块的会议结束时间
			if (nNowTime < nMeetEndTime)
			{
				//将此块当前会议的开始时间，作为初始化会议的开始时间
				//但如果过了自己的打包时间，则不打包，和打包判断有逻辑牵扯
				nInitPeriodStartTime = pBestBlockIndex->nPeriodStartTime;
				LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] 0 %d end\n",nInitPeriodStartTime);
				return true;
			}//当前时间>最新块的会议结束时间，但是当前时间<下一轮的第一个打块时间
			else if ((nNowTime >= nMeetEndTime)&&(nNowTime <= (nMeetEndTime+BLOCK_GEN_TIME)))
			{
				//将上次会议的结束时间，作为初始化会议的开始时间
				nInitPeriodStartTime = nMeetEndTime;
				LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] 1 %d end\n",nInitPeriodStartTime);
				return true;
			}//当前时间>最新块的会议结束时间，第一个10秒过了，但没收到第一个块
			else if(nNowTime > (nMeetEndTime + BLOCK_GEN_TIME))
			{
				//将上次会议的结束时间，作为初始化会议的开始时间
				nInitPeriodStartTime = nMeetEndTime;
				LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] 2 %d end\n",nInitPeriodStartTime);
				return true;
			}

		}
	}

	//以上代码只初始化启动的时候，走一次
	if (!bInit)
	{
        //LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] end\n");
		return true;
	}
    //LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] end\n");
	return false;
    }
    catch(...)
    {
        LogPrintf("[CCarditConsensusMeeting::hasCompleteRev] end by throw\n");
        return true;
    }
}



void CCarditConsensusMeeting::stop()
{
	//timeService.stop();
}


bool CCarditConsensusMeeting::isPreMeetingLastBlock(CBlockIndex* pBlockHeader)
{
	if (NULL == pBlockHeader)
	{
		return false;
	}

	if (pBlockHeader->nTimePeriod == pBlockHeader->nPeriodCount - 1)
	{
		return true;
	}

	return false;
}

int CCarditConsensusMeeting::getCurrentConsensusInfo(int64_t nPeriodStartTime, int32_t nTimePeriod ,  uint160& myHash160)
{
	readLock  rdlock(rwmutex);

	//当前轮  // 添加检测
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

	//以前轮
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

	//当前时间 < oldMettingsList最旧的时间
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

	//前5轮会议中，最旧的时间
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
	CLocalAccount account;
	CMeetingItem meetingItem(account, conList, nStartTime);
	meetingItem.sortConsensusList();
	meetingItem.GetConsensusList(conList);
	LogPrintf("[CCarditConsensusMeeting::GetMeetingList] end\n");
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
