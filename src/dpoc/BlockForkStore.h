//#pragma once

#ifndef BLOCK_FORK_STORE_H
#define BLOCK_FORK_STORE_H

#include "../primitives/block.h"
#include<memory>

class CBlockForkStore
{
public:
	CBlockForkStore();
	CBlockForkStore(const CBlockForkStore &block);

	int getProcessCount();
	void addProcessCount();
	bool operator==(const CBlockForkStore &ps) const;
	void invaildBlock();


	int nProcessCount;
	bool bInvaild;
	
	std::shared_ptr<const CBlock> pblock;
};

#endif