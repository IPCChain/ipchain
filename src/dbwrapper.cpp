// Copyright (c) 2012-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbwrapper.h"

#include "util.h"
#include "random.h"

#include <boost/filesystem.hpp>

#include <leveldb/cache.h>
#include <leveldb/env.h>
#include <leveldb/filter_policy.h>
#include <memenv.h>
#include <stdint.h>
#include "util.h"

//add by xxy

TxDBProcess::TxDBProcess(){
	db = NULL; 
	dbpath = GetDataDir().string() + "/txdb";
	options.create_if_missing = true;
}
TxDBProcess::~TxDBProcess(){}
bool TxDBProcess::Init()
{
	s = leveldb::DB::Open(options, dbpath, &db);
	if (!s.ok())
	{
		LogPrintf("open database txdb false! \n");
		return false;
	}
	return true;
}


bool TxDBProcess::ReadKeyInfo(leveldb::DB *db, char *Key, KeyInfo *keyinfo)
{
	char *recsum = (char *)malloc(17 * sizeof(char));
	char *recmax = (char *)malloc(17 * sizeof(char));
	char *keyinforecord = (char *)malloc(MEMORY_ALLOCATION_MAX*sizeof(char));
	memset(recsum, 0x00, sizeof(recsum));
	memset(recmax, 0x00, sizeof(recmax));
	memset(keyinforecord, 0x00, sizeof(keyinforecord));
	std::string ReadResult;
	s = db->Get(leveldb::ReadOptions(), Key, &ReadResult);
	if (!s.ok())
		return false;
	keyinforecord = (char *)ReadResult.c_str();

	int _pos1 = ReadResult.find('-');
	int _pos2 = ReadResult.find('-', _pos1 + 1);
	int _pos3 = ReadResult.find('-', _pos2 + 1);

	strncpy(keyinfo->StartKey, keyinforecord, _pos1);
	strncpy(keyinfo->EndKey, keyinforecord + _pos1 + 1, _pos2 - _pos1 - 1);
	strncpy(recsum, keyinforecord + _pos2 + 1, _pos3 - _pos2 - 1);
	strcpy(recmax, keyinforecord + _pos3 + 1);

	sscanf(recsum, "%d", &(keyinfo->RecordSum));
	sscanf(recmax, "%d", &(keyinfo->KeyHight));


	return true;


}

bool TxDBProcess::UpdateKeyInfo(leveldb::DB *db, char *Key, KeyInfo *keyinfo)
{
	char *MakeKeyInfoRecord = (char *)malloc(MEMORY_ALLOCATION_MAX*sizeof(char));
	char *recsum = (char *)malloc(17 * sizeof(char));
	char *recmax = (char *)malloc(17 * sizeof(char));
	memset(MakeKeyInfoRecord, 0x00, sizeof(MakeKeyInfoRecord));
	memset(recsum, 0x00, sizeof(recsum));
	memset(recmax, 0x00, sizeof(recmax));
	sprintf(recsum, "%d", keyinfo->RecordSum);
	sprintf(recmax, "%d", keyinfo->KeyHight);
	strcpy(MakeKeyInfoRecord, (const char*)keyinfo->StartKey);
	strcat(MakeKeyInfoRecord, (const char *)"-");
	strcat(MakeKeyInfoRecord, (const char *)keyinfo->EndKey);
	strcat(MakeKeyInfoRecord, (const char *)"-");
	strcat(MakeKeyInfoRecord, (const char*)recsum);
	strcat(MakeKeyInfoRecord, (const char *)"-");
	strcat(MakeKeyInfoRecord, (const char*)recmax);


	s = db->Put(leveldb::WriteOptions(), Key, MakeKeyInfoRecord);
	if (!s.ok())
	{
		return false;
	}

	return true;

}

bool TxDBProcess::InsertTxid(leveldb::DB *db, char *Key, char* txid)
{

	KeyInfo *tmpkeyinfo = (KeyInfo *)malloc(sizeof(KeyInfo));
	memset(tmpkeyinfo, 0x00, sizeof(tmpkeyinfo));
	char *recmax = (char *)malloc(17 * sizeof(char));
	memset(recmax, 0x00, sizeof(recmax));
	if (!ReadKeyInfo(db, Key, tmpkeyinfo))
	{

		strcpy(tmpkeyinfo->StartKey, Key);
		strcat(tmpkeyinfo->StartKey, (const char*)"0000000000000001");
		strcpy(tmpkeyinfo->EndKey, Key);
		strcat(tmpkeyinfo->EndKey, (const char*)"0000000000000001");
		tmpkeyinfo->RecordSum = 1;
		tmpkeyinfo->KeyHight = 1;

	}
	else
	{
		sprintf(recmax, "%d", tmpkeyinfo->KeyHight + 1);
		tmpkeyinfo->RecordSum++;
		tmpkeyinfo->KeyHight++;
		char *tmpstr = (char *)malloc(MEMORY_ALLOCATION_MIN*sizeof(char));
		memset(tmpstr, 0x00, sizeof(tmpstr));
		strcpy(tmpkeyinfo->EndKey, Key);
		strcpy(tmpstr, "0000000000000000");
		strcat(tmpstr, recmax);

		strcat(tmpkeyinfo->EndKey, tmpstr + strlen(recmax));


	}
	if (!UpdateKeyInfo(db, Key, tmpkeyinfo))
	{
		return false;
	}

	s = db->Put(leveldb::WriteOptions(), tmpkeyinfo->EndKey, txid);
	if (!s.ok())
	{

		return false;
	}




	return true;


}

bool TxDBProcess::SelectTxid(leveldb::DB *db, char *Key, std::vector<std::string> &txids, int64_t get_num, int16_t out_of_order)
{
	KeyInfo *tmpkeyinfo = (KeyInfo *)malloc(sizeof(KeyInfo));

	if (!ReadKeyInfo(db, Key, tmpkeyinfo))
		return false;
	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	int64_t i = 0;
	if (out_of_order == 0)
	{

		for (it->Seek(tmpkeyinfo->EndKey); it->Valid() && i < get_num; it->Prev())
		{
			txids.push_back(it->value().ToString());
			i++;
		}
	}
	else
	{
		for (it->Seek(tmpkeyinfo->StartKey); it->Valid() && i < get_num; it->Next())
		{
			txids.push_back(it->value().ToString());
			i++;
		}

	}

	return true;

}

bool TxDBProcess::DeleteTxid(leveldb::DB *db, char *Key, char *txid)
{
	KeyInfo *tempkeyinfo = (KeyInfo *)malloc(sizeof(KeyInfo));
	char *tempstr = (char *)malloc(MEMORY_ALLOCATION_MID*sizeof(char));
	memset(tempkeyinfo, 0x00, sizeof(tempkeyinfo));
	memset(tempstr, 0x00, sizeof(tempstr));

	char *todelkey = NULL;

	if (!ReadKeyInfo(db, Key, tempkeyinfo))
		return false;

	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	for (it->Seek(tempkeyinfo->EndKey); it->Valid(); it->Prev())
	{

		tempstr = (char *)it->value().ToString().c_str();
		if (strcmp(tempstr, txid) == 0)
		{
			todelkey = (char *)it->key().ToString().c_str();

			if (tempkeyinfo->RecordSum == 1)
			{
				if (strcmp(todelkey, tempkeyinfo->StartKey) == 0)
				{
					s = db->Delete(leveldb::WriteOptions(), Key);
					if (!s.ok())
						return false;
				}
			}
			else if (strcmp(todelkey, tempkeyinfo->EndKey) == 0)
			{
				it->Prev();
				strcpy(tempkeyinfo->EndKey, it->key().ToString().c_str());
			}
			else if (strcmp(todelkey, tempkeyinfo->StartKey) == 0)
			{
				it->Next();
				strcpy(tempkeyinfo->StartKey, it->key().ToString().c_str());
			}

			break;
		}
	}

	if (todelkey == NULL)
		return false;

	s = db->Delete(leveldb::WriteOptions(), todelkey);

	if (tempkeyinfo->RecordSum > 1)
	{
		if (!UpdateKeyInfo(db, Key, tempkeyinfo))
			return false;
	}

	return true;

}

int64_t TxDBProcess::GetTotalNum(leveldb::DB *db, char *Key)
{
	KeyInfo *tempkeyinfo = (KeyInfo *)malloc(sizeof(KeyInfo));
	memset(tempkeyinfo, 0x00, sizeof(tempkeyinfo));
	if (!ReadKeyInfo(db, Key, tempkeyinfo))
		return 0;
	return tempkeyinfo->RecordSum;
}
int64_t TxDBProcess::GetHeight(leveldb::DB *db, char *Key)
{
	KeyInfo *tempkeyinfo = (KeyInfo *)malloc(sizeof(KeyInfo));
	memset(tempkeyinfo, 0x00, sizeof(tempkeyinfo));
	if (!ReadKeyInfo(db, Key, tempkeyinfo))
		return 0;
	return tempkeyinfo->KeyHight;

}


/************************************************************************/
/* FuctionName: Insert  (Atomic Updates)
* Explain:  Insert Key-value sequence
* return: ok true   failed false
* @parameter1: db (leveldb::DB *)  input  database path and name
* @parameter2: Key (char *)  input a uniqueness key
* @parameter3: Value (char *)  input a data value
* three put operations make Atomic update
* (key =Key value = sum-max )
* (key = KeyIndex value = Value)
* (key = KeyValue value = KeyIndex)
*  2017-11-22  ck(kevin)
*/
/************************************************************************/

bool TxDBProcess::Insert(leveldb::DB *db, char *Key, char* Value)
{
	//judge to write record is exist
	
	char * keyindex = (char*)malloc(257 * sizeof(char));
	if (SearchKeyTxByIndex(db, Key, keyindex, Value))
	{
		free(keyindex); keyindex = NULL;
		return true;
	}
	free(keyindex); keyindex = NULL;
	
	//define want to use string var and Allocate memory
	char *RecSum = (char *)malloc(17 * sizeof(char));
	char *RecMax = (char *)malloc(17 * sizeof(char));
	char *RecSumAndRecMax = (char *)malloc(35 * sizeof(char));
	char *RecMaxLong = (char *)malloc(35 * sizeof(char));
	
	//get record sum and max record number
	long recsum = GetSum(db, Key);
	long recmax = GetMax(db, Key);


	// make new counter and key sequence
	if (recsum == 0)
	{
		strcpy(RecSumAndRecMax, (const char*)"1-1");
		strcpy(RecMaxLong, (const char*)"0000000000000001");
	}
	else
	{
		recsum++;
		recmax++;
		sprintf(RecSum, "%d", recsum);
		sprintf(RecMax, "%d", recmax);
		strcpy(RecSumAndRecMax, RecSum);
		strcat(RecSumAndRecMax, (const char*)"-");
		strcat(RecSumAndRecMax, RecMax);
		char * tmpstr = (char *)malloc(33 * sizeof(char));
		strcpy(tmpstr, (const char*)"0000000000000000");
		strcat(tmpstr, RecMax);
		strcpy(RecMaxLong, tmpstr + strlen(RecMax));
		free(tmpstr); tmpstr = NULL;
	}

	// make KeyIndex
	char * KeyIndex = (char *)malloc(257 * sizeof(char));
	strcpy(KeyIndex, Key);
	strcat(KeyIndex, RecMaxLong);

	//make KeyValue
	char *KeyValue = (char *)malloc(257 * sizeof(char));
	strcpy(KeyValue, Key);
	strcat(KeyValue, Value);
	//cout << "5" << endl;

	//Automic update
	leveldb::WriteBatch Insertbatch;
	Insertbatch.Put(Key, RecSumAndRecMax);
	Insertbatch.Put(KeyIndex, Value);
	Insertbatch.Put(KeyValue, KeyIndex);

	s = db->Write(leveldb::WriteOptions(), &Insertbatch);
	if (!s.ok())
	{
		free(RecSum); RecSum = NULL;
		free(RecMax); RecMax = NULL;
		free(RecSumAndRecMax); RecSumAndRecMax = NULL;
		free(RecMaxLong); RecMaxLong = NULL;
		free(KeyValue); KeyValue = NULL;
		free(KeyIndex); KeyIndex = NULL;
		return false;
	}

	free(RecSum); RecSum = NULL;
	free(RecMax); RecMax = NULL;
	free(RecSumAndRecMax); RecSumAndRecMax = NULL;
	free(RecMaxLong); RecMaxLong = NULL;
	free(KeyValue); KeyValue = NULL;
	free(KeyIndex); KeyIndex = NULL;
	return true;

}

/************************************************************************/
/* FuctionName: Select
* Explain:  Get values by key
* return: ok true   failed false
* @parameter1: db  (leveldb::DB *) input  database path and name
* @parameter2: Key  （char *） input a uniqueness key
* @parameter3: values  (vector<string>) output  values data
* @parameter4: newest_num  (long) input   want to get values quantity, default(0) return all records
* 2017-11-22  ck(kevin)
*/
/************************************************************************/

long TxDBProcess::Select(leveldb::DB *db, char *Key, std::vector<std::string> &values, long newest_num)
{
	long recsum = GetSum(db, Key);
	if (recsum <= 0) return recsum;
	long recmax = GetMax(db, Key);
	if (recmax <= 0) return recmax;

	char * KeyIndex = (char *)malloc(257 * sizeof(char));
	if (!GetMaxKey(db, Key, KeyIndex))
	{
		return false;
	}
	
	long readsum;
	if (newest_num == 0)
		readsum = recsum;
	else
		readsum = recsum < newest_num ? recsum : newest_num;
	int i = 0;
	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	for (it->Seek(KeyIndex); it->Valid() && i < readsum; it->Prev())
	{
		if (it->value().ToString().find('-') != -1)
			break;
		values.push_back(it->value().ToString());
		i++;	
	}
	delete it;
	free(KeyIndex); KeyIndex = NULL;
	return true;
}

/*Select overloading */
/************************************************************************/
/* FuctionName: Select
* Explain:  Get values by key
* return: ok true   failed false
* @parameter1: db  (leveldb::DB *) input  database path and name
* @parameter2: Key  （char *） input a uniqueness key
* @parameter3: values  (vector<string>) output  values data
* @parameter4: newest_num  (long) input   want to get values quantity
* @parameter5: value (char *) input value to search the value postion and return front newest_num records
*  2017-11-22  ck(kevin)
*/
/************************************************************************/
long TxDBProcess::Select(leveldb::DB *db, char *Key, std::vector<std::string> &values, long newest_num, char *Value)
{
	//get sum the Key-Value and judge recsum is <=0
	long recsum = GetSum(db, Key);
	if (recsum <= 0) return recsum;

	//judge to write record is exist
	char *keyindex = (char*)malloc(257 * sizeof(char));
	if (!SearchKeyTxByIndex(db, Key, keyindex, Value))
		return 0;

	int i = 0;
	long readsum = recsum < newest_num ? recsum : (newest_num+1);

	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	for (it->Seek(keyindex); it->Valid() && i<readsum; it->Prev())
	{
		if (i == 0)
		{
			i++;
			continue;
		}
		if (it->value().ToString().find('-') != -1)
			break;
		values.push_back(it->value().ToString());
		i++;
	}
	delete it;
	free(keyindex); keyindex = NULL;
	return true;
}

/************************************************************************/
/* FuctionName: DeleteTx
* Explain:  delete a key-value and update counter
* return: 1 ok,0 failed
* @parameter1: db  (leveldb::DB *) input  database path and name
* @parameter2: Key  （char *） input a uniqueness key to delete sonkey
* @parameter5: value (char *) input value to delete
*  2017-11-22  ck(kevin)
*/
/************************************************************************/
int TxDBProcess::DeleteTx(leveldb::DB *db, char *Key, char* Value)
{
	// get max and sum
	long max = GetMax(db, Key);
	long sum = GetSum(db, Key);
	if (sum == 0 || max == 0)
		return 0;
	// get keyindex by Key and Value
	char *keyindex = (char *)malloc(257 * sizeof(char));
	char *keyvalue = (char *)malloc(257 * sizeof(char));
	strcpy(keyvalue, Key);
	strcat(keyvalue, Value);
	if (!SearchKeyTxByIndex(db, Key, keyindex, Value))
		return 0;
	// if only one record, delete keyinfo_table and delete keyindex record 
	if (sum == 1)
	{
		leveldb::WriteBatch delbatch;
		delbatch.Delete(Key);
		delbatch.Delete(keyindex);
		delbatch.Delete(keyvalue);
		s = db->Write(leveldb::WriteOptions(), &delbatch);
		if (!s.ok())
		{
			free(keyindex); keyindex = NULL;
			return 0;
		}
		free(keyindex); keyindex = NULL;
		free(keyvalue); keyvalue = NULL;
		return 1;
	}

	//if not only one record then get maxkey record
	char * maxkey = (char *)malloc(257 * sizeof(char));
	if (!GetMaxKey(db, Key, maxkey))
	{
		free(keyindex); keyindex = NULL;
		free(maxkey); maxkey = NULL;
		free(keyvalue); keyvalue = NULL;
		return 0;
	}

	// judge if to delete key is maxkey find it and find prevkey to give getkeyinfo
	char *getkeyinfo = (char *)malloc(257 * sizeof(char));
	if (strcmp(maxkey, (const char*)keyindex) == 0)
	{
		leveldb::Iterator * it = db->NewIterator(leveldb::ReadOptions());
		it->Seek(maxkey);
		it->Prev();
		strcpy(getkeyinfo, (const char*)it->key().ToString().c_str());
		delete it;
	}
	//if to delete key is not the maxkey copy maxkey to getkeyinfo
	else
	{
		strcpy(getkeyinfo, maxkey);
	}
	// all total subtract 1
	sum--;
	// make keyinfo record
	char *maxstr = (char *)malloc(17 * sizeof(char));
	char *sumstr = (char *)malloc(17 * sizeof(char));
	char *sum_max = (char *)malloc(35 * sizeof(char));
	strcpy(maxstr, (const char*)(getkeyinfo + (strlen(getkeyinfo) - 16)));
	sscanf(maxstr, "%d", &max);
	sprintf(sumstr, "%d", sum);
	sprintf(maxstr, "%d", max);
	strcpy(sum_max, sumstr);
	strcat(sum_max, (const char*)"-");
	strcat(sum_max, maxstr);

	leveldb::WriteBatch batch;
	batch.Delete(keyindex);
	batch.Delete(keyvalue);
	batch.Put(Key, sum_max);
	s = db->Write(leveldb::WriteOptions(), &batch);
	if (!s.ok())
	{
		free(getkeyinfo);	getkeyinfo = NULL;
		free(maxkey); maxkey = NULL;
		free(maxstr); maxstr = NULL;
		free(sumstr); sumstr = NULL;
		free(sum_max); sum_max = NULL;
		free(keyindex); keyindex = NULL;
		free(keyvalue); keyvalue = NULL;
		return 0;
	}

	free(getkeyinfo);	getkeyinfo = NULL;
	free(maxkey); maxkey = NULL;
	free(maxstr); maxstr = NULL;
	free(sumstr); sumstr = NULL;
	free(sum_max); sum_max = NULL;
	free(keyindex); keyindex = NULL;
	free(keyvalue); keyvalue = NULL;

	return 1;  //Returns the number of deletes

}


long TxDBProcess::GetSum(leveldb::DB *db, char *Key)
{
	int sum;
	std::string keyvalue;
	s = db->Get(leveldb::ReadOptions(), Key, &keyvalue);
	if (!s.ok())
		return 0;

	char *sumstr = (char *)malloc(17 * sizeof(char));
	char *getkeyinfo = (char *)malloc(33 * sizeof(char));
	//getkeyinfo = (char *)keyvalue.c_str();
	strcpy(getkeyinfo, (const char*)keyvalue.c_str());
	int _pos = keyvalue.find('-');
	strncpy(sumstr, getkeyinfo, _pos);
	sscanf(sumstr, "%d", &sum);
	free(sumstr); sumstr = NULL;
	free(getkeyinfo); getkeyinfo = NULL;
	return sum;

}

long TxDBProcess::GetMax(leveldb::DB *db, char *Key)
{
	int max;
	std::string keyvalue;
	s = db->Get(leveldb::ReadOptions(), Key, &keyvalue);
	if (!s.ok())
		return 0;

	char *maxstr = (char *)malloc(17 * sizeof(char));
	char *getkeyinfo = (char *)malloc(33 * sizeof(char));
	strcpy(getkeyinfo, (const char*)keyvalue.c_str());
	int _pos = keyvalue.find('-');
	strcpy(maxstr, (const char*)(getkeyinfo + _pos + 1));
	sscanf(maxstr, "%d", &max);
	free(maxstr); maxstr = NULL;
	free(getkeyinfo); getkeyinfo = NULL;
	return max;
}


/*Iterator iterates over the calendar to find out if a record exists*/
bool TxDBProcess::SearchKeyTxByItor(leveldb::DB *db, char *Key, char *Value)
{
	int sum = GetSum(db, Key);
	if (sum == 0) return false;
	std::vector<std::string> ExistTxid;
	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	int i = 0;
	for (it->Seek(Key); it->Valid() && i <= sum; it->Next())
	{
		if (strcmp(it->value().ToString().c_str(), Value) == 0)
		{
			ExistTxid.push_back(it->key().ToString());
		}

	}
	delete it;
	if (ExistTxid.size() == 0)
		return false;
	return true;
}

/*The index method query is whether a record exists*/
bool TxDBProcess::SearchKeyTxByIndex(leveldb::DB *db, char *Key, char * KeyIndex, char *Value)
{
	int sum = GetSum(db, Key);
	if (sum == 0) return false;

	//Define query targets Key
	char *searchdest = (char *)malloc(257 * sizeof(char));

	strcpy(searchdest, Key);
	strcat(searchdest, Value);
	std::string keyindex;
	s = db->Get(leveldb::ReadOptions(), searchdest, &keyindex);
	if (!s.ok())
	{
		free(searchdest); searchdest = NULL;
		return false;
	}

	std::string valuestr;
	s = db->Get(leveldb::ReadOptions(), keyindex, &valuestr);
	if (!s.ok())
		return false;

	char *searchvalue = (char *)malloc(129 * sizeof(char));
	strcpy(searchvalue, (const char*)valuestr.c_str());

	if (strcmp(Value, searchvalue) != 0)
	{
		free(searchdest); searchdest = NULL;
		free(searchvalue), searchvalue = NULL;
		return false;
	}
	else
	{
		strcpy(KeyIndex, (const char*)keyindex.c_str());

		free(searchdest); searchdest = NULL;
		free(searchvalue), searchvalue = NULL;
		return true;
	}

}

bool TxDBProcess::GetMaxKey(leveldb::DB *db, char *Key, char *MaxKey)
{
	long recmax = GetMax(db, Key);
	if (recmax <= 0) return false;
	char *RecMax = (char *)malloc(17 * sizeof(char));
	sprintf(RecMax, "%d", recmax);
	char * tmpstr = (char *)malloc(257 * sizeof(char));
	char *ptmpstr = tmpstr;
	strcpy(tmpstr, (const char*)"0000000000000000");
	strcat(tmpstr, RecMax);
	tmpstr += strlen(RecMax);
	strcpy(MaxKey, Key);
	strcat(MaxKey, tmpstr);
	free(RecMax); RecMax = NULL;
	tmpstr = ptmpstr;
	free(tmpstr); tmpstr = NULL;
	return true;
}

/************************************************************************/
/* FuctionName: Insert  (Atomic Updates)
* Explain:  Insert Key-value sequence
* return: ok true   failed false
* @parameter1: db (leveldb::DB *)  input  database path and name
* @parameter2: Key (char *)  input a uniqueness key  txid
* @parameter3: txdatas (std::vector<CTxaddressData> )  iutput a vector  type data
* three put operations make Atomic update

*  2017-11-23  ck(kevin)
*/
/************************************************************************/

bool TxDBProcess::Insert(leveldb::DB *db, char *Key, std::vector<CTxaddressData> &TxInfo)
{

	int nTxInfo = TxInfo.size();

	char *TxInfoStr = (char *)malloc(nTxInfo*(64 * sizeof(char)+sizeof(int)+sizeof(long long)));
	memset(TxInfoStr, 0x00, sizeof(TxInfoStr));
	char *nTypeStr = (char *)malloc(2 * sizeof(char));
	char *amountStr = (char *)malloc(33 * sizeof(char));
	for (int i = 0; i < nTxInfo; i++)
	{
		strcat(TxInfoStr,(const char*)",");
		strcat(TxInfoStr, (const char *)TxInfo.at(i).address.c_str());
		strcat(TxInfoStr, (const char*)",");
		sprintf(nTypeStr, "%d", TxInfo.at(i).ntype);
		strcat(TxInfoStr, nTypeStr);
		strcat(TxInfoStr, (const char*)",");
		sprintf(amountStr, "%lld", TxInfo.at(i).amount);
		strcat(TxInfoStr, amountStr);
		strcat(TxInfoStr, (const char*)"@");

	}
	s = db->Put(leveldb::WriteOptions(), Key, TxInfoStr);
	if (!s.ok())
	{
		free(TxInfoStr); TxInfoStr = NULL;
		free(nTypeStr); nTypeStr = NULL;
		free(amountStr); amountStr = NULL;
		return false;
	}

	free(TxInfoStr); TxInfoStr = NULL;
	free(nTypeStr); nTypeStr = NULL;
	free(amountStr); amountStr = NULL;
	return true;
}


/*Select overloading2 */
/************************************************************************/
/* FuctionName: Select
* Explain:  Get values by key
* return: ok true   failed false
* @parameter1: db  (leveldb::DB *) input  database path and name
* @parameter2: Key  （char *） input a uniqueness key
* @parameter3: txdatas(std::vector<CTxaddressData>) output  std::vector<CTxaddressData> type data

*  2017-11-22  ck(kevin)
*/
/************************************************************************/
bool TxDBProcess::Select(leveldb::DB *db, char *Key, std::vector<CTxaddressData> &TxInfo)
{
	CTxaddressData ctad;
	std::string txinfostr;
	
	char *nTypeStr = (char *)malloc(2 * sizeof(char));
	char *amountStr = (char *)malloc(33 * sizeof(char));
	s = db->Get(leveldb::ReadOptions(), Key, &txinfostr);
	if (!s.ok())
	{
		free(nTypeStr); nTypeStr = NULL;
		free(amountStr); amountStr = NULL;
		return false;
	}
	std::string tmpstr;
	int pos = 0;
	while (pos != -1)
	{
		memset(amountStr, 0x00, sizeof(amountStr));
		pos = txinfostr.find(',');
		txinfostr = txinfostr.substr(pos+1);
		pos = txinfostr.find(',');
		ctad.address = txinfostr.substr(0,pos);
		txinfostr = txinfostr.substr(pos + 1);
		pos = txinfostr.find(',');
		strncpy(nTypeStr, (const char *)txinfostr.c_str(), pos);
		sscanf(nTypeStr, "%d", &ctad.ntype);
		txinfostr = txinfostr.substr(pos + 1);
		pos = txinfostr.find('@');
		std::string str1 = txinfostr.substr(0, pos);
		sscanf((char*)str1.c_str(), "%lld", &ctad.amount);
		txinfostr = txinfostr.substr(pos + 1);
		TxInfo.push_back(ctad);
		pos = txinfostr.find(',');
	
	}

	
	free(nTypeStr); nTypeStr = NULL;
	free(amountStr); amountStr = NULL;


	
	return true;
}


//end

static leveldb::Options GetOptions(size_t nCacheSize)
{
    leveldb::Options options;
    options.block_cache = leveldb::NewLRUCache(nCacheSize / 2);
    options.write_buffer_size = nCacheSize / 4; // up to two write buffers may be held in memory simultaneously
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.compression = leveldb::kNoCompression;
    options.max_open_files = 64;
    if (leveldb::kMajorVersion > 1 || (leveldb::kMajorVersion == 1 && leveldb::kMinorVersion >= 16)) {
        // LevelDB versions before 1.16 consider short writes to be corruption. Only trigger error
        // on corruption in later versions.
        options.paranoid_checks = true;
    }
    return options;
}

CDBWrapper::CDBWrapper(const boost::filesystem::path& path, size_t nCacheSize, bool fMemory, bool fWipe, bool obfuscate)
{
    penv = NULL;
    readoptions.verify_checksums = true;
    iteroptions.verify_checksums = true;
    iteroptions.fill_cache = false;
    syncoptions.sync = true;
    options = GetOptions(nCacheSize);
    options.create_if_missing = true;
    if (fMemory) {
        penv = leveldb::NewMemEnv(leveldb::Env::Default());
        options.env = penv;
    } else {
        if (fWipe) {
            LogPrintf("Wiping LevelDB in %s\n", path.string());
            leveldb::Status result = leveldb::DestroyDB(path.string(), options);
            dbwrapper_private::HandleError(result);
        }
        TryCreateDirectory(path);
        LogPrintf("Opening LevelDB in %s\n", path.string());
    }
    leveldb::Status status = leveldb::DB::Open(options, path.string(), &pdb);
    dbwrapper_private::HandleError(status);
    LogPrintf("Opened LevelDB successfully\n");

    // The base-case obfuscation key, which is a noop.
    obfuscate_key = std::vector<unsigned char>(OBFUSCATE_KEY_NUM_BYTES, '\000');

    bool key_exists = Read(OBFUSCATE_KEY_KEY, obfuscate_key);

    if (!key_exists && obfuscate && IsEmpty()) {
        // Initialize non-degenerate obfuscation if it won't upset
        // existing, non-obfuscated data.
        std::vector<unsigned char> new_key = CreateObfuscateKey();

        // Write `new_key` so we don't obfuscate the key with itself
        Write(OBFUSCATE_KEY_KEY, new_key);
        obfuscate_key = new_key;

        LogPrintf("Wrote new obfuscate key for %s: %s\n", path.string(), HexStr(obfuscate_key));
    }

    LogPrintf("Using obfuscation key for %s: %s\n", path.string(), HexStr(obfuscate_key));
}

CDBWrapper::~CDBWrapper()
{
    delete pdb;
    pdb = NULL;
    delete options.filter_policy;
    options.filter_policy = NULL;
    delete options.block_cache;
    options.block_cache = NULL;
    delete penv;
    options.env = NULL;
}

bool CDBWrapper::WriteBatch(CDBBatch& batch, bool fSync)
{
    leveldb::Status status = pdb->Write(fSync ? syncoptions : writeoptions, &batch.batch);
    dbwrapper_private::HandleError(status);
    return true;
}

// Prefixed with null character to avoid collisions with other keys
//
// We must use a string constructor which specifies length so that we copy
// past the null-terminator.
const std::string CDBWrapper::OBFUSCATE_KEY_KEY("\000obfuscate_key", 14);

const unsigned int CDBWrapper::OBFUSCATE_KEY_NUM_BYTES = 8;

/**
 * Returns a string (consisting of 8 random bytes) suitable for use as an
 * obfuscating XOR key.
 */
std::vector<unsigned char> CDBWrapper::CreateObfuscateKey() const
{
    unsigned char buff[OBFUSCATE_KEY_NUM_BYTES];
    GetRandBytes(buff, OBFUSCATE_KEY_NUM_BYTES);
    return std::vector<unsigned char>(&buff[0], &buff[OBFUSCATE_KEY_NUM_BYTES]);

}

bool CDBWrapper::IsEmpty()
{
    std::unique_ptr<CDBIterator> it(NewIterator());
    it->SeekToFirst();
    return !(it->Valid());
}

CDBIterator::~CDBIterator() { delete piter; }
bool CDBIterator::Valid() { return piter->Valid(); }
void CDBIterator::SeekToFirst() { piter->SeekToFirst(); }
void CDBIterator::Next() { piter->Next(); }

namespace dbwrapper_private {

void HandleError(const leveldb::Status& status)
{
    if (status.ok())
        return;
    LogPrintf("%s\n", status.ToString());
    if (status.IsCorruption())
        throw dbwrapper_error("Database corrupted");
    if (status.IsIOError())
        throw dbwrapper_error("Database I/O error");
    if (status.IsNotFound())
        throw dbwrapper_error("Database entry missing");
    throw dbwrapper_error("Unknown database error");
}

const std::vector<unsigned char>& GetObfuscateKey(const CDBWrapper &w)
{
    return w.obfuscate_key;
}

};
