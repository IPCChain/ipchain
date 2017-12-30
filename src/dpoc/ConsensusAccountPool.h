#ifndef	   _IPCCHAIN_CONSENSUSACCOUNTPOOL_H_201708111059_
#define    _IPCCHAIN_CONSENSUSACCOUNTPOOL_H_201708111059_

#include <list>
#include <mutex>
#include <map>
#include <set>
#include <utility>
#include "ConsensusAccount.h"

/** 缓存块长度 */
static const int CACHED_BLOCK_COUNT = 10;
static const int MAX_TIMEOUT_COUNT = 3;	//连续不打块次数，超时惩罚
static const int MAX_BLOCK_TIME_DIFF = 5; //实际打块时间和计算时间最大差距，单位为秒
static const int PACKAGE_CREDIT_REWARD = 1; //每次记账获得的信用值奖励
static const int TIMEOUT_CREDIT_PUNISH = 100;//每次漏记账的信用值惩罚
static const int MIN_PERIOD_COUNT = 20;		//动态调整押金，会议（MAX[n千, 当前最高块]）记账人数小于该数目的时候，押金为最小押金
static const int MAX_PERIOD_COUNT = 100;	//动态调整押金，会议记账人数到达该数目的时候，押金数目要达到IPC发行量，曲线为 (x/最大人数)平方或四次方

class SnapshotClass {
public:
	//bool changed;					//用于后续压缩，快照列表如没有变化则为false
	//SnapshotClass* referSnapshot;	//如changed为空，则指向其引用的快照项，否则指向自身

	uint16_t pkHashIndex;			//记录对应块的打块公钥索引
	uint32_t blockHeight;			//记录对应块的块高度
	uint64_t timestamp;				//记录对应块的打块时间（准，秒级）
	uint64_t meetingstarttime;		//记录对应块的会议起始时间
	uint64_t meetingstoptime;		//记录对应块的会议截止时间
	uint32_t blockTime;				//记录对应块的实际打包时间（有变动）

	std::set<uint16_t> curCandidateIndexList;			//当前轮候选人列表（索引值，accountlist）
	std::set<uint16_t> curSeriousPunishIndexAdded;		//当前轮新增须严重处罚（索引值，不累计）
	std::map<uint16_t, int> curTimeoutIndexRecord;		//当前轮更新后的timeout记录（索引，连续不打块次数）
	std::set<uint16_t> curRefundIndexList;				//当前轮新增需退款（索引值，不累计）
	std::set<uint16_t> curTimeoutPunishList;			//当前轮新增需超时惩罚（索引值，不累计）
	std::set<uint16_t> curBanList;						//当前轮黑名单（索引值，不累计）

	std::set<uint16_t> cachedIndexsToRefund;			//从N轮前的cur列表和上一轮的状态，以及本次收到的块中包括的交易，共同计算得到打包/校验下一个块所需的退款index
	std::set<uint16_t> cachedTimeoutPunishToRun;		//从上一轮的状态，以及本次收到的块中包括的交易，共同计算得到打包/校验下一个块所需的退款index
	std::vector<std::pair<uint16_t, int64_t>> cachedMeetingAccounts;	//缓存的当前轮记账人列表（根据push的时候计算得到的候选人列表排序后得出）
	std::set<uint16_t> cachedBanList;						//当前轮黑名单（索引值，累计，永不清空）

	SnapshotClass();
	SnapshotClass(const SnapshotClass& in);
	~SnapshotClass() {};

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(pkHashIndex);
		READWRITE(blockHeight);
		READWRITE(timestamp);
		READWRITE(meetingstarttime);
		READWRITE(meetingstoptime);
		READWRITE(blockTime);

		READWRITE(curCandidateIndexList);
		READWRITE(curSeriousPunishIndexAdded);
		READWRITE(curTimeoutIndexRecord);
		READWRITE(curRefundIndexList);
		READWRITE(curTimeoutPunishList);
		READWRITE(curBanList);

		READWRITE(cachedIndexsToRefund);
		READWRITE(cachedTimeoutPunishToRun);
		READWRITE(cachedMeetingAccounts);
		READWRITE(cachedBanList);
	}

	bool operator==( const SnapshotClass& other)
	{
		/*if (changed)
		{
			return (referSnapshot->curCandidateIndexList == other.curCandidateIndexList &&
				referSnapshot->curSeriousPunishIndexAdded == other.curSeriousPunishIndexAdded &&
				referSnapshot->curTimeoutIndexRecord == other.curTimeoutIndexRecord &&
				referSnapshot->curRefundIndexList == other.curRefundIndexList &&
				referSnapshot->curTimeoutPunishList == other.curTimeoutPunishList &&
				referSnapshot->curBanList == other.curBanList &&
				referSnapshot->cachedIndexsToRefund == other.cachedIndexsToRefund &&
				referSnapshot->cachedTimeoutPunishToRun == other.cachedTimeoutPunishToRun &&
				referSnapshot->cachedMeetingAccounts == other.cachedMeetingAccounts &&
				referSnapshot->cachedBanList == other.cachedBanList);
		}
		else
		{*/
			return (curCandidateIndexList == other.curCandidateIndexList &&
				curSeriousPunishIndexAdded == other.curSeriousPunishIndexAdded &&
				curTimeoutIndexRecord == other.curTimeoutIndexRecord &&
				curRefundIndexList == other.curRefundIndexList &&
				curTimeoutPunishList == other.curTimeoutPunishList &&
				curBanList == other.curBanList &&
				cachedIndexsToRefund == other.cachedIndexsToRefund &&
				cachedTimeoutPunishToRun == other.cachedTimeoutPunishToRun &&
				cachedMeetingAccounts == other.cachedMeetingAccounts &&
				cachedBanList == other.cachedBanList);
		//}
	}

	void compressclear() {
		curCandidateIndexList.clear();
		curSeriousPunishIndexAdded.clear();
		curTimeoutIndexRecord.clear();
		curRefundIndexList.clear();
		curTimeoutPunishList.clear();
		curBanList.clear();
		cachedIndexsToRefund.clear();
		cachedTimeoutPunishToRun.clear();
		cachedMeetingAccounts.clear();
		cachedBanList.clear();
	};
};

enum DPOC_errtype {
	EXIT_PUBKEY_NOT_EXIST_IN_LIST = 0,
	EXIT_UNKNOWN_PUBKEY,
	JOIN_PUBKEY_IS_DEPOSING,
	JOIN_PUBKEY_IS_TIMEOUT_PUNISHED,
	GET_CACHED_SNAPSHOT_FAILED,
	EXIT_PUBKEY_IS_DEPOSING,
	EXIT_PUBKEY_IS_TIMEOUT_PUNISHED,
	JOIN_PUBKEY_ALREADY_EXIST_IN_LIST,
	JOIN_TRANS_DEPOSI_TOO_SMALL,
	GET_LAST_SNAPSHOT_FAILED,
	EVIDENCE_ISEMPTY,
	JOIN_PUBKEY_IS_BANNED,
	UNKNOWN_PACKAGE_PUBKEY,
	BLOCK_TOO_NEW_FOR_SNAPSHOT,
	PUNISH_BLOCK,
	PUBKEY_IS_BANNED,
};

class CConsensusAccountPool {

public:
	bool verifyDPOCTx(const CTransaction& tx, DPOC_errtype &errorType);
	bool checkNewBlock(const std::shared_ptr<const CBlock> pblock, uint160 &pubkeyHash, uint32_t blockHeight, DPOC_errtype & errorType);
	bool verifyDPOCBlock(const std::shared_ptr<const CBlock> pblock, uint32_t blockHeight, DPOC_errtype &rejectreason);

	bool pushDPOCBlock(const std::shared_ptr<const CBlock> pblock, uint32_t blockHeight);
	bool popDPOCBlock(uint32_t blockHeight);

	//根据当前列表的缓存，将退款、惩罚交易添加到block中，并对block签名
	bool AddDPOCCoinbaseToBlock(CBlock* pblockNew, CBlockIndex* pindexPrev, uint32_t blockHeight, CMutableTransaction &coinbaseTx);
	//签名函数
	bool addSignToCoinBase(CBlock* pblock, CBlockIndex* pindexPrev, uint32_t blockHeight, CMutableTransaction &coinbaseTx, uint160* pPubKeyHash);

	// 分析收到块的CoinBase取得，Publickey ，以及签名字段
	bool getPublicKeyFromBlock(const CBlock *pblock, CPubKey& outPubkey, std::vector<unsigned char>& outRecvSign);
	bool getPublicKeyFromSignstring(const std::string signstr, CPubKey& outPubkey, std::vector<unsigned char>& outRecvSign);

	//打块人和打块人公钥，查找，压金信用值
	bool ContainPK(uint160 pk, uint16_t& index);
	bool contain(std::vector<std::pair<uint16_t, int64_t>> list, uint16_t indexIn);
	
	bool getCreditFromSnapshotByIndex(SnapshotClass snapshot, const uint16_t indexIn, int64_t &credit);

	//add by xxy
	bool IsAviableUTXO(const uint256 hash);    //判断 申请加入共识的那笔押金是否解冻
	CAmount GetDepositBypkhash(uint160 pkhash);
	uint256 GetTXhashBypkhash(uint160 pkhash);
	bool PunishBlock(const std::shared_ptr<const CBlock> pBadBlock, uint32_t badBlockHeight);

	//快照相关的读写操作，之后要加锁
	bool GetSnapshotByTime(SnapshotClass &snapshot, uint64_t readtime);//获取计划打块时间不晚于传入时间的最后一个快照，单位为秒
	//获取指定打块时间在[start, stop]之间的快照列表，默认一直获取到快照结尾
	bool GetSnapshotsByTime(std::vector<SnapshotClass> &snapshots, uint64_t starttime, uint64_t stoptime=0);

	bool GetSnapshotByHeight(SnapshotClass &snapshot, uint32_t height);//获取块高度与指定高度相同的快照
	//获取高度在[lowest, highest]之间的快照列表，默认一直获取到快照结尾
	bool GetSnapshotsByHeight(std::vector<SnapshotClass> &snapshots, uint32_t lowest, uint32_t highest = 0);

	bool GetLastSnapshot(SnapshotClass &snapshot);//获取当前快照列表末尾的快照

	bool PushSnapshot(SnapshotClass snapshot);//将快照添加到快照列表末尾
	bool RemoveHigherSnapshotByHeight(uint32_t height);//从快照列表中移除所有高于和等于height的快照
	bool RemoveHigherSnapshotByTime(uint64_t timestamp);//从快照列表中移除所有计划打块时间不早于timestamp的快照，单位为秒

	bool getPKIndexBySortedIndex(uint16_t &pkIndex, uint160 &pkhash, std::list<std::shared_ptr<CConsensusAccount >> consensusList, int sortedIndex);
	bool GetTimeoutIndexs(const std::shared_ptr<const CBlock> pblock, SnapshotClass lastsnapshot, std::map<uint16_t, int>& timeoutindexs);

	CAmount GetCurDepositAdjust(uint160 pkhash,uint32_t blockheight);

	bool SetConsensusStatus(const std::string &strStatus, const std::string &strHash);

	uint64_t getCSnapshotIndexSize();
	uint64_t getSnapshotSize(SnapshotClass &snapshot);
	uint64_t getSnapshotIndexEnd();

	//共识列表记录文件
	void writeCandidatelistToFileByHeight(uint32_t nHeight);
	bool writeCandidatelistToFile(const CConsensusAccount &account);
	bool readCandidatelistFromFile();
	uint64_t getConsensAccountSize();
	void writeCandidatelistToFile();
	void flushSnapshotToDisk();

	bool rollbackCandidatelist(uint32_t nHeight);

	//校验块中签名
	bool verifyBlockSign(const CBlock *pblock);

	// 初始化的时候调用，调用之前需要清空
	bool analysisConsensusSnapshots();

	bool InitFinished()
	{
		return analysisfinished;
	};

	// add new block, 分析加入共识，退出共识, ...........等
	bool analysisNewBlock(const std::shared_ptr<const CBlock> pblock);
	

	// 会议调用该操作返回候选人列表
	bool listSnapshots(std::list<std::shared_ptr<CConsensusAccount>> &list);
	bool listSnapshotsToTime(std::list<std::shared_ptr<CConsensusAccount>> &list, int readtime);

	//获得可信列表
	void getTrustList(std::vector<std::string> &trustList);

	/************************************************************************/
	/* 后面的都是旧函数，择机清空                                              */
	/************************************************************************/
	//add a new consensusAccount by publickye
	bool  addConsensusAccountByPublicKey(uint160 &publicKeyHash, CAmount &joinIPC);
	bool  deleteConsenssusAccountByPublickey(uint160 &publicKeyHash);
	bool  contain(uint160& publicKeyHash);
    std::shared_ptr<CConsensusAccount> getConsensAccountByPublickeyHash(uint160 &publicKeyHash);
	void  debugStr();


	//普通的惩罚
	bool addCA2NormalViolationList(uint160 &publicKeyHash);
	bool containNormalViolationListByPublickey(uint160 &publicKeyHash);
	bool getNormalViolationLisSnapshots(std::list<std::shared_ptr<CConsensusAccount>> &listNormalViolation);
	void removeNormalViolationList();


	//严重的惩罚
	bool addCA2CriticalViolationList(uint160 &publicKeyHash);
	bool containCriticalViolationListByPublickey(uint160 &publicKeyHash);
	bool getCriticalViolationLisSnapshots(std::list<std::shared_ptr<CConsensusAccount>> &listCriticalViolation);
	void removeCriticalViolationList();

	//单例模式
	static  CConsensusAccountPool&  Instance();
	~CConsensusAccountPool();

private:
	void printsnapshots(std::vector<SnapshotClass> list);
	void printsets(std::set<uint16_t> list);

private:
	CConsensusAccountPool();
	
	static void CreateInstance();
	static CConsensusAccountPool* _instance;
	static std::once_flag init_flag;

	std::mutex poolMutex;

	// 存放已经加载 共识账号列表
	std::list<std::unique_ptr<CConsensusAccount>> m_listConsensusAccount;

	//生成黑名单，存放严重违规的节点
	std::list<std::unique_ptr<CConsensusAccount>> m_ListCriticalViolation;

	//生成普通惩罚的名单   -普通违规
	std::list<std::unique_ptr<CConsensusAccount>> m_ListNormalViolation;

	typedef boost::shared_lock<boost::shared_mutex> readLock;
	typedef boost::unique_lock<boost::shared_mutex> writeLock;
	boost::shared_mutex rwmutex;

	std::map<uint32_t, SnapshotClass> snapshotlist; //快照列表，key为块Height

	//add begin by li
	std::map<uint32_t, uint64_t> m_mapSnapshotIndex;
	std::string m_strSnapshotPath;
	std::string m_strSnapshotIndexPath;
	std::string m_strSnapshotRevPath;
	std::string m_strCandidatelistPath;

	uint64_t m_u64SnapshotIndexSize;
	uint64_t m_u64ConsensAccountSize;
	uint32_t m_u32WriteHeight;
	//add end by li

	std::vector<CConsensusAccount> candidatelist; //共识列表，存的共识人的HASH
													//std::set<uint16_t> tmpMinerList;
	std::set<uint16_t> tmpcachedIndexsToRefund;  //中间值
	std::set<uint16_t> tmpcachedTimeoutToPunish;
	//std::set<uint16_t> tmpcachedcurCandidateIndexList;
	bool verifysuccessed;
	bool analysisfinished;//分析本地块完成为真
	//启动用公钥，以及将来不会被超时惩罚出去的公钥列表
	std::vector<std::string> trustPKHashList; 	
};
#endif
