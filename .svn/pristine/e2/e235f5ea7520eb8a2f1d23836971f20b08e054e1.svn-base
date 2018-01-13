#include "md5thread.h"
#include <openssl/md5.h>
#include "ipcregister.h"
#include "ipcinspectiontag.h"
#include "ecoincreatedialog.h"
#include <string>
using namespace std;
using namespace boost;
#include <QDateTime>
#include "log/log.h"
#include <QFile>
#include <QFileInfo>
#include <sys/stat.h>
#ifdef Q_OS_MAC
    #define fseeko64  fseeko
#endif
void logtime(string info)
{
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
    LOG_WRITE(LOG_INFO,time.toStdString().c_str(),info.c_str());
}

long long get_file_size(const char *path)
{
    long long filesize = 0;
    struct stat statbuff;
    if(stat(path, &statbuff) < 0){
        return filesize;
    }else{
        filesize = statbuff.st_size;
    }
    return filesize;
}

void  fileToMd5Thread(string filepath,string classname,void* parentthread)
{
#ifdef Q_OS_WIN
    std::wstring wstrfilepath = QString::fromStdString(filepath).toStdWString();
    FILE *fd=_wfopen(wstrfilepath.c_str(),L"rb");
#else
    std::string strfilepath = q2s(QString::fromStdString(filepath));
    FILE *fd=fopen(strfilepath.c_str(),"rb");
#endif
    if(fd == NULL)
    {
        LOG_WRITE(LOG_INFO,"open failed");
            cout << "open failed" << endl;
            return ;
    }
    MD5_CTX c;
    long long  nFileLen = 0;
    unsigned char md5[17]={0};
    nFileLen = get_file_size(filepath.c_str());
    long long  len;
    unsigned char *pData = (unsigned char*)malloc(101*1024*1024);
    if(!pData)
    {
        LOG_WRITE(LOG_INFO,"malloc failed");
            cout << "malloc failed" << endl;
            return ;
    }
    MD5_Init(&c);
    long long havereadsize = 0;
    LOG_WRITE(LOG_INFO,\
              QString::number(len).toStdString().c_str(),\
              QString::number(nFileLen).toStdString().c_str());

    while( 0 != (len = fread(pData, 1, 100*1024*1024, fd) ) )
    {
        boost::this_thread::interruption_point();
            MD5_Update(&c, pData, len);
            if(nFileLen)
            {
                havereadsize += len;
                if((nFileLen - havereadsize - (long long)10000*len) > 0)
                {
                    int back = fseeko64(fd,(long long)(len*10000),SEEK_CUR);
                    havereadsize += len*10000;
                }
                else if((nFileLen - havereadsize)  - (long long)1000*len >0)
                {
                    int back = fseeko64(fd,(long long)(len*1000),SEEK_CUR);
                    havereadsize += len*1000;
                }
                else if( (nFileLen - havereadsize) - (long long)100*len > 0)
                {
                    int back = fseeko64(fd,(long long)(len*100),SEEK_CUR);
                    havereadsize += len*100;
                }
                else if( (nFileLen - havereadsize) - (long long)50*len > 0)
                {      
                    int back = fseeko64(fd,(long long)(len*50),SEEK_CUR);
                    havereadsize += len*50;
                }
                else if( (nFileLen - havereadsize) - (long long)10*len > 0)
                {
                    int back = fseeko64(fd,(long long)(len*10),SEEK_CUR);
                    havereadsize += len*10;
                }
            }
    }
    boost::this_thread::interruption_point();

    LOG_WRITE(LOG_INFO,"md5");
    MD5_Final(md5,&c);
    string strMD5;
    char szTemp[4];
    for(int i = 0; i < 16; i++)
    {
        memset(szTemp, 0, sizeof(szTemp));
        snprintf(szTemp, sizeof(szTemp), "%02x", (int)md5[i]);
        strMD5 += szTemp;
    }
    cout << endl;
    fclose(fd);
    free(pData);
    //cout <<strMD5.c_str()<< endl;
    if(parentthread){
          if(classname == "IpcRegister"){
             IpcRegister*  parent = (IpcRegister*)parentthread;
              parent->fileToMd5(strMD5);
           }
           else if(classname == "IpcInspectionTag"){
               IpcInspectionTag* parent = (IpcInspectionTag*)parentthread;
               parent->fileToMd5(strMD5);
           }
           else if(classname =="eCoinCreate"){
              ecoincreatedialog* parent = (ecoincreatedialog*)parentthread;
              parent->fileToMd5(strMD5);
          }
    }

}
