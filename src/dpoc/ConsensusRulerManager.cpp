
#include "ConsensusRulerManager.h"





#include <vector>
#include <string>

#include "base58.h"
#include "uint256.h"

#include "../util.h"
#include "../chain.h"
#include "../sync.h"
#include "../validation.h"

#include "MeetingItem.h"
#include "DpocMining.h"

#include "ViolationPool.h"
#include "ConsensusAccountPool.h"
#include "../primitives/transaction.h"
#include <iostream>

#include "wallet/wallet.h"
extern CWallet* pwalletMain;



//####################################################################
CConsensuRulerManager*  CConsensuRulerManager::_instance = nullptr;
std::once_flag CConsensuRulerManager::init_flag;


void CConsensuRulerManager::CreateInstance() {
	static CConsensuRulerManager instance;
	CConsensuRulerManager::_instance = &instance;
}

//单例模式
CConsensuRulerManager&  CConsensuRulerManager::Instance()
{
	std::call_once(CConsensuRulerManager::init_flag, CConsensuRulerManager::CreateInstance);
	return *CConsensuRulerManager::_instance;
}


CConsensuRulerManager::CConsensuRulerManager()
{
	pBloomFilter = std::unique_ptr<CBloomFilter>(new CBloomFilter(10000, 0.00001,0, BLOOM_UPDATE_ALL));
}

CConsensuRulerManager::~CConsensuRulerManager()
{
}

//############################################################


/************************************************************************/
/* 后面的是原有函数                                                       */
/************************************************************************/

//   共识会议,违规检测类， 严重违规类
//   收到，前面的会议成员的 收到的包 ；
//   如果，同一个时间点，收到会议成员，两个包，则为严重违规 ;
//   若果  2次，没有收到，

//const std::shared_ptr<const CBlock> pblock

bool CConsensuRulerManager::getPublicKeyFromBlock(const CBlock *pblock , CPubKey& outPubkey , 
			std::vector<unsigned char>&  outRecvSign)
{
	CTransactionRef coinbase = pblock->vtx[0];
	std::string strSignatrue = coinbase->vout[0].GetCheckBlockContent();
	if (strSignatrue.length() < 50)	{
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


bool CConsensuRulerManager::analysisRecvNewBlock(const std::shared_ptr<const CBlock> pblock)
{
	
	{
		int32_t chainHeight = 0;
		LOCK(cs_main);
		chainHeight = chainActive.Height();

		if (chainHeight <= 300) {
			return true;
		}
	}


	//static int64_t  nPeriodStartTime = 0;
	//static bool isReceiveLastBlock = false;
	//获取块中的公钥和签名字符串
	CPubKey recvPublickey;
	std::vector<unsigned char>  recvSign;
	if (!getPublicKeyFromBlock(pblock.get(), recvPublickey , recvSign)) {
		LogPrintf("[analysisRecvNewBlock] getPublicKeyFromBlock is badlly \n");
		return false;
	}
	CKeyID  pubicKey160hash = recvPublickey.GetID();

	// 签名
	std::string str = "IPCChain key verification\n";
	uint256 hash;
	CAmount nfee = pblock->vtx[0]->vout[0].nValue;
	CHash256().Write((unsigned char*)str.data(), str.size()).Write((unsigned char*)&nfee, sizeof(nfee)).Finalize(hash.begin());
	if (recvPublickey.Verify(hash, recvSign)) {
		LogPrintf("[analysisRecvNewBlock]  recvPublickey.Verify  recvSign is OK\n");
	}
	else {
		LogPrintf("[analysisRecvNewBlock]  recvPublickey.Verify  recvSign is error\n");
		return false;
	}

	//2 普通的检查
	if (! checkNewBlock(pblock ,pubicKey160hash)) {
		LogPrintf("[analysisRecvNewBlock]  checkNewBlock error\n");
		return false;
	}
	
	//3 重置过滤器
	if ( pblock->nTimePeriod == pblock->nPeriodCount -1 ) {
		LogPrintf("[analysisRecvNewBlock] finish a round , So need to reset CBloomFilter \n");
		pBloomFilter.release();
		pBloomFilter = std::unique_ptr<CBloomFilter>(new CBloomFilter(10000, 0.00001, 0, BLOOM_UPDATE_ALL));
		//isReceiveLastBlock = true;
	}
	

	//1, 检验是否是重块, 这个比较严重.....
	if (checkRepeadBlock(pblock, pubicKey160hash)) {
		LogPrintf("[analysisRecvNewBlock] checkRepeadBlock double block ,pubkeyhash=(%s) \n", pubicKey160hash.ToString().c_str());
		std::shared_ptr<CMiniMeetingInfo> ptr(new CMiniMeetingInfo(pubicKey160hash, pblock->nPeriodCount, pblock->nPeriodStartTime, pblock->nTimePeriod));
		CViolationPool::Instance().addSeriouViolationToCach(ptr);
		return false;
	}

	LogPrintf("[analysisRecvNewBlock]  scucessfully! \n");
	return true;
}


// #####################################################################
// 检验 ,  受到同一个时段，一个人发送了2个块.
// 如果检测重复，返回true ;如果没有重复在返回，flase
bool CConsensuRulerManager::checkRepeadBlock(const std::shared_ptr<const CBlock> pblock ,uint160 &pubkeyHash) 
{
	// 获取publicHash
	std::vector<unsigned char> key;
	key.resize(28);
	memcpy(&key[0], pubkeyHash.begin(), 20);

	// 时间
	uint64_t nPeriodStartTime = pblock->nPeriodStartTime;
	memcpy(&key[20], &nPeriodStartTime, sizeof(nPeriodStartTime));

	LogPrintf("[checkRepeadBlock] hash=(%s) nPeriodStartTime=(%ld)\n", pubkeyHash.ToString().c_str() , nPeriodStartTime);
	if (pBloomFilter->contains(key)) {
		//
		LogPrintf("[checkRepeadBlock]  check double block hash=(%s)\n" ,pubkeyHash.ToString().c_str());
		return  true;
	}
	
	pBloomFilter->insert(key);
	return false;
}


//###############################################################
// 普通的减产
bool  CConsensuRulerManager::checkNewBlock(const std::shared_ptr<const CBlock> pblock, uint160 &pubkeyHash )
{
	//  调用洪斌的接口，通过header的字段，查出publickeyhash160, 是否与publickey 得到hash160是否一致
	// 从李工的开会 获得 hash160
	 uint160    meetingHash160;   //从共识会议模块，来获得。
								  //添加测试版本带 不检查前200
	 int32_t chainHeight = 0;
	 {
		 LOCK(cs_main);
		 chainHeight = chainActive.Height(); 
	 }

	 int result = CDpocMining::Instance().getCurrentConsensusInfo(pblock->nPeriodStartTime, pblock->nTimePeriod, meetingHash160);
	 LogPrintf("[checkNewBlock] chainHeight=%d reslut=%d nPeriodStartTime=(%ld) nTimePeriod=(%d)\n" , chainHeight , result , pblock->nPeriodStartTime,pblock->nTimePeriod);
	 if (result > 0) {  //      存在会议中
		 if (meetingHash160 == pubkeyHash) { //
		 }
		 else {  // 不是同一个人，非法的包。
			 LogPrintf("[CConsensuRulerManager::checkNewBlock] mismatching meeting publickey hash\n");
			 return false;
		 }
	 }
	 else if (0 == result)  {  //  在缓存的中 没有查到
		 LogPrintf("[CConsensuRulerManager::checkNewBlock] didn't find publickey hash in 5round's meetinginfo\n");
		 return false;
	 }
	 else if (result < 0 )  {  // 已经被换成抛弃。
		 LogPrintf("[CConsensuRulerManager::checkNewBlock] losing in meeting info\n");
		 return true;
	 }

	 //2)   时间是不要校验。
	 uint32_t calcTime = (pblock->nPeriodStartTime + (pblock->nTimePeriod + 1) * 10000) / 1000;
	 if (pblock->nTime != calcTime) {
		 LogPrintf("[CConsensuRulerManager::checkNewBlock]  pblock->nTime != calcTime\n");
		 return false;
	 }

	

	// 校验CoinBase中的严重违规 ,核实自己是否存在 严重违规, 
	// 核实严重违规的合法性
	 CTransactionRef coinBases  = pblock->vtx[0];
	 for (unsigned int index = 0 ; index < coinBases->vout.size() ; ++index){
		 CTxOut  TXout = coinBases->vout[index];
		 if (TYPE_CONSENSUS_SEVERE_PUNISHMENT == TXout.GetCampaignType()) {
			 uint160 hash160 = TXout.devoteLabel.hash;
			 if (!CViolationPool::Instance().containInSeriouList(hash160)) { //误报
				 LogPrintf("[checkNewBlock] %s block contain <%s> punishment , but It'not in our pushment list ", pubkeyHash.ToString(), hash160.ToString());
				 
				 return false;
			 }
		 }
		 else  if (TYPE_CONSENSUS_ORDINARY_PUSNISHMENT == TXout.GetCampaignType())  { //
			 uint160 hash160 = TXout.devoteLabel.hash;
// 			 if (!CViolationPool::Instance().containInTimeoutList(hash160)) {  //误报
// 				 LogPrintf("[checkNewBlock] %s block contain <%s> Timeout punishment , but It'not in our pushment list ", pubkeyHash.ToString(), hash160.ToString());
// 				 return false;
// 			 }
		 }
		  

	 }

	return true;
}

bool CConsensuRulerManager::addSignToCoinBase(CMutableTransaction &coinbaseTx, uint160* pPubKeyHash)
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
			LogPrintf("[addSignToCoinBase] don't find public key ");
			return false;
		}

		CKey vchPrivKeyOut;
		if (pwalletMain->GetKey(publickeyID, vchPrivKeyOut)) {
		}
		else {
			LogPrintf("[addSignToCoinBase] don't find private key");
			return false;
		}

		std::string str = "IPCChain key verification\n";
		uint256 hash;
		CAmount nfee = coinbaseTx.vout[0].nValue;
		CHash256().Write((unsigned char*)str.data(), str.size()).Write((unsigned char*)&nfee, sizeof(nfee)).Finalize(hash.begin());

		std::vector<unsigned char> vchSig;
		vchPrivKeyOut.Sign(hash, vchSig);

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
		coinbaseTx.vout[0].coinbaseScript = strSin2Publickey;
		coinbaseTx.vout[0].txType = 0;
	}
	return true;
}

//添加规则 到Conibase中
bool  CConsensuRulerManager::addRulersToCoinbase(CBlock* pblock ,CMutableTransaction &coinbaseTx, std::shared_ptr<CMiniMeetingInfo> ptrMiniMeetingInfo)
{
	if (NULL ==ptrMiniMeetingInfo  || nullptr == ptrMiniMeetingInfo) {
		return false;
	}

	uint160 PubKeyHash = ptrMiniMeetingInfo->mPubicKey160hash;
	if (PubKeyHash.IsNull()) {
		return false;
	}

	if (!addSignToCoinBase(coinbaseTx, &PubKeyHash)) {
		return false;
	}
	
	//添加 超时违规的校验
	if (ptrMiniMeetingInfo ) {
		pblock->nPeriodStartTime = ptrMiniMeetingInfo->nPeriodStartTime;
	}
	CViolationPool::Instance().checkTimeoutMeeting(pblock);

	// 添加严重惩罚，和惩罚到coinBase
	if (! CViolationPool::Instance().addViolationToCoinBase(coinbaseTx ,ptrMiniMeetingInfo))	{
		return false;
	}



	//add by song  2017/8/16
	//在CoinBase 添加退款CTxOut 
	 for( unsigned int __index =1 ; __index < pblock->vtx.size() ; ++__index) {
		const CTransactionRef  aTransaction = pblock->vtx[__index];
		if (  aTransaction.get() && TYPE_CONSENSUS_QUITE == aTransaction->GetCampaignType()) // 退出交易
		{
			//检查是否有严重的惩罚，如果严重的处罚，则不进行退款
			uint160 pubKeyhash = aTransaction->GetCampaignPublickey();
			if (CViolationPool::Instance().containInSeriouList(pubKeyhash)) {
				LogPrintf("[addSignToCoinBase] %s is in SeriouViolationList, So can't back deposit\n", pubKeyhash.ToString());
			}
			else {
				LogPrintf("[addSignToCoinBase] %s back deposit\n", pubKeyhash.ToString());
				std::shared_ptr<CConsensusAccount> ptrAccount = CConsensusAccountPool::Instance().getConsensAccountByPublickeyHash(pubKeyhash);
				if (ptrAccount && ptrAccount.get()) {
					CTxOut out(ptrAccount->getJoinIPC(), pubKeyhash);
					coinbaseTx.vout.emplace_back(std::move(out));

					LogPrintf("[addSignToCoinBase] hash=%s back deposit IPC=(%ld)\n", pubKeyhash.ToString(), ptrAccount->getJoinIPC());
				}
			}
		}
	}


	return true;
}