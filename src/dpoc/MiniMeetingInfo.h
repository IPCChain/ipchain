
#ifndef  __IPCCHIAN_MinMeetingInfo_H_20170910__
#define  __IPCCHIAN_MinMeetingInfo_H_20170910__

#include "../hash.h"
#include "../uint256.h"
#include "../pubkey.h"

class CMiniMeetingInfo
{
public:
	CMiniMeetingInfo(uint160 pubkey160hash ,uint32_t nPeriodCount, uint64_t nPeriodStartTime,uint32_t nTimePeriod);
	CMiniMeetingInfo(const CMiniMeetingInfo* rhs);
	~CMiniMeetingInfo();
public:
	uint160   mPubicKey160hash;
	uint32_t  nPeriodCount;
	uint64_t  nPeriodStartTime;
	uint32_t  nTimePeriod;
};




#endif

