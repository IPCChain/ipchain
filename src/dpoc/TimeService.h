
#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <iostream>
#include <boost/asio.hpp>
#include "../utiltime.h"
#include <boost/thread/thread.hpp>
#include "../util.h"

//#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#define BLOCK_GEN_TIME (15000)

extern boost::mutex mutexTime;
extern boost::condition_variable cvTime;

class CTimeService;
extern CTimeService timeService;
//NTP包
class CNtpPacket {
public:
	CNtpPacket() {
		_rep._flags = 0xdb;
		//    11.. ....    Leap Indicator: unknown
		//    ..01 1...    NTP Version 3
		//    .... .011    Mode: client
		_rep._pcs = 0x01;//unspecified精度1-16,1最高
		_rep._ppt = 0x01;
		_rep._pcp = 0x01;
		_rep._rdy = 0x01000000;//big-endian
		_rep._rdn = 0x01000000;
		_rep._rid = 0x00000000;
		_rep._ret = 0x0;
		_rep._ort = 0x0;
		_rep._rct = 0x0;
		_rep._trt = 0x0;
	}

	friend std::ostream& operator<<(std::ostream& os, const CNtpPacket& ntpacket) {
		return os.write(reinterpret_cast<const char *>(&ntpacket._rep), sizeof(ntpacket._rep));
	}

	friend std::istream& operator >> (std::istream& is, CNtpPacket& ntpacket) {
		return is.read(reinterpret_cast<char*>(&ntpacket._rep), sizeof(ntpacket._rep));
	}

public:
#pragma pack(1)
	struct NtpHeader {
		uint8_t _flags;//Flags
		uint8_t _pcs;//Peer Clock Stratum
		uint8_t _ppt;//Peer Polling Interval
		uint8_t _pcp;//Peer Clock Precision
		uint32_t _rdy;//Root Delay
		uint32_t _rdn;//Root Dispersion
		uint32_t _rid;//Reference ID
		uint64_t _ret;//Reference Timestamp
		uint64_t _ort;//Origin Timestamp
		uint64_t _rct;//Receive Timestamp
		uint64_t _trt;//Transmit Timestamp
	};
#pragma pack()
	NtpHeader _rep;
};


class CNtpClient 
{
public:
	CNtpClient();
	CNtpClient(const std::string& strServerIp);
	~CNtpClient();
	void reverseByteOrder(uint64_t &in);
	
	//获得网络时间，得到是微秒，是毫秒的1000倍
	int64_t getNetTime(std::string strAddress, int64_t &nOldLocalTime, int64_t &nNewLocalTime);
	int64_t getOneNetTime(std::string strIP);
	//将域名转化为IP地址
	std::vector<std::string> domain2ip(const char *domain, int port);
	
private:
	const uint16_t NTP_PORT;
	std::string _serverIp;
	boost::system::error_code _errc;
};

class CAsyncRecv
{
public:
	
	CAsyncRecv(boost::asio::io_service *io_service_input, boost::asio::ip::udp::socket *socket_input)
		:io_service_(io_service_input), socket_(socket_input),deadline_(*io_service_input)
	{
		//启动boost.asio定时器dealine_timer，设置停止时间点为正无限
		deadline_.expires_at(boost::posix_time::pos_infin);
		check_deadline();
	}
	
	/*~client()
	{
		io_service_ = NULL;
		socket_ = NULL;
	}*/

	std::size_t receive(const boost::asio::mutable_buffer& buffer,
		                boost::posix_time::time_duration timeout, boost::system::error_code& ec)
	{
		deadline_.expires_from_now(timeout);
		ec = boost::asio::error::would_block;
		std::size_t length = 0;
		socket_->async_receive(boost::asio::buffer(buffer),
			                   boost::bind(&CAsyncRecv::handle_receive, _1, _2, &ec, &length));
		do
		{
			io_service_->run_one();
		}
		while (ec == boost::asio::error::would_block);

		return length;
	}

private:
	void check_deadline()
	{
		if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
		{
			try
			{
				socket_->cancel();
			}
			catch (boost::system::system_error e)
			{
				LogPrintf("check_deadline退出，这是正常现象：%s\n", e.code());
			}
			
			deadline_.expires_at(boost::posix_time::pos_infin);
		}
		deadline_.async_wait(boost::bind(&CAsyncRecv::check_deadline, this));
	}

	static void handle_receive(const boost::system::error_code& ec, std::size_t length,
		                       boost::system::error_code* out_ec, std::size_t* out_length)
	{
		*out_ec = ec;
		*out_length = length;
	}

private:
	boost::asio::io_service *io_service_;
	boost::asio::ip::udp::socket *socket_;
	boost::asio::deadline_timer deadline_;
};

//时间偏移差距触发点，超过该值会导致本地时间重设，10秒，单位微秒
const int64_t TIME_OFFSET_BOUNDARY = 10000001;

//网络时间刷新间隔 100分钟 ，单位微秒
const int64_t TIME_REFRESH_TIME = 6000000001;
//1分钟-byTest
//const int64_t TIME_REFRESH_TIME = 60000001;

//8小时的微秒数
const int64_t TIME_8HOURS = 28800000000;
//5分钟的微秒数
const int64_t TIME_5MINUTE = 300000001;
//30秒的微秒数-byTest
//const int64_t TIME_5MINUTE = 30000001;

class CTimeService
{
public:
	CTimeService();
	~CTimeService();

	//当前毫秒时间,根据网络时间调整后
	int64_t GetCurrentTimeMillis();
	//当前微秒时间,根据网络时间调整后
	int64_t  GetCurrentTimeMicros();
	//当前秒时间,根据网络时间调整后
	int64_t  GetCurrentTimeSeconds();

	void start();
	void stop();

	//改变文件路径
	bool ChangeFilePath(std::string &strPath);
	bool IsHasNetTimeOffset();
	
private:
	void GetNetTimeError();
	void GetNetTimeSuccess();
	void GetNetOffsetTime();
	void monitorTimeChange();
	int64_t abs64(int64_t n);

	//读时间服务器地址的配置文件
	bool ReadTimeServerFile(std::multimap<std::string, std::string>& mapConfigInfo);
	//硬编码时间服务器地址配置文件
	bool WriteTimeServerFile();
	bool ReadTimeServerFileToString(std::string &strAddress);
	//得到网络时间
	int64_t GetNetTime(int64_t &nOldLocalTime, int64_t &nNewLocalTime);
	//根据网络时间计算偏移时间
	//void CalculateNetOffsetTime(int64_t nNetTime, int64_t nOldLocalTime);
	void CalculateNetOffsetTime(const int64_t &nNetTime, const int64_t &nOldLocalTime, const int64_t &nNewLocalTime);
	
private:
	//网络偏移时间
	int m_nNetTimeOffset;
	//最后初始化时间
	int64_t m_nLastInitTime;

	//是否新启动节点
	bool m_bNewStart;

	//时间服务，存储网络时间与本地时间，所有与网络交互的时间，经过换算，保证网络时间同步
	boost::thread thrdTimeService;
	//boost::mutex time_mutex;
	boost::shared_mutex  rwmutex;
	typedef boost::shared_lock<boost::shared_mutex> readLock;
	typedef boost::unique_lock<boost::shared_mutex> writeLock;
};



#endif //TIME_SERVICE_H