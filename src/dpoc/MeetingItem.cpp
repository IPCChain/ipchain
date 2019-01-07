
#include "MeetingItem.h"
#include "TimeService.h"
#include <stdio.h> 
#include "ConsensusAccountPool.h"
#include "../validation.h"

extern int g_ConsensusSwitchingHeight;

std::set <uint160> CMeetingItem::g_Account;

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
	auto comparebind = std::bind(&CMeetingItem::compare, this, std::placeholders::_1, std::placeholders::_2);
	consensusList.sort(comparebind);

	if (consensusList.size() <= 1)
	{
		return;
	}
	
	//Get a trusted list
	std::vector<std::string> vecTrustList;
	CConsensusAccountPool::Instance().getTrustList(vecTrustList);

	std::list<std::shared_ptr<CConsensusAccount>> listSubTrust;
	std::list<std::shared_ptr<CConsensusAccount>>::iterator iter = consensusList.begin();
	
	while(iter != consensusList.end())
	{
		uint160  u160hashpubicKey = (*iter)->getPubicKey160hash();
		std::string stru160 = u160hashpubicKey.GetHex(); 
		bool bErase = false;
		for (std::vector<std::string>::iterator iterVector = vecTrustList.begin();
			iterVector != vecTrustList.end(); ++iterVector)
		{
			if (stru160 == *iterVector)
			{
				listSubTrust.push_back(*iter);
				iter = consensusList.erase(iter);
				bErase = true;
				break;
			}
		}
	
		if (!bErase)
		{
			++iter;
		}
	}
	int32_t n32Size = consensusList.size(); //Number of non-trusted nodes
	int32_t n32SubSize = listSubTrust.size(); //Number of trusted nodes
	
	//The trusted node is empty
	if (0 == n32SubSize)
	{
		std::list<std::shared_ptr<CConsensusAccount>>::iterator iterTestConsen = consensusList.begin();
		for (; iterTestConsen != consensusList.end(); ++iterTestConsen)
		{
			if (g_bStdCout)
			{
				std::cout << "A consensusList++" << (*iterTestConsen)->getPubicKey160hash().GetHex() << std::endl;
			}

			if (fPrintToDebugLog)
			{
				LogPrintf("[CMeetingItem::sortConsensusList]A consensusList=%d\n", (*iterTestConsen)->getPubicKey160hash().GetHex());
			}
		}
		
		return;
	}

	//All nodes are trusted nodes
	if ((0 == n32Size)&&(0 < n32SubSize))
	{
		consensusList.clear();
		std::copy(listSubTrust.begin(), listSubTrust.end(), std::back_inserter(consensusList));

		std::list<std::shared_ptr<CConsensusAccount>>::iterator iterTestConsen = consensusList.begin();
		for (; iterTestConsen != consensusList.end(); ++iterTestConsen)
		{
			if (g_bStdCout)
			{
				std::cout << "B consensusList++" << (*iterTestConsen)->getPubicKey160hash().GetHex() << std::endl;
			}

			if (fPrintToDebugLog)
			{
				LogPrintf("[CMeetingItem::sortConsensusList]B consensusList=%d\n", (*iterTestConsen)->getPubicKey160hash().GetHex());
			}
		}
		
		return;
	}

	int32_t n32Num = 0; //Insert the interval
	if (n32Size > n32SubSize)//Consensus list size > Trusted node size
	{
		n32Num = n32Size / n32SubSize;
	}
	else
	{
		//Consensus listsize <= Trusted node size，
		//A trusted node is inserted through an untrusted insert, 
		//and the remaining trusted nodes are placed last
		n32Num = 1;
	}
	int32_t nInsert = 0;
	
	while (!listSubTrust.empty())	
	{
		std::list<std::shared_ptr<CConsensusAccount>>::iterator iterInsert = consensusList.begin();
		
		std::advance(iterInsert, nInsert);
		if (nInsert >= consensusList.size())
		{
			consensusList.push_back(listSubTrust.front());
		}
		else
		{
			consensusList.insert(iterInsert, listSubTrust.front());
		}
		
		listSubTrust.pop_front();
		
		++nInsert;
		nInsert = nInsert + n32Num;
	}	

	std::list<std::shared_ptr<CConsensusAccount>>::iterator iterTestConsen = consensusList.begin();
	for (; iterTestConsen != consensusList.end(); ++iterTestConsen)
	{
		if (g_bStdCout)
		{
			std::cout << "A+B consensusList++" << (*iterTestConsen)->getPubicKey160hash().GetHex() << std::endl;
		}
		if (fPrintToDebugLog)
		{
			LogPrintf("[CMeetingItem::sortConsensusList] A+B consensusList=%d\n", (*iterTestConsen)->getPubicKey160hash().GetHex());
		}
	}

	return;
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


	g_Account.clear ();
	for (auto it: consensusList)
	{
		g_Account.insert (it->getPubicKey160hash());
	}
	
	int nNum = 0;
	for (std::list<std::shared_ptr<CConsensusAccount>>::iterator it = consensusList.begin(); 
		 it != consensusList.end(); ++it)
	{
		//if (g_bStdCout)
		{
			std::cout << "CMeetingItem::startConsensus in---" << consensusList.size()
				<< "---Index---" << nNum << "---startTime---" << nPeriodStartTime
				<< "---myHash---" << uMyHash160.GetHex()
				<< "----PubicKey160hash----" << (*it)->getPubicKey160hash().GetHex() << std::endl;
		}
		LogPrintf("[CMeetingItem::startConsensus]ConsensusListCount--%d--Index--%d--myHash--%d--getPubicKey160hash--%s\n",
			       consensusList.size(), nNum, uMyHash160.GetHex(),(*it)->getPubicKey160hash().GetHex());
		
		if (uMyHash160 == (*it)->getPubicKey160hash())
		{
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
	//Whether the current round is packed, a meeting is packed only once
	if (myHash160.IsNull()|| bHasPackage) 
	{
		return false;
	}

	//You can't pack it in the consensus column
	if (!inConsensusList())
	{
		return false;
	}
	
	if ((nMyPackageTimeEnd <= nNow) && (nNow < (nMyPackageTimeEnd + BLOCK_GEN_TIME)))
	{
		//Set the block mark
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

	if (pBlockHeader->nPeriodStartTime() == nPeriodStartTime 
		&& pBlockHeader->nPeriodCount() == consensusList.size()
		&& pBlockHeader->nTimePeriod() == pBlockHeader->nPeriodCount() - 1) 
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
