// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// NOTE: This file is intended to be customised by the end user, and includes only local node policy logic

#include "primitives/transaction.h"
#include "policy/policy.h"
#include "chainparams.h"

#include "validation.h"
#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"
#include "base58.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <map>  
#include <utility>      // std::pair
#include "dpoc//ConsensusAccountPool.h"

    /**
     * Check transaction inputs to mitigate two
     * potential denial-of-service attacks:
     * 
     * 1. scriptSigs with extra data stuffed into them,
     *    not consumed by scriptPubKey (or P2SH script)
     * 2. P2SH scripts with a crazy number of expensive
     *    CHECKSIG/CHECKMULTISIG operations
     *
     * Why bother? To avoid denial-of-service attacks; an attacker
     * can submit a standard HASH... OP_EQUAL transaction,
     * which will get accepted into blocks. The redemption
     * script can be anything; an attacker could use a very
     * expensive-to-check-upon-redemption script like:
     *   DUP CHECKSIG DROP ... repeated 100 times... OP_1
     */

bool IsStandard(const CScript& scriptPubKey, txnouttype& whichType, const bool witnessEnabled)
{
    std::vector<std::vector<unsigned char> > vSolutions;
    if (!Solver(scriptPubKey, whichType, vSolutions))
        return false;

    if (whichType == TX_MULTISIG)
    {
        unsigned char m = vSolutions.front()[0];
        unsigned char n = vSolutions.back()[0];
        // Support up to x-of-3 multisig txns as standard
        if (n < 1 || n > 3)
            return false;
        if (m < 1 || m > n)
            return false;
    } else if (whichType == TX_NULL_DATA &&
               (!fAcceptDatacarrier || scriptPubKey.size() > nMaxDatacarrierBytes))
          return false;

    else if (!witnessEnabled && (whichType == TX_WITNESS_V0_KEYHASH || whichType == TX_WITNESS_V0_SCRIPTHASH))
        return false;

    return whichType != TX_NONSTANDARD;
}

bool IsStandardTx(const CTransaction& tx, std::string& reason, const bool witnessEnabled)
{
    if (tx.nVersion > CTransaction::MAX_STANDARD_VERSION || tx.nVersion < 1) {
        reason = "version";
        return false;
    }

    // Extremely large transactions with lots of inputs can cost the network
    // almost as much to process as they cost the sender in fees, because
    // computing signature hashes is O(ninputs*txsize). Limiting transactions
    // to MAX_STANDARD_TX_WEIGHT mitigates CPU exhaustion attacks.
    unsigned int sz = GetTransactionWeight(tx);
    if (sz >= MAX_STANDARD_TX_WEIGHT) {
        reason = "tx-size";
        return false;
    }

    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        // Biggest 'standard' txin is a 15-of-15 P2SH multisig with compressed
        // keys (remember the 520 byte limit on redeemScript size). That works
        // out to a (15*(33+1))+3=513 byte redeemScript, 513+1+15*(73+1)+3=1627
        // bytes of scriptSig, which we round off to 1650 bytes for some minor
        // future-proofing. That's also enough to spend a 20-of-20
        // CHECKMULTISIG scriptPubKey, though such a scriptPubKey is not
        // considered standard.
        if (txin.scriptSig.size() > 1650) {
            reason = "scriptsig-size";
            return false;
        }
        if (!txin.scriptSig.IsPushOnly()) {
            reason = "scriptsig-not-pushonly";
            return false;
        }
    }

    unsigned int nDataOut = 0;
    txnouttype whichType;
    BOOST_FOREACH(const CTxOut& txout, tx.vout) {
        if (!::IsStandard(txout.scriptPubKey, whichType, witnessEnabled)) {
            reason = "scriptpubkey";
            return false;
        }

        if (whichType == TX_NULL_DATA)
            nDataOut++;
        else if ((whichType == TX_MULTISIG) && (!fIsBareMultisigStd)) {
            reason = "bare-multisig";
            return false;
        } else if (txout.IsDust(dustRelayFee)) {
            reason = "dust";
            return false;
        }
    }

    // only one OP_RETURN txout is permitted
    if (nDataOut > 1) {
        reason = "multi-op-return";
        return false;
    }

    return true;
}

bool AreInputsStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs)
{
    if (tx.IsCoinBase())
        return true; // Coinbases don't use vin normally

    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const CTxOut& prev = mapInputs.GetOutputFor(tx.vin[i]);

        std::vector<std::vector<unsigned char> > vSolutions;
        txnouttype whichType;
        // get the scriptPubKey corresponding to this input:
        const CScript& prevScript = prev.scriptPubKey;
        if (!Solver(prevScript, whichType, vSolutions))
            return false;

        if (whichType == TX_SCRIPTHASH)
        {
            std::vector<std::vector<unsigned char> > stack;
            // convert the scriptSig into a stack, so we can inspect the redeemScript
            if (!EvalScript(stack, tx.vin[i].scriptSig, SCRIPT_VERIFY_NONE, BaseSignatureChecker(), SIGVERSION_BASE))
                return false;
            if (stack.empty())
                return false;
            CScript subscript(stack.back().begin(), stack.back().end());
            if (subscript.GetSigOpCount(true) > MAX_P2SH_SIGOPS) {
                return false;
            }
        }
    }

    return true;
}

//知产交易模型类型约束检查子函数
bool IsValidIPCModelCheck(const CTransaction& tx, std::map<uint128, IPCLabel>& InOwnerRecord, std::map<uint128, std::pair<CScript, IPCLabel> >& InAuthorRecord,
	std::map<uint128, IPCLabel>& OutOwnerRecord, std::map<uint128, IPCLabel>& OutUniqueRecord, std::multimap<uint128, std::pair<CScript, IPCLabel> >& OutAuthorRecord, CValidationState& state)
{
	std::map<uint128, IPCLabel>::iterator ipcTxIteator;
	std::map<uint128, std::pair<CScript, IPCLabel> >::iterator ipcInAuthorIteator;
	std::multimap<uint128, std::pair<CScript, IPCLabel> >::iterator ipcOutNormalAuthorIteator;
	
	ipcTxIteator = InOwnerRecord.begin();
	while (ipcTxIteator != InOwnerRecord.end())
	{
		//联合检查。对每个所有权登记的输入，输出中必须有一个相同hash，相同扩展type的所有权输出。且输出的日期不能超过输入的日期限定
		if (OutOwnerRecord.count(ipcTxIteator->first) > 0)
		{
			if (ipcTxIteator->second.ExtendType != OutOwnerRecord[ipcTxIteator->first].ExtendType)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-extendtype-changed");

			if(ipcTxIteator->second.startTime > OutOwnerRecord[ipcTxIteator->first].startTime ||
				((ipcTxIteator->second.stopTime < OutOwnerRecord[ipcTxIteator->first].stopTime) && ipcTxIteator->second.stopTime !=0) ||
				(ipcTxIteator->second.stopTime != 0 && OutOwnerRecord[ipcTxIteator->first].stopTime == 0))
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-timecheck-error");

			ipcTxIteator++;
			continue;
		}
		else
			return state.DoS(100, false, REJECT_INVALID, "bad-IPC-ownership-lost");
	}

	//对于授权输入
	ipcInAuthorIteator = InAuthorRecord.begin();
	while (ipcInAuthorIteator != InAuthorRecord.end())
	{
		//如果输入的再授权标志为0，则报错
		if (ipcInAuthorIteator->second.second.reAuthorize == 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-IPC-reAuthor-with-no-permission");

		//再授权操作约束
		if (InOwnerRecord.count(ipcInAuthorIteator->first) <= 0)
		{
			//对于再授权操作，不能有同一hash的所有权输出
			if (OutOwnerRecord.count(ipcInAuthorIteator->first) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-reAuthor-with-ownership-output");

			//对于再授权操作，必须有一个同样Address，同样Label的授权输出（非唯一性）存在（以免授权丢失）
			bool losted = true;
			bool changed = true;
			if (ipcInAuthorIteator->second.second.uniqueAuthorize == 0)	//输入的授权为非排他性授权 此时比较的是OutAuthorRecord中。。
			{
				ipcOutNormalAuthorIteator = OutAuthorRecord.lower_bound(ipcInAuthorIteator->first);
				if (ipcOutNormalAuthorIteator == OutAuthorRecord.upper_bound(ipcInAuthorIteator->first)) //相等说明没有这个hash的授权输出
					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-Authorize-output(hash have changed)");
				while (ipcOutNormalAuthorIteator != OutAuthorRecord.upper_bound(ipcInAuthorIteator->first))
				{
					if (ipcOutNormalAuthorIteator->second.first == ipcInAuthorIteator->second.first)
					{
						losted = false;			
						break;
					}

					ipcOutNormalAuthorIteator++;
				}
			}
			else if (ipcInAuthorIteator->second.second.uniqueAuthorize == 1) //输入的授权为排他性授权
			{
				if (OutUniqueRecord.count(ipcInAuthorIteator->first) == 0)
					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-Authorize(unique)-hash-changed");

				losted = false;		
			}

			if (losted)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-Authorization-lost");

		}

		ipcInAuthorIteator++;
	}

	//对于所有权输出
	ipcTxIteator = OutOwnerRecord.begin();
	while (ipcTxIteator != OutOwnerRecord.end())
	{
		if (OutUniqueRecord.count(ipcTxIteator->first) == 0 && 
			OutAuthorRecord.count(ipcTxIteator->first) == 0 && 
			InOwnerRecord.count(ipcTxIteator->first) > 0 )
		{
			//modify by xxy  此条约束存在问题，先注释 20171006
			//对于所有权转移交易，输出的starttime不能小于当前区块最新块的时间
// 			if (ipcTxIteator->second.startTime < chainActive.Tip()->GetBlockTime())
// 				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-ownership-output-starttime-earlier-than-currentblock");
			//

			//对于所有权转移交易，输入的starttime不能大于当前区块最新块的时间
			if (InOwnerRecord[ipcTxIteator->first].startTime > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-ownership-input-starttime-later-than-currentblock");
		}
		else if (OutUniqueRecord.count(ipcTxIteator->first) == 0 &&
			OutAuthorRecord.count(ipcTxIteator->first) == 0 &&
			InOwnerRecord.count(ipcTxIteator->first) == 0)
		{
			//对于所有权登记交易，不能重复登记出现过，且txid与本交易不同的hash值
			if (pIPCCheckMaps->IPCHashMap.count(ipcTxIteator->first) &&
				pIPCCheckMaps->IPCHashMap[ipcTxIteator->first].first != tx.GetHash())
			{
				std::cout << "ipchash: "<<ipcTxIteator->first.GetHex()<< std::endl;
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-IPChash-repeat");
			}

			//所有权登记交易中，起始时间不能大于结束时间
			if (ipcTxIteator->second.startTime > ipcTxIteator->second.stopTime && ipcTxIteator->second.stopTime != 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-stoptime-earlier-than-starttime");
			//所有权登记时，登记的ipchash不能为空
			if (ipcTxIteator->second.hash.IsNull())
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-IPChash-can't-be-NUll");

		}
		//对每个所有权类型的输出，其输出的再授权标记必须为1
		if (ipcTxIteator->second.reAuthorize != 1)
			return state.DoS(100, false, REJECT_INVALID, "bad-IPC-ownership-output-no-reAuthor");

		ipcTxIteator++;
		continue;
	}
	//对于排他性授权输出
	ipcTxIteator = OutUniqueRecord.begin();
	while (ipcTxIteator != OutUniqueRecord.end())
	{

		if (InOwnerRecord.count(ipcTxIteator->first) > 0) //所有权授权排他性授权
		{
			//对每个唯一授权类型的输出,若是所有权授权产生的，这笔交易中不允许有同一个hash的其它授权输出存在
			if (OutAuthorRecord.count(ipcTxIteator->first) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-uniqueAuthor-with-otherAuthor-this-tx");

			////对每个唯一授权类型的输出，不允许输入的所有权有过当前有效的其他授权存在
			if (InOwnerRecord[ipcTxIteator->first].uniqueAuthorize != 0)  //0:当前没有其他授权  1：当前有其他授权
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-uniqueAuthor-with-otherAuthor-before");

			if (ipcTxIteator->second.startTime < InOwnerRecord[ipcTxIteator->first].startTime ||
				((ipcTxIteator->second.stopTime > InOwnerRecord[ipcTxIteator->first].stopTime) && (InOwnerRecord[ipcTxIteator->first].stopTime != 0)) ||
				ipcTxIteator->second.stopTime > OutOwnerRecord[ipcTxIteator->first].startTime)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-uniqueAuthor-timecheck-error");

		}
		else  //非所有权的再授权时，产生的找零为排他性授权找零
		{
			//非所有权的再授权时，产生的找零为排他性授权找零
			if (InAuthorRecord.count(ipcTxIteator->first) == 0 || InAuthorRecord[ipcTxIteator->first].second.uniqueAuthorize != 1)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-uniqueAuthor-no-ownership-mother");
			//时间需保持一致 （找零）
			if (ipcTxIteator->second.startTime != InAuthorRecord[ipcTxIteator->first].second.startTime || 
				ipcTxIteator->second.stopTime != InAuthorRecord[ipcTxIteator->first].second.stopTime)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-uniqueAuthor-time-error");


		}
		
		ipcTxIteator++;
		continue;
	}

	//普通授权输出
	ipcOutNormalAuthorIteator = OutAuthorRecord.begin();
	while (ipcOutNormalAuthorIteator != OutAuthorRecord.end())
	{
		//对每个授权输出，起止日期不能超过输入的日期
		if (InOwnerRecord.count(ipcOutNormalAuthorIteator->first) > 0) //输入是所有权类型
		{
			if (ipcOutNormalAuthorIteator->second.second.startTime < InOwnerRecord[ipcOutNormalAuthorIteator->first].startTime ||
				((ipcOutNormalAuthorIteator->second.second.stopTime > InOwnerRecord[ipcOutNormalAuthorIteator->first].stopTime) && (InOwnerRecord[ipcOutNormalAuthorIteator->first].stopTime != 0)))
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-normalAuthor-timecheck-error");
		}
		else if (InAuthorRecord.count(ipcOutNormalAuthorIteator->first) > 0) //授权输入
		{
			//时间检查

			if (ipcOutNormalAuthorIteator->second.second.startTime < InAuthorRecord[ipcOutNormalAuthorIteator->first].second.startTime ||
				ipcOutNormalAuthorIteator->second.second.stopTime > InAuthorRecord[ipcOutNormalAuthorIteator->first].second.stopTime)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-reAuthor-timecheck-error");

		}
		else
			return state.DoS(100, false, REJECT_INVALID, "bad-IPC-Author-no-mother"); //输入中没有所有权也没有授权 报错

		ipcOutNormalAuthorIteator++;
		continue;
	}

	return true;
}

//代币交易模型类型约束检查子函数
bool IsValidTokenModelCheck(std::map<std::string, uint64_t>& tokenInRegRecord, std::map<std::string, uint64_t>& tokenInRecord,
	std::map<std::string, uint64_t>& tokenOutRegRecord, std::map<std::string, uint64_t>& tokenOutRecord, CValidationState& state)
{
	std::map<std::string, uint64_t>::iterator tokenTxIteator;

	tokenTxIteator = tokenInRegRecord.begin();
	while (tokenTxIteator != tokenInRegRecord.end())
	{
		//联合检查：对同一个Symbol，输入中不应当同时出现登记类型和交易类型
		if (tokenInRecord.count(tokenTxIteator->first) > 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-Multi-inType");

		//对一个登记类型，输出中必定有对应Symbol的交易类型输出
		if (tokenOutRecord.count(tokenTxIteator->first) <= 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-reg-to-none");

		tokenTxIteator++;
		continue;
	}

	tokenTxIteator = tokenOutRegRecord.begin();
	while (tokenTxIteator != tokenOutRegRecord.end())
	{
		//联合检查：对同一个Symbol，输出中不应当同时出现登记类型和交易类型
		if (tokenOutRecord.count(tokenTxIteator->first) > 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-Multi-outType");
		
		////对一个登记交易而言，其输入中不可能有同样Symbol的登记交易：从逻辑上讲，4的输入必须有5的输出，而且不能同时存在4和5的输出，本条限制已经被包括在校验中了。所以不加也可以，少一次搜索
		//if (tokenInRegRecord.count(tokenTxIteator->first))
		//	return false;
		
		tokenTxIteator++;
		continue;
	}

	//对输出是token代币类型的做类型约束检查
	tokenTxIteator = tokenOutRecord.begin();
	while (tokenTxIteator != tokenOutRecord.end())
	{
		//如果输入中有对应Symbol的登记记录，证明这是一笔发行交易
		if (tokenInRegRecord.count(tokenTxIteator->first) > 0)
		{
			//验证输入中对应交易的总金额与输出中对应交易的总金额相等，否则报错
			if (tokenTxIteator->second != tokenInRegRecord[tokenTxIteator->first])
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-regtotoken-value-unequal");
			tokenTxIteator++;
			continue;
		}
	//	std::cout << "symbol ="<< tokenTxIteator->first<< std::endl;
		//否则，如果输入中有对应Symbol的交易记录，证明这是一笔代币流通交易
		if (tokenInRecord.count(tokenTxIteator->first) > 0)
		{
			//验证输入中对应交易的总金额与输出中对应交易的总金额相等，否则报错
			if (tokenTxIteator->second != tokenInRecord[tokenTxIteator->first])
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-value-unequal");
			tokenTxIteator++;
			continue;
		}
		
		//全部检测均失败，报错
	//	std::cout << tokenTxIteator->second <<" == "<<tokenInRecord[tokenTxIteator->first] << std::endl;
		return state.DoS(100, false, REJECT_INVALID, "bad-Token-output-error");
	}
	return true;
}

bool AreIPCStandard(const CTransaction& tx, CValidationState &state)
{
	//IPC的Coinbase肯定要检查，因为会有逻辑了，但是Coinbase交易一定不是混合交易，目前我们不做处理
	if (tx.IsCoinBase())
		return true; // Coinbases don't use vin normally

	if (!CheckIPCFinalTx(tx))
		return state.DoS(100, false, REJECT_INVALID, "is-no-IPC-final-tx");



	//如果交易不是竞选类型交易，则禁止其输出到系统地址
	//CTxDestination dest = CBitcoinAddress(system_account_address).Get();
	//CScript systemPubkey = GetScriptForDestination(dest);
	txnouttype type;
	std::vector<CTxDestination> txoutdestes;
	int nRequired;

	BOOST_FOREACH(const CTxOut& txout, tx.vout) {
		//解析txout的输出地址
		if (!ExtractDestinations(txout.scriptPubKey, type, txoutdestes, nRequired))
			return state.DoS(100, false, REJECT_INVALID, "txout-address-unextracted,type=" + type);

		if (txout.txType != TXOUT_CAMPAIGN){
			BOOST_FOREACH(CTxDestination &dest, txoutdestes){
				if (Params().system_account_address == CBitcoinAddress(dest).ToString())
					return state.DoS(100, false, REJECT_INVALID, "send-to-system-address-forbidden");
			}
		}
	}

	uint64_t tokenTxInputTotalValue = 0;
	std::map<std::string, uint64_t> tokenInRegRecord; //以Symbol为关键字，记录输入中的token登记类型，与金额值
	std::map<std::string, uint64_t> tokenInRecord; //以Symbol为关键字，记录输入中的token代币值交易类型。同Symbol的多笔代币输入应被累加到一起

	std::map<uint128, IPCLabel> ipcInOwnerRecord; //以ipc hash标签为关键字，记录输入中的所有权类型，与其标签
	std::map<uint128, std::pair<CScript, IPCLabel> > ipcInAuthorRecord; //以ipc hash标签为关键字，记录输入中的授权类型，与其标签

	CTransactionRef prevTx;
	uint256 hashBlock;
	//CScript scriptSystem = GetScriptForDestination(CBitcoinAddress(system_account_address).Get());

	int devoteinCount = 0;
	int IPCinCount = 0;
	int tokeninCount = 0;
	CAmount totalvintvalues = 0;
	for (unsigned int i = 0; i < tx.vin.size(); i++)
	{
		if (!GetTransaction(tx.vin[i].prevout.hash, prevTx, Params().GetConsensus(), hashBlock, true))
		{
			//std::cout << "[AreIPCStandard] :[GetTransaction]:false" << std::endl;
			if (!GetCachedChainTransaction(tx.vin[i].prevout.hash, prevTx))
			{
				//std::cout << "[AreIPCStandard] :[GetCachedChainTransaction]:false" << std::endl;
				return state.DoS(100, false, REJECT_INVALID, "bad-no-input");
			}
				
		}
		const CTxOut& prev = prevTx->vout[tx.vin[i].prevout.n];
		
		totalvintvalues += prev.nValue;
		//如果前置输出（无论什么类型）是系统监管账户，则报错。不允许从监管账户花出
		if (!ExtractDestinations(prev.scriptPubKey, type, txoutdestes, nRequired))
			return state.DoS(100, false, REJECT_INVALID, "txin-address-unextracted,type=" + type);

		BOOST_FOREACH(CTxDestination &dest, txoutdestes){
			if (Params().system_account_address == CBitcoinAddress(dest).ToString())
				return state.DoS(100, false, REJECT_INVALID, "cost-from-systemaccount-forbidden");
		}
		//CScript scriptSystem = GetScriptForDestination(CBitcoinAddress(system_account_address).Get());
		//if (prev.scriptPubKey == scriptSystem)
		//	return state.DoS(100, false, REJECT_INVALID, "cost-from-systemaccount-forbidden");

		switch (prev.txType)
		{
		case 1:	
			if (prev.devoteLabel.ExtendType != TYPE_CONSENSUS_REGISTER)
			{
				return state.DoS(100, false, REJECT_INVALID, "bad-campaign-input");
			}
			if (!CConsensusAccountPool::Instance().IsAviableUTXO(prevTx->GetHash()))    //是申请加入的utxo(押金)之后，再判断这个txid是否解冻
			{
				//return state.DoS(100, false, REJECT_INVALID, "bad-campaign-input,UTXO-is-unusable");
				LogPrintf("txhash :%s  , vin[%d] ---bad-campaign-input,UTXO-is-unusable.\n",tx.GetHash().ToString(),i);
				return false;
			}
			devoteinCount++;
			break;

		case 2:
			//ipcLabel长度不大于255
			if (prev.ipcLabel.size()> 255)
				return state.DoS(100, false, REJECT_INVALID, "Vin2-IPCLabel-length-out-of-bounds");
			//输入的所有权起始时间不等于0时，不能晚于当前区块高度对应的时戳
			if (prev.ipcLabel.startTime != 0 && prev.ipcLabel.startTime > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "IPC-owner-starttime-is-up-yet");
			//如果同一笔交易中有两个以上来源于同一HASH的所有权输入，则报错
			if (ipcInOwnerRecord.count(prev.ipcLabel.hash) > 0)
				return state.DoS(100, false, REJECT_INVALID, "multi-IPC-ownership-in-with-same-hash");

			ipcInOwnerRecord[prev.ipcLabel.hash] = prev.ipcLabel; 
			IPCinCount++;
			break;

		case 3:
			//ipcLabel长度不大于255
			if (prev.ipcLabel.size() > 255)
				return state.DoS(100, false, REJECT_INVALID, "Vin3-IPCLabel-length-out-of-bounds");
			//输入的授权权起始时间不等于0时，不能晚于当前区块高度对应的时戳
			if (prev.ipcLabel.startTime != 0 && prev.ipcLabel.startTime > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "IPC-Author-starttime-is-up-yet");
			//如果同一笔交易中有两个以上来源于同一HASH的授权输入，则报错
			if (ipcInAuthorRecord.count(prev.ipcLabel.hash) > 0)
				return state.DoS(100, false, REJECT_INVALID, "multi-IPC-authorization-in-with-same-hash");

			ipcInAuthorRecord[prev.ipcLabel.hash] = std::make_pair(prev.scriptPubKey, prev.ipcLabel);
			IPCinCount++;
			break;

		case 4:
			//如果输入中已经有一笔Symbol相同的登记来源，拒绝（这种情况不太可能出现）
			if (tokenInRegRecord.count(prev.tokenRegLabel.getTokenSymbol()) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Symbol-repeat");
			//对于从4到5的代币流通交易来说，代币的发行时间需要小于当前区块高度对应的时间
			if (prev.tokenRegLabel.issueDate != 0 && prev.tokenRegLabel.issueDate > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "Token-reg-starttime-is-up-yet");
			tokenInRegRecord[prev.tokenRegLabel.getTokenSymbol()] = prev.tokenRegLabel.totalCount;
			tokeninCount++;
			break;

		case 5:
			//对每一笔Symbol相同的输入，金额累加
			if (tokenInRecord.count(prev.tokenLabel.getTokenSymbol()) > 0)
			{
				tokenTxInputTotalValue = tokenInRecord[prev.tokenLabel.getTokenSymbol()];
				tokenTxInputTotalValue += prev.tokenLabel.value;
				tokenInRecord[prev.tokenLabel.getTokenSymbol()] = tokenTxInputTotalValue;
			}
			else
			{
// 				std::cout <<"push--symbol:" << prev.tokenLabel.getTokenSymbol() << std::endl;
// 				std::cout << "push--tokenvalue: " << prev.tokenLabel.value << std::endl;
				tokenInRecord[prev.tokenLabel.getTokenSymbol()] = prev.tokenLabel.value;
			}
			

			tokeninCount++;
			break;			
				
		default:
			continue;
			break;
		}
	}

	//联合约束。同一笔交易里不能同时有除了普通交易之外的其它任意两种模型输入存在
	if ( (IPCinCount > 0 && tokeninCount > 0) /*||	
		(devoteinCount > 0 && tokeninCount > 0) ||
		(IPCinCount > 0 && devoteinCount > 0)*/)	//申请加入记账的押金 现在改为1类型的了所以约束中两个条件去掉
		return state.DoS(100, false, REJECT_INVALID, "multi-txType-input-forbidden");

	std::string checkStr;

	uint64_t tokenTxOutputTotalValue = 0;
	std::map<std::string, uint64_t> tokenOutRegRecord; //以Symbol为关键字，记录输出中的token代币登记交易类型
	std::map<std::string, uint64_t> tokenOutRecord; //以Symbol为关键字，记录输出中的token代币值交易类型，同一个Symbol的多笔代币输出被累加到一起

	std::map<uint128, IPCLabel> ipcOutOwnerRecord; //以ipc hash标签为关键字，记录输出中的所有权类型，与其标签
	std::map<uint128, IPCLabel> ipcOutUniqueRecord; //记录排他性授权的MAP
	std::multimap<uint128, std::pair<CScript,IPCLabel>> ipcOutAuthorRecord; //允许有多个授权输出，对每个授权输出都需要分别检测许多不同的情况和值，因此需要带上输出公钥地址来区分多个授权输出

	CBitcoinAddress address(Params().system_account_address);
	CScript scriptSystem = GetScriptForDestination(address.Get());
	CScript tmpscript;
	uint160 devoterhash;

	//CTransactionRef prevTx;
	//uint256 hashBlock;
	//if (!GetTransaction(tx.vin[0].prevout.hash, prevTx, Params().GetConsensus(), hashBlock, true))
	//	return false; //没有输入对应的前置输出
	CTxOut prev;

	int IPCoutCount = 0;
	int devoteoutCount = 0;
	int tokenoutCount = 0;
	
	std::vector<CTxDestination> prevdestes;
	std::string curaddress;
	CAmount totalvoutvalues = 0;
	BOOST_FOREACH(const CTxOut& txout, tx.vout) {
		//判断输出中的txLabelLen长度不能大于  uint16_t
		if (txout.txLabel.size() > TXLABLE_MAX_LENGTH)  //大于512后，txLabelLen显示的为不正确的长度
			return state.DoS(100, false, REJECT_INVALID, "Vout-txLabel-length-out-of-bounds");
		totalvoutvalues += txout.nValue;
		bool systenaddressfounded = false;
		switch (txout.txType)
		{
		case 1:
			//不在CoinBase里，类型为1的交易，一定是输出到系统监管账户的。因此如果输出不是指向监管账户，拒绝
// 			if (!ExtractDestinations(txout.scriptPubKey, type, txoutdestes, nRequired))
// 				return state.DoS(100, false, REJECT_INVALID, "txout-address-unextracted,type=" + type);
// 
// 			BOOST_FOREACH(CTxDestination &dest, txoutdestes){
// 				if (Params().system_account_address == CBitcoinAddress(dest).ToString()){
// 					systenaddressfounded = true;
// 					break;
// 				}
// 			}
// 			if (!systenaddressfounded)
// 				return state.DoS(100, false, REJECT_INVALID, "bad-campaign-system-address");

			//申请退出的交易，其输入的发起地址应该只有一个，并且应该与给定的pubkey hash值匹配
			if (txout.devoteLabel.ExtendType == TYPE_CONSENSUS_QUITE)
			{
				if (tx.vin.size() > 1)
					return state.DoS(100, false, REJECT_INVALID, "multi-exit-campaign-inputs");

				//通过vin找到之前的vout的scriptpubkey，顺便验证前一个vout的类型是普通交易输出

				prev = prevTx->vout[tx.vin[0].prevout.n];
				devoterhash = txout.devoteLabel.hash;

				//从当前hash值获取地址
				curaddress = CBitcoinAddress(CKeyID(devoterhash)).ToString();
				//从prevout的脚本恢复CTXDestination
				if (!ExtractDestinations(prev.scriptPubKey, type, prevdestes, nRequired))
					return state.DoS(100, false, REJECT_INVALID, "exit-campaign-prevout-Unextracted,type=" + type);

				std::cout << "\n\nCompare Address: cur=" << curaddress << "\n";
				bool founded = false;
				BOOST_FOREACH(CTxDestination &prevdest, prevdestes){
					std::cout << "prevaddress containes: " << CBitcoinAddress(prevdest).ToString() << "\n";
					if (curaddress == CBitcoinAddress(prevdest).ToString())
					{
						//std::cout << "match address founded\n";
						founded = true;
						break;
					}
				}
				if (!founded)
				{
					std::cout << "Addresses Not Match!\n\n";
					return state.DoS(100, false, REJECT_INVALID, "bad-exit-campaign-devotepubkey-address");
				}

			}
			else if (txout.devoteLabel.ExtendType != TYPE_CONSENSUS_REGISTER && txout.devoteLabel.ExtendType != TYPE_CONSENSUS_SEVERE_PUNISHMENT_REQUEST)
				return state.DoS(100, false, REJECT_INVALID, "construct-other-campaign-tx-forbidden");

			devoteoutCount++;
			break;

		case 2:
			//知产类型输出的ipc值必须为0
			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout2-IPC-nValue-must-be-zero");
			if (txout.ipcLabel.size() > 255)
				return state.DoS(100, false, REJECT_INVALID, "Vout2-IPCLabel-length-out-of-bounds");
			//同一笔交易中同一个hash值的知产输出只能有一个
			if (ipcOutOwnerRecord.count(txout.ipcLabel.hash) > 0)
				return state.DoS(100, false, REJECT_INVALID, "multi-IPC-ownership-output");
			//校验labelLen 和 txLabelLen与数据size()是否一致
			if (txout.ipcLabel.size() != txout.labelLen)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-labelLen-for-ipcLabel");
			if (txout.txLabel.size() != txout.txLabelLen)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-txLabelLen-for-txLabel");
			//所有权转移操作时需要判断前后的ipchash ExtendType labelTitle需要保持一致
			if (ipcInOwnerRecord.count(txout.ipcLabel.hash) > 0) //所有权转移
			{
				if (ipcInOwnerRecord[txout.ipcLabel.hash].hash != txout.ipcLabel.hash ||
					ipcInOwnerRecord[txout.ipcLabel.hash].ExtendType != txout.ipcLabel.ExtendType ||
					ipcInOwnerRecord[txout.ipcLabel.hash].labelTitle != txout.ipcLabel.labelTitle)
					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-send-output(hash/ExtendType/labelTitle)");
			}
			ipcOutOwnerRecord[txout.ipcLabel.hash] = txout.ipcLabel;
			IPCoutCount++;
			break;

		case 3:
			//知产类型输出的ipc值必须为0
			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout3-IPC-nValue-must-be-zero");
			if (txout.ipcLabel.size() > 255)
				return state.DoS(100, false, REJECT_INVALID, "Vout3-IPCLabel-length-out-of-bounds");
			//如果授权输出的起始时间或者结束时间为0，报错
			if (txout.ipcLabel.startTime == 0 || txout.ipcLabel.stopTime == 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-author-time");
			//校验labelLen 和 txLabelLen与数据size()是否一致
			if (txout.ipcLabel.size() != txout.labelLen)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-labelLen-for-ipcLabel");
			if (txout.txLabel.size() != txout.txLabelLen)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-txLabelLen-for-txLabel");
			//授权操作时需要判断前后的ipchash ExtendType labelTitle需要保持一致
			if (ipcInOwnerRecord.count(txout.ipcLabel.hash) > 0) //授权
			{
				if (ipcInOwnerRecord[txout.ipcLabel.hash].hash != txout.ipcLabel.hash ||
					ipcInOwnerRecord[txout.ipcLabel.hash].ExtendType != txout.ipcLabel.ExtendType ||
					ipcInOwnerRecord[txout.ipcLabel.hash].labelTitle != txout.ipcLabel.labelTitle)
					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-Authorize-output(hash/ExtendType/labelTitle)");
			}
			//再授权的这个约束放在IsValidIPCModelCheck
			
			if (txout.ipcLabel.uniqueAuthorize == 1) //独占授权输出和非独占授权输出分开处理
			{
				//同一笔交易中独占授权输出只能有一个
				if (ipcOutUniqueRecord.count(txout.ipcLabel.hash) > 0)
					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-multi-uniqueAuthor-output");

				ipcOutUniqueRecord[txout.ipcLabel.hash] = txout.ipcLabel;
			}
			else
				ipcOutAuthorRecord.insert(std::make_pair(txout.ipcLabel.hash, std::make_pair(txout.scriptPubKey, txout.ipcLabel)));

			IPCoutCount++;
			break;

		case 4:
			//代币类型输出的ipc值必须为0
			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout4-Token-nValue-must-be-zero");
			//检查字段，防止恶意攻击
			checkStr = txout.tokenRegLabel.getTokenSymbol();
			boost::to_upper(checkStr);
			if (checkStr.find("IPC") != std::string::npos || checkStr.find("RMB") != std::string::npos
				|| checkStr.find("USD") != std::string::npos || checkStr.find("EUR") != std::string::npos )
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Symbol-contain-errvalue");
			checkStr = txout.tokenRegLabel.getTokenLabel();
			boost::to_upper(checkStr);
			if (checkStr.find("IPC") != std::string::npos || checkStr.find("RMB") != std::string::npos
				|| checkStr.find("USD") != std::string::npos || checkStr.find("EUR") != std::string::npos)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Label-contain-errvalue");
			//登记时间 不允许小于20171001000000   ---1506787200
			if (txout.tokenRegLabel.issueDate < TOKEN_REGTIME_BOUNDARY)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-issueDate(Regtime)");
			//登记精度
			if (txout.tokenRegLabel.accuracy < 0 || txout.tokenRegLabel.accuracy > 8)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-accuracy(must be 0-8)");
			//登记代币总量 不允许大于100亿  算上精度之后 
			if (txout.tokenRegLabel.totalCount > getTokenAllcoins(txout.tokenRegLabel.accuracy))
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-totalCount(can't larger than Ten billion)");

			//如果历史中（无论是永久还是临时）已经有同样Symbol的Token登记，并且不是本交易的txid，拒绝
			if (pIPCCheckMaps->TokenSymbolMap.count(txout.tokenRegLabel.getTokenSymbol()) > 0 &&
				pIPCCheckMaps->TokenSymbolMap[txout.tokenRegLabel.getTokenSymbol()].first != tx.GetHash())
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokensymbol-repeat");
			//如果历史中已经有同样Hash的Token登记，拒绝
			if (pIPCCheckMaps->TokenHashMap.count(txout.tokenRegLabel.hash) > 0 &&
				pIPCCheckMaps->TokenHashMap[txout.tokenRegLabel.hash].first != tx.GetHash())
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokenhash-repeat");

			//如果本交易中已经有一笔相同Symbol的登记输出，拒绝
			if (tokenOutRegRecord.count(txout.tokenRegLabel.getTokenSymbol()) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokensymbol-repeat");

			tokenOutRegRecord[txout.tokenRegLabel.getTokenSymbol()] = txout.tokenRegLabel.totalCount;

			tokenoutCount++;
			break;

		case 5:
			//代币类型输出的ipc值必须为0
			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout5-Token-nValue-must-be-zero");
			//检查字段，防止恶意攻击
			checkStr = txout.tokenLabel.getTokenSymbol();
			boost::to_upper(checkStr);
			if (checkStr.find("IPC") != std::string::npos)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Symbol-contain-errvalue");

			//对每一笔Symbol相同的输出，金额累加
			if (tokenOutRecord.count(txout.tokenLabel.getTokenSymbol()) > 0)
			{
				tokenTxOutputTotalValue = tokenOutRecord[txout.tokenLabel.getTokenSymbol()];
				tokenTxOutputTotalValue += txout.tokenLabel.value;
				tokenOutRecord[txout.tokenLabel.getTokenSymbol()] = tokenTxOutputTotalValue;
			}
			else
				tokenOutRecord[txout.tokenLabel.getTokenSymbol()] = txout.tokenLabel.value;

			tokenoutCount++;
			break;

		default:
			continue;
			break;
		}
	}

	//联合约束。同一笔交易里不能同时有除了普通交易之外的其它任意两种模型输出存在
	if ( (IPCoutCount > 0 && tokenoutCount > 0) ||
		(IPCoutCount > 0 && devoteoutCount > 0) ||
		(devoteoutCount > 0 && tokenoutCount > 0))
		return state.DoS(100, false, REJECT_INVALID, "multi-txType-output-forbidden");

	if (!IsValidTokenModelCheck(tokenInRegRecord, tokenInRecord, tokenOutRegRecord, tokenOutRecord, state))
		return false;

	if (!IsValidIPCModelCheck(tx, ipcInOwnerRecord, ipcInAuthorRecord, ipcOutOwnerRecord, ipcOutUniqueRecord, ipcOutAuthorRecord, state))
		return false;
	//增加校验交易的费率
	const CTransaction& txver = tx;
	if (!VerifyFee(txver,totalvintvalues-totalvoutvalues))
		return state.DoS(100, false, REJECT_INVALID, "tx-fee-can't-smaller-than-MinFee(0.001/kb)");
	return true;
}

bool IsWitnessStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs)
{
    if (tx.IsCoinBase())
        return true; // Coinbases are skipped

    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        // We don't care if witness for this input is empty, since it must not be bloated.
        // If the script is invalid without witness, it would be caught sooner or later during validation.
        if (tx.vin[i].scriptWitness.IsNull())
            continue;

        const CTxOut &prev = mapInputs.GetOutputFor(tx.vin[i]);

        // get the scriptPubKey corresponding to this input:
        CScript prevScript = prev.scriptPubKey;

        if (prevScript.IsPayToScriptHash()) {
            std::vector <std::vector<unsigned char> > stack;
            // If the scriptPubKey is P2SH, we try to extract the redeemScript casually by converting the scriptSig
            // into a stack. We do not check IsPushOnly nor compare the hash as these will be done later anyway.
            // If the check fails at this stage, we know that this txid must be a bad one.
            if (!EvalScript(stack, tx.vin[i].scriptSig, SCRIPT_VERIFY_NONE, BaseSignatureChecker(), SIGVERSION_BASE))
                return false;
            if (stack.empty())
                return false;
            prevScript = CScript(stack.back().begin(), stack.back().end());
        }

        int witnessversion = 0;
        std::vector<unsigned char> witnessprogram;

        // Non-witness program must not be associated with any witness
        if (!prevScript.IsWitnessProgram(witnessversion, witnessprogram))
            return false;

        // Check P2WSH standard limits
        if (witnessversion == 0 && witnessprogram.size() == 32) {
            if (tx.vin[i].scriptWitness.stack.back().size() > MAX_STANDARD_P2WSH_SCRIPT_SIZE)
                return false;
            size_t sizeWitnessStack = tx.vin[i].scriptWitness.stack.size() - 1;
            if (sizeWitnessStack > MAX_STANDARD_P2WSH_STACK_ITEMS)
                return false;
            for (unsigned int j = 0; j < sizeWitnessStack; j++) {
                if (tx.vin[i].scriptWitness.stack[j].size() > MAX_STANDARD_P2WSH_STACK_ITEM_SIZE)
                    return false;
            }
        }
    }
    return true;
}

CFeeRate incrementalRelayFee = CFeeRate(DEFAULT_INCREMENTAL_RELAY_FEE);
CFeeRate dustRelayFee = CFeeRate(DUST_RELAY_TX_FEE);
unsigned int nBytesPerSigOp = DEFAULT_BYTES_PER_SIGOP;

int64_t GetVirtualTransactionSize(int64_t nWeight, int64_t nSigOpCost)
{
    return (std::max(nWeight, nSigOpCost * nBytesPerSigOp) + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR;
}

int64_t GetVirtualTransactionSize(const CTransaction& tx, int64_t nSigOpCost)
{
    return GetVirtualTransactionSize(GetTransactionWeight(tx), nSigOpCost);
}
