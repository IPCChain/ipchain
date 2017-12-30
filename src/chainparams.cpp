// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

//#include "dpoc/TimeService.h"

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
	//txNew.vout.resize(1);
	//txNew.vout[0].nValue = genesisReward;
	//txNew.vout[0].scriptPubKey = genesisOutputScript;
	txNew.vout.clear();
	CTxOut genesisCoinbase(genesisReward, genesisOutputScript, "75hWHwYVdt265H8UkEdXJdfYofLkVELwSavQc7Nr8ZfxKt1VLSA2vNbSqfCiNL6heYzHqp41F7JT1UjiZhZ8Kp15DdVvKkFFw4yX4XfWHEHy7cYjCDiwpxVdatjRCobGEuWysGXNzoHyNfX61");
	txNew.vout.push_back(std::move(genesisCoinbase));

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;

	//-------------------add begin
	genesis.nPeriodCount = 0;
	genesis.nPeriodStartTime = 1507824000000;//2017年10月13日0时0分0秒
	genesis.nTimePeriod = 0;
	//-----------------add end

    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    //const char* pszTimestamp = "2017/08/04 For test network only";
	const char* pszTimestamp = "2017/10/13 Central bank tests digital currency transactions";
   // const CScript genesisOutputScript = CScript() << ParseHex("049bec86bdd7860fb433069eccf053be26673647ebb8a438ad08fbb7bf53bab39175130855704317dc22e7baccbdee7806f4d5f5fb3b5f21fc2063acbbd2594d00") << OP_CHECKSIG;
	const CScript genesisOutputScript = CScript() << ParseHex("03f774245f3f0f43e17d84b162d6212f064bb89381fd12cc82b11136d61f594e2a") << OP_CHECKSIG;
	return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        consensus.BIP65Height = 0; // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 0; // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;

		//add begin by li at 2017-12-12
        //consensus.fPowNoRetargeting = false;
		consensus.fPowNoRetargeting = true;
        //add end by li at 2017-12-12

        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
// 		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
// 		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 999999999999ULL; // May 1st, 2016
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
		consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 999999999999ULL; // November 15th, 2016.
		consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL; // November 15th, 2017.

        // The best chain should have at least this much work.
        //consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000003f94d1ad391682fe038bf5");
		consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

		//add begin by li at 2017-12-12
        // By default assume that the signatures in ancestors of this block are valid.
        //consensus.defaultAssumeValid = uint256S("0x00000000000000000013176bf8d7dfeab4e1db31dc93bc311b436e82ab226b90"); //453354
		//add begin by li at 2017-12-12
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
		//Version 0.6.3Update2
        pchMessageStart[0] = 0xea;
        pchMessageStart[1] = 0xe8;
        pchMessageStart[2] = 0xe7;
        pchMessageStart[3] = 0xdc;
        nDefaultPort = 15078;
        nPruneAfterHeight = 100000;

		genesis = CreateGenesisBlock(1507824000, 1861207187, 0x1d00ffff, 1, 30000000);

		////计算创世区块的代码？
		//{
		//	printf("Searching for genesis block...\n");
		//	uint256 hashTarget = uint256S("00000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
		//	uint256 thash;
		//
		//	while (true)
		//	{
		//		thash = genesis.GetHash();
		//		if (thash.NumberCompare(hashTarget) < 0)
		//			break;
		//
		//		if ((genesis.nNonce & 0xFFFFFF) == 0)
		//		{
		//			printf("nonce %08X: hash= %s(target=%s)\n", genesis.nNonce, thash.GetHex().c_str(), hashTarget.GetHex().c_str());
		//		}
		//		++genesis.nNonce;
		//		if (genesis.nNonce == 0)
		//		{
		//			printf("NONCE WRAPPED, incrementing time\n");
		//			++genesis.nTime;
		//		}
		//	}
		//	
		//	printf("genesis.nTime = %u\n", genesis.nTime);
		//	printf("genesis.nNonce = %u\n", genesis.nNonce);
		//	printf("genesis.nVersion = %u\n", genesis.nVersion);
		//	printf("genesis.getHash = %s\n", genesis.GetHash().GetHex().c_str());
		//	printf("genesis.hashMerkleRoot = %s\n", genesis.hashMerkleRoot.GetHex().c_str());
		//}


        consensus.hashGenesisBlock = genesis.GetHash();
		//assert(consensus.hashGenesisBlock == uint256S("000000002ca388dedc1a0be616c775e676b5fb8f348ebf727c4ba46bf9acbde7"));
		//assert(genesis.hashMerkleRoot == uint256S("6fb96b156b0fbd0bf23d00b9f31b9287d81eec0ce6f3f67426e9b9818160ac0d"));

        //// Note that of those with the service bits flag, most only support a subset of possible options
        //vSeeds.push_back(CDNSSeedData("bitcoin.sipa.be", "seed.bitcoin.sipa.be", true)); // Pieter Wuille, only supports x1, x5, x9, and xd
        //vSeeds.push_back(CDNSSeedData("bluematt.me", "dnsseed.bluematt.me", true)); // Matt Corallo, only supports x9
        //vSeeds.push_back(CDNSSeedData("dashjr.org", "dnsseed.bitcoin.dashjr.org")); // Luke Dashjr
        //vSeeds.push_back(CDNSSeedData("bitcoinstats.com", "seed.bitcoinstats.com", true)); // Christian Decker, supports x1 - xf
        //vSeeds.push_back(CDNSSeedData("bitcoin.jonasschnelli.ch", "seed.bitcoin.jonasschnelli.ch", true)); // Jonas Schnelli, only supports x1, x5, x9, and xd

		//IPC监管账号地址
		system_account_address = "ZCBSYSTEMZZZZZZZZZZZZZZZZZZZZZVp8TGB";
		MIN_DEPOSI = 10000 * COIN;
		CHECK_START_BLOCKCOUNT = 1;
		ADJUSTDP_BLOCKS = 2102400; //按第一次加入的押金计算的块数
		//base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
 		base58Prefixes[PUBKEY_ADDRESS] = boost::assign::list_of(0x04)(0x1b)(0x54).convert_to_container<std::vector<unsigned char> >(); //生成地址以ZCB打头
		//base58Prefixes[SECRET_KEY] = boost::assign::list_of(0x07)(0xc7)(0xcb).convert_to_container<std::vector<unsigned char> >(); //生成地址以PRV打头
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
				(0, uint256S("00000000092a99c04ceefc5e74dabd905ab1f65166f44bc5f603aa8d1a87ad63"))
        };

        chainTxData = ChainTxData{
            // Data as of block 00000000000000000166d612d5595e2b1cd88d71d695fc580af64d8da8658c23 (height 446482).
			1483472411, // * UNIX timestamp of last known number of transactions
            184495391,  // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            3.2         // * estimated number of transactions per second after that timestamp
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        consensus.BIP65Height = 0; // 00000000007f6655f22f98e72ed80d8b06dc761d5da09df0fa1dc4be4f861eb6
        consensus.BIP66Height = 0; // 000000002104c8c45e99a8853285a3b592602a3ccde2b832481da85e9e4ba182
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;

		//add begin by li at 2017-12-12
        //consensus.fPowAllowMinDifficultyBlocks = true;
		consensus.fPowAllowMinDifficultyBlocks = false;
		//consensus.fPowNoRetargeting = false;
		consensus.fPowNoRetargeting = true;
		//add end by li at 2017-12-12
        
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 999999999999ULL; // 
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL; // 

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
		consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 999999999999ULL; // 
		consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL; //

        // The best chain should have at least this much work.
       // consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000001f057509eba81aed91");
		consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

		//add begin by li at 2017-12-12
        // By default assume that the signatures in ancestors of this block are valid.
        //consensus.defaultAssumeValid = uint256S("0x00000000000128796ee387cf110ccb9d2f36cffaf7f73079c995377c65ac0dcc"); //1079274
		//add begin by li at 2017-12-12
																													   //add end by li at 2017-12-12
		 //Version 0.6.3Update2
		pchMessageStart[0] = 0x06;
		pchMessageStart[1] = 0x05;
		pchMessageStart[2] = 0x03;
		pchMessageStart[3] = 0x02;
        nDefaultPort = 25078;
        nPruneAfterHeight = 1000;

		genesis = CreateGenesisBlock(1507824000, 1861207187, 0x1d00ffff, 1, 30000000);
		consensus.hashGenesisBlock = genesis.GetHash();
	//	assert(consensus.hashGenesisBlock == uint256S("000000002ca388dedc1a0be616c775e676b5fb8f348ebf727c4ba46bf9acbde7"));
	//	assert(genesis.hashMerkleRoot == uint256S("6fb96b156b0fbd0bf23d00b9f31b9287d81eec0ce6f3f67426e9b9818160ac0d"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
       // vSeeds.push_back(CDNSSeedData("testnetbitcoin.jonasschnelli.ch", "testnet-seed.bitcoin.jonasschnelli.ch", true));
       // vSeeds.push_back(CDNSSeedData("petertodd.org", "seed.tbtc.petertodd.org", true));
       // vSeeds.push_back(CDNSSeedData("bluematt.me", "testnet-seed.bluematt.me"));
      //  vSeeds.push_back(CDNSSeedData("bitcoin.schildbach.de", "testnet-seed.bitcoin.schildbach.de"));
		vSeeds.push_back(CDNSSeedData("vps.qingdoutech.com", "seed.qingdoutech.com"));
		//测试网络IPC监管账号地址
		system_account_address = "TCBSYSTEMZZZZZZZZZZZZZZZZZZZZZUawnx4";
		MIN_DEPOSI = 20 * COIN;
		CHECK_START_BLOCKCOUNT = 120;
		ADJUSTDP_BLOCKS = 2102400; //按第一次加入的押金计算的块数
		//base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
		base58Prefixes[PUBKEY_ADDRESS] = boost::assign::list_of(0x03)(0x57)(0x62).convert_to_container<std::vector<unsigned char> >(); //生成地址以TCB打头（B的一位可能不太稳）
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
		//base58Prefixes[SECRET_KEY] = boost::assign::list_of(0x07)(0xc7)(0xcb).convert_to_container<std::vector<unsigned char> >(); //生成地址以PRV打头
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 546, uint256S("000000002a936ca763904c3c35fce2f3556c559c0214345d31b1bcebf76acb70")),
        };

        chainTxData = ChainTxData{
            // Data as of block 00000000c2872f8f8a8935c8e3c5862be9038c97d4de2cf37ed496991166928a (height 1063660)
            1483546230,
            12834668,
            0.15
        };

    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
		consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

		pchMessageStart[0] = 0x1c;
		pchMessageStart[1] = 0x2c;
		pchMessageStart[2] = 0x3c;
		pchMessageStart[3] = 0x4c;
        nDefaultPort = 19444;
        nPruneAfterHeight = 1000;

		genesis = CreateGenesisBlock(1501818090, 1861207187, 0x1d00ffff, 1, 96000000 * COIN);
		consensus.hashGenesisBlock = genesis.GetHash();
// 		assert(consensus.hashGenesisBlock == uint256S("000000002ca388dedc1a0be616c775e676b5fb8f348ebf727c4ba46bf9acbde7"));
// 		assert(genesis.hashMerkleRoot == uint256S("6fb96b156b0fbd0bf23d00b9f31b9287d81eec0ce6f3f67426e9b9818160ac0d"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"))
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        //base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
		base58Prefixes[PUBKEY_ADDRESS] = boost::assign::list_of(0x03)(0x57)(0x63).convert_to_container<std::vector<unsigned char> >(); //生成地址以TCB打头（B的一位可能不太稳）
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
		//base58Prefixes[SECRET_KEY] = boost::assign::list_of(0x07)(0xc7)(0xcb).convert_to_container<std::vector<unsigned char> >(); //生成地址以PRV打头
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();
    }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}
 
