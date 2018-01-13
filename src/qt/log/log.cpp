#include "log.h"
#include <string.h>
#include <time.h>
#include <QDir>
#include  <unistd.h>
#include "util.h"
#ifdef WIN32
	#include <windows.h>
	const std::string DIRECTORY_SEPARATOR = "\\";
#else
	const std::string DIRECTORY_SEPARATOR = "/";
#endif

static Log g_log;

Log::Log()
{
    m_logLevel[LOG_ERROR] = "error";
	m_logLevel[LOG_WARNING] = "warn";
	m_logLevel[LOG_INFO] = "info";
	m_logLevel[LOG_DEBUG] = "debug";
	m_file = NULL;
    //Init();

}

Log::~Log()
{
    if (m_file != NULL) {
		time_t tn = time(NULL);
		struct tm* now = localtime(&tn);
		
		fprintf(m_file, "%02d:%02d:%02d log end..........\n", now->tm_hour, now->tm_min, now->tm_sec);
		fclose(m_file);
	}

}

#ifdef WIN32
std::string GetExePath()
{
	char buffer[MAX_PATH] = { 0 };

    GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string file = buffer;
	std::string path = ".\\";
	size_t pos = file.rfind('\\');

	if (pos < file.size()) {
		path = file.substr(0, pos + 1);
	}

	return path;
}

static void CreateDir(const std::string& path)
{
    CreateDirectoryA(path.c_str(), NULL);
}

#else
#include <sys/stat.h>
#include <sys/types.h> 
#include <unistd.h>

#define MAX_PATH 512
std::string GetExePath()
{
	char buffer[MAX_PATH] = { 0 };
	ssize_t ret = readlink("/proc/self/exe", buffer, MAX_PATH);
	if (ret == -1) {
		printf("not found [/proc/self/exe] return [./] \n");
		return "./";
	}

	std::string file = buffer;
	std::string path = "./";
	size_t pos = file.rfind('/');

	if (pos < file.size()) {
		path = file.substr(0, pos + 1);
	}

	return path;
}

static void CreateDir(const std::string& path)
{
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

#endif

void Log::Init()
{
	CreateLogFile(false);
}

void Log::CreateLogFile(bool newFile)
{
    if(!fPrintToDebugLog)
         return;
    std::string path = GetDataPath();//GetExePath();
    if(path == "")return;
	std::string logPath = path + "log";
	CreateDir(logPath);

	time_t tn = time(NULL);
	struct tm* now = localtime(&tn);
	m_day = now->tm_mday;

	char month[64] = { 0 };
	sprintf(month, "%d", now->tm_mon + 1);
	
	logPath += DIRECTORY_SEPARATOR;
	logPath += month;
	CreateDir(logPath);

	char day[64] = { 0 };
	sprintf(day, "%d", now->tm_mday);
	std::string logFile = logPath + DIRECTORY_SEPARATOR;
	logFile += day;
	logFile += ".txt";

	if (newFile) {
		m_file = fopen(logFile.c_str(), "w");
	} else {
		m_file = fopen(logFile.c_str(), "a+");
	}

	if (m_file == NULL) {
		printf("create log file [%s] error\n", logFile.c_str());
		return;
	}

	fprintf(m_file, "%02d:%02d:%02d log start..........\n", now->tm_hour, now->tm_min, now->tm_sec);

}
void LOG_WRITE(LOG_LEVEL level,const char* str1,\
              const char* str2, \
              const char* str3,\
              const char* str4,\
              const char* str5,\
              const char* str6,\
              const char* str7)
{
    if(!fPrintToDebugLog)
         return;
    std::string str(str1);
    if(str2)
    {
        str.append(" ");
        str.append(str2);
    }
    if(str3)
    {
        str.append(" ");
        str.append(str3);
    }
    if(str4)
    {
        str.append(" ");
        str.append(str4);
    }
    if(str5)
    {
        str.append(" ");
        str.append(str5);
    }
    if(str6)
    {
        str.append(" ");
        str.append(str6);
    }
    if(str7)
    {
        str.append(" ");
        str.append(str7);
    }

    g_log.Debug(level, str.c_str());
}

void Log::Debug(LOG_LEVEL level,const char* str, ...)
{
    if(!m_file){
        Init();
    }
    if(!m_file)
    {
        return;
    }
    time_t tn = time(NULL);
	struct tm* now = localtime(&tn);

	if (m_day != now->tm_mday) {
		m_day = now->tm_mday;
		fprintf(m_file, "%02d:%02d:%02d log end ..........\n", now->tm_hour, now->tm_min, now->tm_sec);
        fclose(m_file);
		CreateLogFile(true);
	}
	fprintf(m_file, "%02d:%02d:%02d ", now->tm_hour, now->tm_min, now->tm_sec);
	va_list args;
	va_start(args, str);
	vfprintf(m_file, str, args);
	va_end(args);
	fprintf(m_file, "\n");
	fflush(m_file);
}

void Log::Append(char* str, ...)
{
    va_list args;
    va_start(args, str);
	vfprintf(m_file, str, args);
    va_end(args);
	fflush(m_file);
}

void Log::Trace(char* data, int len)
{
	fwrite(data, sizeof(char), len, m_file);
	fflush(m_file);
}

void addtest(int a,int v)
{
    return;
}

void LOG_SETPATH(std::string path)
{
   std::cout<<"SetDataPath"<<path.c_str()<<std::endl;
    g_log.SetDataPath(path);
}
std::string q2s(const QString &s)
{
    return std::string((const char*)s.toLocal8Bit());
}
string ToGBKString(const QString& qstr)
{
    QTextCodec* pCodec = QTextCodec::codecForName("gb2312");
    if(!pCodec) return "";

    QByteArray arr = pCodec->fromUnicode(qstr);
    string cstr = arr.data();
    return cstr;
}
