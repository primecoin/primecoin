#include <boost/test/unit_test.hpp>
#include <iostream>

#include "accountbalances.h"
#include "base58.h"
#include "prime.h"

BOOST_AUTO_TEST_SUITE(accountbalances_tests)

BOOST_AUTO_TEST_CASE(accountbalances_testnoaccounts)
{
    mapBlockIndex.clear();
    auto balances = calculateAccountBalances();

    BOOST_CHECK(balances.size() == 0);
}

BOOST_AUTO_TEST_CASE(accountbalances_testoneaccount)
{
    const char* pszDedication = "Block with single transaction";

    RandAddSeedPerfmon();
    CKey key;
    key.MakeNewKey(true);

    CBitcoinAddress account1(key.GetPubKey().GetID());
    CScript script1;
    script1.SetDestination(account1.Get());

    CTransaction txNew;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 0 << CBigNum(999) << std::vector<unsigned char>((const unsigned char*)pszDedication, (const unsigned char*)pszDedication + strlen(pszDedication));
    txNew.vout[0].nValue = COIN;
    txNew.vout[0].scriptPubKey = script1;
    CBlock block;
    block.vtx.push_back(txNew);
    block.hashPrevBlock = 0;
    block.hashMerkleRoot = block.BuildMerkleTree();
    block.nTime    = 1373064429;
    block.nBits    = TargetFromInt(6);
    block.nNonce   = 383;
    block.bnPrimeChainMultiplier = ((uint64) 532541) * (uint64)(2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23);
    unsigned int nBlockSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);
    CDiskBlockPos blockPos;
    CValidationState state;
    mapBlockIndex.clear();
    blockPos.nFile = 1;
    blockPos.nPos = 0;
    mapArgs.insert(std::make_pair("-datadir", "tmp/something"));
    BOOST_CHECK(block.WriteToDisk(blockPos));
    BOOST_CHECK(block.AddToBlockIndex(state, blockPos));
    BOOST_CHECK(mapBlockIndex.size() == 1);
    auto balances = calculateAccountBalances();
    BOOST_CHECK(balances.size() == 1);
    BOOST_CHECK(balances[account1] == COIN);
}

BOOST_AUTO_TEST_CASE(accountbalances_testmultipletransactions)
{
    const char* pszDedication = "Block with two transactions";
    RandAddSeedPerfmon();
    CKey key1;
    key1.MakeNewKey(true);
    CKey key2;
    key2.MakeNewKey(true);

    CBitcoinAddress account1(key1.GetPubKey().GetID());
    CBitcoinAddress account2(key2.GetPubKey().GetID());
    CScript script1;
    script1.SetDestination(account1.Get());
    CScript script2;
    script2.SetDestination(account2.Get());


    CTransaction txNew;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 0 << CBigNum(999) << std::vector<unsigned char>((const unsigned char*)pszDedication, (const unsigned char*)pszDedication + strlen(pszDedication));
    txNew.vout[0].nValue = COIN;
    txNew.vout[0].scriptPubKey = script1;
    CTransaction txNext;
    txNext.vin.resize(1);
    txNext.vout.resize(1);
    txNext.vin[0].prevout.hash = txNew.GetHash();
    txNext.vin[0].prevout.n = 0;
    txNext.vout[0].nValue = COIN;
    txNext.vout[0].scriptPubKey = script2;
    CBlock block;
    block.vtx.push_back(txNew);
    block.vtx.push_back(txNext);
    block.hashPrevBlock = 0;
    block.hashMerkleRoot = block.BuildMerkleTree();
    block.nTime    = 1373064429;
    block.nBits    = TargetFromInt(6);
    block.nNonce   = 383;
    block.bnPrimeChainMultiplier = ((uint64) 532541) * (uint64)(2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23);
    unsigned int nBlockSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);
    CDiskBlockPos blockPos;
    CValidationState state;
    mapBlockIndex.clear();
    blockPos.nFile = 1;
    blockPos.nPos = 0;
    BOOST_CHECK(block.WriteToDisk(blockPos));
    BOOST_CHECK(block.AddToBlockIndex(state, blockPos));
    BOOST_CHECK(mapBlockIndex.size() == 1);
    auto balances = calculateAccountBalances();
    BOOST_CHECK(balances.size() == 2);
    BOOST_CHECK(mempool.mapTx.size() == 2);

    BOOST_CHECK(balances[account1] == 0);
    BOOST_CHECK(balances[account2] == COIN);
}

BOOST_AUTO_TEST_CASE(accountbalances_balances_to_json)
{
    CBitcoinAddress account1("AR3HX837aSBmnDBFJbcHeMHanTC8CNfouc");

    AccountBalances balances;
    balances.insert(std::make_pair(account1, 10000));
    auto output = accountsToJson(balances);
    std::string expected = "{\n"
                           "    \"AR3HX837aSBmnDBFJbcHeMHanTC8CNfouc\": \"10000\"\n"
                           "}\n";
    BOOST_CHECK(output == expected);
}

BOOST_AUTO_TEST_SUITE_END()
