
#include "../util.h"
#include "TimeService.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../timedata.h"
#include <ctype.h>
#include <fstream> 
#include <boost/filesystem.hpp>

CTimeService timeService;
boost::mutex mutexTime;
boost::condition_variable cvTime;

CNtpClient::CNtpClient()
	:NTP_PORT(123)
{
}

CNtpClient::CNtpClient(const std::string& strServerIp)
	        :NTP_PORT(123),_serverIp(strServerIp)
{  
}

CNtpClient::~CNtpClient()
{
}

//网络字节转换
void CNtpClient::reverseByteOrder(uint64_t &in)
{
	uint64_t rs = 0;
	int len = sizeof(uint64_t);
	for (int i = 0; i < len; i++)
	{
		std::memset(reinterpret_cast<uint8_t*>(&rs) + len - 1 - i
			, static_cast<uint8_t> ((in & 0xFFLL << (i * 8)) >> i * 8)
			, 1);
	}
	in = rs;
}
std::vector<std::string> CNtpClient::domain2ip(const char *pDomain, int nPort)
{
	std::vector<std::string> vecStrIp;
	if (NULL == pDomain)
	{
		LogPrintf("[CNtpClient::domain2ip]将域名转化为IP地址错误，输入值为空\n");
		return vecStrIp;
	}
	
	try
	{
		boost::asio::io_service ios;
		//创建resolver对象  
		boost::asio::ip::tcp::resolver slv(ios);
		//创建query对象  
		boost::asio::ip::tcp::resolver::query qry(pDomain, boost::lexical_cast<std::string>(nPort));
		//将int型端口转换为字符串  
        //使用resolve迭代端点  
		boost::asio::ip::tcp::resolver::iterator it = slv.resolve(qry);
		boost::asio::ip::tcp::resolver::iterator iterEnd;
		
		for (; it != iterEnd; it++)
		{
			vecStrIp.push_back((*it).endpoint().address().to_string());
		}
		return vecStrIp;
	}
	catch (boost::system::system_error e)
	{
		LogPrintf("[CNtpClient::domain2ip]将域名转化为IP地址错误，错误信息是：%s\n", e.code());
		return vecStrIp;
	}
	catch (std::exception& e)
	{
		LogPrintf("[CNtpClient::domain2ip]将域名转化为IP地址错误，错误信息是：%s\n", e.what());
		return vecStrIp;
	}
}

int64_t CNtpClient::getNetTime(std::string strAddress, int64_t &nOldLocalTime, int64_t &nNewLocalTime)
{
	std::vector<std::string> vecIP;

	//判断是否是域名
	if (isalpha((strAddress.c_str())[0]))
	{
		std::cout <<"[CNtpClient::getNetTime]domain name: "<< strAddress << std::endl;
		LogPrintf("[CNtpClient::getNetTime]domain name:%s \n", strAddress);
		vecIP = domain2ip(strAddress.c_str(), 0);
	}
	else
	{
		std::cout << "[CNtpClient::getNetTime]IPAddress: " << strAddress << std::endl;
		LogPrintf("[CNtpClient::getNetTime]IPAddress: %s\n", strAddress);
		vecIP.push_back(strAddress);
	}

	//地址错误
	int64_t nRet = 0;
	if (0 == vecIP.size())
	{
		LogPrintf("[CNtpClient::getNetTime]地址错误\n");
		return nRet;
	}

	//遍历地址
	for (int nIndex = 0; nIndex < vecIP.size(); nIndex++)
	{
		nOldLocalTime = GetTimeMicros();
		nRet = getOneNetTime(vecIP[nIndex]);
		nNewLocalTime = GetTimeMicros();
		if (0 < nRet)
		{
			std::cout << "[CNtpClient::getNetTime]IPAddress:" << vecIP[nIndex] <<"***NetTime: "<< nRet << std::endl;
			LogPrintf("[CNtpClient::getNetTime] NetTime: %d\n", nRet);
			break;
		}

		//查找前1个IP还没有取到时间，则退出，防止BUG产生
		//if (1 < nIndex)
		//{
		//	break;
		//}
	}
	if (0 == nRet)
	{
		nOldLocalTime = 0;
		nNewLocalTime = 0;
		std::cout << "[CNtpClient::getNetTime]IPAddress: 0 *** false!"  << std::endl;
		LogPrintf("[CNtpClient::getNetTime] NetTime: 0\n");
	}
	
	return nRet;
}

//获得网络时间，得到是微秒，是毫秒的1000倍
int64_t CNtpClient::getOneNetTime(std::string strIP)
{
	int64_t nRetTime = 0;
	try
	{
		boost::asio::io_service io_Ntp;
		boost::asio::ip::udp::socket _socket(io_Ntp);
		_socket.open(boost::asio::ip::udp::v4());
		boost::asio::ip::udp::endpoint ep(boost::asio::ip::address_v4::from_string(strIP), NTP_PORT);

		CNtpPacket request;
		std::stringstream ss;
		std::string strSendBuf;
		ss << request;
		ss >> strSendBuf;

		std::size_t nSendlenth = _socket.send_to(boost::asio::buffer(strSendBuf), ep);
		
		if (nSendlenth != strSendBuf.size())
		{
			if (_socket.is_open())
			{
				_socket.close();
			}
			return 0;
		}
		
		std::array<uint8_t, 128> arrRecv;

		//接收数据的时候等待5秒
		CAsyncRecv asyncRecv(&io_Ntp, &_socket);
		boost::system::error_code ec;
		std::size_t len = asyncRecv.receive(boost::asio::buffer(arrRecv), boost::posix_time::seconds(5), ec);
		if (ec)
		{
			std::cout << "[CNtpClient::getOneNetTime] can not getTime" << std::endl;
			LogPrintf("[CNtpClient::getOneNetTime] ErrorMessage：%s\n", ec.message());
			if (_socket.is_open())
			{
				_socket.close();
			}
			return 0;
		}

		uint8_t* pBytes = arrRecv.data();

		/****get the last 8 bytes(Transmit Timestamp) from received packet.
		std::memcpy(&last, pBytes + len - 8, sizeof(last));
		****create a NtpPacket*/
		CNtpPacket resonpse;
		std::stringstream strRss;
		strRss.write(reinterpret_cast<const char*>(pBytes), len);
		strRss >> resonpse;

		uint64_t uLast = resonpse._rep._trt;

		//网络字节转换
		reverseByteOrder(uLast);

		int64_t nSeconds = (uLast & 0xFFFFFFFF00000000) >> 32;
		int64_t nMico = uLast & 0x00000000FFFFFFFF;
		//前32位表示从1900年到目前为止所有秒数的整数部分，后32位是微秒数的4294.967296(=2^32/10^6)倍
		//nMico = nMico/4294.967296;
		double dMico = nMico;
		dMico = dMico / 4294.967296;
	
		int64_t nTime1900 = (boost::posix_time::ptime(boost::gregorian::date(1900, 1, 1)) -
			boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1))).total_microseconds();

		//nRetTime = nSeconds * 1000000 + nNewMico;
		//nRetTime = nRetTime + nTime1900;
		nRetTime = nSeconds * 1000000 + dMico;
		nRetTime = nRetTime + nTime1900;
		
		if (_socket.is_open())
		{
			_socket.close();
		}
		
		LogPrintf("[CNtpClient::getOneNetTime] NetTime：%d\n", nRetTime);
		return nRetTime;
    }
	catch (boost::system::system_error e) 
	{
		//_socket.close();
		LogPrintf("[CNtpClient::getOneNetTime] ErrorMessage：%s\n", e.code());
		return 0;
	}
	catch (std::exception& e)
	{
		//_socket.close();
		LogPrintf("[CNtpClient::getOneNetTime] ErrorMessage：%s\n", e.what());
		return 0;
	}
	catch (...)
	{
		LogPrintf("[CNtpClient::getOneNetTime] ErrorMessage\n");
		return 0;
	}
}

CTimeService::CTimeService():m_nNetTimeOffset(0), m_bNewStart(true)
{
	m_nLastInitTime = GetTimeMicros();
}

CTimeService::~CTimeService()
{
	//stopTimeService();
	//stop();
}


void CTimeService::GetNetTimeError()
{
	//5分钟（6000000l微秒）后重试
	m_nLastInitTime = GetCurrentTimeMicros() - TIME_REFRESH_TIME;
	m_nLastInitTime = m_nLastInitTime + TIME_5MINUTE;
}

void CTimeService::GetNetTimeSuccess() 
{
	m_nLastInitTime = GetCurrentTimeMicros();
}

int64_t CTimeService::GetNetTime(int64_t &nOldLocalTime, int64_t &nNewLocalTime)
{
	//从配置文件读取时间服务器地址，去读取每一个时间服务器的地址，直到有一个取到值为止
	std::multimap<std::string, std::string> mapConfigInfo;
	ReadTimeServerFile(mapConfigInfo);
	
	int64_t nNetTime = 0;
	std::multimap<std::string, std::string>::iterator iter_configMap = mapConfigInfo.begin();
	for (; iter_configMap != mapConfigInfo.end(); ++iter_configMap)
	{
		if ("timeserver" == iter_configMap->first)
		{
			//nOldLocalTime = GetTimeMicros();
			CNtpClient ntp;
			nNetTime = ntp.getNetTime(iter_configMap->second, nOldLocalTime, nNewLocalTime);
			//nNewLocalTime = GetTimeMicros();
			
			if (0 < nNetTime)
			{
				break;
			}
			else
			{
				nOldLocalTime = 0;
				nNewLocalTime = 0;
			}
		}
	}

	std::cout << "[CTimeService::GetNetTime] NetTime：" << nNetTime << std::endl;
	LogPrintf("[CTimeService::GetNetTime] NetTime：%d\n", nNetTime);
	return nNetTime;
}

void CTimeService::CalculateNetOffsetTime(const int64_t &nNetTime,const int64_t &nOldLocalTime,const int64_t &nNewLocalTime)
{
	//int64_t nNewLocalTime = GetTimeMicros();
	//m_nNetTimeOffset = (nNetTime + (nNewLocalTime - nNowLocalTime) / 2) - GetTimeMicros();
	{
		writeLock  wtlock(rwmutex);
		//m_nNetTimeOffset = (nNetTime + (nNewLocalTime - nOldLocalTime) / 2) - GetTimeMicros();
		m_nNetTimeOffset = nNetTime - (nNewLocalTime + nOldLocalTime) / 2;
		std::cout << "[CTimeService::CalculateNetOffsetTime] NetTimeOffset: "
			<< m_nNetTimeOffset<<"++++" << nNetTime << "++++" << nNewLocalTime << "++++" << nOldLocalTime << std::endl;
		std::cout << "[CTimeService::CalculateNetOffsetTime] NetTimeOffset: " 
			      << m_nNetTimeOffset<< std::endl;

		LogPrintf("[CTimeService::CalculateNetOffsetTime] NetTimeOffset：%d, nNetTime:%d, nNewLocalTime:%d, nOldLocalTime:%d,GetTimeMicros():%d\n", 
			m_nNetTimeOffset, nNetTime, nNewLocalTime, nOldLocalTime, GetTimeMicros());
	}

	GetNetTimeSuccess();
	
	/*if (m_bNewStart)
	{
		boost::lock_guard<boost::mutex> lockTime(mutexTime);
		cvTime.notify_one();
	}*/
	//新启动节点标志位为假
	m_bNewStart = false;
	return;
}

void CTimeService::GetNetOffsetTime()
{
	//得到网络时间
	int64_t nOldLocalTime = 0;
	int64_t nNewLocalTime = 0;
	int64_t nNetTime = GetNetTime(nOldLocalTime, nNewLocalTime);
	
	if(0 < nNetTime)
	{
		//计算网络偏移时间
		CalculateNetOffsetTime(nNetTime, nOldLocalTime, nNewLocalTime);
		return;
	}
	else 
	{
		LogPrintf("[CTimeService::GetNetOffsetTime] GetNetTime faile,Get NetTime Every 5 Minutes.");
		//新启动节点，连不到配置文件中所有服务器，
		//则每5分钟去重新连一下服务器。在获得服务器时间之前，新节点的开会，打包等等逻辑不与启动
		if (m_bNewStart)
		{
			int64_t nLastGetTime = GetTimeMicros();
			while (true)
			{
				boost::this_thread::interruption_point();

				if (TIME_5MINUTE < GetTimeMicros() - nLastGetTime)
				{
					nOldLocalTime = 0;
					nNewLocalTime = 0;
					int64_t nRetNetTime = GetNetTime(nOldLocalTime, nNewLocalTime);
					//nNewLocalTime = GetTimeMicros();
					if (0 < nRetNetTime)
					{
						CalculateNetOffsetTime(nRetNetTime,nOldLocalTime, nNewLocalTime);
						break;
					}
					else
					{
						nLastGetTime = GetTimeMicros();
					}
				}
				
				MilliSleep(5001);
			}
			return;
		}
		else//非新启动节点，m_nNetTimeOffset保持不变，每5分钟去重连下服务器
		{
			GetNetTimeError();
			return;
		}
	}	
}

/**
* 启动一个服务，监控本地时间的变化
* 如果本地时间有变化，则设置 TimeService.netTimeOffset;
*/
void CTimeService::monitorTimeChange()
{
	try
   {
		//硬编码配置文件
		WriteTimeServerFile();

		//int64_t nLoopTime = GetTimeMicros();

		GetNetOffsetTime();
		LogPrintf("[CTimeService::monitorTimeChange]Init_GetNetOffsetTime,NetTimeOffset:%d----CurrentTimeMicros:%d\n",
				   m_nNetTimeOffset, GetCurrentTimeMicros());
		
		while (true)
		{
			boost::this_thread::interruption_point();

			//动态调整网络时间
			int64_t nNewTime = GetTimeMicros();
		
			//本地时间被调整超过10秒
			/*if ((nNewTime - nLoopTime) > TIME_OFFSET_BOUNDARY)
			{
				GetNetOffsetTime();
				LogPrintf("[CTimeService::monitorTimeChange]LocalTime AdjustNetTimeOffset:%d----CurrentTimeMicros:%d\n",
						  m_nNetTimeOffset, GetCurrentTimeMicros());
			}*/
			//else 
			//GetNetOffsetTime();
			if ((GetCurrentTimeMicros() - m_nLastInitTime) > TIME_REFRESH_TIME)
			{
				//每隔一段时间更新网络时间100分钟	
				GetNetOffsetTime();
				LogPrintf("[CTimeService::monitorTimeChange]Every 100 Minutes,NetTimeOffset:%d----CurrentTimeMicros:%d\n",
						  m_nNetTimeOffset,GetCurrentTimeMicros());
			}
		
			//nLoopTime = nNewTime;
			//休眠5秒
			MilliSleep(1001);
		}
	}
	catch (boost::thread_interrupted & errcod)
	{
		LogPrintf("[CTimeService::monitorTimeChange] end by boost::thread thrdMiningService Interrupt exception was thrown\n");
		return;
	}
	catch (...)
	{
		LogPrintf("[CTimeService::monitorTimeChange] end by throw\n");
		return;
	}
}


//当前微秒时间
int64_t  CTimeService::GetCurrentTimeMicros()
{
	try
	{
		readLock  rdlock(rwmutex);
		return GetTimeMicros() + m_nNetTimeOffset;
	}
	catch (...)
	{
		LogPrintf("[CTimeService::GetCurrentTimeMicros] end by throw\n");
		return -1;
	}
}

//当前毫秒时间
int64_t  CTimeService::GetCurrentTimeMillis()
{
	return GetCurrentTimeMicros()/1000;
}

//当前分钟时间
int64_t  CTimeService::GetCurrentTimeSeconds()
{
	return GetCurrentTimeMillis()/1000;
}


void CTimeService::start()
{
	//thrdTimeService = boost::thread(boost::bind(&CTimeService::monitorTimeChange, this));
	thrdTimeService = boost::thread(&TraceThread<std::function<void()> >, "time", std::function<void()>(std::bind(&CTimeService::monitorTimeChange, this)));
}

void CTimeService::stop()
{
	thrdTimeService.interrupt();
	thrdTimeService.join();
}

int64_t CTimeService::abs64(int64_t n)
{
	return (n >= 0 ? n : -n);
}

bool CTimeService::ReadTimeServerFileToString(std::string &strAddress)
{
	std::string strFilePath;
	boost::filesystem::path dirPath = GetDataDir();
	strFilePath += dirPath.string();
	strFilePath += "/timeserver.conf";

	std::ifstream configFile;
	if (!configFile.is_open())
	{
		configFile.open(strFilePath.c_str());
	}

	if (configFile.is_open())
	{
		configFile.seekg(0, std::ios::beg);

		while (!configFile.eof() && configFile.peek() != EOF)
		{
			std::string str_line;
			std::getline(configFile, str_line);

			strAddress += str_line;	
			strAddress += "\n";
		}

		//关闭文件
		configFile.close();

		return true;
	}
	else
	{
		return false;
	}
}


bool CTimeService::ReadTimeServerFile(std::multimap<std::string, std::string>& mapConfigInfo)
{
	std::string strFilePath;
	boost::filesystem::path dirPath = GetDataDir();
	strFilePath += dirPath.string();
    strFilePath += "/timeserver.conf";

	std::ifstream configFile;
	if (!configFile.is_open())
	{
		configFile.open(strFilePath.c_str());
	}

	if (configFile.is_open())
	{
		configFile.seekg(0, std::ios::beg);
		
		while (!configFile.eof() && configFile.peek() != EOF)
		{
			std::string str_line;
			std::getline(configFile, str_line);
			
			//过滤掉注释信息，即如果首个字符为#就过滤掉这一行
			if (str_line.compare(0, 1, "#") == 0)
			{
				continue;
			}
			std::size_t pos = str_line.find('=');
			std::string str_key = str_line.substr(0, pos);
			std::string str_value = str_line.substr(pos + 1);
			mapConfigInfo.insert(std::pair<std::string, std::string>(str_key, str_value));
		}

		//关闭文件
		configFile.close();

		return true;
	}
	else
	{
		return false;
	}
}

bool CTimeService::WriteTimeServerFile()
{
	std::string strFilePath;
	boost::filesystem::path dirPath = GetDataDir();
	strFilePath += dirPath.string();
	strFilePath += "/timeserver.conf";

	std::ofstream configFile;

	std::string strAddress = "#阿里的NTP时间服务器\n" \
		"timeserver=ntp1.aliyun.com\n"    \
		"timeserver=cn.pool.ntp.org\n"    \
		"timeserver=ntp2.aliyun.com\n"    \
		"timeserver=ntp3.aliyun.com\n"    \
		"timeserver=ntp4.aliyun.com\n"    \
	    "timeserver=ntp5.aliyun.com\n"    \
		"timeserver=ntp6.aliyun.com\n"    \
		"timeserver=ntp7.aliyun.com\n"    \
		"timeserver=time1.aliyun.com\n"   \
		"timeserver=time2.aliyun.com\n"   \
		"timeserver=time3.aliyun.com\n"   \
		"timeserver=time4.aliyun.com\n"   \
		"timeserver=time5.aliyun.com\n"   \
		"timeserver=time6.aliyun.com\n"   \
		"timeserver=time7.aliyun.com\n"   \
		"#上海电信\n"                     \
		"timeserver=ntp.api.bz\n"         \
		"timeserver=ntp.gwadar.cn\n";

	if (boost::filesystem::exists(strFilePath))
	{
		std::string strGetAddress;
		ReadTimeServerFileToString(strGetAddress);

		/*std::string strnew("/home/fengwork/newlog");
		std::ofstream newFile;
		newFile.open(strnew, std::ios::out | std::ios::trunc);
		if (newFile.is_open())
		{
			newFile << strGetAddress;
			newFile.flush();
			newFile.close();
			//return true;
		}*/

		std::cout << strGetAddress << std::endl;
		if (strAddress.size() <= strGetAddress.size())
		{
			LogPrintf("[CTimeService::WriteTimeServerFile] timeserver.conf is existed，\
				       并且信息量大于默认配置信息,不作修改，直接退出");
			return true;
		}
	}

	configFile.open(strFilePath, std::ios::out | std::ios::trunc);
	if (configFile.is_open())
	{
		configFile << strAddress;
		configFile.flush();
		configFile.close();

		return true;
	}

	return false;
}

bool CTimeService::ChangeFilePath(std::string &strPath)
{
	if (strPath.empty())
	{
		LogPrintf("[CDpocInfo::ChangeFilePath] InPut is NULL");
		return false;
	}

	/*strPath += "/timeserver.conf";
	if (strFilePath != strPath)
	{
		writeLock  wtlock(rwmutex);
		boost::filesystem::path oldPath(strFilePath);
		boost::filesystem::path newPath(strPath);
		if (boost::filesystem::exists(oldPath))
		{
			if (infoFile.is_open())
			{
				infoFile.close();
			}
			boost::filesystem::copy_file(newPath, oldPath);
			boost::filesystem::remove(oldPath);
		}

		strFilePath = strPath;
		return true;
	}*/
	LogPrintf("[CTimeService::ChangeFilePath] newPath==oldPath");
	return true;
}

bool CTimeService::IsHasNetTimeOffset()
{
	LogPrintf("[CTimeService::IsHasNetTimeOffset] begin\n");
	readLock  rdlock(rwmutex);
	if (0 == m_nNetTimeOffset)
	{
		LogPrintf("[CTimeService::IsHasNetTimeOffset] end by false\n");
		return false;
	}

	LogPrintf("[CTimeService::IsHasNetTimeOffset] end by true\n");
	return true;
}