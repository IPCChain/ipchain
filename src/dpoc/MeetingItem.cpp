
#include "MeetingItem.h"
#include "TimeService.h"
#include <stdio.h> 


CMeetingItem::CMeetingItem()
{
}
CMeetingItem::CMeetingItem(CLocalAccount &localAccount, std::list<std::shared_ptr<CConsensusAccount>> &conList,
	                       int64_t nStartTime)
	                      :localAccount(localAccount),
	                       nPeriodStartTime(nStartTime),	
	                       nDiffCount(0),
                           nPeriodEndTime(0),
                           nStatus(0),
                           bHasPackage(false),
                           nMyPackageTime(0),
                           nMyPackageTimeEnd(0),
                           nIndex(-1),
                           bInit(false),
	                       bInConsensusList(false)
{
	std::copy(conList.begin(), conList.end(), std::back_inserter(consensusList));
	nPeriodEndTime = nPeriodStartTime + consensusList.size() * BLOCK_GEN_TIME;
}

CMeetingItem::CMeetingItem(const CMeetingItem & meetingItem)
{
	localAccount = meetingItem.localAccount;
	std::copy(meetingItem.consensusList.begin(), meetingItem.consensusList.end(), std::back_inserter(consensusList));
	nPeriodStartTime = meetingItem.nPeriodStartTime;
	nDiffCount = meetingItem.nDiffCount;
	nPeriodEndTime = meetingItem.nPeriodEndTime;
	nStatus = meetingItem.nStatus;
	bHasPackage = meetingItem.bHasPackage;
	myHash160 = meetingItem.myHash160;
	nMyPackageTime = meetingItem.nMyPackageTime;
	nMyPackageTimeEnd = meetingItem.nMyPackageTimeEnd;
	nIndex = meetingItem.nIndex;
	bInit = meetingItem.bInit;
	bInConsensusList = meetingItem.bInConsensusList;
}

CMeetingItem::~CMeetingItem()
{
}

void CMeetingItem::sortConsensusList()
{
	//这里打乱共识的顺序，运用每个节点都统一的startHeight属性，来重新排序consensusList
	//排序
	auto comparebind = std::bind(&CMeetingItem::compare, this, std::placeholders::_1, std::placeholders::_2);
	consensusList.sort(comparebind);
}


int CMeetingItem::compare(std::shared_ptr<CConsensusAccount> &a1, std::shared_ptr<CConsensusAccount> &a2)
{
	uint256 a1Hash = a1->getSortValue();
	uint256 a2Hash = a2->getSortValue();
	if (a1Hash.IsNull())
	{
		a1->setSortValue(getNewHash(nPeriodStartTime, a1));
	}

	if (a2Hash.IsNull())
	{
		a2->setSortValue(getNewHash(nPeriodStartTime, a2));
	}

	return a1->getSortValue() < a2->getSortValue();
}

uint256 CMeetingItem::getNewHash(const int64_t nTime, std::shared_ptr<CConsensusAccount> &account)
{
	std::stringstream ss;
	std::string strTime;
	ss << nTime;
	ss >> strTime;
	uint160 accout160 =  account->getPubicKey160hash();
	std::string strHash160 = accout160.GetHex();
	strHash160 += strTime;
	
	uint256 u256 = Hash(strHash160.begin(), strHash160.end());
	return Hash(u256.begin(), u256.end());
}

void CMeetingItem::startConsensus()
{
	LogPrintf("[CMeetingItem::startConsensus] begin\n");
	bInit = true;
	
	uint160 uMyHash160;
	localAccount.Get160Hash(uMyHash160);
	this->myHash160 = uMyHash160;
	
	int nNum = 0;
	for (std::list<std::shared_ptr<CConsensusAccount>>::iterator it = consensusList.begin(); 
		 it != consensusList.end(); ++it)
	{
		std::cout << "nCMeetingItem::startConsensus in---" << consensusList.size()
			      <<"-------Index-----" << nNum<<"---startTime----"<< nPeriodStartTime
			      <<"------myHash-------" << uMyHash160.GetHex()
			      << "----getPubicKey160hash()----" << (*it)->getPubicKey160hash().GetHex() << std::endl;
		
		LogPrintf("[CMeetingItem::startConsensus]ConsensusListCount--%d--Index--%d--myHash--%d--getPubicKey160hash--%s\n",
			       consensusList.size(), nNum, uMyHash160.GetHex(),(*it)->getPubicKey160hash().GetHex());
		
		if (uMyHash160 == (*it)->getPubicKey160hash())
		{
			//this->myHash160 = uMyHash160;
			nMyPackageTime = nPeriodStartTime + (nNum+1)*BLOCK_GEN_TIME;
			nMyPackageTimeEnd = nMyPackageTime;
			nIndex = nNum;
			bInConsensusList = true;
			break;
		}

		++nNum;
	}
	LogPrintf("[CMeetingItem::startConsensus] end\n");
}

bool CMeetingItem::GetHasPackage()
{
	return bHasPackage;
}

void CMeetingItem::SetHasPackage(bool bSet)
{
	bHasPackage = bSet;
}
bool CMeetingItem::inConsensusList(uint160 uPublicKeyHash)
{
	std::list<std::shared_ptr<CConsensusAccount>>::const_iterator iter = consensusList.begin();
	for (;iter!=consensusList.end();++iter)
	{
		if (uPublicKeyHash == (iter->get())->getPubicKey160hash())
		{
			return true;
		}
	}
	
	return false;
}

bool CMeetingItem::inConsensusList()
{
	return bInConsensusList;
}

bool CMeetingItem::canPackage(int64_t nNow)
{
	//MilliSleep(50);
	//是否本轮已经打包了，一轮会议只打包一次
	if (myHash160.IsNull()|| bHasPackage) 
	{
		return false;
	}

	//没在共识列中，不能打包
	if (!inConsensusList())
	{
		return false;
	}
	
	//当前时间>=自己打包时间，并且<自己的打包时间加1秒，定义为自己的打包时间
	//if ((nNow >= nMyPackageTimeEnd) && (nNow < (nMyPackageTimeEnd + 1000)))
	if ((nMyPackageTimeEnd <= nNow) && (nNow < (nMyPackageTimeEnd + BLOCK_GEN_TIME)))
	{
		//设置打块标志
		SetHasPackage(true);
		return true;
	}

	return false;
}

bool CMeetingItem::canEnd(int64_t nNow)
{
	if (nPeriodEndTime <= nNow)
	{
		LogPrintf("[CMeetingItem::canEnd] last Meeting Time end,Now go to New Meeting\n");
		return true;
	}

	return false;
}

int CMeetingItem::getPeriodCount()
{
	return consensusList.size();
}

int CMeetingItem::getTimePeriod()
{
	uint160 myHash160;
	localAccount.Get160Hash(myHash160);

	int nIndex = 0;
	for (std::list<std::shared_ptr<CConsensusAccount>>::iterator it = consensusList.begin(); 
		 it != consensusList.end(); ++it)
	{
		if (myHash160 == (*it)->getPubicKey160hash())
		{
			return nIndex;
		}
		++nIndex;
	}

	return -1;
}

bool CMeetingItem::isNull()
{
	if (0 >= consensusList.size())
	{
		return true;
	}
	return false;
}

int64_t CMeetingItem::getPeriodStartTime() 
{
	return nPeriodStartTime;
}
void CMeetingItem::setPeriodStartTime(const int64_t periodStartTime)
{
	 nPeriodStartTime = periodStartTime;
}
int64_t CMeetingItem::getPeriodEndTime()
{
	return nPeriodEndTime;
}
void CMeetingItem::setPeriodEndTime(const int64_t periodEndTime)
{
	nPeriodEndTime = periodEndTime;
}
int64_t CMeetingItem::getMyPackageTime()
{
	return nMyPackageTime;
}
void CMeetingItem::setMyPackageTime(const int64_t myPackageTime)
{
	nMyPackageTime = myPackageTime;
}
int64_t CMeetingItem::getMyPackageTimeEnd()
{
	return nMyPackageTimeEnd;
}
void CMeetingItem::setMyPackageTimeEnd(const int64_t myPackageTimeEnd)
{
	nMyPackageTimeEnd = myPackageTimeEnd;
}
int CMeetingItem::getMyPackageIndex()
{
	return nIndex;
}

bool CMeetingItem::isLastBlock(CBlockIndex* pBlockHeader) 
{
	if (NULL == pBlockHeader) 
	{
		return false;
	}

	if (pBlockHeader->nPeriodStartTime == nPeriodStartTime 
		&& pBlockHeader->nPeriodCount == consensusList.size()
		&& pBlockHeader->nTimePeriod == pBlockHeader->nPeriodCount - 1) 
	{
		return true;
	}
	
	return false;
}


uint160 CMeetingItem::getCurrentMettingAccount()
{
	return myHash160;
}

bool CMeetingItem::getOldPublicKey(int nTimePeriod,uint160 &retPublicKey)
{
	int nIndex = 0;
	for (std::list<std::shared_ptr<CConsensusAccount>>::iterator it = consensusList.begin();
		it != consensusList.end(); ++it)
	{
		if (nTimePeriod == nIndex)
		{
			retPublicKey = (*it)->getPubicKey160hash();
			return true;
		}
		++nIndex;
	}

	return false;
}

int CMeetingItem::getConsensusListSize()
{
	return consensusList.size();
}

void CMeetingItem::GetConsensusList(std::list<std::shared_ptr<CConsensusAccount>> &conList)
{
	conList.clear();
	std::copy(consensusList.begin(), consensusList.end(), std::back_inserter(conList));
}