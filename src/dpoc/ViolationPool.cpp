
#include "ViolationPool.h"
#include "primitives/transaction.h"

#include "../util.h"
#include "../validation.h"

#include "DpocMining.h"


#include <set>
#include <algorithm>

CViolationPool*  CViolationPool::_instance = nullptr;
std::once_flag CViolationPool::init_flag;

/////////////////////////////////////////////////

CViolationPool::CViolationPool()
{
}

CViolationPool::~CViolationPool()
{
}

void CViolationPool::CreateInstance() {
	static CViolationPool instance;
	CViolationPool::_instance = &instance;
}


//单例模式
CViolationPool&  CViolationPool::Instance()
{
	std::call_once(CViolationPool::init_flag, CViolationPool::CreateInstance);
	return *CViolationPool::_instance;
}



// 检测到严重违规行为，添加到违规缓存
bool CViolationPool::addSeriouViolationToCach(std::shared_ptr<CMiniMeetingInfo> pMeetingInfo)
{
	std::lock_guard<std::mutex> lock(violationPoolMutex);  // 

	m_cacheSeriouViolation.push_back(pMeetingInfo);



	return true;
}



// 添加严重违规，违规记录到CoinBase中
bool CViolationPool::addViolationToCoinBase(CMutableTransaction &coinbaseTx, std::shared_ptr<CMiniMeetingInfo> ptrMiniMeetingInfo)
{
	// 遍历普通惩罚，严重处罚列表 添加到CoinBase中.
	// 添加违规的节点，到CoinBase
	// 参数1  campaignType  惩罚类型
	// 参数2  campaignPubkeyHash  惩罚共识账号的Public Key HASH
	// CTxOut out =	CTxOut(CampaignType campaignType, uint160 campaignPubkeyHash);

	// 严重惩罚。
	std::lock_guard<std::mutex> lock(violationPoolMutex); 
	LogPrintf("[addViolationToCoinBase] m_cacheSeriouViolation number == %d\n", m_cacheSeriouViolation.size());

	while ( !m_cacheSeriouViolation.empty()) {
		std::shared_ptr<CMiniMeetingInfo> item = m_cacheSeriouViolation.front();
		m_cacheSeriouViolation.pop_front();
		LogPrintf("[addViolationToCoinBase] SeriouViolation  =(%s)\n", item->mPubicKey160hash.ToString());

		//同一论会议中，前一个人，前一个严重违规
		if (item->nPeriodStartTime == ptrMiniMeetingInfo->nPeriodStartTime )	{
			if (ptrMiniMeetingInfo->nTimePeriod == item->nTimePeriod + 1 ) {
				CTxOut out = CTxOut(TYPE_CONSENSUS_SEVERE_PUNISHMENT, item->mPubicKey160hash);
				coinbaseTx.vout.push_back(out);
				LogPrintf("[addViolationToCoinBase] in the same round ,so write in coinbase \n");

				if (!containInSeriouList(item->mPubicKey160hash)) {
					m_listSeriouViolation.push_back(std::unique_ptr<CConsensusAccount>(new CConsensusAccount(item->mPubicKey160hash)));

					////添加违规的信誉。
					//CConsensusAccountPool::Instance().setSeriouViolation(item->mPubicKey160hash);
				}
			}	
			else {
				LogPrintf("[addViolationToCoinBase] in the same round ,too many interval，don't write in coinbase \n");
			}
		}
		

		
	}

	//超时违规   //超时
	LogPrintf("[addViolationToCoinBase] m_TimeoutListResult number == %d\n", m_TimeoutListResult.size());
	std::set<uint160>::iterator iter = m_TimeoutListResult.begin();
	for (; iter != m_TimeoutListResult.end(); ++iter)
	{
		CTxOut out = CTxOut(TYPE_CONSENSUS_ORDINARY_PUSNISHMENT, *iter);
		coinbaseTx.vout.push_back(out);
		LogPrintf("[addViolationToCoinBase] timeout Violate in the same round ,so write in coinbase \n");

		uint160 thisHash = *iter;
		if (!containInTimeoutList(thisHash)) {
			m_listTimeoutViolation.push_back(std::unique_ptr<CConsensusAccount>(new CConsensusAccount(thisHash)));

			//添加违规的信誉。
			//CConsensusAccountPool::Instance().setTimoutViolation(thisHash);
		}
	}
	

	return true;
}

bool CViolationPool::RemoveSeriouList(uint160 &publickeyhash) {
	std::lock_guard<std::mutex> lock(violationPoolMutex);
	std::list<std::unique_ptr<CConsensusAccount>>::iterator  iter = m_listSeriouViolation.begin();
	for (; iter != m_listSeriouViolation.end(); ++iter) {
		if (publickeyhash == (iter->get())->getPubicKey160hash()) {
			m_listSeriouViolation.erase(iter);
			return true;
		}
	}

	return false;
}

bool CViolationPool::containInSeriouList(uint160 &publickeyhash) {
	std::list<std::unique_ptr<CConsensusAccount>>::iterator  iter = m_listSeriouViolation.begin();
	for (; iter != m_listSeriouViolation.end(); ++iter) {
		if (publickeyhash == (iter->get())->getPubicKey160hash()) {
			return true;
		}
	}

	return false;
}


bool CViolationPool::RemoveTimeoutList(uint160 &publickeyhash) {

	std::lock_guard<std::mutex> lock(violationPoolMutex);

	std::list<std::unique_ptr<CConsensusAccount>>::iterator  iter = m_listTimeoutViolation.begin();
	for (; iter != m_listTimeoutViolation.end(); ++iter) {
		uint160  keyhash160 = iter->get()->getPubicKey160hash();
		if (publickeyhash == keyhash160) {
			m_listTimeoutViolation.erase(iter);
			return true;
		}
	}

	return false;
}

bool CViolationPool::containInTimeoutList(uint160 &publickeyhash) {
	
	std::list<std::unique_ptr<CConsensusAccount>>::iterator  iter = m_listTimeoutViolation.begin();
	for (; iter != m_listTimeoutViolation.end(); ++iter) {
		uint160  keyhash160 = iter->get()->getPubicKey160hash();
		if (publickeyhash == keyhash160) {
			LogPrint("[containInTimeoutList] ture  publickeyhash=%s  keyhash160=%s  \n", publickeyhash.ToString(), keyhash160.ToString());
			return true;
		}
		else {
			LogPrint("[containInTimeoutList] false  publickeyhash=%s  keyhash160=%s  \n", publickeyhash.ToString(), keyhash160.ToString());
		}
	}

	return false;
}


bool CViolationPool::AddInSeriouList(uint160 &publickeyhash) 
{
	std::lock_guard<std::mutex> lock(violationPoolMutex); 
	if (containInSeriouList(publickeyhash)) {
		return true;
	}

	m_listSeriouViolation.push_back(std::unique_ptr<CConsensusAccount>(new CConsensusAccount(publickeyhash)));
	//添加违规的信誉。
	//CConsensusAccountPool::Instance().setSeriouViolation(publickeyhash);
	return true;
}


bool CViolationPool::AddInTimeoutList(uint160 &publickeyhash)
{
	std::lock_guard<std::mutex> lock(violationPoolMutex);
	if (containInSeriouList(publickeyhash))
	{
		return true;
	}

	m_listTimeoutViolation.push_back(std::unique_ptr<CConsensusAccount>(new CConsensusAccount(publickeyhash)));

	//添加违规的信誉。
	//CConsensusAccountPool::Instance().setTimoutViolation(publickeyhash);

	return true;
}


bool CViolationPool::checkTimeoutMeeting(CBlock* pblock)
{
	std::set<uint160>  m_TimeoutListRoundlast1;    //上一轮 检测超时违规
	std::set<uint160>  m_TimeoutListRoundLast2;    //上二轮 检测超时违规

	m_TimeoutListResult.clear();

	LOCK(cs_main);
	if (chainActive.Tip() == NULL)
		return true;
	
	{
		//获取上一轮的开始时间，回溯。。。
		int64_t  nPeriodStartTimelast1Round = 0;         //
		int32_t   nPeriodCountlast1Round = 0;              //

		int result = CDpocMining::Instance().getLast1RoundMeetingInfo(pblock->nPeriodStartTime, nPeriodStartTimelast1Round, nPeriodCountlast1Round);
		LogPrintf("getLast1RoundMeetingInfo reslut=(%d)  nPeriodStartTime=%ld  nPeriodStartTimelast1Round=%ld  nPeriodCountlast1Round=%d\n", result, pblock->nPeriodStartTime , nPeriodStartTimelast1Round, nPeriodCountlast1Round);
		if (result > 0) {  //      存在会议中
		}
		else {
			return false;
		}

		if (0 == nPeriodCountlast1Round) {
			LogPrintf("[checkTimeoutMeeting] 0 == nPeriodCountlast1Round ， reture \n");
			return false;
		}


		std::vector<unsigned char> vectorLast1Round;     //
		vectorLast1Round.resize(nPeriodCountlast1Round);  //
		memset(&vectorLast1Round[0], 0, nPeriodCountlast1Round);

		for (CBlockIndex *backIndex = chainActive.Tip(); backIndex->nPeriodStartTime >= nPeriodStartTimelast1Round; backIndex = backIndex->pprev) {
			if (backIndex->nPeriodStartTime == nPeriodStartTimelast1Round) {
				vectorLast1Round[backIndex->nTimePeriod] = 1;
			}
		}

		for (unsigned int nTimePeriod_1 = 0; nTimePeriod_1 < vectorLast1Round.size(); ++nTimePeriod_1) {
			if (0 == vectorLast1Round[nTimePeriod_1]) {
				// 从共识会议中查询
				uint160 hashTimeout1;
				int result = CDpocMining::Instance().getCurrentConsensusInfo(nPeriodStartTimelast1Round, nTimePeriod_1, hashTimeout1);
				LogPrintf("[CConsensuRulerManager::checkTimeoutMeeting ] first round  reslut=(%d)  nPeriodStartTimelast1Round=%ld   nTimePeriod_1=%d  hashTimeout1=%s\n" , result , nPeriodStartTimelast1Round, nTimePeriod_1, hashTimeout1.ToString());
				if (result > 0) {  //      存在会议中
					m_TimeoutListRoundlast1.insert(hashTimeout1);
				}
				else  {  //  在缓存的中 没有查到
					LogPrintf("[checkTimeoutMeeting] the first round , iner error,  return bad value");
					return false;
				}
			}
		}
	}

	
	{
		//去得第二轮 会议的时间
		int64_t nPeriodStartTimeLast2Round = 0;  //   
		int32_t nPeriodCountlast2Round = 0;

		int result = CDpocMining::Instance().getLast2RoundMeetingInfo(pblock->nPeriodStartTime, nPeriodStartTimeLast2Round, nPeriodCountlast2Round);
		LogPrintf("[getLast2RoundMeetingInfo reslut=%d  nPeriodStartTime=%ld  nPeriodStartTimeLast2Round=%ld  nPeriodCountlast2Round=%d\n", result, pblock->nPeriodStartTime , nPeriodStartTimeLast2Round, nPeriodCountlast2Round);
		if (result > 0) {  //      存在会议中
		}
		else {
			return false;
		}

		if (0 == nPeriodCountlast2Round ) {
			LogPrintf("[checkTimeoutMeeting] 0 == nPeriodCountlast2Round ， return bad value\n");
			return false;
		}

		std::vector<unsigned char> vectorLast2Round;   //
		vectorLast2Round.resize(nPeriodCountlast2Round);  //指令
		memset(&vectorLast2Round[0], 0, nPeriodCountlast2Round);

		for (CBlockIndex *backIndex = chainActive.Tip(); backIndex->nPeriodStartTime >= nPeriodStartTimeLast2Round; backIndex = backIndex->pprev) {
			if (backIndex->nPeriodStartTime == nPeriodStartTimeLast2Round) {
				vectorLast2Round[backIndex->nTimePeriod] = 1;
			}
		}


		for (unsigned int nTimePeriod_2=0 ; nTimePeriod_2 < vectorLast2Round.size() ; ++nTimePeriod_2)
		{
			if (0 == vectorLast2Round[nTimePeriod_2]) {
				uint160 hashTimeout2;  
				int result = CDpocMining::Instance().getCurrentConsensusInfo(nPeriodStartTimeLast2Round, nTimePeriod_2, hashTimeout2);
				LogPrintf("[CConsensuRulerManager::checkTimeoutMeeting ] second round reslut=(%d)  nPeriodStartTimeLast2Round=%ld , nTimePeriod_2=%d   hashTimeout2=%s\n", result, nPeriodStartTimeLast2Round, nTimePeriod_2, hashTimeout2.ToString());
				if (result > 0) {  //      存在会议中
					m_TimeoutListRoundLast2.insert(hashTimeout2);
				}
				else  {  //  在缓存的中 没有查到
					LogPrintf("[checkTimeoutMeeting] the  second round , iner error,  return bad value");
					return false;
				}
			}
		}
	}

	// 获得2次的交集。
	m_TimeoutListResult.clear();
	std::set_intersection(m_TimeoutListRoundlast1.begin(), m_TimeoutListRoundlast1.end(), 
		m_TimeoutListRoundLast2.begin(), m_TimeoutListRoundLast2.end(),
		std::inserter(m_TimeoutListResult, m_TimeoutListResult.begin()) );

	// 已经的得到超时违规违规。


	LogPrintf("[checkTimeoutMeeting]  m_TimeoutListResult.size=%d\n"  , m_TimeoutListResult.size());

	return true;
}


void CViolationPool::debugStr(UniValue &debugout)
{ 

	{  //严重违规
		debugout.push_back(Pair("m_listSeriouViolation nums", (int)m_listSeriouViolation.size()));
		std::list<std::unique_ptr<CConsensusAccount>>::const_iterator iter = m_listSeriouViolation.begin();
		for (; iter != m_listSeriouViolation.end(); ++iter)
		{
			CConsensusAccount *acount = iter->get();
			debugout.push_back(Pair(" m_listSeriouViolation  publickeyhash", acount->getPubicKey160hash().GetHex()));
		}
	}

	{  //超时违规
		debugout.push_back(Pair("m_listTimeoutViolation nums", (int)m_listTimeoutViolation.size()));
		std::list<std::unique_ptr<CConsensusAccount>>::iterator iter = m_listTimeoutViolation.begin();
		for (; iter != m_listTimeoutViolation.end(); ++iter)
		{
		//	CConsensusAccount *acount = iter;
			debugout.push_back(Pair("m_listTimeoutViolation  publickeyhash", (*iter)->getPubicKey160hash().ToString() ));
		}
	}

}