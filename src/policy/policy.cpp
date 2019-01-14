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
#include "../src/validation.h"
#include "txmempool.h"

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

extern CTxMemPool mempool;
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

//Check the subfunctions of the model type constraints of the output trading model
bool IsValidIPCModelCheck(const CTransaction& tx, std::map<uint128, IPCLabel>& InOwnerRecord, std::map<uint128, std::pair<CScript, IPCLabel> >& InAuthorRecord,
	std::map<uint128, IPCLabel>& OutOwnerRecord, std::map<uint128, IPCLabel>& OutUniqueRecord, std::multimap<uint128, std::pair<CScript, IPCLabel> >& OutAuthorRecord, CValidationState& state)
{
	std::map<uint128, IPCLabel>::iterator ipcTxIteator;
	std::map<uint128, std::pair<CScript, IPCLabel> >::iterator ipcInAuthorIteator;
	std::multimap<uint128, std::pair<CScript, IPCLabel> >::iterator ipcOutNormalAuthorIteator;
	
	//For ownership input
	ipcTxIteator = InOwnerRecord.begin();
	while (ipcTxIteator != InOwnerRecord.end())
	{
		//Joint inspection. For each ownership input, the output must have a same hash, with the same extension type of ownership output. The output date cannot exceed the date of the input
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

	//For authorization input
	ipcInAuthorIteator = InAuthorRecord.begin();
	while (ipcInAuthorIteator != InAuthorRecord.end())
	{
		//If the reauthorization mark entered is 0, the error is reported
		if (ipcInAuthorIteator->second.second.reAuthorize == 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-IPC-reAuthor-with-no-permission");
		//Authorize operation constraints
		if (InOwnerRecord.count(ipcInAuthorIteator->first) <= 0)
		{
			//For reauthorization operations, no ownership output of the same hash can be found
			if (OutOwnerRecord.count(ipcInAuthorIteator->first) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-reAuthor-with-ownership-output");
	
			bool losted = true;
		
			if (ipcInAuthorIteator->second.second.uniqueAuthorize == 0)	//the authorization for non-exclusive authorization is compared to OutAuthorRecord.
			{
				ipcOutNormalAuthorIteator = OutAuthorRecord.lower_bound(ipcInAuthorIteator->first);
				if (ipcOutNormalAuthorIteator == OutAuthorRecord.upper_bound(ipcInAuthorIteator->first)) //The equivalent specification does not have this hash authorized output
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
			else if (ipcInAuthorIteator->second.second.uniqueAuthorize == 1) //Input authorization is exclusive authorization
			{
		
// 				if (OutUniqueRecord.count(ipcInAuthorIteator->first) == 0)
// 					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-Authorize(unique)-hash-changed");
// 
// 				losted = false;		
			}

			if (losted)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-Authorization-lost");

		}

		ipcInAuthorIteator++;
	}

	//For the output of ownership
	ipcTxIteator = OutOwnerRecord.begin();
	while (ipcTxIteator != OutOwnerRecord.end())
	{
		if (OutUniqueRecord.count(ipcTxIteator->first) == 0 && 
			OutAuthorRecord.count(ipcTxIteator->first) == 0 && 
			InOwnerRecord.count(ipcTxIteator->first) > 0 )
		{

		}
		else if (OutUniqueRecord.count(ipcTxIteator->first) == 0 &&
			OutAuthorRecord.count(ipcTxIteator->first) == 0 &&
			InOwnerRecord.count(ipcTxIteator->first) == 0)
		{
			//For ownership registration transactions, there is no duplication of registration, and txid has different hash values for this transaction
			if (pIPCCheckMaps->IPCHashMap.count(ipcTxIteator->first) &&
				pIPCCheckMaps->IPCHashMap[ipcTxIteator->first].first != tx.GetHash())
			{
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-IPChash-repeat");
			}
			//For ownership of the ownership, the exclusive authorization mark of its output must be 0, ensuring that the ownership of the registration can be exclusive authorized
			if (ipcTxIteator->second.uniqueAuthorize != 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-ownership-output-uniqueAuthorize-error");

		}
		//For each property type output, the reauthorizing mark for its output must be 1
		if (ipcTxIteator->second.reAuthorize != 1)
			return state.DoS(100, false, REJECT_INVALID, "bad-IPC-ownership-output-no-reAuthor");

		ipcTxIteator++;
		continue;
	}

	//Ordinary authorized output
	ipcOutNormalAuthorIteator = OutAuthorRecord.begin();
	while (ipcOutNormalAuthorIteator != OutAuthorRecord.end())
	{
		//For each authorized output, the start-stop date cannot exceed the entered date
		if (InOwnerRecord.count(ipcOutNormalAuthorIteator->first) > 0) //Input is the type of ownership
		{
			if (ipcOutNormalAuthorIteator->second.second.startTime < InOwnerRecord[ipcOutNormalAuthorIteator->first].startTime ||
				((ipcOutNormalAuthorIteator->second.second.stopTime > InOwnerRecord[ipcOutNormalAuthorIteator->first].stopTime) && (InOwnerRecord[ipcOutNormalAuthorIteator->first].stopTime != 0)))
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-normalAuthor-timecheck-error");
		}
		else if (InAuthorRecord.count(ipcOutNormalAuthorIteator->first) > 0) //Authorized to enter
		{
			//Time to check

			if (ipcOutNormalAuthorIteator->second.second.startTime < InAuthorRecord[ipcOutNormalAuthorIteator->first].second.startTime ||
				ipcOutNormalAuthorIteator->second.second.stopTime > InAuthorRecord[ipcOutNormalAuthorIteator->first].second.stopTime)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-reAuthor-timecheck-error");
			
		}
		else
			return state.DoS(100, false, REJECT_INVALID, "bad-IPC-Author-no-mother"); //There is no ownership or authorization in the input

		ipcOutNormalAuthorIteator++;
		continue;
	}

	return true;
}
bool tokenRecordCheck(std::map<std::string, uint64_t>& tokenRecord, std::string& tokenname,
	uint64_t& amount, CValidationState& state, std::string msg )
{
	if (tokenRecord.size() != 1)
		return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokenRecord-sizeerror");
	std::string tempname = tokenRecord.begin()->first;
	if (tokenname.empty())
		tokenname = tempname;
	else{
		if (tokenname != tempname){
			if (msg == "InReg")return state.DoS(100, false, REJECT_INVALID, "bad-Token-InReg-toomanysymbol");
			else if (msg == "In")return state.DoS(100, false, REJECT_INVALID, "bad-Token-In-toomanysymbol");
			else if (msg == "OutReg")return state.DoS(100, false, REJECT_INVALID, "bad-Token-OutReg-toomanysymbol");
			else if (msg == "Out")return state.DoS(100, false, REJECT_INVALID, "bad-Token-Out-toomanysymbol");
			else return state.DoS(100, false, REJECT_INVALID, "bad-Token-InReg-toomanysymbol");
		}
	}
	if (tokenRecord.begin()->second > TOKEN_MAX_VALUE){
		if (msg == "InReg")return state.DoS(100, false, REJECT_INVALID, "bad-Token-InReg-amountbyeondmax");
		else if (msg == "In")return state.DoS(100, false, REJECT_INVALID, "bad-Token-In-amountbyeondmax");
		else if (msg == "OutReg")return state.DoS(100, false, REJECT_INVALID, "bad-Token-OutReg-amountbyeondmax");
		else if (msg == "Out")return state.DoS(100, false, REJECT_INVALID, "bad-Token-Out-amountbyeondmax");
		else return state.DoS(100, false, REJECT_INVALID, "bad-Token-InReg-amountbyeondmax");
	}
	amount += tokenRecord.begin()->second;
	if (amount > TOKEN_MAX_VALUE)		{
		if (msg == "InReg")return state.DoS(100, false, REJECT_INVALID, "bad-Token-InReg-totalamountbyeond");
		else if (msg == "In")return state.DoS(100, false, REJECT_INVALID, "bad-Token-In-totalamountbyeond");
		else if (msg == "OutReg")return state.DoS(100, false, REJECT_INVALID, "bad-Token-OutReg-totalamountbyeond");
		else if (msg == "Out")return state.DoS(100, false, REJECT_INVALID, "bad-Token-Out-totalamountbyeond");
		else return state.DoS(100, false, REJECT_INVALID, "bad-Token-InReg-totalamountbyeond");
	}
	return true;
}
//Scrip transaction model type constraint checking subfunctions
bool IsValidTokenModelCheckForAdd(std::map<std::string, uint64_t>& tokenInRegRecord, std::map<std::string, uint64_t>& tokenInRecord,
std::map<std::string, uint64_t>& tokenOutRegRecord, std::map<std::string, uint64_t>& tokenOutRecord, 
CValidationState& state, int addtokenmodel)
{
	std::map<std::string, uint64_t>::iterator tokenTxIteator;
	if (tokenInRegRecord.size() > 1 || tokenInRecord.size() > 1 ||
		tokenOutRegRecord.size() > 1 || tokenOutRecord.size() > 1){
		return state.DoS(100, false, REJECT_INVALID, "bad-Token-token-toomany");
	}
	if ((tokenInRegRecord.size() == 0 && tokenInRecord.size() == 0 && tokenOutRegRecord.size() == 0) ||
		(tokenOutRegRecord.size() == 0 && tokenOutRecord.size() == 0)||
		(tokenOutRegRecord.size() == 1 && (tokenOutRecord.size() == 1 || tokenInRecord.size() == 1 || tokenInRegRecord.size() == 1))||
		(tokenOutRegRecord.size() == 0 && tokenOutRecord.size() == 0 && tokenInRecord.size() == 0 && tokenInRegRecord.size() == 0)){
		return state.DoS(100, false, REJECT_INVALID, "bad-Token-amount-error");
	}
	std::string tokenname;
	uint64_t intotalamount = 0;
	uint64_t outtotalamount = 0;
	std::cout << "tokenInRegRecord" << std::endl;
	if (tokenInRegRecord.size() == 1 && !tokenRecordCheck(tokenInRegRecord, tokenname, intotalamount, state, "InReg"))
		return false;
	std::cout << "tokenInRecord" << std::endl;
	if (tokenInRecord.size() == 1 && !tokenRecordCheck(tokenInRecord, tokenname, intotalamount, state,"In"))
		return false;
	std::cout << "tokenOutRegRecord" << std::endl;
	if (tokenOutRegRecord.size() == 1 && !tokenRecordCheck(tokenOutRegRecord, tokenname, outtotalamount, state,"OutReg"))
		return false;
	std::cout << "tokenOutRecord" << std::endl;
	if (tokenOutRecord.size() == 1 && !tokenRecordCheck(tokenOutRecord, tokenname, outtotalamount, state,"Out"))
		return false;
	if (tokenOutRegRecord.size() == 0 && intotalamount != outtotalamount){
		std::cout << "intotalamount:" << intotalamount << " outtotalamount:" << outtotalamount << std::endl;
		return state.DoS(100, false, REJECT_INVALID, "bad-Token-vinvout-amountMismatch");
	}
	/*
	tokenTxIteator = tokenInRegRecord.begin();
	while (tokenTxIteator != tokenInRegRecord.end())
	{
		//Joint check: for the same Symbol, there should be no registration type and type of transaction in the input
	//	if (tokenInRecord.count(tokenTxIteator->first) > 0)
	//		return state.DoS(100, false, REJECT_INVALID, "bad-Token-Multi-inType");

		//For a register type, the output must have a transaction type output corresponding to Symbol
		if (tokenOutRecord.count(tokenTxIteator->first) <= 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-reg-to-none");

		tokenTxIteator++;
		//continue;
	}
	if ((Params().NetworkIDString() == CBaseChainParams::MAIN) ||
		((Params().NetworkIDString() == CBaseChainParams::TESTNET) && ((int)chainActive.Height() > 861900)))
	{
		tokenTxIteator = tokenInRecord.begin();
		while (tokenTxIteator != tokenInRecord.end())
		{
			if (tokenOutRecord.count(tokenTxIteator->first) <= 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-token-to-none");

			tokenTxIteator++;
			//continue;
		}
	}
	tokenTxIteator = tokenOutRegRecord.begin();
	while (tokenTxIteator != tokenOutRegRecord.end())
	{
		//Joint check: for the same Symbol, there should be no registration type and type of transaction in the output
		if (tokenOutRecord.count(tokenTxIteator->first) > 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-Multi-outType");

		tokenTxIteator++;
		//continue;
	}

	//The type of token token type is checked for the output
	tokenTxIteator = tokenOutRecord.begin();
	while (tokenTxIteator != tokenOutRecord.end())
	{
		uint64_t totalin = 0;
		//If there is a registration record for Symbol in the input, this is a distribution transaction
		if (tokenInRegRecord.count(tokenTxIteator->first) > 0)
		{
			totalin += tokenInRegRecord[tokenTxIteator->first];
			//Verify that the total amount of the corresponding transaction in the input is equal to the total amount corresponding to the transaction in the output. Otherwise, the report is wrong
			//if (tokenTxIteator->second != tokenInRegRecord[tokenTxIteator->first])
		   //		return state.DoS(100, false, REJECT_INVALID, "bad-Token-regtotoken-value-unequal");
			//tokenTxIteator++;
			//continue;
		}
		//Otherwise, if there is a transaction record of Symbol in the input, it proves that this is a token currency transaction
		if (tokenInRecord.count(tokenTxIteator->first) > 0)
		{
			totalin += tokenInRecord[tokenTxIteator->first];
			//Verify that the total amount of the corresponding transaction in the input is equal to the total amount corresponding to the transaction in the output. Otherwise, the report is wrong
			//if (tokenTxIteator->second != tokenInRecord[tokenTxIteator->first])
			//	return state.DoS(100, false, REJECT_INVALID, "bad-Token-value-unequal");
			//tokenTxIteator++;
			//continue;
		}
		if (totalin != tokenTxIteator->second)
		{
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-output-error");
		}
		if (tokenOutRegRecord.count(tokenTxIteator->first) > 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-Multi-outType");
	}*/
	return true;
}
//Scrip transaction model type constraint checking subfunctions
bool IsValidTokenModelCheck(std::map<std::string, uint64_t>& tokenInRegRecord, std::map<std::string, uint64_t>& tokenInRecord,
	std::map<std::string, uint64_t>& tokenOutRegRecord, std::map<std::string, uint64_t>& tokenOutRecord, CValidationState& state)
{
	std::map<std::string, uint64_t>::iterator tokenTxIteator;

	tokenTxIteator = tokenInRegRecord.begin();
	while (tokenTxIteator != tokenInRegRecord.end())
	{
		//Joint check: for the same Symbol, there should be no registration type and type of transaction in the input
		if (tokenInRecord.count(tokenTxIteator->first) > 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-Multi-inType");

		//For a register type, the output must have a transaction type output corresponding to Symbol
		if (tokenOutRecord.count(tokenTxIteator->first) <= 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-reg-to-none");

		tokenTxIteator++;
		continue;
	}
	if ((Params().NetworkIDString() == CBaseChainParams::MAIN) ||
		((Params().NetworkIDString() == CBaseChainParams::TESTNET) && ((int)chainActive.Height() > 861900)))
	{
		tokenTxIteator = tokenInRecord.begin();
		while (tokenTxIteator != tokenInRecord.end())
		{
			if (tokenOutRecord.count(tokenTxIteator->first) <= 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-token-to-none");

			tokenTxIteator++;
			continue;
		}
	}

	
	tokenTxIteator = tokenOutRegRecord.begin();
	while (tokenTxIteator != tokenOutRegRecord.end())
	{
		//Joint check: for the same Symbol, there should be no registration type and type of transaction in the output
		if (tokenOutRecord.count(tokenTxIteator->first) > 0)
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-Multi-outType");
		
		tokenTxIteator++;
		continue;
	}

	//The type of token token type is checked for the output
	tokenTxIteator = tokenOutRecord.begin();
	while (tokenTxIteator != tokenOutRecord.end())
	{
		//If there is a registration record for Symbol in the input, this is a distribution transaction
		if (tokenInRegRecord.count(tokenTxIteator->first) > 0)
		{
			//Verify that the total amount of the corresponding transaction in the input is equal to the total amount corresponding to the transaction in the output. Otherwise, the report is wrong
			if (tokenTxIteator->second != tokenInRegRecord[tokenTxIteator->first])
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-regtotoken-value-unequal");
			tokenTxIteator++;
			continue;
		}
	
		//Otherwise, if there is a transaction record of Symbol in the input, it proves that this is a token currency transaction
		if (tokenInRecord.count(tokenTxIteator->first) > 0)
		{
			//Verify that the total amount of the corresponding transaction in the input is equal to the total amount corresponding to the transaction in the output. Otherwise, the report is wrong
			if (tokenTxIteator->second != tokenInRecord[tokenTxIteator->first])
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-value-unequal");
			tokenTxIteator++;
			continue;
		}
		
		return state.DoS(100, false, REJECT_INVALID, "bad-Token-output-error");
	}
	return true;
}
bool addTokenClassCompare(const AddTokenLabel& token1, const AddTokenLabel& token2){
	if (token1.version != token2.version ||
		token1.addmode != token2.addmode ||
		token1.hash != token2.hash ||
		token1.totalCount != token2.totalCount ||
		token1.accuracy != token2.accuracy
		){
		std::cout << "info not same" << std::endl;
		return false;
	}
	for (int i = 0; i < 9;i++)
	{
		if (token1.TokenSymbol[i] != token2.TokenSymbol[i]){
			std::cout << "info not same TokenSymbol" << std::endl;
			return false;
		}
	}
	for (int i = 0; i < 17; i++)
	{
		if (token1.label[i] != token2.label[i]){
			std::cout << "info not same label" << std::endl;
			return false;
		}
	}
	return true;
}
bool manualIssuancStandard(const CTxOut& txout, CValidationState &state,
	uint64_t& currentTotalAmount, std::map<std::string,TokenReg>& tokenDataMaptemp,
	std::string txid, int32_t voutIndex, std::string address)
{
	auto itor = tokenDataMaptemp.find(txout.addTokenLabel.getTokenSymbol());
	std::cout << "manualIssuancStandard" << std::endl;
	if (itor != tokenDataMaptemp.end()){
		std::cout << "manualIssuancStandard in" << std::endl;
		if (itor->second.m_tokentype == 4){
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokenDataMap-repeat");
		}
		else if (itor->second.m_tokentype == TXOUT_ADDTOKEN){
			if (itor->second.m_addTokenLabel.size() > 0){
				AddTokenReg &addTokenLabel0 = itor->second.m_addTokenLabel[0];
				if (!addTokenClassCompare(addTokenLabel0.m_addTokenLabel, txout.addTokenLabel)){
					return state.DoS(100, false, REJECT_INVALID, "bad-Token-addTokenClassCompare");
				}
				if (addTokenLabel0.m_txid == txid &&addTokenLabel0.m_vout == voutIndex){
					return state.DoS(100, false, REJECT_INVALID, "bad-Token-txvout-repeat");
				}
				if (addTokenLabel0.address != address){
					return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokenregaddress-notsame");
				}
				
				std::cout << "m_addTokenLabel in" << std::endl;
				auto temp=itor->second.m_addTokenLabel.begin();
				while (temp != itor->second.m_addTokenLabel.end())
				{
					std::cout << "m_addTokenLabel ++" << temp->m_addTokenLabel.currentCount << std::endl;
					currentTotalAmount += temp->m_addTokenLabel.currentCount;
					temp++;
				}
			}
			else
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokenDataMap-error");
		}
		else
			return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokenDataMap-tokentype");
	}
	std::cout << "manualIssuancStandard end" << std::endl;
	return true;
}

bool AreIPCStandard(const CTransaction& tx, CValidationState &state)
{

	if (tx.IsCoinBase())
		return true; // Coinbases don't use vin normally

// 	if (!CheckIPCFinalTx(tx))
// 		return state.DoS(100, false, REJECT_INVALID, "is-no-IPC-final-tx");

	txnouttype type;
	std::vector<CTxDestination> txoutdestes;
	int nRequired;
	std::vector<std::string> txindestes;
	BOOST_FOREACH(const CTxOut& txout, tx.vout) {
		//Parse the output address of txout
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
	std::map<std::string, uint64_t> tokenInRegRecord; //With Symbol as the keyword, record the token registration type in the input with the amount value
	std::map<std::string, uint64_t> tokenInRecord; //Symbol is the key word to record the token value transaction type in the input. The input of multiple tokens with Symbol should be added together.

	std::map<uint128, IPCLabel> ipcInOwnerRecord; //The ipc hash tag is the keyword, recording the type of ownership in the input, and the label.
	std::map<uint128, std::pair<CScript, IPCLabel> > ipcInAuthorRecord; //With ipc hash tag as the keyword, record the authorization type in the input with its tag.

	CTransactionRef prevTx;
	uint256 hashBlock;


	int devoteinCount = 0;
	int IPCinCount = 0;
	int tokeninCount = 0;
	CAmount totalvintvalues = 0;
	uint8_t fatheruraccy = 10; //The legal value can't be 10
	CCoinsView dummy;
	CCoinsViewCache view(&dummy);


	for (unsigned int i = 0; i < tx.vin.size(); i++)
	{	
		{
			LOCK(mempool.cs);
			CCoinsViewMemPool viewMemPool(pcoinsTip, mempool);
			view.SetBackend(viewMemPool);

		}
		CTxOut prev;
		const CCoins* coins = view.AccessCoins(tx.vin[i].prevout.hash);
		if (coins && coins->IsAvailable(tx.vin[i].prevout.n))
		{
			const CTxOut &prevv = coins->vout[tx.vin[i].prevout.n];
			prev = prevv;
			if (prev.txType == TXOUT_TOKENREG)//Tokens to register
				fatheruraccy = prev.tokenRegLabel.accuracy;
			else if (prev.txType == TXOUT_ADDTOKEN)//Tokens to register
				fatheruraccy = prev.addTokenLabel.accuracy;

			totalvintvalues += prev.nValue;
		}
		else{
			if (!GetTransaction(tx.vin[i].prevout.hash, prevTx, Params().GetConsensus(), hashBlock, true))
			{
				//If the parent trades in the same block
				if (!GetCachedChainTransaction(tx.vin[i].prevout.hash, prevTx))
				{
					return state.DoS(100, false, REJECT_INVALID, "bad-no-input");
				}
			}
			const CTxOut &prevv = prevTx->vout[tx.vin[i].prevout.n];
			prev = prevv;
			if (prev.txType == TXOUT_TOKENREG)//Tokens to register
				fatheruraccy = prev.tokenRegLabel.accuracy;
			else if (prev.txType == TXOUT_ADDTOKEN)//Tokens to register
				fatheruraccy = prev.addTokenLabel.accuracy;

			totalvintvalues += prev.nValue;
		}


	
		//If the front-end output (whatever the type) is the system regulatory account, the report is wrong. You're not allowed to spend it from a regulatory account.
		if (!ExtractDestinations(prev.scriptPubKey, type, txoutdestes, nRequired))
			return state.DoS(100, false, REJECT_INVALID, "txin-address-unextracted,type=" + type);

		BOOST_FOREACH(CTxDestination &dest, txoutdestes){
			if (Params().system_account_address == CBitcoinAddress(dest).ToString())
				return state.DoS(100, false, REJECT_INVALID, "cost-from-systemaccount-forbidden");
			txindestes.push_back(CBitcoinAddress(dest).ToString());
		}
		if (prev.txLabelLen > TXLABLE_MAX_LENGTH - 1)  //More than 511, txLabelLen shows the incorrect length
			return state.DoS(100, false, REJECT_INVALID, "Vin-txLabelLen-erro");
		if (prev.txLabel.size() != prev.txLabelLen)  //txLabelLen,txLabel.size(). Don't agree
			return state.DoS(100, false, REJECT_INVALID, "Vin-txLabel-length-not-feet-txLabelLen");


		switch (prev.txType)
		{
		case 1:	
			if (prev.devoteLabel.ExtendType != TYPE_CONSENSUS_REGISTER)
			{
				return state.DoS(100, false, REJECT_INVALID, "bad-campaign-input");
			}
			if (!GetTransaction(tx.vin[i].prevout.hash, prevTx, Params().GetConsensus(), hashBlock, true))
			{
				//If the parent trades in the same block
				if (!GetCachedChainTransaction(tx.vin[i].prevout.hash, prevTx))
				{
					return state.DoS(100, false, REJECT_INVALID, "bad-no-input");
				}
			}
			if (!CConsensusAccountPool::Instance().IsAviableUTXO(prevTx->GetHash()))    //After the application to join utxo (deposit), then determine whether this txid is defrosted.
			{
				LogPrintf("txhash :%s  , vin[%d] ---bad-campaign-input,UTXO-is-unusable.\n",tx.GetHash().ToString(),i);
				return false;
			}
			devoteinCount++;
			break;

		case 2:
			if (prev.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vin2-IPC-nValue-must-be-zero");
			
			if (prev.labelLen > 255)
				return state.DoS(100, false, REJECT_INVALID, "Vin2-IPCLabel-length-out-of-bounds");
			if (prev.ipcLabel.size() != prev.labelLen)
				return state.DoS(100, false, REJECT_INVALID, "Vin2-IPCLabel-length-not-feet-labelLen");
			
			if (prev.ipcLabel.startTime != 0 && prev.ipcLabel.startTime > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "IPC-owner-starttime-is-up-yet");
			
			if (ipcInOwnerRecord.count(prev.ipcLabel.hash) > 0)
				return state.DoS(100, false, REJECT_INVALID, "multi-IPC-ownership-in-with-same-hash");
			
			if (ipcInAuthorRecord.count(prev.ipcLabel.hash) > 0)
				return state.DoS(100, false, REJECT_INVALID, "multi-IPC-authorization-and-ownership-in-with-same-hash");

			ipcInOwnerRecord[prev.ipcLabel.hash] = prev.ipcLabel; 
			IPCinCount++;
			break;

		case 3:
			
			if (prev.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vin3-IPC-nValue-must-be-zero");
			
			if (prev.labelLen > 255)
				return state.DoS(100, false, REJECT_INVALID, "Vin3-IPCLabel-length-out-of-bounds");
			if (prev.ipcLabel.size() != prev.labelLen)
				return state.DoS(100, false, REJECT_INVALID, "Vin3-IPCLabel-length-not-feet-labelLen");
			
			if (prev.ipcLabel.startTime != 0 && prev.ipcLabel.startTime > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "IPC-Author-starttime-is-up-yet");
			
			if (ipcInOwnerRecord.count(prev.ipcLabel.hash) > 0)
				return state.DoS(100, false, REJECT_INVALID, "multi-IPC-ownership-in-with-same-hash");
			
			if (ipcInAuthorRecord.count(prev.ipcLabel.hash) > 0)
				return state.DoS(100, false, REJECT_INVALID, "multi-IPC-authorization-and-ownership-in-with-same-hash");

			ipcInAuthorRecord[prev.ipcLabel.hash] = std::make_pair(prev.scriptPubKey, prev.ipcLabel);
			IPCinCount++;
			break;

		case 4:
			
			if (prev.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vin4-IPC-nValue-must-be-zero");
			
			if (tokenInRegRecord.count(prev.tokenRegLabel.getTokenSymbol()) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Symbol-repeat");
			
			if (prev.tokenRegLabel.issueDate != 0 && prev.tokenRegLabel.issueDate > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "Token-reg-starttime-is-up-yet");
			
			if (prev.tokenRegLabel.accuracy != tokenDataMap[prev.tokenRegLabel.getTokenSymbol()].getAccuracy() && fatheruraccy != prev.tokenRegLabel.accuracy)
			{
				return state.DoS(100, false, REJECT_INVALID, "Vin-Token-accuracy-error");
			}
				
			tokenInRegRecord[prev.tokenRegLabel.getTokenSymbol()] = prev.tokenRegLabel.totalCount;
			tokeninCount++;
			break;

		case 5:
			
			if (prev.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vin5-IPC-nValue-must-be-zero");
		
			if (prev.tokenLabel.accuracy != tokenDataMap[prev.tokenLabel.getTokenSymbol()].getAccuracy())
				return state.DoS(100, false, REJECT_INVALID, "Vin-Token-accuracy-error");

			if (tokenInRecord.count(prev.tokenLabel.getTokenSymbol()) > 0)
			{
				tokenTxInputTotalValue = tokenInRecord[prev.tokenLabel.getTokenSymbol()];
				tokenTxInputTotalValue += prev.tokenLabel.value;
				tokenInRecord[prev.tokenLabel.getTokenSymbol()] = tokenTxInputTotalValue;
			}
			else
			{
				tokenInRecord[prev.tokenLabel.getTokenSymbol()] = prev.tokenLabel.value;
			}

			tokeninCount++;
			break;			
		case TXOUT_ADDTOKEN:

			if (prev.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vin4-IPC-nValue-must-be-zero");

		//	if (tokenInRegRecord.count(prev.addTokenLabel.getTokenSymbol()) > 0)
		//		return state.DoS(100, false, REJECT_INVALID, "bad-Token-Symbol-repeat");

			if (prev.addTokenLabel.issueDate != 0 && prev.addTokenLabel.issueDate > chainActive.Tip()->GetBlockTime())
				return state.DoS(100, false, REJECT_INVALID, "Token-reg-starttime-is-up-yet");
			if (prev.addTokenLabel.height >= chainActive.Height())
				return state.DoS(100, false, REJECT_INVALID, "Token-reg-height-is-up-yet");

			if (prev.addTokenLabel.accuracy != tokenDataMap[prev.addTokenLabel.getTokenSymbol()].getAccuracy() && fatheruraccy != prev.addTokenLabel.accuracy)
			{
				return state.DoS(100, false, REJECT_INVALID, "Vin-Token-accuracy-error");
			}

			//tokenInRegRecord[prev.addTokenLabel.getTokenSymbol()] = prev.addTokenLabel.currentCount;

			if (tokenInRegRecord.count(prev.addTokenLabel.getTokenSymbol()) > 0)
			{
				uint64_t tokenTxInputTotalValue = tokenInRegRecord[prev.addTokenLabel.getTokenSymbol()];
				tokenTxInputTotalValue += prev.addTokenLabel.currentCount;
				tokenInRegRecord[prev.addTokenLabel.getTokenSymbol()] = tokenTxInputTotalValue;
			}
			else
			{
				tokenInRegRecord[prev.addTokenLabel.getTokenSymbol()] = prev.addTokenLabel.currentCount;
			}


			tokeninCount++;
			break;
		default:
			continue;
			break;
		}
	}

	//Joint constraint. The same transaction cannot have any other two model inputs besides the ordinary transaction
	if ( (IPCinCount > 0 && tokeninCount > 0) )
		return state.DoS(100, false, REJECT_INVALID, "multi-txType-input-forbidden");

	std::string checkStr;
	AddTokenLabel paddTokenLabel;
	uint64_t tokenTxOutputTotalValue = 0;
	std::map<std::string, uint64_t> tokenOutRegRecord; //Symbol is the key word to record the token token transaction type in the output.
	std::map<std::string, uint64_t> tokenOutRecord; //With Symbol as the key word, the token value transaction type of the output is recorded, and the output of multiple tokens of the same Symbol is added together.

	std::map<uint128, IPCLabel> ipcOutOwnerRecord; //The ipc hash tag is the key word to record the type of ownership in the output, and its label.
	std::map<uint128, IPCLabel> ipcOutUniqueRecord; //A MAP that records exclusive authorization.
	std::multimap<uint128, std::pair<CScript,IPCLabel>> ipcOutAuthorRecord; //Multiple authorization outputs are allowed, and many different situations and values are required for each authorization output, so you need to bring an output public key address to distinguish multiple authorized outputs.

	CBitcoinAddress address(Params().system_account_address);
	CScript scriptSystem = GetScriptForDestination(address.Get());
	CScript tmpscript;
	uint160 devoterhash;

	CTxOut prev;

	int IPCoutCount = 0;
	int devoteoutCount = 0;
	int tokenoutCount = 0;
	uint64_t addtotalcount = 0;
	uint64_t verifytotalcount = 0;
	int addtokenmodel = -1;
	std::vector<CTxDestination> prevdestes;
	std::string curaddress;
	CAmount totalvoutvalues = 0;
	for (uint32_t voutIndex = 0; voutIndex < tx.vout.size(); voutIndex++) {
		const CTxOut& txout = tx.vout[voutIndex];
		if (txout.txLabelLen > TXLABLE_MAX_LENGTH - 1)  //More than 511, txLabelLen shows the incorrect length.
			return state.DoS(100, false, REJECT_INVALID, "Vout-txLabelLen-erro");
		if (txout.txLabel.size() != txout.txLabelLen)  
			return state.DoS(100, false, REJECT_INVALID, "Vout-txLabel-length-not-feet-txLabelLen");
		totalvoutvalues += txout.nValue;
	
		switch (txout.txType)
		{
		case 1:

			if (txout.devoteLabel.ExtendType == TYPE_CONSENSUS_QUITE)
			{

				bool founded = false;
				for (unsigned int i = 0; i < tx.vin.size(); i++)
				{
					founded = false;
					if (!GetTransaction(tx.vin[i].prevout.hash, prevTx, Params().GetConsensus(), hashBlock, true))
					{
						if (!GetCachedChainTransaction(tx.vin[i].prevout.hash, prevTx))
						{
							return state.DoS(100, false, REJECT_INVALID, "bad-no-input");
						}
					}
					prev = prevTx->vout[tx.vin[i].prevout.n];
					devoterhash = txout.devoteLabel.hash;

					//Gets the address from the current hash value
					curaddress = CBitcoinAddress(CKeyID(devoterhash)).ToString();
					//Restore CTXDestination from the prevout script
					if (!ExtractDestinations(prev.scriptPubKey, type, prevdestes, nRequired))
						return state.DoS(100, false, REJECT_INVALID, "exit-campaign-prevout-Unextracted,type=" + type);

					BOOST_FOREACH(CTxDestination &prevdest, prevdestes){
						if (curaddress == CBitcoinAddress(prevdest).ToString())
						{
							founded = true;
							break;
						}
					}
					if (!founded)
					{
						return state.DoS(100, false, REJECT_INVALID, "bad-exit-campaign-devotepubkey-address");
					}
				}


			}
			else if (txout.devoteLabel.ExtendType != TYPE_CONSENSUS_REGISTER && txout.devoteLabel.ExtendType != TYPE_CONSENSUS_SEVERE_PUNISHMENT_REQUEST)
				return state.DoS(100, false, REJECT_INVALID, "construct-other-campaign-tx-forbidden");

			devoteoutCount++;
			break;

		case 2:
			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout2-IPC-nValue-must-be-zero");
			if (txout.labelLen > 255)
				return state.DoS(100, false, REJECT_INVALID, "Vout2-IPCLabel-length-out-of-bounds");

			if (txout.ipcLabel.size() != txout.labelLen)
				return state.DoS(100, false, REJECT_INVALID, "Vout2-IPCLabel-length-not-feet-labelLen");

			if (txout.ipcLabel.hash.GetHex().length() != 32)
				return state.DoS(100, false, REJECT_INVALID, "Vout2-IPCHash-length-must-be-32");

			if (ipcOutOwnerRecord.count(txout.ipcLabel.hash) > 0)
				return state.DoS(100, false, REJECT_INVALID, "multi-IPC-ownership-output");

			if (txout.ipcLabel.hash.IsNull())
				return state.DoS(100, false, REJECT_INVALID, "IPC-ownership-hash-can't-be-NULL");
			if (txout.ipcLabel.stopTime != 0 && txout.ipcLabel.startTime >= txout.ipcLabel.stopTime)
				return state.DoS(100, false, REJECT_INVALID, "IPC-ownership-starttime-can't-larger-than-stoptime");

			if ((txout.ipcLabel.reAuthorize != 0 && txout.ipcLabel.reAuthorize != 1) ||
				(txout.ipcLabel.uniqueAuthorize != 0 && txout.ipcLabel.uniqueAuthorize != 1))
				return state.DoS(100, false, REJECT_INVALID, "IPCLabel-reAuthorize-or-uniqueAuthorize-out-of-bounds");

			if (ipcInOwnerRecord.count(txout.ipcLabel.hash) > 0)
			{
				if (ipcInOwnerRecord[txout.ipcLabel.hash].hash != txout.ipcLabel.hash ||
					ipcInOwnerRecord[txout.ipcLabel.hash].ExtendType != txout.ipcLabel.ExtendType ||
					ipcInOwnerRecord[txout.ipcLabel.hash].labelTitle != txout.ipcLabel.labelTitle)
					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-send-or-rethorize-output(hash/ExtendType/labelTitle)");


				if (ipcInOwnerRecord[txout.ipcLabel.hash].reAuthorize != txout.ipcLabel.reAuthorize)
					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-send-or-rethorize-output(reAuthorize)");

			}
			ipcOutOwnerRecord[txout.ipcLabel.hash] = txout.ipcLabel;
			IPCoutCount++;
			break;

		case 3:

			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout3-IPC-nValue-must-be-zero");
			if (txout.labelLen > 255)
				return state.DoS(100, false, REJECT_INVALID, "Vout3-IPCLabel-length-out-of-bounds");

			if (txout.ipcLabel.size() != txout.labelLen)
				return state.DoS(100, false, REJECT_INVALID, "Vout3-IPCLabel-length-not-feet-labelLen");

			if (txout.ipcLabel.hash.GetHex().length() != 32)
				return state.DoS(100, false, REJECT_INVALID, "Vout3-IPCHash-length-must-be-32");

			if (txout.ipcLabel.startTime == 0 || txout.ipcLabel.stopTime == 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-IPC-author-time");


			if ((txout.ipcLabel.reAuthorize != 0 && txout.ipcLabel.reAuthorize != 1) ||
				(txout.ipcLabel.uniqueAuthorize != 0 && txout.ipcLabel.uniqueAuthorize != 1))
				return state.DoS(100, false, REJECT_INVALID, "IPCLabel-reAuthorize-or-uniqueAuthorize-out-of-bounds");

			if (ipcInOwnerRecord.count(txout.ipcLabel.hash) > 0)
			{
				if (ipcInOwnerRecord[txout.ipcLabel.hash].hash != txout.ipcLabel.hash ||
					ipcInOwnerRecord[txout.ipcLabel.hash].ExtendType != txout.ipcLabel.ExtendType ||
					ipcInOwnerRecord[txout.ipcLabel.hash].labelTitle != txout.ipcLabel.labelTitle)
					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-Authorize-output(hash/ExtendType/labelTitle)");
			}


			if (txout.ipcLabel.uniqueAuthorize == 1)
			{
				//moddify by xxy 20171216  Exclusive license notes
				// 				//There can only be one exclusive authorized output in the same transaction
				// 				if (ipcOutUniqueRecord.count(txout.ipcLabel.hash) > 0)
				// 					return state.DoS(100, false, REJECT_INVALID, "bad-IPC-multi-uniqueAuthor-output");
				// 
				// 				ipcOutUniqueRecord[txout.ipcLabel.hash] = txout.ipcLabel;
				//end moddify
			}
			else
				ipcOutAuthorRecord.insert(std::make_pair(txout.ipcLabel.hash, std::make_pair(txout.scriptPubKey, txout.ipcLabel)));

			IPCoutCount++;
			break;

		case 4:

			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout4-Token-nValue-must-be-zero");

			checkStr = txout.tokenRegLabel.getTokenSymbol();
			boost::to_upper(checkStr);
			if (checkStr.find("IPC") != std::string::npos || checkStr.find("RMB") != std::string::npos
				|| checkStr.find("USD") != std::string::npos || checkStr.find("EUR") != std::string::npos)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Symbol-contain-errvalue");
			checkStr = txout.tokenRegLabel.getTokenLabel();
			boost::to_upper(checkStr);
			if (checkStr.find("IPC") != std::string::npos || checkStr.find("RMB") != std::string::npos
				|| checkStr.find("USD") != std::string::npos || checkStr.find("EUR") != std::string::npos)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Label-contain-errvalue");

			if (txout.tokenRegLabel.hash.GetHex().length() != 32)
				return state.DoS(100, false, REJECT_INVALID, "Vout4-Hash-length-must-be-32");

			if (txout.tokenRegLabel.issueDate < TOKEN_REGTIME_BOUNDARY)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-issueDate(Regtime)");

			if (txout.tokenRegLabel.accuracy < 0 || txout.tokenRegLabel.accuracy > 8)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-accuracy(must be 0-8)");

			if (txout.tokenRegLabel.totalCount > TOKEN_MAX_VALUE)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-totalCount");

			if (pIPCCheckMaps->TokenSymbolMap.count(txout.tokenRegLabel.getTokenSymbol()) > 0 &&
				pIPCCheckMaps->TokenSymbolMap[txout.tokenRegLabel.getTokenSymbol()].first != tx.GetHash())
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokensymbol-repeat");

			if (pIPCCheckMaps->TokenHashMap.count(txout.tokenRegLabel.hash) > 0 &&
				pIPCCheckMaps->TokenHashMap[txout.tokenRegLabel.hash].first != tx.GetHash())
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokenhash-repeat");


			if (tokenOutRegRecord.count(txout.tokenRegLabel.getTokenSymbol()) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokensymbol-repeat");

			tokenOutRegRecord[txout.tokenRegLabel.getTokenSymbol()] = txout.tokenRegLabel.totalCount;

			tokenoutCount++;
			break;

		case 5:{

			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout5-Token-nValue-must-be-zero");

			if (txout.tokenLabel.accuracy < 0 || txout.tokenLabel.accuracy > 8)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-accuracy(must be 0-8)");

			if (txout.tokenLabel.accuracy != tokenDataMap[txout.tokenLabel.getTokenSymbol()].getAccuracy() && fatheruraccy != txout.tokenLabel.accuracy)
				return state.DoS(100, false, REJECT_INVALID, "Vout-Token-accuracy-error");

			checkStr = txout.tokenLabel.getTokenSymbol();
			boost::to_upper(checkStr);
			if (checkStr.find("IPC") != std::string::npos)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Symbol-contain-errvalue");

			if ((Params().NetworkIDString() == CBaseChainParams::MAIN) ||
				((Params().NetworkIDString() == CBaseChainParams::TESTNET) && ((int)chainActive.Height() > 788900)))
			{
				if (tokenInRecord.count(txout.tokenLabel.getTokenSymbol()) > 0){
					if (txout.tokenLabel.value > tokenInRecord[txout.tokenLabel.getTokenSymbol()])
						return state.DoS(100, false, REJECT_INVALID, "bad-Token-value-errvalue");
				}
				else if (tokenInRegRecord.count(txout.tokenLabel.getTokenSymbol()) > 0)
				{
					if (txout.tokenLabel.value > tokenInRegRecord[txout.tokenLabel.getTokenSymbol()])
						return state.DoS(100, false, REJECT_INVALID, "bad-Token-value-errvalue");
				}

			}

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
		case TXOUT_ADDTOKEN:

			if (txout.nValue != 0)
				return state.DoS(100, false, REJECT_INVALID, "Vout4-Token-nValue-must-be-zero");

			checkStr = txout.addTokenLabel.getTokenSymbol();
			boost::to_upper(checkStr);
			if (checkStr.find("IPC") != std::string::npos || checkStr.find("RMB") != std::string::npos
				|| checkStr.find("USD") != std::string::npos || checkStr.find("EUR") != std::string::npos)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Symbol-contain-errvalue");
			checkStr = txout.addTokenLabel.getTokenLabel();
			boost::to_upper(checkStr);
			if (checkStr.find("IPC") != std::string::npos || checkStr.find("RMB") != std::string::npos
				|| checkStr.find("USD") != std::string::npos || checkStr.find("EUR") != std::string::npos)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Label-contain-errvalue");

			if (checkStr.empty())
				return state.DoS(100, false, REJECT_INVALID, "Vout4-Hash-Symbol-empty");

			if (txout.addTokenLabel.hash.GetHex().length() != 32)
				return state.DoS(100, false, REJECT_INVALID, "Vout4-Hash-length-must-be-32");

			if (txout.addTokenLabel.issueDate < TOKEN_REGTIME_BOUNDARY)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-issueDate(Regtime)");

			if (txout.addTokenLabel.accuracy < 0 || txout.addTokenLabel.accuracy > 8)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-accuracy(must be 0-8)");

			if (txout.addTokenLabel.totalCount > TOKEN_MAX_VALUE ||
				txout.addTokenLabel.totalCount <= 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-totalCount");
			if (addtotalcount == 0){
				addtotalcount = txout.addTokenLabel.totalCount;
				verifytotalcount = addtotalcount;
			}
			else if (addtotalcount != txout.addTokenLabel.totalCount)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-totalCount");

			if (addtokenmodel == -1){
				addtokenmodel = (int)txout.addTokenLabel.addmode;
			}
			else if (addtokenmodel != (int)txout.addTokenLabel.addmode){
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-addmode");
			}

			if (txout.addTokenLabel.currentCount > TOKEN_MAX_VALUE ||
				txout.addTokenLabel.currentCount <= 0 ||
				txout.addTokenLabel.currentCount > txout.addTokenLabel.totalCount)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-currentCount");
			verifytotalcount -= txout.addTokenLabel.currentCount;

			//int addmode = (int)txout.addTokenLabel.addmode;
			if (addtokenmodel != 1 && addtokenmodel != 0){
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-addmode");
			}
			//if (addmode == 0){
			//} 
			//else if (addmode == 1){
			//}
			//else
			//	return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-addmode");
			bool modetokensymbol = false;
			bool modetokenhash = false;
			if (pIPCCheckMaps->TokenSymbolMap.count(txout.addTokenLabel.getTokenSymbol()) > 0 &&
				pIPCCheckMaps->TokenSymbolMap[txout.addTokenLabel.getTokenSymbol()].first != tx.GetHash()){
				if (addtokenmodel == 0){
					return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokensymbol-repeat");
				}
				else{
					modetokensymbol = true;
				}
			}
			if (pIPCCheckMaps->TokenHashMap.count(txout.addTokenLabel.hash) > 0 &&
				pIPCCheckMaps->TokenHashMap[txout.addTokenLabel.hash].first != tx.GetHash()){
				if (addtokenmodel == 0){
					return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokenhash-repeat");
				}
				else{
					modetokenhash = true;
				}
			}
			if (modetokensymbol != modetokenhash){
				if (modetokensymbol)
					return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokensymbol-error");
				if (modetokenhash)
					return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokenhash-error");
			}

			CScript script = txout.scriptPubKey;
			txnouttype typeRet;
			std::vector<CTxDestination> prevdestes;
			int nRequiredRet;
			std::string address;
			bool fValidAddress = ExtractDestinations(script, typeRet, prevdestes, nRequiredRet);
			if (fValidAddress){
				BOOST_FOREACH(CTxDestination &prevdest, prevdestes){
					CBitcoinAddress add(prevdest);
					address = CBitcoinAddress(prevdest).ToString();
					break;
				}
			}
			if (address.empty())
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-voutaddress-error");
			auto result = find(txindestes.begin(), txindestes.end(), address);
			if (result == txindestes.end()){
				BOOST_FOREACH(auto &txindestesaddress, txindestes)
					std::cout << " txindestes address : " << txindestesaddress << std::endl;
				std::cout << " reg address : " << address << std::endl;
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-tokenregaddress-notmine");
			}

			if (addtokenmodel == 1 && modetokensymbol && modetokenhash){
				uint64_t currentTotalAmount = 0;
				if (!manualIssuancStandard(txout, state, currentTotalAmount, tokenDataMap, tx.GetHash().ToString(), voutIndex, address)){
					//	||!manualIssuancStandard(txout, state, currentTotalAmount, newTokenDataMap, tx.GetHash().ToString(), voutIndex)){
					return false;
				}
				if (txout.addTokenLabel.currentCount > txout.addTokenLabel.totalCount - currentTotalAmount)
					return state.DoS(100, false, REJECT_INVALID, "bad-Token-currentCount-beyond");
			}

			if (!paddTokenLabel.getTokenSymbol().empty()){
				if (1 == addtokenmodel)
					return state.DoS(100, false, REJECT_INVALID, "bad-Token-vouts-number");
				if (!addTokenClassCompare(paddTokenLabel, txout.addTokenLabel)){
					return state.DoS(100, false, REJECT_INVALID, "bad-Token-vouts-Incompatible");
				}
			}
			else{
				paddTokenLabel = txout.addTokenLabel;
			}

			if (addtokenmodel == 1 && tokenOutRegRecord.count(txout.addTokenLabel.getTokenSymbol()) > 0)
				return state.DoS(100, false, REJECT_INVALID, "bad-Token-manualIssuanc-repeat");
			//if (tokenOutRegRecord.count(txout.addTokenLabel.getTokenSymbol()) == 0)
			//	tokenOutRegRecord[txout.addTokenLabel.getTokenSymbol()] = txout.addTokenLabel.totalCount;

			if (tokenOutRegRecord.count(txout.addTokenLabel.getTokenSymbol()) > 0)
			{
				uint64_t value = tokenOutRegRecord[txout.addTokenLabel.getTokenSymbol()];
				value += txout.addTokenLabel.currentCount;
				tokenOutRegRecord[txout.addTokenLabel.getTokenSymbol()] = value;
			}
			else
				tokenOutRegRecord[txout.addTokenLabel.getTokenSymbol()] = txout.addTokenLabel.currentCount;


			tokenoutCount++;
		}
			break;

		case 0:
			continue;
			break;
		default:
			return state.DoS(100, false, REJECT_INVALID, "can't-support-output-Type");
		}
	}
	if (verifytotalcount != 0 && addtokenmodel == 0)
		return state.DoS(100, false, REJECT_INVALID, "bad-Token-Reg-totalCount");
	//Joint constraint. The same transaction cannot have any other two model outputs other than ordinary transactions.
	if ( (IPCoutCount > 0 && tokenoutCount > 0) ||
		(IPCoutCount > 0 && devoteoutCount > 0) ||
		(devoteoutCount > 0 && tokenoutCount > 0))
		return state.DoS(100, false, REJECT_INVALID, "multi-txType-output-forbidden");
	if (addtokenmodel != -1 || (tokenOutRecord.size()>0 && tokenDataMap.count(tokenOutRecord.begin()->first)>0 && tokenDataMap[tokenOutRecord.begin()->first].m_tokentype == TXOUT_ADDTOKEN)){
		if (!IsValidTokenModelCheckForAdd(tokenInRegRecord, tokenInRecord, tokenOutRegRecord, tokenOutRecord, state, addtokenmodel))
			return false;
	} 
	else{
		if (!IsValidTokenModelCheck(tokenInRegRecord, tokenInRecord, tokenOutRegRecord, tokenOutRecord, state))
			return false;
	}
	

	if (!IsValidIPCModelCheck(tx, ipcInOwnerRecord, ipcInAuthorRecord, ipcOutOwnerRecord, ipcOutUniqueRecord, ipcOutAuthorRecord, state))
		return false;
	//Increase the rate of check transactions
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
