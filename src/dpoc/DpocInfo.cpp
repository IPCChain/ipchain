
#include "DpocInfo.h"
#include "../util.h"

CDpocInfo*  CDpocInfo::_instance = NULL;
std::once_flag CDpocInfo::init_flag;

CDpocInfo::CDpocInfo():strLocalFirst("localAccount"),
                        strSumFirst("sumNum"),
                        strListFirst("packeageList"),
                        strAccoutFirst("accout"),
                        strHightFirst("hight"),
                        strFeeFirst("fee"),
                        strTimeFirst("time"),
	                    strStatusFirst("consensusStatus"),
	                    strPunishHashFirst("punishHash"),
	                    m_bHasJoinCampaign(false)
{
	m_strLocalAccout.clear();
	boost::filesystem::path dirPath = GetDataDir();
	strFilePath += dirPath.string();
	strFilePath += "/dpoc_info";
	infoFile.open(strFilePath, std::ios::out| ~std::ios::trunc);
	infoFile.close();
}

CDpocInfo::~CDpocInfo()
{
	if (infoFile.is_open())
	{
		infoFile.close();
	}
}

void CDpocInfo::CreateInstance() {
	static CDpocInfo instance;
	CDpocInfo::_instance = &instance;
}

CDpocInfo&  CDpocInfo::Instance()
{
	std::call_once(CDpocInfo::init_flag, CDpocInfo::CreateInstance);
	return *CDpocInfo::_instance;
}

void CDpocInfo::ReadFromFile(std::string &strJson)
{
	writeLock  wtlock(rwmutex);
	if (!infoFile.is_open())
	{
		infoFile.open(strFilePath);
	}

	if (infoFile.is_open())
	{
		infoFile.seekg(0, std::ios::beg);
		
		while (!infoFile.eof() && infoFile.peek() != EOF)
		{
			std::string strTemp;
			std::getline(infoFile, strTemp);
			strJson += strTemp;
		}	

		infoFile.close();
	}
}

void CDpocInfo::WriteToFile(const std::string &strJson)
{
	writeLock  wtlock(rwmutex);
	if (infoFile.is_open())
	{
		infoFile.close();	
	}

	infoFile.open(strFilePath, std::ios::in | std::ios::out | std::ios::trunc);

	if (infoFile.is_open())
	{
		infoFile << strJson;
		infoFile.flush();

		infoFile.close();
	}
}

bool CDpocInfo::GetBlockInfo(std::vector<BlockInfo> &vecInfo,int &nSum)
{
	try
	{
		std::string strJson;
		ReadFromFile(strJson);
		if (strJson.empty())
		{
			return false;
		}
		std::size_t position = strJson.find(strLocalFirst);
		if (position == std::string::npos)
		{
			return false;
		}

		std::size_t positionSum = strJson.find(strSumFirst);
		if (positionSum == std::string::npos)
		{
			return false;
		}

		std::stringstream stream;
		stream << strJson;

		boost::property_tree::ptree pt;
	
		boost::property_tree::json_parser::read_json<boost::property_tree::ptree>(stream, pt);
	
		nSum = pt.get<int>(strSumFirst);
		if (0 == nSum)
		{
			return true;
		}

		boost::property_tree::ptree pChild = pt.get_child(strListFirst);
		for (boost::property_tree::ptree::iterator it = pChild.begin(); it != pChild.end(); ++it)
		{
			boost::property_tree::ptree p2 = it->second; 
			BlockInfo stInfo;
			stInfo.strAccout = p2.get<std::string>(strAccoutFirst);
			stInfo.strHight = p2.get<std::string>(strHightFirst);
			stInfo.strFee = p2.get<std::string>(strFeeFirst);
			stInfo.strTime = p2.get<std::string>(strTimeFirst);
			vecInfo.push_back(stInfo);
		}
	}
	catch (boost::property_tree::json_parser::json_parser_error &errCode)
	{
		LogPrintf("[CDpocInfo::GetBlockInfo] error message is %s\n", errCode.message());
		return false;
	}
	catch (boost::property_tree::ptree_bad_path &errCode)
	{
		LogPrintf("[CDpocInfo::GetBlockInfo] error message is %s\n", errCode.what());
		return false;
	}
	catch (...)
	{
		LogPrintf("[CDpocInfo::GetBlockInfo] return false\n");
		return false;
	}

	return true;
}

bool CDpocInfo::GetLocalAccount(std::string &strPublicKey)
{
	try
	{
		strPublicKey.clear();

		std::string strJson;
		ReadFromFile(strJson);
		if (strJson.empty())
		{
			LogPrintf("[CCDpocInfo::GetLocalAccout] return false by strJson.empty()\n");
			return false;
		}

		std::size_t position = strJson.find(strLocalFirst);
		if (position == std::string::npos)
		{
			LogPrintf("[CCDpocInfo::GetLocalAccout] return false by position == std::string::npos\n");
			return false;
		}
		std::stringstream stream;
		stream << strJson;

		boost::property_tree::ptree pt;

		boost::property_tree::json_parser::read_json<boost::property_tree::ptree>(stream, pt);
		strPublicKey = pt.get<std::string>(strLocalFirst);
	}
	catch (boost::property_tree::json_parser::json_parser_error &errCode)
	{
		LogPrintf("CCDpocInfo::GetLocalAccout error message is %s\n", errCode.message());
		return false;
	}
	catch (boost::property_tree::ptree_bad_path &errCode)
	{
		LogPrintf("CCDpocInfo::GetLocalAccout error message is %s\n", errCode.what());
		return false;
	}
	catch (...)
	{
		LogPrintf("[CCDpocInfo::GetLocalAccout] return false\n");
		return false;
	}


	if (strPublicKey.empty())
	{
		LogPrintf("[CCDpocInfo::GetLocalAccout] return false\n");
		return false;
	}
	else
	{
		LogPrintf("[CCDpocInfo::GetLocalAccout] return true\n");
		return true;
	}
	
}

int CDpocInfo::SetLocalAccount(const std::string &strPublicKey)
{
	try
	{
		std::string strJson;
		ReadFromFile(strJson);
	
		boost::property_tree::ptree root;

		std::size_t position = strJson.find(strLocalFirst);
	
		//If the file is empty
		if ((strJson.empty())|| (position == std::string::npos))
		{
			root.put<std::string>(strLocalFirst, strPublicKey);
			root.put<int>(strSumFirst, 0);
		}
		else
		{
			//This PublicKey is already in the local file and exits
			if (IsHasLocalAccount())
			{
				LogPrintf("[CDpocInfo::SetLocalAccout]end by false -1\n");
				return -1;
			}

			std::stringstream oldSStream;
			oldSStream << strJson;
			
			boost::property_tree::json_parser::read_json<boost::property_tree::ptree>(oldSStream, root);
			
			root.put<std::string>(strLocalFirst, strPublicKey);	
			root.put<int>(strSumFirst, 0);
		}
		
		std::stringstream wSStream;
		boost::property_tree::write_json(wSStream, root);
		
		WriteToFile(wSStream.str());
	}
	catch (boost::property_tree::json_parser::json_parser_error &errCode)
	{
		LogPrintf("[CDpocInfo::SetLocalAccout] error message is %s\n", errCode.message());
		return -2;
	}
	catch (...)
	{
		LogPrintf("[CDpocInfo::SetLocalAccout] return false\n");
		return -2;
	}

	LogPrintf("[CDpocInfo::SetLocalAccout]end by true\n");
	 return 0;
}

bool CDpocInfo::SetBlockInfo(const BlockInfo &stBlockInfo, const std::string &strPublicKey)
{
	LogPrintf("[CDpocInfo::SetBlockInfo] begin\n");
	try
	{
		if (-2 == SetLocalAccount(strPublicKey))
		{
			return false;
		}

		std::string strJson;
		ReadFromFile(strJson);

		std::stringstream oldSStream;
		oldSStream << strJson;
		boost::property_tree::ptree root;
		boost::property_tree::json_parser::read_json<boost::property_tree::ptree>(oldSStream, root);

		boost::property_tree::ptree newPtree;
		newPtree.put<std::string>(strAccoutFirst, stBlockInfo.strAccout);
		newPtree.put<std::string>(strHightFirst, stBlockInfo.strHight);
		newPtree.put<std::string>(strFeeFirst, stBlockInfo.strFee);
		newPtree.put<std::string>(strTimeFirst, stBlockInfo.strTime);

		int nSum = root.get<int>(strSumFirst);
		
		if (0 < root.count(strListFirst))
		{
			boost::property_tree::ptree pChild = root.get_child(strListFirst);

			int nCount = pChild.count("");
			if (20 <= nCount)
			{
				pChild.pop_back();
				nSum = nCount - 1;
			}

			pChild.push_front(make_pair("", newPtree));
			root.put_child(strListFirst, pChild);
			++nSum;
		}
		else
		{
			boost::property_tree::ptree newChild;
			newChild.push_front(make_pair("", newPtree));
			root.add_child(strListFirst, newChild);
			++nSum;
		}
		root.put<int>(strSumFirst, nSum);

		std::stringstream wSStream;
		boost::property_tree::write_json(wSStream, root);
		WriteToFile(wSStream.str());
	}
	catch (boost::property_tree::json_parser::json_parser_error &errCode)
	{
		LogPrintf("[CDpocInfo::SetBlockInfo] error message is %s\n", errCode.message());
		return false;
	}
	catch (boost::property_tree::ptree_bad_path &errCode)
	{
		LogPrintf("[CDpocInfo::SetBlockInfo] error message is %s\n", errCode.what());
		return false;
	}
	catch (...)
	{
		LogPrintf("[CDpocInfo::SetBlockInfo] return false\n");
		return false;
	}
	LogPrintf("[CDpocInfo::SetBlockInfo] return true\n");
	return true;
}

bool CDpocInfo::RemoveInfo()
{
	LogPrintf("[CDpocInfo::RemoveInfo] begin\n");
	try
	{
		std::string strStatus;
		std::string strPunishHash;
		GetConsensusStatus(strStatus, strPunishHash);

		std::string strJson;
		WriteToFile(strJson);

		if (!(strStatus.empty()&&strPunishHash.empty()))
		{
			SetConsensusStatus(strStatus, strPunishHash,true);
		}

		std::string strPublicKey;
		setLocalAccoutVar(strPublicKey);
	}
	catch (...)
	{
		LogPrintf("[CDpocInfo::RemoveInfo] return false\n");
		return false;
	}
	LogPrintf("[CDpocInfo::RemoveInfo] end by true\n");
	return true;

}

bool CDpocInfo::ChangeFilePath(std::string &strPath)
{
	if (strPath.empty())
	{
		LogPrintf("[CDpocInfo::ChangeFilePath] InPut is NULL\n");
		return false;
	}

	strPath += "/dpoc_info";
	if (strFilePath != strPath)
	{
		try
		{
			writeLock  wtlock(rwmutex);
			if (infoFile.is_open())
			{
				infoFile.close();
			}
			boost::filesystem::path oldPath(strFilePath);
			boost::filesystem::path newPath(strPath);

			if (boost::filesystem::exists(oldPath))
			{
				boost::filesystem::rename(oldPath,newPath);
			}

			strFilePath = strPath;
			return true;
		}
		catch (...)
		{
			LogPrintf("[CDpocInfo::ChangeFilePath] ChangeFilePath return false\n");
			return false;
		}
	}

	LogPrintf("[CDpocInfo::ChangeFilePath] newPath==oldPath");
	return true;
}

bool CDpocInfo::GetConsensusStatus(std::string &strStatus,std::string &strPunishHash)
{
	try
	{
		std::string strJson;
		ReadFromFile(strJson);
		if (strJson.empty())
		{
			return false;
		}

		std::stringstream stream;
		stream << strJson;

		boost::property_tree::ptree pt;
	
		boost::property_tree::json_parser::read_json<boost::property_tree::ptree>(stream, pt);
		strStatus = pt.get<std::string>(strStatusFirst);
		strPunishHash = pt.get<std::string>(strPunishHashFirst);
	}
	catch (boost::property_tree::json_parser::json_parser_error &errCode)
	{
		LogPrintf("CDpocInfo::GetConsensusStatus error message is %s\n", errCode.message());
		return false;
	}
	catch (boost::property_tree::ptree_bad_path &errCode)
	{
		LogPrintf("CDpocInfo::GetConsensusStatus error message is %s\n", errCode.what());
		return false;
	}
	catch (...)
	{
		LogPrintf("[CDpocInfo::GetConsensusStatus] return false\n");
		return false;
	}

	LogPrintf("[CDpocInfo::GetConsensusStatus] end\n");
	if (strStatus.empty())
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool CDpocInfo::SetConsensusStatus(const std::string &strStatus, const std::string &strPunishHash,const bool bRemove)
{
	try
	{
		std::string strJson;
		ReadFromFile(strJson);

		boost::property_tree::ptree root;

		//When clearing the consensus list, bRemove is set to true
		if (bRemove)
		{
			if (strJson.empty())
			{
				root.put<std::string>(strStatusFirst, strStatus);
				root.put<std::string>(strPunishHashFirst, strPunishHash);
			}
		}
		else
		{
			//If the file is empty, because there is no consensus, there will be no consensus, only one judgment
			if (strJson.empty())
			{
				return false;
			}

			//If there is no PublicKey in the local file, exit
			if (!IsHasLocalAccount())
			{
				return false;
			}

			std::stringstream oldSStream;
			oldSStream << strJson;

			boost::property_tree::json_parser::read_json<boost::property_tree::ptree>(oldSStream, root);

			root.put<std::string>(strStatusFirst, strStatus);
			root.put<std::string>(strPunishHashFirst, strPunishHash);
		}
		std::stringstream wSStream;
		boost::property_tree::write_json(wSStream, root);

		WriteToFile(wSStream.str());
	}
	catch (boost::property_tree::json_parser::json_parser_error &errCode)
	{
		LogPrintf("[CDpocInfo::SetConsensusStatus]error message is %s\n", errCode.message());
		return false;
	}
	catch (...)
	{
		LogPrintf("[CDpocInfo::SetConsensusStatus] return false\n");
		return false;
	}

	LogPrintf("[CDpocInfo::SetConsensusStatus] return true\n");
	return true;
}

bool CDpocInfo::SetTotalAmount(int64_t n64Amount)
{
	writeLockAmount  wtlock(rwmutexAmount);
	std::stringstream ss;
	ss << n64Amount;
	std::string strAmount;
	ss >> strAmount;

	std::string strFilePath;
	boost::filesystem::path dirPath = GetDataDir();
	strFilePath += dirPath.string();
	strFilePath += "/TotalAmount";

	std::ofstream configFile;
	configFile.open(strFilePath, std::ios::out | std::ios::trunc);
	if (configFile.is_open())
	{
		configFile << strAmount;
		configFile.flush();
		configFile.close();
	}
	
	return true;
}

int64_t CDpocInfo::GetTotalAmount()
{
	writeLockAmount  wtlock(rwmutexAmount);
	std::string strFilePath;
	boost::filesystem::path dirPath = GetDataDir();
	strFilePath += dirPath.string();
	strFilePath += "/TotalAmount";

	std::string strAmount;
	if (boost::filesystem::exists(strFilePath))
	{
		std::ifstream configFile;
		configFile.open(strFilePath.c_str());

		if (configFile.is_open())
		{
			configFile.seekg(0, std::ios::beg);

			while (!configFile.eof() && configFile.peek() != EOF)
			{
				std::string str_line;
				std::getline(configFile, str_line);

				strAmount += str_line;
			}

			configFile.close();

			std::stringstream ss;
		    ss << strAmount;
			int64_t nAmount = 0;
			ss >> nAmount;
			return nAmount;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

bool CDpocInfo::IsHasLocalAccount()
{
	if (getJoinCampaign())
	{
		return true;
	}
	return false;
}

void CDpocInfo::setJoinCampaign(bool bJoinCampaign)
{
	joinLock  wtlockJoin(mutexJoin);
	m_bHasJoinCampaign = bJoinCampaign;
}

bool CDpocInfo::getJoinCampaign()
{
	joinLock  wtlockJoin(mutexJoin);
	return m_bHasJoinCampaign;
}

bool CDpocInfo::isReadStdCout()
{
	std::string strFilePath;
	boost::filesystem::path dirPath = GetDataDir();
	strFilePath += dirPath.string();
	strFilePath += "/stdcout.conf";

	if (boost::filesystem::exists(strFilePath))
	{
		std::string strIsCout;
		std::ifstream configFile;
		configFile.open(strFilePath.c_str());

		if (configFile.is_open())
		{
			configFile.seekg(0, std::ios::beg);

			while (!configFile.eof() && configFile.peek() != EOF)
			{
				std::string str_line;
				std::getline(configFile, str_line);

				strIsCout += str_line;
			}

			configFile.close();

			std::stringstream ss;
			ss << strIsCout;
			bool bIsCout = false;
			ss >> bIsCout;

			return bIsCout;
		}
		else
		{
			return false;
		}
	}
	else
	{
		bool bIsCout = false;
		std::stringstream ss;
		ss << bIsCout;
		std::string strIsCout;
		ss >> strIsCout;

		std::ofstream configFile;
		configFile.open(strFilePath, std::ios::out | std::ios::trunc);
		if (configFile.is_open())
		{
			configFile << strIsCout;
			configFile.flush();
			configFile.close();
		}
		return bIsCout;
	}
	return false;
}

void CDpocInfo::setLocalAccoutVar(std::string strPublicKey)
{
	PKLock  localAccoutlock(mutexPK);
	m_strLocalAccout = strPublicKey;
	if (strPublicKey.empty())
	{
		m_strLocalAccout.clear();
	}
	
}

bool CDpocInfo::getLocalAccoutVar(std::string &strPublicKey)
{
	PKLock  localAccoutlock(mutexPK);
	strPublicKey = m_strLocalAccout;
	if (m_strLocalAccout.empty())
	{
		return false;
	}
	return true;
}
