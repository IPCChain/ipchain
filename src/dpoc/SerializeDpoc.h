//#pragma once

#ifndef SERIALIZE_DPOC_H
#define SERIALIZE_DPOC_H

#include "../serialize.h"
#include <boost/filesystem.hpp>
#include "../util.h"
#include "../streams.h"
#include "../chainparams.h"
#include "../clientversion.h"

template<typename T>
class CSerializeDpoc
{
public:
	//CSerializeDpoc();
	//~CSerializeDpoc();


	bool ReadFromDisk(T &item, const uint64_t nSeek, const std::string &strFileName)
	{
		boost::filesystem::path pathTmp = GetDataDir() / strFileName;
		FILE *file = fopen(pathTmp.string().c_str(), "rb");
		//Î»ÒÆ
		if (fseek(file, nSeek, SEEK_SET)) {
			LogPrintf("CSerializeDpoc::ReadFromDisk return false by feek is error \n");
			fclose(file);
			return false;
		}

		CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
		if (filein.IsNull())
		{
			LogPrintf("CSerializeDpoc::ReadFromDisk return false by filein.IsNull() \n");
			return false;
		}

		try {
			filein >> item;
		}
		catch (const std::exception& e) {
			LogPrintf("CSerializeDpoc::ReadFromDisk return false by %d \n", e.what());
			filein.fclose();
			return false;
		}

		filein.fclose();
		return true;
	}

	bool WriteToDisk(const T &item, const std::string &strFileName)
	{
		boost::filesystem::path pathTmp = GetDataDir() / strFileName;
		FILE *file = fopen(pathTmp.string().c_str(), "ab+");
		CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
		if (fileout.IsNull())
		{
			LogPrintf("CSerializeDpoc::WriteToDisk return false by fileout.IsNull() \n");
			fclose(file);
			return false;
		}

		try {
			fileout << item;
		}
		catch (const std::exception& e) {
			LogPrintf("CSerializeDpoc::WriteToDisk return false by %d \n", e.what());
			return false;
		}
		//FileCommit(fileout.Get());
		fileout.fclose();
		return true;
	}

	bool WriteToDiskWithFlush(const T &item, const std::string &strFileName)
	{
		boost::filesystem::path pathTmp = GetDataDir() / strFileName;
		FILE *file = fopen(pathTmp.string().c_str(), "ab+");
		CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
		if (fileout.IsNull())
		{
			LogPrintf("CSerializeDpoc::WriteToDisk return false by fileout.IsNull() \n");
			fclose(file);
			return false;
		}

		try {
			fileout << item;
		}
		catch (const std::exception& e) {
			LogPrintf("CSerializeDpoc::WriteToDisk return false by %d \n", e.what());
			return false;
		}
		FileCommit(fileout.Get());
		fileout.fclose();
		return true;
	}

	bool WriteToDiskCover(const T &item, const std::string &strFileName)
	{
		boost::filesystem::path pathTmp = GetDataDir() / strFileName;
		FILE *file = fopen(pathTmp.string().c_str(), "wb");
		CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
		if (fileout.IsNull())
		{
			LogPrintf("CSerializeDpoc::WriteToDiskCover return false by fileout.IsNull() \n");
			fclose(file);
			return false;
		}

		try {
			fileout << item;
		}
		catch (const std::exception& e) {
			LogPrintf("CSerializeDpoc::WriteToDiskCover return false by %d \n", e.what());
			return false;
		}
		//FileCommit(fileout.Get());
		fileout.fclose();
		return true;
	}

	bool WriteToDiskSeek(const T &item, const uint64_t nSeek, const std::string &strFileName)
	{
		//boost::filesystem::path pathTmp = GetDataDir() / strFileName;
		/*FILE *file = fopen(pathTmp.string().c_str(), "wb+");
		CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
		if (fileout.IsNull())
		{
			LogPrintf("CSerializeDpoc::WriteToDiskCover return false by fileout.IsNull() \n");
			fclose(file);
			return false;
		}*/
		boost::filesystem::path pathTmp = GetDataDir() / strFileName;
		FILE *file = fopen(pathTmp.string().c_str(), "ab+");
		//Î»ÒÆ
		if (fseek(file, nSeek, SEEK_SET)) {
			LogPrintf("CSerializeDpoc::WriteToDiskSeek return false by feek is error \n");
			fclose(file);
			return false;
		}

		CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
		if (fileout.IsNull())
		{
			fclose(file);
			LogPrintf("CSerializeDpoc::WriteToDiskSeek return false by filein.IsNull() \n");
			return false;
		}


		try {
			fileout << item;
		}
		catch (const std::exception& e) {
			LogPrintf("CSerializeDpoc::WriteToDiskSeek return false by %d \n", e.what());
			return false;
		}
		//FileCommit(fileout.Get());
		fileout.fclose();
		return true;
	}

	

};

class CSnapshotIndex
{
public:
	CSnapshotIndex()
	{

	}
	~CSnapshotIndex()
	{

	}

	uint32_t nHeight;
	uint64_t nOffset;

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(nHeight);
		READWRITE(nOffset);
	}
};

class CSerializeIndexRev
{
public:
	uint64_t nOffset;

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(nOffset);
	}
};








#endif //SERIALIZE_DPOC_H
