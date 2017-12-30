
#include <iostream>
#include <utility>

#include "../util.h"
#include "../chain.h"
#include "../sync.h"
#include "../validation.h"
#include "base58.h"
#include "uint256.h"
#include "chainparams.h"
#include "protocol.h"
#include "wallet/wallet.h"
#include "net.h"
#include "consensus/validation.h"
#include "univalue.h"

#include "ConsensusAccountPool.h"
#include "ConsensusRulerManager.h"
#include "DpocInfo.h"
#include "DpocMining.h"
#include "TimeService.h"
#include "SerializeDpoc.h"
#include <boost/filesystem.hpp>
//#include "../init.h"


#include "wallet/wallet.h"
extern CWallet* pwalletMain;

boost::shared_mutex checkmutex;

SnapshotClass::SnapshotClass() {
	//changed = true;
	//referSnapshot = this;

	pkHashIndex = 0;
	blockHeight = 0;
	timestamp = 0;
	meetingstarttime = 0;
	meetingstoptime = 0;
	blockTime = 0;

	curCandidateIndexList.clear();
	curSeriousPunishIndexAdded.clear();
	curTimeoutIndexRecord.clear();
	curRefundIndexList.clear();
	cachedIndexsToRefund.clear();
	curTimeoutPunishList.clear();
	cachedTimeoutPunishToRun.clear();
	cachedMeetingAccounts.clear();
	curBanList.clear();
	cachedBanList.clear();
}
SnapshotClass::SnapshotClass(const SnapshotClass& in){
	//changed = in.changed;
	//referSnapshot = in.referSnapshot;
	timestamp = in.timestamp;
	blockHeight = in.blockHeight;
	meetingstarttime = in.meetingstarttime;
	meetingstoptime = in.meetingstoptime;
	blockTime = in.blockTime;
	pkHashIndex = in.pkHashIndex;

	//std::copy(in.curCandidateIndexList.begin(), in.curCandidateIndexList.end(), curCandidateIndexList.begin());
	curCandidateIndexList = in.curCandidateIndexList;

	curSeriousPunishIndexAdded = in.curSeriousPunishIndexAdded;
	curTimeoutIndexRecord = in.curTimeoutIndexRecord;
	curTimeoutPunishList = in.curTimeoutPunishList;
	cachedTimeoutPunishToRun = in.cachedTimeoutPunishToRun;
	curRefundIndexList = in.curRefundIndexList;
	cachedIndexsToRefund = in.cachedIndexsToRefund;
	cachedMeetingAccounts = in.cachedMeetingAccounts;
	curBanList = in.curBanList;
	cachedBanList = in.cachedBanList;
}


void CConsensusAccountPool::printsets(std::set<uint16_t> list)
{
	std::cout << "--------------------list--------------------" << std::endl;
	LogPrintf("--------------------list--------------------\n");
	std::set<uint16_t>::iterator it;
	for (it = list.begin(); it != list.end(); it++) {
		if (*it == 0) continue;
		std::cout << *it << " ";
		LogPrintf("%d	", *it);
	}
	std::cout << std::endl;
	LogPrintf("\n");
}

void CConsensusAccountPool::printsnapshots(std::vector<SnapshotClass> list)
{
	std::cout << "--------------------check snapshots--------------------" << std::endl;
	std::set<uint16_t>::iterator it;
	std::map<uint16_t, int>::iterator mapit;
	std::vector<std::pair<uint16_t, int64_t>>::iterator meetingit;
	
	std::cout << "Current Candidate Index List:" << std::endl;
	LogPrintf("Current Candidate Index List:\n");
	for (int i=0; i<list.size(); i++)
	{
		std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).curCandidateIndexList.begin(); it != list.at(i).curCandidateIndexList.end(); it++) {
			std::cout << *it << " ";
			LogPrintf("%d	", *it);
		}
		std::cout << std::endl;
		LogPrintf("\n");
	}
	std::cout << std::endl;
	LogPrintf("\n");


	std::cout << "This Loop‘s Miners Index List:" << std::endl;
	LogPrintf("This Loop‘s Miners Index List:\n");
	for (int i = 0; i < list.size(); i++)
	{
		std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (meetingit = list.at(i).cachedMeetingAccounts.begin(); meetingit != list.at(i).cachedMeetingAccounts.end(); meetingit++) {
			std::cout << (*meetingit).first << "-" << (*meetingit).second << " ";
			LogPrintf("%d-%d	", (*meetingit).first, (*meetingit).second);
		}
		std::cout << std::endl;
		LogPrintf("\n");
	}
	std::cout << std::endl;
	LogPrintf("\n");

	std::cout << "New Refund Added:" << std::endl;
	LogPrintf("New Refund Added:\n");
	for (int i = 0; i < list.size(); i++)
	{
		std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).curRefundIndexList.begin(); it != list.at(i).curRefundIndexList.end(); it++) {
			std::cout << *it << " ";
			LogPrintf("%d	", *it);
		}
		std::cout << std::endl;
		LogPrintf("\n");
	}
	std::cout << std::endl;
	LogPrintf("\n");
	
	std::cout << "Cached Candidate Index To Refund List:" << std::endl;
	LogPrintf("Cached Candidate Index To Refund List:\n");
	for (int i = 0; i < list.size(); i++)
	{
		std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).cachedIndexsToRefund.begin(); it != list.at(i).cachedIndexsToRefund.end(); it++) {
			std::cout << *it << " ";
			LogPrintf("%d	", *it);
		}
		std::cout << std::endl;
		LogPrintf("\n");
	}
	std::cout << std::endl;
	LogPrintf("\n");

	std::cout << "Current Timeout Index List:" << std::endl;
	LogPrintf("Current Timeout Index List:\n");
	for (int i = 0; i < list.size(); i++)
	{
		std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (mapit = list.at(i).curTimeoutIndexRecord.begin(); mapit != list.at(i).curTimeoutIndexRecord.end(); mapit++) {
			std::cout << mapit->first << "-" << mapit->second << " ";
			LogPrintf("%d-%d	", mapit->first, mapit->second);
		}
		std::cout << std::endl;
		LogPrintf("\n");
	}
	std::cout << std::endl;
	LogPrintf("\n");

	std::cout << "New Timeout Punishment Added:" << std::endl;
	LogPrintf("New Timeout Punishment Added:\n");
	for (int i = 0; i < list.size(); i++)
	{
		std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).curTimeoutPunishList.begin(); it != list.at(i).curTimeoutPunishList.end(); it++) {
			std::cout << *it << " ";
			LogPrintf("%d	", *it);
		}
		std::cout << std::endl;
		LogPrintf("\n");
	}
	std::cout << std::endl;
	LogPrintf("\n");

	std::cout << "Cached Candidate Index To Timeout Punishment List:" << std::endl;
	LogPrintf("Cached Candidate Index To Timeout Punishment List:\n");
	for (int i = 0; i < list.size(); i++)
	{
		std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).cachedTimeoutPunishToRun.begin(); it != list.at(i).cachedTimeoutPunishToRun.end(); it++) {
			std::cout << *it << " ";
			LogPrintf("%d	", *it);
		}
		std::cout << std::endl;
		LogPrintf("\n");
	}
	std::cout << std::endl;
	LogPrintf("\n");

	std::cout << "New Ban:" << std::endl;
	LogPrintf("New Ban :\n");
	for (int i = 0; i < list.size(); i++)
	{
		std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).curBanList.begin(); it != list.at(i).curBanList.end(); it++) {
			std::cout << *it << " ";
			LogPrintf("%d	", *it);
		}
		std::cout << std::endl;
		LogPrintf("\n");
	}
	std::cout << std::endl;
	LogPrintf("\n");

	std::cout << "Cached Ban List:" << std::endl;
	LogPrintf("Cached Ban List:\n");
	for (int i = 0; i < list.size(); i++)
	{
		std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).cachedBanList.begin(); it != list.at(i).cachedBanList.end(); it++) {
			std::cout << *it << " ";
			LogPrintf("%d	", *it);
		}
		std::cout << std::endl;
		LogPrintf("\n");
	}
	std::cout << std::endl;
	LogPrintf("\n");

	std::cout << "Meeting Time:" << std::endl;
	LogPrintf("Meeting Time:\n");
	for (int i = 0; i < list.size(); i++)
	{
		std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		std::cout << list.at(i).meetingstarttime << "  " << list.at(i).meetingstoptime;

		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		LogPrintf("%u	%u", 
			list.at(i).meetingstarttime, list.at(i).meetingstoptime);
		
		std::cout << std::endl;
		LogPrintf("\n");
	}
	std::cout << std::endl;
	std::cout << "--------------------finish--------------------" << std::endl;
	std::cout << std::endl;
	LogPrintf("--------------------finish--------------------\n\n");

}


//初始化 静态变量
CConsensusAccountPool*  CConsensusAccountPool::_instance = nullptr;
std::once_flag CConsensusAccountPool::init_flag;

 CConsensusAccountPool::CConsensusAccountPool()
{
	 snapshotlist.clear();
	 m_mapSnapshotIndex.clear();
	 candidatelist.clear();
	 //tmpMinerList.clear();
	 tmpcachedIndexsToRefund.clear();//u64ConsensAccountSize
	 tmpcachedTimeoutToPunish.clear();
	 //tmpcachedcurCandidateIndexList.clear();
	 verifysuccessed = false;
	 analysisfinished = false;
	 m_strSnapshotPath = std::string("Snapshot");
	 m_strSnapshotIndexPath = std::string("SnapshotIndex");
	 m_strSnapshotRevPath = std::string("SnapshotRev");
	 m_strCandidatelistPath = std::string("Candidatelist");
	 m_u32WriteHeight = 0;

	 trustPKHashList.push_back("887f8428ddb84e05d3d32b86fb5a15a4d4bf9a0e");
	 trustPKHashList.push_back("fa9d550b92eda9e46b614c929475839be63e29bf");
	 trustPKHashList.push_back("605214fa27729b0da150c0553a381c13eff89b8c");
	 trustPKHashList.push_back("913aa01fc08e8c417a2bb188ec92d01dc7ff1821");
	 trustPKHashList.push_back("106058dba20c685d45d1efb609e60c5ed29ccb84");
	 trustPKHashList.push_back("308ad5ee64d0fa0a782463a74904034d4f3974bb");
	 trustPKHashList.push_back("2a8c1906e5b6a4b35e24376b0f56a4e965454ca5");

	 m_u64SnapshotIndexSize = getCSnapshotIndexSize();
	 m_u64ConsensAccountSize = getConsensAccountSize();
}
 CConsensusAccountPool::~CConsensusAccountPool()
{
	 std::cout << "CConsensusAccountPool Destruct begin" << std::endl;
	// writeCandidatelistToFile();
	 //flushSnapshotToDisk();
	 std::cout << "CConsensusAccountPool Destruct end" << std::endl;


	/* std::cout << "CConsensusAccountPool Destruct writeCandidatelistToFile end" << std::endl;
	 boost::filesystem::path pathTmp = GetDataDir() / m_strSnapshotPath;
	 FILE *file = fopen(pathTmp.string().c_str(), "ab+");
	 if (file != NULL)
	 {
		 FileCommit(file);
	 }
	 fclose(file);
	 std::cout << "CConsensusAccountPool Destruct FileCommitSnapshot end" << std::endl;

	 boost::filesystem::path pathTmpIndex = GetDataDir() / m_strSnapshotIndexPath;
	 FILE *fileIndex = fopen(pathTmpIndex.string().c_str(), "ab+");
	 if (fileIndex != NULL)
	 {
		 FileCommit(fileIndex);
	 }
	 fclose(fileIndex);

	 std::cout << "CConsensusAccountPool Destruct FileCommitIndex end" << std::endl;

	 boost::filesystem::path pathTmpRev = GetDataDir() / m_strSnapshotRevPath;
	 FILE *fileRev = fopen(pathTmpRev.string().c_str(), "ab+");
	 if (fileRev != NULL)
	 {
		 FileCommit(fileRev);
	 }
	 fclose(fileRev);
	 std::cout << "CConsensusAccountPool Destruct FileCommitRev end" << std::endl;*/

}

void CConsensusAccountPool::CreateInstance() {
	 static CConsensusAccountPool instance;
	 CConsensusAccountPool::_instance = &instance;
 }

//单例模式
CConsensusAccountPool&  CConsensusAccountPool::Instance()
{
	std::call_once(CConsensusAccountPool::init_flag, CConsensusAccountPool::CreateInstance);
	return *CConsensusAccountPool::_instance;
}


//##################################功能分界####################################

bool CConsensusAccountPool::getPublicKeyFromBlock(const CBlock *pblock, CPubKey& outPubkey,
	std::vector<unsigned char>&  outRecvSign)
{
	CTransactionRef coinbase = pblock->vtx[0];
	std::string strSignatrue = coinbase->vout[0].GetCheckBlockContent();
	if (strSignatrue.length() < 50) {
		return  false;
	}

	std::vector<unsigned char> vchReceive;
	if (!DecodeBase58(strSignatrue, vchReceive)) {
		std::cout << "[getPublicKeyFromBlock] DecodeBase58  vchReceive is err ===" << strSignatrue.length() << std::endl;
		return false;
	}

	uint32_t  siglen = vchReceive[0];
	uint32_t  sigPubkey = vchReceive[1 + siglen];
	//std::cout << "siglen=" << siglen << " sigPubkey=" << sigPubkey << "  vchReceive.size=" << vchReceive.size() << std::endl;

	//得到签名
	std::vector<unsigned char>  recvSign;
	recvSign.resize(siglen);
	for (std::vector<unsigned char>::size_type ix = 0; ix < siglen; ++ix) {
		recvSign[ix] = vchReceive[ix + 1];
	}
	//给到publickey
	std::vector<unsigned char>  recvPublickey;
	recvPublickey.resize(sigPubkey);
	for (std::vector<unsigned char>::size_type ix = 0; ix < sigPubkey; ++ix) {
		recvPublickey[ix] = vchReceive[ix + 2 + siglen];
	}

	CPubKey PublicKeyRevc(recvPublickey);
	outPubkey = PublicKeyRevc;
	outRecvSign.swap(recvSign);
	return true;
}

bool CConsensusAccountPool::getPublicKeyFromSignstring(const std::string signstr, CPubKey& outPubkey,
	std::vector<unsigned char>&  outRecvSign)
{
	if (signstr.length() < 50) {
		return  false;
	}

	std::vector<unsigned char> vchReceive;
	if (!DecodeBase58(signstr, vchReceive)) {
		std::cout << "[getPublicKeyFromSignstring] DecodeBase58  vchReceive is err ===" << signstr.length() << std::endl;
		return false;
	}

	uint32_t  siglen = vchReceive[0];
	uint32_t  sigPubkey = vchReceive[1 + siglen];
	//std::cout << "siglen=" << siglen << " sigPubkey=" << sigPubkey << "  vchReceive.size=" << vchReceive.size() << std::endl;

	//得到签名
	std::vector<unsigned char>  recvSign;
	recvSign.resize(siglen);
	for (std::vector<unsigned char>::size_type ix = 0; ix < siglen; ++ix) {
		recvSign[ix] = vchReceive[ix + 1];
	}
	//给到publickey
	std::vector<unsigned char>  recvPublickey;
	recvPublickey.resize(sigPubkey);
	for (std::vector<unsigned char>::size_type ix = 0; ix < sigPubkey; ++ix) {
		recvPublickey[ix] = vchReceive[ix + 2 + siglen];
	}

	CPubKey PublicKeyRevc(recvPublickey);
	outPubkey = PublicKeyRevc;
	outRecvSign.swap(recvSign);
	return true;
}

bool CConsensusAccountPool::verifyDPOCTx(const CTransaction& tx, DPOC_errtype &errorType)
{
	SnapshotClass lastsnapshot;
	if (!GetLastSnapshot(lastsnapshot))
	{
		std::cout << "[CConsensusAccountPool::verifyDPOCTx] 获取最后一个快照失败\n";
		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 获取最后一个快照失败\n");
		errorType = GET_LAST_SNAPSHOT_FAILED;
		return false;
	}

	if (tx.GetCampaignType() == TYPE_CONSENSUS_REGISTER)
	{
		std::cout << "verify join" << std::endl;
		//申请加入交易，获取公钥HASH值
		uint160 curHash = tx.GetCampaignPublickey();
		CAmount ipcvalue = tx.GetRegisterIPC();

		//加入交易金额不足设定值，拒绝--ban
		if (ipcvalue < Params().MIN_DEPOSI)
		{
			std::cout << "Register Value < " << Params().MIN_DEPOSI << std::endl;
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 押金 %d 不足设定值 %d\n", ipcvalue, Params().MIN_DEPOSI);
			errorType = JOIN_TRANS_DEPOSI_TOO_SMALL;
			return false;
		}

		//寻找下标ID
		uint16_t pkhashindex;
		if (!ContainPK(curHash, pkhashindex))
		{
			pkhashindex = candidatelist.size();
			//candidatelist.push_back(CConsensusAccount(curHash, ipcvalue));
			candidatelist.push_back(CConsensusAccount(curHash, ipcvalue,tx.GetHash()));

			//设置高度
			//candidatelist[curHash].setHeight(lastsnapshot.blockHeight + 1);
			//std::vector<CConsensusAccount>::iterator iter = candidatelist.end();
			//--iter;
			//iter->setHeight(lastsnapshot.blockHeight+1);
			//writeCandidatelistToFile(CConsensusAccount(curHash, ipcvalue, tx.GetHash()));
		}
// 		else
// 		{
// 			candidatelist[pkhashindex].setJoinIPC(ipcvalue);
// 			candidatelist[pkhashindex].setTxhash(tx.GetHash());
// 		}
		//如果已经在当前候选人列表中，则禁止再次加入
		//如果之前有退款正在处理中，则直到执行完退款交易都不能再次加入
		if (lastsnapshot.curCandidateIndexList.count(pkhashindex))
		{
			std::set<uint16_t>::iterator iter = lastsnapshot.curCandidateIndexList.begin();
			for (;iter!= lastsnapshot.curCandidateIndexList.end();++iter)
			{
				LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 公钥HASH++%d",*iter);
			}
			std::cout << "Account already existed in cached list or refunding" << std::endl;
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 公钥HASH已经在当前候选人列表中%d++%d\n", pkhashindex,lastsnapshot.blockHeight);
			errorType = JOIN_PUBKEY_ALREADY_EXIST_IN_LIST;
			return false;
		}
		else if (lastsnapshot.curBanList.count(pkhashindex) ||
			lastsnapshot.cachedBanList.count(pkhashindex))
		{
			std::cout << "Account Banned" << std::endl;
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 当前公钥HASH在黑名单列表中\n");
			errorType = JOIN_PUBKEY_IS_BANNED;
			return false;
		}
		else if (lastsnapshot.cachedTimeoutPunishToRun.count(pkhashindex) ||
			lastsnapshot.curTimeoutPunishList.count(pkhashindex))
		{
			std::cout << "Account timeout punish 1" << std::endl;
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 当前公钥HASH正被执行超时处罚\n");
			errorType = JOIN_PUBKEY_IS_TIMEOUT_PUNISHED;
			return false;
		}
		else if (lastsnapshot.cachedIndexsToRefund.count(pkhashindex) ||
			lastsnapshot.curRefundIndexList.count(pkhashindex))
		{
			std::cout << "Account refunding 1" << std::endl;
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 当前公钥HASH正在执行退款操作\n");
			errorType = JOIN_PUBKEY_IS_DEPOSING;
			return false;
		}
		else
		{
			//在缓存块长度覆盖的快照中如果有待执行的退款，也不允许其再次加入
			uint32_t cachedHeight = lastsnapshot.blockHeight - CACHED_BLOCK_COUNT;
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 准备获取块高度在%d-%d之间的缓存\n", cachedHeight, lastsnapshot.blockHeight);
			std::vector<SnapshotClass> cachedSnapshotList;
			if (!GetSnapshotsByHeight(cachedSnapshotList, cachedHeight))
			{
				LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 获取缓存的快照列表失败！\n");
				errorType = GET_CACHED_SNAPSHOT_FAILED;
				return false;
			}

			std::vector<SnapshotClass>::iterator pSnapshot;

			for (pSnapshot = cachedSnapshotList.begin(); pSnapshot != cachedSnapshotList.end(); pSnapshot++)
			{
				if ((*pSnapshot).curTimeoutPunishList.count(pkhashindex))
				{
					std::cout << "Account timeout punish" << std::endl;
					LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 当前公钥HASH正被执行超时处罚\n");
					errorType = EXIT_PUBKEY_IS_TIMEOUT_PUNISHED;
					return false;
				}
				if ((*pSnapshot).curRefundIndexList.count(pkhashindex))
				{
					std::cout << "Account refunding" << std::endl;
					LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 当前公钥HASH正在执行退款操作\n");
					errorType = EXIT_PUBKEY_IS_DEPOSING;
					return false;
				}
			}
		}
	}
	else if (tx.GetCampaignType() == TYPE_CONSENSUS_QUITE)
	{
		std::cout << "verify exit campaign" << std::endl;
		//申请退出交易，获取公钥HASH值
		uint160 curHash = tx.GetCampaignPublickey();
		//寻找下标ID
		uint16_t pkhashindex;
		if (!ContainPK(curHash, pkhashindex))
		{
			//记录里没有这条公钥hash，显然不能让退出
			std::cout << "Account didn't existed in pubkey record to exit" << std::endl;
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 公钥HASH不在索引vector中\n");
			errorType = EXIT_UNKNOWN_PUBKEY;
			return false;
		}

		//如果不在当前候选人列表中，不让退出
		if (!lastsnapshot.curCandidateIndexList.count(pkhashindex))
		{
			printsets(lastsnapshot.curCandidateIndexList);

			std::cout << "Account didn't existed in current candidate list" << std::endl;
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 公钥HASH不在当前候选人列表中\n");
			errorType = EXIT_PUBKEY_NOT_EXIST_IN_LIST;
			return false;
		}

	}
	//严重惩罚
	/*else if (tx.GetCampaignType() == TYPE_CONSENSUS_SEVERE_PUNISHMENT_REQUEST)
	{
		//对证据进行验签
		std::string rawEvidence = tx.GetCampaignEvidence();
		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 从交易中获取的原始证据=%s\n", rawEvidence.c_str());
		if (rawEvidence.empty())
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 证据格式不对，拒绝\n");
			errorType = EVIDENCE_ISEMPTY;
			return false;
		}
		
		UniValue params;
		if (!params.read(std::string("[") + rawEvidence + std::string("]")))
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] UniValue::read() failed\n");
			return false;
		}
		else if (!params.isArray())
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] params is not Array\n");
			return false;
		}
		else if (params.size() != 1)
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] params.size = %d, not 1\n", params.size());
			return false;
		}

		const UniValue& o = params[0].get_obj();
		const UniValue& pubtext = find_value(o, "badblockpubtexthex");
		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 明文=%s\n", pubtext.get_str().c_str());
		const UniValue& blocksign = find_value(o, "badblocksign");
		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 坏块全部签名报文=%s\n", blocksign.get_str().c_str());


		//获取块中的公钥和签名字符串
		CPubKey recvPublickey;
		std::vector<unsigned char>  recvSign;
		if (!getPublicKeyFromSignstring(blocksign.get_str(), recvPublickey, recvSign)) {
			std::cout << "CConsensusAccountPool::verifyDPOCTx: getPublicKeyFromSignstring failed" << std::endl;
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] getPublicKeyFromBlock is badlly \n");
			return false;
		}
		CKeyID  badpubicKey160hash = recvPublickey.GetID();
		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 证据中的公钥hash=%s\n", badpubicKey160hash.GetHex().c_str());

		std::vector<unsigned char> pubdata = ParseHex(pubtext.get_str());

		std::vector<unsigned char>::iterator byteit;
		int i = 0;
		unsigned char pubtextBuff[pubdata.size()];
		for (byteit = pubdata.begin(); byteit != pubdata.end(); byteit++, i++)
		{
			pubtextBuff[i] = *byteit;
		}

		uint256 hash;
		CHash256 hashoperator;
		hashoperator.Write(pubtextBuff, pubdata.size());
		hashoperator.Finalize(hash.begin());

		if (recvPublickey.Verify(hash, recvSign)) {
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx()]  recvPublickey.Verify  recvSign is OK\n");
		}
		else {
			std::cout << "CConsensusAccountPool::verifyDPOCTx(): Verify evidence sign failed" << std::endl;
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 块头信息校验失败！交易为伪造，拒绝！\n");
			return false;
		}

		if (tx.GetCampaignPublickey() != badpubicKey160hash)
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 惩罚交易指定的公钥HASH和证据中的HASH不一致！交易无效！\n");
			return false;
		}

		//如果待惩罚公钥HASH已经存在于黑名单中，直接返回正确


		//从明文中拆解信息，与快照做对比验证
		uint64_t badPeriodStartTime;
		uint32_t badPeriodCount;
		uint32_t badTimePeriod;
		uint32_t badBlockTime;
		uint64_t badCAmount;
		uint32_t badBlockHeight;
		uint256 badhashPrevBlock;

		memcpy((unsigned char*)&badPeriodStartTime, pubtextBuff, 8);
		memcpy((unsigned char*)&badPeriodCount, pubtextBuff + 8, 4);
		memcpy((unsigned char*)&badTimePeriod, pubtextBuff + 8 + 4, 4);
		memcpy((unsigned char*)&badBlockTime, pubtextBuff + 8 + 4 + 4, 4);
		memcpy((unsigned char*)&badCAmount, pubtextBuff + 8 + 4 + 4 + 4, 8);
		memcpy((unsigned char*)&badBlockHeight, pubtextBuff + 8 + 4 + 4 + 4 + 8, 4);
		memcpy((unsigned char*)badhashPrevBlock.begin(), pubtextBuff + 8 + 4 + 4 + 4 + 8 + 4, 32);

		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 证据包括的会议起始时间=%d, 会议人数=%d, 打块序号=%d, 打块实际时间=%d, Coinbase金额=%d, 打块高度=%d, 前置HASH=%s\n",
			badPeriodStartTime, badPeriodCount, badTimePeriod, badBlockTime, badCAmount, badBlockHeight, badhashPrevBlock.GetHex().c_str());

		//计算得到的应打块时间和实际打块时间相差超过5s，或者实际打块时间早于应打块时间。处罚交易验证通过
		if (badBlockTime < (badPeriodStartTime+ badTimePeriod*BLOCK_GEN_TIME+BLOCK_GEN_TIME)/1000 ||
			badBlockTime - MAX_BLOCK_TIME_DIFF > (badPeriodStartTime + badTimePeriod*BLOCK_GEN_TIME + BLOCK_GEN_TIME)/1000)
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 实际打块时间和应打块时间差超出范围，通过处罚\n");
			return true;
		}
		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 自身信息校验正确，准备Check匹配快照\n");

		//根据会议起始时间找到具备相同会议起始时间的快照，如果找不到，则往前推一轮，找结束时间等于当前起始时间的快照。如果找不到，说明该验证信息有问题。处罚交易验证通过
		SnapshotClass checkSnapshot;
		if (!GetSnapshotByTime(checkSnapshot, badPeriodStartTime/1000))
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 找不到早于证据中的会议起始时间%d的块，通过处罚\n", badPeriodStartTime);
			return true;
		}
		if (checkSnapshot.meetingstarttime != badPeriodStartTime && checkSnapshot.meetingstoptime != badPeriodStartTime)
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 找到的块既不是证据中会议同一轮，也不是证据中会议上一轮，通过处罚\n");
			return true;
		}

		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 找到匹配的前置快照，准备Check候选人列表\n");

		SnapshotClass checkCachedSnapshot;
		SnapshotClass checkCurSnapshot;
		//根据会议起始时间-缓存时间寻找缓存快照并得到候选人列表。如候选人个数不匹配，处罚交易验证通过。
		if (!GetSnapshotByTime(checkCachedSnapshot, (badPeriodStartTime - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME)/1000 ))
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 找不到缓存快照，无法得到候选人列表，通过处罚\n");
			return true;
		}

		//找上一个块，计算黑名单列表用
		if (!GetSnapshotByTime(checkCurSnapshot, (badPeriodStartTime + badTimePeriod * BLOCK_GEN_TIME)/1000))
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 找不到缓存快照，无法得到候选人列表，通过处罚\n");
			return true;
		}

		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 找到对应的候选人列表，准备Check数目\n");

		std::set<uint16_t> cachedAccounts = checkCachedSnapshot.curCandidateIndexList;
		//这里记得把黑名单里包括的人去掉，暂时先不做

		if (cachedAccounts.size() != badPeriodCount)
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx]候选人数目不匹配，通过处罚\n");
			return true;
		}

		//根据会议起始时间和候选人列表计算排序。
		//根据当前排序index，找到对应的应打块公钥，如应打块公钥和当前签名中公钥不匹配，处罚交易验证通过。
		std::list<std::shared_ptr<CConsensusAccount >> consensusList;
		consensusList.clear();
		std::set<uint16_t>::iterator candidateit;
		for (candidateit = cachedAccounts.begin(); candidateit != cachedAccounts.end(); candidateit++)
		{
			std::shared_ptr<CConsensusAccount> tmpaccount = std::make_shared<CConsensusAccount>(candidatelist.at(*candidateit));
			consensusList.push_back(tmpaccount);
		}

		bool result = CDpocMining::Instance().GetMeetingList(badPeriodStartTime, consensusList);
		if (result)
		{
			uint160 correctpubkeyhash;
			uint16_t correctpkhashindex;
			if (!getPKIndexBySortedIndex(correctpkhashindex, correctpubkeyhash, consensusList, badTimePeriod))
			{
				LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 从排序列表和块序号获取当前打块公钥失败\n");
				return false;
			}

			std::cout << "GetMeetingList get right hash=" << correctpubkeyhash.GetHex() << std::endl;
			if (badpubicKey160hash != correctpubkeyhash)
			{
				LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 公钥HASH不匹配，通过处罚\n");
				return true;
			}
		}

		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 打块人公钥HASH校验通过，查询是否连续出块\n");

		//快照中对应前置块高度有块，但是该高度的HASH和证据中前置HASH不一致，处罚交易验证通过。
		if (mapBlockIndex.count(badhashPrevBlock) == 0)
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 证据中包括的前置hash不在当前BlockChain中，通过处罚\n");
			return true;
		}

		CBlockIndex* prevBlockIndex = mapBlockIndex[badhashPrevBlock];
		if (prevBlockIndex->nHeight != badBlockHeight - 1)
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 前置HASH指向的块高度(%d)和证据中块高度的前置高度(%d)不一致，通过处罚\n",
				prevBlockIndex->nHeight, badBlockHeight-1);
			return true;
		}

		//如果前置hash的快照对应的应打块时间和证据对应的一致，证明是连续打块
		SnapshotClass prevSnapshot;
		if (!GetSnapshotByHeight(prevSnapshot, prevBlockIndex->nHeight))
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 获取前置HASH对应块的快照失败，不应该有这个错误\n");
			return false;
		}
		if (prevSnapshot.timestamp > (badPeriodStartTime + badTimePeriod*BLOCK_GEN_TIME) / 1000)
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 通过HASH找到的前置快照，其应打块时间与证据包括的应打块时间之差在一个周期之内，证明是连续打块，通过处罚\n");
			return true;
		}		

		//所有情况都不符合，拒绝该处罚交易
		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 错误的处罚交易！\n");
		return false;

	}*/

	return true;
}

bool CConsensusAccountPool::PunishBlock(const std::shared_ptr<const CBlock> pBadBlock, uint32_t badBlockHeight)
{
	return false;
	//构建证据，发送惩罚交易
	/*std::string pubtext;
	CAmount nfee = pBadBlock->vtx[0]->GetValueOut();

	unsigned char dataBuff[10240];
	int datalen = 0;
	memcpy(dataBuff, (unsigned char*)&(pBadBlock->nPeriodStartTime), sizeof(pBadBlock->nPeriodStartTime));
	datalen += sizeof(pBadBlock->nPeriodStartTime);
	memcpy(dataBuff + datalen, (unsigned char*)&(pBadBlock->nPeriodCount), sizeof(pBadBlock->nPeriodCount));
	datalen += sizeof(pBadBlock->nPeriodCount);
	memcpy(dataBuff + datalen, (unsigned char*)&(pBadBlock->nTimePeriod), sizeof(pBadBlock->nTimePeriod));
	datalen += sizeof(pBadBlock->nTimePeriod);
	memcpy(dataBuff + datalen, (unsigned char*)&(pBadBlock->nTime), sizeof(pBadBlock->nTime));
	datalen += sizeof(pBadBlock->nTime);
	memcpy(dataBuff + datalen, (unsigned char*)&nfee, sizeof(nfee));
	datalen += sizeof(nfee);
	memcpy(dataBuff + datalen, (unsigned char*)&badBlockHeight, sizeof(badBlockHeight));
	datalen += sizeof(badBlockHeight);
	memcpy(dataBuff + datalen, (unsigned char*)pBadBlock->GetBlockHeader().hashPrevBlock.begin(), 32);//256 bits
	datalen += 32;
	for (int i = 0; i < datalen; i++)
	{
		char tmp[3];
		sprintf(tmp, "%02x", dataBuff[i]);
		pubtext = pubtext + std::string(tmp);
	}

	CTransactionRef coinbase = pBadBlock->vtx[0];
	std::string strSignatrue = coinbase->vout[0].GetCheckBlockContent();
	std::string evidence = "{\"badblockpubtexthex\":\""+pubtext+"\",\"badblocksign\":\""+ strSignatrue +"\"}";
	LogPrintf("[CConsensusAccountPool::PunishBlock] 构造的证据=%s\n", evidence);

	//获取块中的公钥和签名字符串
	CPubKey recvPublickey;
	std::vector<unsigned char>  recvSign;
	if (!getPublicKeyFromBlock(pBadBlock.get(), recvPublickey, recvSign)) {
		std::cout << "CConsensusAccountPool::verifyDPOCBlock(): getPublicKeyFromBlock failed" << std::endl;
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock()] getPublicKeyFromBlock is badlly \n");
		return false;
	}
	CKeyID  publickey160hash = recvPublickey.GetID();

	//增加判断：如果这个错误块是自己打的，不做惩罚
	std::string selfpubkeystr;
	if (CDpocInfo::Instance().GetLocalAccount(selfpubkeystr))
	{
		uint160 selfpubkeyhash;
		selfpubkeyhash.SetHex(selfpubkeystr);
		if (publickey160hash == selfpubkeyhash)
		{
			LogPrintf("[CConsensusAccountPool::PunishBlock] 跟本地公钥一致，不构建惩罚交易，直接返回\n");
			return true;
		}
	}

	CWalletTx wtx;
	if (pwalletMain == NULL)
	{
		LogPrintf("[CConsensusAccountPool::PunishBlock] pwalletMain == NULL"); 
		return true;
	}

	if (pwalletMain->GetBroadcastTransactions() && !g_connman) {
		LogPrintf("[CConsensusAccountPool::PunishBlock] Error: Peer-to-peer functionality missing or disabled\n");
		return false;
	}
	LogPrintf("[CConsensusAccountPool::PunishBlock] pwalletMain != NULL");
	// Create and send the transaction
	CReserveKey reservekey(pwalletMain);
	CAmount nFeeRequired = 0;
	std::string strError;
	int nChangePosRet = -1;
	std::cout << "CConsensusAccountPool::PunishBlock:证据=" << evidence << std::endl;
	if (!pwalletMain->PunishRequest(publickey160hash, evidence, wtx, reservekey, nFeeRequired, nChangePosRet, strError)) {
		LogPrintf("[CConsensusAccountPool::PunishBlock] PunishRequest err = %s\n", strError);
		return false;
	}
	CValidationState state;
	if (!pwalletMain->CommitTransaction(wtx, reservekey, g_connman.get(), state)) {
		strError = strprintf("Error: The transaction was rejected! Reason given: %s", state.GetRejectReason());
		LogPrintf("[CConsensusAccountPool::PunishBlock] CommitTransaction err = %s\n", strError);
		return false;
	}

	std::cout << wtx.GetHash().GetHex() << std::endl;
	LogPrintf("[CConsensusAccountPool::PunishBlock] 惩罚公钥HASH=%s的块交易已发送，txid=%s\n", publickey160hash.GetHex().c_str(), wtx.tx->GetHash().GetHex().c_str());
	
	return true;
	*/
}

bool  CConsensusAccountPool::checkNewBlock(const std::shared_ptr<const CBlock> pblock, uint160 &pubkeyHash, uint32_t blockHeight, DPOC_errtype &errorType)
{
	//调用新增加的getmeetinglist来匹配规则，抛弃原来的做法

	LogPrintf("[CConsensusAccountPool::checkNewBlock] INBLOCK nPeriodStartTime %d, nPeriodCount %d, nTimePeriod %d, blockTime %d height=%d\n",
		pblock->nPeriodStartTime, pblock->nPeriodCount, pblock->nTimePeriod, pblock->nTime, blockHeight);

	SnapshotClass lastsnapshot;
	if (!GetLastSnapshot(lastsnapshot))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 获取快照记录尾块失败\n");
		return false;
	}
	//如果当前块高度高于尾快照高度+1，说明收到的块太新了，不应该处理
	if (blockHeight > lastsnapshot.blockHeight +1)
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 传入块太新了，无法验证\n");
		errorType = BLOCK_TOO_NEW_FOR_SNAPSHOT;
		return false;
	}

	//如果当前公钥在黑名单中，则拒绝该块
	uint16_t pkindex;
	if (!ContainPK(pubkeyHash, pkindex))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 打包公钥未知，拒绝\n");
		errorType = UNKNOWN_PACKAGE_PUBKEY;
		return false;
	}

	//2)   时间校验。
	//校验当前块以打包序号和会议开始时间计算得到的应打包时间，需要比之前的快照靠后
	int64_t calcTime = (pblock->nPeriodStartTime + (pblock->nTimePeriod + 1) * BLOCK_GEN_TIME) / 1000;
	LogPrintf("[CConsensusAccountPool::checkNewBlock] 当前计算得到的应打包时间=%d, 快照中最后一个块的应打包时间=%d\n", calcTime, lastsnapshot.timestamp);
	if (calcTime < (lastsnapshot.timestamp + BLOCK_GEN_TIME/1000 ))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 当前计算得到的应打包时间与快照中最后一个块差距小于%d ms，连续打块，严重处罚！\n", BLOCK_GEN_TIME);
		return false;
	}
	if (pblock->nTime != calcTime) {
		std::cout << "[CConsensusAccountPool::checkNewBlock]  pblock->nTime != calcTime" << std::endl;
		LogPrintf("[CConsensusAccountPool::checkNewBlock]  pblock->nTime != calcTime\n");
		/*if ((pblock->nTime - calcTime < 0 || pblock->nTime - calcTime > MAX_BLOCK_TIME_DIFF) &&
			lastsnapshot.blockHeight >= Params().CHECK_START_BLOCKCOUNT)
		{
			LogPrintf("[CConsensusAccountPool::checkNewBlock] 打包时间早于计算时间，或打包时间和计算时间相差大于%d秒，严重处罚\n", MAX_BLOCK_TIME_DIFF);
			PunishBlock(pblock, blockHeight);
			//errorType = PUNISH_BLOCK;
			return false;
		}*/
		if ((pblock->nTime - calcTime < 0) &&(lastsnapshot.blockHeight >= Params().CHECK_START_BLOCKCOUNT))
		{
			LogPrintf("[CConsensusAccountPool::checkNewBlock] 打包时间早于计算时间，或打包时间和计算时间相差大于%d秒，严重处罚\n", MAX_BLOCK_TIME_DIFF);
			errorType = PUNISH_BLOCK;
			return false;
		}
		else if ((pblock->nTime - calcTime > MAX_BLOCK_TIME_DIFF) &&(lastsnapshot.blockHeight >= Params().CHECK_START_BLOCKCOUNT))
		{   //有可能打块延迟
			LogPrintf("[CConsensusAccountPool::checkNewBlock] 打包时间早于计算时间，或打包时间和计算时间相差大于%d秒，严重处罚\n", MAX_BLOCK_TIME_DIFF);
			return false;
		}
	}
	std::cout << "calcTime check passed" << std::endl;

	//寻找当前会议起始时间对应快照
	int64_t curStarttime = pblock->nPeriodStartTime / 1000;
	SnapshotClass meetingStartSnapshot;
	if (!GetSnapshotByTime(meetingStartSnapshot, curStarttime))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 获取本轮会议起始时间对应快照失败\n");
		return false;
	}


	std::cout << "search map get height=" << meetingStartSnapshot.blockHeight << std::endl;
	LogPrintf("[CConsensusAccountPool::checkNewBlock] 根据当前块起始时间%d 在快照列表中找到的块高度为%d\n",
		curStarttime, meetingStartSnapshot.blockHeight);
	//从开始检测的块高度起就应该正常开会了，块头时间早于第一个块，有BUG
	if (meetingStartSnapshot.blockHeight == 0 && meetingStartSnapshot.timestamp > curStarttime)
	{
		std::cout << "Invalid Block StartHeight!" << std::endl;
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 当前块时间早于创世块，错误，严重处罚！\n");
		errorType = PUNISH_BLOCK;
		return false;
	}

	//时间不匹配，可能是漏块也可能是错块
	if (meetingStartSnapshot.timestamp != curStarttime 
		&& meetingStartSnapshot.blockHeight>= Params().CHECK_START_BLOCKCOUNT) 
	{
		std::cout << "search map get time=" << meetingStartSnapshot.timestamp << ", curstarttime=" << curStarttime << std::endl;
		std::cout << "当前块起始时间与找到的快照时间不一致" << std::endl;
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 当前块起始时间与根据该时间找到的快照打包时间不一致，需要进一步校验是否为漏块。curstarttime=%d，snapshot time=%d\n",
			curStarttime, meetingStartSnapshot.timestamp);

		//漏块情况处理：首先忽略掉漏掉整整一轮块的可能性
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 快照的会议起始时间=%d, 结束时间=%d\n",
			meetingStartSnapshot.meetingstarttime, meetingStartSnapshot.meetingstoptime);
		//判断找到的上一个块的结束时间是否为当前块的起始时间，如果是，说明中间漏掉的块跨轮了，块仍然是有效的。
		if (meetingStartSnapshot.meetingstoptime != pblock->nPeriodStartTime)
		{
			LogPrintf("[CConsensusAccountPool::checkNewBlock] 快照的会议结束时间不等于当前块会议起始时间，可能漏块超过一轮，错误\n");
			return false;
		}
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 快照的会议结束时间等于当前块会议起始时间，有跨轮漏块现象\n");
	}
	//6
	//按照找到的块的结束时间（即当前轮的开始时间）往前减去 缓存长度*打包时间 以该时间为准找到缓存块
	uint64_t meetingstarttime = meetingStartSnapshot.meetingstoptime;
	int foundcachedcount = CACHED_BLOCK_COUNT;
	if (meetingStartSnapshot.blockHeight < Params().CHECK_START_BLOCKCOUNT)
	{
		foundcachedcount = meetingStartSnapshot.blockHeight < CACHED_BLOCK_COUNT ? meetingStartSnapshot.blockHeight : CACHED_BLOCK_COUNT;
	}
	uint32_t cachedTime = (meetingStartSnapshot.meetingstoptime - foundcachedcount * BLOCK_GEN_TIME) / 1000;
	LogPrintf("[CConsensusAccountPool::checkNewBlock] 计算得到的缓存时间为 %d \n", cachedTime);
	
	//为本轮打块的共识人
	SnapshotClass cachedsnapshot;
	if (!GetSnapshotByTime(cachedsnapshot, cachedTime))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 获取缓存时间对应快照失败，打块信息有问题，严重处罚！\n");
		return false;
	}
	
	LogPrintf("[CConsensusAccountPool::checkNewBlock] 找到的块高度= %d , 找到的块时间= %d \n",
		cachedsnapshot.blockHeight, cachedsnapshot.timestamp);
	
	//To Do: 从候选人列表中去掉黑名单【这一步暂时不做，只是拒收黑名单中的块】
	std::set<uint16_t> accounts = cachedsnapshot.curCandidateIndexList;
	
	//7
	if (accounts.size() != pblock->nPeriodCount && cachedsnapshot.curCandidateIndexList.size() != pblock->nPeriodCount)
	{
		std::cout << "accounts.size()++" << accounts.size() << "++pblock->nPeriodCount++" << pblock->nPeriodCount << std::endl;
		std::cout << "cachedsnapshot.curCandidateIndexList.size()++" << cachedsnapshot.curCandidateIndexList.size()
			<< "+++pblock->nPeriodCount++" << pblock->nPeriodCount << std::endl;

		std::cout << "Meeting Account counts not match, forbidden" << std::endl;
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 块中记录的开会人数和快照中的开会人数不一致，也和去掉黑名单后的人数不一致。块中开会人数=%d,从快照计算的开会人数=%d，去掉黑名单后的人数=%d，严重处罚！\n",
			pblock->nPeriodCount,cachedsnapshot.curCandidateIndexList.size(), accounts.size());
		
		return false;
	}

	std::cout << "found match cached block " << cachedsnapshot.blockHeight << std::endl;
	printsets(accounts);
	std::list<std::shared_ptr<CConsensusAccount >> consensusList;
	consensusList.clear();
	std::set<uint16_t>::iterator candidateit;
	for (candidateit= accounts.begin(); candidateit != accounts.end(); candidateit++)
	{
		std::shared_ptr<CConsensusAccount> tmpaccount = std::make_shared<CConsensusAccount>(candidatelist.at(*candidateit));
		consensusList.push_back(tmpaccount);
	}

	//8
	bool result = CDpocMining::Instance().GetMeetingList(meetingstarttime, consensusList);
	std::cout<< "GetMeetingList result=" << result << " list size ="<<consensusList.size() << std::endl;
	if (result)
	{
		//按顺序打印本轮会议公钥HASH索引值
		LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 起始时间为%d的会议打块人排序：\n", pblock->nPeriodStartTime);
		std::list<std::shared_ptr<CConsensusAccount >>::iterator consensusListIt;
		for (consensusListIt = consensusList.begin(); consensusListIt != consensusList.end(); consensusListIt++)
		{
			uint16_t tmpIndex = -1;
			uint160 tmppkhash = (*consensusListIt)->getPubicKey160hash();
			ContainPK(tmppkhash, tmpIndex);
			LogPrintf("%d ", tmpIndex);
		}
		LogPrintf("\n\n");

		uint160 correctpubkeyhash;
		uint16_t correctpkhashindex;
		if (!getPKIndexBySortedIndex(correctpkhashindex, correctpubkeyhash, consensusList, pblock->nTimePeriod))
		{
			LogPrintf("[CConsensusAccountPool::checkNewBlock] 从排序列表和块序号获取当前打块公钥失败\n");
			return false;
		}


		std::cout << "GetMeetingList get right hash=" << correctpubkeyhash.GetHex() << std::endl;
		if (pubkeyHash != correctpubkeyhash)
		{
			std::cout << "block hash=" << pubkeyHash.GetHex() << ", right hash in Meeting=" << correctpubkeyhash.GetHex() << std::endl;
			std::cout << "mismatching meeting publickey hash" << std::endl;
			LogPrintf("[CConsensusAccountPool::checkNewBlock] 打包人公钥HASH不匹配，块中PKHash=%s，从快照计算得到的正确PKHash=%s，严重处罚！\n",
				pubkeyHash.GetHex().c_str(), correctpubkeyhash.GetHex().c_str());
			return false;
		}
	}
	else
	{
		std::cout << "get sorted accounts list failed" << std::endl;
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 获取会议排序列表失败\n");
		return false;
	}
	std::cout << "Account in meeting check passed" << std::endl;

	//检查HASH跟快照尾对应的前置块HASH是否一致
	if (!mapBlockIndex.count(pblock->GetBlockHeader().hashPrevBlock))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 当前列表中没有该block的前置HASH %s，严重处罚！\n", pblock->GetBlockHeader().hashPrevBlock.GetHex().c_str());

		return false;
	}
	CBlockIndex* pblockindex = mapBlockIndex[pblock->GetBlockHeader().hashPrevBlock];
	if (pblockindex->nHeight != lastsnapshot.blockHeight)
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 该block的前置HASH %s 对应的前置块高度为%d, 快照列表末尾块高度为%d ,不匹配，严重处罚！\n",
			pblock->GetBlockHeader().hashPrevBlock.GetHex().c_str(), pblockindex->nHeight, lastsnapshot.blockHeight);
		
		return false;
	}

	std::cout << "checkNewBlock exit" << std::endl;
	return true;
}

bool CConsensusAccountPool::verifyDPOCBlock(const std::shared_ptr<const CBlock> pblock, uint32_t blockHeight, DPOC_errtype &rejectreason)
{
	std::cout << "CConsensusAccountPool::verifyDPOCBlock() called" << std::endl;

	LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 请求读写锁\n");
	writeLock wtlock(checkmutex);
	LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 进入锁\n");

	//获取块中的公钥和签名字符串
	CPubKey recvPublickey;
	std::vector<unsigned char>  recvSign;
	if (!getPublicKeyFromBlock(pblock.get(), recvPublickey, recvSign)) {
		std::cout << "CConsensusAccountPool::verifyDPOCBlock(): getPublicKeyFromBlock failed" << std::endl;
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock()] getPublicKeyFromBlock is badlly is MisbeHaving\n");

		//11-10修改-应该ban
		rejectreason = PUNISH_BLOCK;
		return false;
	}
	CKeyID  pubicKey160hash = recvPublickey.GetID();

	//special process for the 50759
	//if ((50761 == blockHeight) || (51992 == blockHeight)|| (129705 == blockHeight) || (129706 == blockHeight) || (129707 == blockHeight) || (129708 == blockHeight))
	//if(129800 >  blockHeight)
	if ((50761 == blockHeight) || (51992 == blockHeight)) 
	{
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 50759 ++%d+++%d+++%d++++%d\n", pblock->nPeriodCount,
			pblock->nTimePeriod, pblock->nPeriodStartTime, pblock->nTime);
		//string strP;
		//CDpocInfo::Instance.GetLocalAccount(strP);
		/*for (std::vector<std::string>::iterator trustit = trustPKHashList.begin();
			trustit != trustPKHashList.end(); trustit++)
		{
			uint160 uIN;
			uIN.SetHex(*trustit);
			LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 50759 -- %s  %s \n", *trustit, pubicKey160hash.GetHex().c_str());
			if (pubicKey160hash == uIN)
			{
				LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 50759 ok !");
				return true;
			}
		}
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 50759 true !");
		return false;*/
		return true;
	}

	if ((blockHeight<130296) && (blockHeight>=129705))
	{
		return true;
	}

	if ((blockHeight == 173677)&&(pblock->nPeriodStartTime > 1512194220000))
	{
		return true;
	}

	if (blockHeight == 173883)
	{
		return true;
	}

	if ((blockHeight >= 218079) && (blockHeight <= 535430))
	{
		return true;
	}	

	//时间校验。测试网络前120个块略过，正式网络从第1个块起校验
	if (blockHeight >= Params().CHECK_START_BLOCKCOUNT)
	{
		//验签
		uint256 hash;
		CAmount nfee = pblock->vtx[0]->GetValueOut();
		CHash256 hashoperator;

		hashoperator.Write((unsigned char*)&(pblock->nPeriodStartTime), sizeof(pblock->nPeriodStartTime));//起始时间，8Byte
		hashoperator.Write((unsigned char*)&(pblock->nPeriodCount), sizeof(pblock->nPeriodCount));//会议总人数，4Byte
		hashoperator.Write((unsigned char*)&(pblock->nTimePeriod), sizeof(pblock->nTimePeriod));//打块人序号，4Byte
		hashoperator.Write((unsigned char*)&(pblock->nTime), sizeof(pblock->nTime));//实际打块时间，4Byte
		hashoperator.Write((unsigned char*)&nfee, sizeof(nfee));//总费用，8Byte
		hashoperator.Write((unsigned char*)&blockHeight, sizeof(blockHeight));//总费用，8Byte
		hashoperator.Write((unsigned char*)pblock->GetBlockHeader().hashPrevBlock.begin(), 32);//前置块hash， 256bit
		hashoperator.Finalize(hash.begin());

		unsigned char dataBuff[10240];
		int datalen = 0;
		memcpy(dataBuff, (unsigned char*)&(pblock->nPeriodStartTime), sizeof(pblock->nPeriodStartTime));
		datalen += sizeof(pblock->nPeriodStartTime);
		memcpy(dataBuff + datalen, (unsigned char*)&(pblock->nPeriodCount), sizeof(pblock->nPeriodCount));
		datalen += sizeof(pblock->nPeriodCount);
		memcpy(dataBuff + datalen, (unsigned char*)&(pblock->nTimePeriod), sizeof(pblock->nTimePeriod));
		datalen += sizeof(pblock->nTimePeriod);
		memcpy(dataBuff + datalen, (unsigned char*)&(pblock->nTime), sizeof(pblock->nTime));
		datalen += sizeof(pblock->nTime);
		memcpy(dataBuff + datalen, (unsigned char*)&nfee, sizeof(nfee));
		datalen += sizeof(nfee);
		memcpy(dataBuff + datalen, (unsigned char*)&blockHeight, sizeof(blockHeight));
		datalen += sizeof(blockHeight);
		memcpy(dataBuff + datalen, (unsigned char*)pblock->GetBlockHeader().hashPrevBlock.begin(), 32);//256 bits
		datalen += 32;
		std::string datahex;
		for (int i = 0; i < datalen; i++)
		{
			char tmp[3];
			sprintf(tmp, "%02x", dataBuff[i]);
			datahex = datahex + std::string(tmp);
		}
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock()] 验签明文=%s , 明文HASH=%s , 公钥HASH=%s\n", datahex.c_str(), hash.GetHex().c_str(), pubicKey160hash.GetHex().c_str());

		if (recvPublickey.Verify(hash, recvSign)) {
			LogPrintf("[CConsensusAccountPool::verifyDPOCBlock()]  recvPublickey.Verify  recvSign is OK\n");
		}
		else {
			std::cout << "CConsensusAccountPool::verifyDPOCBlock(): Verify block sign failed" << std::endl;
			LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 块头信息校验失败！该块为伪造，无法验证身份，无法处罚\n");
			
			//11-10Test
			rejectreason = PUNISH_BLOCK;
			return false;
		}
		//取当前会议记账排序列表，校验是否轮到该人打块
		if (!checkNewBlock(pblock, pubicKey160hash, blockHeight, rejectreason)) {
			std::cout << "Block Check Failed" << std::endl;
			LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 块验证 checkNewBlock 失败\n");
			return false;
		}
		//std::cout << "VerifyDPOCBlock() finish calling for checkNewBlock" << std::endl;
	}


	//按照块高度去list中取缓存的快照，计算当前临时快照
	//std::cout << "current snapshot list size = " << snapshotlist.size() << ", current block height=" << blockHeight << std::endl;
	
	tmpcachedIndexsToRefund.clear();
	tmpcachedTimeoutToPunish.clear();
	verifysuccessed = false;

	SnapshotClass lastSnapshot;
	if (GetLastSnapshot(lastSnapshot))
	{
		tmpcachedIndexsToRefund = lastSnapshot.cachedIndexsToRefund;
		tmpcachedTimeoutToPunish = lastSnapshot.cachedTimeoutPunishToRun;

		uint32_t cachedHeight = (lastSnapshot.blockHeight > CACHED_BLOCK_COUNT)?(lastSnapshot.blockHeight-CACHED_BLOCK_COUNT):0;
		SnapshotClass cachedSnapshot;
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 欲获取缓存高度%d的快照\n", cachedHeight);
		if (!GetSnapshotByHeight(cachedSnapshot, cachedHeight))
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 获取缓存快照失败\n");
			return false;
		}
		std::set<uint16_t>::iterator it;
		for (it = cachedSnapshot.curRefundIndexList.begin(); it != cachedSnapshot.curRefundIndexList.end(); it++)
		{
			std::cout << "get cached refund " << *it << std::endl;
			if (!tmpcachedIndexsToRefund.count(*it))
				tmpcachedIndexsToRefund.insert(*it);
		}
		std::cout << "Current Candidate Indexs To Refund:" << std::endl;
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock]当前保存的等待退款的临时列表为：\n");
		printsets(tmpcachedIndexsToRefund);

		for (it = cachedSnapshot.curTimeoutPunishList.begin(); it != cachedSnapshot.curTimeoutPunishList.end(); it++)
		{
			std::cout << "get cached refund " << *it << std::endl;
			if (!tmpcachedTimeoutToPunish.count(*it))
				tmpcachedTimeoutToPunish.insert(*it);
		}
		std::cout << "Current Candidate Indexs To Timeout Punish:" << std::endl;
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock]当前保存的准备执行超时惩罚的临时列表为：\n");
		printsets(tmpcachedTimeoutToPunish);

		//分析交易
		for (const auto& tx : pblock->vtx)
		{
			if (blockHeight > 127000)
			{
				if (tx->IsCoinBase())
				{
					LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 检验Coinbase交易\n");
					//读CoinBase，从缓存列表中去掉已经执行的退款交易
					for (const CTxOut& txout : tx->vout)
					{
						if (txout.GetCampaignType() == TYPE_CONSENSUS_RETURN_DEPOSI)
						{
							uint16_t pkindex;
							if (!ContainPK(txout.devoteLabel.hash, pkindex))
							{
								LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 退款公钥不在缓存列表中\n");
								return false;
							}

							if (!tmpcachedIndexsToRefund.count(pkindex))
							{
								LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 当前缓存列表中没有退款记录\n");
								return false;
							}
						}
						else if (txout.GetCampaignType() == TYPE_CONSENSUS_ORDINARY_PUSNISHMENT)
						{
							uint16_t pkindex;
							if (!ContainPK(txout.devoteLabel.hash, pkindex))
							{
								LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 惩罚公钥不在缓存列表中\n");
								return false;
							}
							if (!tmpcachedTimeoutToPunish.count(pkindex))
							{
								LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 当前缓存列表中没有惩罚记录\n");
								return false;
							}

						}
					}
				}
			}

			if (tx->GetTxType() != 1)
				continue;

			//申请加入退出--普通交易
			DPOC_errtype errtype;
			if (!verifyDPOCTx(*tx, errtype))
			{
				LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] verifyDPOCTx返回的错误类型为 %d\n", errtype);
				return false;
			}
		}
		
	}
	else
	{
		std::cout << "snapshotlist is empty!" << std::endl;
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock]获取快照末尾失败，快照列表可能为空！\n");
	}
	
	
	//所有校验做完后，标记验证位为真，pushblock根据标记位决定是否使用中间变量
	std::cout << "Verify Passed" << std::endl;
	verifysuccessed = true;
	std::cout << "VerifyDPOCBlock(): success exit" << std::endl << std::endl;

	LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] 退出锁\n");
	return true;
}

bool CConsensusAccountPool::ContainPK(uint160 pk, uint16_t& index)
{
	for (int i = 0; i < candidatelist.size(); i++) {
		if (candidatelist.at(i).getPubicKey160hash() == pk)
		{
			index = i;
			return true;
		}
	}
	index = -1;
	return false;
}

bool CConsensusAccountPool::contain(std::vector<std::pair<uint16_t, int64_t>> list, uint16_t indexIn)
{
	std::vector<std::pair<uint16_t, int64_t>>::iterator it;
	for (it=list.begin(); it!=list.end(); it++)
	{
		if ((*it).first == indexIn)
			return true;
	}
	return false;
}

CAmount  CConsensusAccountPool::GetDepositBypkhash(uint160 pkhash)
{
	CAmount nValue = 0;
	uint16_t index = -1;
	if (!ContainPK(pkhash, index))
	{
		LogPrintf("[CConsensusAccountPool::GetDepositBypkhash] 候选人列表中查不到当前记账的hash\n");
		return nValue;
	}

	return candidatelist[index].getJoinIPC();
}
uint256 CConsensusAccountPool::GetTXhashBypkhash(uint160 pkhash)
{
	uint256 hash ;
	hash.SetNull();
	uint16_t index = -1;
	if (!ContainPK(pkhash, index))
	{
		LogPrintf("[CConsensusAccountPool::GetDepositBypkhash] 候选人列表中查不到当前记账的hash\n");
		return hash;
	}

	return candidatelist[index].getTxhash();
}
bool CConsensusAccountPool::IsAviableUTXO(const uint256 hash)
{
	LogPrintf("[CConsensusAccountPool::IsAviableUTXO] 获取utxo状态\n");
	SnapshotClass cursnapshot;
	if (!GetLastSnapshot(cursnapshot))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] 获取快照记录尾块失败\n");
		return false;
	}
	//判断当前txid是否在候选人列表 或 执行退款列表中，在则返回false
	std::set<uint16_t>::iterator refundit;
	for (refundit = cursnapshot.curCandidateIndexList.begin(); refundit != cursnapshot.curCandidateIndexList.end(); refundit++)
	{
		if (candidatelist[*refundit].getTxhash() == hash)
		return false;
	}
	for (refundit = cursnapshot.cachedIndexsToRefund.begin(); refundit != cursnapshot.cachedIndexsToRefund.end(); refundit++)
	{
		if (candidatelist[*refundit].getTxhash() == hash)
		return false;
	}
	for (refundit = cursnapshot.cachedTimeoutPunishToRun.begin(); refundit != cursnapshot.cachedTimeoutPunishToRun.end(); refundit++)
	{
		if (candidatelist[*refundit].getTxhash() == hash)
		return false;
	}

	//取缓存长度的快照列表，并判断是否在该缓存长度快照的退款申请列表中，在则返回false
	std::vector<SnapshotClass> cachedSnapshots;
	uint32_t cachedHeight = cursnapshot.blockHeight - CACHED_BLOCK_COUNT;
	LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 准备获取块高度在%d-%d之间的缓存\n", cachedHeight, cursnapshot.blockHeight);
	if (!GetSnapshotsByHeight(cachedSnapshots, cachedHeight))
	{
		LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 获取缓存的快照列表失败\n");
		return false;
	}
	std::vector<SnapshotClass>::iterator searchIt;
	bool founded = false;
	for (searchIt = cachedSnapshots.begin(); searchIt != cachedSnapshots.end(); searchIt++)
	{

		for (refundit = (*searchIt).curRefundIndexList.begin(); refundit != (*searchIt).curRefundIndexList.end(); refundit++)
		{
			if (candidatelist[*refundit].getTxhash() == hash)
			return false;
		}
		for (refundit = (*searchIt).curTimeoutPunishList.begin(); refundit != (*searchIt).curTimeoutPunishList.end(); refundit++)
		{
			if (candidatelist[*refundit].getTxhash() == hash)
			return false;
		}
	}

	//都不在，则返回true
	return true;
}

bool CConsensusAccountPool::getCreditFromSnapshotByIndex(SnapshotClass snapshot, const uint16_t indexIn, int64_t &credit)
{
	LogPrintf("[CConsensusAccountPool::getCreditFromSnapshotByIndex] called\n");
	//从当前会议列表中搜寻index所对应的值
	for (std::vector<std::pair<uint16_t, int64_t>>::iterator accountit = snapshot.cachedMeetingAccounts.begin();
		accountit != snapshot.cachedMeetingAccounts.end(); accountit ++)
	{
		if ( (*accountit).first == indexIn)//找到当前快照中会议记账信息，读取信用值并返回
		{
			credit = (*accountit).second;
			LogPrintf("[CConsensusAccountPool::getCreditFromSnapshotByIndex] 从快照中读到索引%d的信用值%d\n", indexIn, credit);
			return true;
		}
	}

	return false;
}

bool CConsensusAccountPool::pushDPOCBlock(const std::shared_ptr<const CBlock> pblock, uint32_t blockHeight)
{
	std::cout << "pushDPOCBlock(): in" << std::endl;
	LogPrintf("[CConsensusAccountPool::pushDPOCBlock] IN block height= %d , timestamp= %d , realtime = %d , nPeriodStartTime= %d , accountNumber= %d , accountindex= %d\n",
		blockHeight, (pblock->nPeriodStartTime + (pblock->nTimePeriod + 1) * BLOCK_GEN_TIME) / 1000,
		pblock->nTime, pblock->nPeriodStartTime, pblock->nPeriodCount, pblock->nTimePeriod);


	SnapshotClass newsnapshot;
	//std::cout << "snapshotlist size=" << snapshotlist.size() << ", blockheight=" << blockHeight << std::endl;
	newsnapshot.blockTime = pblock->nTime;
	//std::cout << "pushDPOCBlock(): inblock time= " << pblock->nTime << std::endl;
	newsnapshot.timestamp = (pblock->nPeriodStartTime + (pblock->nTimePeriod + 1) * BLOCK_GEN_TIME) / 1000;
	newsnapshot.meetingstarttime = pblock->nPeriodStartTime;
	//std::cout << "pushDPOCBlock(): inblock Periodstarttime= " << pblock->nPeriodStartTime << std::endl;
	newsnapshot.meetingstoptime = pblock->nPeriodStartTime + pblock->nPeriodCount * BLOCK_GEN_TIME;
	//newsnapshot.cachedMeetingAccounts = tmpMinerList;
	newsnapshot.blockHeight = blockHeight;

	LogPrintf("[CConsensusAccountPool::pushDPOCBlock] tmp snapshot height= %d , timestamp= %d , realtime = %d , meetingstarttime= %d , meetingstoptime= %d\n",
		newsnapshot.blockHeight, newsnapshot.timestamp, newsnapshot.blockTime,
		newsnapshot.meetingstarttime, newsnapshot.meetingstoptime);

	SnapshotClass lastSnapshot;
	if (!GetLastSnapshot(lastSnapshot))//传世块
	{
		//创世块被push进来的时候。需要添加首批会议名单	
		uint160 MeetingHash;
		CAmount ipcvalue;
		MeetingHash.SetHex(trustPKHashList[0]);
		ipcvalue = Params().MIN_DEPOSI + CACHED_BLOCK_COUNT;
		candidatelist.push_back(CConsensusAccount(MeetingHash, ipcvalue));

		//设置加入共识高度0
		//std::vector<CConsensusAccount>::iterator iter = candidatelist.end();
		//--iter;
		//candidatelist[MeetingHash].setHeight(0);

		//writeCandidatelistToFile(CConsensusAccount(MeetingHash, ipcvalue));
		newsnapshot.pkHashIndex = 0;
		newsnapshot.curCandidateIndexList.insert(0);
		newsnapshot.cachedMeetingAccounts.push_back(std::make_pair(0, 0));

		newsnapshot.curTimeoutIndexRecord.clear();
		newsnapshot.cachedBanList.clear();
		newsnapshot.timestamp = newsnapshot.meetingstoptime / 1000;//创世块的打块时间跟会议结束时间要一致
	}
	else
	{
		newsnapshot.curCandidateIndexList = lastSnapshot.curCandidateIndexList;
		newsnapshot.cachedIndexsToRefund = lastSnapshot.cachedIndexsToRefund;
		newsnapshot.cachedTimeoutPunishToRun = lastSnapshot.cachedTimeoutPunishToRun;
		newsnapshot.curTimeoutIndexRecord = lastSnapshot.curTimeoutIndexRecord;
		newsnapshot.cachedBanList = lastSnapshot.cachedBanList;
		//从退款名单中删除黑名单包括的索引
		std::set<uint16_t>::iterator banit;
		for (banit = newsnapshot.cachedBanList.begin(); banit != newsnapshot.cachedBanList.end(); banit++)
		{
			if (newsnapshot.cachedIndexsToRefund.count(*banit))
			{
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 从退款名单中去掉黑名单包括的索引%d\n",*banit);
				newsnapshot.cachedIndexsToRefund.erase(*banit);
			}
		}

		if (verifysuccessed)
		{
			std::set<uint16_t>::iterator itforrefund;
			for (itforrefund = tmpcachedIndexsToRefund.begin(); itforrefund != tmpcachedIndexsToRefund.end(); itforrefund++)
			{
				newsnapshot.cachedIndexsToRefund.insert(*itforrefund);
			}
			for (itforrefund = tmpcachedTimeoutToPunish.begin(); itforrefund != tmpcachedTimeoutToPunish.end(); itforrefund++)
			{
				newsnapshot.cachedTimeoutPunishToRun.insert(*itforrefund);
			}
		}

		//根据当前轮会议开始时间 减去缓存时间 找到快照
		//std::cout << "pushDPOCBlock(): meetingstarttime= " << pblock->nPeriodStartTime << std::endl;
		uint64_t cacheTime = (pblock->nPeriodStartTime - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME) / 1000;
		//std::cout << "pushDPOCBlock(): cacheTime= " << cacheTime << std::endl;
		SnapshotClass cachedsnapshot;
		if (!GetSnapshotByTime(cachedsnapshot, cacheTime))
		{
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 读取缓存快照失败！\n");
			return false;
		}

		//获取块中的公钥和签名字符串
		CPubKey recvPublickey;
		std::vector<unsigned char>  recvSign;
		if (!getPublicKeyFromBlock(pblock.get(), recvPublickey, recvSign)) {
			std::cout << "CConsensusAccountPool::pushDPOCBlock(): getPublicKeyFromBlock failed" << std::endl;
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 获取块中的公钥和签名字符串失败 \n");
			return false;
		}
		CKeyID  curHash = recvPublickey.GetID();
		uint16_t curblockpkhashindex;
		if (!ContainPK(curHash, curblockpkhashindex))
		{
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 打块公钥不在缓存列表中，错误\n");
			return false;
		}
		LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 当前打块公钥 %s 索引值%d\n", curHash.GetHex().c_str(), curblockpkhashindex);
		newsnapshot.pkHashIndex = curblockpkhashindex;//将打块公钥索引值记入快照

		//增加打块公钥信用值
		CConsensusAccount tmpaccount = candidatelist[curblockpkhashindex];
		tmpaccount.setCredit(tmpaccount.getCredit() + PACKAGE_CREDIT_REWARD);
		candidatelist[curblockpkhashindex] = tmpaccount;
		//add begin by li at 2017/11/20
		//candidatelist[curblockpkhashindex].setHeight(lastSnapshot.blockHeight+1);
		//add end by li at 2017/11/20
		LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 增加公钥索引%d的信用值到%d\n", curblockpkhashindex, candidatelist[curblockpkhashindex].getCredit());

		//计算当前块到上一个块超时违规情况
		if (snapshotlist.size() > Params().CHECK_START_BLOCKCOUNT)
		{
			//去掉该块的超时记录
			if (newsnapshot.curTimeoutIndexRecord.count(curblockpkhashindex) /*&&
				newsnapshot.curTimeoutIndexRecord[curblockpkhashindex] < MAX_TIMEOUT_COUNT*/)
			{
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 删除该块的超时记录\n");
				newsnapshot.curTimeoutIndexRecord.erase(curblockpkhashindex);
			}

			std::map<uint16_t, int> timeoutaccountindexs;
			if (!GetTimeoutIndexs(pblock, lastSnapshot, timeoutaccountindexs))
			{
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 获取超时记录失败\n");
				return false;
			}

			//当前超时记录列表刷新
			//uint16_t curtimeoutindex;
			std::map<uint16_t, int>::iterator timeoutIt;
			for (timeoutIt = timeoutaccountindexs.begin(); timeoutIt != timeoutaccountindexs.end(); timeoutIt++)
			{
				//当前公钥如果不在处罚缓存或者处罚记录中，则将其记录为新增超时记录
				//uint16_t 
				uint16_t curtimeoutindex = timeoutIt->first;
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 计算得到的超时公钥HASH索引=%d\n", curtimeoutindex);
				printsets(newsnapshot.cachedTimeoutPunishToRun);
				printsets(newsnapshot.curTimeoutPunishList);
				if (newsnapshot.cachedTimeoutPunishToRun.count(curtimeoutindex) ||
					newsnapshot.curTimeoutPunishList.count(curtimeoutindex))
				{
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 超时处罚记录或新增超时处罚记录中包括该索引，不新增超时记录\n");
					continue;
				}
				if (lastSnapshot.curBanList.count(curtimeoutindex) ||
					lastSnapshot.cachedBanList.count(curtimeoutindex))
				{
					newsnapshot.curTimeoutIndexRecord.erase(curtimeoutindex);
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 严重处罚记录中包括该索引，去掉该节点的超时记录\n");
					continue;
				}

				std::vector<SnapshotClass> cachedSnapshots;
				uint32_t cachedHeight = lastSnapshot.blockHeight - CACHED_BLOCK_COUNT;
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 准备获取块高度在%d-%d之间的缓存\n", cachedHeight, lastSnapshot.blockHeight);
				if (!GetSnapshotsByHeight(cachedSnapshots, cachedHeight))
				{
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 获取缓存的快照列表失败\n");
					return false;
				}
				std::vector<SnapshotClass>::iterator searchIt;
				bool founded = false;
				for (searchIt = cachedSnapshots.begin(); searchIt != cachedSnapshots.end(); searchIt++)
				{
					printsets((*searchIt).curTimeoutPunishList);
					if ((*searchIt).curTimeoutPunishList.count(curtimeoutindex))
					{
						LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 缓存的新增超时处罚记录中包括该索引，不新增超时记录\n");
						founded = true;
						break;
					}
				}
				if (founded)
					continue;

				int count = timeoutIt->second;
				if (newsnapshot.curTimeoutIndexRecord.count(curtimeoutindex))
				{
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 当前超时记录中存在该索引，计数累加\n");
					count = newsnapshot.curTimeoutIndexRecord[curtimeoutindex] + count;
					newsnapshot.curTimeoutIndexRecord[curtimeoutindex] = count;
				}
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 新增索引%d的超时记录\n", curtimeoutindex);
				newsnapshot.curTimeoutIndexRecord[curtimeoutindex] = count;

				//扣减当前公钥的信用值
				/*CConsensusAccount curAccount = candidatelist[curtimeoutindex];
				int64_t curCredit = curAccount.getCredit();
				curCredit -= TIMEOUT_CREDIT_PUNISH * timeoutIt->second;
				curAccount.backupCredit(); //一旦被加入惩罚列表，记录会清空，每次扣减都需要备份一下，以便pop的时候回滚
				curAccount.setCredit(curCredit);
				candidatelist[curtimeoutindex] = curAccount;
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 扣减索引%d信用值到%d\n", curtimeoutindex, curCredit);
			*/
			}

			//根据超时记录列表，一旦有N个连续不打块的记录，执行超时惩罚
			for (timeoutIt = newsnapshot.curTimeoutIndexRecord.begin();
				timeoutIt != newsnapshot.curTimeoutIndexRecord.end();
				timeoutIt++)
			{
				//当前公钥如果在信任列表中，则不罚出
				bool founded = false;
				for (std::vector<std::string>::iterator it = trustPKHashList.begin(); it != trustPKHashList.end(); it++)
				{
					uint160 believedPubkey;
					believedPubkey.SetHex(*it);
					if (candidatelist.at(timeoutIt->first).getPubicKey160hash() == believedPubkey)
					{
						LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 当前公钥受信任，不被超时处罚！\n");
						founded = true;
						break;
					}
				}
				if (founded)
					continue;

				if (timeoutIt->second >= MAX_TIMEOUT_COUNT)
				{
					uint16_t pkhashindex = timeoutIt->first;

					std::cout << "PKHASH " << pkhashindex << " TIMEOUT exit campaign" << std::endl;
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 公钥HASH索引%d超时，记入超时惩罚列表\n", pkhashindex);

					newsnapshot.curTimeoutPunishList.insert(pkhashindex);
					newsnapshot.curTimeoutIndexRecord.erase(pkhashindex);
					if (newsnapshot.curCandidateIndexList.count(pkhashindex))
					{
						newsnapshot.curCandidateIndexList.erase(pkhashindex);
					}
				}
			}

		}

		//计算当前打块顺序索引并塞进快照列表
		std::list<std::shared_ptr<CConsensusAccount >> consensusList;
		consensusList.clear();
		std::set<uint16_t>::iterator candidateit;
		for (candidateit = cachedsnapshot.curCandidateIndexList.begin(); candidateit != cachedsnapshot.curCandidateIndexList.end(); candidateit++)
		{
			std::shared_ptr<CConsensusAccount> tmpaccount = std::make_shared<CConsensusAccount>(candidatelist.at(*candidateit));
			consensusList.push_back(tmpaccount);
		}
		bool result = CDpocMining::Instance().GetMeetingList(pblock->nPeriodStartTime, consensusList);
		if (result)
		{
			std::list<std::shared_ptr<CConsensusAccount >>::iterator consensusListIt;
			newsnapshot.cachedMeetingAccounts.clear();
			for (consensusListIt = consensusList.begin(); consensusListIt != consensusList.end(); consensusListIt++)
			{
				uint16_t tmpIndex = -1;
				uint160 tmppkhash = (*consensusListIt)->getPubicKey160hash();
				ContainPK(tmppkhash, tmpIndex);
				newsnapshot.cachedMeetingAccounts.push_back(std::make_pair(tmpIndex, (*consensusListIt)->getCredit()));
			}
		}

	}

	//分析交易
	for (const auto& tx : pblock->vtx)
	{
		if (tx->IsCoinBase())
		{
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 处理Coinbase交易 %s\n", tx->GetHash().GetHex().c_str());
			//读CoinBase，从缓存列表中去掉已经执行的退款交易
			for (const CTxOut& txout : tx->vout)
			{
				uint16_t pkindex;
				if (txout.GetCampaignType() == TYPE_CONSENSUS_RETURN_DEPOSI)
				{
					if (ContainPK(txout.devoteLabel.hash, pkindex))
					{
						LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 处理退款交易\n");
						if (newsnapshot.cachedIndexsToRefund.count(pkindex))
						{
							newsnapshot.cachedIndexsToRefund.erase(pkindex);
													
							//从超时列表中删除已退款账户记录
							newsnapshot.curTimeoutIndexRecord.erase(pkindex);

							//如果是自己在记账的账户(且当前不是在分析本地块），则删除之
							std::string localpkhashhexstring;
							std::cout << "should remove " << txout.devoteLabel.hash.GetHex() << std::endl;
							if (CDpocInfo::Instance().GetLocalAccount(localpkhashhexstring)) {

								uint160 accountmyself;
								accountmyself.SetHex(localpkhashhexstring);
								if (accountmyself == txout.devoteLabel.hash &&
									analysisfinished) {
									std::cout << "Remove local account " << localpkhashhexstring << std::endl;
									LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 删除本地记帐账户 %s\n", localpkhashhexstring.c_str());
									
									//add begin by li at 2017/10/23 14:30
									std::string strStatus;
									std::string strPunishHash;
									if (CDpocInfo::Instance().GetConsensusStatus(strStatus, strPunishHash))
									{
										int nStatus = atoi(strStatus.c_str());
										nStatus += 3;
									
										std::stringstream ss;
										ss << nStatus;
										std::string strStatusNew = ss.str();

										std::string strHash(txout.devoteLabel.hash.ToString());
										SetConsensusStatus(strStatusNew, strHash);
									}
									//add end by li at 2017/10/23 14:30
									
									CDpocInfo::Instance().RemoveInfo();
									CDpocInfo::Instance().setJoinCampaign(false);
								}
							}
							continue;
						}
					}
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 退款公钥不在缓存列表中\n");
				}
				//根据惩罚状态更新惩罚列表
				else if (txout.GetCampaignType() == TYPE_CONSENSUS_ORDINARY_PUSNISHMENT)
				{
					if (ContainPK(txout.devoteLabel.hash, pkindex))
					{
						//add begin by li at 2017/10/28 19:00
						std::map<uint16_t, int> timeoutaccountindexs;
						if (!GetTimeoutIndexs(pblock, lastSnapshot, timeoutaccountindexs))
						{
							LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 获取超时记录失败\n");
							return false;
						}

						//当前超时记录列表刷新
						//uint16_t curtimeoutindex;
						std::map<uint16_t, int>::iterator timeoutIt = timeoutaccountindexs.begin();
						for (; timeoutIt != timeoutaccountindexs.end(); timeoutIt++)
						{
							uint16_t curtimeoutindex = timeoutIt->first;
							CConsensusAccount &curAccount = candidatelist[curtimeoutindex];
							int64_t curCredit = curAccount.getCredit();
							curCredit -= TIMEOUT_CREDIT_PUNISH;
							//一旦被加入惩罚列表，记录会清空，每次扣减都需要备份一下，以便pop的时候回滚
							curAccount.backupCredit();
							curAccount.setCredit(curCredit);
							candidatelist[curtimeoutindex] = curAccount;

							//candidatelist[curtimeoutindex].setHeight(lastSnapshot.blockHeight + 1);
							LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 扣减索引%d信用值到%d\n", curtimeoutindex, curCredit);
						}
						//add end by li at 2017/10/28 19:00

						//将已经执行的超时处罚从超时记录中清除
						LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 准备删除超时记录\n");
						std::vector<SnapshotClass> vecforprint;
						vecforprint.push_back(newsnapshot);
						printsnapshots(vecforprint);	 
						if (newsnapshot.cachedTimeoutPunishToRun.count(pkindex))
						{
							LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 超时惩罚执行完毕，从惩罚列表中去掉索引%d\n", pkindex);
							newsnapshot.cachedTimeoutPunishToRun.erase(pkindex);
						}
						//if (newsnapshot.curTimeoutIndexRecord.count(pkindex))
						//{
						//	LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 超时惩罚执行完毕，从超时记录列表中去掉索引%d\n", pkindex);
						//	newsnapshot.curTimeoutIndexRecord.erase(pkindex);
						//}
						
						////将该公钥信用值置为0
						//CConsensusAccount tmpaccount = candidatelist[pkindex];
						//tmpaccount.setCredit(0);
						//candidatelist[pkindex] = tmpaccount;

						

						//add begin by li at 2017/10/23 14:30
						//设置普通处罚的值
						std::string strStatus("1");
						std::string strHash(txout.devoteLabel.hash.ToString());
						SetConsensusStatus(strStatus, strHash);
						//add end by li at 2017/10/23 14:30

						//给该公钥退款
						newsnapshot.curRefundIndexList.insert(pkindex);
						LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 执行超时退款给索引%d\n", pkindex);

						continue;
					}
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 超时惩罚公钥不在缓存列表中，情况异常\n");
				}
				/*else if (txout.GetCampaignType() == TYPE_CONSENSUS_SEVERE_PUNISHMENT)
				{
					if (ContainPK(txout.devoteLabel.hash, pkindex))
					{
						//将该公钥存入当前缓存的黑名单
						if (!newsnapshot.cachedBanList.count(pkindex))
						{
							LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 执行严重惩罚，更新黑名单\n");
							newsnapshot.cachedBanList.insert(pkindex);

							//将该公钥信用值置为0
							CConsensusAccount tmpaccount = candidatelist[pkindex];
							tmpaccount.backupCredit();
							tmpaccount.setCredit(0);
							candidatelist[pkindex] = tmpaccount;
							LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 严重惩罚，公钥索引%d的信用值到%d\n", pkindex, candidatelist[pkindex].getCredit());
						}

						//add begin by li at 2017/10/23 14:30
						//设置严重处罚的值
						std::string strStatus("2");
						std::string strHash(txout.devoteLabel.hash.ToString());
						SetConsensusStatus(strStatus, strHash);
						//add end by li at 2017/10/23 14:30

						//给该公钥退款
						newsnapshot.curRefundIndexList.insert(pkindex);
						LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 执行严重惩罚退款\n");
	
					}
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 严重惩罚公钥不在缓存列表中，情况异常\n");
				}*/
			}

			continue;
		}

		if (tx->GetTxType() != 1)
			continue;

		if (tx->GetCampaignType() == TYPE_CONSENSUS_REGISTER)
		{
			std::cout << "join campaign" << std::endl;
			//申请加入交易，获取公钥HASH值
			uint160 curHash = tx->GetCampaignPublickey();
			//寻找下标ID
			uint16_t pkhashindex;
			CAmount ipcvalue = tx->GetRegisterIPC();
			if (!ContainPK(curHash, pkhashindex))
			{
				pkhashindex = candidatelist.size();
				candidatelist.push_back(CConsensusAccount(curHash, ipcvalue,tx->GetHash()));

				//设置共识账号列表中共识账号加入的高度
				//candidatelist[curHash].setHeight(lastSnapshot.blockHeight + 1);
			}
			else
			{
				//虽然该公钥已经加入过了，但是其押金需要更新
// 				CConsensusAccount tmpaccount = candidatelist[pkhashindex];
// 				tmpaccount.setJoinIPC(ipcvalue);
// 				tmpaccount.setTxhash(tx->GetHash()); //增加txhash 更新
// 				candidatelist[pkhashindex] = tmpaccount;
				candidatelist[pkhashindex].setJoinIPC(ipcvalue);
				candidatelist[pkhashindex].setTxhash(tx->GetHash());
				//candidatelist[pkhashindex].setHeight(lastSnapshot.blockHeight + 1);
				LogPrintf("[更新candidatelist] 当前加入的公钥%d的押金=%d, 当前申请公钥信用值为%d,txhash ：%s\n", pkhashindex, candidatelist[pkhashindex].getJoinIPC(), candidatelist[pkhashindex].getCredit(), candidatelist[pkhashindex].getTxhash().ToString());
			}
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 当前加入的公钥%d的押金=%d, 当前申请公钥信用值为%d,txhash ：%s\n", pkhashindex, candidatelist[pkhashindex].getJoinIPC(), candidatelist[pkhashindex].getCredit(),
				candidatelist[pkhashindex].getTxhash().ToString());
			std::cout << "当前加入的的押金=" << ipcvalue;
			
			//根据动态调整的押金值和当前公钥信用值计算得到当前公钥的押金门限值
			CAmount curDeposiThreshold = GetCurDepositAdjust(curHash, blockHeight);
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 当前动态调整后押金阈值为%d\n", curDeposiThreshold);
			
			if (ipcvalue >= curDeposiThreshold)
			{
				if (!newsnapshot.curCandidateIndexList.count(pkhashindex))
				{
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 押金满足额度要求，加入记账\n");
					std::cout << "押金满足额度要求，加入申请成功通过，公钥已加入候选人列表" << std::endl;
					newsnapshot.curCandidateIndexList.insert(pkhashindex);
				}
			}
			else
			{
				if (!newsnapshot.cachedIndexsToRefund.count(pkhashindex))
				{
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 押金不满足额度要求，退款\n");
					std::cout << "押金不满足额度要求，执行退款" << std::endl;
					newsnapshot.cachedIndexsToRefund.insert(pkhashindex);
				}
			}
			
		}
		else if (tx->GetCampaignType() == TYPE_CONSENSUS_QUITE)
		{
			std::cout << "exit campaign" << std::endl;
			//申请退出交易，获取公钥HASH值
			uint160 curHash = tx->GetCampaignPublickey();
			//寻找下标ID
			uint16_t pkhashindex;
			if (!ContainPK(curHash, pkhashindex))
			{
				//pkhashindex = accountlist.size();
				//accountlist.push_back(CConsensusAccount(curHash, tx->GetRegisterIPC());
				std::cout << "Warning! unknown pubkeyhash want to exit campaigntype!" << std::endl;
			}

			std::cout << "pkhash=" << curHash.GetHex() << ", index=" << pkhashindex << std::endl;

			if (newsnapshot.curCandidateIndexList.count(pkhashindex))
			{
				newsnapshot.curCandidateIndexList.erase(pkhashindex);
				newsnapshot.curRefundIndexList.insert(pkhashindex);
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 执行正常退出退款给索引%d\n", pkhashindex);
			}

			//add begin by li at 2017/11/20 
			//正常退出，也要扣减信用值
			//uint16_t curtimeoutindex = timeoutIt->first;
			/*CConsensusAccount curAccount = candidatelist[pkhashindex];
			int64_t curCredit = curAccount.getCredit();
			curCredit -= TIMEOUT_CREDIT_PUNISH;
			curAccount.backupCredit();
			curAccount.setCredit(curCredit);
			candidatelist[pkhashindex] = curAccount;
			//candidatelist[pkhashindex].setHeight(lastSnapshot.blockHeight + 1);
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 扣减索引%d信用值到%d\n", pkhashindex, curCredit);	*/
			//add end by li at 2017/11/20 
		}
		/*else if (tx->GetCampaignType() == TYPE_CONSENSUS_SEVERE_PUNISHMENT_REQUEST)
		{
			//执行严重处罚，获取公钥HASH值
			uint160 curHash = tx->GetCampaignPublickey();
			//寻找下标ID
			uint16_t pkhashindex = candidatelist.size();
			if (!ContainPK(curHash, pkhashindex))
			{
				LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 对不在缓存列表中的公钥%s执行严重处罚，将公钥塞入缓存列表\n", curHash.GetHex().c_str());
				CAmount value = 0;
				CConsensusAccount tmpAccount(curHash, value);
				candidatelist[pkhashindex] = tmpAccount;
			}
			std::cout << "pkhash=" << curHash.GetHex() << ", index=" << pkhashindex << std::endl;

			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] 向黑名单中添加索引%d\n", pkhashindex);
			newsnapshot.curBanList.insert(pkhashindex);

			//如果当前超时记录中有该索引对应的记录，则删除该记录
			if (newsnapshot.curTimeoutIndexRecord.count(pkhashindex))
			{
				newsnapshot.curTimeoutIndexRecord.erase(pkhashindex);
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 严重处罚记录中包括该索引，去掉该节点的超时记录\n");
				continue;			
			}

			//如果当前会议列表中有该索引对应的记录，则删除
			if (newsnapshot.curCandidateIndexList.count(pkhashindex))
			{
				newsnapshot.curCandidateIndexList.erase(pkhashindex);
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 当前候选人列表中包括该索引，去掉该节点的候选人资格\n");
			}
		}*/
	}

	////To Do:压缩：如果新快照与上一个一样，则将新快照指向前置快照，同时记得将所有引用快照改为指针指向的参数
	//if (lastSnapshot == newsnapshot)
	//{
	//	newsnapshot.changed = false;
	//	newsnapshot.referSnapshot = lastSnapshot.referSnapshot;
	//	newsnapshot.compressclear();
	//}

	if (!PushSnapshot(newsnapshot))
	{
		LogPrintf("[] 向列表中增加快照失败！\n");
		return false;
	}

	uint32_t cachedHeight = (blockHeight > CACHED_BLOCK_COUNT) ? blockHeight - CACHED_BLOCK_COUNT : 0;
	std::cout << "list from " << cachedHeight << " to " << blockHeight << " snapshots" << std::endl;
	std::vector<SnapshotClass> listforprint;
	if (!GetSnapshotsByHeight(listforprint, cachedHeight, blockHeight))
	{
		LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 获取高度%d-%d的缓存快照以打印结果失败\n", blockHeight - CACHED_BLOCK_COUNT, blockHeight);
	}
	//listforprint.clear();
	//{
	//	LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 请求只读锁\n");
	//	readLock rdlock(rwmutex);
	//	LogPrintf("[CConsensusAccountPool::pushDPOCBlock] 进入锁\n");
	//	for (int i = (blockHeight > CACHED_BLOCK_COUNT) ? blockHeight - CACHED_BLOCK_COUNT : 0; i <= blockHeight; i++)
	//	{
	//		listforprint.push_back(snapshotlist[i]);
	//	}
	//}

	printsnapshots(listforprint);

	//writeCandidatelistToFile();
	//std::cout << "snapshotlist size=" << snapshotlist.size() << std::endl << std::endl;
	std::cout << "pushDPOCBlock(): success exit" << std::endl << std::endl;
	return true;
}

bool CConsensusAccountPool::rollbackCandidatelist(uint32_t nHeight)
{
	LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 请求读写锁\n");
	writeLock wtlock(rwmutex);
	LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 进入锁\n");
	std::map<uint32_t, SnapshotClass>::iterator iterList = snapshotlist.end();
	while (iterList != snapshotlist.begin())
	{
		iterList--;
		if (nHeight < iterList->first)
		{
			uint16_t tmpIndex;
			std::map<uint32_t, SnapshotClass>::iterator iterLast = iterList;
			if (iterLast != snapshotlist.begin())
			{
				iterLast--;
			}

			//从上一个快照到当前待删除快照，如果执行了黑名单操作，需要回滚信用值
			for (std::set<uint16_t>::iterator banit = iterLast->second.curBanList.begin();
				banit != iterLast->second.curBanList.end(); banit++)
			{
				tmpIndex = *banit;
				candidatelist[tmpIndex].revertCredit();
				LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
				//直接设置当前节点的信用值到前一个快照中的信用值
				int64_t tmpCredit;
				if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
				{
					candidatelist[tmpIndex].setCredit(tmpCredit);
					LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 直接从快照回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
				}
			}

			//对于当前待删除快照，其打块人公钥增加的信用值需要回滚
			tmpIndex = iterList->second.pkHashIndex;
			//直接设置当前节点的信用值到前一个快照中的信用值
			int64_t tmpCredit;
			if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
			{
				candidatelist[tmpIndex].setCredit(tmpCredit);
				LogPrintf("[CConsensusAccountPool::popDPOCBlock] 直接从快照回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
			}

			//超时记录中扣减的信用值需要回滚
			std::vector<SnapshotClass> listforprint;
			listforprint.push_back(iterLast->second);
			listforprint.push_back(iterList->second);
			printsnapshots(listforprint);
			for (std::map<uint16_t, int>::iterator timeoutit = iterList->second.curTimeoutIndexRecord.begin();
				timeoutit != iterList->second.curTimeoutIndexRecord.end(); timeoutit++)
			{
				LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 当前待删除快照的超时记录中包括索引%d\n", timeoutit->first);
				//如果当前快照超时记录中有存在于上一个快照超时记录中的项，则回滚超时次数*超时惩罚
				if (iterLast->second.curTimeoutIndexRecord.count(timeoutit->first))
				{
					LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 上一个快照的超时记录中包括索引%d\n", timeoutit->first);
					LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 上一个快照的超时记录索引%d的次数%d，待删除快照中的次数%d\n",
						timeoutit->first, iterLast->second.curTimeoutIndexRecord[timeoutit->first], timeoutit->second);

					if (iterLast->second.curTimeoutIndexRecord[timeoutit->first] < timeoutit->second)
					{
						tmpIndex = timeoutit->first;
						//直接设置当前节点的信用值到前一个快照中的信用值
						int64_t tmpCredit;
						if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
						{
							candidatelist[tmpIndex].setCredit(tmpCredit);
							LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 直接从快照回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
						}
					}
				}
				//如果当前快照超时记录不存在于上一个快照中，也需要回滚超时次数*超时惩罚
				else
				{
					tmpIndex = timeoutit->first;
					int64_t tmpCredit;
					if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
					{
						candidatelist[tmpIndex].setCredit(tmpCredit);
						LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 直接从快照回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
					}
				}
			}

			//如果当前快照有超时惩罚，而上一个快照中没有这个超时惩罚，则需要revert
			for (std::set<uint16_t>::iterator punishit = iterList->second.curTimeoutPunishList.begin();
				punishit != iterList->second.curTimeoutPunishList.end();
				punishit++)
			{
				tmpIndex = (*punishit);
				LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 待删除快照的超时惩罚列表中包括索引%d\n", tmpIndex);
				if (!iterLast->second.curTimeoutPunishList.count(tmpIndex) &&
					iterLast->second.curTimeoutIndexRecord.count(tmpIndex))
				{						
					//直接设置当前节点的信用值到前一个快照中的信用值
					int64_t tmpCredit;
					if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
					{
						candidatelist[tmpIndex].setCredit(tmpCredit);
						LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 直接从快照回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
					}
				}
			}

			LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 弹出高度为%d的快照\n", iterLast->first);
		}
		else
		{
			break;
		}
	}
}


bool CConsensusAccountPool::popDPOCBlock( uint32_t blockHeight)
{
	LogPrintf("[CConsensusAccountPool::popDPOCBlock] 请求读写锁\n");
	writeLock wtlock(rwmutex);
	LogPrintf("[CConsensusAccountPool::popDPOCBlock] 进入锁\n");
	std::map<uint32_t, SnapshotClass>::iterator iterList = snapshotlist.end();
	while(iterList != snapshotlist.begin())
	{
		iterList--;
		if (blockHeight < iterList->first)
		{
			uint16_t tmpIndex;
			std::map<uint32_t, SnapshotClass>::iterator iterLast = iterList;
			if (iterLast != snapshotlist.begin())
			{
				iterLast--;
			}

			//从上一个快照到当前待删除快照，如果执行了黑名单操作，需要回滚信用值
			for (std::set<uint16_t>::iterator banit = iterLast->second.curBanList.begin();
				banit != iterLast->second.curBanList.end(); banit++)
			{
				tmpIndex = *banit;
				candidatelist[tmpIndex].revertCredit();
				LogPrintf("[CConsensusAccountPool::popDPOCBlock] 回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
				//直接设置当前节点的信用值到前一个快照中的信用值
				int64_t tmpCredit;
				if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
				{
					candidatelist[tmpIndex].setCredit(tmpCredit);
					LogPrintf("[CConsensusAccountPool::popDPOCBlock] 直接从快照回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
				}
			}

			//对于当前待删除快照，其打块人公钥增加的信用值需要回滚
			tmpIndex = iterList->second.pkHashIndex;
			//candidatelist[tmpIndex].setCredit(candidatelist[tmpIndex].getCredit() - PACKAGE_CREDIT_REWARD);
			//LogPrintf("[CConsensusAccountPool::popDPOCBlock] 回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
			//直接设置当前节点的信用值到前一个快照中的信用值
			int64_t tmpCredit;
			if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
			{
				candidatelist[tmpIndex].setCredit(tmpCredit);
				LogPrintf("[CConsensusAccountPool::popDPOCBlock] 直接从快照回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
			}

			//超时记录中扣减的信用值需要回滚
			std::vector<SnapshotClass> listforprint;
			listforprint.push_back(iterLast->second);
			listforprint.push_back(iterList->second);
			printsnapshots(listforprint);
			for (std::map<uint16_t, int>::iterator timeoutit = iterList->second.curTimeoutIndexRecord.begin();
				timeoutit != iterList->second.curTimeoutIndexRecord.end(); timeoutit++)
			{
				LogPrintf("[CConsensusAccountPool::popDPOCBlock] 当前待删除快照的超时记录中包括索引%d\n", timeoutit->first);
				//如果当前快照超时记录中有存在于上一个快照超时记录中的项，则回滚超时次数*超时惩罚
				if (iterLast->second.curTimeoutIndexRecord.count(timeoutit->first))
				{
					LogPrintf("[CConsensusAccountPool::popDPOCBlock] 上一个快照的超时记录中包括索引%d\n", timeoutit->first);
					LogPrintf("[CConsensusAccountPool::popDPOCBlock] 上一个快照的超时记录索引%d的次数%d，待删除快照中的次数%d\n",
						timeoutit->first, iterLast->second.curTimeoutIndexRecord[timeoutit->first], timeoutit->second);
				
					if (iterLast->second.curTimeoutIndexRecord[timeoutit->first] < timeoutit->second)
					{
						tmpIndex = timeoutit->first;
						//CConsensusAccount tmpAccount = candidatelist[timeoutit->first];
						//int64_t curCredit = tmpAccount.getCredit();
						//curCredit += (iterLast->second.curTimeoutIndexRecord[timeoutit->first] - timeoutit->second) * TIMEOUT_CREDIT_PUNISH;
						//tmpAccount.setCredit(curCredit);
						//candidatelist[timeoutit->first] = tmpAccount;
						//LogPrintf("[CConsensusAccountPool::popDPOCBlock] 回滚公钥索引%d的信用值到%d\n", timeoutit->first, candidatelist[timeoutit->first].getCredit());
						//直接设置当前节点的信用值到前一个快照中的信用值
						int64_t tmpCredit;
						if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
						{
							candidatelist[tmpIndex].setCredit(tmpCredit);
							LogPrintf("[CConsensusAccountPool::popDPOCBlock] 直接从快照回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
						}
					}
				}
				//如果当前快照超时记录不存在于上一个快照中，也需要回滚超时次数*超时惩罚
				else
				{
					tmpIndex = timeoutit->first;
					//CConsensusAccount tmpAccount = candidatelist[timeoutit->first];
					//int64_t curCredit = tmpAccount.getCredit();
					//curCredit += timeoutit->second * TIMEOUT_CREDIT_PUNISH;
					//tmpAccount.setCredit(curCredit);
					//candidatelist[timeoutit->first] = tmpAccount;
					//LogPrintf("[CConsensusAccountPool::popDPOCBlock] 回滚公钥索引%d的信用值到%d\n", timeoutit->first, candidatelist[timeoutit->first].getCredit());						//直接设置当前节点的信用值到前一个快照中的信用值
					int64_t tmpCredit;
					if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
					{
						candidatelist[tmpIndex].setCredit(tmpCredit);
						LogPrintf("[CConsensusAccountPool::popDPOCBlock] 直接从快照回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
					}
				}
			}

			//如果当前快照有超时惩罚，而上一个快照中没有这个超时惩罚，则需要revert
			for (std::set<uint16_t>::iterator punishit = iterList->second.curTimeoutPunishList.begin();
				punishit != iterList->second.curTimeoutPunishList.end();
				punishit++)
			{
				tmpIndex = (*punishit);
				LogPrintf("[CConsensusAccountPool::popDPOCBlock] 待删除快照的超时惩罚列表中包括索引%d\n", tmpIndex);
				if (!iterLast->second.curTimeoutPunishList.count(tmpIndex) &&
					iterLast->second.curTimeoutIndexRecord.count(tmpIndex))
				{
					//candidatelist[tmpIndex].revertCredit();
					//LogPrintf("[CConsensusAccountPool::popDPOCBlock] 回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());						//直接设置当前节点的信用值到前一个快照中的信用值
					int64_t tmpCredit;
					if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
					{
						candidatelist[tmpIndex].setCredit(tmpCredit);
						LogPrintf("[CConsensusAccountPool::popDPOCBlock] 直接从快照回滚公钥索引%d的信用值到%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
					}
				}
			}

			LogPrintf("[CConsensusAccountPool::popDPOCBlock] 弹出高度为%d的快照\n", iterLast->first);

			//add begin by li 

			//截断候选人列表
			
			/*if (blockHeight < candidatelist.back().getHeight())
			{
				candidatelist.pop_back();
			}*/
			writeCandidatelistToFile();
			//将修改后文件冲入内存
			//flushSnapshotToDisk();
			uint32_t u32Height = iterList->first;

			boost::filesystem::path pathTmpIndex = GetDataDir() / m_strSnapshotIndexPath;
			uint64_t u64Num = 0;
			if (boost::filesystem::exists(pathTmpIndex))
			{
				uint64_t u64FileSizeIndex = boost::filesystem::file_size(pathTmpIndex);
				u64Num = u64FileSizeIndex / m_u64SnapshotIndexSize;
			}

			if (u32Height <= u64Num)
			{
				//截断SnapshotIndex文件
				//boost::filesystem::path pathTmpIndex = GetDataDir() / m_strSnapshotIndexPath;
				uint64_t fileSizeIndex = 0;
				if (boost::filesystem::exists(pathTmpIndex))
				{
					fileSizeIndex = boost::filesystem::file_size(pathTmpIndex);
					fileSizeIndex = fileSizeIndex - m_u64SnapshotIndexSize;
				
					FILE *fileIndex = fopen(pathTmpIndex.string().c_str(), "ab+");
					if (fileIndex)
					{
						TruncateFile(fileIndex, fileSizeIndex);
					}
					fclose(fileIndex);
				}

				//截断Snapshot文件
				boost::filesystem::path pathTmp = GetDataDir() / m_strSnapshotPath;
				uint64_t fileSize = 0;
				if (boost::filesystem::exists(pathTmp))
				{
					fileSize = boost::filesystem::file_size(pathTmp);
					fileSize = m_mapSnapshotIndex[iterList->first];

					FILE *file = fopen(pathTmp.string().c_str(), "ab+");
					if (file)
					{
						TruncateFile(file, fileSize);
					}
					fclose(file);
				}

				//将修改后文件冲入内存
				flushSnapshotToDisk();
			}
			
			m_mapSnapshotIndex.erase(iterList->first);
			//add end by li

			iterList = snapshotlist.erase(iterList);
		}
		else
		{
			break;
		}	
	}

	//writeCandidatelistToFile();
	//将修改后文件冲入内存
	//flushSnapshotToDisk();
	
	LogPrintf("[CConsensusAccountPool::popDPOCBlock] end. \n");
	
	return true;
}


bool CConsensusAccountPool::AddDPOCCoinbaseToBlock(CBlock* pblockNew, CBlockIndex* pindexPrev, uint32_t blockHeight, CMutableTransaction &coinbaseTx)
{
	SnapshotClass cursnapshot;
	if (!GetLastSnapshot(cursnapshot))
	{
		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] 无法获取当前最新的快照，也许因为快照列表为空，返回正确\n");
		return true;
	}

	//构造并加入全部当前需要退款的txout
	std::set<uint16_t>::iterator refundit;
	for (refundit = cursnapshot.cachedIndexsToRefund.begin(); refundit!= cursnapshot.cachedIndexsToRefund.end(); refundit++)
	{
		//if (cursnapshot.cachedMeetingAccounts.count(*refundit))
		if(contain(cursnapshot.cachedMeetingAccounts, *refundit))
		{
			std::cout << "Relay Account" << *refundit << "'s Refund" << std::endl;
			LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] 延迟对公钥押金%的退款\n", *refundit);
			continue;
		}
		if (cursnapshot.curBanList.count(*refundit) || cursnapshot.cachedBanList.count(*refundit))
		{
			LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] 退款账号在黑名单中！\n");
			//LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] 退款账号在黑名单中！拒绝退款，押金清零！\n");
			//CAmount zero = 0;
			//CConsensusAccount curaccount(candidatelist[*refundit].getPubicKey160hash(), zero);
			//candidatelist[*refundit] = curaccount;
			//continue;
		}

		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] 当前退款的公钥%d的押金=%d\n", *refundit, candidatelist[*refundit].getJoinIPC());
		CAmount zero = 0;
		//CTxOut curRefund(candidatelist[*refundit].getJoinIPC(), candidatelist[*refundit].getPubicKey160hash());
		CTxOut curRefund(zero, candidatelist[*refundit].getPubicKey160hash());
		coinbaseTx.vout.emplace_back(std::move(curRefund));

		//退款后将押金数目清零，以防超时再次执行退款
		uint32_t nHeight = candidatelist[*refundit].getHeight();
		CConsensusAccount curaccount(candidatelist[*refundit].getPubicKey160hash(), zero);	
		candidatelist[*refundit] = curaccount;
		//candidatelist[*refundit].setHeight(nHeight);

		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] 退款后的押金=%d\n", candidatelist[*refundit].getJoinIPC());
	}

	//执行普通处罚
	for (refundit = cursnapshot.cachedTimeoutPunishToRun.begin(); refundit != cursnapshot.cachedTimeoutPunishToRun.end(); refundit++)
	{
		//对于还在本轮记账的账户，延迟其惩罚操作到下一轮再处理，因为如果本轮还没有轮到它打块就处罚了的话，会造成打块失败
		//if (cursnapshot.cachedMeetingAccounts.count(*refundit))
		if (contain(cursnapshot.cachedMeetingAccounts, *refundit))
		{
			std::cout << "Relay Account" << *refundit << "'s Refund" << std::endl;
			LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] 延迟对公钥索引%d的退款\n", *refundit);
			continue;
		}
		std::cout << "Add Refund to Coinbase tx" << std::endl;

		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] 当前普通处罚的公钥%d的押金=%d", *refundit, candidatelist[*refundit].getJoinIPC());
		CTxOut curTimeoutPunish(TYPE_CONSENSUS_ORDINARY_PUSNISHMENT, candidatelist[*refundit].getPubicKey160hash());
		coinbaseTx.vout.emplace_back(std::move(curTimeoutPunish));
		//CTxOut curRefund(candidatelist[*refundit].getJoinIPC(), candidatelist[*refundit].getPubicKey160hash());
		//coinbaseTx.vout.emplace_back(std::move(curRefund));
	}

	//执行严重处罚
	for (refundit = cursnapshot.curBanList.begin(); refundit != cursnapshot.curBanList.end(); refundit++)
	{
		//严重处罚，立刻执行		
		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] 当前严重处罚的公钥%d的押金=%d", *refundit, candidatelist[*refundit].getJoinIPC());
		CTxOut curBanTx(TYPE_CONSENSUS_SEVERE_PUNISHMENT, candidatelist[*refundit].getPubicKey160hash());
		coinbaseTx.vout.emplace_back(std::move(curBanTx));
	}


	//取本地账户签名
	std::string pubkeystring;
	if (!CDpocInfo::Instance().GetLocalAccount(pubkeystring))
	{
		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] ERROR! no local account\n");
		//std::cout << "AddDPOCCoinbaseToBlock(): ERROR! no local account!" << std::endl;
		return false;
	}
	uint160 curpk;
	curpk.SetHex(pubkeystring);
	if (!addSignToCoinBase(pblockNew, pindexPrev, blockHeight, coinbaseTx, &curpk)) {
		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock]  ERROR! add sign failed\n");
		//std::cout << "AddDPOCCoinbaseToBlock(): ERROR! add sign failed!" << std::endl;
		return false;
	}

	//std::cout << "AddDPOCCoinbaseToBlock(): success exit" << std::endl;
	LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] end by tre\n ");
	return true;
}

bool CConsensusAccountPool::addSignToCoinBase(CBlock* pblock, CBlockIndex* pindexPrev, uint32_t blockHeight, CMutableTransaction &coinbaseTx, uint160* pPubKeyHash)
{
	if (nullptr == pPubKeyHash || NULL == pPubKeyHash) {
		return false;
	}

	if (pPubKeyHash->IsNull()) {
		return false;
	}
	//  添加签名 
	if (pPubKeyHash) {
		CKeyID		publickeyID(*pPubKeyHash);
		CPubKey		vchPubKeyOut;
		if (pwalletMain->GetPubKey(publickeyID, vchPubKeyOut)) {
			//LogPrintf("[addRulersToCoinbase] public keyhash=%s", vchPubKeyOut.GetHash().ToString().c_str());
		}
		else {
			LogPrintf("[CConsensusAccountPool::addSignToCoinBase] don't find public key ");
			return false;
		}
		//获取私钥时，先解密下，确保能正确拿到私钥
		if (pwalletMain->IsLocked())
			pwalletMain->Unlock(pwalletMain->curstrWalletPassphrase);
		CKey vchPrivKeyOut;
		if (pwalletMain->GetKey(publickeyID, vchPrivKeyOut)) {
			LogPrintf("[CConsensusAccountPool::addSignToCoinBase] pwalletMain->GetKey() success! \n");
		}
		else {
			LogPrintf("[CConsensusAccountPool::addSignToCoinBase] isLocked = %d \n ",pwalletMain->IsLocked());
			LogPrintf("[CConsensusAccountPool::addSignToCoinBase] don't find private key");
			return false;
		}
		pwalletMain->Lock(); //重新加密
		//std::string str = "IPCChain key verification\n";
		uint256 hash;
		CAmount nfee = 0;
		for (int i=0; i<coinbaseTx.vout.size(); i++)
		{
			nfee += coinbaseTx.vout[i].nValue;
		}
		CHash256 hashoperator;

		hashoperator.Write((unsigned char*)&(pblock->nPeriodStartTime), sizeof(pblock->nPeriodStartTime));//起始时间，8Byte
		hashoperator.Write((unsigned char*)&(pblock->nPeriodCount), sizeof(pblock->nPeriodCount));//会议总人数，4Byte
		hashoperator.Write((unsigned char*)&(pblock->nTimePeriod), sizeof(pblock->nTimePeriod));//打块人序号，4Byte
		hashoperator.Write((unsigned char*)&(pblock->nTime), sizeof(pblock->nTime));//实际打块时间，4Byte
		hashoperator.Write((unsigned char*)&nfee, sizeof(nfee));//总费用，8Byte
		hashoperator.Write((unsigned char*)&blockHeight, sizeof(blockHeight));//总费用，8Byte
		hashoperator.Write((unsigned char*)pindexPrev->GetBlockHash().begin(), 32);//前置块hash， 256bit
		hashoperator.Finalize(hash.begin());

		std::vector<unsigned char> vchSig;
		vchPrivKeyOut.Sign(hash, vchSig);

		unsigned char dataBuff[10240];
		int datalen = 0;
		memcpy(dataBuff, (unsigned char*)&(pblock->nPeriodStartTime), sizeof(pblock->nPeriodStartTime));
		datalen += sizeof(pblock->nPeriodStartTime);
		memcpy(dataBuff + datalen, (unsigned char*)&(pblock->nPeriodCount), sizeof(pblock->nPeriodCount));
		datalen += sizeof(pblock->nPeriodCount);
		memcpy(dataBuff + datalen, (unsigned char*)&(pblock->nTimePeriod), sizeof(pblock->nTimePeriod));
		datalen += sizeof(pblock->nTimePeriod);
		memcpy(dataBuff + datalen, (unsigned char*)&(pblock->nTime), sizeof(pblock->nTime));
		datalen += sizeof(pblock->nTime);
		memcpy(dataBuff + datalen, (unsigned char*)&nfee, sizeof(nfee));
		datalen += sizeof(nfee);
		memcpy(dataBuff + datalen, (unsigned char*)&blockHeight, sizeof(blockHeight));//当前块高度，4Byte
		datalen += sizeof(blockHeight);
		memcpy(dataBuff + datalen, (unsigned char*)pindexPrev->GetBlockHash().begin(), 32);//256 bits
		datalen += 32;
		std::string datahex;
		for (int i = 0; i < datalen; i++)
		{
			char tmp[3];
			sprintf(tmp, "%02x", dataBuff[i]);
			datahex = datahex + std::string(tmp);
		}
		LogPrintf("[CConsensusAccountPool::addSignToCoinBase] 签名明文=%s , 明文HASH=%s , 公钥HASH=%s\n", datahex.c_str(), hash.GetHex().c_str(), pPubKeyHash->GetHex().c_str());

		// 生成signature的缓存
		std::vector<unsigned char> vchSigSend;
		vchSigSend.resize(2 + vchPubKeyOut.size() + vchSig.size());

		//缓存的签名
		unsigned char vchSigLen = (unsigned char)vchSig.size();
		vchSigSend[0] = vchSigLen;
		for (std::vector<unsigned char>::size_type ix = 0; ix < vchSig.size(); ++ix) {
			vchSigSend[ix + 1] = vchSig[ix];
		}

		//缓存public key
		vchSigSend[vchSigLen + 1] = (unsigned char)vchPubKeyOut.size();
		for (std::vector<unsigned char>::size_type ix = 0; ix < vchPubKeyOut.size(); ++ix) {
			vchSigSend[ix + 2 + vchSigLen] = vchPubKeyOut[ix];
		}

		//	std::cout << "vchSig.size=" << vchSig.size() << " vchPubKeyOut.size=" << vchPubKeyOut.size() << "  vchSigSend.size=" << vchSigSend.size() << std::endl;
		std::string strSin2Publickey = EncodeBase58(vchSigSend);
		//std::cout << "data=" << str << ", fee=" << nfee << ", hash=" << hash.GetHex() << std::endl;
		//std::cout << "sign=" << strSin2Publickey << std::endl;
		LogPrintf("[CConsensusAccountPool::addSignToCoinBase] 签名=%s\n", strSin2Publickey);
		coinbaseTx.vout[0].coinbaseScript = strSin2Publickey;
		coinbaseTx.vout[0].txType = 0;
	}
	return true;
}



bool CConsensusAccountPool::GetSnapshotByTime(SnapshotClass &snapshot, uint64_t readtime)
{
	LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 请求只读锁\n");
	readLock rdlock(rwmutex);
	LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 进入锁\n");

	if (snapshotlist.size()==0)
	{
		LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 快照列表为空，无法check\n");
		LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 退出锁\n");
		return false;
	}

	LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 传入欲读取的缓存时间= %d \n", readtime);
	std::cout << "AccountPool::GetSnapshotByTime() called: time=" << readtime << std::endl;

	//找到时间对应的块作为缓存块，给上面返值，并缓存候选人列表
	if (snapshotlist.size() > 0)
	{
		std::map<uint32_t, SnapshotClass>::iterator mapit = snapshotlist.end();
		mapit--;
		LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 最后一个快照的时间= %d \n", mapit->second.timestamp);
		std::cout << "AccountPool::GetSnapshotByTime() last snapshotime =" << mapit->second.timestamp << std::endl;
		while (mapit->second.timestamp > readtime)
		{
			//LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 找到的快照时间= %d \n", mapit->second.timestamp);
			//std::cout << "current snapshot time=" << mapit->second.timestamp << std::endl;
			if (mapit == snapshotlist.begin()) //没找到
			{
				std::cout << "ERROR! didn't find matched snapshot time" << std::endl << std::endl;
				LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 没找到匹配的快照，退出锁\n");
				return false;
			}
			mapit--;
		}
		LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 找到的快照块高度= %d , 时间= %d \n",
			mapit->second.blockHeight, mapit->second.timestamp);

		snapshot = mapit->second;
		LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 退出锁\n");
		return true;
	}

	LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 快照列表为空\n");
	LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 退出锁\n");
	return false;
}


bool CConsensusAccountPool::GetSnapshotsByTime(std::vector<SnapshotClass> &snapshotList, uint64_t starttime, uint64_t stoptime)
{
	LogPrintf("[CConsensusAccountPool::GetSnapshotsByTime] 请求只读锁\n");
	readLock rdlock(rwmutex);
	LogPrintf("[CConsensusAccountPool::GetSnapshotsByTime] 进入锁\n");
	LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 退出锁\n");
	return false;
}

bool CConsensusAccountPool::GetSnapshotByHeight(SnapshotClass &snapshot, uint32_t height)//获取块高度与指定高度相同的快照
{
	LogPrintf("[CConsensusAccountPool::GetSnapshotByHeight] 请求只读锁\n");
	readLock rdlock(rwmutex);
	LogPrintf("[CConsensusAccountPool::GetSnapshotByHeight] 进入锁\n");

	if (snapshotlist.empty())
	{
		std::cout << "[CConsensusAccountPool::GetSnapshotByHeight] 快照列表为空！\n";
		LogPrintf("[CConsensusAccountPool::GetSnapshotByHeight] 快照列表为空！\n");
		LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 退出锁\n");
		return false;
	}

	std::map<uint32_t, SnapshotClass>::iterator mapit = snapshotlist.end();
	mapit--;
	while (mapit->second.blockHeight > height && mapit != snapshotlist.begin())
		mapit--;

	if (mapit->second.blockHeight > height)
	{
		std::cout << "[CConsensusAccountPool::GetSnapshotsByHeight] 快照列表中没有不高于此高度的块！\n";
		LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] 快照列表中没有不高于此高度的块！\n");
		LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 退出锁\n");
		return false;
	}

	snapshot = mapit->second;
	LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 退出锁\n");
	return true;
}

bool CConsensusAccountPool::GetSnapshotsByHeight(std::vector<SnapshotClass> &snapshots, uint32_t lowest, uint32_t highest)//获取块高度与指定高度相同的快照
{
	LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] 请求只读锁\n");
	readLock rdlock(rwmutex);
	LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] 进入锁\n");

	LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] 需要读取的块高度范围在%d-%d(结束高度为0表示一直读取到最后)\n", lowest, highest);

	if (snapshotlist.empty())
	{
		std::cout << "[CConsensusAccountPool::GetSnapshotsByHeight] 快照列表为空！\n";
		LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] 快照列表为空！\n");
		LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 退出锁\n");
		return false;
	}

	std::map<uint32_t, SnapshotClass>::iterator mapit = snapshotlist.end();
	mapit--;

	if (highest != 0)//指定了最高高度
	{
		while (mapit->second.blockHeight > highest && mapit != snapshotlist.begin())
			mapit--;

		if (mapit == snapshotlist.begin())
		{
			std::cout << "[CConsensusAccountPool::GetSnapshotsByHeight] 快照列表中没有低于最高高度的块！\n";
			LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] 快照列表中没有低于最高高度的块！\n");
			LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 退出锁\n");
			return false;
		}
	}

	snapshots.clear();
	while (mapit->second.blockHeight >= lowest)
	{
		LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] 找到快照高度%d\n", mapit->second.blockHeight);
		snapshots.push_back(mapit->second);
		if (mapit == snapshotlist.begin())
			break;
		mapit--;
	}

	LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] 退出锁\n");
	return true;
}

bool CConsensusAccountPool::GetLastSnapshot(SnapshotClass &snapshot)//获取当前快照列表末尾的快照
{
	LogPrintf("[CConsensusAccountPool::GetLastSnapshot] 请求只读锁\n");
	readLock rdlock(rwmutex);
	LogPrintf("[CConsensusAccountPool::GetLastSnapshot] 进入锁\n");

	if (snapshotlist.empty())
	{
		std::cout << "[CConsensusAccountPool::GetLastSnapshot] 快照列表为空！\n";
		LogPrintf("[CConsensusAccountPool::GetLastSnapshot] 快照列表为空！\n");
		LogPrintf("[CConsensusAccountPool::GetLastSnapshot] 退出锁\n");
		return false;
	}

	std::map<uint32_t, SnapshotClass>::iterator mapit = snapshotlist.end();
	mapit--;

	snapshot = mapit->second;
	LogPrintf("[CConsensusAccountPool::GetLastSnapshot] 获取快照尾高度=%d, 计划打块时间=%d,实际打块时间=%d\n",
		snapshot.blockHeight, snapshot.timestamp, snapshot.blockTime);

	LogPrintf("[CConsensusAccountPool::GetLastSnapshot] 退出锁\n");
	return true;
}

bool CConsensusAccountPool::PushSnapshot(SnapshotClass snapshot)//将快照添加到快照列表末尾
{
	LogPrintf("[CConsensusAccountPool::PushSnapshot] 请求读写锁\n");
	writeLock wtlock(rwmutex);
	LogPrintf("[CConsensusAccountPool::PushSnapshot] 进入锁\n");

	if (snapshotlist.count(snapshot.blockHeight))
	{
		LogPrintf("[CConsensusAccountPool::PushSnapshot] 列表中已经有高度为 %d 的快照\n", snapshot.blockHeight);
		LogPrintf("[CConsensusAccountPool::PushSnapshot] 退出锁\n");
		return false;
	}

	LogPrintf("[CConsensusAccountPool::PushSnapshot] 向列表中添加高度为 %d 的快照\n", snapshot.blockHeight);
	snapshotlist[snapshot.blockHeight] = snapshot;

	//add begin by li
	//同步写文件
	
	//if ((snapshot.blockHeight ==2000) && (snapshot.blockHeight != 0))
	if ((0 == (snapshot.blockHeight % 2000)) && (snapshot.blockHeight != 0))
	{
		int64_t n1 = GetTimeMillis();
		LogPrintf("[CConsensusAccountPool::PushSnapshot] ++++++++n1=%d\n", n1);
		//写候选人列表
		//writeCandidatelistToFileByHeight(snapshot.blockHeight);
		writeCandidatelistToFile();

		boost::filesystem::path pathTmp = GetDataDir() / m_strSnapshotPath;
		uint64_t fileSize = 0;
		if (boost::filesystem::exists(pathTmp))
		{
			fileSize = boost::filesystem::file_size(pathTmp);
		}

		int nIndex = 0;
		if (snapshot.blockHeight == 2000)
		{
			nIndex = 0;
		}
		else
		{
			boost::filesystem::path pathTmpIndex = GetDataDir() / m_strSnapshotIndexPath;
			if (boost::filesystem::exists(pathTmpIndex))
			{
				uint64_t u64FileSizeIndex = boost::filesystem::file_size(pathTmpIndex);
				uint64_t u64Num = u64FileSizeIndex / m_u64SnapshotIndexSize;
				uint64_t u64Mod = u64Num % 2000;

				nIndex = snapshot.blockHeight - 1999;
				nIndex = nIndex + u64Mod;
			}
		}

		std::map<uint32_t, SnapshotClass>::iterator iter = snapshotlist.find(nIndex);

		boost::filesystem::path pathTmp1 = GetDataDir() / m_strSnapshotPath;
		FILE *file1 = fopen(pathTmp1.string().c_str(), "ab+");
		if (file1 == NULL)
		{
			LogPrintf("[CConsensusAccountPool::PushSnapshot] file1 is NULL\n");
			return false;
		}

		boost::filesystem::path pathTmpIndex1 = GetDataDir() / m_strSnapshotIndexPath;
		FILE *fileIndex1 = fopen(pathTmpIndex1.string().c_str(), "ab+");
		if (fileIndex1 == NULL)
		{
			LogPrintf("[CConsensusAccountPool::PushSnapshot] fileIndex1 is NULL\n");
			return false;
		}

		int nIndexTest = 0;
		for (; iter != snapshotlist.end(); ++iter)
		{
			++nIndexTest;
			//写Index
			//得到快照文件的起始偏移位置

			CSnapshotIndex  sanpshotIndex;
			sanpshotIndex.nHeight = iter->first;
			sanpshotIndex.nOffset = fileSize;

			//LogPrintf("[CConsensusAccountPool::PushSnapshot] ++++++++%d+++%d\n", sanpshotIndex.nHeight, sanpshotIndex.nOffset);
			//先写块文件
			//int64_t nTest1 = GetTimeMillis();
			//CSerializeDpoc<SnapshotClass> serializeSnapshot;
			//serializeSnapshot.WriteToDisk(iter->second, m_strSnapshotPath);

			//********************************************************************
			CAutoFile fileout1(file1, SER_DISK, CLIENT_VERSION);
			if (fileout1.IsNull())
			{
				LogPrintf("CSerializeDpoc::WriteToDisk return false by fileout.IsNull() \n");
				fclose(file1);
				return false;
			}

			try {
				fileout1 << (iter->second);
			}
			catch (const std::exception& e) {
				LogPrintf("CSerializeDpoc::WriteToDisk return false by %d \n", e.what());
				return false;
			}
			file1 = fileout1.release();
			//*********************************************************************


			//serializeSnapshot.WriteToDiskWithFlush(snapshot, m_strSnapshotPath);
			//int64_t nTest2 = GetTimeMillis();
			//LogPrintf("[CConsensusAccountPool::PushSnapshot] nTest2-nTest1=%d\n", nTest2 - nTest1);

			//记录Index文件实际结束位置
			//CSerializeDpoc<CSnapshotIndex> serializeIndex;
			//serializeIndex.WriteToDisk(sanpshotIndex, m_strSnapshotIndexPath);

			//**********************************

			CAutoFile fileoutIndex1(fileIndex1, SER_DISK, CLIENT_VERSION);
			if (fileoutIndex1.IsNull())
			{
				LogPrintf("CSerializeDpoc::WriteToDisk return false by fileout.IsNull() \n");
				fclose(fileIndex1);
				return false;
			}

			try {
				fileoutIndex1 << sanpshotIndex;
			}
			catch (const std::exception& e) {
				LogPrintf("CSerializeDpoc::WriteToDisk return false by %d \n", e.what());
				return false;
			}
			fileIndex1 = fileoutIndex1.release();
			//***********************************************

			//serializeIndex.WriteToDiskWithFlush(sanpshotIndex, m_strSnapshotIndexPath);
			//int64_t nTest3 = GetTimeMillis();
			//LogPrintf("[CConsensusAccountPool::PushSnapshot] nTest3-nTest2=%d\n", nTest3 - nTest2);

			m_mapSnapshotIndex[sanpshotIndex.nHeight] = sanpshotIndex.nOffset;

			fileSize = fileSize + getSnapshotSize(iter->second);
			//fileSize += 1;
		}
		fclose(file1);
		fclose(fileIndex1);
		file1 = NULL;
		fileIndex1 = NULL;

		flushSnapshotToDisk();
		int64_t n5 = GetTimeMillis();
		
		LogPrintf("[CConsensusAccountPool::PushSnapshot] n5-n1=%d\n", n5 - n1);
	}

	//写Index结束位置
	//uint64_t u64SnapshotIndexSize = m_u64SnapshotIndexSize;// getCSnapshotIndexSize();
	//CSerializeIndexRev indexRev;
	//indexRev.nOffset = u64SnapshotIndexSize + u64SnapshotIndexEnd;
	//CSerializeDpoc<CSerializeIndexRev> serializeIndexRev;
	//serializeIndexRev.WriteToDiskCover(indexRev, m_strSnapshotRevPath);


	/*boost::filesystem::path pathIndexTmp = GetDataDir() / m_strSnapshotIndexPath;
	uint64_t fileSizeEnd = boost::filesystem::file_size(pathIndexTmp);
	std::stringstream ss;
	ss << fileSizeEnd;
	std::string strIndexEnd = ss.str();

	boost::filesystem::path pathTmpEnd = GetDataDir() / m_strSnapshotRevPath;
	std::ofstream configFile;
	configFile.open(pathTmpEnd.string(), std::ios::out | std::ios::trunc);
	if (configFile.is_open())
	{
		configFile << strIndexEnd;
		configFile.flush();
		configFile.close();
	}*/
	//add end by li

	LogPrintf("[CConsensusAccountPool::PushSnapshot] 退出锁\n");
	return true;
}

bool CConsensusAccountPool::RemoveHigherSnapshotByHeight(uint32_t height)//从快照列表中移除所有高于和等于height的快照
{
	LogPrintf("[CConsensusAccountPool::RemoveHigherSnapshotByHeight] 请求读写锁\n");
	writeLock wtlock(rwmutex);
	LogPrintf("[CConsensusAccountPool::RemoveHigherSnapshotByHeight] 进入锁\n");
	LogPrintf("[CConsensusAccountPool::RemoveHigherSnapshotByHeight] 退出锁\n");
	return false;
}

bool CConsensusAccountPool::RemoveHigherSnapshotByTime(uint64_t timestamp)//从快照列表中移除所有实际打块时间不早于timestamp的快照，单位为秒
{
	LogPrintf("[CConsensusAccountPool::RemoveHigherSnapshotByTime] 请求读写锁\n");
	writeLock wtlock(rwmutex);
	LogPrintf("[CConsensusAccountPool::RemoveHigherSnapshotByTime] 进入锁\n");
	LogPrintf("[CConsensusAccountPool::RemoveHigherSnapshotByTime] 退出锁\n");
	return false;
}

bool CConsensusAccountPool::getPKIndexBySortedIndex(uint16_t &pkIndex, uint160 &pkhash, std::list<std::shared_ptr<CConsensusAccount >> consensusList, int sortedIndex)
{
	LogPrintf("[CConsensusAccountPool::getPKIndexBySortedIndex] 待查找候选人列表size=%d ，待查找index=%d\n", consensusList.size(), sortedIndex);
	std::list<std::shared_ptr<CConsensusAccount> >::iterator listit = consensusList.begin();
	for (int i=0; i<sortedIndex && i<consensusList.size() && listit != consensusList.end(); i++)
	{
		//std::cout << i << std::endl;
		listit++;
	}
	pkhash = (*listit)->getPubicKey160hash();
	return ContainPK(pkhash, pkIndex);
}

bool CConsensusAccountPool::GetTimeoutIndexs(const std::shared_ptr<const CBlock> pblock, SnapshotClass lastsnapshot, std::map<uint16_t, int>& timeoutindexs)
{
	timeoutindexs.clear();

	//计算从当前块到快照末尾漏掉了几个块，分别都是谁
	int timediff = (pblock->nPeriodStartTime + pblock->nTimePeriod * BLOCK_GEN_TIME + BLOCK_GEN_TIME) / 1000 - lastsnapshot.timestamp;
	std::cout << "打块时间差=" << timediff << std::endl;

	if (timediff > BLOCK_GEN_TIME/1000)
	{
		std::cout << "[CConsensusAccountPool::GetTimeoutIndexs] 有Timeout漏块\n";
		LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 有Timeout漏块\n");

		//计算当前轮会议的排序
		uint64_t currentmeetingcachedtime = (pblock->nPeriodStartTime - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME) /1000;
		SnapshotClass currentmeetingcachedsnapshot;
		if (!GetSnapshotByTime(currentmeetingcachedsnapshot, currentmeetingcachedtime))
		{
			LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 获取缓存的候选人列表失败\n");
			return false;
		}

		std::list<std::shared_ptr<CConsensusAccount >> consensusList;
		consensusList.clear();
		std::set<uint16_t>::iterator candidateit;
		for (candidateit = currentmeetingcachedsnapshot.curCandidateIndexList.begin();
			candidateit != currentmeetingcachedsnapshot.curCandidateIndexList.end();
			candidateit++)
		{
			std::shared_ptr<CConsensusAccount> tmpaccount = std::make_shared<CConsensusAccount>(candidatelist.at(*candidateit));
			consensusList.push_back(tmpaccount);
		}
		bool result = CDpocMining::Instance().GetMeetingList(pblock->nPeriodStartTime, consensusList);
		if (!result)
		{
			LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 计算当前轮会议排序失败\n");
			return false;
		}
		////按顺序打印本轮会议公钥HASH索引值
		//LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 起始时间为%d的会议打块人排序：\n", pblock->nPeriodStartTime);
		//std::list<std::shared_ptr<CConsensusAccount >>::iterator consensusListIt;
		//for (consensusListIt =  consensusList.begin(); consensusListIt != consensusList.end(); consensusListIt ++)
		//{
		//	uint16_t tmpIndex = -1;
		//	uint160 tmppkhash = (*consensusListIt)->getPubicKey160hash();
		//	ContainPK(tmppkhash, tmpIndex);
		//	LogPrintf("%d ", tmpIndex);
		//}
		//LogPrintf("\n\n");

		std::cout << "GetMeetingList result=" << result << "list size =" << consensusList.size() << std::endl;

		//判断快照末尾的块和当前块是否为同一轮会议，或者是上一轮会议的尾块
		if (lastsnapshot.meetingstarttime == pblock->nPeriodStartTime || lastsnapshot.timestamp == pblock->nPeriodStartTime / 1000)
		{
			std::cout << "仅本轮漏块" << std::endl;
			int index = 0;
			while ((pblock->nPeriodStartTime + index*BLOCK_GEN_TIME + BLOCK_GEN_TIME) / 1000 <= lastsnapshot.timestamp)
			{
				std::cout << "开始时间" << lastsnapshot.meetingstarttime << ", 序号" << index << ", 尾块打块时间" << lastsnapshot.timestamp << std::endl;
				index++;
			}
			std::cout << "第一个漏打的排序=" << index << std::endl;
			//找到漏打块的index索引值，开始处理超时
			for (index; index < pblock->nTimePeriod; index++)
			{
				std::cout << "[CConsensusAccountPool::GetTimeoutIndexs] 本轮公钥HASH排序" << index << "的节点没有打块，记入超时记录\n";
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 本轮公钥HASH排序%d 的节点没有打块，记入超时记录\n", index);
				uint16_t tmpPubkeyIndex;
				uint160 tmpPubkeyHash;
				if (!getPKIndexBySortedIndex(tmpPubkeyIndex, tmpPubkeyHash, consensusList, index))
				{
					LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 没有找到该公钥HASH的索引！\n", index);
					return false;
				}
				std::cout << "公钥HASH索引" << tmpPubkeyIndex << "记录一次超时" << std::endl;
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 公钥HASH索引%d记录一次超时\n", tmpPubkeyIndex);
				int count = 1;
				if (timeoutindexs.count(tmpPubkeyIndex))
				{
					count = timeoutindexs[tmpPubkeyIndex] ++;
				}
				timeoutindexs[tmpPubkeyIndex] = count;
			}
		}
		else
		{
			std::cout << "跨轮漏块" << std::endl;
			int index = 0;
			//记录当前轮漏块
			for (index = 0; index < pblock->nTimePeriod; index++)
			{
				std::cout << "[CConsensusAccountPool::GetTimeoutIndexs] 当前轮排序为" << index << "的节点没有打块，记入超时记录\n";
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 当前轮排序为%d 的节点没有打块，记入超时记录\n", index);
				uint16_t tmpPubkeyIndex;
				uint160 tmpPubkeyHash;
				if (!getPKIndexBySortedIndex(tmpPubkeyIndex, tmpPubkeyHash, consensusList, index))
				{
					LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 没有找到该公钥HASH的索引！\n", index);
					return false;
				}
				std::cout << "公钥HASH索引" << tmpPubkeyIndex << "记录一次超时" << std::endl;
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 公钥HASH索引%d记录一次超时\n", tmpPubkeyIndex);
				int count = 1;
				if (timeoutindexs.count(tmpPubkeyIndex))
				{
					count = timeoutindexs[tmpPubkeyIndex] ++;
				}
				timeoutindexs[tmpPubkeyIndex] = count;
			}

			//计算上一轮缓存列表
			currentmeetingcachedtime = (lastsnapshot.meetingstarttime - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME) / 1000;
			if (!GetSnapshotByTime(currentmeetingcachedsnapshot, currentmeetingcachedtime))
			{
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 获取上一轮缓存的候选人列表失败\n");
				return false;
			}
			consensusList.clear();
			std::set<uint16_t>::iterator candidateit;
			for (candidateit = currentmeetingcachedsnapshot.curCandidateIndexList.begin();
				candidateit != currentmeetingcachedsnapshot.curCandidateIndexList.end();
				candidateit++)
			{
				std::shared_ptr<CConsensusAccount> tmpaccount = std::make_shared<CConsensusAccount>(candidatelist.at(*candidateit));
				consensusList.push_back(tmpaccount);
			}
			bool result = CDpocMining::Instance().GetMeetingList(lastsnapshot.meetingstarttime, consensusList);
			if (!result)
			{
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 计算上一轮会议排序失败\n");
				return false;
			}

			////按顺序打印上一轮轮会议公钥HASH索引值
			//LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 起始时间为%d的会议（上轮）打块人排序：\n", lastsnapshot.meetingstarttime);
			//std::list<std::shared_ptr<CConsensusAccount >>::iterator consensusListIt;
			//for (consensusListIt = consensusList.begin(); consensusListIt != consensusList.end(); consensusListIt++)
			//{
			//	uint16_t tmpIndex = -1;
			//	uint160 tmppkhash = (*consensusListIt)->getPubicKey160hash();
			//	ContainPK(tmppkhash, tmpIndex);
			//	LogPrintf("%d ", tmpIndex);
			//}
			//LogPrintf("\n\n");
			//

			//计算上一轮漏块到末尾
			while ((lastsnapshot.meetingstarttime + index*BLOCK_GEN_TIME + BLOCK_GEN_TIME) / 1000 <= lastsnapshot.timestamp)
			{
				index++;
			}
			std::cout << "第一个漏打的排序=" << index << std::endl;
			//找到漏打块的index索引值，开始处理超时
			for (index; lastsnapshot.meetingstarttime + index*BLOCK_GEN_TIME + BLOCK_GEN_TIME <= lastsnapshot.meetingstoptime; index++)
			{
				std::cout << "[CConsensusAccountPool::GetTimeoutIndexs] 上一轮公钥HASH排序为" << index << "的节点没有打块，记入超时记录\n";
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 上一轮公钥HASH排序为%d 的节点没有打块，记入超时记录\n", index);
				uint16_t tmpPubkeyIndex;
				uint160 tmpPubkeyHash;
				if (!getPKIndexBySortedIndex(tmpPubkeyIndex, tmpPubkeyHash, consensusList, index))
				{
					LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 没有找到该公钥HASH的索引！\n", index);
					return false;
				}
				std::cout << "公钥HASH索引" << tmpPubkeyIndex << "记录一次超时" << std::endl;
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] 公钥HASH %s 索引%d记录一次超时\n", tmpPubkeyHash.GetHex().c_str(), tmpPubkeyIndex);
				int count = 1;
				if (timeoutindexs.count(tmpPubkeyIndex))
				{
					count = timeoutindexs[tmpPubkeyIndex] ++;
				}
				timeoutindexs[tmpPubkeyIndex] = count;
			}

		}
	}
	return true;
}	

CAmount CConsensusAccountPool::GetCurDepositAdjust(uint160 pkhash, uint32_t blockheight)
{
	CAmount deposit = Params().MIN_DEPOSI; //最小押金调为MIN_DEPOSI

	for (std::vector<std::string>::iterator trustit = trustPKHashList.begin();
		trustit != trustPKHashList.end(); trustit++)
	{
		if (pkhash.GetHex() == *trustit)
		{
			LogPrintf("[CConsensusAccountPool::GetCurDepositThreshold] 信任公钥，押金=%d (%f IPC)\n", deposit, (double)deposit / COIN);
			return deposit;
		}
	}

	//uint32_t currentblockheight = chainActive.Height();
	uint32_t currentblockheight = blockheight;
	uint32_t consultblockheight = (currentblockheight-1 )/ 1000;
	consultblockheight *= 1000;
	if (consultblockheight > chainActive.Tip()->nHeight)
		consultblockheight = chainActive.Tip()->nHeight;
	CBlockIndex* pblockindex = chainActive[consultblockheight];
	if (NULL == pblockindex)
	{
		LogPrintf("[CConsensusAccountPool::GetCurDepositThreshold] NULL = pblockindex!!!! \n");
		return Params().MIN_DEPOSI * 34;
	}
	uint32_t count = pblockindex->nPeriodCount;
//	if (count < chainActive.Tip()->nPeriodCount) count = chainActive.Tip()->nPeriodCount;
	LogPrintf("[CConsensusAccountPool::GetCurDepositThreshold] 取MAX[%d个块, 当前最高块]的会议人数)：%d\n",consultblockheight, count);
	
	if (count <= 20)
	{
		deposit = Params().MIN_DEPOSI;
	}
	else if (count <= 30)
	{
		deposit = Params().MIN_DEPOSI * 2;
	}
	else if (count <= 40)
	{
		deposit = Params().MIN_DEPOSI * 3;
	}
	else if (count <= 50)
	{
		deposit = Params().MIN_DEPOSI * 5;
	}
	else if (count <= 60)
	{
		deposit = Params().MIN_DEPOSI * 8;
	}
	else if (count <= 70)
	{
		deposit = Params().MIN_DEPOSI * 13;
	}
	else if (count <= 100)
	{
		deposit = Params().MIN_DEPOSI * 21;
	}
	else
	{
		deposit = Params().MIN_DEPOSI * 34;
	}

	LogPrintf("[CConsensusAccountPool::GetCurDepositThreshold] 当前按照人数计算出的调整费用=%d (%f IPC)\n", deposit, (double)deposit/COIN);

	uint16_t tmpIndex;
	if (ContainPK(pkhash, tmpIndex))
	{
		/*if (blockheight > 85000 && blockheight <= Params().ADJUSTDP_BLOCKS)//如果在第一年内
		{
			deposit = candidatelist[tmpIndex].getJoinIPC();
			LogPrintf("[CConsensusAccountPool::GetCurDepositThreshold] 第一年内押金费用保持第一次加入押金数目=%d (%f IPC)\n", deposit, (double)deposit / COIN);
			return  deposit;
		}*/
		//根据信用值减免调整费用
		int64_t Credit = candidatelist[tmpIndex].getCredit();

		Credit -= Credit % 1000;//信用值千位取整

		deposit -= Credit * COIN;
		if (deposit < Params().MIN_DEPOSI)
		{
			deposit = Params().MIN_DEPOSI;
		}
		
		LogPrintf("[CConsensusAccountPool::GetCurDepositThreshold] 根据信用值减免后的调整费用=%d (%f IPC)\n", deposit, (double)deposit / COIN);
		
	}
	//deposit = Params().MIN_DEPOSI;
	return deposit ;
}

/************************************************************************/
/* 后面的都是旧函数                                                       */
/************************************************************************/

bool CConsensusAccountPool::listSnapshotsToTime(std::list<std::shared_ptr<CConsensusAccount>> &listConsus, int readtime)
{
	//std::cout << "AccountPool::listSnapshotsToTime() called: time=" << readtime << std::endl;
	listConsus.clear();
	//std::lock_guard<std::mutex> lock(poolMutex);
	SnapshotClass cachedsnapshot;
	if (!GetSnapshotByTime(cachedsnapshot, readtime))
	{
		LogPrintf("[CConsensusAccountPool::listSnapshotsToTime] 查找缓存快照失败\n");
		return false;
	}

	std::set<uint16_t> accounts = cachedsnapshot.curCandidateIndexList;
	std::set<uint16_t>::const_iterator iter;

	////清除黑名单中存在的公钥【这一步暂时不做了，拒收黑名单的块就行了】
	//SnapshotClass meetingstarttimesnapshot;
	//if (!GetSnapshotByTime(meetingstarttimesnapshot, readtime + (CACHED_BLOCK_COUNT * BLOCK_GEN_TIME)/1000) )
	//{公钥索引16的信用值
	//	LogPrintf("[CConsensusAccountPool::listSnapshotsToTime] 查找当前会议开始时间快照失败\n");
	//	return false;
	//}
	//
	//for (iter = meetingstarttimesnapshot.curBanList.begin(); iter!=meetingstarttimesnapshot.curBanList.end(); iter++)
	//{
	//	if (accounts.count(*iter))
	//	{
	//		LogPrintf("[CConsensusAccountPool::listSnapshotsToTime] 从候选人列表中去掉黑名单包括的索引%d\n", *iter);
	//		accounts.erase(*iter);
	//	}
	//}
	//for (iter = meetingstarttimesnapshot.cachedBanList.begin(); iter != meetingstarttimesnapshot.cachedBanList.end(); iter++)
	//{
	//	if (accounts.count(*iter))
	//	{
	//		LogPrintf("[CConsensusAccountPool::listSnapshotsToTime] 从候选人列表中去掉黑名单包括的索引%d\n", *iter);
	//		accounts.erase(*iter);
	//	}
	//}
	for (iter = accounts.begin(); iter != accounts.end(); iter++)
	{
	
		CConsensusAccount acount = candidatelist.at(*iter);
		listConsus.emplace_back(std::make_shared<CConsensusAccount>(acount));
	
	}

	return true;
}

bool CConsensusAccountPool::listSnapshots(std::list<std::shared_ptr<CConsensusAccount>> &listConsus){
	std::cout << "AccountPool::listSnapshots() called" << std::endl;

	int64_t nNowTime = timeService.GetCurrentTimeMillis()/1000;

	return listSnapshotsToTime(listConsus, nNowTime);
}

//从本地数据库中分析
bool  CConsensusAccountPool::analysisConsensusSnapshots()
{
	LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] begin\n");

	uint32_t nHeight = 0;
	{
		writeLock wtlock(rwmutex);
		//加载共识公钥HASH
		readCandidatelistFromFile();

		boost::filesystem::path pathIndex = GetDataDir() / m_strSnapshotIndexPath;
		boost::filesystem::path pathSnapshot = GetDataDir() / m_strSnapshotPath;
		if (boost::filesystem::exists(pathIndex) && boost::filesystem::exists(pathSnapshot))
		{
			//得到SerializeIndexEnd文件中SerializeIndex的结束位置
			uint64_t fileSizeSnapshot = boost::filesystem::file_size(pathSnapshot);

			uint64_t u64fileSize = boost::filesystem::file_size(pathIndex);
			for (uint64_t nBegin = 0; nBegin <= u64fileSize - m_u64SnapshotIndexSize; nBegin += m_u64SnapshotIndexSize)
			{
				CSerializeDpoc<CSnapshotIndex> serializeIndex;
				CSnapshotIndex  sanpshotIndex;
				serializeIndex.ReadFromDisk(sanpshotIndex, nBegin, m_strSnapshotIndexPath);
				
				//加载快照
				if (sanpshotIndex.nOffset < fileSizeSnapshot)
				{
					CSerializeDpoc<SnapshotClass> serializeSnapshot;
					SnapshotClass snapshot;
					serializeSnapshot.ReadFromDisk(snapshot, sanpshotIndex.nOffset, m_strSnapshotPath);

					snapshotlist[sanpshotIndex.nHeight] = snapshot;
					m_mapSnapshotIndex[sanpshotIndex.nHeight] = sanpshotIndex.nOffset;

					if ((sanpshotIndex.nOffset+getSnapshotSize(snapshot)) >= fileSizeSnapshot)
					{
						break;
					}
				}
				else
				{
					break;
				}
			}//for

		}

		if (1 < snapshotlist.size())
		{
			std::map<uint32_t, SnapshotClass>::iterator iterMap = snapshotlist.end();
			--iterMap;
			nHeight = iterMap->first;
			LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] snapshotlist高度：%d\n", nHeight);
			LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] snapshotlist：%d\n", snapshotlist.size());
		}

		LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] 读取文件完成\n");
	}

	int nChainHeight = 0;
	{
		LOCK(cs_main);
		nChainHeight = chainActive.Height();
		LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] chainActive高度：%d\n", nChainHeight);
	}

	//如果chainActive高度<Snapshot高度
	if (nChainHeight < (snapshotlist.size() - 1))
	{
		//截断候选人列表,并写入文件
		rollbackCandidatelist(nChainHeight);
		writeCandidatelistToFile();
		boost::filesystem::path pathCandidatelist = GetDataDir() / m_strCandidatelistPath;
		FILE *fileCandidatelist = fopen(pathCandidatelist.string().c_str(), "ab+");
		if (fileCandidatelist != NULL)
		{
			FileCommit(fileCandidatelist);
		}
		fclose(fileCandidatelist);
		
		//snapshot文件高度
		uint64_t u64SnapshotFileSize = m_mapSnapshotIndex[nChainHeight +1];
		//snapshotIndex文件高度
		int nTruncateNum = snapshotlist.size() - nChainHeight ;
		--nTruncateNum;

		std::map<uint32_t, uint64_t>::iterator iterIndex = m_mapSnapshotIndex.find(nChainHeight+1);
		m_mapSnapshotIndex.erase(iterIndex, m_mapSnapshotIndex.end());
		
		//删除多余的快照列表
		std::map<uint32_t, SnapshotClass>::iterator iterSnapShot = snapshotlist.find(nChainHeight + 1);
		snapshotlist.erase(iterSnapShot, snapshotlist.end());
		
		//LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] %d\n", snapshotlist.size());
		
		//截断SnapshotIndex文件
		boost::filesystem::path pathTmpIndex = GetDataDir() / m_strSnapshotIndexPath;
		uint64_t fileSizeIndex = 0;
		if (boost::filesystem::exists(pathTmpIndex))
		{
			fileSizeIndex = boost::filesystem::file_size(pathTmpIndex);
			fileSizeIndex = fileSizeIndex - m_u64SnapshotIndexSize*nTruncateNum;
		
			FILE *fileIndex = fopen(pathTmpIndex.string().c_str(), "ab+");
			if (fileIndex)
			{
				TruncateFile(fileIndex, fileSizeIndex);
			}
			fclose(fileIndex);
		}

		//截断Snapshot文件
		boost::filesystem::path pathTmp = GetDataDir() / m_strSnapshotPath;
		uint64_t fileSize = 0;
		if (boost::filesystem::exists(pathTmp))
		{
			FILE *file = fopen(pathTmp.string().c_str(), "ab+");
			if (file)
			{
				TruncateFile(file, u64SnapshotFileSize);
			}
			fclose(file);
		}

		LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] chainActive高度<Snapshot高度,处理完成\n");
	}

	//如果chainActive高度>Snapshot高度
	//从文件读
	std::cout << "AccountPool::analysisConsensusSnapshots() called" << std::endl;
	//LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] 被调用\n");
	CBlockIndex* pblockindex = NULL; 
	{
		LOCK(cs_main);
		if (chainActive.Genesis() == NULL)
		{
			LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] chainActive.Genesis()为空，返回正确\n");
			analysisfinished = true;
			return true;
		}

		pblockindex = chainActive.Genesis();
	}
	
	while (NULL != pblockindex)
	{
		boost::this_thread::interruption_point();

		//if (ShutdownRequested())
		//	break;

		if (( pblockindex->nHeight <= nHeight )&& nHeight!=0)
		{
			{
				LOCK(cs_main);
				pblockindex = chainActive.Next(pblockindex);
			}
			continue;
		}

		CBlock block;
		{
			LOCK(cs_main);
			if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())){
				LogPrintf("[analysisConsensusSnapshots] Can't read block from disk");
				continue;
			}
		}
		std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(block);
		DPOC_errtype errorType;
		if (!verifyDPOCBlock(shared_pblock, pblockindex->nHeight, errorType))
			return false;

		LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] 准备push块高度为%d的块\n", pblockindex->nHeight);
		if (!pushDPOCBlock(shared_pblock, pblockindex->nHeight))
			return false;
		
		{
			LOCK(cs_main);
			pblockindex = chainActive.Next(pblockindex);
		}

		//MilliSleep(1);
	}

	analysisfinished = true;
	LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] end by 处理本地块完毕，返回正确\n");
	return true;
}


bool CConsensusAccountPool::analysisNewBlock(const std::shared_ptr<const CBlock> pblock)
{

	std::cout << "AccountPool::analysisNewBlock() called" << std::endl;

	//std::cout << "IPC-----CConsensusAccountPool::analysisNewBlock111111" << std::endl;
	//if (!pblock || pblock->IsNull() )
	//	return false;
	//std::cout << "IPC-----CConsensusAccountPool::analysisNewBlock22222" << std::endl;
	//for (auto it = pblock->vtx.begin(); it != pblock->vtx.end(); it++)
	//{
	//	const CTransactionRef  aTransaction = *it;
	//	std::cout << "IPC-----CConsensusAccountPool::analysisNewBlock333333" << std::endl;
	//	std::cout << "IPC-----CConsensusAccountPool::analysisNewBlock9999---（"<< aTransaction->GetCampaignType() << std::endl;
	//	// 1. 竞选加入
	//	//CampaignType_t type == aTransaction->GetCampaignType() ；
	//
	//	if (TYPE_CONSENSUS_REGISTER == aTransaction->GetCampaignType())
	//	{
	//		//1.1. 判断交易的IPC押金数目，是否小于
	//		static const CAmount COIN = 100000000;
	//		CAmount  joinipc = aTransaction->GetRegisterIPC();
	//		if (joinipc >= 40 * COIN) {
	//			uint160 pubKeyhash = aTransaction->GetCampaignPublickey();
	//
	//			std::cout << "Join transation hash= " << aTransaction->GetHash().GetHex() << "  pubKeyhash=" << pubKeyhash.GetHex() << std::endl;
	//			if (!CConsensusAccountPool::Instance().contain(pubKeyhash))
	//			{
	//				CConsensusAccountPool::Instance().addConsensusAccountByPublicKey(pubKeyhash, joinipc);
	//			}
	//		}
	//	}
	//	else if (TYPE_CONSENSUS_QUITE == aTransaction->GetCampaignType()) // 退出交易
	//	{
	//		uint160 pubKeyhash = aTransaction->GetCampaignPublickey();
	//
	//		std::cout << "Exit transation hash= " << aTransaction->GetHash().GetHex() << "  pubKeyhash=" << pubKeyhash.GetHex() << std::endl;
	//		CConsensusAccountPool::Instance().deleteConsenssusAccountByPublickey(pubKeyhash);
	//
	//		std::cout << "commit remove dpoc api\n";
	//		
	//	}
	//}
	//std::cout << "IPC-----CConsensusAccountPool::analysisNewBlock4444" << std::endl;
	return true;
}


//######################################################################

//###########################################################################################
//同步public key 来添加共识成员。
bool CConsensusAccountPool::addConsensusAccountByPublicKey(uint160 &publiykeyhash ,CAmount &joinipc) {
	std::lock_guard<std::mutex> lock(poolMutex);
	if (contain(publiykeyhash))
	{
		return true;
	}
	m_listConsensusAccount.emplace_back(std::unique_ptr<CConsensusAccount>(new CConsensusAccount(publiykeyhash,joinipc)));
	return true;
}

//删除掉账号
bool CConsensusAccountPool::deleteConsenssusAccountByPublickey(uint160 &publiykeyhash)
{
	std::lock_guard<std::mutex> lock(poolMutex);

	std::list<std::unique_ptr<CConsensusAccount>>::iterator iter = m_listConsensusAccount.begin();
	for (; iter != m_listConsensusAccount.end(); )
	{
		if (publiykeyhash == (iter->get())->getPubicKey160hash())
		{
			iter = m_listConsensusAccount.erase(iter);

			//add by li
			//交易成功发送后，调用底层系统接口删除公钥
			std::string strPublicKey;
			int nRet = CDpocInfo::Instance().GetLocalAccount(strPublicKey);
			if (strPublicKey == publiykeyhash.GetHex())
			{
				CDpocInfo::Instance().RemoveInfo();
				CDpocInfo::Instance().setJoinCampaign(false);
			}
			return true;
		}
		else {
			iter++;
		}
	}
	return false;
}

//判断是否已经含有这个公钥，链表不会存在相同的公钥
bool CConsensusAccountPool::contain(uint160& publiykeyhash)
{
	std::list<std::unique_ptr<CConsensusAccount>>::const_iterator iter = m_listConsensusAccount.begin();
	for ( ; iter != m_listConsensusAccount.end() ; ++iter )
	{
		if (publiykeyhash == (iter->get())->getPubicKey160hash())
		{
			return true;
		}
	}
	return false;
}



std::shared_ptr<CConsensusAccount> CConsensusAccountPool::getConsensAccountByPublickeyHash(uint160 &publicKeyHash) {
	std::lock_guard<std::mutex> lock(poolMutex);

	std::list<std::unique_ptr<CConsensusAccount>>::const_iterator iter = m_listConsensusAccount.begin();
	for (; iter != m_listConsensusAccount.end(); ++iter)
	{
		CConsensusAccount *acount = iter->get();
		if (publicKeyHash == acount->getPubicKey160hash())
		{
			return std::make_shared<CConsensusAccount>(new CConsensusAccount(*acount));
		}
	}
	
	return NULL;
}

//#############################################################################333
//测试输出
void CConsensusAccountPool::debugStr() {
	std::cout << "[CConsensusAccountPool::debugStr]   num " << m_listConsensusAccount.size() << std::endl;

}



//#####################################################################################################
//普通的惩罚
bool CConsensusAccountPool::containNormalViolationListByPublickey(uint160& publiykeyhashy) {
	std::list<std::unique_ptr<CConsensusAccount>>::const_iterator iter = m_ListNormalViolation.begin();
	for (; iter != m_ListNormalViolation.end(); ++iter)
	{
		if (publiykeyhashy == (iter->get())->getPubicKey160hash())
		{
			return true;
		}
	}
	return false;
}

bool CConsensusAccountPool::addCA2NormalViolationList(uint160 &publicKeyHash) 
{
	std::lock_guard<std::mutex> lock(poolMutex);
	if (containNormalViolationListByPublickey(publicKeyHash))
	{
		return true;
	}
	m_ListNormalViolation.emplace_back(std::unique_ptr<CConsensusAccount>(new CConsensusAccount(publicKeyHash)));
	return true;
}

void CConsensusAccountPool::removeNormalViolationList() {
	std::lock_guard<std::mutex> lock(poolMutex);
	m_ListNormalViolation.clear();
}

bool CConsensusAccountPool::getNormalViolationLisSnapshots(std::list<std::shared_ptr<CConsensusAccount>> &listNormalViolation) {
	listNormalViolation.clear();
	std::lock_guard<std::mutex> lock(poolMutex);

	std::list<std::unique_ptr<CConsensusAccount>>::const_iterator iter = m_ListNormalViolation.begin();
	for (; iter != m_listConsensusAccount.end(); ++iter)
	{
		CConsensusAccount *acount = iter->get();
		listNormalViolation.emplace_back(std::make_shared<CConsensusAccount>(new CConsensusAccount(*acount)));
	}
	return true;
}


//###############################################################################
// 严重违规
bool CConsensusAccountPool::containCriticalViolationListByPublickey(uint160 &publicKeyHash) {
	std::lock_guard<std::mutex> lock(poolMutex);
	std::list<std::unique_ptr<CConsensusAccount>>::const_iterator iter = m_ListCriticalViolation.begin();
	for (; iter != m_ListCriticalViolation.end(); ++iter)
	{
		if (publicKeyHash == (iter->get())->getPubicKey160hash())
		{
			return true;
		}
	}
	return false;
}

bool CConsensusAccountPool::addCA2CriticalViolationList(uint160 &publicKeyHash) {
	std::lock_guard<std::mutex> lock(poolMutex);

	std::list<std::unique_ptr<CConsensusAccount>>::const_iterator iter = m_ListCriticalViolation.begin();
	for (; iter != m_ListCriticalViolation.end(); ++iter)
	{
		if (publicKeyHash == (iter->get())->getPubicKey160hash())
		{
			return true;
		}
	}

	m_ListCriticalViolation.emplace_back(std::unique_ptr<CConsensusAccount>(new CConsensusAccount(publicKeyHash)));
	return true;
}

void CConsensusAccountPool::removeCriticalViolationList() {
	std::lock_guard<std::mutex> lock(poolMutex);

	m_ListCriticalViolation.clear();
}

bool CConsensusAccountPool::getCriticalViolationLisSnapshots(std::list<std::shared_ptr<CConsensusAccount>> &listCriticalViolation)
{
	listCriticalViolation.clear();
	std::lock_guard<std::mutex> lock(poolMutex);

	std::list<std::unique_ptr<CConsensusAccount>>::const_iterator iter = m_ListCriticalViolation.begin();
	for (; iter != m_listConsensusAccount.end(); ++iter)
	{
		CConsensusAccount *acount = iter->get();
		listCriticalViolation.emplace_back(std::make_shared<CConsensusAccount>(new CConsensusAccount(*acount)));
	}

	return true;
}

//add begin by li at 2017/10/23 14:30
bool CConsensusAccountPool::SetConsensusStatus(const std::string &strStatus, const std::string &strHash)
{
	//设置处罚的值
	std::string strPublicKey;
	CDpocInfo::Instance().GetLocalAccount(strPublicKey);

	bool bRet = false;
	if ((strPublicKey == strHash)&&(!strPublicKey.empty()))
	{
		//std::string strStatus("1");
	    bRet = CDpocInfo::Instance().SetConsensusStatus(strStatus, strPublicKey);
	}

	LogPrintf("[CConsensusAccountPool::SetConsensusStatus] 本人公钥%s,设置共识状态返回值 %d\n", strPublicKey, bRet);
}
//add end by li at 2017/10/23 14:30

uint64_t CConsensusAccountPool::getCSnapshotIndexSize()
{
	CSnapshotIndex snapshotIndex;
	CDataStream ssSnapshotIndex(SER_DISK, CLIENT_VERSION);
	ssSnapshotIndex << snapshotIndex;
	
	return ssSnapshotIndex.size();
}

uint64_t CConsensusAccountPool::getSnapshotSize(SnapshotClass &snapshot)
{
	CDataStream ssSnapshot(SER_DISK, CLIENT_VERSION);
	ssSnapshot << snapshot;

	return ssSnapshot.size();
}
uint64_t CConsensusAccountPool::getSnapshotIndexEnd()
{
	//得到SerializeIndexEnd文件中SerializeIndex的结束位置
	boost::filesystem::path pathTmp = GetDataDir() / m_strSnapshotRevPath;
	if (!boost::filesystem::exists(pathTmp))
	{
		return 0;
	}

	CSerializeIndexRev indexRev;
	CSerializeDpoc<CSerializeIndexRev> serializeIndexRev;
	serializeIndexRev.ReadFromDisk(indexRev, 0, m_strSnapshotRevPath);
	return indexRev.nOffset;
}

void CConsensusAccountPool::writeCandidatelistToFile()
{
	boost::filesystem::path pathTmp = GetDataDir() / m_strCandidatelistPath;
	if (boost::filesystem::exists(pathTmp))
	{
		boost::filesystem::remove(pathTmp);
	}
	CSerializeDpoc<CConsensusAccount> serializeAccount;
	std::vector<CConsensusAccount>::iterator iter = candidatelist.begin();
	for (; iter != candidatelist.end();++iter)
	{
	   serializeAccount.WriteToDisk(*iter, m_strCandidatelistPath);
	}	
}

void CConsensusAccountPool::writeCandidatelistToFileByHeight(uint32_t nHeight)
{
	boost::filesystem::path pathTmp = GetDataDir() / m_strCandidatelistPath;
	if (boost::filesystem::exists(pathTmp))
	{
		boost::filesystem::remove(pathTmp);
	}
	CSerializeDpoc<CConsensusAccount> serializeAccount;
	std::vector<CConsensusAccount>::iterator iter = candidatelist.begin();
	for (; iter != candidatelist.end(); ++iter)
	{
		if (iter->getHeight()<= nHeight)
		{
			serializeAccount.WriteToDisk(*iter, m_strCandidatelistPath);
		}
	}
}


bool CConsensusAccountPool::writeCandidatelistToFile(const CConsensusAccount &account)
{
	CSerializeDpoc<CConsensusAccount> serializeAccount;
	return serializeAccount.WriteToDiskWithFlush(account, m_strCandidatelistPath);
}

bool CConsensusAccountPool::readCandidatelistFromFile()
{
	boost::filesystem::path pathTmp = GetDataDir() / m_strCandidatelistPath;
	if (boost::filesystem::exists(pathTmp))
	{
		uint64_t u64fileSize = boost::filesystem::file_size(pathTmp);
		uint64_t u64Seek = m_u64ConsensAccountSize;//getConsensAccountSize();
		for (int nIndex = 0; nIndex <= u64fileSize - u64Seek; nIndex += u64Seek)
		{
			CConsensusAccount account;
			CSerializeDpoc<CConsensusAccount> serializeAccount;
			serializeAccount.ReadFromDisk(account, nIndex, m_strCandidatelistPath);
		    candidatelist.push_back(account);
		}	
	}
	return false;
}

uint64_t CConsensusAccountPool::getConsensAccountSize()
{
	CConsensusAccount account;
	CDataStream ssConsensusAccount(SER_DISK, CLIENT_VERSION);
	ssConsensusAccount << account;
	return ssConsensusAccount.size();
}

void CConsensusAccountPool::flushSnapshotToDisk()
{
	//writeCandidatelistToFile();

	std::cout << "CConsensusAccountPool Destruct writeCandidatelistToFile end" << std::endl;
	boost::filesystem::path pathTmp = GetDataDir() / m_strSnapshotPath;
	FILE *file = fopen(pathTmp.string().c_str(), "ab+");
	if (file != NULL)
	{
		FileCommit(file);
	}
	fclose(file);
	std::cout << "CConsensusAccountPool Destruct FileCommitSnapshot end" << std::endl;

	boost::filesystem::path pathTmpIndex = GetDataDir() / m_strSnapshotIndexPath;
	FILE *fileIndex = fopen(pathTmpIndex.string().c_str(), "ab+");
	if (fileIndex != NULL)
	{
		FileCommit(fileIndex);
	}
	fclose(fileIndex);

	std::cout << "CConsensusAccountPool Destruct FileCommitIndex end" << std::endl;

	boost::filesystem::path pathCandidatelist = GetDataDir() / m_strCandidatelistPath;
	FILE *fileCandidatelist = fopen(pathCandidatelist.string().c_str(), "ab+");
	if (fileCandidatelist != NULL)
	{
		FileCommit(fileCandidatelist);
	}
	fclose(fileCandidatelist);
	std::cout << "CConsensusAccountPool Destruct FileCommitCandidatelist end" << std::endl;
}

bool CConsensusAccountPool::verifyBlockSign(const CBlock *pblock)
{
	//获取块中的公钥和签名字符串
	/*CPubKey recvPublickey;
	std::vector<unsigned char>  recvSign;
	if (!getPublicKeyFromBlock(pblock, recvPublickey, recvSign)) {
		LogPrintf("[CConsensusAccountPool::verifyBlockSign] getPublicKeyFromBlock is badlly\n");
		return false;
	}
	CKeyID  pubicKey160hash = recvPublickey.GetID();
	//验签
	uint256 hash;
	CAmount nfee = pblock->vtx[0]->GetValueOut();
	CHash256 hashoperator;

	hashoperator.Write((unsigned char*)&(pblock->nPeriodStartTime), sizeof(pblock->nPeriodStartTime));//起始时间，8Byte
	hashoperator.Write((unsigned char*)&(pblock->nPeriodCount), sizeof(pblock->nPeriodCount));//会议总人数，4Byte
	hashoperator.Write((unsigned char*)&(pblock->nTimePeriod), sizeof(pblock->nTimePeriod));//打块人序号，4Byte
	hashoperator.Write((unsigned char*)&(pblock->nTime), sizeof(pblock->nTime));//实际打块时间，4Byte
	hashoperator.Write((unsigned char*)&nfee, sizeof(nfee));//总费用，8Byte
	hashoperator.Write((unsigned char*)&blockHeight, sizeof(blockHeight));//总费用，8Byte
	hashoperator.Write((unsigned char*)pblock->GetBlockHeader().hashPrevBlock.begin(), 32);//前置块hash， 256bit
	hashoperator.Finalize(hash.begin());

	unsigned char dataBuff[10240];
	int datalen = 0;
	memcpy(dataBuff, (unsigned char*)&(pblock->nPeriodStartTime), sizeof(pblock->nPeriodStartTime));
	datalen += sizeof(pblock->nPeriodStartTime);
	memcpy(dataBuff + datalen, (unsigned char*)&(pblock->nPeriodCount), sizeof(pblock->nPeriodCount));
	datalen += sizeof(pblock->nPeriodCount);
	memcpy(dataBuff + datalen, (unsigned char*)&(pblock->nTimePeriod), sizeof(pblock->nTimePeriod));
	datalen += sizeof(pblock->nTimePeriod);
	memcpy(dataBuff + datalen, (unsigned char*)&(pblock->nTime), sizeof(pblock->nTime));
	datalen += sizeof(pblock->nTime);
	memcpy(dataBuff + datalen, (unsigned char*)&nfee, sizeof(nfee));
	datalen += sizeof(nfee);
	memcpy(dataBuff + datalen, (unsigned char*)&blockHeight, sizeof(blockHeight));
	datalen += sizeof(blockHeight);
	memcpy(dataBuff + datalen, (unsigned char*)pblock->GetBlockHeader().hashPrevBlock.begin(), 32);//256 bits
	//datalen += 32;
	//std::string datahex;
	for (int i = 0; i < datalen; i++)
	{
		char tmp[3];
		sprintf(tmp, "%02x", dataBuff[i]);
		datahex = datahex + std::string(tmp);
	}
	//LogPrintf("[CConsensusAccountPool::verifyBlockSign] 验签明文=%s , 明文HASH=%s , 公钥HASH=%s\n", datahex.c_str(), hash.GetHex().c_str(), pubicKey160hash.GetHex().c_str());

	if (recvPublickey.Verify(hash, recvSign)) {
		//LogPrintf("[CConsensusAccountPool::verifyBlockSign]  recvPublickey.Verify  recvSign is OK\n");
	}
	else {
	//	LogPrintf("[CConsensusAccountPool::verifyBlockSign] 块头信息校验失败！该块为伪造，无法验证身份，无法处罚\n");
		return false;
	}
	*/
	return true;
}

void CConsensusAccountPool::getTrustList(std::vector<std::string> &trustList)
{
	std::copy(trustPKHashList.begin(), trustPKHashList.end(), std::back_inserter(trustList));
}