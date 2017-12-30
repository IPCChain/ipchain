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
	//����ģʽ
	static  CDpocInfo&  Instance();
	~CDpocInfo();
	
	//�õ������˺���Ϣ
	bool GetLocalAccount(std::string &strPublicKey);
	//������block����Ϣ
	bool GetBlockInfo(std::vector<BlockInfo> &listInfo, int &nSum);
	
	//���ñ����˺�
	int SetLocalAccount(const std::string &strPublicKey);
	//����block��Ϣ�ͱ����˺�
	bool SetBlockInfo(const BlockInfo &stBlockInfo, const std::string &strPublicKey);

	//ɾ���ļ���Ϣ
	bool RemoveInfo();
	//�Ƿ��ڱ����ļ����Ѿ���strPublicKeyֵ
	bool IsHasLocalAccount();
	bool ChangeFilePath(std::string &strPath);

	//��ʶ�е�״̬
	//����״̬ 0 strPunishHashΪ��
	//��ͨ�ͷ� 1 strPunishHashΪ���ͷ���ԿHASH
	//���سͷ� 2 strPunishHashΪ���ͷ���ԿHASH
	//�Ѿ��˿� ǰ״̬+3 strPunishHashΪ���ͷ���ԿHASH
	bool GetConsensusStatus(std::string &strStatus, std::string &strPunishHash);
	bool SetConsensusStatus(const std::string &strStatus, const std::string &strPunishHash, const bool Remove = false);

	//���ü����ܶ�
	bool SetTotalAmount(int64_t n64Amount);
	//�õ������ܶ�
	int64_t GetTotalAmount();

	void setJoinCampaign(bool bJoinCampaign);
	bool getJoinCampaign();

private:
	//���ļ��ж���Ϣ
	void ReadFromFile(std::string &strJson);
	//����Ϣд���ļ�
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

	//�Ƿ��ڼ���
	bool m_bHasJoinCampaign;

	boost::mutex  rwmutex;
	typedef boost::unique_lock<boost::mutex> writeLock;

	boost::mutex  rwmutexAmount;
	typedef boost::unique_lock<boost::mutex> writeLockAmount;

	boost::mutex  mutexJoin;
	typedef boost::unique_lock<boost::mutex> joinLock;
};


#endif //DPOC_INFO_H
