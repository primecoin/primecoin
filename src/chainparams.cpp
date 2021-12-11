// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>

#include <tinyformat.h>
#include <util.h>
#include <utilstrencodings.h>

#include <assert.h>
#include <memory>

#include <chainparamsseeds.h>

unsigned int TargetFromInteger(unsigned int nLength, unsigned int nFractionalBits)
{
    return (nLength << nFractionalBits);
}

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward, CBigNum bnPrimeChainMultiplier)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 0 << CScriptNum(999) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock block;
    block.nVersion = nVersion;
    block.nTime    = nTime;
    block.nBits    = nBits;
    block.nNonce   = nNonce;
    block.bnPrimeChainMultiplier = bnPrimeChainMultiplier;
	block.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    block.hashPrevBlock.SetNull();
    block.hashMerkleRoot = BlockMerkleRoot(block);
    
    return block;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=963d17ba4dc753138078a2f56afb3af9674e2546822badff26837db9a0152106, ver=0x00000002, hashPrevBlock=0000000000000000000000000000000000000000000000000000000000000000, hashMerkleRoot=aca30eb61dffbb9412d0ae743c3d74554f710853daec40ebd2514e830e05c9ff, nTime=1373064429, nBits=06000000, nNonce=383, vtx=1)
 *  CTransaction(hash=aca30eb61d, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *    CTxIn(COutPoint(0000000000, 4294967295), coinbase 0002e7034c5d53756e6e79204b696e67202d2064656469636174656420746f205361746f736869204e616b616d6f746f20616e6420616c6c2077686f206861766520666f7567687420666f72207468652066726565646f6d206f66206d616e6b696e64)
 *    CScriptWitness()
 *    CTxOut(nValue=1.00000000, scriptPubKey=)
 *
 * Prime::Origin=8965952996020407064364391577136065268670542909213664815741310568010946895662880601479081230
 *
 */

// We are using this to validate if the chain design does check out
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward, CBigNum bnPrimeChainMultiplier)
{
    const CScript genesisOutputScript = CScript();
    const char* pszTimestamp = "Sunny King - dedicated to Satoshi Nakamoto and all who have fought for the freedom of mankind";
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward, bnPrimeChainMultiplier);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
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
        consensus.BIP16Height = 0; // 963d17ba4dc753138078a2f56afb3af9674e2546822badff26837db9a0152106
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x963d17ba4dc753138078a2f56afb3af9674e2546822badff26837db9a0152106");
        consensus.BIP65Height = 99999999;
        consensus.BIP66Height = 99999999;
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 7 * 24 * 60 * 60; // a week
        consensus.nPowTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.nTargetInitialLength = 7;
        consensus.nTargetMinLength = 6;
        
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1479168000; // November 15th, 2016.
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1510704000; // November 15th, 2017.

        // The best chain should have at least this much work.
        // consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000003f94d1ad391682fe038bf5");

        // By default assume that the signatures in ancestors of this block are valid.
        // consensus.defaultAssumeValid = uint256S("0x00000000000000000013176bf8d7dfeab4e1db31dc93bc311b436e82ab226b90"); //453354

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xe4;
        pchMessageStart[1] = 0xe7;
        pchMessageStart[2] = 0xe5;
        pchMessageStart[3] = 0xe7;
        nDefaultPort = 9911;
        nPruneAfterHeight = 100000;

        const CBigNum bnPrimeChainMultiplier = ((uint64_t) 532541) * (uint64_t)(2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23);
        genesis = CreateGenesisBlock(1373064429, 383, TargetFromInteger(6, 24), 2, COIN, bnPrimeChainMultiplier);
        consensus.hashGenesisBlock = genesis.GetHash();
        
        assert(consensus.hashGenesisBlock 	== uint256S("0x963d17ba4dc753138078a2f56afb3af9674e2546822badff26837db9a0152106"));
        assert(genesis.hashMerkleRoot 		== uint256S("0xaca30eb61dffbb9412d0ae743c3d74554f710853daec40ebd2514e830e05c9ff"));

        vSeeds.emplace_back("seed.primecoin.info");
        vSeeds.emplace_back("primeseed.muuttuja.org");
        vSeeds.emplace_back("seed.primecoin.org");
        vSeeds.emplace_back("xpm.dnsseed.coinsforall.io");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,23);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,83);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,151);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "pm";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = { 
			{
                {  854866, uint256S("0xb709d14ce9e24a044cbe6182c3ae05312b88291446c688bdce13207d4778bdb3")},
                { 1617271, uint256S("0x54b0c41a8aae8d8ceebc4e2366b577f34c4ad7a1a629d2e8ce9d721cd4d704b1")},
                { 2592901, uint256S("0xde07ddf776c3e5bbee0f4e67ba2802934a9152f79421273a0b682ab2d4162c05")},
                { 2908166, uint256S("0xeee0c0cec4e5d7395bdd2fa36a2b804ac75bcbff0ddc550a86d47ec1890b0694")},
                { 3685413, uint256S("0x47519676a8175884cd87db091e42c3a9a66fa566a368d3ba1897cd2cab5a5e5f")}
			}
		};

        chainTxData = ChainTxData {
            // Data as of block 0x47519676a8175884cd87db091e42c3a9a66fa566a368d3ba1897cd2cab5a5e5f (height 3685413).
            1589479273, // * UNIX timestamp of last known number of transactions
            7202526,  // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            0.03         // * estimated number of transactions per second after that timestamp
        };

        // Deployment of upgrade fee rule, destroy fee
        consensus.RFC2Height = 99999999; // to be determined
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 0; // 221156cf301bc3585e72de34fe1efdb6fbd703bc27cfc468faa1cdd889d0efa0
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x221156cf301bc3585e72de34fe1efdb6fbd703bc27cfc468faa1cdd889d0efa0");
        consensus.BIP65Height = 3058199; // approximate December 26, 2021
        consensus.BIP66Height = 3058199;
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 7 * 24 * 60 * 60; // a week
        consensus.nPowTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.nTargetInitialLength = 5;
        consensus.nTargetMinLength = 2;
        
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1635714336; // October 31, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1637884800; // November 26, 2021

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 4070908800; // January 1st 2099
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 4102444799; // December 31 2099

        // The best chain should have at least this much work.
        // consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000002830dab7f76dbb7d63");

        // By default assume that the signatures in ancestors of this block are valid.
        // consensus.defaultAssumeValid = uint256S("0x0000000002e9e7b00e1f6dc5123a04aad68dd0f0968d8c7aa45f6640795c37b1"); //1135275

        pchMessageStart[0] = 0xfb;
        pchMessageStart[1] = 0xfe;
        pchMessageStart[2] = 0xcb;
        pchMessageStart[3] = 0xc3;
        nDefaultPort = 9913;
        nPruneAfterHeight = 1000;

        const CBigNum bnPrimeChainMultiplier = ((uint64_t) 585641) * (uint64_t)(2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23);
        genesis = CreateGenesisBlock(1373063882, 1513, TargetFromInteger(6, 24), 2, COIN, bnPrimeChainMultiplier);
        consensus.hashGenesisBlock = genesis.GetHash();
        
        assert(consensus.hashGenesisBlock == uint256S("0x221156cf301bc3585e72de34fe1efdb6fbd703bc27cfc468faa1cdd889d0efa0"));
        assert(genesis.hashMerkleRoot == uint256S("0xaca30eb61dffbb9412d0ae743c3d74554f710853daec40ebd2514e830e05c9ff"));

        vSeeds.emplace_back("testseed.primecoin.info");
        vSeeds.emplace_back("primeseedtn.muuttuja.org");
        vSeeds.emplace_back("seed.testnet.primecoin.org");
        vSeeds.emplace_back("xpmtestnet.dnsseed.coinsforall.io");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tb";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


        checkpointData = {
            {
                { 1442707, uint256S("0xda3f7eb0c590bdbb806fc0f494d15e7d6f1f7d2385a1a63908fc3a9bbaa59ba2")}
            }
        };

        chainTxData = ChainTxData{
            // Data as of block 0xda3f7eb0c590bdbb806fc0f494d15e7d6f1f7d2385a1a63908fc3a9bbaa59ba2 (height 1442707)
            1517811562,
            1484979,
            0.01
        };

        // Deployment of upgrade fee rule, destroy fee
        consensus.RFC2Height = 3058199; // approximate December 26, 2021

    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Height = 0; // always enforce P2SH BIP16 on regtest
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 7 * 24 * 60 * 60; // week
        consensus.nPowTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.nTargetInitialLength = 5;
        consensus.nTargetMinLength = 2;
        
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 9915;
        nPruneAfterHeight = 1000;

        const CBigNum bnPrimeChainMultiplier = ((uint64_t) 585641) * (uint64_t)(2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23);
        genesis = CreateGenesisBlock(1296688602, 2, 0x207fffff, 1, 50 * COIN, bnPrimeChainMultiplier);
        consensus.hashGenesisBlock = genesis.GetHash();
        // assert(consensus.hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
        // assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = {
            {
                {0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "bcrt";
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}
