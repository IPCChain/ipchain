#include <iostream>
#include "../util.h"
#include "ConsensusAccount.h"

// 普通的构造函数
CConsensusAccount::CConsensusAccount(const uint160& pubkeyhash, CAmount& JoinIPC, const uint256& hash, int64_t credit)
{
	mPubicKey160hash =	pubkeyhash;
	mSortValue = uint256();
	miCredit = credit;
	txhash = hash;
	creditBackup = 0;

	mJoinIPC = JoinIPC;
}

CConsensusAccount::CConsensusAccount(const uint160& pubkeyhash, CAmount& JoinIPC, int64_t credit)
{
	mPubicKey160hash = pubkeyhash;
	mSortValue = uint256();
	miCredit = credit;
	txhash = uint256();
	creditBackup = 0;

	mJoinIPC = JoinIPC;
}

CConsensusAccount::CConsensusAccount(const uint160& pubkeyhash)
{
	mPubicKey160hash = pubkeyhash;
	mSortValue = uint256();
	miCredit = 0;
	mJoinIPC = 0;
}

CConsensusAccount::CConsensusAccount()
{
	miCredit = 0;
	creditBackup = 0;					   
	mJoinIPC = 0;
}


CConsensusAccount::CConsensusAccount(const CConsensusAccount* rhs) {
	this->mPubicKey160hash = rhs->mPubicKey160hash;
	this->mSortValue = rhs->mSortValue;
	this->miCredit = rhs->miCredit;
	this->mJoinIPC = rhs->mJoinIPC;
}

CConsensusAccount::~CConsensusAccount()
{
	debugOut();
	//std::cout << "[CConsensusAccount::~CConsensusAccount()  ]" << std::endl;
	LogPrintf("[[CConsensusAccount::~CConsensusAccount()]\n");
}
//
CConsensusAccount& CConsensusAccount::operator= (const CConsensusAccount& rhs) {
	if (this == &rhs){
		return *this;
	}

	this->mPubicKey160hash	= rhs.mPubicKey160hash;
	this->mSortValue		= rhs.mSortValue;
	this->miCredit			= rhs.miCredit;
	this->mJoinIPC			= rhs.mJoinIPC;

	return *this;
}

uint256 CConsensusAccount::getSortValue(){
	return mSortValue;
}
void CConsensusAccount::setSortValue(uint256 uSortValue) {
	 mSortValue = uSortValue;
}
uint160 CConsensusAccount::getPubicKey160hash() {
	return  mPubicKey160hash;
}

void CConsensusAccount::debugOut() {
	//std::cout << mPubicKey160hash.ToString();
	LogPrintf("%s ", mPubicKey160hash.GetHex().c_str());
}

CAmount CConsensusAccount::getJoinIPC() {
	return mJoinIPC;
}

void CConsensusAccount::backupCredit() { 
	LogPrintf("[CConsensusAccount::backupCredit] 备份当前信用值%d到备份值,覆盖原备份值%d\n", miCredit, creditBackup);
	creditBackup = miCredit; 
};
void CConsensusAccount::revertCredit() {
	LogPrintf("[CConsensusAccount::revertCredit] 从备份值%d恢复信用值，覆盖原信用值%d\n", creditBackup, miCredit);
	miCredit = creditBackup;
};

//##############################################################################33

CLocalAccount::CLocalAccount()
{
	std::string strPublicKey;
	
	CDpocInfo::Instance().GetLocalAccount(strPublicKey);
	pubicKey160hash.SetHex(strPublicKey);
}

bool CLocalAccount::IsNull()
{
	if (pubicKey160hash.IsNull())
	{
		return true;
	}
	return false;
}

int CLocalAccount::Get160Hash(uint160 &hash160)
{
	//std::string strPublicKey;
	//int nRet = CDpocInfo::Instance().GetLocalAccount(strPublicKey);
	//if (-1 == nRet)
	//{
	//	return -1;
	//}
	//pubicKey160hash.SetHex(strPublicKey);
	hash160 = pubicKey160hash;
	return 0;
}

int CLocalAccount::Set160Hash()
{
	std::string strPublicKey;
	//bool bRet = 
	CDpocInfo::Instance().GetLocalAccount(strPublicKey);
	//if (!bRet)
	//{
	//	return -1;
	//}
	pubicKey160hash.SetHex(strPublicKey);
	return 0;
}

CLocalAccount::CLocalAccount(const CLocalAccount& loaclAccount)
{
	pubicKey160hash = loaclAccount.pubicKey160hash;
}

