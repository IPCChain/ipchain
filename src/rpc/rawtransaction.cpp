// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "chain.h"
#include "coins.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "init.h"
#include "keystore.h"
#include "validation.h"
#include "merkleblock.h"
#include "net.h"
#include "policy/policy.h"
#include "primitives/transaction.h"
#include "rpc/server.h"
#include "script/script.h"
#include "script/script_error.h"
#include "script/sign.h"
#include "script/standard.h"
#include "txmempool.h"
#include "uint256.h"
#include "utilstrencodings.h"
#include "validation.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>

#include <univalue.h>


using namespace std;

void ScriptPubKeyToJSON(const CScript& scriptPubKey, UniValue& out, bool fIncludeHex)
{
    txnouttype type;
    vector<CTxDestination> addresses;
    int nRequired;

    out.push_back(Pair("asm", ScriptToAsmStr(scriptPubKey)));
    if (fIncludeHex)
        out.push_back(Pair("hex", HexStr(scriptPubKey.begin(), scriptPubKey.end())));

    if (!ExtractDestinations(scriptPubKey, type, addresses, nRequired)) {
        out.push_back(Pair("type", GetTxnOutputType(type)));
        return;
   }

    out.push_back(Pair("reqSigs", nRequired));
    out.push_back(Pair("type", GetTxnOutputType(type)));

    UniValue a(UniValue::VARR);
    BOOST_FOREACH(const CTxDestination& addr, addresses)
        a.push_back(CBitcoinAddress(addr).ToString());
    out.push_back(Pair("addresses", a));
}
bool isForIsolation = true;
void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry)
{
    entry.push_back(Pair("txid", tx.GetHash().GetHex()));
    entry.push_back(Pair("hash", tx.GetWitnessHash().GetHex()));
    entry.push_back(Pair("size", (int)::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION)));
    entry.push_back(Pair("vsize", (int)::GetVirtualTransactionSize(tx)));
    entry.push_back(Pair("version", tx.nVersion));
    entry.push_back(Pair("locktime", (int64_t)tx.nLockTime));
	CTransactionRef prevTx;
	uint256 hashblock;
	std::vector<CTxDestination> txoutdestes;
	std::vector<std::string> addresses;
	addresses.clear();
	txnouttype type;
	int nRequired;
    UniValue vin(UniValue::VARR);		
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const CTxIn& txin = tx.vin[i];
        UniValue in(UniValue::VOBJ);
        if (tx.IsCoinBase())
            in.push_back(Pair("coinbase", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
        else {
            in.push_back(Pair("txid", txin.prevout.hash.GetHex()));
            in.push_back(Pair("vout", (int64_t)txin.prevout.n));
			if (fTxIndex && (!isForIsolation || fAddressIndex))
			{
				UniValue add(UniValue::VARR);
				if (!GetTransaction(txin.prevout.hash, prevTx, Params().GetConsensus(), hashblock, true))
				{
					if (!GetCachedChainTransaction(tx.vin[i].prevout.hash, prevTx))
						throw JSONRPCError(RPC_VERIFY_ERROR, "bad-input,Wrongful.");
				}
				const CTxOut& prev = prevTx->vout[txin.prevout.n];
				if (!ExtractDestinations(prev.scriptPubKey, type, txoutdestes, nRequired))
					throw JSONRPCError(RPC_VERIFY_ERROR, "txin-address-unextracted.");

				BOOST_FOREACH(CTxDestination &dest, txoutdestes){
					add.push_back(CBitcoinAddress(dest).ToString());	
				}
				
		
				in.push_back(Pair("addresses",add));
				in.push_back(Pair("type",(int64_t)prev.txType));
				switch ((int64_t)prev.txType)
				{
					case 4:
						{
							  in.push_back(Pair("tokensymbol", prev.tokenRegLabel.getTokenSymbol()));
							  UniValue tt = ValueFromTCoins(prev.tokenRegLabel.totalCount, (int)tokenDataMap[prev.tokenRegLabel.getTokenSymbol()].getAccuracy()).get_str();
							  std::string strvalue = tt.get_str();
							  in.push_back(Pair("tokenvalue", strvalue));
							  in.push_back(Pair("accuracy", prev.tokenRegLabel.accuracy));
						}
						break;
					case 5:
						{
							  in.push_back(Pair("tokensymbol", prev.tokenLabel.getTokenSymbol()));
							  UniValue tt = ValueFromTCoins(prev.tokenLabel.value, (int)tokenDataMap[prev.tokenLabel.getTokenSymbol()].getAccuracy());
							  std::string strvalue = tt.get_str();
							  in.push_back(Pair("tokenvalue", strvalue));
							  in.push_back(Pair("accuracy", prev.tokenLabel.accuracy));
						}
						break;
					case TXOUT_ADDTOKEN:
					{
							in.push_back(Pair("tokensymbol", prev.addTokenLabel.getTokenSymbol()));
							UniValue totalCount = ValueFromTCoins(prev.addTokenLabel.totalCount, (int)(tokenDataMap[prev.addTokenLabel.getTokenSymbol()].getAccuracy()));
							UniValue currentCount = ValueFromTCoins(prev.addTokenLabel.currentCount, (int)(tokenDataMap[prev.addTokenLabel.getTokenSymbol()].getAccuracy()));
							in.push_back(Pair("automode", prev.addTokenLabel.addmode));
							in.push_back(Pair("height", prev.addTokenLabel.height));
							in.push_back(Pair("totalCount", totalCount.get_str()));
							in.push_back(Pair("currentCount", currentCount.get_str()));
					}
						break;
					default:
						break;
				}
				in.push_back(Pair("nvalue", ValueFromAmount(prev.nValue)));
			}
			//end
            UniValue o(UniValue::VOBJ);
            o.push_back(Pair("asm", ScriptToAsmStr(txin.scriptSig, true)));
            o.push_back(Pair("hex", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
            in.push_back(Pair("scriptSig", o));
        }
        if (tx.HasWitness()) {
                UniValue txinwitness(UniValue::VARR);
                for (unsigned int j = 0; j < tx.vin[i].scriptWitness.stack.size(); j++) {
                    std::vector<unsigned char> item = tx.vin[i].scriptWitness.stack[j];
                    txinwitness.push_back(HexStr(item.begin(), item.end()));
                }
                in.push_back(Pair("txinwitness", txinwitness));
        }
        in.push_back(Pair("sequence", (int64_t)txin.nSequence));
        vin.push_back(in);
    }
    entry.push_back(Pair("vin", vin));
    UniValue vout(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& txout = tx.vout[i];
        UniValue out(UniValue::VOBJ);
      out.push_back(Pair("value", ValueFromAmount(txout.nValue)));
        out.push_back(Pair("n", (int64_t)i));
		out.push_back(Pair("type", (int64_t)txout.txType));

		switch (txout.txType)
		{
		case 1:
			out.push_back(Pair("devotetype", txout.devoteLabel.ExtendType));
			out.push_back(Pair("devotepubkeyhash160", txout.devoteLabel.hash.GetHex()));
			out.push_back(Pair("txLabelLen", txout.txLabelLen));
			out.push_back(Pair("txLabel", txout.txLabel));
			break;

		case 4:
			{
				  out.push_back(Pair("TokenSymbol", txout.tokenRegLabel.getTokenSymbol()));
				  out.push_back(Pair("TokenValue", uint64_t(0)/*txout.tokenRegLabel.value*/));
				  out.push_back(Pair("TokenHash", txout.tokenRegLabel.hash.GetHex()));
				  out.push_back(Pair("TokenLabel", txout.tokenRegLabel.getTokenLabel()));
				  out.push_back(Pair("TokenIssue", txout.tokenRegLabel.issueDate));
				  UniValue tt;
				  if (isForIsolation)
				  {
					  tt = ValueFromTCoins(txout.tokenRegLabel.totalCount, txout.tokenRegLabel.accuracy);
				  }
				  else
				  {
					  tt = ValueFromTCoins(txout.tokenRegLabel.totalCount, (int)(tokenDataMap[txout.tokenRegLabel.getTokenSymbol()].getAccuracy()));
				  }
				  std::string strvalue = tt.get_str();
				  out.push_back(Pair("TokenTotalCount", strvalue));
				  out.push_back(Pair("accuracy", txout.tokenRegLabel.accuracy));
				  out.push_back(Pair("txLabelLen", txout.txLabelLen));
				  out.push_back(Pair("txLabel", txout.txLabel));
			}
			
			break;

		case 5:
			{
				  out.push_back(Pair("TokenSymbol", txout.tokenLabel.getTokenSymbol()));
				  UniValue tt;
				  if (isForIsolation)
				  {
					  tt = ValueFromTCoins(txout.tokenLabel.value, txout.tokenLabel.accuracy);
				  }
				  else
				  {
					  tt = ValueFromTCoins(txout.tokenLabel.value, (int)tokenDataMap[txout.tokenLabel.getTokenSymbol()].getAccuracy());
				  }
 				  std::string strvalue = tt.get_str();
				  out.push_back(Pair("TokenValue", strvalue));
				  out.push_back(Pair("accuracy", txout.tokenLabel.accuracy));
				  out.push_back(Pair("txLabelLen", txout.txLabelLen));
				  out.push_back(Pair("txLabel", txout.txLabel));
			}
			
			break;

		case 2:
		case 3:
			out.push_back(Pair("extype", txout.ipcLabel.ExtendType));
			out.push_back(Pair("starttime", txout.ipcLabel.startTime));
			out.push_back(Pair("stoptime", txout.ipcLabel.stopTime));
			out.push_back(Pair("reauthorize", txout.ipcLabel.reAuthorize));
			out.push_back(Pair("uniqueauthorize", txout.ipcLabel.uniqueAuthorize));
			out.push_back(Pair("hashLen", txout.ipcLabel.hashLen));
			out.push_back(Pair("IPChash", txout.ipcLabel.hash.GetHex()));
			out.push_back(Pair("IPCTitle", txout.ipcLabel.labelTitle));
			out.push_back(Pair("txLabelLen", txout.txLabelLen));
			out.push_back(Pair("txLabel",txout.txLabel)); 
			break;
		case TXOUT_ADDTOKEN:
		{
				  out.push_back(Pair("TokenSymbol", txout.addTokenLabel.getTokenSymbol()));
				  out.push_back(Pair("TokenValue", uint64_t(0)/*txout.tokenRegLabel.value*/));
				  out.push_back(Pair("TokenHash", txout.addTokenLabel.hash.GetHex()));
				  out.push_back(Pair("TokenLabel", txout.addTokenLabel.getTokenLabel()));
				  out.push_back(Pair("TokenIssue", txout.addTokenLabel.issueDate));
				  UniValue TokenTotalCount;
				  if (isForIsolation)
				  {
					  TokenTotalCount = ValueFromTCoins(txout.addTokenLabel.totalCount, txout.addTokenLabel.accuracy);
				  }
				  else
				  {
					  TokenTotalCount = ValueFromTCoins(txout.addTokenLabel.totalCount, (int)tokenDataMap[txout.addTokenLabel.getTokenSymbol()].getAccuracy()).get_str();
				  }
				  UniValue TokenCurrentCount;
				  if (isForIsolation)
				  {
					  TokenCurrentCount = ValueFromTCoins(txout.addTokenLabel.currentCount, txout.addTokenLabel.accuracy);
				  }
				  else
				  {
					  TokenCurrentCount = ValueFromTCoins(txout.addTokenLabel.currentCount, (int)tokenDataMap[txout.addTokenLabel.getTokenSymbol()].getAccuracy()).get_str();
				  }
				  out.push_back(Pair("automode", txout.addTokenLabel.addmode));
				  out.push_back(Pair("height", txout.addTokenLabel.height));
				  out.push_back(Pair("TokenTotalCount", TokenTotalCount.get_str()));
				  out.push_back(Pair("TokenCurrentCount", TokenCurrentCount.get_str()));

				  out.push_back(Pair("accuracy", txout.addTokenLabel.accuracy));
				  out.push_back(Pair("txLabelLen", txout.txLabelLen));
				  out.push_back(Pair("txLabel", txout.txLabel));

				   

		}

			break;
		default:
		case 0:
			break;
		}

        UniValue o(UniValue::VOBJ);
        ScriptPubKeyToJSON(txout.scriptPubKey, o, true);
        out.push_back(Pair("scriptPubKey", o));
        vout.push_back(out);
    }
    entry.push_back(Pair("vout", vout));

    if (!hashBlock.IsNull()) {
        entry.push_back(Pair("blockhash", hashBlock.GetHex()));
        BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex* pindex = (*mi).second;
            if (chainActive.Contains(pindex)) {
                entry.push_back(Pair("confirmations", 1 + chainActive.Height() - pindex->nHeight));
                entry.push_back(Pair("time", pindex->GetBlockTime()));
                entry.push_back(Pair("blocktime", pindex->GetBlockTime()));
            }
            else
                entry.push_back(Pair("confirmations", 0));
        }
    }
}

UniValue getrawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw runtime_error(
            "getrawtransaction \"txid\" ( verbose )\n"

            "\nNOTE: By default this function only works for mempool transactions. If the -txindex option is\n"
            "enabled, it also works for blockchain transactions.\n"
            "DEPRECATED: for now, it also works for transactions with unspent outputs.\n"

            "\nReturn the raw transaction data.\n"
            "\nIf verbose is 'true', returns an Object with information about 'txid'.\n"
            "If verbose is 'false' or omitted, returns a string that is serialized, hex-encoded data for 'txid'.\n"

            "\nArguments:\n"
            "1. \"txid\"      (string, required) The transaction id\n"
            "2. verbose       (bool, optional, default=false) If false, return a string, otherwise return a json object\n"

            "\nResult (if verbose is not set or set to false):\n"
            "\"data\"      (string) The serialized, hex-encoded data for 'txid'\n"

            "\nResult (if verbose is set to true):\n"
            "{\n"
            "  \"hex\" : \"data\",       (string) The serialized, hex-encoded data for 'txid'\n"
            "  \"txid\" : \"id\",        (string) The transaction id (same as provided)\n"
            "  \"hash\" : \"id\",        (string) The transaction hash (differs from txid for witness transactions)\n"
            "  \"size\" : n,             (numeric) The serialized transaction size\n"
            "  \"vsize\" : n,            (numeric) The virtual transaction size (differs from size for witness transactions)\n"
            "  \"version\" : n,          (numeric) The version\n"
            "  \"locktime\" : ttt,       (numeric) The lock time\n"
            "  \"vin\" : [               (array of json objects)\n"
            "     {\n"
            "       \"txid\": \"id\",    (string) The transaction id\n"
            "       \"vout\": n,         (numeric) \n"
			"       \"type\": n,         (numeric) The txout type \n"
			"       \"nvalue\": x.xxx,   (numeric) The value in " + CURRENCY_UNIT + "\n "
            "       \"scriptSig\": {     (json object) The script\n"
            "         \"asm\": \"asm\",  (string) asm\n"
            "         \"hex\": \"hex\"   (string) hex\n"
            "       },\n"
            "       \"sequence\": n      (numeric) The script sequence number\n"
            "       \"txinwitness\": [\"hex\", ...] (array of string) hex-encoded witness data (if any)\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"vout\" : [              (array of json objects)\n"
            "     {\n"
            "       \"value\" : x.xxx,            (numeric) The value in " + CURRENCY_UNIT + "\n"
            "       \"n\" : n,                    (numeric) index\n"
			"       \"type\": n,                  (numeric) The txout type \n"
			"       .......										\n"
			"       .......										\n"
            "       \"scriptPubKey\" : {          (json object)\n"
            "         \"asm\" : \"asm\",          (string) the asm\n"
            "         \"hex\" : \"hex\",          (string) the hex\n"
            "         \"reqSigs\" : n,            (numeric) The required sigs\n"
            "         \"type\" : \"pubkeyhash\",  (string) The type, eg 'pubkeyhash'\n"
            "         \"addresses\" : [           (json array of string)\n"
            "           \"address\"        (string) ipchain address\n"
            "           ,...\n"
            "         ]\n"
            "       }\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"blockhash\" : \"hash\",   (string) the block hash\n"
            "  \"confirmations\" : n,      (numeric) The confirmations\n"
            "  \"time\" : ttt,             (numeric) The transaction time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"blocktime\" : ttt         (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getrawtransaction", "\"mytxid\"")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" true")
            + HelpExampleRpc("getrawtransaction", "\"mytxid\", true")
        );

    LOCK(cs_main);

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    // Accept either a bool (true) or a num (>=1) to indicate verbose output.
    bool fVerbose = false;
    if (request.params.size() > 1) {
        if (request.params[1].isNum()) {
            if (request.params[1].get_int() != 0) {
                fVerbose = true;
            }
        }
        else if(request.params[1].isBool()) {
            if(request.params[1].isTrue()) {
                fVerbose = true;
            }
        }
        else {
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid type provided. Verbose parameter must be a boolean.");
        } 
    }

    CTransactionRef tx;
    uint256 hashBlock;
    if (!GetTransaction(hash, tx, Params().GetConsensus(), hashBlock, true))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string(fTxIndex ? "No such mempool or blockchain transaction"
            : "No such mempool transaction. Use -txindex to enable blockchain transaction queries") +
            ". Use gettransaction for wallet transactions.");

    string strHex = EncodeHexTx(*tx, RPCSerializationFlags());
	
    if (!fVerbose)
        return strHex;

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("hex", strHex));
    TxToJSON(*tx, hashBlock, result);

    return result;
}

UniValue getfeeoftxid(const JSONRPCRequest& request)
{
	if (request.fHelp || request.params.size() < 1 || request.params.size() > 1)
		throw runtime_error(
		"getfeeoftxid \"txid\" \n"

		"\nNOTE: By default this function only works for mempool transactions. If the -txindex option is\n"
		"enabled, it also works for blockchain transactions.\n"
		"DEPRECATED: for now, it also works for transactions with unspent outputs.\n"

		"\nReturn the raw transaction fee.\n"
		"\nIf verbose is 'true', returns an Object with information about 'txid'.\n"

		"\nArguments:\n"
		"1. \"txid\"      (string, required) The transaction id\n"

		"\nResult (if verbose is not set or set to false):\n"
		"\"data\"      (string) The serialized, hex-encoded data for 'txid'\n"

		"\nResult :\n"
		"{\n"
		"  \"fee\" : \"amount\",        (string) The transaction fee \n"
		"}\n"

		"\nExamples:\n"
		+ HelpExampleCli("getfeeoftxid", "\"mytxid\"")
		);

	LOCK(cs_main);

	uint256 hash = ParseHashV(request.params[0], "parameter 1");

	CTransactionRef tx;
	uint256 hashBlock;
	if (!GetTransaction(hash, tx, Params().GetConsensus(), hashBlock, true))
		throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string(fTxIndex ? "No such mempool or blockchain transaction"
		: "No such mempool transaction. Use -txindex to enable blockchain transaction queries") +
		". Use gettransaction for wallet transactions.");
	CAmount nValue = 0;
	nValue = Getfeebytxid(*tx);
	UniValue result(UniValue::VOBJ);
	result.push_back(Pair("fee", ValueFromAmount(nValue)));
	
	return result;
}

UniValue gettxoutproof(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 1 && request.params.size() != 2))
        throw runtime_error(
            "gettxoutproof [\"txid\",...] ( blockhash )\n"
            "\nReturns a hex-encoded proof that \"txid\" was included in a block.\n"
            "\nNOTE: By default this function only works sometimes. This is when there is an\n"
            "unspent output in the utxo for this transaction. To make it always work,\n"
            "you need to maintain a transaction index, using the -txindex command line option or\n"
            "specify the block in which the transaction is included manually (by blockhash).\n"
            "\nArguments:\n"
            "1. \"txids\"       (string) A json array of txids to filter\n"
            "    [\n"
            "      \"txid\"     (string) A transaction hash\n"
            "      ,...\n"
            "    ]\n"
            "2. \"blockhash\"   (string, optional) If specified, looks for txid in the block with this hash\n"
            "\nResult:\n"
            "\"data\"           (string) A string that is a serialized, hex-encoded data for the proof.\n"
        );

    set<uint256> setTxids;
    uint256 oneTxid;
    UniValue txids = request.params[0].get_array();
    for (unsigned int idx = 0; idx < txids.size(); idx++) {
        const UniValue& txid = txids[idx];
        if (txid.get_str().length() != 64 || !IsHex(txid.get_str()))
            throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid txid ")+txid.get_str());
        uint256 hash(uint256S(txid.get_str()));
        if (setTxids.count(hash))
            throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated txid: ")+txid.get_str());
       setTxids.insert(hash);
       oneTxid = hash;
    }

    LOCK(cs_main);

    CBlockIndex* pblockindex = NULL;

    uint256 hashBlock;
    if (request.params.size() > 1)
    {
        hashBlock = uint256S(request.params[1].get_str());
        if (!mapBlockIndex.count(hashBlock))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        pblockindex = mapBlockIndex[hashBlock];
    } else {
        CCoins coins;
        if (pcoinsTip->GetCoins(oneTxid, coins) && coins.nHeight > 0 && coins.nHeight <= chainActive.Height())
            pblockindex = chainActive[coins.nHeight];
    }

    if (pblockindex == NULL)
    {
        CTransactionRef tx;
        if (!GetTransaction(oneTxid, tx, Params().GetConsensus(), hashBlock, false) || hashBlock.IsNull())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not yet in block");
        if (!mapBlockIndex.count(hashBlock))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Transaction index corrupt");
        pblockindex = mapBlockIndex[hashBlock];
    }

    CBlock block;
    if(!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus()))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");

    unsigned int ntxFound = 0;
    for (const auto& tx : block.vtx)
        if (setTxids.count(tx->GetHash()))
            ntxFound++;
    if (ntxFound != setTxids.size())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "(Not all) transactions not found in specified block");

    CDataStream ssMB(SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
    CMerkleBlock mb(block, setTxids);
    ssMB << mb;
    std::string strHex = HexStr(ssMB.begin(), ssMB.end());
    return strHex;
}

UniValue verifytxoutproof(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            "verifytxoutproof \"proof\"\n"
            "\nVerifies that a proof points to a transaction in a block, returning the transaction it commits to\n"
            "and throwing an RPC error if the block is not in our best chain\n"
            "\nArguments:\n"
            "1. \"proof\"    (string, required) The hex-encoded proof generated by gettxoutproof\n"
            "\nResult:\n"
            "[\"txid\"]      (array, strings) The txid(s) which the proof commits to, or empty array if the proof is invalid\n"
        );

    CDataStream ssMB(ParseHexV(request.params[0], "proof"), SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
    CMerkleBlock merkleBlock;
    ssMB >> merkleBlock;

    UniValue res(UniValue::VARR);

    vector<uint256> vMatch;
    vector<unsigned int> vIndex;
    if (merkleBlock.txn.ExtractMatches(vMatch, vIndex) != merkleBlock.header.hashMerkleRoot)
        return res;

    LOCK(cs_main);

    if (!mapBlockIndex.count(merkleBlock.header.GetHash()) || !chainActive.Contains(mapBlockIndex[merkleBlock.header.GetHash()]))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found in chain");

    BOOST_FOREACH(const uint256& hash, vMatch)
        res.push_back(hash.GetHex());
    return res;
}
/*./ipchain-cli -testnet  TESTcreaterawtransaction "[{\"txid\":\"a291eddeb9dc5dd13528b37d0c1f4ead87715b604283ac66876d3c99494f2aca\",\"vout\":0}]" "[{\"address\":\"TCBWcEXaTSmWVWFKGsftWedbCCKXpisVqYCo\",\"tokensymbol\":\"KKK1\",\"tokenhash\":\"11111111111111111111111111111111\",\"tokenlabel\":\"ss\",\"tokenissue\":1506787200,\"totalcount\":2000,\"accuracy\":2,\"version\":1,\"addmode\":1,\"height\":35000,\"currentCount\":200,\"extendinfo\":\"sd\",\"value\":0,\"txtype\":6,\"txLabel\":\"\"}]" 6
*/
UniValue TESTcreaterawtransaction(const JSONRPCRequest& request)
{
	if (request.fHelp || request.params.size() < 2 || request.params.size() > 3)
        throw runtime_error(
            "createrawtransaction [{\"txid\":\"id\",\"vout\":n},...] {\"address\":amount,\"data\":\"hex\",...} ( locktime )\n"
            "\nCreate a transaction spending the given inputs and creating new outputs.\n"
            "Outputs can be addresses or data.\n"
            "Returns hex-encoded raw transaction.\n"
            "Note that the transaction's inputs are not signed, and\n"
            "it is not stored in the wallet or transmitted to the network.\n"

            "\nArguments:\n"
            "1. \"inputs\"                (array, required) A json array of json objects\n"
            "     [\n"
            "       {\n"
            "         \"txid\":\"id\",    (string, required) The transaction id\n"
            "         \"vout\":n,         (numeric, required) The output number\n"
            "         \"sequence\":n      (numeric, optional) The sequence number\n"
            "       } \n"
            "       ,...\n"
            "     ]\n"
            "2. \"outputs\"               (object, required) a json object with outputs\n"
            "    {\n"
            "      \"address\": x.xxx,    (numeric or string, required) The key is the ipchain address, the numeric value (can be string) is the " + CURRENCY_UNIT + " amount\n"
            "      \"data\": \"hex\"      (string, required) The key is \"data\", the value is hex encoded data\n"
            "      ,...\n"
            "    }\n"
            "3. txtype                  (numeric, optional, default=0) tx type \n"
            "\nResult:\n"
            "\"transaction\"              (string) hex string of the transaction\n"

            "\nExamples:\n"
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"address\\\":0.01}\"")
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"data\\\":\\\"00010203\\\"}\"")
            + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"address\\\":0.01}\"")
            + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"data\\\":\\\"00010203\\\"}\"")
        );
	RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VARR)(UniValue::VARR)(UniValue::VNUM), true);
    if (request.params[0].isNull() || request.params[1].isNull())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

    UniValue inputs = request.params[0].get_array();
	UniValue outputs = request.params[1].get_array();

    CMutableTransaction rawTx;
	int TxType = 0;
    if (request.params.size() > 2 && !request.params[2].isNull()) {
		TxType = request.params[2].get_int();
    }

	multimap<string, int> mapInput;
	multimap<string, int>::iterator m;
	bool isFindchange = false;
	CAmount nVinValue = 0;
	CScript strScriptnewout;
	std::map<uint256, CWalletTx> mapVinWalletTx;
	mapVinWalletTx.clear();
	set<pair<const CWalletTx*, unsigned int> > setCoins;
	setCoins.clear();
    for (unsigned int idx = 0; idx < inputs.size(); idx++) {
        const UniValue& input = inputs[idx];
        const UniValue& o = input.get_obj();

        uint256 txid = ParseHashO(o, "txid");

        const UniValue& vout_v = find_value(o, "vout");
        if (!vout_v.isNum())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
        int nOutput = vout_v.get_int();
        if (nOutput < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");


		CTransactionRef vintx;
		uint256 hashBlock;
		if (!GetTransaction(txid, vintx, Params().GetConsensus(), hashBlock, true))
			throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string(fTxIndex ? "No such mempool or blockchain transaction"
			: "No such mempool transaction. Use -txindex to enable blockchain transaction queries") +
			". Use gettransaction for wallet transactions.");
		if (!isFindchange)
		{
			strScriptnewout = vintx->vout[nOutput].scriptPubKey;
			isFindchange = true;
		}
		nVinValue += vintx->vout[nOutput].nValue;

		mapInput.insert(make_pair(txid.GetHex(), nOutput));

        uint32_t nSequence = (rawTx.nLockTime ? std::numeric_limits<uint32_t>::max() - 1 : std::numeric_limits<uint32_t>::max());

        // set the sequence number if passed in the parameters object
        const UniValue& sequenceObj = find_value(o, "sequence");
        if (sequenceObj.isNum()) {
            int64_t seqNr64 = sequenceObj.get_int64();
            if (seqNr64 < 0 || seqNr64 > std::numeric_limits<uint32_t>::max())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, sequence number is out of range");
            else
                nSequence = (uint32_t)seqNr64;
        }
		
        CTxIn in(COutPoint(txid, nOutput), CScript(), nSequence);

        rawTx.vin.push_back(in);
		const CWallet* pwallet = pwalletMain;
		CWalletTx prewtx(pwallet,vintx);
		mapVinWalletTx.insert(make_pair(txid, prewtx));
		const CWalletTx* pprewtx = &(mapVinWalletTx[txid]);
		setCoins.insert(make_pair(pprewtx, nOutput));
// 		if (pwalletMain->mapWallet.count(txid) == 0)
// 			throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, txid can't found");
// 		CWalletTx& prewtx = pwalletMain->mapWallet[txid];
// 		const CWalletTx* pprewtx = &prewtx;
// 		
// 		setCoins.insert(make_pair(pprewtx, nOutput));
    }
	CAmount totalvalue = 0;
	set<CBitcoinAddress> setNormalAddress;
	multimap<CBitcoinAddress, uint128> setIPCAddress;
	multimap<CBitcoinAddress, string> setTokenAddress;
	for (unsigned int idx = 0; idx < outputs.size(); idx++) {
		const UniValue& output = outputs[idx];
		const UniValue& o = output.get_obj();

		const UniValue& txDataObj = find_value(o, "data");
		if (txDataObj.isStr() || txDataObj.isNum())
		{
			std::vector<unsigned char> data = ParseHexV(txDataObj.getValStr(), "Data");
			CTxOut out(0, CScript() << OP_RETURN << data);
			rawTx.vout.push_back(out);
		}
		else
		{
			const UniValue& txAddressObj = find_value(o, "address");
			const UniValue& txAmountObj = find_value(o, "value");
			if (!txAddressObj.isStr() || !txAmountObj.isNum())
			{
				throw JSONRPCError(RPC_INVALID_PARAMETER, ("Invalid parameter, none of address or amount"));
			}

			const string& name_ = txAddressObj.get_str();
			CBitcoinAddress address(name_);
			if (!address.IsValid())
				throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid ipchain address: ") + name_);


			CScript scriptPubKey = GetScriptForDestination(address.Get());
			CAmount nAmount = AmountFromValue(txAmountObj);
			CTxOut out;

			//Screening transaction type
			const UniValue& txTypeObj = find_value(o, "txtype");
			if (!txTypeObj.isNum())
			{
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, txType is not a Number");
			}
			uint8_t txType = txTypeObj.get_int();


	
			const UniValue& exTypeObj = find_value(o, "extype");
			const UniValue& txLabelObj = find_value(o, "txLabel");

			//IPC transaction value
			const UniValue& startTimeObj = find_value(o, "starttime");
			const UniValue& stopTimeObj = find_value(o, "stoptime");
			const UniValue& reAuthObj = find_value(o, "reAu");
			const UniValue& uniAuthObj = find_value(o, "uniAu");
			const UniValue& ipcHashObj = find_value(o, "IPChash");
			const UniValue& ipcTitleObj = find_value(o, "IPCtitle");

			//Value of token trade
			const UniValue& TokenSymbolObj = find_value(o, "tokensymbol");
			const UniValue& TokenHashObj = find_value(o, "tokenhash");
			const UniValue& TokenLabelObj = find_value(o, "tokenlabel");
			const UniValue& TokenIssueObj = find_value(o, "tokenissue");
			const UniValue& TokenTotalCountObj = find_value(o, "totalcount");
			const UniValue& TokenValueObj = find_value(o, "tokenvalue");
			const UniValue& TokenAccuracyObj = find_value(o, "accuracy");

			//Addtoken
			const UniValue& TokenVersionObj = find_value(o, "version");
			const UniValue& TokenAddmodeObj = find_value(o, "addmode");
			const UniValue& TokenHeightObj = find_value(o, "height");
			const UniValue& TokenCurrentCountObj = find_value(o, "currentCount");
			const UniValue& TokenExtendinfoObj = find_value(o, "extendinfo");

			//Campaign - related values
			const UniValue& devotetypeObj = find_value(o, "devotetype");
			const UniValue& devotepubkeyhash160Obj = find_value(o, "devotepubkeyhash160");

			IPCLabel ipcLabel;
			DevoteLabel devoteLabel;
			TokenRegLabel regLabel;
			AddTokenLabel regaddLabel;
			TokenLabel tokenlabel;
			
			std::string checkStr;
			std::string devoterpubkeyhashstring;
			uint8_t devoteType;
			CKeyID gottenKeyID;
			std::string devotecontentstring;
			std::string strtxLabel = "";
			strtxLabel=txLabelObj.get_str();  
	
			switch (txType)
			{
			case 1://For trading
				devoterpubkeyhashstring = devotepubkeyhash160Obj.get_str();
				devoteLabel.hash.SetHex(devoterpubkeyhashstring);
				devoteType = devotetypeObj.get_int();
				devoteLabel.ExtendType = devoteType;
				out = CTxOut(nAmount, devoteLabel);
				break;

			case 2://Ownership transaction
			case 3://Licensing deals
				if (!exTypeObj.isNum() ||
					!startTimeObj.isStr() ||
					!stopTimeObj.isStr() ||
					!reAuthObj.isNum() ||
					!uniAuthObj.isNum() ||
					!ipcHashObj.isStr() ||
					!ipcTitleObj.isStr())
				{
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, IPC Params is invalid");
				}
				if (!ParseUInt32(startTimeObj.get_str(), &ipcLabel.startTime))
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, IPC startTime is invalid");
				if (!ParseUInt32(stopTimeObj.get_str(), &ipcLabel.stopTime))
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, IPC stopTime is invalid");
				ipcLabel.hash.SetHex(ipcHashObj.get_str());	

				setIPCAddress.insert(make_pair(address, ipcLabel.hash));


				ipcLabel.ExtendType = exTypeObj.get_int();
				ipcLabel.reAuthorize = reAuthObj.get_int();
				ipcLabel.uniqueAuthorize = uniAuthObj.get_int();
				ipcLabel.labelTitle = ipcTitleObj.get_str();
				out = CTxOut(nAmount, scriptPubKey, (TransactionOutType_t)txType, ipcLabel, strtxLabel);
				break;

			case 4: //Trade in token currency
				if (!TokenSymbolObj.isStr() ||
					!TokenHashObj.isStr() ||
					!TokenLabelObj.isStr() ||
					!TokenIssueObj.isNum() ||
					!TokenTotalCountObj.isNum()||
					!TokenAccuracyObj.isNum())
				{
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, Token Params is invalid");
				}
				//There are no various forms of ipc in Symbol and label
				checkStr = TokenSymbolObj.get_str();
				boost::to_upper(checkStr);				
				if (checkStr.find("IPC") != string::npos)
				{
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenSymbol cannot contain IPC");
				}
				if (checkStr.size() > 8)
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenSymbol MaxLen = 8");

				checkStr = TokenLabelObj.get_str();
				boost::to_upper(checkStr);
				if (checkStr.find("IPC") != string::npos)
				{
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenLabel cannot contain IPC");
				}
				if (checkStr.size() > 16)
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenLabel MaxLen = 16");

				//TokenHash
				checkStr = TokenHashObj.get_str();
				if (checkStr.size() > 32)
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenHash MaxLen = 32(16Byte)");


				memcpy(regLabel.TokenSymbol, TokenSymbolObj.get_str().c_str(), TokenSymbolObj.get_str().size()>8 ? 8 : TokenSymbolObj.get_str().size());

				setTokenAddress.insert(make_pair(address, tokenlabel.getTokenSymbol()));

				regLabel.hash.SetHex(TokenHashObj.get_str());
				memcpy(regLabel.label, TokenLabelObj.get_str().c_str(), TokenLabelObj.get_str().size() > 16 ? 16 : TokenLabelObj.get_str().size());
				regLabel.issueDate = TokenIssueObj.get_int();
				regLabel.totalCount = TokenTotalCountObj.get_int64();
				regLabel.value = TokenTotalCountObj.get_int64();
				regLabel.accuracy = TokenAccuracyObj.get_int();
				out = CTxOut(nAmount, scriptPubKey, regLabel,strtxLabel);
				break;

			case 5: //Tokens trading
				if (!TokenSymbolObj.isStr() ||
					!TokenValueObj.isNum() ||
					!TokenAccuracyObj.isNum())
				{
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, Token Params is invalid");
				}
				checkStr = TokenSymbolObj.get_str();
				boost::to_upper(checkStr);
				if (checkStr.find("IPC") != string::npos)
				{
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenSymbol cannot contain IPC");
				}
				if (checkStr.size() > 8)
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenSymbol MaxLen = 8");

				memcpy(tokenlabel.TokenSymbol, TokenSymbolObj.get_str().c_str(), TokenSymbolObj.get_str().size() > 8 ? 8 : TokenSymbolObj.get_str().size());

				setTokenAddress.insert(make_pair(address, tokenlabel.getTokenSymbol()));

				tokenlabel.value = TokenValueObj.get_int64();
				tokenlabel.accuracy = TokenAccuracyObj.get_int();
				out = CTxOut(nAmount, scriptPubKey, tokenlabel,strtxLabel);
				break;

			case 6: //AddToken test
				if (!TokenSymbolObj.isStr() ||
					!TokenHashObj.isStr() ||
					!TokenLabelObj.isStr() ||
					!TokenIssueObj.isNum() ||
					!TokenTotalCountObj.isNum() ||
					!TokenAccuracyObj.isNum()||
					!TokenVersionObj.isNum() ||
					!TokenAddmodeObj.isNum() ||
					!TokenHeightObj.isNum() ||
					!TokenCurrentCountObj.isNum() ||
					!TokenExtendinfoObj.isStr() 
				)
				{
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, Token Params is invalid");
				}
				//There are no various forms of ipc in Symbol and label
				checkStr = TokenSymbolObj.get_str();
				boost::to_upper(checkStr);
				if (checkStr.find("IPC") != string::npos)
				{
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenSymbol cannot contain IPC");
				}
				if (checkStr.size() > 8)
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenSymbol MaxLen = 8");

				checkStr = TokenLabelObj.get_str();
				boost::to_upper(checkStr);
				if (checkStr.find("IPC") != string::npos)
				{
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenLabel cannot contain IPC");
				}
				if (checkStr.size() > 16)
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenLabel MaxLen = 16");

				//TokenHash
				checkStr = TokenHashObj.get_str();
				if (checkStr.size() > 32)
					throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, TokenHash MaxLen = 32(16Byte)");


				memcpy(regaddLabel.TokenSymbol, TokenSymbolObj.get_str().c_str(), TokenSymbolObj.get_str().size() > 8 ? 8 : TokenSymbolObj.get_str().size());

				setTokenAddress.insert(make_pair(address, tokenlabel.getTokenSymbol()));

				regaddLabel.hash.SetHex(TokenHashObj.get_str());
				memcpy(regaddLabel.label, TokenLabelObj.get_str().c_str(), TokenLabelObj.get_str().size() > 16 ? 16 : TokenLabelObj.get_str().size());
				regaddLabel.issueDate = TokenIssueObj.get_int();
				regaddLabel.totalCount = TokenTotalCountObj.get_int64();
				regaddLabel.accuracy = TokenAccuracyObj.get_int();
				regaddLabel.version = TokenVersionObj.get_int64();
				regaddLabel.addmode = TokenAddmodeObj.get_int64();
				regaddLabel.height = TokenHeightObj.get_int64();
				regaddLabel.currentCount = TokenCurrentCountObj.get_int64();
				regaddLabel.extendinfo = TokenExtendinfoObj.get_str();

				out = CTxOut(nAmount, scriptPubKey, regaddLabel, strtxLabel);
				break;
			case 0://General trading
				setNormalAddress.insert(address);

				out = CTxOut(nAmount, scriptPubKey);
				break;
			default:
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, txType out of range");

			}

			rawTx.vout.push_back(out);
			totalvalue += out.nValue;


		}

	}
	CAmount nvalue1(0);
	CTxOut newout(nvalue1, strScriptnewout);
	rawTx.vout.push_back(newout);


	if (!(pwalletMain->DummySignTx(rawTx, setCoins))) {
		throw JSONRPCError(RPC_INVALID_PARAMETER, "DummySignTx erro!");
	}
	unsigned int nBytes = GetVirtualTransactionSize(rawTx);
	CAmount fee = pwalletMain->GetMinimumFee(nBytes, 8, mempool) * 2;
	//Add a change
	if (TxType == 4 || TxType == 6)
	{
		CAmount newoutvalue = nVinValue - totalvalue - AmountFromValue(100) - fee;
		rawTx.vout[rawTx.vout.size() - 1].nValue = newoutvalue;
	}
	else
	{
		CAmount newoutvalue = nVinValue - totalvalue - fee;
		rawTx.vout[rawTx.vout.size() - 1].nValue = newoutvalue;
	}

    return EncodeHexTx(rawTx);
}

UniValue createrawtransaction(const JSONRPCRequest& request)
{
	if (request.fHelp || request.params.size() < 2 || request.params.size() > 2)
		throw runtime_error(
		"createrawtransaction [{\"txid\":\"id\",\"vout\":n},...] {\"address\":amount,...} \n"
		"\nCreate a transaction spending the given inputs and creating new outputs.\n"
		"Outputs can be addresses .\n"
		"Returns hex-encoded raw transaction.\n"
		"Note that the transaction's inputs are not signed, and\n"
		"it is not stored in the wallet or transmitted to the network.\n"

		"\nArguments:\n"
		"1. \"inputs\"                (array, required) A json array of json objects\n"
		"     [\n"
		"       {\n"
		"         \"txid\":\"id\",    (string, required) The transaction id\n"
		"         \"vout\":n,         (numeric, required) The output number\n"
		"       } \n"
		"       ,...\n"
		"     ]\n"
		"2. \"outputs\"               (object, required) a json object with outputs\n"
		"    {\n"
		"      \"address\": x.xxx,    (numeric or string, required) The key is the ipchain address, the numeric value (can be string) is the " + CURRENCY_UNIT + " amount\n"
		"      ,...\n"
		"    }\n"
		"\nResult:\n"
		"\"transaction\"              (string) hex string of the transaction\n"

		"\nExamples:\n"
		+ HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"address\\\":0.01}\"")
		+ HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"address\\\":0.01}\"")
		);

	RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VARR)(UniValue::VOBJ)(UniValue::VNUM), true);
	if (request.params[0].isNull() || request.params[1].isNull())
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

	UniValue inputs = request.params[0].get_array();
	UniValue sendTo = request.params[1].get_obj();

	CMutableTransaction rawTx;
	bool isFindNormalchange = false;
	bool isFindTokenchange = false;
	CScript strScriptNormalnewout;
	CScript strScripTokentnewout;
	CAmount nVinAllValue = 0;
	CAmount nVoutAllValue = 0;
	uint64_t nVinAllTokenValue = 0;
	uint64_t nVoutAllTokenValue = 0;
	set<pair<const CWalletTx*, unsigned int> > setCoins;
	setCoins.clear();
	TokenLabel tokenlabel;
	std::string  strtokensymbol;
	for (unsigned int idx = 0; idx < inputs.size(); idx++) {
		const UniValue& input = inputs[idx];
		const UniValue& o = input.get_obj();

		uint256 txid = ParseHashO(o, "txid");
		CTransactionRef vintx;
		uint256 hashBlock;
		if (!GetTransaction(txid, vintx, Params().GetConsensus(), hashBlock, true))
			throw JSONRPCError(RPC_INVALID_PARAMETER, std::string(fTxIndex ? "No such mempool or blockchain transaction"
			: "No such mempool transaction. Use -txindex to enable blockchain transaction queries") +
			". Use gettransaction for wallet transactions.");

		const UniValue& vout_v = find_value(o, "vout");
		if (!vout_v.isNum())
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
		int nOutput = vout_v.get_int();
		if (nOutput < 0 || nOutput >= vintx->vout.size())
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");

		uint32_t nSequence = (rawTx.nLockTime ? std::numeric_limits<uint32_t>::max() - 1 : std::numeric_limits<uint32_t>::max());

	
		const CTxOut& preout = vintx->vout[nOutput];
		uint8_t utxotype = preout.txType;
		if (utxotype != TXOUT_NORMAL && /*utxotype != TXOUT_CAMPAIGN && utxotype != TXOUT_TOKENREG &&*/ utxotype != TXOUT_TOKEN)
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vin txType must be 0 or 5 ");
	
		if (!isFindNormalchange && utxotype == TXOUT_NORMAL)
		{
			strScriptNormalnewout = preout.scriptPubKey;
			isFindNormalchange = true;
		}
		if (!isFindTokenchange && utxotype == TXOUT_TOKEN)
		{
			strScripTokentnewout = preout.scriptPubKey;
			isFindTokenchange = true;
			tokenlabel = preout.tokenLabel;
			strtokensymbol = preout.tokenLabel.getTokenSymbol();
		}
		if (utxotype == TXOUT_NORMAL)
			nVinAllValue += preout.nValue;
		if (utxotype == TXOUT_TOKEN)
		{
			if (strtokensymbol != preout.tokenLabel.getTokenSymbol())
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, Multiple token types are not allowed in input ");
			nVinAllTokenValue += preout.tokenLabel.value;
		}
		CTxIn in(COutPoint(txid, nOutput), CScript(), nSequence);
		rawTx.vin.push_back(in);

		if (pwalletMain->mapWallet.count(txid) == 0)
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, txid can't found");
		CWalletTx& prewtx = pwalletMain->mapWallet[txid];
		const CWalletTx* pprewtx = &prewtx;
		setCoins.insert(make_pair(pprewtx, nOutput));
	}
	set<CBitcoinAddress> setAddress;
	vector<string> addrList = sendTo.getKeys();
	BOOST_FOREACH(const string& name_, addrList) {

			CBitcoinAddress address(name_);
			if (!address.IsValid())
				throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid IPChain address: ") + name_);

			if (setAddress.count(address))
				throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ") + name_);
			setAddress.insert(address);

			CScript scriptPubKey = GetScriptForDestination(address.Get());
			if (isFindTokenchange)
			{
				TokenLabel outtokenlabel = tokenlabel;
				int64_t outvalue= TCoinsFromValue(sendTo[name_], tokenlabel.accuracy);
				if (outvalue <= 0 )
					throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, Invalid values: ") + sendTo[name_].getValStr());
				 outtokenlabel.value = (uint64_t)outvalue;
				CAmount namount = 0;
				CTxOut out(namount, scriptPubKey,outtokenlabel);
			
				rawTx.vout.push_back(out);
				nVoutAllTokenValue += outtokenlabel.value;
			}
			else{
				CAmount nAmount = AmountFromValue(sendTo[name_]);
				CTxOut out(nAmount, scriptPubKey);
				if (nAmount <= 0)
					throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, Invalid values: ") + sendTo[name_].getValStr());
				rawTx.vout.push_back(out);
				nVoutAllValue += nAmount;
			}
			
	}

	if (isFindTokenchange)
	{
		CAmount namount = 0;
		if (nVinAllTokenValue < nVoutAllTokenValue)
			throw JSONRPCError(RPC_INVALID_PARAMETER, "The token you want to send is not enough!");
		uint64_t nChangevalue = nVinAllTokenValue - nVoutAllTokenValue;
		TokenLabel outtokenchange = tokenlabel;
		outtokenchange.value = nChangevalue;
		CTxOut newTokenchangeOut(namount, strScripTokentnewout, outtokenchange);
		if (nChangevalue > 0)
			rawTx.vout.push_back(newTokenchangeOut);
	}
	CAmount nvalue1(0);
	CTxOut newout(nvalue1, strScriptNormalnewout);
	rawTx.vout.push_back(newout);

	if (!(pwalletMain->DummySignTx(rawTx, setCoins))) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "DummySignTx error!");
	}
	unsigned int nBytes = GetVirtualTransactionSize(rawTx);
	CAmount fee = pwalletMain->GetMinimumFee(nBytes, 8, mempool);

	CAmount newoutvalue = nVinAllValue - nVoutAllValue - fee;
	if (newoutvalue < 0)
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot support the  fee!");
	rawTx.vout[rawTx.vout.size() - 1].nValue = newoutvalue;
	if (newoutvalue == 0)
	{
		std::vector<CTxOut>::iterator  it= rawTx.vout.end();
		it--;
		rawTx.vout.erase(it);
	}
	

	return EncodeHexTx(rawTx);
}

UniValue createrawtransactionforisolation(const JSONRPCRequest& request)
{
	if (request.fHelp || request.params.size() < 2 || request.params.size() > 2)
		throw runtime_error(
        "createrawtransactionforisolation(createrawtransactionForIsolation) [{\"txid\":\"id\",\"vout\":n},...] {\"address\":amount,...} \n"
		"\nCreate a transaction spending the given inputs and creating new outputs.\n"
		"Outputs can be addresses .\n"
		"Returns hex-encoded raw transaction.\n"
		"Note that the transaction's inputs are not signed, and\n"
		"it is not stored in the wallet or transmitted to the network.\n"

		"\nArguments:\n"
		"1. \"inputs\"                (array, required) A json array of json objects\n"
		"     [\n"
		"       {\n"
		"         \"amount\":nvalue,			(numeric, required) The utxo amount\n"
		"         \"scriptPubKey\":\"XXXXXXX\",		(string, required) The utxo scriptPubKey\n"
		"         \"txid\":\"id\",    (string, required) The transaction id\n"
		"         \"vout\":n,         (numeric, required) The output number\n"
		"       } \n"
		"       ,...\n"
		"     ]\n"
		"2. \"outputs\"               (object, required) a json object with outputs\n"
		"    {\n"
		"      \"address\": x.xxx,    (numeric or string, required) The key is the ipchain address, the numeric value (can be string) is the " + CURRENCY_UNIT + " amount\n"
		"      ,...\n"
		"    }\n"
		"\nResult:\n"
		"\"transaction\"              (string) hex string of the transaction\n"

		"\nExamples:\n"
        + HelpExampleCli("createrawtransactionforisolation", "\"[{\\\"amount\\\":2.00000000,\\\"scriptPubKey\\\":\\\"2103fd90d60174ba0351edd1fa299fcf88b16c1433afe16b882c3a268062d9a4fc38ac\\\",\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"address\\\":0.01}\"")
        + HelpExampleRpc("createrawtransactionforisolation", "\"[{\\\"amount\\\":2.00000000,\\\"scriptPubKey\\\":\\\"2103fd90d60174ba0351edd1fa299fcf88b16c1433afe16b882c3a268062d9a4fc38ac\\\",\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"address\\\":0.01}\"")
		);

	RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VARR)(UniValue::VOBJ), true);
	if (request.params[0].isNull() || request.params[1].isNull())
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

	UniValue inputs = request.params[0].get_array();
	UniValue sendTo = request.params[1].get_obj();

	CMutableTransaction rawTx;
	bool isFindNormalchange = false;
	bool isFindTokenchange = false;
	CScript strScriptNormalnewout;
	CScript strScriptTokennewout;
	CAmount nVinAllValue = 0;
	CAmount nVoutAllValue = 0;
	uint64_t nVinAllTokenValue = 0;
	uint64_t nVoutAllTokenValue = 0;
	std::string  strtokensymbol;
	uint8_t tokenaccuracy = 0;
	TokenLabel tokenlabel;
	std::vector<CScript> vinscriptPubkeys;
	unsigned int nvin = inputs.size();
	vinscriptPubkeys.reserve(nvin);
	vinscriptPubkeys.clear();
	for (unsigned int idx = 0; idx < inputs.size(); idx++) {
		const UniValue& input = inputs[idx];
		const UniValue& o = input.get_obj();
		std::vector<std::string> inkeys;
		inkeys = o.getKeys();
		if (find_value(o, "txid").isNull() || find_value(o, "vout").isNull() ||
			find_value(o, "amount").isNull() || find_value(o, "scriptPubKey").isNull())
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing keys");
		uint256 txid = ParseHashO(o, "txid");
		const UniValue& vout_v = find_value(o, "vout");

		uint8_t utxotype = TXOUT_NORMAL;
		std::string  stokensymbol;
		if (!find_value(o, "symbol").isNull())
		{
			if (find_value(o, "accuracy").isNull())
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing accuracy key");
			utxotype = TXOUT_TOKEN;

		}
		const UniValue& vout_value = find_value(o, "amount");
	
		
		const UniValue& vout_scriptPubKey = find_value(o, "scriptPubKey");
		if (!vout_v.isNum())
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
		int nOutput = vout_v.get_int();
		
		uint32_t nSequence = (rawTx.nLockTime ? std::numeric_limits<uint32_t>::max() - 1 : std::numeric_limits<uint32_t>::max());
		
		CTxIn in(COutPoint(txid, nOutput), CScript(), nSequence);
		rawTx.vin.push_back(in);

		vector<unsigned char> pkData(ParseHexO(o, "scriptPubKey"));
		CScript scriptPubKey(pkData.begin(), pkData.end());
		vinscriptPubkeys.push_back(scriptPubKey);
		if (!isFindNormalchange && utxotype == TXOUT_NORMAL)
		{
			strScriptNormalnewout = scriptPubKey;
			isFindNormalchange = true;
		}
		if (!isFindTokenchange && utxotype == TXOUT_TOKEN)
		{
			strScriptTokennewout = scriptPubKey;
			isFindTokenchange = true;
			strtokensymbol = find_value(o, "symbol").get_str();
			int accnum = find_value(o, "accuracy").get_int32();
			if (accnum < 0 || accnum > 8)
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, The accuracy should not be less than 0 or more than 8");
			tokenaccuracy = (uint8_t)accnum;
			memcpy((char*)(tokenlabel.TokenSymbol), (char*)(strtokensymbol.c_str()), sizeof(tokenlabel.TokenSymbol));
			tokenlabel.accuracy = tokenaccuracy;
		}
		if (utxotype == TXOUT_NORMAL)
		{
			CAmount nvalue = AmountFromValue(vout_value);
			nVinAllValue += nvalue;
		}
		if (utxotype == TXOUT_TOKEN)
		{
			stokensymbol = find_value(o, "symbol").get_str();
			if (strtokensymbol != stokensymbol)
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, Multiple token types are not allowed in input ");
			int accnum = find_value(o, "accuracy").get_int32();
			if (accnum < 0 || accnum > 8)
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, The accuracy should not be less than 0 or more than 8");
			if (accnum != tokenaccuracy)
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, The accuracy of the tokens in the input is not allowed to be different ");
			nVinAllTokenValue += TCoinsFromValue(vout_value, accnum);
		}
	}
	set<CBitcoinAddress> setAddress;
	vector<string> addrList = sendTo.getKeys();
	BOOST_FOREACH(const string& name_, addrList) {

		CBitcoinAddress address(name_);
		if (!address.IsValid())
			throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid IPChain address: ") + name_);

		if (setAddress.count(address))
			throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ") + name_);
		setAddress.insert(address);

		CScript scriptPubKey = GetScriptForDestination(address.Get());
		if (isFindTokenchange)
		{
			TokenLabel outtokenlabel = tokenlabel;
			int64_t outvalue = TCoinsFromValue(sendTo[name_], tokenaccuracy);
			if (outvalue <= 0)
				throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, Invalid values: ") + sendTo[name_].getValStr());
			outtokenlabel.value = (uint64_t)outvalue;
			CAmount namount = 0;
			CTxOut out(namount, scriptPubKey, outtokenlabel);

			rawTx.vout.push_back(out);
			nVoutAllTokenValue += outtokenlabel.value;
		}
		else{
			CAmount nAmount = AmountFromValue(sendTo[name_]);
			CTxOut out(nAmount, scriptPubKey);
			if (nAmount <= 0)
				throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, Invalid values: ") + sendTo[name_].getValStr());
			rawTx.vout.push_back(out);
			nVoutAllValue += nAmount;
		}

	}
	if (isFindTokenchange)
	{
		CAmount namount = 0;
		if (nVinAllTokenValue < nVoutAllTokenValue)
			throw JSONRPCError(RPC_INVALID_PARAMETER, "The token you want to send is not enough!");
		uint64_t nChangevalue = nVinAllTokenValue - nVoutAllTokenValue;
		TokenLabel outtokenchange = tokenlabel;
		outtokenchange.value = nChangevalue;
		CTxOut newTokenchangeOut(namount, strScriptTokennewout, outtokenchange);
		if (nChangevalue > 0)
			rawTx.vout.push_back(newTokenchangeOut);
	}
	CAmount nvalue1(0);
	CTxOut newout(nvalue1, strScriptNormalnewout);
	rawTx.vout.push_back(newout);

	for (unsigned int idx = 0; idx < inputs.size(); idx++)
	{
		const CScript& scriptPubKey = vinscriptPubkeys[idx];
		SignatureData sigdata;

		if (!ProduceSignature(DummySignatureCreator(pwalletMain), scriptPubKey, sigdata))
		{
			throw JSONRPCError(RPC_INVALID_PARAMETER, "ProduceSignature DummySign error!");
		}
		else {
			UpdateTransaction(rawTx, idx, sigdata);
		}

	}


	unsigned int nBytes = GetVirtualTransactionSize(rawTx);
	CAmount fee = pwalletMain->GetMinimumFee(nBytes, 8, mempool);

	CAmount newoutvalue = nVinAllValue - nVoutAllValue - fee;
	if (newoutvalue < 0)
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot support the  fee!");
	rawTx.vout[rawTx.vout.size() - 1].nValue = newoutvalue;
	if (newoutvalue == 0)
	{
		std::vector<CTxOut>::iterator  it = rawTx.vout.end();
		it--;
		rawTx.vout.erase(it);
	}


	return EncodeHexTx(rawTx);
}

UniValue decoderawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            "decoderawtransaction \"hexstring\"\n"
            "\nReturn a JSON object representing the serialized, hex-encoded transaction.\n"

            "\nArguments:\n"
            "1. \"hexstring\"      (string, required) The transaction hex string\n"

            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"id\",        (string) The transaction id\n"
            "  \"hash\" : \"id\",        (string) The transaction hash (differs from txid for witness transactions)\n"
            "  \"size\" : n,             (numeric) The transaction size\n"
            "  \"vsize\" : n,            (numeric) The virtual transaction size (differs from size for witness transactions)\n"
            "  \"version\" : n,          (numeric) The version\n"
            "  \"locktime\" : ttt,       (numeric) The lock time\n"
            "  \"vin\" : [               (array of json objects)\n"
            "     {\n"
            "       \"txid\": \"id\",    (string) The transaction id\n"
            "       \"vout\": n,         (numeric) The output number\n"
			"       \"addresses\" : [    (json array of string)\n"
			"           \"12tvKAXCxZjSmdNbao16dKXC8tRWfcF5oc\"   (string) ipchain address\n"
			"       \"type\": n,        (numeric) The txout type \n"
			"       \"nvalue\": x.xxx,   (numeric) The value in " + CURRENCY_UNIT + "\n "
            "       \"scriptSig\": {     (json object) The script\n"
            "         \"asm\": \"asm\",  (string) asm\n"
            "         \"hex\": \"hex\"   (string) hex\n"
            "       },\n"
            "       \"txinwitness\": [\"hex\", ...] (array of string) hex-encoded witness data (if any)\n"
            "       \"sequence\": n     (numeric) The script sequence number\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"vout\" : [             (array of json objects)\n"
            "     {\n"
            "       \"value\" : x.xxx,            (numeric) The value in " + CURRENCY_UNIT + "\n"
            "       \"n\" : n,                    (numeric) index\n"
			"       \"type\": n,                  (numeric) The txout type \n"
			"       .......										\n"
			"       .......										\n"
            "       \"scriptPubKey\" : {          (json object)\n"
            "         \"asm\" : \"asm\",          (string) the asm\n"
            "         \"hex\" : \"hex\",          (string) the hex\n"
            "         \"reqSigs\" : n,            (numeric) The required sigs\n"
            "         \"type\" : \"pubkeyhash\",  (string) The type, eg 'pubkeyhash'\n"
            "         \"addresses\" : [           (json array of string)\n"
            "           \"12tvKAXCxZjSmdNbao16dKXC8tRWfcF5oc\"   (string) ipchain address\n"
            "           ,...\n"
            "         ]\n"
            "       }\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("decoderawtransaction", "\"hexstring\"")
            + HelpExampleRpc("decoderawtransaction", "\"hexstring\"")
        );

    LOCK(cs_main);
    RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VSTR));

    CMutableTransaction mtx;

    if (!DecodeHexTx(mtx, request.params[0].get_str(), true))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

    UniValue result(UniValue::VOBJ);
    TxToJSON(CTransaction(std::move(mtx)), uint256(), result);

    return result;
}

UniValue decodescript(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            "decodescript \"hexstring\"\n"
            "\nDecode a hex-encoded script.\n"
            "\nArguments:\n"
            "1. \"hexstring\"     (string) the hex encoded script\n"
            "\nResult:\n"
            "{\n"
            "  \"asm\":\"asm\",   (string) Script public key\n"
            "  \"hex\":\"hex\",   (string) hex encoded public key\n"
            "  \"type\":\"type\", (string) The output type\n"
            "  \"reqSigs\": n,    (numeric) The required signatures\n"
            "  \"addresses\": [   (json array of string)\n"
            "     \"address\"     (string) ipchain address\n"
            "     ,...\n"
            "  ],\n"
            "  \"p2sh\",\"address\" (string) address of P2SH script wrapping this redeem script (not returned if the script is already a P2SH).\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("decodescript", "\"hexstring\"")
            + HelpExampleRpc("decodescript", "\"hexstring\"")
        );

    RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VSTR));

    UniValue r(UniValue::VOBJ);
    CScript script;
    if (request.params[0].get_str().size() > 0){
        vector<unsigned char> scriptData(ParseHexV(request.params[0], "argument"));
        script = CScript(scriptData.begin(), scriptData.end());
    } else {
        // Empty scripts are valid
    }
    ScriptPubKeyToJSON(script, r, false);

    UniValue type;
    type = find_value(r, "type");

    if (type.isStr() && type.get_str() != "scripthash") {
        // P2SH cannot be wrapped in a P2SH. If this script is already a P2SH,
        // don't return the address for a P2SH of the P2SH.
        r.push_back(Pair("p2sh", CBitcoinAddress(CScriptID(script)).ToString()));
    }

    return r;
}

/** Pushes a JSON object for script verification or signing errors to vErrorsRet. */
static void TxInErrorToJSON(const CTxIn& txin, UniValue& vErrorsRet, const std::string& strMessage)
{
    UniValue entry(UniValue::VOBJ);
    entry.push_back(Pair("txid", txin.prevout.hash.ToString()));
    entry.push_back(Pair("vout", (uint64_t)txin.prevout.n));
    entry.push_back(Pair("scriptSig", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
    entry.push_back(Pair("sequence", (uint64_t)txin.nSequence));
    entry.push_back(Pair("error", strMessage));
    vErrorsRet.push_back(entry);
}

UniValue signrawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 4)
        throw runtime_error(
            "signrawtransaction \"hexstring\" ( [{\"txid\":\"id\",\"vout\":n,\"scriptPubKey\":\"hex\",\"redeemScript\":\"hex\"},...] [\"privatekey1\",...] sighashtype )\n"
            "\nSign inputs for raw transaction (serialized, hex-encoded).\n"
            "The second optional argument (may be null) is an array of previous transaction outputs that\n"
            "this transaction depends on but may not yet be in the block chain.\n"
            "The third optional argument (may be null) is an array of base58-encoded private\n"
            "keys that, if given, will be the only keys used to sign the transaction.\n"
#ifdef ENABLE_WALLET
            + HelpRequiringPassphrase() + "\n"
#endif

            "\nArguments:\n"
            "1. \"hexstring\"     (string, required) The transaction hex string\n"
            "2. \"prevtxs\"       (string, optional) An json array of previous dependent transaction outputs\n"
            "     [               (json array of json objects, or 'null' if none provided)\n"
            "       {\n"
            "         \"txid\":\"id\",             (string, required) The transaction id\n"
            "         \"vout\":n,                  (numeric, required) The output number\n"
            "         \"scriptPubKey\": \"hex\",   (string, required) script key\n"
            "         \"redeemScript\": \"hex\",   (string, required for P2SH or P2WSH) redeem script\n"
            "         \"amount\": value            (numeric, required) The amount spent\n"
            "       }\n"
            "       ,...\n"
            "    ]\n"
            "3. \"privkeys\"     (string, optional) A json array of base58-encoded private keys for signing\n"
            "    [                  (json array of strings, or 'null' if none provided)\n"
            "      \"privatekey\"   (string) private key in base58-encoding\n"
            "      ,...\n"
            "    ]\n"
            "4. \"sighashtype\"     (string, optional, default=ALL) The signature hash type. Must be one of\n"
            "       \"ALL\"\n"
            "       \"NONE\"\n"
            "       \"SINGLE\"\n"
            "       \"ALL|ANYONECANPAY\"\n"
            "       \"NONE|ANYONECANPAY\"\n"
            "       \"SINGLE|ANYONECANPAY\"\n"

            "\nResult:\n"
            "{\n"
            "  \"hex\" : \"value\",           (string) The hex-encoded raw transaction with signature(s)\n"
            "  \"complete\" : true|false,   (boolean) If the transaction has a complete set of signatures\n"
            "  \"errors\" : [                 (json array of objects) Script verification errors (if there are any)\n"
            "    {\n"
            "      \"txid\" : \"hash\",           (string) The hash of the referenced, previous transaction\n"
            "      \"vout\" : n,                (numeric) The index of the output to spent and used as input\n"
            "      \"scriptSig\" : \"hex\",       (string) The hex-encoded signature script\n"
            "      \"sequence\" : n,            (numeric) Script sequence number\n"
            "      \"error\" : \"text\"           (string) Verification or signing error related to the input\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("signrawtransaction", "\"myhex\"")
            + HelpExampleRpc("signrawtransaction", "\"myhex\"")
        );

#ifdef ENABLE_WALLET
    LOCK2(cs_main, pwalletMain ? &pwalletMain->cs_wallet : NULL);
#else
    LOCK(cs_main);
#endif
    RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VSTR)(UniValue::VARR)(UniValue::VARR)(UniValue::VSTR), true);

    vector<unsigned char> txData(ParseHexV(request.params[0], "argument 1"));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    vector<CMutableTransaction> txVariants;
    while (!ssData.empty()) {
        try {
            CMutableTransaction tx;
            ssData >> tx;
            txVariants.push_back(tx);
        }
        catch (const std::exception&) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
        }
    }

    if (txVariants.empty())
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Missing transaction");

    // mergedTx will end up with all the signatures; it
    // starts as a clone of the rawtx:
    CMutableTransaction mergedTx(txVariants[0]);

    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        BOOST_FOREACH(const CTxIn& txin, mergedTx.vin) {
            const uint256& prevHash = txin.prevout.hash;
            CCoins coins;
            view.AccessCoins(prevHash); // this is certainly allowed to fail
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    bool fGivenKeys = false;
    CBasicKeyStore tempKeystore;
    if (request.params.size() > 2 && !request.params[2].isNull()) {
        fGivenKeys = true;
        UniValue keys = request.params[2].get_array();
        for (unsigned int idx = 0; idx < keys.size(); idx++) {
            UniValue k = keys[idx];
            CBitcoinSecret vchSecret;
            bool fGood = vchSecret.SetString(k.get_str());
            if (!fGood)
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
            CKey key = vchSecret.GetKey();
            if (!key.IsValid())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range");
            tempKeystore.AddKey(key);
        }
    }
#ifdef ENABLE_WALLET
    else if (pwalletMain)
        EnsureWalletIsUnlocked();
#endif

    // Add previous txouts given in the RPC call:
    if (request.params.size() > 1 && !request.params[1].isNull()) {
        UniValue prevTxs = request.params[1].get_array();
        for (unsigned int idx = 0; idx < prevTxs.size(); idx++) {
            const UniValue& p = prevTxs[idx];
            if (!p.isObject())
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"txid'\",\"vout\",\"scriptPubKey\"}");

            UniValue prevOut = p.get_obj();

            RPCTypeCheckObj(prevOut,
                {
                    {"txid", UniValueType(UniValue::VSTR)},
                    {"vout", UniValueType(UniValue::VNUM)},
                    {"scriptPubKey", UniValueType(UniValue::VSTR)},
                });

            uint256 txid = ParseHashO(prevOut, "txid");

            int nOut = find_value(prevOut, "vout").get_int();
            if (nOut < 0)
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "vout must be positive");

            vector<unsigned char> pkData(ParseHexO(prevOut, "scriptPubKey"));
            CScript scriptPubKey(pkData.begin(), pkData.end());

            {
                CCoinsModifier coins = view.ModifyCoins(txid);
                if (coins->IsAvailable(nOut) && coins->vout[nOut].scriptPubKey != scriptPubKey) {
                    string err("Previous output scriptPubKey mismatch:\n");
                    err = err + ScriptToAsmStr(coins->vout[nOut].scriptPubKey) + "\nvs:\n"+
                        ScriptToAsmStr(scriptPubKey);
                    throw JSONRPCError(RPC_DESERIALIZATION_ERROR, err);
                }
                if ((unsigned int)nOut >= coins->vout.size())
                    coins->vout.resize(nOut+1);
                coins->vout[nOut].scriptPubKey = scriptPubKey;
                coins->vout[nOut].nValue = 0;
                if (prevOut.exists("amount")) {
                    coins->vout[nOut].nValue = AmountFromValue(find_value(prevOut, "amount"));
                }
            }

            // if redeemScript given and not using the local wallet (private keys
            // given), add redeemScript to the tempKeystore so it can be signed:
            if (fGivenKeys && (scriptPubKey.IsPayToScriptHash() || scriptPubKey.IsPayToWitnessScriptHash())) {
                RPCTypeCheckObj(prevOut,
                    {
                        {"txid", UniValueType(UniValue::VSTR)},
                        {"vout", UniValueType(UniValue::VNUM)},
                        {"scriptPubKey", UniValueType(UniValue::VSTR)},
                        {"redeemScript", UniValueType(UniValue::VSTR)},
                    });
                UniValue v = find_value(prevOut, "redeemScript");
                if (!v.isNull()) {
                    vector<unsigned char> rsData(ParseHexV(v, "redeemScript"));
                    CScript redeemScript(rsData.begin(), rsData.end());
                    tempKeystore.AddCScript(redeemScript);
                }
            }
        }
    }

#ifdef ENABLE_WALLET
    const CKeyStore& keystore = ((fGivenKeys || !pwalletMain) ? tempKeystore : *pwalletMain);
#else
    const CKeyStore& keystore = tempKeystore;
#endif

    int nHashType = SIGHASH_ALL;
    if (request.params.size() > 3 && !request.params[3].isNull()) {
        static map<string, int> mapSigHashValues =
            boost::assign::map_list_of
            (string("ALL"), int(SIGHASH_ALL))
            (string("ALL|ANYONECANPAY"), int(SIGHASH_ALL|SIGHASH_ANYONECANPAY))
            (string("NONE"), int(SIGHASH_NONE))
            (string("NONE|ANYONECANPAY"), int(SIGHASH_NONE|SIGHASH_ANYONECANPAY))
            (string("SINGLE"), int(SIGHASH_SINGLE))
            (string("SINGLE|ANYONECANPAY"), int(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY))
            ;
        string strHashType = request.params[3].get_str();
        if (mapSigHashValues.count(strHashType))
            nHashType = mapSigHashValues[strHashType];
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid sighash param");
    }

    bool fHashSingle = ((nHashType & ~SIGHASH_ANYONECANPAY) == SIGHASH_SINGLE);

    // Script verification errors
    UniValue vErrors(UniValue::VARR);

    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(mergedTx);
    // Sign what we can:
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++) {
        CTxIn& txin = mergedTx.vin[i];
        const CCoins* coins = view.AccessCoins(txin.prevout.hash);
        if (coins == NULL || !coins->IsAvailable(txin.prevout.n)) {
            TxInErrorToJSON(txin, vErrors, "Input not found or already spent");
            continue;
        }
        const CScript& prevPubKey = coins->vout[txin.prevout.n].scriptPubKey;
        const CAmount& amount = coins->vout[txin.prevout.n].nValue;

        SignatureData sigdata;
        // Only sign SIGHASH_SINGLE if there's a corresponding output:
        if (!fHashSingle || (i < mergedTx.vout.size()))
            ProduceSignature(MutableTransactionSignatureCreator(&keystore, &mergedTx, i, amount, nHashType), prevPubKey, sigdata);

        // ... and merge in other signatures:
        BOOST_FOREACH(const CMutableTransaction& txv, txVariants) {
            if (txv.vin.size() > i) {
                sigdata = CombineSignatures(prevPubKey, TransactionSignatureChecker(&txConst, i, amount), sigdata, DataFromTransaction(txv, i));
            }
        }

        UpdateTransaction(mergedTx, i, sigdata);

        ScriptError serror = SCRIPT_ERR_OK;
        if (!VerifyScript(txin.scriptSig, prevPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txConst, i, amount), &serror)) {
            TxInErrorToJSON(txin, vErrors, ScriptErrorString(serror));
        }
    }
    bool fComplete = vErrors.empty();
	unsigned int nBytes = GetVirtualTransactionSize(mergedTx);
	
    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("hex", EncodeHexTx(mergedTx)));
    result.push_back(Pair("complete", fComplete));
    if (!vErrors.empty()) {
        result.push_back(Pair("errors", vErrors));
    }

    return result;
}

UniValue sendrawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw runtime_error(
            "sendrawtransaction \"hexstring\" ( allowhighfees )\n"
            "\nSubmits raw transaction (serialized, hex-encoded) to local node and network.\n"
            "\nAlso see createrawtransaction and signrawtransaction calls.\n"
            "\nArguments:\n"
            "1. \"hexstring\"    (string, required) The hex string of the raw transaction)\n"
            "2. allowhighfees    (boolean, optional, default=false) Allow high fees\n"
            "\nResult:\n"
            "\"hex\"             (string) The transaction hash in hex\n"
            "\nExamples:\n"
            "\nCreate a transaction\n"
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\" : \\\"mytxid\\\",\\\"vout\\\":0}]\" \"{\\\"myaddress\\\":0.01}\"") +
            "Sign the transaction, and get back the hex\n"
            + HelpExampleCli("signrawtransaction", "\"myhex\"") +
            "\nSend the transaction (signed hex)\n"
            + HelpExampleCli("sendrawtransaction", "\"signedhex\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("sendrawtransaction", "\"signedhex\"")
        );

    LOCK(cs_main);
    RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VSTR)(UniValue::VBOOL));

    // parse hex string from parameter
    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    CTransactionRef tx(MakeTransactionRef(std::move(mtx)));
    const uint256& hashTx = tx->GetHash();
// 
    bool fLimitFree = false;
    CAmount nMaxRawTxFee = maxTxFee;
    if (request.params.size() > 1 && request.params[1].get_bool())
        nMaxRawTxFee = 0;

    CCoinsViewCache &view = *pcoinsTip;
    const CCoins* existingCoins = view.AccessCoins(hashTx);
    bool fHaveMempool = mempool.exists(hashTx);
    bool fHaveChain = existingCoins && existingCoins->nHeight < 1000000000;
    if (!fHaveMempool && !fHaveChain) {
        // push to local node and sync with wallets
        CValidationState state;
        bool fMissingInputs;
        if (!AcceptToMemoryPool(mempool, state, std::move(tx), fLimitFree, &fMissingInputs, NULL, false, nMaxRawTxFee)) {
            if (state.IsInvalid()) {
                throw JSONRPCError(RPC_TRANSACTION_REJECTED, strprintf("%i: %s", state.GetRejectCode(), state.GetRejectReason()));
            } else {
                if (fMissingInputs) {
                    throw JSONRPCError(RPC_TRANSACTION_ERROR, "Missing inputs");
                }
                throw JSONRPCError(RPC_TRANSACTION_ERROR, state.GetRejectReason());
            }
        }
    } else if (fHaveChain) {
        throw JSONRPCError(RPC_TRANSACTION_ALREADY_IN_CHAIN, "transaction already in block chain");
    }
    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
	
    CInv inv(MSG_TX, hashTx);
    g_connman->ForEachNode([&inv](CNode* pnode)
    {
        pnode->PushInventory(inv);
    });

    return hashTx.GetHex();
}


UniValue base58decodetohexstring(const JSONRPCRequest& request)
{
	if (request.fHelp || request.params.size() < 1 || request.params.size() > 1)
		throw runtime_error(
			"base58decodetohexstring \"hexstring\"\n"
		);
	RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VSTR));
	std::vector<unsigned char> vch;
	if (!DecodeBase58(request.params[0].get_str().c_str(), vch))
		throw JSONRPCError(RPC_INVALID_PARAMS, "Error: invalid Base58 Input String");
	std::string result = HexStr(vch.begin(), vch.end());
	return result;
}

UniValue base58encodefromhexstring(const JSONRPCRequest& request)
{
	if (request.fHelp || request.params.size() < 1 || request.params.size() > 1)
		throw runtime_error(
		"base58encodefromhexstring \"hexstring\"\n"
		);
	RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VSTR));
	std::vector<unsigned char> vch = ParseHex(request.params[0].get_str());
	return EncodeBase58(&vch[0], &vch[0] + vch.size());
}

UniValue gensystemaddress(const JSONRPCRequest& request)
{
	if (request.fHelp || request.params.size() < 1)
		throw runtime_error(
		"gensystemaddress \"hexstring\"\n"
		);
	RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VSTR));
	//Public key plus prefix
	std::vector<unsigned char>vch = ParseHex("035762" + request.params[0].get_str());
	//Do two sha256
	uint256 hash;
	CSHA256().Write(&vch[0], vch.size()).Finalize(hash.begin());
	CSHA256().Write(hash.begin(), 32).Finalize(hash.begin());
	//Get the first four bytes attached
	std::vector<unsigned char> result = ParseHex("035762" + request.params[0].get_str() + "00000000");
	memcpy(&result[0] + 23, hash.begin(), 4);
	std::string tmp = EncodeBase58(&result[0], &result[0] + result.size());
	return HexStr(result.begin(), result.end()) + "\n" + EncodeBase58(&result[0], &result[0] + result.size());
}

UniValue gensystemprivkey(const JSONRPCRequest& request)
{
	if (request.fHelp || request.params.size() < 1)
		throw runtime_error(
			"base58encodehexstring \"hexstring\"\n"
		);
	RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VSTR));
	//Public key plus prefix
	std::vector<unsigned char>vch = ParseHex("ef" + request.params[0].get_str());
	//Do two sha256
	uint256 hash;
	CSHA256().Write(&vch[0], vch.size()).Finalize(hash.begin());
	CSHA256().Write(hash.begin(), 32).Finalize(hash.begin());
	//Get the first four bytes attached
	std::vector<unsigned char> result = ParseHex("ef" + request.params[0].get_str() + "00000000");
	memcpy(&result[0] + 34, hash.begin(), 4);
	std::string tmp = EncodeBase58(&result[0], &result[0] + result.size());
	return HexStr(result.begin(), result.end()) + "\n" + EncodeBase58(&result[0], &result[0] + result.size());
}


static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "rawtransactions",    "getrawtransaction",      &getrawtransaction,      true,  {"txid","verbose"} },
    { "rawtransactions",    "createrawtransaction",   &createrawtransaction,   true,  {"inputs","outputs"} },
    { "hide",            "createrawtransactionForIsolation", &createrawtransactionforisolation, true, { "inputs", "outputs" } },
    { "rawtransactions", "createrawtransactionforisolation", &createrawtransactionforisolation, true, { "inputs", "outputs" } },
    { "rawtransactions",    "decoderawtransaction",   &decoderawtransaction,   true,  {"hexstring"} },
    { "rawtransactions",    "decodescript",           &decodescript,           true,  {"hexstring"} },
    { "rawtransactions",    "sendrawtransaction",     &sendrawtransaction,     false, {"hexstring","allowhighfees"} },
    { "rawtransactions",    "signrawtransaction",     &signrawtransaction,     false, {"hexstring","prevtxs","privkeys","sighashtype"} }, /* uses wallet if enabled */
	{ "rawtransactions",	"getfeeoftxid",		      &getfeeoftxid,		   true,  { "txid" } },

    { "blockchain",         "gettxoutproof",          &gettxoutproof,          true,  {"txids", "blockhash"} },
    { "blockchain",         "verifytxoutproof",       &verifytxoutproof,       true,  {"proof"} },
	

    { "rawtransactions",    "base58decodetohexstring",  &base58decodetohexstring,  true,  {"hexstring"} },
    { "rawtransactions",    "base58encodefromhexstring", &base58encodefromhexstring,  true,  {"hexstring"} },
//    { "rawtransactions",    "gensystemaddress",			&gensystemaddress,  true,  {"hexstring"} },
//	{ "rawtransactions",    "gensystemprivkey",			&gensystemprivkey,  true,  { "hexstring" } },
	{ "rawtransactions", "TESTcreaterawtransaction", &TESTcreaterawtransaction, true, { "inputs", "outputs","type" } },

};

void RegisterRawTransactionRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
