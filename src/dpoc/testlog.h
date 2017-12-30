
//此文件只用作本地测试使用，不加入最终版本，
//最终版本Makefile.am文件亦不加入此文件

#ifndef TEST_LOG_H
#define TEST_LOG_H

#include <iostream>
#include <fstream>

#include <vector>
#include <set>
#include "../serialize.h"
#include <boost/filesystem.hpp>
#include "../util.h"
#include "../streams.h"
#include "../chainparams.h"
#include "../clientversion.h"


using namespace std;


void static debuglog(string str,int64_t i)
{
	std::stringstream ss;
	string newstr;
	ss << i;
	ss >> newstr;
	str = str + newstr;
	
	std::ofstream examplefile;
	examplefile.open("/tmp/testb.log", ios::out | ios::app);
	if (examplefile.is_open())
	{
		//examplefile.write()
		examplefile << str << endl;
		examplefile.close();
	}
}

void static wirtePublickey(string str)
{
	/*std::stringstream ss;
	string newstr;
	ss << i;
	ss >> newstr;
	str = str + newstr;*/

	std::ofstream examplefile;
	examplefile.open("/tmp/publickey.in", ios::out | ios::app);
	if (examplefile.is_open())
	{
		//examplefile.write()
		examplefile << str << endl;
		examplefile.close();
	}
}

void static readPublickey(string &str)
{
	std::ifstream examplefile;
	examplefile.open("/tmp/publickey.in", ios::in);
	if (examplefile.is_open())
	{
		examplefile >> str;
		examplefile.close();
	}
}

class testClass
{
public:

	std::set<uint16_t> curCandidateIndexList;
	std::set<uint16_t> set16;
	std::vector<std::pair<uint16_t, int64_t>> vec1;

	uint256 u256a;

	testClass()
	{
		curCandidateIndexList.clear();
		set16.clear();
		vec1.clear();
	}
	testClass(testClass &tes1)
	{
		this->curCandidateIndexList = tes1.curCandidateIndexList;
		this->set16 = tes1.set16;
		this->vec1 = tes1.vec1;
		this->u256a = tes1.u256a;
	}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action)
	{
		READWRITE(curCandidateIndexList);
		READWRITE(set16);
		READWRITE(vec1);
		READWRITE(u256a);
	}
};
void static WriteTestClassNew(testClass test1)
{
	std::string tmpfn = "tmpTest";
	boost::filesystem::path pathTmp = GetDataDir() / tmpfn;
	FILE *file = fopen(pathTmp.string().c_str(), "ab+");
	CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
	if (fileout.IsNull())
	{
		return ;
	}
	try {
		fileout << test1;
	}
	catch (const std::exception& e) {
		return ;
	}
	FileCommit(fileout.Get());
	fileout.fclose();
}


void static WriteTestClass(testClass test1)
{
	CDataStream ssPeers(SER_DISK, CLIENT_VERSION);
	ssPeers << FLATDATA(Params().MessageStart());
	ssPeers << test1;

	// open temp output file, and associate with CAutoFile
	std::string tmpfn = "tmpTest";
	boost::filesystem::path pathTmp = GetDataDir() / tmpfn;
	FILE *file = fopen(pathTmp.string().c_str(), "ab+");
	CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
	if (fileout.IsNull())
	{
		return;
	}
		
	// Write and commit header, data
	try {
		fileout << ssPeers;
	}
	catch (const std::exception& e) {
		return;
	}
	FileCommit(fileout.Get());
	fileout.fclose();
}
void static readTestClass(testClass &test1,int nSeek)
{
	std::cout << "readTestClass-------111111" << std::endl;
	std::string tmpfn = "tmpTest";
	boost::filesystem::path pathTmp = GetDataDir() / tmpfn;
	// open input file, and associate with CAutoFile
	FILE *file = fopen(pathTmp.string().c_str(), "rb");
	//位移
	if (fseek(file, nSeek, SEEK_SET)) {
		//LogPrintf("Unable to seek to position %u of %s\n", pos.nPos, path.string());
		fclose(file);
		return ;
	}

	CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
	if (filein.IsNull())
	{
		return;
	}

	std::cout << "readTestClass-------22222+++" << sizeof(uint256) << std::endl;
	
	uint64_t dataSize = boost::filesystem::file_size(pathTmp);
	std::cout << "readTestClass-------3333++"<< dataSize << std::endl;
	dataSize = dataSize - nSeek;
	std::vector<unsigned char> vchData;
	vchData.resize(dataSize);
	
	
	try {
		filein.read((char *)&vchData[0], dataSize);
	}
	catch (const std::exception& e) {
		return;
	}
	filein.fclose();

	std::cout << "readTestClass-------7777++"  << std::endl;

	CDataStream ssPeers(vchData, SER_DISK, CLIENT_VERSION);

	unsigned char pchMsgTmp[4];
	try {
		
		ssPeers >> FLATDATA(pchMsgTmp);

		std::cout << "readTestClass-------8888++" << std::endl;

		if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
			return;

		std::cout << "readTestClass-------9999++" << std::endl;
		
		ssPeers >> test1;

		std::cout << "readTestClass-------101010++" << std::endl;
	}
	catch (const std::exception& e) {
		return;
			
	}

}


void static readTestClassNew(testClass &test1, int nSeek)
{
	std::cout << "readTestClass-------111111" << std::endl;
	std::string tmpfn = "tmpTest";
	boost::filesystem::path pathTmp = GetDataDir() / tmpfn;
	// open input file, and associate with CAutoFile
	FILE *file = fopen(pathTmp.string().c_str(), "rb");
	//位移
	if (fseek(file, nSeek, SEEK_SET)) {
		//LogPrintf("Unable to seek to position %u of %s\n", pos.nPos, path.string());
		fclose(file);
		return;
	}

	CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
	if (filein.IsNull())
	{
		return;
	}

	try {
		filein >> test1;
	}
	catch (const std::exception& e) {
		filein.fclose();
		return;
	}
	filein.fclose();

}

#endif
