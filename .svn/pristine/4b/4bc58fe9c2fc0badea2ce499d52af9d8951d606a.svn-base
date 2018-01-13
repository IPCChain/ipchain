
#include "../util.h"
#include "TimeService.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../timedata.h"
#include <ctype.h>
#include <fstream> 
#include <boost/filesystem.hpp>
#include <thread>
#include<chrono>

bool g_bStdCout = false;
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
		LogPrintf("[CNtpClient::domain2ip]Convert domain name to IP address error，input is null\n");
		return vecStrIp;
	}
	
	try
	{
		boost::asio::io_service ios;
		boost::asio::ip::tcp::resolver slv(ios);
		boost::asio::ip::tcp::resolver::query qry(pDomain, boost::lexical_cast<std::string>(nPort));  
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
		LogPrintf("[CNtpClient::domain2ip]Convert domain name to IP address error，error message：%s\n", e.code());
		return vecStrIp;
	}
	catch (std::exception& e)
	{
		LogPrintf("[CNtpClient::domain2ip]Convert domain name to IP address error，error message：%s\n", e.what());
		return vecStrIp;
	}
}

int64_t CNtpClient::getNetTime(std::string strAddress, int64_t &nOldLocalTime, int64_t &nNewLocalTime)
{
	std::vector<std::string> vecIP;

	//Decide if it's a domain name
	if (isalpha((strAddress.c_str())[0]))
	{
		LogPrintf("[CNtpClient::getNetTime]domain name:%s \n", strAddress);
		vecIP = domain2ip(strAddress.c_str(), 0);
	}
	else
	{
		LogPrintf("[CNtpClient::getNetTime]IPAddress: %s\n", strAddress);
		vecIP.push_back(strAddress);
	}

	int64_t nRet = 0;
	if (0 == vecIP.size())
	{
		LogPrintf("[CNtpClient::getNetTime]address error\n");
		return nRet;
	}

	for (int nIndex = 0; nIndex < vecIP.size(); nIndex++)
	{
		nOldLocalTime = GetTimeMicros();
		nRet = getOneNetTime(vecIP[nIndex]);
		nNewLocalTime = GetTimeMicros();
		if (0 < nRet)
		{
			LogPrintf("[CNtpClient::getNetTime] NetTime: %d\n", nRet);
			break;
		}
	}
	if (0 == nRet)
	{
		nOldLocalTime = 0;
		nNewLocalTime = 0;
		LogPrintf("[CNtpClient::getNetTime] NetTime: 0\n");
	}
	
	return nRet;
}

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

		//Wait 5 seconds to receive data
		CAsyncRecv asyncRecv(&io_Ntp, &_socket);
		boost::system::error_code ec;
		std::size_t len = asyncRecv.receive(boost::asio::buffer(arrRecv), boost::posix_time::seconds(5), ec);
		if (ec)
		{
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

		reverseByteOrder(uLast);

		int64_t nSeconds = (uLast & 0xFFFFFFFF00000000) >> 32;
		int64_t nMico = uLast & 0x00000000FFFFFFFF;

		/*32-bit said so far in 1900 before all of the number of seconds the integer part, 
		32 is the number of milliseconds after 4294.967296 (32/10 = 2 ^ ^ 6) times*/
		double dMico = nMico;
		dMico = dMico / 4294.967296;
	
		int64_t nTime1900 = (boost::posix_time::ptime(boost::gregorian::date(1900, 1, 1)) -
			boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1))).total_microseconds();

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
		LogPrintf("[CNtpClient::getOneNetTime] ErrorMessage：%s\n", e.code());
		return 0;
	}
	catch (std::exception& e)
	{
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
}

void CTimeService::GetNetTimeError()
{
	//5 minutes (6000000l microsecond) retry
	m_nLastInitTime = GetCurrentTimeMicros() - TIME_REFRESH_TIME;
	m_nLastInitTime = m_nLastInitTime + TIME_5MINUTE;
}

void CTimeService::GetNetTimeSuccess() 
{
	m_nLastInitTime = GetCurrentTimeMicros();
}

int64_t CTimeService::GetNetTime(int64_t &nOldLocalTime, int64_t &nNewLocalTime)
{
	//Read the time server address from the configuration file 
	//to read the address of each time server until there is a fetch value
	std::multimap<std::string, std::string> mapConfigInfo;
	ReadTimeServerFile(mapConfigInfo);
	
	int64_t nNetTime = 0;
	std::multimap<std::string, std::string>::iterator iter_configMap = mapConfigInfo.begin();
	for (; iter_configMap != mapConfigInfo.end(); ++iter_configMap)
	{
		if ("timeserver" == iter_configMap->first)
		{
			CNtpClient ntp;
			nNetTime = ntp.getNetTime(iter_configMap->second, nOldLocalTime, nNewLocalTime);
			
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

	LogPrintf("[CTimeService::GetNetTime] NetTime：%d\n", nNetTime);
	return nNetTime;
}

void CTimeService::CalculateNetOffsetTime(const int64_t &nNetTime,const int64_t &nOldLocalTime,const int64_t &nNewLocalTime)
{
	{
		writeLock  wtlock(rwmutex);
		m_nNetTimeOffset = nNetTime - (nNewLocalTime + nOldLocalTime) / 2;

		LogPrintf("[CTimeService::CalculateNetOffsetTime] NetTimeOffset：%d, nNetTime:%d, nNewLocalTime:%d, nOldLocalTime:%d,GetTimeMicros():%d\n", 
			m_nNetTimeOffset, nNetTime, nNewLocalTime, nOldLocalTime, GetTimeMicros());
	}

	GetNetTimeSuccess();
	
	//The new start node flag bit is false
	m_bNewStart = false;
	return;
}

void CTimeService::GetNetOffsetTime()
{
	//get network time
	int64_t nOldLocalTime = 0;
	int64_t nNewLocalTime = 0;
	int64_t nNetTime = GetNetTime(nOldLocalTime, nNewLocalTime);
	
	if(0 < nNetTime)
	{
		//Calculate network offset time
		CalculateNetOffsetTime(nNetTime, nOldLocalTime, nNewLocalTime);
		return;
	}
	else 
	{
		LogPrintf("[CTimeService::GetNetOffsetTime] GetNetTime faile,Get NetTime Every 5 Minutes.");

		//New start node, not even all servers in the configuration file,
		//Reconnect the server every five minutes.
		//Before the server time, the new node's session, packaging, and other logic are not started
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
				
				//MilliSleep(5001);
				std::chrono::milliseconds dura(5001);
				std::this_thread::sleep_for(dura);
			}
			return;
		}
		else//Non-new start node, m_nNetTimeOffset remains unchanged, and reconnect to the server every 5 minutes
		{
			GetNetTimeError();
			return;
		}
	}	
}

void CTimeService::monitorTimeChange()
{
	try
   {
		//Hard-coded configuration file
		WriteTimeServerFile();

		GetNetOffsetTime();
		LogPrintf("[CTimeService::monitorTimeChange]Init_GetNetOffsetTime,NetTimeOffset:%d----CurrentTimeMicros:%d\n",
				   m_nNetTimeOffset, GetCurrentTimeMicros());
		
		int64_t n64LastTime = GetCurrentTimeMicros();
		while (true)
		{
			boost::this_thread::interruption_point();

			//Adjust the network time dynamically
			int64_t n64NewTime = GetCurrentTimeMicros();
			if ((n64NewTime - n64LastTime > TIME_1HALFSECOND) || (n64LastTime - n64NewTime > TIME_1HALFSECOND))
			{
				//Server time jump, deviation >500 ms, according to network time adjustment.
				GetNetOffsetTime();
				n64NewTime = GetCurrentTimeMicros();
				LogPrintf("[CTimeService::monitorTimeChange]Server time jumps ,NetTimeOffset:%d----CurrentTimeMicros:%d\n",
					m_nNetTimeOffset, GetCurrentTimeMicros());
			}
			else if ((GetCurrentTimeMicros() - m_nLastInitTime) > TIME_REFRESH_TIME)
			{
				//Update network time every 100 minutes
				GetNetOffsetTime();
				n64NewTime = GetCurrentTimeMicros();
				LogPrintf("[CTimeService::monitorTimeChange]Every 100 Minutes,NetTimeOffset:%d----CurrentTimeMicros:%d\n",
						  m_nNetTimeOffset,GetCurrentTimeMicros());
			}

			n64LastTime = n64NewTime;

			//MilliSleep(1001);
			std::chrono::milliseconds dura(1000);
			std::this_thread::sleep_for(dura);
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

int64_t  CTimeService::GetCurrentTimeMillis()
{
	return GetCurrentTimeMicros()/1000;
}

int64_t  CTimeService::GetCurrentTimeSeconds()
{
	return GetCurrentTimeMillis()/1000;
}

void CTimeService::start()
{
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
			
			//Filter out the comment message, 
			//which is to filter out the line if the first character is # 
			if (str_line.compare(0, 1, "#") == 0)
			{
				continue;
			}
			std::size_t pos = str_line.find('=');
			std::string str_key = str_line.substr(0, pos);
			std::string str_value = str_line.substr(pos + 1);
			mapConfigInfo.insert(std::pair<std::string, std::string>(str_key, str_value));
		}

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

	std::string strAddress = "#Ali's NTP time server\n" \
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
		"#Shanghai telecom\n"                     \
		"timeserver=ntp.api.bz\n"         \
		"timeserver=ntp.gwadar.cn\n";

	if (boost::filesystem::exists(strFilePath))
	{
		std::string strGetAddress;
		ReadTimeServerFileToString(strGetAddress);

		if (strAddress.size() <= strGetAddress.size())
		{
			LogPrintf("[CTimeService::WriteTimeServerFile] timeserver.conf is existed，\
				       And the information is greater than the default configuration information");
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

