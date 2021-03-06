// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/client.h"
#include "rpc/protocol.h"
#include "util.h"

#include <set>
#include <stdint.h>

#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <univalue.h>

using namespace std;

class CRPCConvertParam
{
public:
    std::string methodName; //!< method whose params want conversion
    int paramIdx;           //!< 0-based idx of param to convert
    std::string paramName;  //!< parameter name
};

/**
 * Specifiy a (method, idx, name) here if the argument is a non-string RPC
 * argument and needs to be converted from JSON.
 *
 * @note Parameter indexes start from 0.
 */
static const CRPCConvertParam vRPCConvertParams[] =
{
    { "setmocktime", 0, "timestamp" },
    { "generate", 0, "nblocks" },
    { "generate", 1, "maxtries" },
	{ "generatedpoc", 0, "nblocks" },
	{ "generatedpoc", 1, "nPeriodCount" },
	{ "generatedpoc", 2, "nPeriodStartTime" },
	{ "generatedpoc", 3, "nTimePeriod" },
	{ "getdpoclist", 0, "num" },
    { "generatetoaddress", 0, "nblocks" },
    { "generatetoaddress", 2, "maxtries" },
    { "getnetworkhashps", 0, "nblocks" },
    { "getnetworkhashps", 1, "height" },
    { "sendtoaddress", 1, "amount" },
    { "sendtoaddress", 4, "subtractfeefromamount" },
	{ "IPCSendToAddressWithTxLabel", 1, "index" },
    { "ipcsendtoaddresswithtxLabel", 1, "index" },
	{ "IPCAuthorToAddressWithTxLabel", 1, "index" },
    { "ipcauthortoaddresswithtxlabel", 1, "index" },
    { "settxfee", 0, "amount" },
    { "getreceivedbyaddress", 1, "minconf" },
    { "getreceivedbyaccount", 1, "minconf" },
    { "listreceivedbyaddress", 0, "minconf" },
    { "listreceivedbyaddress", 1, "include_empty" },
    { "listreceivedbyaddress", 2, "include_watchonly" },
    { "listreceivedbyaccount", 0, "minconf" },
    { "listreceivedbyaccount", 1, "include_empty" },
    { "listreceivedbyaccount", 2, "include_watchonly" },
    { "getbalance", 1, "minconf" },
    { "getbalance", 2, "include_watchonly" },
    { "getblockhash", 0, "height" },
    { "waitforblockheight", 0, "height" },
    { "waitforblockheight", 1, "timeout" },
    { "waitforblock", 1, "timeout" },
    { "waitfornewblock", 0, "timeout" },
    { "move", 2, "amount" },
    { "move", 3, "minconf" },
    { "sendfrom", 2, "amount" },
    { "sendfrom", 3, "minconf" },
    { "listtransactions", 1, "count" },
    { "listtransactions", 2, "skip" },
    { "listtransactions", 3, "include_watchonly" },
	{ "listiptransactions", 0, "iptype" },
	{ "listiptransactions", 1, "count" },
	{ "listiptransactions", 2, "skip" },
	{ "listiptransactions", 3, "include_watchonly" },
    { "getiptransaction", 1, "include_watchonly" },
	{ "listtokentransactions", 1, "count" },
	{ "listtokentransactions", 2, "skip" },
	{ "listtokentransactions", 3, "include_watchonly" },
    { "listtokentransactionsforunion", 1, "count" },
    { "listtokentransactionsforunion", 2, "skip" },
    { "listaccounts", 0, "minconf" },
    { "listaccounts", 1, "include_watchonly" },
    { "walletpassphrase", 1, "timeout" },
    { "getblocktemplate", 0, "template_request" },
    { "listsinceblock", 1, "target_confirmations" },
    { "listsinceblock", 2, "include_watchonly" },
    { "sendmany", 1, "amounts" },
//     { "sendmany", 2, "minconf" },
//     { "sendmany", 4, "subtractfeefrom" },
	{ "sendtokenmany", 2, "amounts" },
    { "unionverifytokenvout", 2, "amounts" },
    { "unionsendtokenmany", 2, "amounts" },
    { "unionsendtokenmany", 3, "needsign" },
    { "unionsendtokenmany", 4, "maxvinsize" },
    { "unionsendtokenmany", 5, "minvinsize" },
    { "unionsendtokenmany", 6, "minconfirmation" },
    { "addmultisigaddress", 0, "nrequired" },
    { "unionsignintegrated", 1, "keys" },
    { "createmultiaddress", 0, "nrequired" },
    { "createmultiaddress", 1, "keys" },
    { "createmultisig", 0, "nrequired" },
    { "createmultisig", 1, "keys" },
    { "getunioncoins", 0, "addresses" },
    { "listunspent", 0, "minconf" },
    { "listunspent", 1, "maxconf" },
    { "listunspent", 2, "addresses" },
	{ "listunspent", 3, "include_unsafe" },
	{ "listunspentNormal", 0, "minconf" },
	{ "listunspentNormal", 1, "maxconf" },
	{ "listunspentNormal", 2, "addresses" },
	{ "listunspentNormal", 3, "include_unsafe" },
    { "listunspentnormal", 0, "minconf" },
    { "listunspentnormal", 1, "maxconf" },
    { "listunspentnormal", 2, "addresses" },
    { "listunspentnormal", 3, "include_unsafe" },
    { "listunspentIPC", 0, "minconf" },
	{ "listunspentIPC", 1, "maxconf" },
	{ "listunspentIPC", 2, "addresses" },
	{ "listunspentIPC", 3, "include_unsafe" },
    { "listunspentipc", 0, "minconf" },
    { "listunspentipc", 1, "maxconf" },
    { "listunspentipc", 2, "addresses" },
    { "listunspentipc", 3, "include_unsafe" },
	{ "listunspentToken", 0, "minconf" },
	{ "listunspentToken", 1, "maxconf" },
	{ "listunspentToken", 2, "addresses" },
	{ "listunspentToken", 3, "include_unsafe" },
    { "listunspenttoken", 0, "minconf" },
    { "listunspenttoken", 1, "maxconf" },
    { "listunspenttoken", 2, "addresses" },
    { "listunspenttoken", 3, "include_unsafe" },
	{ "listunspentTokenBySymbol", 1, "minconf" },
	{ "listunspentTokenBySymbol", 2, "maxconf" },
    { "listunspentTokenBySymbol", 3, "addresses" },
	{ "listunspentTokenBySymbol", 4, "include_unsafe" },
    { "listunspenttokenbysymbol", 1, "minconf" },
    { "listunspenttokenbysymbol", 2, "maxconf" },
    { "listunspenttokenbysymbol", 3, "addresses" },
    { "listunspenttokenbysymbol", 4, "include_unsafe" },
    { "getblock", 1, "verbose" },
    { "getblockheader", 1, "verbose" },
    { "gettransaction", 1, "include_watchonly" },
	{ "gettokentransaction", 1, "include_watchonly" },
    { "getrawtransaction", 1, "verbose" },
    { "createrawtransaction", 0, "inputs" },
    { "createrawtransaction", 1, "outputs" },
	{ "createrawtransactionForIsolation", 0, "inputs" },
	{ "createrawtransactionForIsolation", 1, "outputs" },
    { "createrawtransactionforisolation", 0, "inputs" },
    { "createrawtransactionforisolation", 1, "outputs" },
    { "signrawtransaction", 1, "prevtxs" },
    { "signrawtransaction", 2, "privkeys" },
    { "sendrawtransaction", 1, "allowhighfees" },
    { "fundrawtransaction", 1, "options" },
    { "gettxout", 1, "n" },
    { "gettxout", 2, "include_mempool" },
    { "gettxoutproof", 0, "txids" },
    { "lockunspent", 0, "unlock" },
    { "lockunspent", 1, "transactions" },
    { "importprivkey", 2, "rescan" },
    { "importprivkeybibipay", 2, "rescan" },
    { "importaddress", 2, "rescan" },
    { "importaddress", 3, "p2sh" },
    { "importpubkey", 2, "rescan" },
    { "importmulti", 0, "requests" },
    { "importmulti", 1, "options" },
    { "verifychain", 0, "checklevel" },
    { "verifychain", 1, "nblocks" },
    { "pruneblockchain", 0, "height" },
    { "keypoolrefill", 0, "newsize" },
    { "getrawmempool", 0, "verbose" },
    { "estimatefee", 0, "nblocks" },
    { "estimatepriority", 0, "nblocks" },
    { "estimatesmartfee", 0, "nblocks" },
    { "estimatesmartpriority", 0, "nblocks" },
    { "prioritisetransaction", 1, "priority_delta" },
    { "prioritisetransaction", 2, "fee_delta" },
    { "setban", 2, "bantime" },
    { "setban", 3, "absolute" },
    { "setnetworkactive", 0, "state" },
    { "getmempoolancestors", 1, "verbose" },
    { "getmempooldescendants", 1, "verbose" },
    { "bumpfee", 1, "options" },
    // Echo with conversion (For testing only)
    { "echojson", 0, "arg0" },
    { "echojson", 1, "arg1" },
    { "echojson", 2, "arg2" },
    { "echojson", 3, "arg3" },
    { "echojson", 4, "arg4" },
    { "echojson", 5, "arg5" },
    { "echojson", 6, "arg6" },
    { "echojson", 7, "arg7" },
    { "echojson", 8, "arg8" },
    { "echojson", 9, "arg9" },
    { "uniongettxidsfromtxrecord", 1, "minblocknum" },
    { "uniongettxidsfromtxrecord", 2, "maxblocknum" },
    { "addmultiadd", 2, "relatedtome" },
	{ "getaddressutxos", 1, "type" },
	{ "addtokenregtoaddress", 2, "addtokenreginfo" },
	{ "TESTcreaterawtransaction", 0, "inputs" },
	{ "TESTcreaterawtransaction", 1, "outputs" },
	{ "TESTcreaterawtransaction", 2, "type" },
	


};

class CRPCConvertTable
{
private:
    std::set<std::pair<std::string, int>> members;
    std::set<std::pair<std::string, std::string>> membersByName;

public:
    CRPCConvertTable();

    bool convert(const std::string& method, int idx) {
        return (members.count(std::make_pair(method, idx)) > 0);
    }
    bool convert(const std::string& method, const std::string& name) {
        return (membersByName.count(std::make_pair(method, name)) > 0);
    }
};

CRPCConvertTable::CRPCConvertTable()
{
    const unsigned int n_elem =
        (sizeof(vRPCConvertParams) / sizeof(vRPCConvertParams[0]));

    for (unsigned int i = 0; i < n_elem; i++) {
        members.insert(std::make_pair(vRPCConvertParams[i].methodName,
                                      vRPCConvertParams[i].paramIdx));
        membersByName.insert(std::make_pair(vRPCConvertParams[i].methodName,
                                            vRPCConvertParams[i].paramName));
    }
}

static CRPCConvertTable rpcCvtTable;

/** Non-RFC4627 JSON parser, accepts internal values (such as numbers, true, false, null)
 * as well as objects and arrays.
 */
UniValue ParseNonRFCJSONValue(const std::string& strVal)
{
    UniValue jVal;
    if (!jVal.read(std::string("[")+strVal+std::string("]")) ||
        !jVal.isArray() || jVal.size()!=1)
        throw runtime_error(string("Error parsing JSON:")+strVal);
    return jVal[0];
}

UniValue RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VARR);

    for (unsigned int idx = 0; idx < strParams.size(); idx++) {
        const std::string& strVal = strParams[idx];

        if (!rpcCvtTable.convert(strMethod, idx)) {
            // insert string value directly
            params.push_back(strVal);
        } else {
            // parse string as JSON, insert bool/number/object/etc. value
            params.push_back(ParseNonRFCJSONValue(strVal));
        }
    }

    return params;
}

UniValue RPCConvertNamedValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VOBJ);

    for (const std::string &s: strParams) {
        size_t pos = s.find("=");
        if (pos == std::string::npos) {
            throw(std::runtime_error("No '=' in named argument '"+s+"', this needs to be present for every argument (even if it is empty)"));
        }

        std::string name = s.substr(0, pos);
        std::string value = s.substr(pos+1);

        if (!rpcCvtTable.convert(strMethod, name)) {
            // insert string value directly
            params.pushKV(name, value);
        } else {
            // parse string as JSON, insert bool/number/object/etc. value
            params.pushKV(name, ParseNonRFCJSONValue(value));
        }
    }

    return params;
}
