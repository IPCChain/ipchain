
#ifndef MEETING_ITEM_H
#define MEETING_ITEM_H
#include <stdint.h>
#include <list>
#include "ConsensusAccount.h"
#include "../chain.h"
#include <memory>


//#include "CarditConsensusMeeting.h"
//class CCarditConsensusMeeting;

//һ�������Ļ����Ҫ���Ӿ�������˳��ʶ�����������������ݼ�¼
class CMeetingItem
{
public:
	CMeetingItem();
	~CMeetingItem();
	CMeetingItem(CLocalAccount &localAccount, std::list<std::shared_ptr<CConsensusAccount>> &consensusList,int64_t nStartTime);
	CMeetingItem(const CMeetingItem & meetingItem);
	
	//����������ҹ�ʶ��˳������ÿ���ڵ㶼ͳһ��startHeight���ԣ�����������consensusList
	//ÿ�λ���ʱ���ڣ�����˳����һ����
	//��ʼ�������ϵ���
	void sortConsensusList();
	
	//��ʼ��ʶ����ʱ���ݶ���������������������߶�	
	void startConsensus();

	//��ǰ�Ƿ��ֵ��Ҵ����
	bool canPackage(int64_t nNow);

	bool GetHasPackage();
	void SetHasPackage(bool bSet);

	/**
	* ��ǰ�Ƿ������㱾�ֽ�������������������ѡһ���⼴��
	* 1���߶ȴﵽ�����߶�
	* 2������ʱ�����ڽ���
	*/
	bool canEnd(int64_t nNow);
	
	//��ȡ��ǰ�ֵ�ʱ��������Ҳ�����ɶ����˲��빲ʶ
	int getPeriodCount();
	
	//��ȡ�ҵĹ�ʶʱ��
	int getTimePeriod();

	//�жϱ��λ����Ҫ�Ƿ�Ϊ��
	bool isNull();

	int64_t getPeriodStartTime();
	void setPeriodStartTime(const int64_t periodStartTime);

	int64_t getPeriodEndTime();
	void setPeriodEndTime(const int64_t periodEndTime);

	int64_t getMyPackageTime();
	void setMyPackageTime(const int64_t myPackageTime);

	int64_t getMyPackageTimeEnd();
	void setMyPackageTimeEnd(const int64_t myPackageTimeEnd);

	int getMyPackageIndex();
	
	//�ÿ��Ƿ��Ǹ��ֹ�ʶ�����һ����
	bool isLastBlock(CBlockIndex* pBlockHeader);

	//�Ƿ��ڹ�ʶ�б���
	bool inConsensusList(uint160 uPublicKeyHash);
	bool inConsensusList();
	uint160 getCurrentMettingAccount();
	bool getOldPublicKey(int nTimePeriod, uint160 &retPublicKey);

	int getConsensusListSize();

	void GetConsensusList(std::list<std::shared_ptr<CConsensusAccount>> &conList);
	
public:
	//�ҵ�hash160
	uint160 myHash160;
private:
	int compare(std::shared_ptr<CConsensusAccount> &a1, std::shared_ptr<CConsensusAccount> &a2);
	uint256 getNewHash(const int64_t nTime, std::shared_ptr<CConsensusAccount> &account);

	std::list<std::shared_ptr<CConsensusAccount>> consensusList;

	//��ʶ����
	CLocalAccount localAccount;
	//���ֵ����о�����Ϣ����
	
	//���ֶ�Ӧ�Ŀ�ʼʱ��㣬��λ��
	int64_t nPeriodStartTime;
	//��һ�ֵ�ƫ��
	int64_t nDiffCount;
	//��ǰ�ֽ���ʱ�䣬��λ��
	int64_t nPeriodEndTime;
	//���ֵĹ�ʶ״̬
	int nStatus;

	//���Ƿ��Ѿ��������
	bool bHasPackage;
	//�ҵ�hash160
//	uint160 myHash160;
	//�ҵĴ��ʱ��
	int64_t nMyPackageTime;
	int64_t nMyPackageTimeEnd;
	//�ڼ����������0��ʼ��
	int nIndex;

	//�Ƿ��ʼ�����
	bool bInit;

	//�����Ƿ��ڹ�ʶ�б���
	bool bInConsensusList;
	
};

/*class CompareAccount
{
public:

	bool operator()(CConsensusAccount a1, CConsensusAccount  a2)
	{
	

	}
};*/
 


#endif //MEETING_ITEM_H