
#include "BlockForkStore.h"

CBlockForkStore::CBlockForkStore():nProcessCount(0),bInvaild(false)
{
	pblock.reset();
}


CBlockForkStore::CBlockForkStore(const CBlockForkStore &block)
{
	nProcessCount = block.nProcessCount;
	bInvaild = block.bInvaild;
	pblock = block.pblock;
}

int CBlockForkStore::getProcessCount()
{
	return nProcessCount;
}

void CBlockForkStore::addProcessCount()
{
	++nProcessCount;
}

bool CBlockForkStore::operator==(const CBlockForkStore &pBlockForkStore) const
{
	if ((this->pblock->GetHash() == pBlockForkStore.pblock->GetHash())
		&&(this->bInvaild == pBlockForkStore.bInvaild))
	{
		return true;
	}
	else
	{
		return false;
	} 
}

void CBlockForkStore::invaildBlock()
{
	bInvaild = true;
}

