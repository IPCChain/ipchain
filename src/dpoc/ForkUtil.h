
#ifndef    __IPCCHIAN_CForkUtil_H_20170920__
#define    __IPCCHIAN_CForkUtil_H_20170920__


#include <list>
#include <mutex>


class  CBlockIndex;


class CForkUtil
{
public:
	//回归当前块，
	bool  disConnectBlockIndex(CBlockIndex *pblockIndex); 

	//重置网络 
	bool     resetNetWorking();

	void     _stopNetWorking();
	void     _startNetworking();


	static  CForkUtil&  Instance();
private:
	CForkUtil();
	~CForkUtil();

	static void CreateInstance();
	static CForkUtil* _instance;
	static std::once_flag init_flag;

private:

};




#endif


