
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
#include "DpocInfo.h"
#include "DpocMining.h"
#include "TimeService.h"
#include "SerializeDpoc.h"
#include <boost/filesystem.hpp>
#include "wallet/wallet.h"

extern CWallet* pwalletMain;
boost::shared_mutex checkmutex;

SnapshotClass::SnapshotClass() {
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
	timestamp = in.timestamp;
	blockHeight = in.blockHeight;
	meetingstarttime = in.meetingstarttime;
	meetingstoptime = in.meetingstoptime;
	blockTime = in.blockTime;
	pkHashIndex = in.pkHashIndex;
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
	if (g_bStdCout)
	{
		std::cout << "--------------------list--------------------" << std::endl;
	}
	LogPrintf("--------------------list--------------------\n");
	std::set<uint16_t>::iterator it;
	for (it = list.begin(); it != list.end(); it++) {
		if (*it == 0)
		{
			continue;
		}
		if (g_bStdCout)
		{
			if (g_bStdCout)
			{
				std::cout << *it << " ";
			}
		}
		LogPrintf("%d	", *it);
	}
	if (g_bStdCout)
	{
		std::cout << std::endl;
	}
	LogPrintf("\n");
}

void CConsensusAccountPool::printsnapshots(std::vector<SnapshotClass> list)
{
    if(!fPrintToDebugLog)
        return;
	if (g_bStdCout)
	{
		std::cout << "--------------------check snapshots--------------------" << std::endl;
	}
	std::set<uint16_t>::iterator it;
	std::map<uint16_t, int>::iterator mapit;
	std::vector<std::pair<uint16_t, int64_t>>::iterator meetingit;
	
	if (g_bStdCout)
	{
		std::cout << "Current Candidate Index List:" << std::endl;
	}
	LogPrintf("Current Candidate Index List:\n");
	for (int i=0; i<list.size(); i++)
	{
		if (g_bStdCout)
		{
			std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		}
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).curCandidateIndexList.begin(); it != list.at(i).curCandidateIndexList.end(); it++) {
			if (g_bStdCout)
			{
				std::cout << *it << " ";
			}
			LogPrintf("%d	", *it);
		}
		if (g_bStdCout)
		{
			std::cout << std::endl;
		}
		LogPrintf("\n");
	}
	if (g_bStdCout)
	{
		std::cout << std::endl;
	}
	LogPrintf("\n");

	if (g_bStdCout)
	{
		std::cout << "This Loop‘s Miners Index List:" << std::endl;
	}
	LogPrintf("This Loop‘s Miners Index List:\n");
	for (int i = 0; i < list.size(); i++)
	{
		if (g_bStdCout)
		{
			std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		}
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (meetingit = list.at(i).cachedMeetingAccounts.begin(); meetingit != list.at(i).cachedMeetingAccounts.end(); meetingit++) {
			if (g_bStdCout)
			{
				std::cout << (*meetingit).first << "-" << (*meetingit).second << " ";

			}
			LogPrintf("%d-%d	", (*meetingit).first, (*meetingit).second);
		}
		if (g_bStdCout)
		{
			std::cout << std::endl;
		}
		LogPrintf("\n");
	}
	if (g_bStdCout)
	{
		std::cout << std::endl;
	}
	LogPrintf("\n");

	if (g_bStdCout)
	{
		std::cout << "New Refund Added:" << std::endl;
	}
	LogPrintf("New Refund Added:\n");
	for (int i = 0; i < list.size(); i++)
	{
		if (g_bStdCout)
		{
			std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		}
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).curRefundIndexList.begin(); it != list.at(i).curRefundIndexList.end(); it++) {
			if (g_bStdCout)
			{
				std::cout << *it << " ";
			}
			LogPrintf("%d	", *it);
		}
		if (g_bStdCout)
		{
			std::cout << std::endl;
		}
		LogPrintf("\n");
	}
	if (g_bStdCout)
	{
		std::cout << std::endl;
	}
	LogPrintf("\n");
	
	if (g_bStdCout)
	{
		std::cout << "Cached Candidate Index To Refund List:" << std::endl;
	}
	LogPrintf("Cached Candidate Index To Refund List:\n");
	for (int i = 0; i < list.size(); i++)
	{
		if (g_bStdCout)
		{
			std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		}
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).cachedIndexsToRefund.begin(); it != list.at(i).cachedIndexsToRefund.end(); it++) {
			if (g_bStdCout)
			{
				std::cout << *it << " ";
			}
			LogPrintf("%d	", *it);
		}
		if (g_bStdCout)
		{
			std::cout << std::endl;
		}
		LogPrintf("\n");
	}
	if (g_bStdCout)
	{
		std::cout << std::endl;
	}
	LogPrintf("\n");

	if (g_bStdCout)
	{
		std::cout << "Current Timeout Index List:" << std::endl;
	}
	LogPrintf("Current Timeout Index List:\n");
	for (int i = 0; i < list.size(); i++)
	{
		if (g_bStdCout)
		{
			std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		}
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (mapit = list.at(i).curTimeoutIndexRecord.begin(); mapit != list.at(i).curTimeoutIndexRecord.end(); mapit++) {
			if (g_bStdCout)
			{
				std::cout << mapit->first << "-" << mapit->second << " ";
			}
			LogPrintf("%d-%d	", mapit->first, mapit->second);
		}
		if (g_bStdCout)
		{
			std::cout << std::endl;
		}
		LogPrintf("\n");
	}
	if (g_bStdCout)
	{
		std::cout << std::endl;
	}
	LogPrintf("\n");

	if (g_bStdCout)
	{
		std::cout << "New Timeout Punishment Added:" << std::endl;
	}
	LogPrintf("New Timeout Punishment Added:\n");
	for (int i = 0; i < list.size(); i++)
	{
		if (g_bStdCout)
		{
			std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		}
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).curTimeoutPunishList.begin(); it != list.at(i).curTimeoutPunishList.end(); it++) {
			if (g_bStdCout)
			{
				std::cout << *it << " ";
			}
			LogPrintf("%d	", *it);
		}
		if (g_bStdCout)
		{
			std::cout << std::endl;
		}
		LogPrintf("\n");
	}
	if (g_bStdCout)
	{
		std::cout << std::endl;
	}
	LogPrintf("\n");

	if (g_bStdCout)
	{
		std::cout << "Cached Candidate Index To Timeout Punishment List:" << std::endl;
	}
	LogPrintf("Cached Candidate Index To Timeout Punishment List:\n");
	for (int i = 0; i < list.size(); i++)
	{
		if (g_bStdCout)
		{
			std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		}
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).cachedTimeoutPunishToRun.begin(); it != list.at(i).cachedTimeoutPunishToRun.end(); it++) {
			if (g_bStdCout)
			{
				std::cout << *it << " ";
			}
			LogPrintf("%d	", *it);
		}
		if (g_bStdCout)
		{
			std::cout << std::endl;
		}
		LogPrintf("\n");
	}
	if (g_bStdCout)
	{
		std::cout << std::endl;
	}
	LogPrintf("\n");

	if (g_bStdCout)
	{
		std::cout << "New Ban:" << std::endl;
	}
	LogPrintf("New Ban :\n");
	for (int i = 0; i < list.size(); i++)
	{
		if (g_bStdCout)
		{
			std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		}
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).curBanList.begin(); it != list.at(i).curBanList.end(); it++) {
			if (g_bStdCout)
			{
				std::cout << *it << " ";
			}
			LogPrintf("%d	", *it);
		}
		if (g_bStdCout)
		{
			std::cout << std::endl;
		}
		LogPrintf("\n");
	}
	if (g_bStdCout)
	{
		std::cout << std::endl;
	}
	LogPrintf("\n");

	if (g_bStdCout)
	{
		std::cout << "Cached Ban List:" << std::endl;
	}
	LogPrintf("Cached Ban List:\n");
	for (int i = 0; i < list.size(); i++)
	{
		if (g_bStdCout)
		{
			std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
		}
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		for (it = list.at(i).cachedBanList.begin(); it != list.at(i).cachedBanList.end(); it++) {
			if (g_bStdCout)
			{
				std::cout << *it << " ";
			}
			LogPrintf("%d	", *it);
		}
		if (g_bStdCout)
		{
			std::cout << std::endl;
		}
		LogPrintf("\n");
	}
	if (g_bStdCout)
	{
		std::cout << std::endl;
	}
	LogPrintf("\n");

	if (g_bStdCout)
	{
		std::cout << "Meeting Time:" << std::endl;
	}
	LogPrintf("Meeting Time:\n");
	for (int i = 0; i < list.size(); i++)
	{
		if (g_bStdCout)
		{
			std::cout << list.at(i).blockHeight << " | " << list.at(i).pkHashIndex << " 	| " << list.at(i).blockTime << " | ";
			std::cout << list.at(i).meetingstarttime << "  " << list.at(i).meetingstoptime;
		}
		LogPrintf("%d	|%03d	|%d	|", list.at(i).blockHeight, list.at(i).pkHashIndex, list.at(i).blockTime);
		LogPrintf("%u	%u", 
			list.at(i).meetingstarttime, list.at(i).meetingstoptime);

		if (g_bStdCout)
		{
			std::cout << std::endl;
		}
		LogPrintf("\n");
	}
	if (g_bStdCout)
	{
		std::cout << std::endl;
		std::cout << "--------------------finish--------------------" << std::endl;
		std::cout << std::endl;
	}
	LogPrintf("--------------------finish--------------------\n\n");

}

CConsensusAccountPool*  CConsensusAccountPool::_instance = nullptr;
std::once_flag CConsensusAccountPool::init_flag;

CConsensusAccountPool::CConsensusAccountPool()
{
	snapshotlist.clear();
	m_mapSnapshotIndex.clear();
	candidatelist.clear();
	tmpcachedIndexsToRefund.clear();
	tmpcachedTimeoutToPunish.clear();
	verifysuccessed = false;
	analysisfinished = false;
	m_strSnapshotPath = std::string("Snapshot");
	m_strSnapshotIndexPath = std::string("SnapshotIndex");
	m_strSnapshotRevPath = std::string("SnapshotRev");
	m_strCandidatelistPath = std::string("Candidatelist");
	m_u32WriteHeight = 0;

	bReboot = false;

	
	if(Params().NetworkIDString() == CBaseChainParams::MAIN)
	{
		trustPKHashList.push_back("1f324deede30e9f3ae61addbcaed3e2246ed9e77");
		trustPKHashList.push_back("ac4b6c5565ec567102cc666f8efef635546b3a3c");
		trustPKHashList.push_back("24edef7392b8264db505aa709161a9b733cf1153");
		trustPKHashList.push_back("2656438e6581fb298231b398c54b090fa1e7765f");
		trustPKHashList.push_back("55ca5cf8e8381e11e3fb155faf546b60cf966a81");

		trustPKHashList.push_back("8f13e80390ce115e6523b80ebab2d07065c40202");
		trustPKHashList.push_back("03cc22fa2ff3ab748a90c2478d3267e9869e8969");
		trustPKHashList.push_back("9afc6a2110620348b4f427e6c20609e14234ad18");
		trustPKHashList.push_back("115cfeac3fd7c1a0288c771cada4831d77a7b11b");
		trustPKHashList.push_back("f76dfdccd2487d60f449a7676a67eacd5adefe92");

		trustPKHashList.push_back("3bfb0bfc964a138b2f68133911aa5cdabfc5942c");
		trustPKHashList.push_back("19fa2c073142290358243a1c244f170bbbe2aea5");
		trustPKHashList.push_back("9afdfb65154bb930045a7be2c1fedfb957de69f3");
		trustPKHashList.push_back("a7b0231277f4e9e3b8d48efb417ddf5203a9c86a");
		trustPKHashList.push_back("2426551e1fd1ff6d440a8bfdb519803f53175696");

		trustPKHashList.push_back("f057fed2179897f1dc21e3078882d719c543cbe5");
		trustPKHashList.push_back("7ee049deb78f0768cf13ebc6560a5eb1fd98f509");
		trustPKHashList.push_back("6d19330318e9f5a88175fadff7abd07064c149bc");
		trustPKHashList.push_back("37b3b9813f91ccdb293270c78dc1ddf71dd7d95b");
		trustPKHashList.push_back("c2b862e0809808dc97efda21c1e2bca62b1b4f26");
	}
	else if (Params().NetworkIDString() == CBaseChainParams::TESTNET)
	{
		//TestChain
		trustPKHashList.push_back("e34450364c0b7569162998f7c0082f880df76946");
		trustPKHashList.push_back("e16d04b1cf614a3c897c30ed97fd21f11b017161");
		trustPKHashList.push_back("c29622b296bcf805f37846c6318f6f3425742063");
		trustPKHashList.push_back("942b392a5564796039990752062a91b6487e6c96");
		trustPKHashList.push_back("6e131a09afa65805f736c084ef79e9be7d413556");

		trustPKHashList.push_back("40b27001c168cb8e5dfbb7999340d930eb8369d5");
		trustPKHashList.push_back("c725507037d6a7a3e0bb7dab582670abd68c0559");
		trustPKHashList.push_back("75e963dca9a292607c26c3d6a04d218ed986ec1b");
		trustPKHashList.push_back("01015f28af241b0493ed8aa50a91aed48abb0c3b");
		trustPKHashList.push_back("cf96d36126e6962769e1883d20465e4b4ceaab85");

		trustPKHashList.push_back("48b8e2475b4d949254af74d257afb9b35fdb9bb4");
		trustPKHashList.push_back("1e5f5feb1590609cf1f4dd2ca134449a201140e6");
		trustPKHashList.push_back("02bef0749efd62e672a82188e1ddeb78b7ad417f");
		trustPKHashList.push_back("9e2f2490fbce7d073c98031af5a62c7ed6dfa281");
		trustPKHashList.push_back("d58124f372db3a195a85eb47f35c50ca31fb8b46");

		trustPKHashList.push_back("5db186c3b09c1139d99dc7a8fe1d2cec436c036f");
		trustPKHashList.push_back("923cd7662c415bcc852fbecd0eb550a8192ea432");
		trustPKHashList.push_back("ea565638fb61dcbb0e5f4f1b7384044b17431674");
		trustPKHashList.push_back("7852d6b46fdfbb1a879057ead698190695944757");
		trustPKHashList.push_back("2358401fd6ec7774033253684c85bd3e2d188f66");
	}

	m_u64SnapshotIndexSize = getCSnapshotIndexSize();
	m_u64ConsensAccountSize = getConsensAccountSize();
}

 CConsensusAccountPool::~CConsensusAccountPool()
{
}

void CConsensusAccountPool::CreateInstance() {
	 static CConsensusAccountPool instance;
	 CConsensusAccountPool::_instance = &instance;
 }

CConsensusAccountPool&  CConsensusAccountPool::Instance()
{
	std::call_once(CConsensusAccountPool::init_flag, CConsensusAccountPool::CreateInstance);
	return *CConsensusAccountPool::_instance;
}

bool CConsensusAccountPool::getPublicKeyFromBlock(const CBlock *pblock, CPubKey& outPubkey,std::vector<unsigned char>&  outRecvSign)
{
	CTransactionRef coinbase = pblock->vtx[0];
	std::string strSignatrue = coinbase->vout[0].GetCheckBlockContent();
	if (strSignatrue.length() < 50) {
		return  false;
	}

	std::vector<unsigned char> vchReceive;
	if (!DecodeBase58(strSignatrue, vchReceive)) {
		if (g_bStdCout)
		{
			std::cout << "[getPublicKeyFromBlock] DecodeBase58  vchReceive is err =" << strSignatrue.length() << std::endl;
		}
		return false;
	}

	uint32_t  siglen = vchReceive[0];
	uint32_t  sigPubkey = vchReceive[1 + siglen];

	//Get the signature
	std::vector<unsigned char>  recvSign;
	recvSign.resize(siglen);
	for (std::vector<unsigned char>::size_type ix = 0; ix < siglen; ++ix) {
		recvSign[ix] = vchReceive[ix + 1];
	}
	
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
		if (g_bStdCout)
		{
			std::cout << "[getPublicKeyFromSignstring] DecodeBase58  vchReceive is err ===" << signstr.length() << std::endl;
		}
		return false;
	}

	uint32_t  siglen = vchReceive[0];
	uint32_t  sigPubkey = vchReceive[1 + siglen];

	std::vector<unsigned char>  recvSign;
	recvSign.resize(siglen);
	for (std::vector<unsigned char>::size_type ix = 0; ix < siglen; ++ix) {
		recvSign[ix] = vchReceive[ix + 1];
	}

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
		if (g_bStdCout)
		{
			std::cout << "[CConsensusAccountPool::verifyDPOCTx] Get the last snapshot to fail\n";
		}
		LogPrintf("[CConsensusAccountPool::verifyDPOCTx] Get the last snapshot to fail\n");
		errorType = GET_LAST_SNAPSHOT_FAILED;
		return false;
	}

	if (tx.GetCampaignType() == TYPE_CONSENSUS_REGISTER)
	{
		//Apply to join the transaction to get the public key HASH value
		uint160 curHash = tx.GetCampaignPublickey();
		CAmount ipcvalue = tx.GetRegisterIPC();

		//If the transaction amount is not sufficient, it is rejected
		if (ipcvalue < Params().MIN_DEPOSI)
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] The deposit %d Insufficient, set value %d\n", ipcvalue, Params().MIN_DEPOSI);
			errorType = JOIN_TRANS_DEPOSI_TOO_SMALL;
			return false;
		}

		//Look for subscript ID
		uint16_t pkhashindex;
		if (!ContainPK(curHash, pkhashindex))
		{
			//The candidate list does not include the hash of the campaign, and returns true directly
			return true;
		}

		//If you are already in the current list of candidates, you are not allowed to join again
		//If a refund is being processed, the refund transaction cannot be rejoined
		if (lastsnapshot.curCandidateIndexList.count(pkhashindex))
		{
			std::set<uint16_t>::iterator iter = lastsnapshot.curCandidateIndexList.begin();
			for (;iter!= lastsnapshot.curCandidateIndexList.end();++iter)
			{
				LogPrintf("[CConsensusAccountPool::verifyDPOCTx] publickey HASH %d",*iter);
			}
			
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] The public key HASH is already in the current candidate list %d++%d\n", pkhashindex,lastsnapshot.blockHeight);
			errorType = JOIN_PUBKEY_ALREADY_EXIST_IN_LIST;
			return false;
		}
		else if (lastsnapshot.curBanList.count(pkhashindex) ||
			lastsnapshot.cachedBanList.count(pkhashindex))
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] The current public key HASH is in the blacklist list\n");
			errorType = JOIN_PUBKEY_IS_BANNED;
			return false;
		}
		else if (lastsnapshot.cachedTimeoutPunishToRun.count(pkhashindex) ||
			lastsnapshot.curTimeoutPunishList.count(pkhashindex))
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] The current public key HASH is being punished with a timeout\n");
			errorType = JOIN_PUBKEY_IS_TIMEOUT_PUNISHED;
			return false;
		}
		else if (lastsnapshot.cachedIndexsToRefund.count(pkhashindex) ||
			lastsnapshot.curRefundIndexList.count(pkhashindex))
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] The current public key HASH is performing a refund operation\n");
			errorType = JOIN_PUBKEY_IS_DEPOSING;
			return false;
		}
		else
		{
			//If a refund is to be performed in a snapshot of the cache block length, it is not allowed to join again
			uint32_t cachedHeight = lastsnapshot.blockHeight - CACHED_BLOCK_COUNT;
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] Prepare to get the block height between %d-%d cache\n", cachedHeight, lastsnapshot.blockHeight);
			std::vector<SnapshotClass> cachedSnapshotList;
			if (!GetSnapshotsByHeight(cachedSnapshotList, cachedHeight))
			{
				LogPrintf("[CConsensusAccountPool::verifyDPOCTx] The list of cached snapshots failed！\n");
				errorType = GET_CACHED_SNAPSHOT_FAILED;
				return false;
			}

			std::vector<SnapshotClass>::iterator pSnapshot;

			for (pSnapshot = cachedSnapshotList.begin(); pSnapshot != cachedSnapshotList.end(); pSnapshot++)
			{
				if ((*pSnapshot).curTimeoutPunishList.count(pkhashindex))
				{
					LogPrintf("[CConsensusAccountPool::verifyDPOCTx] The current public key HASH is being punished with a timeout\n");
					errorType = EXIT_PUBKEY_IS_TIMEOUT_PUNISHED;
					return false;
				}
				if ((*pSnapshot).curRefundIndexList.count(pkhashindex))
				{
					LogPrintf("[CConsensusAccountPool::verifyDPOCTx] The current public key HASH is performing a refund operation\n");
					errorType = EXIT_PUBKEY_IS_DEPOSING;
					return false;
				}
			}
		}
	}
	else if (tx.GetCampaignType() == TYPE_CONSENSUS_QUITE)
	{
		if (g_bStdCout)
		{
			std::cout << "verify exit campaign" << std::endl;
		}

		uint160 curHash = tx.GetCampaignPublickey();
		uint16_t pkhashindex;
		if (!ContainPK(curHash, pkhashindex))
		{
			//The record doesn't have this public key hash, obviously you can't get out of it
			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] The public key HASH is not in the index vector\n");
			errorType = EXIT_UNKNOWN_PUBKEY;
			return false;
		}

		//If not in the current list of candidates, do not let exit
		if (!lastsnapshot.curCandidateIndexList.count(pkhashindex))
		{
			printsets(lastsnapshot.curCandidateIndexList);

			LogPrintf("[CConsensusAccountPool::verifyDPOCTx] The public key HASH is not in the current candidate list\n");
			errorType = EXIT_PUBKEY_NOT_EXIST_IN_LIST;
			return false;
		}
	}

	return true;
}

bool  CConsensusAccountPool::checkNewBlock(const std::shared_ptr<const CBlock> pblock, uint160 &pubkeyHash, uint32_t blockHeight, DPOC_errtype &errorType)
{
	LogPrintf("[CConsensusAccountPool::checkNewBlock] INBLOCK nPeriodStartTime %d, nPeriodCount %d, nTimePeriod %d, blockTime %d height=%d\n",
		pblock->nPeriodStartTime, pblock->nPeriodCount, pblock->nTimePeriod, pblock->nTime, blockHeight);

	SnapshotClass lastsnapshot;
	if (!GetLastSnapshot(lastsnapshot))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] Gets a snapshot to record the tail block failed\n");
		return false;
	}

	if (blockHeight > lastsnapshot.blockHeight +1)
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] The incoming block is too new to no verify\n");
		errorType = BLOCK_TOO_NEW_FOR_SNAPSHOT;
		return false;
	}

	//If the current public key is on the blacklist, reject the block
	uint16_t pkindex;
	if (!ContainPK(pubkeyHash, pkindex))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] Packaging public key unknown, reject\n");
		errorType = UNKNOWN_PACKAGE_PUBKEY;
		return false;
	}

	//Time to check
	int64_t calcTime = (pblock->nPeriodStartTime + (pblock->nTimePeriod + 1) * BLOCK_GEN_TIME) / 1000;
	LogPrintf("[CConsensusAccountPool::checkNewBlock] The current calculation should be packaged time=%d, The last block in the snapshot should be packaged Time=%d\n", calcTime, lastsnapshot.timestamp);
	if (calcTime < (lastsnapshot.timestamp + BLOCK_GEN_TIME/1000 ))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] The last block gap in the current calculation is less than the last one in the snapshot %d ms，Continuous block, severe punishment！\n", BLOCK_GEN_TIME);
		return false;
	}

	if (pblock->nTime != calcTime) {
		LogPrintf("[CConsensusAccountPool::checkNewBlock]  pblock->nTime != calcTime\n");
		
		if ((pblock->nTime - calcTime < 0) &&(lastsnapshot.blockHeight >= Params().CHECK_START_BLOCKCOUNT))
		{
			LogPrintf("[CConsensusAccountPool::checkNewBlock] Packaging time is earlier than computation time，Or the time difference between the package time and the computation time is greater than %d seconds\n", MAX_BLOCK_TIME_DIFF);
			errorType = PUNISH_BLOCK;
			return false;
		}
		else if ((pblock->nTime - calcTime > MAX_BLOCK_TIME_DIFF) &&(lastsnapshot.blockHeight >= Params().CHECK_START_BLOCKCOUNT))
		{   //It's possible to have a block delay
			LogPrintf("[CConsensusAccountPool::checkNewBlock] The packaging time is earlier than the calculation time, or the time difference between the packaging time and the computation time is greater than %d seconds\n", MAX_BLOCK_TIME_DIFF);
			return false;
		}
	}
	
	//Look for a snapshot of the current session's start time
	int64_t curStarttime = pblock->nPeriodStartTime / 1000;
	SnapshotClass meetingStartSnapshot;
	if (!GetSnapshotByTime(meetingStartSnapshot, curStarttime))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] A snapshot of the start time of this session failed\n");
		return false;
	}

	LogPrintf("[CConsensusAccountPool::checkNewBlock] current start time %d ,block height in snapshot %d\n",
		curStarttime, meetingStartSnapshot.blockHeight);

	if (meetingStartSnapshot.blockHeight == 0 && meetingStartSnapshot.timestamp > curStarttime)
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] The current block time is earlier than the creation block, error！\n");
		errorType = PUNISH_BLOCK;
		return false;
	}

	if (!bReboot)
	{
		//the time doesn't match, it could be the missing block or the wrong block
		if (meetingStartSnapshot.timestamp != curStarttime
			&& meetingStartSnapshot.blockHeight >= Params().CHECK_START_BLOCKCOUNT)
		{
			LogPrintf("[CConsensusAccountPool::checkNewBlock] The current block start time is not consistent with the snapshot packaging time found at that time，You need to verify that the block is missing。curstarttime=%d，snapshot time=%d\n",
				curStarttime, meetingStartSnapshot.timestamp);

			//Case handling: ignore the possibility of missing a full round
			LogPrintf("[CConsensusAccountPool::checkNewBlock] Snapshot meeting starttime=%d, endtime=%d\n",
				meetingStartSnapshot.meetingstarttime, meetingStartSnapshot.meetingstoptime);

			//Determines whether the end time of the last block found is the start time of the current block
			//If it is, the block that is missing in the middle is cross-round, and the block is still valid
			if (meetingStartSnapshot.meetingstoptime != pblock->nPeriodStartTime)
			{
				LogPrintf("[CConsensusAccountPool::checkNewBlock] The end of the session of the snapshot does not equal the start time of the current block，There may be more than one round，error\n");
				return false;
			}
			LogPrintf("[CConsensusAccountPool::checkNewBlock] The end of the session of the snapshot is equal to the starting time of the current block meeting, with the phenomenon of cross-wheel leakage\n");
		}
	}
	else
	{
		bReboot = false;
	}

	//find the cache block
	uint64_t meetingstarttime = meetingStartSnapshot.meetingstoptime;
	int foundcachedcount = CACHED_BLOCK_COUNT;
	if (meetingStartSnapshot.blockHeight < Params().CHECK_START_BLOCKCOUNT)
	{
		foundcachedcount = meetingStartSnapshot.blockHeight < CACHED_BLOCK_COUNT ? meetingStartSnapshot.blockHeight : CACHED_BLOCK_COUNT;
	}
	uint32_t cachedTime = (meetingStartSnapshot.meetingstoptime - foundcachedcount * BLOCK_GEN_TIME) / 1000;
	LogPrintf("[CConsensusAccountPool::checkNewBlock] Calculate the cache time is %d \n", cachedTime);
	
	//The consensus of this round
	SnapshotClass cachedsnapshot;
	if (!GetSnapshotByTime(cachedsnapshot, cachedTime))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] The cache time corresponds to a snapshot failure, and the block information is problematic\n");
		return false;
	}
	
	LogPrintf("[CConsensusAccountPool::checkNewBlock] find block height = %d , find block time= %d \n",
		cachedsnapshot.blockHeight, cachedsnapshot.timestamp);
	
	std::set<uint16_t> accounts = cachedsnapshot.curCandidateIndexList;
	
	printsets(accounts);
	if (accounts.size() != pblock->nPeriodCount && cachedsnapshot.curCandidateIndexList.size() != pblock->nPeriodCount)
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] The number of meetings recorded in the block is inconsistent with the number of meetings in the snapshot,PeriodCount=%d,cachedsnapshot PeriodCount=%d，The number of people removed from the blacklist=%d \n",
			pblock->nPeriodCount,cachedsnapshot.curCandidateIndexList.size(), accounts.size());
		if (Params().NetworkIDString() == CBaseChainParams::TESTNET)
		{
			if (blockHeight >= 32050 && blockHeight <= 32070)
			{
				LogPrintf("[CConsensusAccountPool::checkNewBlock] ignore the blockheight %d check \n",blockHeight);
			}
			else{
				return false;
			}
		}
		else
			return false;
	}
	
	std::list<std::shared_ptr<CConsensusAccount >> consensusList;
	consensusList.clear();
	std::set<uint16_t>::iterator candidateit;
	for (candidateit= accounts.begin(); candidateit != accounts.end(); candidateit++)
	{
		std::shared_ptr<CConsensusAccount> tmpaccount = std::make_shared<CConsensusAccount>(candidatelist.at(*candidateit));
		consensusList.push_back(tmpaccount);
	}

	bool result = CDpocMining::Instance().GetMeetingList(meetingstarttime, consensusList);
	if (result)
	{
		//Print the public key HASH index value of this round  sequentially
		LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] startime %d meeting pockager sort：\n", pblock->nPeriodStartTime);
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
			LogPrintf("[CConsensusAccountPool::checkNewBlock] The current block public key failed from the sorting list and block ordinal\n");
			return false;
		}

		if (pubkeyHash != correctpubkeyhash)
		{
			LogPrintf("[CConsensusAccountPool::checkNewBlock] packager public key HASH don't match，in block PKHash=%s，snapshot PKHash=%s\n",
				pubkeyHash.GetHex().c_str(), correctpubkeyhash.GetHex().c_str());
			return false;
		}
	}
	else
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] Get a list of meeting collation failures\n");
		return false;
	}

	//Check whether the HASH is consistent with the front-block HASH corresponding to the tail of the snapshot
	if (!mapBlockIndex.count(pblock->GetBlockHeader().hashPrevBlock))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] There is no prefix HASH for this block in the current list %s \n", pblock->GetBlockHeader().hashPrevBlock.GetHex().c_str());

		return false;
	}
	CBlockIndex* pblockindex = mapBlockIndex[pblock->GetBlockHeader().hashPrevBlock];
	if (pblockindex->nHeight != lastsnapshot.blockHeight)
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] this block prevHASH %s ,prevblock height %d, snapshot tail height %d ,no match\n",
			pblock->GetBlockHeader().hashPrevBlock.GetHex().c_str(), pblockindex->nHeight, lastsnapshot.blockHeight);
		
		return false;
	}

	if (g_bStdCout)
	{
		std::cout << "checkNewBlock exit" << std::endl;
	}
	return true;
}

bool CConsensusAccountPool::verifyDPOCBlock(const std::shared_ptr<const CBlock> pblock, uint32_t blockHeight, DPOC_errtype &rejectreason)
{
	writeLock wtlock(checkmutex);

	CPubKey recvPublickey;
	std::vector<unsigned char>  recvSign;
	if (!getPublicKeyFromBlock(pblock.get(), recvPublickey, recvSign)) {
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock()] getPublicKeyFromBlock is badlly is MisbeHaving\n");

		rejectreason = PUNISH_BLOCK;
		return false;
	}
	CKeyID  pubicKey160hash = recvPublickey.GetID();

	//time check。testnet The first 120 blocks are slightly over, and the main network is checked from the first block
	if (blockHeight >= Params().CHECK_START_BLOCKCOUNT)
	{
		//Verify that the hash in the signature is in the consensus list of the block
		if (!verifyPkInCandidateList(pblock, pubicKey160hash))
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCBlock()] The hash in the signature is not in the consensus list of the block \n");
			return false;
		}

		uint256 hash;
		CAmount nfee = pblock->vtx[0]->GetValueOut();
		CHash256 hashoperator;

		hashoperator.Write((unsigned char*)&(pblock->nPeriodStartTime), sizeof(pblock->nPeriodStartTime));//8Byte
		hashoperator.Write((unsigned char*)&(pblock->nPeriodCount), sizeof(pblock->nPeriodCount));//4Byte
		hashoperator.Write((unsigned char*)&(pblock->nTimePeriod), sizeof(pblock->nTimePeriod));//4Byte
		hashoperator.Write((unsigned char*)&(pblock->nTime), sizeof(pblock->nTime));//4Byte
		hashoperator.Write((unsigned char*)&nfee, sizeof(nfee));//8Byte
		hashoperator.Write((unsigned char*)pblock->GetBlockHeader().hashPrevBlock.begin(), 32);//prevhash， 256bit
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
		memcpy(dataBuff + datalen, (unsigned char*)pblock->GetBlockHeader().hashPrevBlock.begin(), 32);//256 bits
		datalen += 32;
		std::string datahex;
		for (int i = 0; i < datalen; i++)
		{
			char tmp[3];
			sprintf(tmp, "%02x", dataBuff[i]);
			datahex = datahex + std::string(tmp);
		}
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock()] clear=%s , clear HASH=%s , publickey HASH=%s\n", datahex.c_str(), hash.GetHex().c_str(), pubicKey160hash.GetHex().c_str());

		if (recvPublickey.Verify(hash, recvSign)) {
			LogPrintf("[CConsensusAccountPool::verifyDPOCBlock()]  recvPublickey.Verify  recvSign is OK\n");
		}
		else 
		{
			if (g_bStdCout)
			{
				std::cout << "CConsensusAccountPool::verifyDPOCBlock(): Verify block sign failed" << std::endl;
			}
			LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] block head info check faile！The block is forged and unable to verify identity\n");
			
			rejectreason = PUNISH_BLOCK;
			return false;
		}

		//reboot
		if (REBOOTLABEL == pblock->nNonce)
		{
			if (verifyPkIsTrustNode(pubicKey160hash))
			{
				bReboot = true;
			}
			else
			{
				return false;
			}
		}
		
		if (!checkNewBlock(pblock, pubicKey160hash, blockHeight, rejectreason)) {
			if (g_bStdCout)
			{
				std::cout << "Block Check Failed" << std::endl;
			}
			LogPrintf("[CConsensusAccountPool::verifyDPOCBlock]  checkNewBlock faile\n");
			return false;
		}
	}

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
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] get cache snapshot height is %d\n", cachedHeight);
		if (!GetSnapshotByHeight(cachedSnapshot, cachedHeight))
		{
			LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] to get cache snapshot faile\n");
			return false;
		}
		std::set<uint16_t>::iterator it;
		for (it = cachedSnapshot.curRefundIndexList.begin(); it != cachedSnapshot.curRefundIndexList.end(); it++)
		{
			if (!tmpcachedIndexsToRefund.count(*it))
				tmpcachedIndexsToRefund.insert(*it);
		}
		
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock]The currently saved temporary list of waiting refunds is：\n");
		printsets(tmpcachedIndexsToRefund);

		for (it = cachedSnapshot.curTimeoutPunishList.begin(); it != cachedSnapshot.curTimeoutPunishList.end(); it++)
		{
			if (!tmpcachedTimeoutToPunish.count(*it))
				tmpcachedTimeoutToPunish.insert(*it);
		}
		
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock]The temporary list of the currently saved prepared execution timeout penalties is：\n");
		printsets(tmpcachedTimeoutToPunish);

		for (const auto& tx : pblock->vtx)
		{
			if (tx->IsCoinBase())
			{
				LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] check Coinbase tx\n");
				
				for (const CTxOut& txout : tx->vout)
				{
					if (txout.GetCampaignType() == TYPE_CONSENSUS_RETURN_DEPOSI)
					{
						uint16_t pkindex;
						if (!ContainPK(txout.devoteLabel.hash, pkindex))
						{
							LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] The refund public key is not in the cache list\n");
							return false;
						}

						if (!tmpcachedIndexsToRefund.count(pkindex))
						{
							LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] There is no refund record in the current cache list\n");
							return false;
						}
					}
					else if (txout.GetCampaignType() == TYPE_CONSENSUS_ORDINARY_PUSNISHMENT)
					{
						uint16_t pkindex;
						if (!ContainPK(txout.devoteLabel.hash, pkindex))
						{
							LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] The penalty public key is not in the cache list\n");
							return false;
						}
						if (!tmpcachedTimeoutToPunish.count(pkindex))
						{
							LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] There is no penalty record in the current cache list\n");
							return false;
						}

					}
				}
			}
			
			if (tx->GetTxType() != 1)
				continue;

		
			DPOC_errtype errtype;
			if (!verifyDPOCTx(*tx, errtype))
			{
				LogPrintf("[CConsensusAccountPool::verifyDPOCBlock] verifyDPOCTx return false error type: %d\n", errtype);
				return false;
			}
		}	
	}
	else
	{
		LogPrintf("[CConsensusAccountPool::verifyDPOCBlock]A snapshot list may be empty if you fail at the end of the snapshot！\n");
	}
	
	verifysuccessed = true;
	if (g_bStdCout)
	{
		std::cout << "VerifyDPOCBlock(): success exit" << std::endl << std::endl;
	}

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
		LogPrintf("[CConsensusAccountPool::GetDepositBypkhash] The current account hash is not found in the list of candidates\n");
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
		LogPrintf("[CConsensusAccountPool::GetDepositBypkhash] A hash of the current account is not found in the candidate list\n");
		return hash;
	}

	return candidatelist[index].getTxhash();
}

bool CConsensusAccountPool::IsAviableUTXO(const uint256 hash)
{
	LogPrintf("[CConsensusAccountPool::IsAviableUTXO] get utxo state\n");
	SnapshotClass cursnapshot;
	if (!GetLastSnapshot(cursnapshot))
	{
		LogPrintf("[CConsensusAccountPool::checkNewBlock] Gets a snapshot to record the tail block failed\n");
		return false;
	}
	
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

	std::vector<SnapshotClass> cachedSnapshots;
	uint32_t cachedHeight = cursnapshot.blockHeight - CACHED_BLOCK_COUNT;
	LogPrintf("[CConsensusAccountPool::pushDPOCBlock] to get cachedHeight between %d-%d \n", cachedHeight, cursnapshot.blockHeight);
	if (!GetSnapshotsByHeight(cachedSnapshots, cachedHeight))
	{
		LogPrintf("[CConsensusAccountPool::pushDPOCBlock] to get the list of cached snapshots failed\n");
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

	return true;
}

bool CConsensusAccountPool::getCreditFromSnapshotByIndex(SnapshotClass snapshot, const uint16_t indexIn, int64_t &credit)
{
	LogPrintf("[CConsensusAccountPool::getCreditFromSnapshotByIndex] called\n");
	for (std::vector<std::pair<uint16_t, int64_t>>::iterator accountit = snapshot.cachedMeetingAccounts.begin();
		accountit != snapshot.cachedMeetingAccounts.end(); accountit ++)
	{
		if ( (*accountit).first == indexIn)
		{
			credit = (*accountit).second;
			LogPrintf("[CConsensusAccountPool::getCreditFromSnapshotByIndex] from snapshot get index=%d,credit=%d\n", indexIn, credit);
			return true;
		}
	}

	return false;
}

bool CConsensusAccountPool::pushDPOCBlock(const std::shared_ptr<const CBlock> pblock, uint32_t blockHeight)
{
	if (analysisfinished)
	{
		setTotalAmount(*pblock, true);
	}
	
	LogPrintf("[CConsensusAccountPool::pushDPOCBlock] IN block height= %d , timestamp= %d , realtime = %d , nPeriodStartTime= %d , accountNumber= %d , accountindex= %d\n",
		blockHeight, (pblock->nPeriodStartTime + (pblock->nTimePeriod + 1) * BLOCK_GEN_TIME) / 1000,
		pblock->nTime, pblock->nPeriodStartTime, pblock->nPeriodCount, pblock->nTimePeriod);

	SnapshotClass newsnapshot;
	newsnapshot.blockTime = pblock->nTime;
	newsnapshot.timestamp = (pblock->nPeriodStartTime + (pblock->nTimePeriod + 1) * BLOCK_GEN_TIME) / 1000;
	newsnapshot.meetingstarttime = pblock->nPeriodStartTime;
	newsnapshot.meetingstoptime = pblock->nPeriodStartTime + pblock->nPeriodCount * BLOCK_GEN_TIME;
	newsnapshot.blockHeight = blockHeight;

	LogPrintf("[CConsensusAccountPool::pushDPOCBlock] tmp snapshot height= %d , timestamp= %d , realtime = %d , meetingstarttime= %d , meetingstoptime= %d\n",
		newsnapshot.blockHeight, newsnapshot.timestamp, newsnapshot.blockTime,
		newsnapshot.meetingstarttime, newsnapshot.meetingstoptime);

	SnapshotClass lastSnapshot;
	if (!GetLastSnapshot(lastSnapshot))//generate block
	{
		uint160 MeetingHash;
		CAmount ipcvalue;
		MeetingHash.SetHex(trustPKHashList[0]);
		ipcvalue = Params().MIN_DEPOSI + CACHED_BLOCK_COUNT;
		candidatelist.push_back(CConsensusAccount(MeetingHash, ipcvalue));

		newsnapshot.pkHashIndex = 0;
		newsnapshot.curCandidateIndexList.insert(0);
		newsnapshot.cachedMeetingAccounts.push_back(std::make_pair(0, 0));

		newsnapshot.curTimeoutIndexRecord.clear();
		newsnapshot.cachedBanList.clear();
		newsnapshot.timestamp = newsnapshot.meetingstoptime / 1000;
	}
	else
	{
		newsnapshot.curCandidateIndexList = lastSnapshot.curCandidateIndexList;
		newsnapshot.cachedIndexsToRefund = lastSnapshot.cachedIndexsToRefund;
		newsnapshot.cachedTimeoutPunishToRun = lastSnapshot.cachedTimeoutPunishToRun;
		newsnapshot.curTimeoutIndexRecord = lastSnapshot.curTimeoutIndexRecord;
		newsnapshot.cachedBanList = lastSnapshot.cachedBanList;
		
		std::set<uint16_t>::iterator banit;
		for (banit = newsnapshot.cachedBanList.begin(); banit != newsnapshot.cachedBanList.end(); banit++)
		{
			if (newsnapshot.cachedIndexsToRefund.count(*banit))
			{
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] Remove the blacklist included in the refund list index=%d\n",*banit);
				newsnapshot.cachedIndexsToRefund.erase(*banit);
			}
		}

		if (verifysuccessed)
		{
			tmpcachedIndexsToRefund.clear();
			tmpcachedTimeoutToPunish.clear();
			verifysuccessed = false;

			SnapshotClass lastSnapshot;
			if (GetLastSnapshot(lastSnapshot))
			{
				tmpcachedIndexsToRefund = lastSnapshot.cachedIndexsToRefund;
				tmpcachedTimeoutToPunish = lastSnapshot.cachedTimeoutPunishToRun;

				uint32_t cachedHeight = (lastSnapshot.blockHeight > CACHED_BLOCK_COUNT) ? (lastSnapshot.blockHeight - CACHED_BLOCK_COUNT) : 0;
				SnapshotClass cachedSnapshot;
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] ready to get snapshot height = %d\n", cachedHeight);
				if (!GetSnapshotByHeight(cachedSnapshot, cachedHeight))
				{
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] get cache snapshot faile\n");
					return false;
				}

				std::set<uint16_t>::iterator it;
				for (it = cachedSnapshot.curRefundIndexList.begin(); it != cachedSnapshot.curRefundIndexList.end(); it++)
				{
					if (!tmpcachedIndexsToRefund.count(*it))
						tmpcachedIndexsToRefund.insert(*it);
				}
				
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock]The currently saved temporary list of waiting refunds is：\n");
				printsets(tmpcachedIndexsToRefund);

				for (it = cachedSnapshot.curTimeoutPunishList.begin(); it != cachedSnapshot.curTimeoutPunishList.end(); it++)
				{
					if (!tmpcachedTimeoutToPunish.count(*it))
						tmpcachedTimeoutToPunish.insert(*it);
				}
				
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock]The temporary list of the currently saved prepared execution timeout penalties is：\n");
				printsets(tmpcachedTimeoutToPunish);

			}
			
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

		uint64_t cacheTime = (pblock->nPeriodStartTime - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME) / 1000;
		SnapshotClass cachedsnapshot;
		if (!GetSnapshotByTime(cachedsnapshot, cacheTime))
		{
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] GetSnapshotByTime faile！\n");
			return false;
		}

		CPubKey recvPublickey;
		std::vector<unsigned char>  recvSign;
		if (!getPublicKeyFromBlock(pblock.get(), recvPublickey, recvSign)) {
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] get publickey and sigedsting in block faile\n");
			return false;
		}

		CKeyID  curHash = recvPublickey.GetID();
		uint16_t curblockpkhashindex;
		if (!ContainPK(curHash, curblockpkhashindex))
		{
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] publickey is not in cashed list，error\n");
			return false;
		}
		LogPrintf("[CConsensusAccountPool::pushDPOCBlock] now publickey=%s index=%d\n", curHash.GetHex().c_str(), curblockpkhashindex);
		newsnapshot.pkHashIndex = curblockpkhashindex;

		CConsensusAccount tmpaccount = candidatelist[curblockpkhashindex];
		tmpaccount.setCredit(tmpaccount.getCredit() + PACKAGE_CREDIT_REWARD);
		candidatelist[curblockpkhashindex] = tmpaccount;
		LogPrintf("[CConsensusAccountPool::pushDPOCBlock] add index = %d,credit=%d\n", curblockpkhashindex, candidatelist[curblockpkhashindex].getCredit());

		//Calculate the current block to the previous block timeout violation
		if (snapshotlist.size() > Params().CHECK_START_BLOCKCOUNT)
		{
			if (newsnapshot.curTimeoutIndexRecord.count(curblockpkhashindex))
			{
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] Delete the timeout record of the block\n");
				newsnapshot.curTimeoutIndexRecord.erase(curblockpkhashindex);
			}

			std::map<uint16_t, int> timeoutaccountindexs;
			if (!GetTimeoutIndexs(pblock, lastSnapshot, timeoutaccountindexs))
			{
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] Failed to obtain timeout record\n");
				return false;
			}

			//The current timeout record list is refreshed
			std::map<uint16_t, int>::iterator timeoutIt;
			for (timeoutIt = timeoutaccountindexs.begin(); timeoutIt != timeoutaccountindexs.end(); timeoutIt++)
			{
				//If the current public key is not in the penalty cache or the penalty record, it is recorded as a new timeout record
				uint16_t curtimeoutindex = timeoutIt->first;
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] Calculated the timeout key HASH index=%d\n", curtimeoutindex);
				printsets(newsnapshot.cachedTimeoutPunishToRun);
				printsets(newsnapshot.curTimeoutPunishList);
				if (newsnapshot.cachedTimeoutPunishToRun.count(curtimeoutindex) ||
					newsnapshot.curTimeoutPunishList.count(curtimeoutindex))
				{
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] The record of the penalty record or the new overtime penalty record includes the index, and no overtime record is added\n");
					continue;
				}
				if (lastSnapshot.curBanList.count(curtimeoutindex) ||
					lastSnapshot.cachedBanList.count(curtimeoutindex))
				{
					newsnapshot.curTimeoutIndexRecord.erase(curtimeoutindex);
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] This index is included in the serious penalty record, which removes the timeout record of the node\n");
					continue;
				}

				std::vector<SnapshotClass> cachedSnapshots;
				uint32_t cachedHeight = lastSnapshot.blockHeight - CACHED_BLOCK_COUNT;
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] ready to get snapshots when height in %d-%d\n", cachedHeight, lastSnapshot.blockHeight);
				if (!GetSnapshotsByHeight(cachedSnapshots, cachedHeight))
				{
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] get snapshot faile\n");
					return false;
				}
				std::vector<SnapshotClass>::iterator searchIt;
				bool founded = false;
				for (searchIt = cachedSnapshots.begin(); searchIt != cachedSnapshots.end(); searchIt++)
				{
					printsets((*searchIt).curTimeoutPunishList);
					if ((*searchIt).curTimeoutPunishList.count(curtimeoutindex))
					{
						LogPrintf("[CConsensusAccountPool::pushDPOCBlock] The cached new timeout penalty record includes the index, which does not add a timeout record\n");
						founded = true;
						break;
					}
				}
				if (founded)
					continue;

				int count = timeoutIt->second;
				if (newsnapshot.curTimeoutIndexRecord.count(curtimeoutindex))
				{
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] This index is present in the current timeout record, and the count is cumulative\n");
					count = newsnapshot.curTimeoutIndexRecord[curtimeoutindex] + count;
					newsnapshot.curTimeoutIndexRecord[curtimeoutindex] = count;
				}
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock]  add timeout recode in index = %d\n", curtimeoutindex);
				newsnapshot.curTimeoutIndexRecord[curtimeoutindex] = count;
			}

			//According to the time-out record list, once there are N consecutive records that do not block, the execution timeout penalty
			std::map<uint16_t, int>::iterator iterTimeout = newsnapshot.curTimeoutIndexRecord.begin();
			
			while(iterTimeout != newsnapshot.curTimeoutIndexRecord.end())
			{
				bool founded = false;
				for (std::vector<std::string>::iterator it = trustPKHashList.begin(); it != trustPKHashList.end(); it++)
				{
					uint160 believedPubkey;
					believedPubkey.SetHex(*it);
					if (candidatelist.at(iterTimeout->first).getPubicKey160hash() == believedPubkey)
					{
						LogPrintf("[CConsensusAccountPool::pushDPOCBlock] The current public key is trusted not to be penalized！\n");
						founded = true;
						break;
					}
				}
				if (founded)
				{
					++iterTimeout;
					continue;
				}
					
				bool bErase = false;
				if (iterTimeout->second >= MAX_TIMEOUT_COUNT)
				{
					uint16_t pkhashindex = iterTimeout->first;

					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] publickey index=%d timeout，Record the timeout penalty list\n", pkhashindex);

					newsnapshot.curTimeoutPunishList.insert(pkhashindex);

					//iterTimeout = 
					++iterTimeout;
					newsnapshot.curTimeoutIndexRecord.erase(pkhashindex);
					bErase = true;

					if (newsnapshot.curCandidateIndexList.count(pkhashindex))
					{
						newsnapshot.curCandidateIndexList.erase(pkhashindex);
					}
				}

				if (!bErase)
				{
					++iterTimeout;
				}
			}

		}

		//Calculate the current chunk order index and plug in the snapshot list
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

	//Analysis of trading
	for (const auto& tx : pblock->vtx)
	{
		if (tx->IsCoinBase())
		{
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] Deal with Coinbase TX %s\n", tx->GetHash().GetHex().c_str());
			//Read CoinBase to remove the already executed refund transaction from the cache list
			for (const CTxOut& txout : tx->vout)
			{
				uint16_t pkindex;
				if (txout.GetCampaignType() == TYPE_CONSENSUS_RETURN_DEPOSI)
				{
					if (ContainPK(txout.devoteLabel.hash, pkindex))
					{
						LogPrintf("[CConsensusAccountPool::pushDPOCBlock] Deal with refund\n");
						if (newsnapshot.cachedIndexsToRefund.count(pkindex))
						{
							newsnapshot.cachedIndexsToRefund.erase(pkindex);
													
							//Remove the refund account record from the timeout list
							newsnapshot.curTimeoutIndexRecord.erase(pkindex);

							//If it is a billing account (and is not currently analyzing a local block), delete it
							std::string localpkhashhexstring;
							
							if (CDpocInfo::Instance().getLocalAccoutVar(localpkhashhexstring))
							{
								uint160 accountmyself;
								accountmyself.SetHex(localpkhashhexstring);
								if (accountmyself == txout.devoteLabel.hash &&
									analysisfinished) {
									
									LogPrintf("[CConsensusAccountPool::pushDPOCBlock] Delete the local accounting account %s\n", localpkhashhexstring.c_str());
									
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
									
									CDpocInfo::Instance().RemoveInfo();
									CDpocInfo::Instance().setJoinCampaign(false);
								}
							}
							continue;
						}
					}
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] The refund public key is not in the cache list\n");
				}
				//Update the penalty list according to the penalty status
				else if (txout.GetCampaignType() == TYPE_CONSENSUS_ORDINARY_PUSNISHMENT)
				{
					if (ContainPK(txout.devoteLabel.hash, pkindex))
					{
						if ((pkindex >= 0) && (pkindex < candidatelist.size()))
						{
							CConsensusAccount &curAccount = candidatelist[pkindex];
							int64_t curCredit = curAccount.getCredit();
							curCredit -= TIMEOUT_CREDIT_PUNISH;
							//Once added to the penalty list, the record is cleared, each time the deduction needs to be backed up, 
							//so that pop can be rolled back
							curAccount.backupCredit();
							curAccount.setCredit(curCredit);
							candidatelist[pkindex] = curAccount;

							LogPrintf("[CConsensusAccountPool::pushDPOCBlock] Index=%d  Credit value to %d\n", pkindex, curCredit);
						}
						
						//The timeout penalty that has been executed is cleared from the timeout record
						LogPrintf("[CConsensusAccountPool::pushDPOCBlock] Prepare to delete the timeout record\n");
						std::vector<SnapshotClass> vecforprint;
						vecforprint.push_back(newsnapshot);
						printsnapshots(vecforprint);	 
						if (newsnapshot.cachedTimeoutPunishToRun.count(pkindex))
						{
							LogPrintf("[CConsensusAccountPool::pushDPOCBlock] The timeout penalty is executed，Remove the index = %d from the penalty list\n", pkindex);
							newsnapshot.cachedTimeoutPunishToRun.erase(pkindex);
						}
				
						//Set the average penalty value
						std::string strStatus("1");
						std::string strHash(txout.devoteLabel.hash.ToString());
						SetConsensusStatus(strStatus, strHash);

						//Give the public key a refund
						newsnapshot.curRefundIndexList.insert(pkindex);
						LogPrintf("[CConsensusAccountPool::pushDPOCBlock] Execute the timeout refund to the index=%d\n", pkindex);

						continue;
					}
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] The timeout penalty public key is not in the cache list，Abnormal situation\n");
				}
			}

			continue;
		}

		if (tx->GetTxType() != 1)
			continue;

		if (tx->GetCampaignType() == TYPE_CONSENSUS_REGISTER)
		{
			if (g_bStdCout)
			{
				std::cout << "join campaign" << std::endl;
			}

			uint160 curHash = tx->GetCampaignPublickey();
			uint16_t pkhashindex;
			CAmount ipcvalue = tx->GetRegisterIPC();
			if (!ContainPK(curHash, pkhashindex))
			{
				pkhashindex = candidatelist.size();
				candidatelist.push_back(CConsensusAccount(curHash, ipcvalue,tx->GetHash()));
			}
			else
			{
				//Although the public key has been added, the deposit needs to be updated
				candidatelist[pkhashindex].setJoinIPC(ipcvalue);
				candidatelist[pkhashindex].setTxhash(tx->GetHash());
				LogPrintf("[update candidatelist] The currently added public key=%d The deposit=%d, The current public key credit value is %d,txhash：%s\n", 
					pkhashindex, candidatelist[pkhashindex].getJoinIPC(), candidatelist[pkhashindex].getCredit(), candidatelist[pkhashindex].getTxhash().ToString());
			}
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] The currently added public key=%d The deposit=%d, The current public key credit value=%d,txhash ：%s\n", 
				pkhashindex, candidatelist[pkhashindex].getJoinIPC(), candidatelist[pkhashindex].getCredit(),
				candidatelist[pkhashindex].getTxhash().ToString());
			
			//Based on the dynamic adjustment of the deposit value and the current public key credit value, 
			//the limit value of the security gate of the current public key is obtained
			CAmount curDeposiThreshold = GetCurDepositAdjust(curHash, blockHeight);
			LogPrintf("[CConsensusAccountPool::pushDPOCBlock] The current dynamic adjustment of the deposit threshold is %d\n", curDeposiThreshold);
			
			if (ipcvalue >= curDeposiThreshold)
			{
				if (!newsnapshot.curCandidateIndexList.count(pkhashindex))
				{
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] The deposit meets the requirements of the quota，To join campaign\n");
					if (g_bStdCout)
					{
						std::cout << "The deposit meets the requirements of the quota，Application for successful adoption，The public key has been added to the list of candidates" << std::endl;
					}
					newsnapshot.curCandidateIndexList.insert(pkhashindex);
				}
			}
			else
			{
				if (!newsnapshot.cachedIndexsToRefund.count(pkhashindex))
				{
					LogPrintf("[CConsensusAccountPool::pushDPOCBlock] The deposit does not meet the quota request , carries out the refund\n");
					if (g_bStdCout)
					{
						std::cout << "The deposit does not meet the quota request and carries out the refund" << std::endl;
					}
					newsnapshot.cachedIndexsToRefund.insert(pkhashindex);
				}
			}
			
		}
		else if (tx->GetCampaignType() == TYPE_CONSENSUS_QUITE)
		{
			uint160 curHash = tx->GetCampaignPublickey();
			uint16_t pkhashindex;
			if (!ContainPK(curHash, pkhashindex))
			{
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] ContainPK faile");
			}

			if (newsnapshot.curCandidateIndexList.count(pkhashindex))
			{
				newsnapshot.curCandidateIndexList.erase(pkhashindex);
				newsnapshot.curRefundIndexList.insert(pkhashindex);
				LogPrintf("[CConsensusAccountPool::pushDPOCBlock] Perform a normal exit refund to the index=%d\n", pkhashindex);
			}
		}
	}

	if (!PushSnapshot(newsnapshot))
	{
		LogPrintf("[CConsensusAccountPool::pushDPOCBlock]PushSnapshot Adding a snapshot to the list faile！\n");
		return false;
	}

	uint32_t cachedHeight = (blockHeight > CACHED_BLOCK_COUNT) ? blockHeight - CACHED_BLOCK_COUNT : 0;
	std::vector<SnapshotClass> listforprint;
	if (!GetSnapshotsByHeight(listforprint, cachedHeight, blockHeight))
	{
		LogPrintf("[CConsensusAccountPool::pushDPOCBlock] get snapshot height %d-%d to print reslut faile\n", blockHeight - CACHED_BLOCK_COUNT, blockHeight);
	}

	printsnapshots(listforprint);

	return true;
}

bool CConsensusAccountPool::rollbackCandidatelist(uint32_t nHeight)
{
	writeLock wtlock(rwmutex);
	
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

			//From the previous snapshot to the current pending delete snapshot, 
			//if the blacklist operation is performed, the credit value is rolled back
			for (std::set<uint16_t>::iterator banit = iterLast->second.curBanList.begin();
				banit != iterLast->second.curBanList.end(); banit++)
			{
				tmpIndex = *banit;
				candidatelist[tmpIndex].revertCredit();
				LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] pop publickey Index=%d,creditValue=%d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
				
				//Sets the credit value of the current node directly to the credit value in the previous snapshot
				int64_t tmpCredit;
				if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
				{
					candidatelist[tmpIndex].setCredit(tmpCredit);
					LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] Rollback the public key index = %d directly from the snapshot, To creditValue %d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
				}
			}

			//For the current pending delete snapshot, the credit value of the added public key of the block person needs to be rolled back
			tmpIndex = iterList->second.pkHashIndex;

			//Sets the credit value of the current node directly to the credit value in the previous snapshot
			int64_t tmpCredit;
			if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
			{
				candidatelist[tmpIndex].setCredit(tmpCredit);
				LogPrintf("[CConsensusAccountPool::popDPOCBlock] Rollback the public key index=%d directly from the snapshot,To creditValue %d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
			}

			//The credit value that is deducted in the timeout record needs to be rolled back
			std::vector<SnapshotClass> listforprint;
			listforprint.push_back(iterLast->second);
			listforprint.push_back(iterList->second);
			printsnapshots(listforprint);
			for (std::map<uint16_t, int>::iterator timeoutit = iterList->second.curTimeoutIndexRecord.begin();
				timeoutit != iterList->second.curTimeoutIndexRecord.end(); timeoutit++)
			{
				LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] The current timeout record for deleting a snapshot includes indexe = %d\n", timeoutit->first);
				
				if (iterLast->second.curTimeoutIndexRecord.count(timeoutit->first))
				{
					LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] An index=%d is included in the timeout record for the previous snapshot\n", timeoutit->first);
					LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] Last snapshot of the timeout record index=%d number=%d，The number of times to delete a snapshot = %d\n",
						timeoutit->first, iterLast->second.curTimeoutIndexRecord[timeoutit->first], timeoutit->second);

					if (iterLast->second.curTimeoutIndexRecord[timeoutit->first] < timeoutit->second)
					{
						tmpIndex = timeoutit->first;
						
						int64_t tmpCredit;
						if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
						{
							candidatelist[tmpIndex].setCredit(tmpCredit);
							LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] rollback the public key index=%d directly from the snapshot,To creditValue %d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
						}
					}
				}

				else
				{
					tmpIndex = timeoutit->first;
					int64_t tmpCredit;
					if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
					{
						candidatelist[tmpIndex].setCredit(tmpCredit);
						LogPrintf("[CConsensusAccountPool::rollbackCandidatelist]  rollback the public key index=%d directly from the snapshot,To creditValue %d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
					}
				}
			}

			//If the current snapshot has a timeout penalty, 
			//and the previous snapshot does not have this timeout penalty, revert is required
			for (std::set<uint16_t>::iterator punishit = iterList->second.curTimeoutPunishList.begin();
				punishit != iterList->second.curTimeoutPunishList.end();
				punishit++)
			{
				tmpIndex = (*punishit);

				LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] An index=%d is included in the timeout penalty list for the snapshot\n", tmpIndex);
				if (!iterLast->second.curTimeoutPunishList.count(tmpIndex) &&
					iterLast->second.curTimeoutIndexRecord.count(tmpIndex))
				{						
					//Sets the credit value of the current node directly to the credit value in the previous snapshot
					int64_t tmpCredit;
					if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
					{
						candidatelist[tmpIndex].setCredit(tmpCredit);
						LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] 2rollback the public key index=%d directly from the snapshot,To creditValue %d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
					}
				}
			}

			LogPrintf("[CConsensusAccountPool::rollbackCandidatelist] pop Snapshot height=%d\n", iterLast->first);
		}
		else
		{
			break;
		}
	}
}

bool CConsensusAccountPool::popDPOCBlock( uint32_t blockHeight)
{
	writeLock wtlock(rwmutex);

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

			for (std::set<uint16_t>::iterator banit = iterLast->second.curBanList.begin();
				banit != iterLast->second.curBanList.end(); banit++)
			{
				tmpIndex = *banit;
				candidatelist[tmpIndex].revertCredit();
				LogPrintf("[CConsensusAccountPool::popDPOCBlock] rollback the public key index=%d,To creditValue %d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
				
				int64_t tmpCredit;
				if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
				{
					candidatelist[tmpIndex].setCredit(tmpCredit);
					LogPrintf("[CConsensusAccountPool::popDPOCBlock] rollback the public key index=%d directly from the snapshot,To creditValue %d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
				}
			}

			tmpIndex = iterList->second.pkHashIndex;
			
			int64_t tmpCredit;
			if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
			{
				candidatelist[tmpIndex].setCredit(tmpCredit);
				LogPrintf("[CConsensusAccountPool::popDPOCBlock] rollback the public key index=%d directly from the snapshot,To creditValue %d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
			}

			std::vector<SnapshotClass> listforprint;
			listforprint.push_back(iterLast->second);
			listforprint.push_back(iterList->second);
			printsnapshots(listforprint);
			for (std::map<uint16_t, int>::iterator timeoutit = iterList->second.curTimeoutIndexRecord.begin();
				timeoutit != iterList->second.curTimeoutIndexRecord.end(); timeoutit++)
			{
				LogPrintf("[CConsensusAccountPool::popDPOCBlock] The current timeout record for deleting a snapshot includes index = %d\n", timeoutit->first);
				
				if (iterLast->second.curTimeoutIndexRecord.count(timeoutit->first))
				{
					LogPrintf("[CConsensusAccountPool::popDPOCBlock] An index=%d is included in the timeout record for the previous snapshot\n", timeoutit->first);
					LogPrintf("[CConsensusAccountPool::popDPOCBlock] Last snapshot of the timeout record index=%d number=%d，The number of times to delete a snapshot = %d\n\n",
						timeoutit->first, iterLast->second.curTimeoutIndexRecord[timeoutit->first], timeoutit->second);
				
					if (iterLast->second.curTimeoutIndexRecord[timeoutit->first] < timeoutit->second)
					{
						tmpIndex = timeoutit->first;
					
						int64_t tmpCredit;
						if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
						{
							candidatelist[tmpIndex].setCredit(tmpCredit);
							LogPrintf("[CConsensusAccountPool::popDPOCBlock] rollback the public key index=%d directly from the snapshot,To creditValue %d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
						}
					}
				}
				else
				{
					tmpIndex = timeoutit->first;
				
					int64_t tmpCredit;
					if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
					{
						candidatelist[tmpIndex].setCredit(tmpCredit);
						LogPrintf("[CConsensusAccountPool::popDPOCBlock] rollback the public key index=%d directly from the snapshot,To creditValue %d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
					}
				}
			}

			for (std::set<uint16_t>::iterator punishit = iterList->second.curTimeoutPunishList.begin();
				punishit != iterList->second.curTimeoutPunishList.end();
				punishit++)
			{
				tmpIndex = (*punishit);
				LogPrintf("[CConsensusAccountPool::popDPOCBlock] An index=%d is included in the timeout penalty list for the snapshot\n", tmpIndex);
				if (!iterLast->second.curTimeoutPunishList.count(tmpIndex) &&
					iterLast->second.curTimeoutIndexRecord.count(tmpIndex))
				{
					int64_t tmpCredit;
					if (getCreditFromSnapshotByIndex(iterLast->second, tmpIndex, tmpCredit))
					{
						candidatelist[tmpIndex].setCredit(tmpCredit);
						LogPrintf("[CConsensusAccountPool::popDPOCBlock] rollback the public key index=%d directly from the snapshot,To creditValue %d\n", tmpIndex, candidatelist[tmpIndex].getCredit());
					}
				}
			}

			LogPrintf("[CConsensusAccountPool::popDPOCBlock] pop snapshot height=%d\n", iterLast->first);

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
				writeCandidatelistToFile();

				//Truncate the SnapshotIndex file
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

				//Truncate the Snapshot file
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

				flushSnapshotToDisk();
			}
			
			m_mapSnapshotIndex.erase(iterList->first);

			iterList = snapshotlist.erase(iterList);
		}
		else
		{
			break;
		}	
	}
	
	LogPrintf("[CConsensusAccountPool::popDPOCBlock] end. \n");
	
	return true;
}


bool CConsensusAccountPool::AddDPOCCoinbaseToBlock(CBlock* pblockNew, CBlockIndex* pindexPrev, uint32_t blockHeight, CMutableTransaction &coinbaseTx)
{
	SnapshotClass cursnapshot;
	if (!GetLastSnapshot(cursnapshot))
	{
		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] You can't get the latest snapshot, perhaps because the snapshot list is empty and return true\n");
		return true;
	}

	std::set<uint16_t>::iterator refundit;
	for (refundit = cursnapshot.cachedIndexsToRefund.begin(); refundit!= cursnapshot.cachedIndexsToRefund.end(); refundit++)
	{
		if(contain(cursnapshot.cachedMeetingAccounts, *refundit))
		{
			//LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] Delay the refund of the public key deposit %d\n", *refundit);
			continue;
		}
		if (cursnapshot.curBanList.count(*refundit) || cursnapshot.cachedBanList.count(*refundit))
		{
			LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] The refund account is on the blacklist！\n");
		}

		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] The public key=%d of the current refund, The deposit=%d\n", *refundit, candidatelist[*refundit].getJoinIPC());
		CAmount zero = 0;
		CTxOut curRefund(zero, candidatelist[*refundit].getPubicKey160hash());
		coinbaseTx.vout.emplace_back(std::move(curRefund));

		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] Refundable deposit=%d\n", candidatelist[*refundit].getJoinIPC());
	}

	//Ordinary punishment
	for (refundit = cursnapshot.cachedTimeoutPunishToRun.begin(); refundit != cursnapshot.cachedTimeoutPunishToRun.end(); refundit++)
	{
		if (contain(cursnapshot.cachedMeetingAccounts, *refundit))
		{
			LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] Delay the refund of the public key index %d\n", *refundit);
			continue;
		}
		std::cout << "Add Refund to Coinbase tx" << std::endl;

		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] Timeout Punish publickey=%d,the credit=%d", *refundit, candidatelist[*refundit].getJoinIPC());
		CTxOut curTimeoutPunish(TYPE_CONSENSUS_ORDINARY_PUSNISHMENT, candidatelist[*refundit].getPubicKey160hash());
		coinbaseTx.vout.emplace_back(std::move(curTimeoutPunish));
	}

	for (refundit = cursnapshot.curBanList.begin(); refundit != cursnapshot.curBanList.end(); refundit++)
	{
		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] Severe punishment publickey=%d,the credit=%d", *refundit, candidatelist[*refundit].getJoinIPC());
		CTxOut curBanTx(TYPE_CONSENSUS_SEVERE_PUNISHMENT, candidatelist[*refundit].getPubicKey160hash());
		coinbaseTx.vout.emplace_back(std::move(curBanTx));
	}

	std::string pubkeystring;
	if (!CDpocInfo::Instance().getLocalAccoutVar(pubkeystring))
	{
		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] ERROR! no local account\n");
		return false;
	}

	uint160 curpk;
	curpk.SetHex(pubkeystring);
	if (!addSignToCoinBase(pblockNew, pindexPrev, blockHeight, coinbaseTx, &curpk)) {
		LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock]  ERROR! add sign failed\n");
		return false;
	}
	
	LogPrintf("[CConsensusAccountPool::AddDPOCCoinbaseToBlock] end by true\n ");
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
 
	if (pPubKeyHash) {
		CKeyID		publickeyID(*pPubKeyHash);
		CPubKey		vchPubKeyOut;
		if (pwalletMain->GetPubKey(publickeyID, vchPubKeyOut)) {
			LogPrintf("[CConsensusAccountPool::addSignToCoinBase] GetPubKey is ok\n");
		}
		else {
			LogPrintf("[CConsensusAccountPool::addSignToCoinBase] don't find public key ");
			return false;
		}
		
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
		pwalletMain->Lock(); //To encrypt
		
		uint256 hash;
		CAmount nfee = 0;
		for (int i=0; i<coinbaseTx.vout.size(); i++)
		{
			nfee += coinbaseTx.vout[i].nValue;
		}
		CHash256 hashoperator;

		hashoperator.Write((unsigned char*)&(pblock->nPeriodStartTime), sizeof(pblock->nPeriodStartTime));//8Byte
		hashoperator.Write((unsigned char*)&(pblock->nPeriodCount), sizeof(pblock->nPeriodCount));//4Byte
		hashoperator.Write((unsigned char*)&(pblock->nTimePeriod), sizeof(pblock->nTimePeriod));//4Byte
		hashoperator.Write((unsigned char*)&(pblock->nTime), sizeof(pblock->nTime));//4Byte
		hashoperator.Write((unsigned char*)&nfee, sizeof(nfee));//8Byte
		hashoperator.Write((unsigned char*)pindexPrev->GetBlockHash().begin(), 32);//256bit
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
		memcpy(dataBuff + datalen, (unsigned char*)pindexPrev->GetBlockHash().begin(), 32);//256 bits
		datalen += 32;
		std::string datahex;
		for (int i = 0; i < datalen; i++)
		{
			char tmp[3];
			sprintf(tmp, "%02x", dataBuff[i]);
			datahex = datahex + std::string(tmp);
		}
		LogPrintf("[CConsensusAccountPool::addSignToCoinBase] Signature clear=%s , clear HASH=%s , public key HASH=%s\n", datahex.c_str(), hash.GetHex().c_str(), pPubKeyHash->GetHex().c_str());

		// Generate signature cache
		std::vector<unsigned char> vchSigSend;
		vchSigSend.resize(2 + vchPubKeyOut.size() + vchSig.size());

		//Cached signature
		unsigned char vchSigLen = (unsigned char)vchSig.size();
		vchSigSend[0] = vchSigLen;
		for (std::vector<unsigned char>::size_type ix = 0; ix < vchSig.size(); ++ix) {
			vchSigSend[ix + 1] = vchSig[ix];
		}

		vchSigSend[vchSigLen + 1] = (unsigned char)vchPubKeyOut.size();
		for (std::vector<unsigned char>::size_type ix = 0; ix < vchPubKeyOut.size(); ++ix) {
			vchSigSend[ix + 2 + vchSigLen] = vchPubKeyOut[ix];
		}

		std::string strSin2Publickey = EncodeBase58(vchSigSend);
		
		LogPrintf("[CConsensusAccountPool::addSignToCoinBase] sign=%s\n", strSin2Publickey);
		coinbaseTx.vout[0].coinbaseScript = strSin2Publickey;
		coinbaseTx.vout[0].txType = 0;
	}
	return true;
}

bool CConsensusAccountPool::GetSnapshotByTime(SnapshotClass &snapshot, uint64_t readtime)
{
	readLock rdlock(rwmutex);

	if (snapshotlist.size()==0)
	{
		LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] The snapshot list is empty，cannot check\n");
		return false;
	}

	LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] Passed-in cache time= %d \n", readtime);

	//Find the time corresponding block as a cache block, give it back value, and cache the list of candidates
	if (snapshotlist.size() > 0)
	{
		std::map<uint32_t, SnapshotClass>::iterator mapit = snapshotlist.end();
		mapit--;
		LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] The last snapshot time= %d \n", mapit->second.timestamp);
		
		while (mapit->second.timestamp > readtime)
		{
			if (mapit == snapshotlist.begin()) 
			{
				LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] No matching snapshot was found\n");
				return false;
			}
			mapit--;
		}
		LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] Find the height of the snapshot block= %d , time= %d \n",
			mapit->second.blockHeight, mapit->second.timestamp);

		snapshot = mapit->second;
		return true;
	}

	LogPrintf("[CConsensusAccountPool::GetSnapshotByTime] The snapshot list is empty\n");

	return false;
}


bool CConsensusAccountPool::GetSnapshotByHeight(SnapshotClass &snapshot, uint32_t height)
{
	readLock rdlock(rwmutex);

	if (snapshotlist.empty())
	{
		LogPrintf("[CConsensusAccountPool::GetSnapshotByHeight] The snapshot list is empty！\n");
		return false;
	}

	std::map<uint32_t, SnapshotClass>::iterator mapit = snapshotlist.end();
	mapit--;
	while (mapit->second.blockHeight > height && mapit != snapshotlist.begin())
		mapit--;

	if (mapit->second.blockHeight > height)
	{
		if (g_bStdCout)
		{
			std::cout << "[CConsensusAccountPool::GetSnapshotsByHeight] There is no block in the snapshot list that is not higher than this height！\n";
		}
		LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] There is no block in the snapshot list that is not higher than this height！\n");
		
		return false;
	}

	snapshot = mapit->second;
	return true;
}

bool CConsensusAccountPool::GetSnapshotsByHeight(std::vector<SnapshotClass> &snapshots, uint32_t lowest, uint32_t highest)
{
	readLock rdlock(rwmutex);

	LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] The height of the block to be read is in %d-%d\n", lowest, highest);

	if (snapshotlist.empty())
	{
		LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] The snapshot list is empty！\n");
		return false;
	}

	std::map<uint32_t, SnapshotClass>::iterator mapit = snapshotlist.end();
	mapit--;

	if (highest != 0)
	{
		while (mapit->second.blockHeight > highest && mapit != snapshotlist.begin())
			mapit--;

		if (mapit == snapshotlist.begin())
		{
			LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] There is no less than the highest height in the snapshot list！\n");
			return false;
		}
	}

	snapshots.clear();
	while (mapit->second.blockHeight >= lowest)
	{
		LogPrintf("[CConsensusAccountPool::GetSnapshotsByHeight] Find the snapshot height %d\n", mapit->second.blockHeight);
		snapshots.push_back(mapit->second);
		if (mapit == snapshotlist.begin())
			break;
		mapit--;
	}

	return true;
}

bool CConsensusAccountPool::GetLastSnapshot(SnapshotClass &snapshot)
{
	readLock rdlock(rwmutex);

	if (snapshotlist.empty())
	{
		LogPrintf("[CConsensusAccountPool::GetLastSnapshot] The snapshot list is empty！\n");
		return false;
	}

	std::map<uint32_t, SnapshotClass>::iterator mapit = snapshotlist.end();
	mapit--;

	snapshot = mapit->second;
	LogPrintf("[CConsensusAccountPool::GetLastSnapshot] Get the snapshot tail height=%d, Plan package time=%d,Actually package time=%d\n",
		snapshot.blockHeight, snapshot.timestamp, snapshot.blockTime);

	return true;
}

bool CConsensusAccountPool::PushSnapshot(SnapshotClass snapshot)
{
	writeLock wtlock(rwmutex);
	
	if (snapshotlist.count(snapshot.blockHeight))
	{
		LogPrintf("[CConsensusAccountPool::PushSnapshot] add  height = %d of sanpshot to list\n", snapshot.blockHeight);
		return false;
	}

	LogPrintf("[CConsensusAccountPool::PushSnapshot] add  height = %d of sanpshot to list\n", snapshot.blockHeight);
	snapshotlist[snapshot.blockHeight] = snapshot;

	if (snapshotlist.size() > SNAPSHOTLENGTH)
	{
		snapshotlist.erase(snapshotlist.begin());
		m_mapSnapshotIndex.erase(m_mapSnapshotIndex.begin());
	}

	//Synchronous write file
	if ((0 == (snapshot.blockHeight % 2000)) && (snapshot.blockHeight != 0))
	{
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
			
			//Gets the start offset of the snapshot file
			CSnapshotIndex  sanpshotIndex;
			sanpshotIndex.nHeight = iter->first;
			sanpshotIndex.nOffset = fileSize;

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
			m_mapSnapshotIndex[sanpshotIndex.nHeight] = sanpshotIndex.nOffset;
			fileSize = fileSize + getSnapshotSize(iter->second);
		}
		fclose(file1);
		fclose(fileIndex1);
		file1 = NULL;
		fileIndex1 = NULL;

		flushSnapshotToDisk();
	}

	return true;
}

bool CConsensusAccountPool::getPKIndexBySortedIndex(uint16_t &pkIndex, uint160 &pkhash, std::list<std::shared_ptr<CConsensusAccount >> consensusList, int sortedIndex)
{
	LogPrintf("[CConsensusAccountPool::getPKIndexBySortedIndex] To find the list of candidates size=%d ， To find index=%d\n", consensusList.size(), sortedIndex);
	std::list<std::shared_ptr<CConsensusAccount> >::iterator listit = consensusList.begin();
	for (int i=0; i<sortedIndex && i<consensusList.size() && listit != consensusList.end(); i++)
	{
		listit++;
	}
	if (listit == consensusList.end())
		return false;
	
	pkhash = (*listit)->getPubicKey160hash();
	return ContainPK(pkhash, pkIndex);
}

bool CConsensusAccountPool::GetTimeoutIndexs(const std::shared_ptr<const CBlock> pblock, SnapshotClass lastsnapshot, std::map<uint16_t, int>& timeoutindexs)
{
	timeoutindexs.clear();

	//Calculate the number of blocks from the current block to the end of the snapshot
	int timediff = (pblock->nPeriodStartTime + pblock->nTimePeriod * BLOCK_GEN_TIME + BLOCK_GEN_TIME) / 1000 - lastsnapshot.timestamp;

	if (timediff > BLOCK_GEN_TIME/1000)
	{
		LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] there is Timeout Missing blocks\n");

		//Calculate the sorting of the current wheel meeting
		uint64_t currentmeetingcachedtime = (pblock->nPeriodStartTime - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME) /1000;
		SnapshotClass currentmeetingcachedsnapshot;
		if (!GetSnapshotByTime(currentmeetingcachedsnapshot, currentmeetingcachedtime))
		{
			LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] The list of candidates for the cache failed\n");
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
			LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] Calculate the current round meeting sort failure\n");
			return false;
		}
		
		//Determine whether the block at the end of the snapshot is the same wheel meeting as the current block, or the tail block of the previous session
		if (lastsnapshot.meetingstarttime == pblock->nPeriodStartTime || lastsnapshot.timestamp == pblock->nPeriodStartTime / 1000)
		{
			int index = 0;
			while ((pblock->nPeriodStartTime + index*BLOCK_GEN_TIME + BLOCK_GEN_TIME) / 1000 <= lastsnapshot.timestamp)
			{
				index++;
			}
			
			//Find the index index of the missing block and start processing the timeout
			for (index; index < pblock->nTimePeriod; index++)
			{
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] this round public key HASH sort %d node has no block，Record the timeout\n", index);
				uint16_t tmpPubkeyIndex;
				uint160 tmpPubkeyHash;
				if (!getPKIndexBySortedIndex(tmpPubkeyIndex, tmpPubkeyHash, consensusList, index))
				{
					LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] not find public key HASH index！\n", index);
					continue;
				}
				
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] public key HASH index %d record one timeout\n", tmpPubkeyIndex);
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
			int index = 0;
			
			for (index = 0; index < pblock->nTimePeriod; index++)
			{
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] this round sort %d node has no block，Record the timeout\n", index);
				uint16_t tmpPubkeyIndex;
				uint160 tmpPubkeyHash;
				if (!getPKIndexBySortedIndex(tmpPubkeyIndex, tmpPubkeyHash, consensusList, index))
				{
					LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] not find public key HASH index！\n", index);
					continue;
				}
				
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] public key HASH Index %d record one timeout\n", tmpPubkeyIndex);
				int count = 1;
				if (timeoutindexs.count(tmpPubkeyIndex))
				{
					count = timeoutindexs[tmpPubkeyIndex] ++;
				}
				timeoutindexs[tmpPubkeyIndex] = count;
			}

			currentmeetingcachedtime = (lastsnapshot.meetingstarttime - CACHED_BLOCK_COUNT * BLOCK_GEN_TIME) / 1000;
			if (!GetSnapshotByTime(currentmeetingcachedsnapshot, currentmeetingcachedtime))
			{
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] get the previous round cashe CandidateList false\n");
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
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] The calculation of the previous session failed\n");
				return false;
			}

			//Calculate the last round of missing blocks to the end
			while ((lastsnapshot.meetingstarttime + index*BLOCK_GEN_TIME + BLOCK_GEN_TIME) / 1000 <= lastsnapshot.timestamp)
			{
				index++;
			}
			
			//Find the index index of the missing block and start processing the timeout
			for (index; lastsnapshot.meetingstarttime + index*BLOCK_GEN_TIME + BLOCK_GEN_TIME <= lastsnapshot.meetingstoptime; index++)
			{
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] The previous round public key HASH sort %d node has no block，Record the timeout\n", index);
				uint16_t tmpPubkeyIndex;
				uint160 tmpPubkeyHash;
				if (!getPKIndexBySortedIndex(tmpPubkeyIndex, tmpPubkeyHash, consensusList, index))
				{
					LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] No index of the public key HASH was found！%d \n", index);
					continue;
				}
				
				LogPrintf("[CConsensusAccountPool::GetTimeoutIndexs] publickey HASH %s Index %d record one timeout\n", tmpPubkeyHash.GetHex().c_str(), tmpPubkeyIndex);
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
	CAmount deposit = Params().MIN_DEPOSI;

	for (std::vector<std::string>::iterator trustit = trustPKHashList.begin();
		trustit != trustPKHashList.end(); trustit++)
	{
		if (pkhash.GetHex() == *trustit)
		{
			LogPrintf("[CConsensusAccountPool::GetCurDepositThreshold] trust publickey，cash pledge=%d (%f IPC)\n", deposit, (double)deposit / COIN);
			return deposit;
		}
	}

	uint32_t currentblockheight = blockheight;
	uint32_t consultblockheight = (currentblockheight - 1) / 1000;
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
	LogPrintf("[CConsensusAccountPool::GetCurDepositThreshold] get MAX[%d block, current best lock]meeting num)：%d\n", consultblockheight, count);

	if (count < 20)
	{
		deposit = Params().MIN_DEPOSI;
	}
	else if (count < 30)
	{
		deposit = Params().MIN_DEPOSI * 2;
	}
	else if (count < 40)
	{
		deposit = Params().MIN_DEPOSI * 3;
	}
	else if (count < 50)
	{
		deposit = Params().MIN_DEPOSI * 5;
	}
	else if (count < 60)
	{
		deposit = Params().MIN_DEPOSI * 8;
	}
	else if (count < 70)
	{
		deposit = Params().MIN_DEPOSI * 13;
	}
	else if (count < 100)
	{
		deposit = Params().MIN_DEPOSI * 21;
	}
	else
	{
		deposit = Params().MIN_DEPOSI * 34;
	}

	LogPrintf("[CConsensusAccountPool::GetCurDepositThreshold] Current adjustment costs based on the number of people=%d (%f IPC)\n", deposit, (double)deposit / COIN);

	uint16_t tmpIndex;
	if (ContainPK(pkhash, tmpIndex))
	{

		if (blockheight <= Params().ADJUSTDP_BLOCKS)
		{
			if (0 < candidatelist[tmpIndex].getCredit())
			{
				deposit = candidatelist[tmpIndex].getJoinIPC();
				LogPrintf("[CConsensusAccountPool::GetCurDepositThreshold] The deposit fee shall be maintained for the first time during the first year =%d (%f IPC)\n", deposit, (double)deposit / COIN);
				return  deposit;
			}
			return deposit;

		}

		int64_t Credit = candidatelist[tmpIndex].getCredit();

		Credit -= Credit % 1000;
		deposit -= (Params().MIN_DEPOSI*Credit) / 10000;

		if (deposit < Params().MIN_DEPOSI)
		{
			deposit = Params().MIN_DEPOSI;
		}

		LogPrintf("[CConsensusAccountPool::GetCurDepositThreshold] Adjust the cost according to the credit value deduction=%d (%f IPC)\n", deposit, (double)deposit / COIN);

	}

	return deposit;
}


bool CConsensusAccountPool::listSnapshotsToTime(std::list<std::shared_ptr<CConsensusAccount>> &listConsus, int readtime)
{
	listConsus.clear();
	SnapshotClass cachedsnapshot;
	if (!GetSnapshotByTime(cachedsnapshot, readtime))
	{
		LogPrintf("[CConsensusAccountPool::listSnapshotsToTime] Lookup cache snapshot failed\n");
		return false;
	}

	std::set<uint16_t> accounts = cachedsnapshot.curCandidateIndexList;
	std::set<uint16_t>::const_iterator iter;

	for (iter = accounts.begin(); iter != accounts.end(); iter++)
	{
	
		CConsensusAccount acount = candidatelist.at(*iter);
		listConsus.emplace_back(std::make_shared<CConsensusAccount>(acount));
	
	}

	return true;
}

bool CConsensusAccountPool::listSnapshots(std::list<std::shared_ptr<CConsensusAccount>> &listConsus)
{
	int64_t nNowTime = timeService.GetCurrentTimeMillis()/1000;
	return listSnapshotsToTime(listConsus, nNowTime);
}

bool  CConsensusAccountPool::analysisConsensusSnapshots()
{
	LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] begin\n");
	
	int nAllSnapshotSize = 0;
	{
		writeLock wtlock(rwmutex);
		//Load consensus public key HASH
		readCandidatelistFromFile();

		boost::filesystem::path pathIndex = GetDataDir() / m_strSnapshotIndexPath;
		boost::filesystem::path pathSnapshot = GetDataDir() / m_strSnapshotPath;
		if (boost::filesystem::exists(pathIndex) && boost::filesystem::exists(pathSnapshot))
		{
			uint64_t fileSizeSnapshot = boost::filesystem::file_size(pathSnapshot);
			uint64_t u64fileSize = boost::filesystem::file_size(pathIndex);

			if ((fileSizeSnapshot > 0)&&(u64fileSize >= m_u64SnapshotIndexSize))
			{
				for (uint64_t nBegin = 0; nBegin <= u64fileSize - m_u64SnapshotIndexSize; nBegin += m_u64SnapshotIndexSize)
				{
					CSerializeDpoc<CSnapshotIndex> serializeIndex;
					CSnapshotIndex  sanpshotIndex;
					serializeIndex.ReadFromDisk(sanpshotIndex, nBegin, m_strSnapshotIndexPath);
				
					//Load the snapshot
					if (sanpshotIndex.nOffset < fileSizeSnapshot)
					{
						CSerializeDpoc<SnapshotClass> serializeSnapshot;
						SnapshotClass snapshot;
						serializeSnapshot.ReadFromDisk(snapshot, sanpshotIndex.nOffset, m_strSnapshotPath);

						snapshotlist[sanpshotIndex.nHeight] = snapshot;
						m_mapSnapshotIndex[sanpshotIndex.nHeight] = sanpshotIndex.nOffset;
					
						if(snapshotlist.size() > SNAPSHOTLENGTH)
						{
							snapshotlist.erase(snapshotlist.begin());
							m_mapSnapshotIndex.erase(m_mapSnapshotIndex.begin());
						}
					
						++nAllSnapshotSize;
					
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
		}

		if (1 < snapshotlist.size())
		{
			std::map<uint32_t, SnapshotClass>::iterator iterMap = snapshotlist.end();
			
			--iterMap;
			int nHeightTest = iterMap->first;
			uint32_t nHeight = iterMap->first;
			nAllSnapshotSize = iterMap->first;
			LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] snapshotlist height TEST：%d\n", nHeightTest);
			LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] snapshotlist height ：%d\n", nHeight);	
		}

		LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] snapshotlist size：%d\n", snapshotlist.size());
		LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] Read the file completion and read the height from the file %d\n", nAllSnapshotSize);
	}

	int nChainHeight = 0;
	{
		LOCK(cs_main);
		nChainHeight = chainActive.Height();
		LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] chainActive height：%d\n", nChainHeight);
	}

	//chainActive height < Snapshot height 
	if (nChainHeight < nAllSnapshotSize)
	{
		//Truncate the candidate list and write to the file
		rollbackCandidatelist(nChainHeight);
		writeCandidatelistToFile();
		boost::filesystem::path pathCandidatelist = GetDataDir() / m_strCandidatelistPath;
		FILE *fileCandidatelist = fopen(pathCandidatelist.string().c_str(), "ab+");
		if (fileCandidatelist != NULL)
		{
			FileCommit(fileCandidatelist);
		}
		fclose(fileCandidatelist);
		
		//The snapshot file height
		uint64_t u64SnapshotFileSize = m_mapSnapshotIndex[nChainHeight +1];
		//snapshotIndex file height
		int nTruncateNum = nAllSnapshotSize - nChainHeight;

		std::map<uint32_t, uint64_t>::iterator iterIndex = m_mapSnapshotIndex.find(nChainHeight+1);
		if (iterIndex != m_mapSnapshotIndex.end())
		{
			m_mapSnapshotIndex.erase(iterIndex, m_mapSnapshotIndex.end());
		}
		
		//Remove the redundant snapshot list
		std::map<uint32_t, SnapshotClass>::iterator iterSnapShot = snapshotlist.find(nChainHeight + 1);
		if (iterSnapShot != snapshotlist.end())
		{
			snapshotlist.erase(iterSnapShot, snapshotlist.end());
		}
		
		//Truncate the SnapshotIndex file
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

		//Truncate the Snapshot file
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

		LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] chainActive height<Snapshot height,Processing is complete\n");
	}
	else
	{
		LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] is called\n");

		CBlockIndex* pblockindex = NULL; 
		{
			LOCK(cs_main);
			if (chainActive.Genesis() == NULL)
			{
				LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] chainActive.Genesis()is true ，return true\n");
				analysisfinished = true;
				return true;
			}

			pblockindex = chainActive.Genesis();
		}
	
		while (NULL != pblockindex)
		{
			boost::this_thread::interruption_point();

			if (( pblockindex->nHeight <= nAllSnapshotSize)&& nAllSnapshotSize !=0)
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

			LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] ready to push block height = %d\n", pblockindex->nHeight);
			if (!pushDPOCBlock(shared_pblock, pblockindex->nHeight))
				return false;
		
			{
				LOCK(cs_main);
				pblockindex = chainActive.Next(pblockindex);
			}
		}
	}

	analysisfinished = true;
	LogPrintf("[CConsensusAccountPool::analysisConsensusSnapshots] end by The local block is completed and returned correctly\n");
	return true;
}

bool CConsensusAccountPool::SetConsensusStatus(const std::string &strStatus, const std::string &strHash)
{
	std::string strPublicKey;
	CDpocInfo::Instance().getLocalAccoutVar(strPublicKey);

	bool bRet = false;
	if ((strPublicKey == strHash)&&(!strPublicKey.empty()))
	{
	    bRet = CDpocInfo::Instance().SetConsensusStatus(strStatus, strPublicKey);
	}

	LogPrintf("[CConsensusAccountPool::SetConsensusStatus] public key %s,Set the consensus return value %d\n", strPublicKey, bRet);
}

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
		
		uint64_t u64Seek = m_u64ConsensAccountSize;
		if (u64fileSize >= u64Seek)
		{
			for (int nIndex = 0; nIndex <= u64fileSize - u64Seek; nIndex += u64Seek)
			{
				CConsensusAccount account;
				CSerializeDpoc<CConsensusAccount> serializeAccount;
				serializeAccount.ReadFromDisk(account, nIndex, m_strCandidatelistPath);
				candidatelist.push_back(account);
			}
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
	boost::filesystem::path pathTmp = GetDataDir() / m_strSnapshotPath;
	FILE *file = fopen(pathTmp.string().c_str(), "ab+");
	if (file != NULL)
	{
		FileCommit(file);
	}
	fclose(file);

	boost::filesystem::path pathTmpIndex = GetDataDir() / m_strSnapshotIndexPath;
	FILE *fileIndex = fopen(pathTmpIndex.string().c_str(), "ab+");
	if (fileIndex != NULL)
	{
		FileCommit(fileIndex);
	}
	fclose(fileIndex);

	boost::filesystem::path pathCandidatelist = GetDataDir() / m_strCandidatelistPath;
	FILE *fileCandidatelist = fopen(pathCandidatelist.string().c_str(), "ab+");
	if (fileCandidatelist != NULL)
	{
		FileCommit(fileCandidatelist);
	}
	fclose(fileCandidatelist);
}

bool CConsensusAccountPool::verifyBlockSign(const CBlock *pblock)
{
	CPubKey recvPublickey;
	std::vector<unsigned char>  recvSign;
	if (!getPublicKeyFromBlock(pblock, recvPublickey, recvSign)) {
		LogPrintf("[CConsensusAccountPool::verifyBlockSign] getPublicKeyFromBlock is badlly\n");
		return false;
	}
	CKeyID  pubicKey160hash = recvPublickey.GetID();

	uint256 hash;
	CAmount nfee = pblock->vtx[0]->GetValueOut();
	CHash256 hashoperator;

	hashoperator.Write((unsigned char*)&(pblock->nPeriodStartTime), sizeof(pblock->nPeriodStartTime));//8Byte
	hashoperator.Write((unsigned char*)&(pblock->nPeriodCount), sizeof(pblock->nPeriodCount));//4Byte
	hashoperator.Write((unsigned char*)&(pblock->nTimePeriod), sizeof(pblock->nTimePeriod));//4Byte
	hashoperator.Write((unsigned char*)&(pblock->nTime), sizeof(pblock->nTime));//4Byte
	hashoperator.Write((unsigned char*)&nfee, sizeof(nfee));//8Byte
	hashoperator.Write((unsigned char*)pblock->GetBlockHeader().hashPrevBlock.begin(), 32);//256bit
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
	memcpy(dataBuff + datalen, (unsigned char*)pblock->GetBlockHeader().hashPrevBlock.begin(), 32);//256 bits
	datalen += 32;
	std::string datahex;
	for (int i = 0; i < datalen; i++)
	{
		char tmp[3];
		sprintf(tmp, "%02x", dataBuff[i]);
		datahex = datahex + std::string(tmp);
	}
	LogPrintf("[CConsensusAccountPool::verifyBlockSign] Attestation clear=%s , clear HASH=%s , publickey HASH=%s\n", datahex.c_str(), hash.GetHex().c_str(), pubicKey160hash.GetHex().c_str());

	if (recvPublickey.Verify(hash, recvSign)) {
		LogPrintf("[CConsensusAccountPool::verifyBlockSign]  recvPublickey.Verify  recvSign is OK\n");
	}
	else {
		LogPrintf("[CConsensusAccountPool::verifyBlockSign] The bulk information check failed!The block is forged, unable to verify identity, and cannot be penalized\n");
		return false;
	}
	
	LogPrintf("[CConsensusAccountPool::verifyBlockSign] end true\n");
	return true;
}

void CConsensusAccountPool::getTrustList(std::vector<std::string> &trustList)
{
	std::copy(trustPKHashList.begin(), trustPKHashList.end(), std::back_inserter(trustList));
}

bool CConsensusAccountPool::getSignPkByBlockIndex(const CBlockIndex *pindexTest, CKeyID  &pubicKey160hash)
{
	std::shared_ptr<CBlock> pblockNew = std::make_shared<CBlock>();
	if (!ReadBlockFromDisk(*pblockNew, pindexTest, Params().GetConsensus()))
	{
		LogPrintf("[GetTrustNodeHeightOnChain] end by false ReadBlockFromDisk return false\n");
		return false;
	}

	//Gets the public key and the signature string in the block
	CPubKey recvPublickey;
	std::vector<unsigned char>  recvSign;
	if (!getPublicKeyFromBlock(pblockNew.get(), recvPublickey, recvSign))
	{
		LogPrintf("[GetTrustNodeHeightOnChain] end by false getPublicKeyFromBlock return false\n");
		return false;
	}

	pubicKey160hash = recvPublickey.GetID();
	return true;
}

bool CConsensusAccountPool::getConsensusListByBlock(const CBlock& block, std::set<uint16_t> &setAccounts, std::list<std::shared_ptr<CConsensusAccount >> &consensusList)
{
	int64_t curStarttime = block.nPeriodStartTime / 1000;
	SnapshotClass meetingStartSnapshot;
	if (!GetSnapshotByTime(meetingStartSnapshot, curStarttime))
	{
		LogPrintf("[CConsensusAccountPool::getConsensusListByBlock] A snapshot of the start time of this session failed\n");
		return false;
	}

	uint64_t meetingstarttime = meetingStartSnapshot.meetingstoptime;
	int foundcachedcount = CACHED_BLOCK_COUNT;
	if (meetingStartSnapshot.blockHeight < Params().CHECK_START_BLOCKCOUNT)
	{
		foundcachedcount = meetingStartSnapshot.blockHeight < CACHED_BLOCK_COUNT ? meetingStartSnapshot.blockHeight : CACHED_BLOCK_COUNT;
	}
	uint32_t cachedTime = (meetingStartSnapshot.meetingstoptime - foundcachedcount * BLOCK_GEN_TIME) / 1000;
	LogPrintf("[CConsensusAccountPool::getConsensusListByBlock] The calculated cache time is %d \n", cachedTime);

	SnapshotClass cachedsnapshot;
	if (!GetSnapshotByTime(cachedsnapshot, cachedTime))
	{
		LogPrintf("[CConsensusAccountPool::getConsensusListByBlock] The cache time corresponds to a snapshot failure, and the block information is problematic\n");
		return false;
	}

	setAccounts = cachedsnapshot.curCandidateIndexList;
	consensusList.clear();
	std::set<uint16_t>::iterator candidateit;
	for (candidateit = setAccounts.begin(); candidateit != setAccounts.end(); candidateit++)
	{
		std::shared_ptr<CConsensusAccount> tmpaccount = std::make_shared<CConsensusAccount>(candidatelist.at(*candidateit));
		consensusList.push_back(tmpaccount);
	}

	return true;
}

bool CConsensusAccountPool::checkPackagerInCurrentList(const CBlock& block)
{
	std::set<uint16_t> setAccounts;
	std::list<std::shared_ptr<CConsensusAccount >> consensusList;
	if (!getConsensusListByBlock(block, setAccounts, consensusList))
	{
		LogPrintf("[CConsensusAccountPool::checkPackagerInCurrentList] end by false getConsensusListByBlock return false\n");
		return  false;
	}

	CPubKey recvPublickey;
	std::vector<unsigned char>  recvSign;
	if (!getPublicKeyFromBlock(&block, recvPublickey, recvSign))
	{
		LogPrintf("[CConsensusAccountPool::checkPackagerInCurrentList] end by false getPublicKeyFromBlock return false\n");
		return false;
	}
	CKeyID pubicKey160hash = recvPublickey.GetID();

	std::list<std::shared_ptr<CConsensusAccount >>::iterator iter = consensusList.begin();
	for (; iter != consensusList.end(); ++iter)
	{
		if (pubicKey160hash == (*iter)->getPubicKey160hash())
		{
			LogPrintf("[CConsensusAccountPool::checkPackagerInCurrentList] end by true\n");
			return true;
		}
	}

	LogPrintf("[CConsensusAccountPool::checkPackagerInCurrentList] end by false\n");
	return false;
}

bool CConsensusAccountPool::setTotalAmount(const CBlock &block,bool bAdd)
{
	CPubKey recvPublickey;
	std::vector<unsigned char>  recvSign;
	if (!getPublicKeyFromBlock(&block, recvPublickey, recvSign)) {

		LogPrintf("[CConsensusAccountPool::setTotalAmount] getPublicKeyFromBlock return false\n");
		return false;
	}
	CKeyID  pubicKey160hash = recvPublickey.GetID();

	std::string strPublickey;
	CDpocInfo::Instance().getLocalAccoutVar(strPublickey);
	if (pubicKey160hash.GetHex() == strPublickey)
	{
		int64_t n64Amount = CDpocInfo::Instance().GetTotalAmount();
		if (bAdd)
		{
			n64Amount += block.vtx[0]->vout[0].nValue;
		}
		else
		{
			n64Amount -= block.vtx[0]->vout[0].nValue;
		}

		CDpocInfo::Instance().SetTotalAmount(n64Amount);
		LogPrintf("[CConsensusAccountPool::setTotalAmount] return true at TotalAmount = %d\n", n64Amount);
	}
	return true;
}

bool CConsensusAccountPool::verifyPkInCandidateListByTime(int64_t curStarttime, CKeyID &pubicKey160hash)
{
	//Look for a snapshot of the current session's start time
	SnapshotClass meetingStartSnapshot;
	if (!GetSnapshotByTime(meetingStartSnapshot, curStarttime))
	{
		LogPrintf("[CConsensusAccountPool::verifyPkInCandidateList] A snapshot of the start time of this session failed\n");
		return false;
	}

	uint64_t meetingstarttime = meetingStartSnapshot.meetingstoptime;
	int foundcachedcount = CACHED_BLOCK_COUNT;
	if (meetingStartSnapshot.blockHeight < Params().CHECK_START_BLOCKCOUNT)
	{
		foundcachedcount = meetingStartSnapshot.blockHeight < CACHED_BLOCK_COUNT ? meetingStartSnapshot.blockHeight : CACHED_BLOCK_COUNT;
	}
	uint32_t cachedTime = (meetingStartSnapshot.meetingstoptime - foundcachedcount * BLOCK_GEN_TIME) / 1000;
	LogPrintf("[CConsensusAccountPool::verifyPkInCandidateList] The calculated cache time is %d \n", cachedTime);

	SnapshotClass cachedsnapshot;
	if (!GetSnapshotByTime(cachedsnapshot, cachedTime))
	{
		LogPrintf("[CConsensusAccountPool::verifyPkInCandidateList] Take the cache time corresponding to the snapshot failure, block information has a problem\n");
		return false;
	}

	LogPrintf("[CConsensusAccountPool::verifyPkInCandidateList] Find the height of the block= %d , Find the block time= %d \n",
		cachedsnapshot.blockHeight, cachedsnapshot.timestamp);

	std::set<uint16_t> accounts = cachedsnapshot.curCandidateIndexList;
	printsets(accounts);

	//If the current public key is on the blacklist, reject the block
	uint16_t pkindex;
	if (!ContainPK(pubicKey160hash, pkindex))
	{
		LogPrintf("[CConsensusAccountPool::verifyPkInCandidateList] Packaging public key unknown, reject\n");
		return false;
	}

	std::set<uint16_t>::iterator iter = cachedsnapshot.curCandidateIndexList.begin();
	for (; iter != cachedsnapshot.curCandidateIndexList.end(); ++iter)
	{
		if (pkindex == *iter)
		{
			LogPrintf("[CConsensusAccountPool::verifyPkInCandidateList] return true\n");
			return true;
		}
	}

	LogPrintf("[CConsensusAccountPool::verifyPkInCandidateList] return false\n");
	return  false;
}

bool CConsensusAccountPool::verifyPkInCandidateListByIndex(CBlockIndex *pIndex, CKeyID &pubicKey160hash)
{
	if (!verifyPkInCandidateListByTime(pIndex->nPeriodStartTime / 1000, pubicKey160hash))
	{
		LogPrintf("[CConsensusAccountPool::verifyPkInCandidateListByIndex] return false\n");
		return false;
	}

	LogPrintf("[CConsensusAccountPool::verifyPkInCandidateListByIndex] return true\n");
	return true;
}

bool CConsensusAccountPool::verifyPkInCandidateList(const std::shared_ptr<const CBlock> pblock, CKeyID  &pubicKey160hash)
{
	if (!verifyPkInCandidateListByTime(pblock->nPeriodStartTime / 1000, pubicKey160hash))
	{
		LogPrintf("[CConsensusAccountPool::verifyPkInCandidateList] return false\n");
		return false;
	}

	LogPrintf("[CConsensusAccountPool::verifyPkInCandidateList] return true\n");
	return true;
}

bool CConsensusAccountPool::verifyPkIsTrustNode(std::string strPublicKey)
{
	std::vector<std::string> vecTrustList;
	getTrustList(vecTrustList);
	std::vector<std::string>::iterator iterTrust = vecTrustList.begin();
	for (; iterTrust != vecTrustList.end(); ++iterTrust)
	{
		if (*iterTrust == strPublicKey)
		{
			return true;
		}
	}

	return false;
}

bool CConsensusAccountPool::verifyPkIsTrustNode(CKeyID  &pubicKey160hash)
{
	return verifyPkIsTrustNode(pubicKey160hash.GetHex());
}
