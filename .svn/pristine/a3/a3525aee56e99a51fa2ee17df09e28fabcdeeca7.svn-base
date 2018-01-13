
#ifndef DPOC_INFO_H
#define DPOC_INFO_H

#define BOOST_SPIRIT_THREADSAFE

#include <iostream>
#include <string>
#include <sstream>
#include <fstream> 
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include<mutex>

#define REBOOTLABEL 0x1FFFFFFF

struct BlockInfo
{
	std::string strAccout;
	std::string strHight;
	std::string strFee;
	std::string strTime;
};


class CDpocInfo
{
public:
	static  CDpocInfo&  Instance();
	~CDpocInfo();
	
	//Get local account information
	bool GetLocalAccount(std::string &strPublicKey);
	//Parse out the block's information
	bool GetBlockInfo(std::vector<BlockInfo> &listInfo, int &nSum);
	
	//Set up your local account
	int SetLocalAccount(const std::string &strPublicKey);
	//Set block information and local account
	bool SetBlockInfo(const BlockInfo &stBlockInfo, const std::string &strPublicKey);
	//Delete file info
	bool RemoveInfo();
	//There is already a strPublicKey value in the local file
	bool IsHasLocalAccount();
	bool ChangeFilePath(std::string &strPath);

	//Status in consensus
	//The normal state  0   strPunishHash is null
	//Common punishment 1   StrPunishHash is used to punish the public key HASH
	//Severe punishment 2   StrPunishHash is used to punish the public key HASH
	//Have a refund   Former state+3      StrPunishHash is used to punish the public key HASH
	bool GetConsensusStatus(std::string &strStatus, std::string &strPunishHash);
	bool SetConsensusStatus(const std::string &strStatus, const std::string &strPunishHash, const bool Remove = false);

	//Set the total amount of account
	bool SetTotalAmount(int64_t n64Amount);
	int64_t GetTotalAmount();

	void setJoinCampaign(bool bJoinCampaign);
	bool getJoinCampaign();
	//Sets the value of the local account in memory
	void setLocalAccoutVar(std::string strPublicKey);
	bool getLocalAccoutVar(std::string &strPublicKey);

	bool isReadStdCout();

private:
	void ReadFromFile(std::string &strJson);
	void WriteToFile(const std::string &strJson);
		
private:
	CDpocInfo();

	static void CreateInstance();
	static CDpocInfo* _instance;
	static std::once_flag init_flag;

	std::fstream infoFile;
	std::string strFilePath;
	const std::string strLocalFirst;
	const std::string strSumFirst;
	const std::string strListFirst;
	const std::string strAccoutFirst;
	const std::string strHightFirst;
	const std::string strFeeFirst;
	const std::string strTimeFirst;

	const std::string strPunishHashFirst;
	const std::string strStatusFirst;

	//Are you keeping an account
	bool m_bHasJoinCampaign;
	//The public key of charge to an account
	std::string m_strLocalAccout;
	boost::mutex  mutexPK;
	typedef boost::unique_lock<boost::mutex> PKLock;
	
	boost::mutex  rwmutex;
	typedef boost::unique_lock<boost::mutex> writeLock;

	boost::mutex  rwmutexAmount;
	typedef boost::unique_lock<boost::mutex> writeLockAmount;

	boost::mutex  mutexJoin;
	typedef boost::unique_lock<boost::mutex> joinLock;
};
#endif //DPOC_INFO_H
