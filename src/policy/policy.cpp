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

//֪������ģ������Լ������Ӻ���
bool IsValidIPCModelCheck(const CTransaction& tx, std::map<uint128, IPCLabel>& InOwnerRecord, std::map<uint128, std::pair<CScript, IPCLabel> >& InAuthorRecord,
	std::map<uint128, IPCLabel>& OutOwnerRecord, std::map<uint128, IPCLabel>& OutUniqueRecord, std::multimap<uint128, std::pair<CScript, IPCLabel> >& OutAuthorRecord, CValidationState& state)
{
	std::map<uint128, IPCLabel>::iterator ipcTxIteator;
	std::map<uint128, std::pair<CScript, IPCLabel> >::iterator ipcInAuthorIteator;
	std::multimap<uint128, std::pair<CScript, IPCLabel> >::iterator ipcOutNormalAuthorIteator;
	
	ipcTxIteator = InOwnerRecord.begin();
	while (ipcTxIteator != InOwnerRecord.end())
	{
		//���ϼ�顣��ÿ������Ȩ�Ǽǵ����룬����б�����һ����ͬhash����ͬ��չtype������Ȩ���������������ڲ��ܳ�������������޶�
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

	//������Ȩ����
	ipcInAuthorIteator = InAuthorRecord.begin();
	while (ipcInAuthorIteator != InAuthorRecord.end())
	{
		//������������Ȩ��־Ϊ0���򱨴�
		if (ipcInAuthorIteator->second.second.reAuthorize == 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-IPC-reAuthor-with-no-permission");

		//����Ȩ����Լ��
		if (InOwnerRecord.count(ipcInAuthorIteator->first) <= 0)
		{
			//��������Ȩ������������ͬһhash������Ȩ���
			if (OutOwnerRecord.count(ipcInAuthorIteator->first) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-reAuthor-with-ownership-output");

			//��������Ȩ������������һ��ͬ��Address��ͬ��Label����Ȩ�������Ψһ�ԣ����ڣ�������Ȩ��ʧ��
			bool losted = true;
			bool changed = true;
			if (ipcInAuthorIteator->second.second.uniqueAuthorize == 0)	//�������ȨΪ����������Ȩ ��ʱ�Ƚϵ���OutAuthorRecord�С���
			{
				ipcOutNormalAuthorIteator = OutAuthorRecord.lower_bound(ipcInAuthorIteator->first);
				if (ipcOutNormalAuthorIteator == OutAuthorRecord.upper_bound(ipcInAuthorIteator->first)) //���˵��û�����hash����Ȩ���
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
			else if (ipcInAuthorIteator->second.second.uniqueAuthorize == 1) //�������ȨΪ��������Ȩ
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

	//��������Ȩ���
	ipcTxIteator = OutOwnerRecord.begin();
	while (ipcTxIteator != OutOwnerRecord.end())
	{
		if (OutUniqueRecord.count(ipcTxIteator->first) == 0 && 
			OutAuthorRecord.count(ipcTxIteator->first) == 0 && 
			InOwnerRecord.count(ipcTxIteator->first) > 0 )
		{
			//modify by xxy  ����Լ���������⣬��ע�� 20171006
			//��������Ȩת�ƽ��ף������starttime����С�ڵ�ǰ�������¿��ʱ��
// 			if (ipcTxIteator->second.startTime < chainActive.Tip()->GetBlockTime())
// 				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-ownership-output-starttime-earlier-than-currentblock");
			//

			//��������Ȩת�ƽ��ף������starttime���ܴ��ڵ�ǰ�������¿��ʱ��
			if (InOwnerRecord[ipcTxIteator->first].startTime > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-ownership-input-starttime-later-than-currentblock");
		}
		else if (OutUniqueRecord.count(ipcTxIteator->first) == 0 &&
			OutAuthorRecord.count(ipcTxIteator->first) == 0 &&
			InOwnerRecord.count(ipcTxIteator->first) == 0)
		{
			//��������Ȩ�Ǽǽ��ף������ظ��Ǽǳ��ֹ�����txid�뱾���ײ�ͬ��hashֵ
			if (pIPCCheckMaps->IPCHashMap.count(ipcTxIteator->first) &&
				pIPCCheckMaps->IPCHashMap[ipcTxIteator->first].first != tx.GetHash())
			{
				std::cout << "ipchash: "<<ipcTxIteator->first.GetHex()<< std::endl;
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-IPChash-repeat");
			}

			//����Ȩ�Ǽǽ����У���ʼʱ�䲻�ܴ��ڽ���ʱ��
			if (ipcTxIteator->second.startTime > ipcTxIteator->second.stopTime && ipcTxIteator->second.stopTime != 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-stoptime-earlier-than-starttime");
			//����Ȩ�Ǽ�ʱ���Ǽǵ�ipchash����Ϊ��
			if (ipcTxIteator->second.hash.IsNull())
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-IPChash-can't-be-NUll");

		}
		//��ÿ������Ȩ���͵�����������������Ȩ��Ǳ���Ϊ1
		if (ipcTxIteator->second.reAuthorize != 1)
			return state.DoS(100, false, REJECT_INVALID, "bad-IPC-ownership-output-no-reAuthor");

		ipcTxIteator++;
		continue;
	}
	//������������Ȩ���
	ipcTxIteator = OutUniqueRecord.begin();
	while (ipcTxIteator != OutUniqueRecord.end())
	{

		if (InOwnerRecord.count(ipcTxIteator->first) > 0) //����Ȩ��Ȩ��������Ȩ
		{
			//��ÿ��Ψһ��Ȩ���͵����,��������Ȩ��Ȩ�����ģ���ʽ����в�������ͬһ��hash��������Ȩ�������
			if (OutAuthorRecord.count(ipcTxIteator->first) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-uniqueAuthor-with-otherAuthor-this-tx");

			////��ÿ��Ψһ��Ȩ���͵���������������������Ȩ�й���ǰ��Ч��������Ȩ����
			if (InOwnerRecord[ipcTxIteator->first].uniqueAuthorize != 0)  //0:��ǰû��������Ȩ  1����ǰ��������Ȩ
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-uniqueAuthor-with-otherAuthor-before");

			if (ipcTxIteator->second.startTime < InOwnerRecord[ipcTxIteator->first].startTime ||
				((ipcTxIteator->second.stopTime > InOwnerRecord[ipcTxIteator->first].stopTime) && (InOwnerRecord[ipcTxIteator->first].stopTime != 0)) ||
				ipcTxIteator->second.stopTime > OutOwnerRecord[ipcTxIteator->first].startTime)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-uniqueAuthor-timecheck-error");

		}
		else  //������Ȩ������Ȩʱ������������Ϊ��������Ȩ����
		{
			//������Ȩ������Ȩʱ������������Ϊ��������Ȩ����
			if (InAuthorRecord.count(ipcTxIteator->first) == 0 || InAuthorRecord[ipcTxIteator->first].second.uniqueAuthorize != 1)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-uniqueAuthor-no-ownership-mother");
			//ʱ���豣��һ�� �����㣩
			if (ipcTxIteator->second.startTime != InAuthorRecord[ipcTxIteator->first].second.startTime || 
				ipcTxIteator->second.stopTime != InAuthorRecord[ipcTxIteator->first].second.stopTime)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-uniqueAuthor-time-error");


		}
		
		ipcTxIteator++;
		continue;
	}

	//��ͨ��Ȩ���
	ipcOutNormalAuthorIteator = OutAuthorRecord.begin();
	while (ipcOutNormalAuthorIteator != OutAuthorRecord.end())
	{
		//��ÿ����Ȩ�������ֹ���ڲ��ܳ������������
		if (InOwnerRecord.count(ipcOutNormalAuthorIteator->first) > 0) //����������Ȩ����
		{
			if (ipcOutNormalAuthorIteator->second.second.startTime < InOwnerRecord[ipcOutNormalAuthorIteator->first].startTime ||
				((ipcOutNormalAuthorIteator->second.second.stopTime > InOwnerRecord[ipcOutNormalAuthorIteator->first].stopTime) && (InOwnerRecord[ipcOutNormalAuthorIteator->first].stopTime != 0)))
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-normalAuthor-timecheck-error");
		}
		else if (InAuthorRecord.count(ipcOutNormalAuthorIteator->first) > 0) //��Ȩ����
		{
			//ʱ����

			if (ipcOutNormalAuthorIteator->second.second.startTime < InAuthorRecord[ipcOutNormalAuthorIteator->first].second.startTime ||
				ipcOutNormalAuthorIteator->second.second.stopTime > InAuthorRecord[ipcOutNormalAuthorIteator->first].second.stopTime)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-reAuthor-timecheck-error");

		}
		else
			return state.DoS(100, false, REJECT_INVALID, "bad-IPC-Author-no-mother"); //������û������ȨҲû����Ȩ ����

		ipcOutNormalAuthorIteator++;
		continue;
	}

	return true;
}

//���ҽ���ģ������Լ������Ӻ���
bool IsValidTokenModelCheck(std::map<std::string, uint64_t>& tokenInRegRecord, std::map<std::string, uint64_t>& tokenInRecord,
	std::map<std::string, uint64_t>& tokenOutRegRecord, std::map<std::string, uint64_t>& tokenOutRecord, CValidationState& state)
{
	std::map<std::string, uint64_t>::iterator tokenTxIteator;

	tokenTxIteator = tokenInRegRecord.begin();
	while (tokenTxIteator != tokenInRegRecord.end())
	{
		//���ϼ�飺��ͬһ��Symbol�������в�Ӧ��ͬʱ���ֵǼ����ͺͽ�������
		if (tokenInRecord.count(tokenTxIteator->first) > 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-Multi-inType");

		//��һ���Ǽ����ͣ�����бض��ж�ӦSymbol�Ľ����������
		if (tokenOutRecord.count(tokenTxIteator->first) <= 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-reg-to-none");

		tokenTxIteator++;
		continue;
	}

	tokenTxIteator = tokenOutRegRecord.begin();
	while (tokenTxIteator != tokenOutRegRecord.end())
	{
		//���ϼ�飺��ͬһ��Symbol������в�Ӧ��ͬʱ���ֵǼ����ͺͽ�������
		if (tokenOutRecord.count(tokenTxIteator->first) > 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-Multi-outType");
		
		////��һ���Ǽǽ��׶��ԣ��������в�������ͬ��Symbol�ĵǼǽ��ף����߼��Ͻ���4�����������5����������Ҳ���ͬʱ����4��5����������������Ѿ���������У�����ˡ����Բ���Ҳ���ԣ���һ������
		//if (tokenInRegRecord.count(tokenTxIteator->first))
		//	return false;
		
		tokenTxIteator++;
		continue;
	}

	//�������token�������͵�������Լ�����
	tokenTxIteator = tokenOutRecord.begin();
	while (tokenTxIteator != tokenOutRecord.end())
	{
		//����������ж�ӦSymbol�ĵǼǼ�¼��֤������һ�ʷ��н���
		if (tokenInRegRecord.count(tokenTxIteator->first) > 0)
		{
			//��֤�����ж�Ӧ���׵��ܽ��������ж�Ӧ���׵��ܽ����ȣ����򱨴�
			if (tokenTxIteator->second != tokenInRegRecord[tokenTxIteator->first])
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-regtotoken-value-unequal");
			tokenTxIteator++;
			continue;
		}
	//	std::cout << "symbol ="<< tokenTxIteator->first<< std::endl;
		//��������������ж�ӦSymbol�Ľ��׼�¼��֤������һ�ʴ�����ͨ����
		if (tokenInRecord.count(tokenTxIteator->first) > 0)
		{
			//��֤�����ж�Ӧ���׵��ܽ��������ж�Ӧ���׵��ܽ����ȣ����򱨴�
			if (tokenTxIteator->second != tokenInRecord[tokenTxIteator->first])
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-value-unequal");
			tokenTxIteator++;
			continue;
		}
		
		//ȫ������ʧ�ܣ�����
	//	std::cout << tokenTxIteator->second <<" == "<<tokenInRecord[tokenTxIteator->first] << std::endl;
		return state.DoS(100, false, REJECT_INVALID, "bad-Token-output-error");
	}
	return true;
}

bool AreIPCStandard(const CTransaction& tx, CValidationState &state)
{
	//IPC��Coinbase�϶�Ҫ��飬��Ϊ�����߼��ˣ�����Coinbase����һ�����ǻ�Ͻ��ף�Ŀǰ���ǲ�������
	if (tx.IsCoinBase())
		return true; // Coinbases don't use vin normally

	if (!CheckIPCFinalTx(tx))
		return state.DoS(100, false, REJECT_INVALID, "is-no-IPC-final-tx");



	//������ײ��Ǿ�ѡ���ͽ��ף����ֹ�������ϵͳ��ַ
	//CTxDestination dest = CBitcoinAddress(system_account_address).Get();
	//CScript systemPubkey = GetScriptForDestination(dest);
	txnouttype type;
	std::vector<CTxDestination> txoutdestes;
	int nRequired;

	BOOST_FOREACH(const CTxOut& txout, tx.vout) {
		//����txout�������ַ
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
	std::map<std::string, uint64_t> tokenInRegRecord; //��SymbolΪ�ؼ��֣���¼�����е�token�Ǽ����ͣ�����ֵ
	std::map<std::string, uint64_t> tokenInRecord; //��SymbolΪ�ؼ��֣���¼�����е�token����ֵ�������͡�ͬSymbol�Ķ�ʴ�������Ӧ���ۼӵ�һ��

	std::map<uint128, IPCLabel> ipcInOwnerRecord; //��ipc hash��ǩΪ�ؼ��֣���¼�����е�����Ȩ���ͣ������ǩ
	std::map<uint128, std::pair<CScript, IPCLabel> > ipcInAuthorRecord; //��ipc hash��ǩΪ�ؼ��֣���¼�����е���Ȩ���ͣ������ǩ

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
		//���ǰ�����������ʲô���ͣ���ϵͳ����˻����򱨴�������Ӽ���˻�����
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
			if (!CConsensusAccountPool::Instance().IsAviableUTXO(prevTx->GetHash()))    //����������utxo(Ѻ��)֮�����ж����txid�Ƿ�ⶳ
			{
				//return state.DoS(100, false, REJECT_INVALID, "bad-campaign-input,UTXO-is-unusable");
				LogPrintf("txhash :%s  , vin[%d] ---bad-campaign-input,UTXO-is-unusable.\n",tx.GetHash().ToString(),i);
				return false;
			}
			devoteinCount++;
			break;

		case 2:
			//ipcLabel���Ȳ�����255
			if (prev.ipcLabel.size()> 255)
				return state.DoS(100, false, REJECT_INVALID, "Vin2-IPCLabel-length-out-of-bounds");
			//���������Ȩ��ʼʱ�䲻����0ʱ���������ڵ�ǰ����߶ȶ�Ӧ��ʱ��
			if (prev.ipcLabel.startTime != 0 && prev.ipcLabel.startTime > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "IPC-owner-starttime-is-up-yet");
			//���ͬһ�ʽ�����������������Դ��ͬһHASH������Ȩ���룬�򱨴�
			if (ipcInOwnerRecord.count(prev.ipcLabel.hash) > 0)
				return state.DoS(100, false, REJECT_INVALID, "multi-IPC-ownership-in-with-same-hash");

			ipcInOwnerRecord[prev.ipcLabel.hash] = prev.ipcLabel; 
			IPCinCount++;
			break;

		case 3:
			//ipcLabel���Ȳ�����255
			if (prev.ipcLabel.size() > 255)
				return state.DoS(100, false, REJECT_INVALID, "Vin3-IPCLabel-length-out-of-bounds");
			//�������ȨȨ��ʼʱ�䲻����0ʱ���������ڵ�ǰ����߶ȶ�Ӧ��ʱ��
			if (prev.ipcLabel.startTime != 0 && prev.ipcLabel.startTime > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "IPC-Author-starttime-is-up-yet");
			//���ͬһ�ʽ�����������������Դ��ͬһHASH����Ȩ���룬�򱨴�
			if (ipcInAuthorRecord.count(prev.ipcLabel.hash) > 0)
				return state.DoS(100, false, REJECT_INVALID, "multi-IPC-authorization-in-with-same-hash");

			ipcInAuthorRecord[prev.ipcLabel.hash] = std::make_pair(prev.scriptPubKey, prev.ipcLabel);
			IPCinCount++;
			break;

		case 4:
			//����������Ѿ���һ��Symbol��ͬ�ĵǼ���Դ���ܾ������������̫���ܳ��֣�
			if (tokenInRegRecord.count(prev.tokenRegLabel.getTokenSymbol()) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Symbol-repeat");
			//���ڴ�4��5�Ĵ�����ͨ������˵�����ҵķ���ʱ����ҪС�ڵ�ǰ����߶ȶ�Ӧ��ʱ��
			if (prev.tokenRegLabel.issueDate != 0 && prev.tokenRegLabel.issueDate > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "Token-reg-starttime-is-up-yet");
			tokenInRegRecord[prev.tokenRegLabel.getTokenSymbol()] = prev.tokenRegLabel.totalCount;
			tokeninCount++;
			break;

		case 5:
			//��ÿһ��Symbol��ͬ�����룬����ۼ�
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

	//����Լ����ͬһ�ʽ����ﲻ��ͬʱ�г�����ͨ����֮���������������ģ���������
	if ( (IPCinCount > 0 && tokeninCount > 0) /*||	
		(devoteinCount > 0 && tokeninCount > 0) ||
		(IPCinCount > 0 && devoteinCount > 0)*/)	//���������˵�Ѻ�� ���ڸ�Ϊ1���͵�������Լ������������ȥ��
		return state.DoS(100, false, REJECT_INVALID, "multi-txType-input-forbidden");

	std::string checkStr;

	uint64_t tokenTxOutputTotalValue = 0;
	std::map<std::string, uint64_t> tokenOutRegRecord; //��SymbolΪ�ؼ��֣���¼����е�token���ҵǼǽ�������
	std::map<std::string, uint64_t> tokenOutRecord; //��SymbolΪ�ؼ��֣���¼����е�token����ֵ�������ͣ�ͬһ��Symbol�Ķ�ʴ���������ۼӵ�һ��

	std::map<uint128, IPCLabel> ipcOutOwnerRecord; //��ipc hash��ǩΪ�ؼ��֣���¼����е�����Ȩ���ͣ������ǩ
	std::map<uint128, IPCLabel> ipcOutUniqueRecord; //��¼��������Ȩ��MAP
	std::multimap<uint128, std::pair<CScript,IPCLabel>> ipcOutAuthorRecord; //�����ж����Ȩ�������ÿ����Ȩ�������Ҫ�ֱ�����಻ͬ�������ֵ�������Ҫ���������Կ��ַ�����ֶ����Ȩ���

	CBitcoinAddress address(Params().system_account_address);
	CScript scriptSystem = GetScriptForDestination(address.Get());
	CScript tmpscript;
	uint160 devoterhash;

	//CTransactionRef prevTx;
	//uint256 hashBlock;
	//if (!GetTransaction(tx.vin[0].prevout.hash, prevTx, Params().GetConsensus(), hashBlock, true))
	//	return false; //û�������Ӧ��ǰ�����
	CTxOut prev;

	int IPCoutCount = 0;
	int devoteoutCount = 0;
	int tokenoutCount = 0;
	
	std::vector<CTxDestination> prevdestes;
	std::string curaddress;
	CAmount totalvoutvalues = 0;
	BOOST_FOREACH(const CTxOut& txout, tx.vout) {
		//�ж�����е�txLabelLen���Ȳ��ܴ���  uint16_t
		if (txout.txLabel.size() > TXLABLE_MAX_LENGTH)  //����512��txLabelLen��ʾ��Ϊ����ȷ�ĳ���
			return state.DoS(100, false, REJECT_INVALID, "Vout-txLabel-length-out-of-bounds");
		totalvoutvalues += txout.nValue;
		bool systenaddressfounded = false;
		switch (txout.txType)
		{
		case 1:
			//����CoinBase�����Ϊ1�Ľ��ף�һ���������ϵͳ����˻��ġ��������������ָ�����˻����ܾ�
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

			//�����˳��Ľ��ף�������ķ����ַӦ��ֻ��һ��������Ӧ���������pubkey hashֵƥ��
			if (txout.devoteLabel.ExtendType == TYPE_CONSENSUS_QUITE)
			{
				if (tx.vin.size() > 1)
					return state.DoS(100, false, REJECT_INVALID, "multi-exit-campaign-inputs");

				//ͨ��vin�ҵ�֮ǰ��vout��scriptpubkey��˳����֤ǰһ��vout����������ͨ�������

				prev = prevTx->vout[tx.vin[0].prevout.n];
				devoterhash = txout.devoteLabel.hash;

				//�ӵ�ǰhashֵ��ȡ��ַ
				curaddress = CBitcoinAddress(CKeyID(devoterhash)).ToString();
				//��prevout�Ľű��ָ�CTXDestination
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
			//֪�����������ipcֵ����Ϊ0
			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout2-IPC-nValue-must-be-zero");
			if (txout.ipcLabel.size() > 255)
				return state.DoS(100, false, REJECT_INVALID, "Vout2-IPCLabel-length-out-of-bounds");
			//ͬһ�ʽ�����ͬһ��hashֵ��֪�����ֻ����һ��
			if (ipcOutOwnerRecord.count(txout.ipcLabel.hash) > 0)
				return state.DoS(100, false, REJECT_INVALID, "multi-IPC-ownership-output");
			//У��labelLen �� txLabelLen������size()�Ƿ�һ��
			if (txout.ipcLabel.size() != txout.labelLen)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-labelLen-for-ipcLabel");
			if (txout.txLabel.size() != txout.txLabelLen)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-txLabelLen-for-txLabel");
			//����Ȩת�Ʋ���ʱ��Ҫ�ж�ǰ���ipchash ExtendType labelTitle��Ҫ����һ��
			if (ipcInOwnerRecord.count(txout.ipcLabel.hash) > 0) //����Ȩת��
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
			//֪�����������ipcֵ����Ϊ0
			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout3-IPC-nValue-must-be-zero");
			if (txout.ipcLabel.size() > 255)
				return state.DoS(100, false, REJECT_INVALID, "Vout3-IPCLabel-length-out-of-bounds");
			//�����Ȩ�������ʼʱ����߽���ʱ��Ϊ0������
			if (txout.ipcLabel.startTime == 0 || txout.ipcLabel.stopTime == 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-author-time");
			//У��labelLen �� txLabelLen������size()�Ƿ�һ��
			if (txout.ipcLabel.size() != txout.labelLen)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-labelLen-for-ipcLabel");
			if (txout.txLabel.size() != txout.txLabelLen)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-txLabelLen-for-txLabel");
			//��Ȩ����ʱ��Ҫ�ж�ǰ���ipchash ExtendType labelTitle��Ҫ����һ��
			if (ipcInOwnerRecord.count(txout.ipcLabel.hash) > 0) //��Ȩ
			{
				if (ipcInOwnerRecord[txout.ipcLabel.hash].hash != txout.ipcLabel.hash ||
					ipcInOwnerRecord[txout.ipcLabel.hash].ExtendType != txout.ipcLabel.ExtendType ||
					ipcInOwnerRecord[txout.ipcLabel.hash].labelTitle != txout.ipcLabel.labelTitle)
					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-Authorize-output(hash/ExtendType/labelTitle)");
			}
			//����Ȩ�����Լ������IsValidIPCModelCheck
			
			if (txout.ipcLabel.uniqueAuthorize == 1) //��ռ��Ȩ����ͷǶ�ռ��Ȩ����ֿ�����
			{
				//ͬһ�ʽ����ж�ռ��Ȩ���ֻ����һ��
				if (ipcOutUniqueRecord.count(txout.ipcLabel.hash) > 0)
					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-multi-uniqueAuthor-output");

				ipcOutUniqueRecord[txout.ipcLabel.hash] = txout.ipcLabel;
			}
			else
				ipcOutAuthorRecord.insert(std::make_pair(txout.ipcLabel.hash, std::make_pair(txout.scriptPubKey, txout.ipcLabel)));

			IPCoutCount++;
			break;

		case 4:
			//�������������ipcֵ����Ϊ0
			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout4-Token-nValue-must-be-zero");
			//����ֶΣ���ֹ���⹥��
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
			//�Ǽ�ʱ�� ������С��20171001000000   ---1506787200
			if (txout.tokenRegLabel.issueDate < TOKEN_REGTIME_BOUNDARY)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-issueDate(Regtime)");
			//�ǼǾ���
			if (txout.tokenRegLabel.accuracy < 0 || txout.tokenRegLabel.accuracy > 8)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-accuracy(must be 0-8)");
			//�ǼǴ������� ���������100��  ���Ͼ���֮�� 
			if (txout.tokenRegLabel.totalCount > getTokenAllcoins(txout.tokenRegLabel.accuracy))
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-totalCount(can't larger than Ten billion)");

			//�����ʷ�У����������û�����ʱ���Ѿ���ͬ��Symbol��Token�Ǽǣ����Ҳ��Ǳ����׵�txid���ܾ�
			if (pIPCCheckMaps->TokenSymbolMap.count(txout.tokenRegLabel.getTokenSymbol()) > 0 &&
				pIPCCheckMaps->TokenSymbolMap[txout.tokenRegLabel.getTokenSymbol()].first != tx.GetHash())
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokensymbol-repeat");
			//�����ʷ���Ѿ���ͬ��Hash��Token�Ǽǣ��ܾ�
			if (pIPCCheckMaps->TokenHashMap.count(txout.tokenRegLabel.hash) > 0 &&
				pIPCCheckMaps->TokenHashMap[txout.tokenRegLabel.hash].first != tx.GetHash())
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokenhash-repeat");

			//������������Ѿ���һ����ͬSymbol�ĵǼ�������ܾ�
			if (tokenOutRegRecord.count(txout.tokenRegLabel.getTokenSymbol()) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokensymbol-repeat");

			tokenOutRegRecord[txout.tokenRegLabel.getTokenSymbol()] = txout.tokenRegLabel.totalCount;

			tokenoutCount++;
			break;

		case 5:
			//�������������ipcֵ����Ϊ0
			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout5-Token-nValue-must-be-zero");
			//����ֶΣ���ֹ���⹥��
			checkStr = txout.tokenLabel.getTokenSymbol();
			boost::to_upper(checkStr);
			if (checkStr.find("IPC") != std::string::npos)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Symbol-contain-errvalue");

			//��ÿһ��Symbol��ͬ�����������ۼ�
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

	//����Լ����ͬһ�ʽ����ﲻ��ͬʱ�г�����ͨ����֮���������������ģ���������
	if ( (IPCoutCount > 0 && tokenoutCount > 0) ||
		(IPCoutCount > 0 && devoteoutCount > 0) ||
		(devoteoutCount > 0 && tokenoutCount > 0))
		return state.DoS(100, false, REJECT_INVALID, "multi-txType-output-forbidden");

	if (!IsValidTokenModelCheck(tokenInRegRecord, tokenInRecord, tokenOutRegRecord, tokenOutRecord, state))
		return false;

	if (!IsValidIPCModelCheck(tx, ipcInOwnerRecord, ipcInAuthorRecord, ipcOutOwnerRecord, ipcOutUniqueRecord, ipcOutAuthorRecord, state))
		return false;
	//����У�齻�׵ķ���
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
