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
//class CDpocInfo;

//extern CDpocInfo g_dpocInfo;

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
	//单例模式
	static  CDpocInfo&  Instance();
	~CDpocInfo();
	
	//得到本地账号信息
	bool GetLocalAccount(std::string &strPublicKey);
	//解析出block的信息
	bool GetBlockInfo(std::vector<BlockInfo> &listInfo, int &nSum);
	
	//设置本地账号
	int SetLocalAccount(const std::string &strPublicKey);
	//设置block信息和本地账号
	bool SetBlockInfo(const BlockInfo &stBlockInfo, const std::string &strPublicKey);

	//删除文件信息
	bool RemoveInfo();
	//是否在本地文件中已经有strPublicKey值
	bool IsHasLocalAccount();
	bool ChangeFilePath(std::string &strPath);

	//共识中的状态
	//正常状态 0 strPunishHash为空
	//普通惩罚 1 strPunishHash为被惩罚公钥HASH
	//严重惩罚 2 strPunishHash为被惩罚公钥HASH
	//已经退款 前状态+3 strPunishHash为被惩罚公钥HASH
	bool GetConsensusStatus(std::string &strStatus, std::string &strPunishHash);
	bool SetConsensusStatus(const std::string &strStatus, const std::string &strPunishHash, const bool Remove = false);

	//设置记账总额
	bool SetTotalAmount(int64_t n64Amount);
	//得到记账总额
	int64_t GetTotalAmount();

	void setJoinCampaign(bool bJoinCampaign);
	bool getJoinCampaign();

private:
	//从文件中读信息
	void ReadFromFile(std::string &strJson);
	//将信息写入文件
	void WriteToFile(const std::string &strJson);
		
private:
	CDpocInfo();

	static void CreateInstance();
	static CDpocInfo* _instance;
	static std::once_flag init_flag;

	std::fstream infoFile;
	//std::ofstream outFile;
	std::string strFilePath;
	//std::string &strJson;
	const std::string strLocalFirst;
	const std::string strSumFirst;
	const std::string strListFirst;
	const std::string strAccoutFirst;
	const std::string strHightFirst;
	const std::string strFeeFirst;
	const std::string strTimeFirst;

	const std::string strPunishHashFirst;
	const std::string strStatusFirst;

	//是否在记账
	bool m_bHasJoinCampaign;

	boost::mutex  rwmutex;
	typedef boost::unique_lock<boost::mutex> writeLock;

	boost::mutex  rwmutexAmount;
	typedef boost::unique_lock<boost::mutex> writeLockAmount;

	boost::mutex  mutexJoin;
	typedef boost::unique_lock<boost::mutex> joinLock;
};


#endif //DPOC_INFO_H
