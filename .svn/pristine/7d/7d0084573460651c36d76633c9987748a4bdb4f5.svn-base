#include "stateinfo.h"
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

//static Log g_log;
static stateinfo state_info;
stateinfo::stateinfo()
{
}

stateinfo::~stateinfo()
{
}

#ifdef WIN32


static void CreateDir(const std::string& path)
{
    CreateDirectoryA(path.c_str(), NULL);
}

#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PATH 512

static void CreateDir(const std::string& path)
{
    mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

#endif

void STATE_SETPATH(std::string path)
{
   std::cout<<"SetDataPath"<<path.c_str()<<std::endl;
    state_info.SetDataPath(path);
}
void stateinfo::Init()
{

    std::string path = GetDataPath();//GetExePath();
    if(path == "")return;
    std::string logPath = path ;//+ "log";
    CreateDir(logPath);
    logPath += "stateinfo";
    CreateDir(logPath);

    char day[64] = { 0 };
    std::string logFile = logPath + DIRECTORY_SEPARATOR;
    logFile += "state";
    logFile += ".txt";

    m_file = QString::fromStdString(logFile);

    LOG_WRITE(LOG_INFO,"m_file",m_file.toStdString().c_str());

}
void STATE_WRITE(const char* str1)
{

    std::string str(str1);

    state_info.write( str.c_str());

}

void stateinfo::write(const char* str)
{
    Init();
    QFile file(m_file);
    if(file.exists()){
    }else{
    }
    if(!file.open(QIODevice::ReadWrite)){
        return;
    }else{
    }

    qint64 pos;
    pos = file.size();
    file.seek(pos);

    QString content =str;
    qint64 length = -1;
    length = file.write(content.toLatin1(),content.length());

    if(length == -1){
    }else{
    }
    file.close();
}
QString STATE_READ()
{
    QString w;
    w = state_info.read();
    return w;
}
QString stateinfo::read()
{

    Init();
    QFile file(m_file);
    if(file.exists()){
    }else{
    }
    if(!file.open(QIODevice::ReadWrite)){
         return "";
    }else{
    }
    int line = 1;
    char buffer[1024];
    while(!file.atEnd()){
        qint64 length = file.readLine(buffer,1024);
        if(length != -1){
            //  qDebug()<<line++<<":"<<buffer;
        }
    }
    QString s = buffer;
    file.close();
    return s;


}
void STATE_CLEAR()
{
    state_info.clear();
}
void stateinfo::clear()
{

    Init();
    QFile file(m_file);
    if(file.exists()){
    }else{
    }
    file.open(QFile::WriteOnly|QFile::Truncate);
    file.close();
}
