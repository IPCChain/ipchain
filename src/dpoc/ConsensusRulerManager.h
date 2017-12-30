
#ifndef  _IPCCHAIN_CONSENSUSRULERMANAGER_H_201708291527_
#define  _IPCCHAIN_CONSENSUSRULERMANAGER_H_201708291527_

#include <deque>
#include <memory.h>
#include "mutex"

#include "uint256.h"
#include "primitives/block.h"
#include "pubkey.h"
#include "bloom.h"
#include "MiniMeetingInfo.h"

struct CRulerItem {
	uint160  pubkeyhash;
	//��ʱ�ι�ʶ������
	int32_t  nPeriodCount;
	//���ֹ�ʶ��ʼ��ʱ��㣬��
	int64_t  nPeriodStartTime;
	// ʱ�Σ�һ�ֹ�ʶ�еĵڼ���ʱ��Σ�������֤�Ĺ�ʶ����
	int32_t  nTimePeriod;
	int32_t  nTimers;
};



class CConsensuRulerManager
{
public:

	/************************************************************************/
	/* �����Ǿɵķ���                                                         */
	/************************************************************************/

	// �����յ��¿�
	bool analysisRecvNewBlock(const std::shared_ptr<const CBlock> pblock);

	//��ӳͷ� ������ ,��CoinBase 
	bool  addRulersToCoinbase(CBlock* pblock ,CMutableTransaction &coinbaseTx, std::shared_ptr<CMiniMeetingInfo> ptrMiniMeetingInfo);

	//����ģʽ
	static  CConsensuRulerManager&  Instance();

	// �����յ����CoinBaseȡ�ã�Publickey ���Լ�ǩ���ֶ�
	bool getPublicKeyFromBlock(const CBlock *pblock, CPubKey& outPubkey, std::vector<unsigned char>& outRecvSign);
private:
	
	//���ǰ�浽 CoinBase��
	bool addSignToCoinBase(CMutableTransaction &coinbaseTx, uint160* pPubKeyHash);

	// ���  ��ͳһ����ʱ���յ��ر� �����سͷ�
	bool  checkRepeadBlock(const std::shared_ptr<const CBlock> pblock, uint160& pubkeyHash);

	// ��ͨ�������� 
	bool  checkNewBlock(const std::shared_ptr<const CBlock> pblock, uint160 &pubkeyHash);
private:
	CConsensuRulerManager();
	~CConsensuRulerManager();

	std::unique_ptr<CBloomFilter>  pBloomFilter;

	std::mutex rulerMutex;
	static void CreateInstance();
	static CConsensuRulerManager* _instance;
	static std::once_flag init_flag;

	std::deque<std::unique_ptr<CRulerItem>> mQueueRecvItem;
};




#endif // !
