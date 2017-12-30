#ifndef _LOG_H__
#define _LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <string>
using namespace std;
#include <iostream>
#include <QString>
#include <QTextCodec>

typedef enum {
	LOG_ERROR = 0,
	LOG_WARNING,
	LOG_INFO,
	LOG_DEBUG
} LOG_LEVEL;

class Log {
public:
	Log();
    ~Log();
	
    void Init();
    void Debug(LOG_LEVEL level,const char* str, ...);
    void Append(char* str, ...);
    void Trace(char* data, int len);
    void SetDataPath(std::string path){m_datapath=path+"/";}
private:
    void CreateLogFile(bool newFile);
    std::string GetDataPath(){return m_datapath;}
    std::string m_datapath;
    FILE* m_file;
    std::string m_logLevel[4];
    int m_day;
};
std::string q2s(const QString &s);
std::string GetExePath();
void LOG_SETPATH(std::string);
void LOG_WRITE(LOG_LEVEL level,const char* str1,\
              const char* str2 = NULL, \
              const char* str3 = NULL,\
              const char* str4 = NULL,\
              const char* str5 = NULL,\
              const char* str6 = NULL,\
              const char* str7 = NULL);

// QString(Unicode) -> std::string (GBK)
    string ToGBKString(const QString& qstr);
#endif
