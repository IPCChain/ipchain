
#include "BlockForkService.h"
#include "../util.h"
#include "../chain.h"
#include "../validation.h"
#include "TimeService.h"
#include "BlockForkStore.h"
#include  <stdlib.h>
#include "../consensus/params.h"
#include "../consensus/validation.h"
#include "../consensus/consensus.h"
#include "../chainparams.h"
#include "DpocMining.h"
#include "ConsensusRulerManager.h"
#include "../consensus/merkle.h"
#include "core_io.h"
#include "ForkUtil.h"

//初始化 静态变量
CBlockForkService*  CBlockForkService::_instance = NULL;
std::once_flag CBlockForkService::init_flag;

CBlockForkService::CBlockForkService():nLocalBestHashLastTime(0), m_pBlockIndex(NULL)
{
}

CBlockForkService::~CBlockForkService()
{
}

void CBlockForkService::CreateInstance() {
	static CBlockForkService instance;
	CBlockForkService::_instance = &instance;
}

//单例模式
CBlockForkService&  CBlockForkService::Instance()
{
	std::call_once(CBlockForkService::init_flag, CBlockForkService::CreateInstance);
	return *CBlockForkService::_instance;
}

void CBlockForkService::start()
{
	thrdBlockForkService = boost::thread(boost::bind(&CBlockForkService::monitor, this));
}

void CBlockForkService::stop()
{
	thrdBlockForkService.interrupt();
	thrdBlockForkService.join();
}

bool CBlockForkService::addBlockFork(const std::shared_ptr<const CBlock> &pblock)
{
	LogPrintf("[CBlockForkService::addBlockFork] begin \n");
	//-----Test begin------------------------------------------------------------
	//得到块里的高度
	CTransactionRef pCoinbaseTx = pblock->vtx[0];
	CScript scriptSig = pCoinbaseTx->vin[0].scriptSig;

	std::string strHight = ScriptToAsmStr(scriptSig);
	std::string strLeft = strHight.substr(0, strHight.find_first_of(" "));
	int nblockForkHight = atoi(strLeft.c_str());

	std::cout << "[CBlockForkService::addBlockFork]--------------------" << nblockForkHight << std::endl;
	LogPrintf("[CBlockForkService::addBlockFork]---------------------%d\n", nblockForkHight);
	//------Test end-----------------------------------------------------------

	//writeLock  wtlock(rwmutex);
	std::unique_lock<std::mutex> lockFork(forkMutex);
	CBlockForkStore newBlockForkStore;
	newBlockForkStore.pblock = pblock;
	m_listBlockFork.push_front(newBlockForkStore);
	m_listBlockFork.unique();

	LogPrintf("[CBlockForkService::addBlockFork] end \n");
	return true;
}

void CBlockForkService::removeBlock(std::list<CBlockForkStore>::iterator &iterList)
{
	LogPrintf("[BlockForkService::discardBlock] begin \n");

	iterList = m_listBlockFork.erase(iterList);

	LogPrintf("[BlockForkService::discardBlock] end by no discardBlock \n");	
	return;
}

void CBlockForkService::removeInvaildBlock()
{
	LogPrintf("[CBlockForkService::removeInvaildBlock()] begin \n");
	
	//删除校验不通过节点
	std::list<CBlockForkStore>::iterator iter = m_listBlockFork.begin();
	for (; iter != m_listBlockFork.end(); ++iter)
	{
		if (iter->bInvaild)
		{
			iter = m_listBlockFork.erase(iter);
		}
	}
	
	LogPrintf("[CBlockForkService::removeInvaildBlock()] end \n");
	
	return;
}

void CBlockForkService::removeForkBlock()
{
	LogPrintf("[CBlockForkService::removeForkBlock()] begin \n");

	//删除加到主链上的分叉列表中的值
	std::list<CBlockForkStore>::iterator iterRollback = m_listBlockFork.begin();
	for (; iterRollback != m_listBlockFork.end(); ++iterRollback)
	{
		std::list<CBlockForkStore>::iterator iter = m_listBlockFork.begin();
		for (; iter != m_listBlockFork.end(); ++iter)
		{
			if ((*iterRollback) == (*iter))
			{
				iter = m_listBlockFork.erase(iter);
				break;
			}
		}
	}

	LogPrintf("[CBlockForkService::removeForkBlock()] end \n");

	return;
}

void CBlockForkService::monitor()
{
	LogPrintf("[CBlockForkService::monitor] begin \n");
	try
	{
		while (true)
		{
			boost::this_thread::interruption_point();
			
			MilliSleep(10000);
			scanning();
			
		}
		LogPrintf("[CBlockForkService::monitor] end \n");
	}
	catch (boost::thread_interrupted & errcod)
	{
		LogPrintf("[CBlockForkService::monitor] end by boost::thread_interrupted error \n");
		return;
	}
}

void CBlockForkService::scanning()
{
	LogPrintf("[CBlockForkService::monitor] begin \n");
	std::cout << "CBlockForkService::scanning" << std::endl;

	CBlockIndex* pBestBlockIndex = NULL;
	{
	   LOCK(cs_main);
	   pBestBlockIndex = chainActive.Tip();
	}

	if (NULL == pBestBlockIndex)
	{
	   LogPrintf("CBlockForkService::scanning 得到最新块出现错误");
	   return ;
	}

	//1、本地最新块的保持60秒不变，则重置网络
	if (!(pBestBlockIndex->GetBlockHash() == u256LocalBestHash)) {
	    u256LocalBestHash = pBestBlockIndex->GetBlockHash();
	    nLocalBestHashLastTime = timeService.GetCurrentTimeMillis();
	}
	else
	{
		if (timeService.GetCurrentTimeMillis() - nLocalBestHashLastTime > 6*BLOCK_GEN_TIME)
		{
			//达到条件，重置网络,调用宋工接口函数
			
			CForkUtil::Instance().resetNetWorking();
	
			LogPrintf("本地最新块的保持60秒不变，则重置网络");
			return ;
		}
	}

	//2、处理所有待处理分叉块列表
	std::unique_lock<std::mutex> lockFork(forkMutex);

	std::list<CBlockForkStore>::iterator iterList = m_listBlockFork.begin();
	for (;iterList != m_listBlockFork.end(); ++iterList)
	{
		//最多分析60次，就丢弃掉
		if (iterList->getProcessCount() > 60) {
			removeBlock(iterList);
		}
		else
		{
			iterList->addProcessCount();
			
			//找到分叉链，并且长度大于主链
			if (processForkBlock(*iterList))
			{
				//回滚分叉
				processRollbackList();

				removeInvaildBlock();
				removeForkBlock();
				iterList = m_listBlockFork.begin();
			}
			
			//删除没校验通过的块
			removeInvaildBlock();
			iterList = m_listBlockFork.begin();
		}
	}

	LogPrintf("[CBlockForkService::monitor] end \n");
	return ;
}



bool CBlockForkService::processForkBlock(CBlockForkStore &blockForkStore)
{
	LogPrintf("[CBlockForkService::processForkBlock] begin \n");

	std::cout << "CBlockForkService::processForkBlock****" << m_listBlockFork.size() << std::endl;
	//CBlock& blockFork = blockFork.getBlock();
	//std::vector<CTransactionRef> vtx = blockFork._pblock->vtx;
	const CBlock* pblockFork = blockForkStore.pblock.get();
	const CBlock& blockFork = *pblockFork;

	//1、计算本地最新区块和处理区块高度差
	//得到块里的高度
	CTransactionRef pCoinbaseTx= blockForkStore.pblock->vtx[0];
	CScript scriptSig = pCoinbaseTx->vin[0].scriptSig;
	
	std::string strHight = ScriptToAsmStr(scriptSig);
	std::string strLeft = strHight.substr(0,strHight.find_first_of(" "));
	int nblockForkHight = atoi(strLeft.c_str());
	
	std::cout << "scriptSig.height****************" << nblockForkHight << std::endl;
	
	CBlockIndex* pBestBlockIndex = NULL;
	{
		LOCK(cs_main);
		pBestBlockIndex = chainActive.Tip();
	}

	if (NULL == pBestBlockIndex)
	{
		LogPrintf("[CBlockForkService::processForkBlock] end by 得到最新块出现错误 \n");
		return false;
	}

	//最新区块-要处理区块高度>20,放弃处理此块
	if (abs(pBestBlockIndex->nHeight - nblockForkHight) > 20) {
		std::cout << "CBlockForkService::processForkBlock**pBestBlockIndex->nHeight - nblockForkHight begin**" << std::endl;
		blockForkStore.invaildBlock();
		std::cout << "CBlockForkService::processForkBlock**pBestBlockIndex->nHeight - nblockForkHight end**" << std::endl;
		
		LogPrintf("[CBlockForkService::processForkBlock] end by 最新区块与待处理区块高度相差大于20 \n");
		return false;
	}

	//****************块基础校验
	//2、Block校验Marke
	{
		std::cout << "CBlockForkService::processForkBlock**BlockMerkleRoot**00000" << std::endl;

		bool mutated;// = false;
		uint256 hashMerkleRoot2 = BlockMerkleRoot(blockFork, &mutated);
		if (blockFork.hashMerkleRoot != hashMerkleRoot2)
		{
			blockForkStore.invaildBlock();
			std::cout << "CBlockForkService::processForkBlock**BlockMerkleRoot22222**" << std::endl;
			LogPrintf("[CBlockForkService::processForkBlock] end by 待处理块Marke交易不通过 \n");
			return false;
		}
		std::cout << "CBlockForkService::processForkBlock**BlockMerkleRoot33333**" << std::endl;

		// Check for merkle tree malleability (CVE-2012-2459): repeating sequences
		// of transactions in a block without affecting the merkle root of a block,
		// while still invalidating it.
		if (mutated)
		{
			std::cout << "CBlockForkService::processForkBlock**BlockMerkleRoot4444**" << std::endl;
			blockForkStore.invaildBlock();
			LogPrintf("[CBlockForkService::processForkBlock] end by 待处理块Marke交易不通过 \n");
			return false;
		}
		std::cout << "CBlockForkService::processForkBlock**BlockMerkleRoot5555**" << std::endl;
	}

	//3、Size limits
	if (blockFork.vtx.empty() || blockFork.vtx.size() > MAX_BLOCK_BASE_SIZE || ::GetSerializeSize(blockFork, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) > MAX_BLOCK_BASE_SIZE)
	{
		std::cout << "CBlockForkService::processForkBlock**6666**" << std::endl;
		blockForkStore.invaildBlock();
		LogPrintf("[CBlockForkService::processForkBlock] end by Size limits is error \n");
		return false;
	}
	std::cout << "CBlockForkService::processForkBlock**77777777**" << std::endl;

	//4、coinbase校验
	//First transaction must be coinbase, the rest must not be
	if (blockFork.vtx.empty() || !blockFork.vtx[0]->IsCoinBase())
	{
		std::cout << "CBlockForkService::processForkBlock**888888**" << std::endl;

		LogPrintf("[CBlockForkService::processForkBlock] end by coinbase check is error \n");
		blockForkStore.invaildBlock();
		return false;
	}
	std::cout << "CBlockForkService::processForkBlock**9999999**" << std::endl;
	
	for (unsigned int i = 1; i < blockFork.vtx.size(); i++)
	{
		if (blockFork.vtx[i]->IsCoinBase())
		{
			LogPrintf("[CBlockForkService::processForkBlock] end by coinbase check is error \n");
			blockForkStore.invaildBlock();
			return false;
		}
	}
	std::cout << "CBlockForkService::processForkBlock**newnew000000000000**" << std::endl;


	//5、Block签名脚本校验
	//获取块中的公钥和签名字符串
	/*CPubKey recvPublickey;
	std::vector<unsigned char>  recvSign;
	if (!CConsensuRulerManager::Instance().getPublicKeyFromBlock(&blockFork, recvPublickey, recvSign)) {
		LogPrintf("getPublicKeyFromBlock is badlly \n");
		std::cout << "CBlockForkService::processForkBlock**newnew11111111**" << std::endl;
		discardBlock(blockForkStore);
		return ;
	}
    //CKeyID  pubicKey160hash = recvPublickey.GetID();
	std::cout << "CBlockForkService::processForkBlock**newnew222222**" << std::endl;
	// 签名
	std::string str = "IPCChain key verification\n";
	uint256 hash;
	CAmount nfee = blockFork.vtx[0]->vout[0].nValue;
	CHash256().Write((unsigned char*)str.data(), str.size()).Write((unsigned char*)&nfee, sizeof(nfee)).Finalize(hash.begin());
	if (recvPublickey.Verify(hash, recvSign)) {
		LogPrintf(" recvPublickey.Verify  recvSign is OK\n");
	}
	else {
		std::cout << "CBlockForkService::processForkBlock**newnew333333**" << std::endl;
		discardBlock(blockForkStore);
		LogPrintf("recvPublickey.Verify  recvSign is error\n");
		return ;
	}*/
	//Block签名脚本校验


	//6、是否是重复块
	{
		std::cout << "CBlockForkService::processForkBlock**newnew55555**"<< mapBlockIndex.size()<< std::endl;
		//if (mapBlockIndex.count(blockFork.GetHash()) > 0)
		uint256 hash = blockFork.GetHash();
		BlockMap::iterator it = mapBlockIndex.find(hash);
		if (it != mapBlockIndex.end())
		{
			std::cout << "CBlockForkService::processForkBlock**newnew55555" << std::endl;
			blockForkStore.invaildBlock();
			return false;
		}
	}


	std::cout << "CBlockForkService::processForkBlock**newnew66666**" << std::endl;
	std::cout << "----111-------" << blockFork.nPeriodStartTime + (blockFork.nTimePeriod + 1) * BLOCK_GEN_TIME << std::endl;
	std::cout << "----222-------" << timeService.GetCurrentTimeMillis() << std::endl;
	//7、如果分叉的块比当前时间还要快很多，则可能是恶意节点在诱导分叉，直接丢弃
	//问题************************相差多少秒
	//if ((blockFork.nPeriodStartTime + (blockFork.nTimePeriod + 1)*BLOCK_GEN_TIME)
	//	- timeService.GetCurrentTimeMillis() > 2 * BLOCK_GEN_TIME)
	if (blockFork.nTime - timeService.GetCurrentTimeMillis() > 2 * BLOCK_GEN_TIME)
	{
		std::cout << "CBlockForkService::processForkBlock**newnew77777**" << std::endl;
		blockForkStore.invaildBlock();
		return false;
	}
	std::cout << "CBlockForkService::processForkBlock**newnew8888**" << std::endl;

	//8、先找到该块的父块再说，如果找不到，则不处理
	uint256 prevHash = blockFork.hashPrevBlock;
	BlockMap::iterator iterMap = mapBlockIndex.find(prevHash);
	if (iterMap != mapBlockIndex.end())//在本地链中找到了父块
	{
		std::cout << "CBlockForkService::processForkBlock find father****" << std::endl;
		m_listRollback.push_back(blockForkStore);
		m_pBlockIndex = mapBlockIndex[prevHash];
		return true;
	}
	else// 本地链中没有找到父块
	{
		//在存储的分叉块中找到了父块
		CBlockForkStore retBlockFork;
		if (findPrvBlockInForkBlocks(prevHash, retBlockFork))
		{
			m_listRollback.push_back(blockForkStore);

			std::cout << "CBlockForkService::processForkBlock return processForkBlock****" << std::endl;
			LogPrintf("[CBlockForkService::processForkBlock] end by 本地链没有找到父块，在存储的分叉链中找到父块，递归直达找到链上的父块\n");
			return processForkBlock(retBlockFork);
		}
		else
		{
			LogPrintf("[CBlockForkService::processForkBlock] end by 本地链和存储的分叉块中都没有找到父块\n");
			return false;
		}
	}
	
	LogPrintf("[CBlockForkService::processForkBlock] end \n");
	return false;
}


bool CBlockForkService::findPrvBlockInForkBlocks(const uint256 &prevHash, CBlockForkStore &retBlockFork)
{
	LogPrintf("[CBlockForkService::findPrvBlockInForkBlocks] begin \n");
	//处理 所有待处理分叉块列表
	std::list<CBlockForkStore>::iterator iterFork = m_listBlockFork.begin();
	for (; iterFork != m_listBlockFork.end(); ++iterFork)
	{
		if (prevHash == (iterFork->pblock)->GetHash())
		{
			retBlockFork = *iterFork;
			return true;
		}
	}
	LogPrintf("[CBlockForkService::findInForkBlocks] end \n");
	return false;
}

void CBlockForkService::processRollbackList()
{
	LogPrintf("[CBlockForkService::ProcessRollbackList] begin \n");

	if (m_pBlockIndex == NULL)
	{
		LogPrintf("[CBlockForkService::ProcessRollbackList] end by m_pBlockIndex is NULL \n");
		return;
	}
	if (m_listRollback.size()<=0)
	{
		LogPrintf("[CBlockForkService::ProcessRollbackList] end by m_listRollback is NULL \n");
	    return;
	}

	CBlockIndex* pBestBlockIndex = NULL;
	{
		LOCK(cs_main);
		pBestBlockIndex = chainActive.Tip();
	}
	if (NULL == pBestBlockIndex)
	{
		LogPrintf("[CBlockForkService::ProcessRollbackList] end by 得到最新块出现错误 \n");
		return;
	}
	
	std::cout << "CBlockForkService::processForkBlock huigunqian****" << m_listRollback.size() << std::endl;

	//如果分叉链的高度 > 本地块的高度
	if (((m_pBlockIndex->nHeight + m_listRollback.size()) > pBestBlockIndex->nHeight)
		&& (2 < m_listRollback.size()))
	{
		std::cout << "CBlockForkService::processForkBlock huigunqian*11111111***" << std::endl;

		//回滚块和交易
		int nIndex = pBestBlockIndex->nHeight;
		while (nIndex > m_pBlockIndex->nHeight)
		{
			CBlockIndex* pBlockForkIndex = NULL;
			{
				LOCK(cs_main);
				pBlockForkIndex = chainActive[nIndex];
			}

			if (NULL == pBlockForkIndex)
			{
				LogPrintf("CBlockForkService::processForkBlock得到最新块出现错误");
				return;
			}

			std::cout << "CBlockForkService::processForkBlock huigunqian*22222222***" << std::endl;

			CForkUtil::Instance().disConnectBlockIndex(pBlockForkIndex);

			--nIndex;
		}

		//将ForkList加入主链
		//处理 所有待处理分叉块列表
		std::list<CBlockForkStore>::reverse_iterator iterFork = m_listRollback.rbegin();
		for (; iterFork != m_listRollback.rend(); ++iterFork)
		{
			std::cout << "*********** ProcessNewBlock************" << std::endl;
			//std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(iter);
			ProcessNewBlock(Params(), iterFork->pblock, true, NULL);
		}

		std::cout << "***********CDpocMining::Instance().reStart()************" << std::endl;

		//重置会议
		CDpocMining::Instance().reStart();
	}

	LogPrintf("[CBlockForkService::ProcessRollbackList] begin \n");
	return;
}

