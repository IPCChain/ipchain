
#ifndef BLOCK_FORK_H
#define BLOCK_FORK_H
#include <boost/shared_ptr.hpp>
#include <memory>
#include <boost/thread/thread.hpp>
#include <list>
#include <mutex>
#include "../primitives/block.h"
#include "BlockForkStore.h"
#include "../chain.h"

class CBlockForkService
{
public:	
	~CBlockForkService();

	//单例模式
	static  CBlockForkService&  Instance();

	//将块加入分叉待处理表中
	bool addBlockFork(const std::shared_ptr<const CBlock> &pblock);
	void scanning();
	bool processForkBlock(CBlockForkStore &blockForkStore);
	void monitor();

	void start();
	void stop();

private:
	static void CreateInstance();
	void removeBlock(std::list<CBlockForkStore>::iterator &iter);

	void removeInvaildBlock();
	void removeForkBlock();
	//根据块的前置Hash在分叉列表中找父块
	bool findPrvBlockInForkBlocks(const uint256 &prevHash, CBlockForkStore &retBlockFork);
	//处理回滚对列
	void processRollbackList();
	
private:
	int64_t nLocalBestHashLastTime;
	uint256 u256LocalBestHash;
	std::list<CBlockForkStore> m_listBlockFork;//存储的待处理的分叉块
	std::list<CBlockForkStore> m_listRollback;//待回滚分叉链
	CBlockIndex *m_pBlockIndex;
	
private:
	CBlockForkService();
	boost::thread thrdBlockForkService;

	static CBlockForkService* _instance;
	static std::once_flag init_flag;

	std::mutex forkMutex;
	//boost::shared_mutex  rwmutex;
	//typedef boost::shared_lock<boost::shared_mutex> readLock;
	//typedef boost::unique_lock<boost::shared_mutex> writeLock;
};

#endif 
