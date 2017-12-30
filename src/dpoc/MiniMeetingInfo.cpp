
#include "MiniMeetingInfo.h"

CMiniMeetingInfo::CMiniMeetingInfo(uint160 pubkey160hash, uint32_t PeriodCount, uint64_t PeriodStartTime, uint32_t  TimePeriod)
{
	mPubicKey160hash = pubkey160hash;
	nPeriodCount = PeriodCount;
	nPeriodStartTime = PeriodStartTime;
	nTimePeriod = TimePeriod;
}

CMiniMeetingInfo::~CMiniMeetingInfo()
{

}



CMiniMeetingInfo::CMiniMeetingInfo(const CMiniMeetingInfo* rhs)
{
	this->mPubicKey160hash  = rhs->mPubicKey160hash;
	this->nPeriodCount		= rhs->nPeriodCount;
	this->nPeriodStartTime	= rhs->nPeriodStartTime;
	this->nTimePeriod = this->nTimePeriod;
}