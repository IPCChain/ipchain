
 
#include "ForkUtil.h"

#include "validation.h"
#include "txmempool.h"
#include "consensus/validation.h"

#include "validationinterface.h"
#include "chain.h"
#include "chainparams.h"


#include "util.h"
#include "utiltime.h"


#include "net.h"
#include "scheduler.h"

//////////////////////////////////////////////////////////////////////////
//导入 函数  
enum FlushStateMode {
	FLUSH_STATE_NONE,
	FLUSH_STATE_IF_NEEDED,
	FLUSH_STATE_PERIODIC,
	FLUSH_STATE_ALWAYS
};

extern CTxMemPool mempool;
extern  bool AbortNode(CValidationState& state, const std::string& strMessage, const std::string& userMessage = "");
extern  bool  FlushStateToDisk(CValidationState &state, FlushStateMode mode, int nManualPruneHeight = 0);
extern  void  UpdateTip(CBlockIndex *pindexNew, const CChainParams& chainParams);


//////////////////////////////////////////////////////////////////////////

extern  std::unique_ptr<CConnman> g_connman;
CConnman::Options* pConnOptions;
CScheduler* pScheduler;
extern  bool InitError(const std::string& str);

//////////////////////////////////////////////////////////////////////////



CForkUtil*			CForkUtil::_instance = nullptr;
std::once_flag		CForkUtil::init_flag; 


void  CForkUtil::CreateInstance()
{
	static CForkUtil instance;
	CForkUtil::_instance = &instance;
}

CForkUtil& CForkUtil::Instance()
{
	std::call_once(CForkUtil::init_flag, CForkUtil::CreateInstance);
	return *CForkUtil::_instance;
}


CForkUtil::CForkUtil() 
{
}

CForkUtil::~CForkUtil()
{
}




// 回滚区块，以及区块中的交易   
bool CForkUtil::disConnectBlockIndex(CBlockIndex *pindexDelete)
{
	// 1， 检验指针
	if (NULL == pindexDelete  || nullptr ==pindexDelete){
		return false;
	}

	// Read block from disk.
	const CChainParams &chainparams  = Params();
	CValidationState  state;
	CBlock block;

	if (!ReadBlockFromDisk(block, pindexDelete, chainparams.GetConsensus())) {
		error("[disConnectBlockIndex]  ReadBlockFromDisk is badlly ");
		return false;
	}
		
	// Apply the block atomically to the chain state.
	int64_t nStart = GetTimeMicros();
	{
		CCoinsViewCache view(pcoinsTip);
		if (!DisconnectBlock(block, state, pindexDelete, view))
			return error("disConnectBlockIndex(): DisconnectBlock %s failed", pindexDelete->GetBlockHash().ToString());
		bool flushed = view.Flush();
		assert(flushed);
	}
	LogPrint("bench", "- Disconnect block: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);
	// Write the chain state to disk, if necessary.
	if (!FlushStateToDisk(state,  FLUSH_STATE_IF_NEEDED))
		return false;

	if (true) {
		// Resurrect mempool transactions from the disconnected block.
		std::vector<uint256> vHashUpdate;
		for (const auto& it : block.vtx) {
			const CTransaction& tx = *it;
			// ignore validation errors in resurrected transactions
			CValidationState stateDummy;
			if (tx.IsCoinBase() || !AcceptToMemoryPool(mempool, stateDummy, it, false, NULL, NULL, true)) {
				mempool.removeRecursive(tx, MemPoolRemovalReason::REORG);
			}
			else if (mempool.exists(tx.GetHash())) {
				vHashUpdate.push_back(tx.GetHash());
			}
		}
		// AcceptToMemoryPool/addUnchecked all assume that new mempool entries have
		// no in-mempool children, which is generally not true when adding
		// previously-confirmed transactions back to the mempool.
		// UpdateTransactionsFromBlock finds descendants of any transactions in this
		// block that were added back and cleans up the mempool state.
		mempool.UpdateTransactionsFromBlock(vHashUpdate);
	}

	// Update chainActive and related variables.
	UpdateTip(pindexDelete->pprev, chainparams);
	// Let wallets know transactions went from 1-confirmed to
	// 0-confirmed or conflicted:
	for (const auto& tx : block.vtx) {
		GetMainSignals().SyncTransaction(*tx, pindexDelete->pprev, CMainSignals::SYNC_TRANSACTION_NOT_IN_BLOCK);
	}
	return true;
}



// 重置网络
bool CForkUtil::resetNetWorking() 
{
	LogPrintf("[CForkUtil::resetNetWorking]   ........... \n");
	std::string strNodeError;
	CConnman& connman = *g_connman;

	LogPrintf("[CForkUtil::resetNetWorking]   Interrupt()........... \n");
	connman.Interrupt();

	LogPrintf("[CForkUtil::resetNetWorking]   connman.Stop();()........... \n");
	connman.Stop();

	LogPrintf("[CForkUtil::resetNetWorking]   connman.Start;()........... \n");
	if (!connman.Start(*pScheduler, strNodeError, *pConnOptions)) {
		LogPrintf("[CForkUtil::resetNetWorking]   after start .....%s \n", strNodeError);
		return InitError(strNodeError);
	}
	
	return true;
}

void CForkUtil::_startNetworking()
{
	std::string strNodeError;
	CConnman& connman = *g_connman;

	LogPrintf("[CForkUtil::_startNetworking]   connman.Start;()........... \n");
	if (!connman.Start(*pScheduler, strNodeError, *pConnOptions)) {
		LogPrintf("[CForkUtil::resetNetWorking]   after start .....%s \n", strNodeError);
	 InitError(strNodeError);
	}
}


void  CForkUtil::_stopNetWorking()
{
	std::string strNodeError;
	CConnman& connman = *g_connman;
	LogPrintf("[CForkUtil::_stopNetWorking]   Interrupt()........... \n");
	connman.Interrupt();

	LogPrintf("[CForkUtil::_stopNetWorking]   connman.Stop();()........... \n");
	connman.Stop();
}