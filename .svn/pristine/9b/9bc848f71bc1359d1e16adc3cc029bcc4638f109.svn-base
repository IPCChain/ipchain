#ifndef STATEINFO
#define STATEINFO
#include <stdio.h>
#include <stdarg.h>
#include <string>
using namespace std;
#include <iostream>
#include <QString>
#include <QTextCodec>
#include <QFile>


class stateinfo {
public:
    stateinfo();
    ~stateinfo();

    void Init();
    void write(const char* str);
    void clear();
    QString read();
    void SetDataPath(std::string path){m_datapath=path+"/";}
private:
    void CreateLogFile();
    std::string GetDataPath(){return m_datapath;}
    std::string m_datapath;
   // QFile* m_file;
    QString m_file;
    int m_day;
};



void STATE_WRITE(const char* str1);
void STATE_SETPATH(std::string);
QString STATE_READ();
void STATE_CLEAR();




#endif // STATEINFO

