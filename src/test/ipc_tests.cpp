// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "serialize.h"
#include "streams.h"
#include "hash.h"
#include "data/tx_invalid.json.h"
#include "data/tx_valid.json.h"
#include "test/test_bitcoin.h"

#include "rpc/server.h"
#include "rpc/client.h"

#include "clientversion.h"
#include "checkqueue.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "key.h"
#include "keystore.h"
#include "validation.h" // For CheckTransaction
#include "policy/policy.h"
#include "script/script.h"
#include "script/sign.h"
#include "script/script_error.h"
#include "script/standard.h"
#include "utilstrencodings.h"

#include <map>
#include <string>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>

#include <univalue.h>

typedef std::vector<unsigned char> valtype;
class CSerializeMethodsTestSingle1
{
protected:
	int intval;
	bool boolval;
	std::string stringval;
	const char* charstrval;
	CTransactionRef txval;
public:
	CSerializeMethodsTestSingle1() = default;
	CSerializeMethodsTestSingle1(int intvalin, bool boolvalin, std::string stringvalin, const char* charstrvalin, CTransaction txvalin) : intval(intvalin), boolval(boolvalin), stringval(std::move(stringvalin)), charstrval(charstrvalin), txval(MakeTransactionRef(txvalin)){}
	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(intval);
		READWRITE(boolval);
		READWRITE(stringval);
		READWRITE(FLATDATA(charstrval));
		READWRITE(txval);
	}

	bool operator==(const CSerializeMethodsTestSingle1& rhs)
	{
		return  intval == rhs.intval && \
			boolval == rhs.boolval && \
			stringval == rhs.stringval && \
			strcmp(charstrval, rhs.charstrval) == 0 && \
			*txval == *rhs.txval;
	}
};

class CSerializeMethodsTestMany1 : public CSerializeMethodsTestSingle1
{
public:
	using CSerializeMethodsTestSingle1::CSerializeMethodsTestSingle1;
	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITEMANY(intval, boolval, stringval, FLATDATA(charstrval), txval);
	}
};


// In script_tests.cpp
extern UniValue read_json(const std::string& jsondata);

extern std::map<std::string, unsigned int> mapFlagNames;

extern unsigned int ParseScriptFlags(std::string strFlags);

extern std::string FormatScriptFlags(unsigned int flags);

extern UniValue CallRPC(std::string args);

BOOST_FIXTURE_TEST_SUITE(ipc_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(ipc_normalTX)
{
	UniValue ret;
	std::string prevout;
	std::string lastresult;
	std::string check;
	UniValue r;
	std::string privkey1 = "\"KzsXybp9jX64P5ekX1KUxRQ79Jht9uzW7LorgwE65i5rWACL6LQe\"";
	std::string privkey2 = "\"Kyhdf5LuKTRx4ge69ybABsiUAWjVRK4XGxAKk2FQLp2HjGMy87Z4\"";
	std::string notsigned;

	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":0,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11}]");
	lastresult = ret.getValStr();
	BOOST_CHECK_EQUAL(lastresult, "0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb40100000000ffffffff0100ab904100000000000017a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000");

	check = "{\"txid\":\"c694e7a6bc14eb9df9ba39f099b197345d620fa714dcdc542ae69fe0ceae3734\",\"hash\":\"c694e7a6bc14eb9df9ba39f099b197345d620fa714dcdc542ae69fe0ceae3734\",\"size\":86,\"vsize\":86,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295}],\"vout\":[{\"value\":11.00000000,\"n\":0,\"type\":0,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}";
	BOOST_CHECK_NO_THROW(ret = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(ret.write(), check);
	
	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == false);
	
	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[" + privkey1 + "," + privkey2 + "]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == true);
	lastresult = r.write();
	BOOST_CHECK_EQUAL(lastresult, "{\"hex\":\"0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb401000000910047304402201dfecd454eabe946faae801dacb386c148983bb8f680bb9173528b023bfd7fa8022017ad854c1e54f92649fff7bec0d9e61fc5d871bcf018e52bfb441c68d2c2b93d0147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052aeffffffff0100ab904100000000000017a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000\",\"complete\":true}");
	
	lastresult = find_value(r.get_obj(), "hex").get_str();
	//decode
	BOOST_CHECK_NO_THROW(r = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(r.write(), "{\"txid\":\"4f5fd67790c34a917f5a7f4ce409b433b61baa245128a3adbef94b1f781f1907\",\"hash\":\"4f5fd67790c34a917f5a7f4ce409b433b61baa245128a3adbef94b1f781f1907\",\"size\":231,\"vsize\":231,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\",\"vout\":1,\"scriptSig\":{\"asm\":\"0 304402201dfecd454eabe946faae801dacb386c148983bb8f680bb9173528b023bfd7fa8022017ad854c1e54f92649fff7bec0d9e61fc5d871bcf018e52bfb441c68d2c2b93d[ALL] 512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\",\"hex\":\"0047304402201dfecd454eabe946faae801dacb386c148983bb8f680bb9173528b023bfd7fa8022017ad854c1e54f92649fff7bec0d9e61fc5d871bcf018e52bfb441c68d2c2b93d0147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"},\"sequence\":4294967295}],\"vout\":[{\"value\":11.00000000,\"n\":0,\"type\":0,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");



	
	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":0,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]");
	lastresult = ret.getValStr();
	BOOST_CHECK_EQUAL(lastresult, "0200000001dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd0100000000ffffffff01008c864700000000000017a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000");

	BOOST_CHECK_NO_THROW(ret = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(ret.write(), "{\"txid\":\"00f2464fb679e6c5e2028d5b0310a9b232964782da31534fa9e52f6a06f1defc\",\"hash\":\"00f2464fb679e6c5e2028d5b0310a9b232964782da31534fa9e52f6a06f1defc\",\"size\":86,\"vsize\":86,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295}],\"vout\":[{\"value\":12.00000000,\"n\":0,\"type\":0,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");
	
}

BOOST_AUTO_TEST_CASE(ipc_mutiple_normalTx)
{
	UniValue ret;
	std::string prevout;
	std::string lastresult;
	std::string check;
	UniValue r;
	std::string privkey1 = "\"KzsXybp9jX64P5ekX1KUxRQ79Jht9uzW7LorgwE65i5rWACL6LQe\"";
	std::string privkey2 = "\"Kyhdf5LuKTRx4ge69ybABsiUAWjVRK4XGxAKk2FQLp2HjGMy87Z4\"";
	std::string notsigned;

	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"},"
		"{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":0,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11}]");
	lastresult = ret.getValStr();
	BOOST_CHECK_EQUAL(lastresult, "0200000002f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb40100000000ffffffffdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd0100000000ffffffff0100ab904100000000000017a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000");
	
	BOOST_CHECK_NO_THROW(ret = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(ret.write(), "{\"txid\":\"54a274ba98c4846dc0f2a607c0389bc07844511369d9fbdedb8bc52669f5a95a\",\"hash\":\"54a274ba98c4846dc0f2a607c0389bc07844511369d9fbdedb8bc52669f5a95a\",\"size\":127,\"vsize\":127,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295},{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295}],\"vout\":[{\"value\":11.00000000,\"n\":0,\"type\":0,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");
	
	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == false);
	
	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[" + privkey1 + "," + privkey2 + "]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == true);
	
	
	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":0,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11},"
		"{\"txtype\":0,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);
	
	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":0,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11},"
		"{\"txtype\":0,\"address\":\"1GRi7C9Ku4ktJwzXUQKFoSdcQRTKYEdret\",\"value\":12}]");
	lastresult = ret.getValStr();
	BOOST_CHECK_EQUAL(lastresult, "0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb40100000000ffffffff0200ab904100000000000017a914b10c9df5f7edf436c697f02f1efdba4cf39961518700008c86470000000000001976a914a9362f1cf3c47797f927d976817230d294060a4688ac0000000000");
	
	BOOST_CHECK_NO_THROW(ret = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(ret.write(), "{\"txid\":\"927448c1b849afb08e1e63d3467aa5e6851ec3d0e860d92a58a9c91fda33fb9a\",\"hash\":\"927448c1b849afb08e1e63d3467aa5e6851ec3d0e860d92a58a9c91fda33fb9a\",\"size\":123,\"vsize\":123,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295}],\"vout\":[{\"value\":11.00000000,\"n\":0,\"type\":0,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}},{\"value\":12.00000000,\"n\":1,\"type\":0,\"scriptPubKey\":{\"asm\":\"OP_DUP OP_HASH160 a9362f1cf3c47797f927d976817230d294060a46 OP_EQUALVERIFY OP_CHECKSIG\",\"hex\":\"76a914a9362f1cf3c47797f927d976817230d294060a4688ac\",\"reqSigs\":1,\"type\":\"pubkeyhash\",\"addresses\":[\"1GRi7C9Ku4ktJwzXUQKFoSdcQRTKYEdret\"]}}]}");

	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == false);
	
	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[" + privkey1 + "," + privkey2 + "]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == true);
	

	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"},"
		"{\"txid\":\"b1c232fa13c89a976503ff562d036b147dfe4131b49d01a9876624eaa99ddf1b\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":0,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11},"
		"{\"txtype\":0,\"address\":\"1GRi7C9Ku4ktJwzXUQKFoSdcQRTKYEdret\",\"value\":12}]");
	lastresult = ret.getValStr();
	BOOST_CHECK_EQUAL(lastresult, "0200000002f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb40100000000ffffffff1bdf9da9ea246687a9019db43141fe7d146b032d56ff0365979ac813fa32c2b10100000000ffffffff0200ab904100000000000017a914b10c9df5f7edf436c697f02f1efdba4cf39961518700008c86470000000000001976a914a9362f1cf3c47797f927d976817230d294060a4688ac0000000000");
	
	BOOST_CHECK_NO_THROW(ret = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(ret.write(), "{\"txid\":\"3fcdc1a3c9db60a39f17921607c05f15a74d58e47d7dd159f1b0547b0fdb1498\",\"hash\":\"3fcdc1a3c9db60a39f17921607c05f15a74d58e47d7dd159f1b0547b0fdb1498\",\"size\":164,\"vsize\":164,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295},{\"txid\":\"b1c232fa13c89a976503ff562d036b147dfe4131b49d01a9876624eaa99ddf1b\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295}],\"vout\":[{\"value\":11.00000000,\"n\":0,\"type\":0,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}},{\"value\":12.00000000,\"n\":1,\"type\":0,\"scriptPubKey\":{\"asm\":\"OP_DUP OP_HASH160 a9362f1cf3c47797f927d976817230d294060a46 OP_EQUALVERIFY OP_CHECKSIG\",\"hex\":\"76a914a9362f1cf3c47797f927d976817230d294060a4688ac\",\"reqSigs\":1,\"type\":\"pubkeyhash\",\"addresses\":[\"1GRi7C9Ku4ktJwzXUQKFoSdcQRTKYEdret\"]}}]}");

	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == false);
	
	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[" + privkey1 + "," + privkey2 + "]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == true);

}

BOOST_AUTO_TEST_CASE(ipc_ipTx)
{
	UniValue ret;
	std::string prevout;
	UniValue r;
	std::string privkey1 = "\"KzsXybp9jX64P5ekX1KUxRQ79Jht9uzW7LorgwE65i5rWACL6LQe\"";
	std::string privkey2 = "\"Kyhdf5LuKTRx4ge69ybABsiUAWjVRK4XGxAKk2FQLp2HjGMy87Z4\"";
	std::string notsigned;


	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":\"1\","
		"\"starttime\":0,"
		"\"stoptime\":0,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"testtitle\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":1,"
		"\"starttime\":\"0\","
		"\"stoptime\":0,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"testtitle\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);


	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":1,"
		"\"starttime\":0,"
		"\"stoptime\":\"0\","
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"testtitle\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);

	
	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":1,"
		"\"starttime\":0,"
		"\"stoptime\":0,"
		"\"reauthorize\":\"1\","
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"testtitle\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":1,"
		"\"starttime\":0,"
		"\"stoptime\":0,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":\"0\","
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"testtitle\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);

	
	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":1,"
		"\"starttime\":0,"
		"\"stoptime\":0,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":f393847c97508f24b772281deea475cd,"
		"\"IPCTitle\":\"testtitle\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":1,"
		"\"starttime\":0,"
		"\"stoptime\":0,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":testtitle,"
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":1,"
		"\"starttime\":0,"
		"\"stoptime\":0,"
		"\"reauthorize\":\"0\","
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"testtitle\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":0,"
		"\"starttime\":10000,"
		"\"stoptime\":5000,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"title\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11}]"), std::runtime_error);


	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":0,"
		"\"starttime\":0,"
		"\"stoptime\":0,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"title\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11}]");
	BOOST_CHECK_EQUAL(ret.getValStr(), "0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb40100000000ffffffff0100ab904100000000020022000000000000000000010010cd75a4ee1d2872b7248f50977c8493f3057469746c6517a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000");

	
	BOOST_CHECK_NO_THROW(ret = CallRPC("decoderawtransaction 0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb40100000000ffffffff0100ab904100000000020022000000000000000000010010cd75a4ee1d2872b7248f50977c8493f3057469746c6517a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000"));
	BOOST_CHECK_EQUAL(ret.write(), "{\"txid\":\"a7ccd285cb7a3f58929069a0a2bd7a2524dbb9282fb0a3bf90dc3d2730332622\",\"hash\":\"a7ccd285cb7a3f58929069a0a2bd7a2524dbb9282fb0a3bf90dc3d2730332622\",\"size\":121,\"vsize\":121,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295}],\"vout\":[{\"value\":11.00000000,\"n\":0,\"type\":2,\"extype\":0,\"starttime\":0,\"stoptime\":0,\"reauthorize\":1,\"uniqueauthorize\":0,\"IPChash\":\"f393847c97508f24b772281deea475cd\",\"IPCTitle\":\"title\",\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");
	

	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	r = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":0,"
		"\"starttime\":0,"
		"\"stoptime\":0,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"testtitle\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11}]");

	notsigned = r.get_str();
	r = CallRPC(std::string("signrawtransaction ") + notsigned + " " + prevout + " " + "[]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == false);

	r = CallRPC(std::string("signrawtransaction ") + notsigned + " " + prevout + " " + "[" + privkey1 + "," + privkey2 + "]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == true);
	BOOST_CHECK_EQUAL(r.write(), "{\"hex\":\"0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb401000000910047304402202c349c4f7f0e96f77c32240b1caea022583aee50e957e70e677270d508e9a59c022004548220cc23c9dc1b3d8116acb7932bd7a29a3fd9e149c4f1c2e498e10311120147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052aeffffffff0100ab904100000000020026000000000000000000010010cd75a4ee1d2872b7248f50977c8493f309746573747469746c6517a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000\",\"complete\":true}");

	//decode
	BOOST_CHECK_NO_THROW(r = CallRPC("decoderawtransaction 0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb401000000910047304402202c349c4f7f0e96f77c32240b1caea022583aee50e957e70e677270d508e9a59c022004548220cc23c9dc1b3d8116acb7932bd7a29a3fd9e149c4f1c2e498e10311120147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052aeffffffff0100ab904100000000020026000000000000000000010010cd75a4ee1d2872b7248f50977c8493f309746573747469746c6517a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000"));
	BOOST_CHECK_EQUAL(r.write(), "{\"txid\":\"268c4830e97febfb83a6a28d301edfcec252675f3b6f99ffb9d11052eed1d377\",\"hash\":\"268c4830e97febfb83a6a28d301edfcec252675f3b6f99ffb9d11052eed1d377\",\"size\":270,\"vsize\":270,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\",\"vout\":1,\"scriptSig\":{\"asm\":\"0 304402202c349c4f7f0e96f77c32240b1caea022583aee50e957e70e677270d508e9a59c022004548220cc23c9dc1b3d8116acb7932bd7a29a3fd9e149c4f1c2e498e1031112[ALL] 512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\",\"hex\":\"0047304402202c349c4f7f0e96f77c32240b1caea022583aee50e957e70e677270d508e9a59c022004548220cc23c9dc1b3d8116acb7932bd7a29a3fd9e149c4f1c2e498e10311120147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"},\"sequence\":4294967295}],\"vout\":[{\"value\":11.00000000,\"n\":0,\"type\":2,\"extype\":0,\"starttime\":0,\"stoptime\":0,\"reauthorize\":1,\"uniqueauthorize\":0,\"IPChash\":\"f393847c97508f24b772281deea475cd\",\"IPCTitle\":\"testtitle\",\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");




	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":0,"
		"\"starttime\":0,"
		"\"stoptime\":0,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"testtitle\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11}]");

	BOOST_CHECK_EQUAL(ret.getValStr(), "0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb40100000000ffffffff0100ab904100000000020026000000000000000000010010cd75a4ee1d2872b7248f50977c8493f309746573747469746c6517a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000");

	
	
	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":3,"
		"\"extype\":0,"
		"\"starttime\":0,"
		"\"stoptime\":10000,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"title\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11}]"), std::runtime_error);
	

	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":3,"
		"\"extype\":0,"
		"\"starttime\":10000,"
		"\"stoptime\":0,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"title\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11}]"), std::runtime_error);


	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":3,"
		"\"extype\":0,"
		"\"starttime\":10000,"
		"\"stoptime\":5000,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"title\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11}]"), std::runtime_error);


	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":3,"
		"\"extype\":0,"
		"\"starttime\":1000,"
		"\"stoptime\":2000,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"title\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11}]");

	BOOST_CHECK_EQUAL(ret.getValStr(), "0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb40100000000ffffffff0100ab90410000000003002200e8030000d0070000010010cd75a4ee1d2872b7248f50977c8493f3057469746c6517a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000");

	
	BOOST_CHECK_NO_THROW(ret = CallRPC("decoderawtransaction 0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb40100000000ffffffff0100ab904100000000030022000000000000000000010010cd75a4ee1d2872b7248f50977c8493f3057469746c6517a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000"));
	BOOST_CHECK_EQUAL(ret.write(), "{\"txid\":\"2c13c6c490b5bc3c152554abead7751d9a56ce264020e45bec6b17d24bbb2073\",\"hash\":\"2c13c6c490b5bc3c152554abead7751d9a56ce264020e45bec6b17d24bbb2073\",\"size\":121,\"vsize\":121,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295}],\"vout\":[{\"value\":11.00000000,\"n\":0,\"type\":3,\"extype\":0,\"starttime\":0,\"stoptime\":0,\"reauthorize\":1,\"uniqueauthorize\":0,\"IPChash\":\"f393847c97508f24b772281deea475cd\",\"IPCTitle\":\"title\",\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");

	
	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	r = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":3,"
		"\"extype\":1,"
		"\"starttime\":2000,"
		"\"stoptime\":3000,"
		"\"reauthorize\":0,"
		"\"uniqueauthorize\":1,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"title\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":11}]");

	notsigned = r.get_str();
	r = CallRPC(std::string("signrawtransaction ") + notsigned + " " + prevout + " " + "[]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == false);

	r = CallRPC(std::string("signrawtransaction ") + notsigned + " " + prevout + " " + "[" + privkey1 + "," + privkey2 + "]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == true);
	BOOST_CHECK_EQUAL(r.write(), "{\"hex\":\"0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb4010000009200483045022100ec5f8cf27a8cee3d2383756d3aaf867591a7236a5b334b90d9d3388abd908916022060cf4dc1f7045ef04e4fe0804fe09934c6582135a345906dd487e1edcc69a0b50147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052aeffffffff0100ab90410000000003002201d0070000b80b0000000110cd75a4ee1d2872b7248f50977c8493f3057469746c6517a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000\",\"complete\":true}");

	notsigned = find_value(r.get_obj(), "hex").get_str();

	//decode
	BOOST_CHECK_NO_THROW(ret = CallRPC(std::string("decoderawtransaction") + " " + notsigned));
	BOOST_CHECK_EQUAL(ret.write(), "{\"txid\":\"4d6d3254186bbe59d7fa4213f3ce922755caa91f0de08508e3a14b5b6a5caa69\",\"hash\":\"4d6d3254186bbe59d7fa4213f3ce922755caa91f0de08508e3a14b5b6a5caa69\",\"size\":267,\"vsize\":267,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\",\"vout\":1,\"scriptSig\":{\"asm\":\"0 3045022100ec5f8cf27a8cee3d2383756d3aaf867591a7236a5b334b90d9d3388abd908916022060cf4dc1f7045ef04e4fe0804fe09934c6582135a345906dd487e1edcc69a0b5[ALL] 512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\",\"hex\":\"00483045022100ec5f8cf27a8cee3d2383756d3aaf867591a7236a5b334b90d9d3388abd908916022060cf4dc1f7045ef04e4fe0804fe09934c6582135a345906dd487e1edcc69a0b50147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"},\"sequence\":4294967295}],\"vout\":[{\"value\":11.00000000,\"n\":0,\"type\":3,\"extype\":1,\"starttime\":2000,\"stoptime\":3000,\"reauthorize\":0,\"uniqueauthorize\":1,\"IPChash\":\"f393847c97508f24b772281deea475cd\",\"IPCTitle\":\"title\",\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");

}

BOOST_AUTO_TEST_CASE(ipc_mutiple_ipTx)
{
	UniValue ret;
	std::string prevout;
	UniValue r;
	std::string privkey1 = "\"KzsXybp9jX64P5ekX1KUxRQ79Jht9uzW7LorgwE65i5rWACL6LQe\"";
	std::string privkey2 = "\"Kyhdf5LuKTRx4ge69ybABsiUAWjVRK4XGxAKk2FQLp2HjGMy87Z4\"";
	std::string notsigned;
	

	prevout =
		"[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":2,"
		"\"extype\":1,"
		"\"starttime\":0,"
		"\"stoptime\":0,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"testtitle\","
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12},"
		"{\"txtype\":2,"
		"\"extype\":1,"
		"\"starttime\":0,"
		"\"stoptime\":0,"
		"\"reauthorize\":1,"
		"\"uniqueauthorize\":0,"
		"\"IPChash\":\"f393847c97508f24b772281deea475cd\","
		"\"IPCTitle\":\"testtitle\","
		"\"address\":\"1GRi7C9Ku4ktJwzXUQKFoSdcQRTKYEdret\",\"value\":12}]"), std::runtime_error);


	

}

BOOST_AUTO_TEST_CASE(ipc_devoteTx)
{
	UniValue ret;
	std::string prevout;
	std::string lastresult;
	UniValue r;
	std::string privkey1 = "\"KzsXybp9jX64P5ekX1KUxRQ79Jht9uzW7LorgwE65i5rWACL6LQe\"";
	std::string privkey2 = "\"Kyhdf5LuKTRx4ge69ybABsiUAWjVRK4XGxAKk2FQLp2HjGMy87Z4\"";
	std::string notsigned;


	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":\"1\",\"devotevalue\":10000,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":-1,\"devotevalue\":10000,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":5,\"devotevalue\":10000,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":1,\"devotevalue\":\"1\",\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);


	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":1,\"devotevalue\":-1,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]"), std::runtime_error);



	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":1,\"devotevalue\":305419896,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]");
	lastresult = ret.getValStr();
	BOOST_CHECK_EQUAL(lastresult, "0200000001dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd0100000000ffffffff01008c8647000000000104785634120017a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000");

	BOOST_CHECK_NO_THROW(ret = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(ret.write(), "{\"txid\":\"2dc98acd7b83cbad6d1595f46d4fb7ce934f5a3512547f4f25adca15b2187b1e\",\"hash\":\"2dc98acd7b83cbad6d1595f46d4fb7ce934f5a3512547f4f25adca15b2187b1e\",\"size\":91,\"vsize\":91,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295}],\"vout\":[{\"value\":12.00000000,\"n\":0,\"type\":1,\"devotevalue\":305419896,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");

	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == false);

	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[" + privkey1 + "," + privkey2 + "]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == true);
	BOOST_CHECK_EQUAL(r.write(), "{\"hex\":\"0200000001dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd01000000910047304402207cf0f312b474e02caefab703740b4f4f8ee38c376655ed47e6278be6f0e3d0c802204dd13bfe92f160e823d02ba609d27f342d7ff3e36aac5c73f0bd05ac60e207a80147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052aeffffffff01008c8647000000000104785634120017a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000\",\"complete\":true}");
	lastresult = find_value(r.get_obj(), "hex").get_str();

	//decode
	BOOST_CHECK_NO_THROW(r = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(r.write(), "{\"txid\":\"35013474bb1a917fd92c8eb6edbf840d2f180b2edf828eab5bd1ad60c125db53\",\"hash\":\"35013474bb1a917fd92c8eb6edbf840d2f180b2edf828eab5bd1ad60c125db53\",\"size\":236,\"vsize\":236,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\",\"vout\":1,\"scriptSig\":{\"asm\":\"0 304402207cf0f312b474e02caefab703740b4f4f8ee38c376655ed47e6278be6f0e3d0c802204dd13bfe92f160e823d02ba609d27f342d7ff3e36aac5c73f0bd05ac60e207a8[ALL] 512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\",\"hex\":\"0047304402207cf0f312b474e02caefab703740b4f4f8ee38c376655ed47e6278be6f0e3d0c802204dd13bfe92f160e823d02ba609d27f342d7ff3e36aac5c73f0bd05ac60e207a80147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"},\"sequence\":4294967295}],\"vout\":[{\"value\":12.00000000,\"n\":0,\"type\":1,\"devotevalue\":305419896,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");


	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":1,\"devotevalue\":10000,\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\",\"value\":12}]");
	lastresult = ret.getValStr();
	BOOST_CHECK_EQUAL(ret.getValStr(), "0200000001dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd0100000000ffffffff01008c864700000000010210270017a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000");

	
	BOOST_CHECK_NO_THROW(ret = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(ret.write(), "{\"txid\":\"4909e751b56ec452a55a6dd1be3317db44431fd913718e0a2bcdc85e3fa0e22d\",\"hash\":\"4909e751b56ec452a55a6dd1be3317db44431fd913718e0a2bcdc85e3fa0e22d\",\"size\":89,\"vsize\":89,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295}],\"vout\":[{\"value\":12.00000000,\"n\":0,\"type\":1,\"devotevalue\":10000,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");

	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == false);

	r = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[" + privkey1 + "," + privkey2 + "]");
	BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == true);
	BOOST_CHECK_EQUAL(r.write(), "{\"hex\":\"0200000001dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd010000009100473044022071949d5002274ddf8efd5af9230f92e50ff15b3db8bad973691f12ecfb8f6e79022001117795d0d9c6df211106597f5a865f6a80f2ab6714a39aa3a8cbcd24d970c70147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052aeffffffff01008c864700000000010210270017a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000\",\"complete\":true}");
	lastresult = find_value(r.get_obj(), "hex").get_str();

	//decode
	BOOST_CHECK_NO_THROW(r = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(r.write(), "{\"txid\":\"4faec2e43711733701df7538dd7540bc776d26c962e9f394c00a4360648aa872\",\"hash\":\"4faec2e43711733701df7538dd7540bc776d26c962e9f394c00a4360648aa872\",\"size\":234,\"vsize\":234,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\",\"vout\":1,\"scriptSig\":{\"asm\":\"0 3044022071949d5002274ddf8efd5af9230f92e50ff15b3db8bad973691f12ecfb8f6e79022001117795d0d9c6df211106597f5a865f6a80f2ab6714a39aa3a8cbcd24d970c7[ALL] 512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\",\"hex\":\"00473044022071949d5002274ddf8efd5af9230f92e50ff15b3db8bad973691f12ecfb8f6e79022001117795d0d9c6df211106597f5a865f6a80f2ab6714a39aa3a8cbcd24d970c70147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"},\"sequence\":4294967295}],\"vout\":[{\"value\":12.00000000,\"n\":0,\"type\":1,\"devotevalue\":10000,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");


}

BOOST_AUTO_TEST_CASE(ipc_tokenreg)
{
	UniValue ret;
	std::string prevout;
	std::string lastresult;
	UniValue r;
	std::string privkey1 = "\"KzsXybp9jX64P5ekX1KUxRQ79Jht9uzW7LorgwE65i5rWACL6LQe\"";
	std::string privkey2 = "\"Kyhdf5LuKTRx4ge69ybABsiUAWjVRK4XGxAKk2FQLp2HjGMy87Z4\"";
	std::string notsigned;

	

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":\"4\","
		"\"TokenSymbol\":305419896,"
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"TestTokenLabel\","
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_NO_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":\"abcdefgh\","
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"TestTokenLabel\","
		"\"TokenIssue\":1501818090,"
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"));
	
	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":305419896,"
		"\"TokenHash\":f393847c97508f24b772281deea475cd,"
		"\"TokenLabel\":\"TestTokenLabel\","
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000,"
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":305419896,"
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":0000,"
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":305419896,"
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"0000\","
		"\"TokenIssue\":\"1501818090\"," 
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":305419896,"
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"0000\","
		"\"TokenIssue\":1501818090,"
		"\"TokenTotalCount\":100000w,"
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);


	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":\"34ipCdd\","
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"0000\","
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":\"34ipcdd\","
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"0000\","
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":\"34IPCdd\","
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"0000\","
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);
	
	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":\"bbcabcde\","
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"32aipcpc\","
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":\"bbcabcde\","
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"32aIPCpc\","
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":\"bbcabcde\","
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"32aIpcpc\","
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":\"bbcabcdef\","
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"32aIpcpc\","
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);

	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	BOOST_CHECK_THROW(CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":\"bbcabcde\","
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"01020304050607080\","
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]"), std::runtime_error);
	
	
	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":4,"
		"\"TokenSymbol\":\"abcdefgh\","
		"\"TokenHash\":\"f393847c97508f24b772281deea475cd\","
		"\"TokenLabel\":\"TestTokenLabel\","
		"\"TokenIssue\":1501818090," 
		"\"TokenTotalCount\":100000,"
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]");
	lastresult = ret.getValStr();
	BOOST_CHECK_EQUAL(lastresult, "0200000001dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd0100000000ffffffff01008c864700000000043c6162636465666768a086010000000000cd75a4ee1d2872b7248f50977c8493f354657374546f6b656e4c6162656c0000eaec8359a08601000000000017a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000");
	
	BOOST_CHECK_NO_THROW(ret = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(ret.write(), "{\"txid\":\"ef8a0b789fe3d0688f479cc6e0284dbabca3d8f082b3632c528af8052d7ec073\",\"hash\":\"ef8a0b789fe3d0688f479cc6e0284dbabca3d8f082b3632c528af8052d7ec073\",\"size\":146,\"vsize\":146,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295}],\"vout\":[{\"value\":12.00000000,\"n\":0,\"type\":4,\"TokenSymbol\":\"abcdefgh\",\"TokenValue\":100000,\"TokenHash\":\"f393847c97508f24b772281deea475cd\",\"TokenLabel\":\"TestTokenLabel\",\"TokenIssue\":1501818090,\"TokenTotalCount\":100000,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");
	
	ret = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[" + privkey1 + "," + privkey2 + "]");
	BOOST_CHECK(find_value(ret.get_obj(), "complete").get_bool() == true);

}

BOOST_AUTO_TEST_CASE(ipc_tokentx)
{
	UniValue ret;
	std::string prevout;
	std::string lastresult;
	UniValue r;
	std::string privkey1 = "\"KzsXybp9jX64P5ekX1KUxRQ79Jht9uzW7LorgwE65i5rWACL6LQe\"";
	std::string privkey2 = "\"Kyhdf5LuKTRx4ge69ybABsiUAWjVRK4XGxAKk2FQLp2HjGMy87Z4\"";
	std::string notsigned;

	
	prevout =
		"[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\","
		"\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
		"\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
	ret = CallRPC(std::string("createrawtransaction ") + prevout + " " +
		"[{\"txtype\":5,"
		"\"TokenSymbol\":\"abcdefgh\","
		"\"TokenValue\":100000," 
		"\"address\":\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\","
		"\"value\":12}]");
	lastresult = ret.getValStr();
	BOOST_CHECK_EQUAL(lastresult, "0200000001dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd0100000000ffffffff01008c86470000000005106162636465666768a08601000000000017a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000");

	BOOST_CHECK_NO_THROW(ret = CallRPC(std::string("decoderawtransaction") + " " + lastresult));
	BOOST_CHECK_EQUAL(ret.write(), "{\"txid\":\"acc9556b8937e1f7f73b3737dc998cddb3f699634c4443219127c68f5811b93e\",\"hash\":\"acc9556b8937e1f7f73b3737dc998cddb3f699634c4443219127c68f5811b93e\",\"size\":102,\"vsize\":102,\"version\":2,\"locktime\":0,\"vin\":[{\"txid\":\"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\",\"vout\":1,\"scriptSig\":{\"asm\":\"\",\"hex\":\"\"},\"sequence\":4294967295}],\"vout\":[{\"value\":12.00000000,\"n\":0,\"type\":5,\"TokenSymbol\":\"abcdefgh\",\"TokenValue\":100000,\"scriptPubKey\":{\"asm\":\"OP_HASH160 b10c9df5f7edf436c697f02f1efdba4cf3996151 OP_EQUAL\",\"hex\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\",\"reqSigs\":1,\"type\":\"scripthash\",\"addresses\":[\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\"]}}]}");

	ret = CallRPC(std::string("signrawtransaction ") + lastresult + " " + prevout + " " + "[" + privkey1 + "," + privkey2 + "]");
	BOOST_CHECK(find_value(ret.get_obj(), "complete").get_bool() == true);

}


BOOST_AUTO_TEST_CASE(ipc_TxSend)
{
	UniValue r;
	// Only check failure cases for sendrawtransaction, there's no network to send to...
	BOOST_CHECK_THROW(CallRPC("sendrawtransaction"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("sendrawtransaction null"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("sendrawtransaction DEADBEEF"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC(std::string("sendrawtransaction ") + "0200000001f393847c97508f24b772281deea475cd3e0f719f321794e5da7cf8587e28ccb4010000009200483045022100a1261a7abd6092b69ee34651b65d003ad3e1bab63ed8394581f0e2c8018ec6a502203d4a1396e076b5c79a26848d894c19146d8043ccb03ca589201f81e761ac74620147512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052aeffffffff0100ab904100000000020022010000000000000000000110cd75a4ee1d2872b7248f50977c8493f3057469746c6517a914b10c9df5f7edf436c697f02f1efdba4cf3996151870000000000" + " extra"), std::runtime_error);


}




BOOST_AUTO_TEST_CASE(rpc_rawparams)
{
	// Test raw transaction API argument handling
	UniValue r;

	BOOST_CHECK_THROW(CallRPC("getrawtransaction"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("getrawtransaction not_hex"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("getrawtransaction a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed not_int"), std::runtime_error);

	BOOST_CHECK_THROW(CallRPC("createrawtransaction"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("createrawtransaction null null"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("createrawtransaction not_array"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("createrawtransaction [] {}"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("createrawtransaction {} {}"), std::runtime_error);
	BOOST_CHECK_NO_THROW(CallRPC("createrawtransaction [] []"));
	BOOST_CHECK_THROW(CallRPC("createrawtransaction [] [] extra"), std::runtime_error);

	BOOST_CHECK_THROW(CallRPC("decoderawtransaction"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("decoderawtransaction null"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("decoderawtransaction DEADBEEF"), std::runtime_error);

	
	//std::string rawtx = "0100000001a15d57094aa7a21a28cb20b59aab8fc7d1149a3bdbcddba9c622e4f5f6a99ece010000006c493046022100f93bb0e7d8db7bd46e40132d1f8242026e045f03a0efe71bbb8e3f475e970d790221009337cd7f1f929f00cc6ff01f03729b069a7c21b59b1736ddfee5db5946c5da8c0121033b9b137ee87d5a812d6f506efdd37f0affa7ffc310711c06c7f3e097c9447c52ffffffff0100e1f505000000001976a9140389035a9225b3839e2bbf32d826a1e222031fd888ac00000000";
	std::string rawtx = "0100000001a15d57094aa7a21a28cb20b59aab8fc7d1149a3bdbcddba9c622e4f5f6a99ece010000006c493046022100f93bb0e7d8db7bd46e40132d1f8242026e045f03a0efe71bbb8e3f475e970d790221009337cd7f1f929f00cc6ff01f03729b069a7c21b59b1736ddfee5db5946c5da8c0121033b9b137ee87d5a812d6f506efdd37f0affa7ffc310711c06c7f3e097c9447c52ffffffff0100e1f505000000000000001976a9140389035a9225b3839e2bbf32d826a1e222031fd888ac0000000000";
	BOOST_CHECK_NO_THROW(r = CallRPC(std::string("decoderawtransaction ") + rawtx));
	//BOOST_CHECK_EQUAL(find_value(r.get_obj(), "size").get_int(), 193);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "size").get_int(), 197);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "version").get_int(), 1);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "locktime").get_int(), 0);
	BOOST_CHECK_THROW(r = CallRPC(std::string("decoderawtransaction ") + rawtx + " extra"), std::runtime_error);

	BOOST_CHECK_THROW(CallRPC("signrawtransaction"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("signrawtransaction null"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("signrawtransaction ff00"), std::runtime_error);
	BOOST_CHECK_NO_THROW(CallRPC(std::string("signrawtransaction ") + rawtx));
	BOOST_CHECK_NO_THROW(CallRPC(std::string("signrawtransaction ") + rawtx + " null null NONE|ANYONECANPAY"));
	BOOST_CHECK_NO_THROW(CallRPC(std::string("signrawtransaction ") + rawtx + " [] [] NONE|ANYONECANPAY"));
	BOOST_CHECK_THROW(CallRPC(std::string("signrawtransaction ") + rawtx + " null null badenum"), std::runtime_error);

	// Only check failure cases for sendrawtransaction, there's no network to send to...
	BOOST_CHECK_THROW(CallRPC("sendrawtransaction"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("sendrawtransaction null"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("sendrawtransaction DEADBEEF"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC(std::string("sendrawtransaction ") + rawtx + " extra"), std::runtime_error);
}
BOOST_AUTO_TEST_CASE(rpc_togglenetwork)
{
	UniValue r;

	r = CallRPC("getnetworkinfo");
	bool netState = find_value(r.get_obj(), "networkactive").get_bool();
	BOOST_CHECK_EQUAL(netState, true);

	BOOST_CHECK_NO_THROW(CallRPC("setnetworkactive false"));
	r = CallRPC("getnetworkinfo");
	int numConnection = find_value(r.get_obj(), "connections").get_int();
	BOOST_CHECK_EQUAL(numConnection, 0);

	netState = find_value(r.get_obj(), "networkactive").get_bool();
	BOOST_CHECK_EQUAL(netState, false);

	BOOST_CHECK_NO_THROW(CallRPC("setnetworkactive true"));
	r = CallRPC("getnetworkinfo");
	netState = find_value(r.get_obj(), "networkactive").get_bool();
	BOOST_CHECK_EQUAL(netState, true);
}
BOOST_AUTO_TEST_CASE(rpc_createraw_op_return)
{
	BOOST_CHECK_NO_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] [{\"data\":\"68656c6c6f776f726c64\"}]"));

	// Allow more than one data transaction output
	BOOST_CHECK_NO_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] [{\"data\":\"68656c6c6f776f726c64\",\"data\":\"68656c6c6f776f726c64\"}]"));

	// Key not "data" (bad address)
	BOOST_CHECK_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"somedata\":\"68656c6c6f776f726c64\"}"), std::runtime_error);

	// Bad hex encoding of data output
	BOOST_CHECK_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"data\":\"12345\"}"), std::runtime_error);
	BOOST_CHECK_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"data\":\"12345g\"}"), std::runtime_error);

	// Data 81 bytes long
	BOOST_CHECK_NO_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] [{\"data\":\"010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081\"}]"));
}

BOOST_AUTO_TEST_CASE(json_parse_errors)
{
	// Valid
	BOOST_CHECK_EQUAL(ParseNonRFCJSONValue("1.0").get_real(), 1.0);
	// Valid, with leading or trailing whitespace
	BOOST_CHECK_EQUAL(ParseNonRFCJSONValue(" 1.0").get_real(), 1.0);
	BOOST_CHECK_EQUAL(ParseNonRFCJSONValue("1.0 ").get_real(), 1.0);

	BOOST_CHECK_THROW(AmountFromValue(ParseNonRFCJSONValue(".19e-6")), std::runtime_error); //should fail, missing leading 0, therefore invalid JSON
	BOOST_CHECK_EQUAL(AmountFromValue(ParseNonRFCJSONValue("0.00000000000000000000000000000000000001e+30 ")), 1);
	// Invalid, initial garbage
	BOOST_CHECK_THROW(ParseNonRFCJSONValue("[1.0"), std::runtime_error);
	BOOST_CHECK_THROW(ParseNonRFCJSONValue("a1.0"), std::runtime_error);
	// Invalid, trailing garbage
	BOOST_CHECK_THROW(ParseNonRFCJSONValue("1.0sds"), std::runtime_error);
	BOOST_CHECK_THROW(ParseNonRFCJSONValue("1.0]"), std::runtime_error);
	// BTC addresses should fail parsing
	BOOST_CHECK_THROW(ParseNonRFCJSONValue("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"), std::runtime_error);
	BOOST_CHECK_THROW(ParseNonRFCJSONValue("3J98t1WpEZ73CNmQviecrnyiWrnqRhWNL"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
