
#ifndef MEETING_ITEM_H
#define MEETING_ITEM_H
#include <stdint.h>
#include <list>
#include "ConsensusAccount.h"
#include "../chain.h"
#include <memory>

class CMeetingItem
{
public:
	CMeetingItem();
	~CMeetingItem();
	CMeetingItem(CLocalAccount &localAccount, std::list<std::shared_ptr<CConsensusAccount>> &consensusList,int64_t nStartTime);
	CMeetingItem(const CMeetingItem & meetingItem);
	
	//Sort, disrupt the order of consensus
	void sortConsensusList();	
	void startConsensus();
	//It's my turn to pack
	bool canPackage(int64_t nNow);
	bool GetHasPackage();
	void SetHasPackage(bool bSet);
	bool canEnd(int64_t nNow);
	
	int getPeriodCount();
	//Get my consensus period
	int getTimePeriod();
	//Determine whether the minutes of this meeting are empty
	bool isNull();
	int64_t getPeriodStartTime();
	void setPeriodStartTime(const int64_t periodStartTime);
	int64_t getPeriodEndTime();
	void setPeriodEndTime(const int64_t periodEndTime);
	void setMyPackageTime(const int64_t myPackageTime);
	int64_t getMyPackageTime();
	int64_t getMyPackageTimeEnd();
	void setMyPackageTimeEnd(const int64_t myPackageTimeEnd);
	int getMyPackageIndex();
	
	//This block is the last block of the consensuss
	bool isLastBlock(CBlockIndex* pBlockHeader);
	//Is it in the consensus list
	bool inConsensusList(uint160 uPublicKeyHash);
	bool inConsensusList();
	uint160 getCurrentMettingAccount();
	bool getOldPublicKey(int nTimePeriod, uint160 &retPublicKey);
	int getConsensusListSize();
	void GetConsensusList(std::list<std::shared_ptr<CConsensusAccount>> &conList);
	
public:
	uint160 myHash160;

	static std::set <uint160> g_Account;

private:
	int compare(std::shared_ptr<CConsensusAccount> &a1, std::shared_ptr<CConsensusAccount> &a2);
	uint256 getNewHash(const int64_t nTime, std::shared_ptr<CConsensusAccount> &account);
	std::list<std::shared_ptr<CConsensusAccount>> consensusList;

	CLocalAccount localAccount;
	//The corresponding start time points
	int64_t nPeriodStartTime;
	//The previous one was offset
	int64_t nDiffCount;
	//The current round end time
	int64_t nPeriodEndTime;
	//This round of consensus
	int nStatus;
	//Have I already packed my bag
	bool bHasPackage;
	//My packing time
	int64_t nMyPackageTime;
	int64_t nMyPackageTimeEnd;
	//The first few packages (starting at 0)
	int nIndex;
	//The initialization is complete
	bool bInit;
	//I am in the consensus list
	bool bInConsensusList;
};
#endif //MEETING_ITEM_H
