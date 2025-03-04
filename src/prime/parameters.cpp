// Copyright (c) 2017 Ahmad A Kazi (Empinel/Plaxton) 
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>

#include "chainparams.h"
#include "util.h"
#include "streams.h"
#include "parameters.h"

using namespace std;

PrimeCoin prime;

// Get block work value for main chain protocol
uint256 PrimeCoin::GetPrimeBlockProof(const CBlockIndex& block, const Consensus::Params& consensus_params)
{
    // Primecoin: 
    // Difficulty multiplier of extra prime is estimated by nWorkTransitionRatio
    // Difficulty multiplier of fractional is estimated by
    //   r = 1/TransitionRatio
    //   length >= n discovery rate = 1
    //   length > n discovery rate = 1/TransitionRatio
    //   length == n discovery rate: 1 - 1/TransitionRatio
    //   meeting target rate 1/FractionalDiff * (1 - 1/TransitionRatio) + 1/TranstionRatio
    //   fractionalDiff = nFractionalDiffculty / nFractionalDifficultyMin
    //   fractional multiplier = 1 / meeting target rate
    //       = (TransitionRatio * FractionalDiff) / (TransitionRatio - 1 + FractionalDiff)
    uint64_t nFractionalDifficulty = TargetGetFractionalDifficulty(block.nBits);
    CBigNum bnWork = 1;

    for (unsigned int nCount = 0; nCount < TargetGetLength(block.nBits); nCount++)
        bnWork *= nWorkTransitionRatio;

    bnWork *= ((uint64_t) nWorkTransitionRatio) * nFractionalDifficulty;
    bnWork /= (((uint64_t) nWorkTransitionRatio - 1) * nFractionalDifficultyMin + nFractionalDifficulty);

    return bnWork.getuint256();
}

unsigned int PrimeCoin::GetPrimeWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& consensus_params)
{
    int64_t nTargetTimespan = consensus_params.nPowTargetTimespan;
    int64_t nTargetSpacing = consensus_params.nPowTargetSpacing;
    unsigned int nBits = TargetGetLimit(consensus_params);

    // Genesis block
    if (pindexLast == NULL)
        return nBits;

    const CBlockIndex* pindexPrev = pindexLast;

    if (pindexPrev->pprev == NULL)
        return TargetGetInitial(consensus_params); // first block

    const CBlockIndex* pindexPrevPrev = pindexPrev->pprev;

    if (pindexPrevPrev->pprev == NULL)
        return TargetGetInitial(consensus_params); // second block

    // Bitcoin: continuous target adjustment on every block
    int64_t nInterval = nTargetTimespan / nTargetSpacing;
    int64_t nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();
    if (!TargetGetNext(pindexPrev->nBits, nInterval, nTargetSpacing, nActualSpacing, nBits, consensus_params))
        return error("GetNextWorkRequired() : failed to get next target");

    return nBits;
}

bool PrimeCoin::CheckPrimeProofs(uint256 hashBlockHeader, unsigned int nBits, const CBigNum& bnProbablePrime, unsigned int& nChainType, unsigned int& nChainLength, const Consensus::Params& params)
{
    if (!CheckPrimeProofOfWork(hashBlockHeader, nBits, bnProbablePrime, nChainType, nChainLength, params))
        return error("CheckProofOfWork() : check failed for prime proof-of-work");

    return true;
}

CAmount PrimeCoin::GetPrimeBlockValue(int nBits, const Consensus::Params& consensus_params)
{
    uint64_t nSubsidy = 0;

    if (!TargetGetMint(nBits, nSubsidy, consensus_params))
        error("GetBlockValue() : invalid mint value");

    return ((CAmount)nSubsidy);
}

/**
void static PrimeCoin::PrimeMiner(CWallet *pwallet)
{
    LogPrintf("BitcoinMiner started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("bitcoin-miner");

    // Bitcoin miner
    if (pminer.get() == NULL)
        pminer.reset(new CPrimeMiner()); // init miner control object

    // Each thread has its own key and counter
    CReserveKey reservekey(pwallet);
    unsigned int nExtraNonce = 0;

    double dTimeExpected = 0;   // time expected to prime chain (micro-second)
    double dTimeExpectedPrev = 0; // time expected to prime chain last time
    bool fIncrementPrimorial = true; // increase or decrease primorial factor

    try { 
        while(true) {

            if (Params().MiningRequiresPeers()) {
                // Busy-wait for the network to come online so we don't waste time mining
                // on an obsolete chain. In regtest mode we expect to fly solo.
                do {
                    bool fvNodesEmpty;
                    {
                        LOCK(cs_vNodes);
                        fvNodesEmpty = vNodes.empty();
                    }
                    if (!fvNodesEmpty && !IsInitialBlockDownload())
                        break;
                    MilliSleep(1000);
                } while (true);
            }

            //
            // Create new block
            //
            unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
            CBlockIndex* pindexPrev = chainActive.Tip();

            auto_ptr<CBlockTemplate> pblocktemplate(CreateNewBlockWithKey(reservekey));
            
            if (!pblocktemplate.get())
                return;

            CBlock *pblock = &pblocktemplate->block;
            IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

            if (fDebug && GetBoolArg("-printmining", true))
                LogPrintf("Running BitcoinMiner with %u transactions in block (%u bytes)\n", pblock->vtx.size(),
                   ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION));

            //
            // Search
            //
            int64_t nStart = GetTime();
            bool fNewBlock = true;
            unsigned int nTriedMultiplier = 0;

            // Bitcoin: try to find hash that is probable prime
            do
            {
                uint256 hashBlockHeader = pblock->GetHeaderHash();
                if (hashBlockHeader < hashBlockHeaderLimit)
                    continue; // must meet minimum requirement
                if (ProbablePrimalityTestWithTrialDivision(CBigNum(hashBlockHeader), 1000))
                    break;
            } while (++(pblock->nNonce) < 0xffff0000);
            
            if (pblock->nNonce >= 0xffff0000)
                continue;

            // Bitcoin: primorial fixed multiplier
            CBigNum bnPrimorial;
            unsigned int nRoundTests = 0;
            unsigned int nRoundPrimesHit = 0;
            int64_t nPrimeTimerStart = GetTimeMicros();
            Primorial(pminer->nPrimorialMultiplier, bnPrimorial);

            while (true)
            {
                unsigned int nTests = 0;
                unsigned int nPrimesHit = 0;

                // Bitcoin: mine for prime chain
                unsigned int nProbableChainLength;
                if (MineProbablePrimeChain(*pblock, bnPrimorial, fNewBlock, nTriedMultiplier, nProbableChainLength, nTests, nPrimesHit))
                {
                    SetThreadPriority(THREAD_PRIORITY_NORMAL);
                    // CheckWork(pblock, *pwalletMain, reservekey);
                    ProcessBlockFound(pblock, *pwallet, reservekey);
                    SetThreadPriority(THREAD_PRIORITY_LOWEST);
                    break;
                }

                nRoundTests += nTests;
                nRoundPrimesHit += nPrimesHit;

                // Meter primes/sec
                static int64_t nPrimeCounter;
                static int64_t nSieveCounter;
                static int64_t nTestCounter;
                static double dChainExpected;

                if (nHPSTimerStart == 0)
                {
                    nHPSTimerStart = GetTimeMillis();
                    nPrimeCounter = 0;
                    nSieveCounter = 0;
                    nTestCounter = 0;
                    dChainExpected = 0;
                }
                else
                {
                    nPrimeCounter += nPrimesHit;
                    nTestCounter += nTests;
                    if (fNewBlock)
                        nSieveCounter++;
                }
                if (GetTimeMillis() - nHPSTimerStart > 60000)
                {
                    static CCriticalSection cs;
                    {
                        LOCK(cs);
                        if (GetTimeMillis() - nHPSTimerStart > 60000)
                        {
                            double dPrimesPerMinute = 60000.0 * nPrimeCounter / (GetTimeMillis() - nHPSTimerStart);
                            dPrimesPerSec = dPrimesPerMinute / 60.0;
                            double dTestsPerMinute = 60000.0 * nTestCounter / (GetTimeMillis() - nHPSTimerStart);
                            double dSievesPerHour = 3600000.0 * nSieveCounter / (GetTimeMillis() - nHPSTimerStart);
                            dChainsPerDay = 86400000.0 * dChainExpected / (GetTimeMillis() - nHPSTimerStart);
                            nHPSTimerStart = GetTimeMillis();
                            nPrimeCounter = 0;
                            nSieveCounter = 0;
                            nTestCounter = 0;
                            dChainExpected = 0;
                            static int64_t nLogTime = 0;
                            if (GetTime() - nLogTime > 60)
                            {
                                nLogTime = GetTime();
                                LogPrintf("%s primemeter %9.0f prime/h %9.0f test/h %6.0f sieve/h %3.6f chain/d\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nLogTime).c_str(), dPrimesPerMinute * 60.0, dTestsPerMinute * 60.0, dSievesPerHour, dChainsPerDay);
                            }
                        }
                    }
                }

                // Check for stop or if block needs to be rebuilt
                boost::this_thread::interruption_point();

                if (vNodes.empty())
                    break;
                if (pblock->nNonce >= 0xffff0000)
                    break;
                if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 10)
                    break;
                if (pindexPrev != chainActive.Tip())
                    break;

                if (fNewBlock)
                {
                    // Bitcoin: a sieve+primality round completes
                    // Bitcoin: estimate time to block
                    int64_t nRoundTime = (GetTimeMicros() - nPrimeTimerStart); 
                    dTimeExpected = (double) nRoundTime / max(1u, nRoundTests);
                    double dRoundChainExpected = (double) nRoundTests;
                    double dPrimeProbability = EstimateCandidatePrimeProbability();
                    for (unsigned int n = 0; n < TargetGetLength(pblock->nBits); n++)
                    {
                        dTimeExpected = dTimeExpected / max(0.01, dPrimeProbability);
                        dRoundChainExpected *= dPrimeProbability;
                    }

                    dChainExpected += dRoundChainExpected;
                    LogPrintf("BitcoinMiner() : Round primorial=%u tests=%u primes=%u time=%uus pprob=%1.6f tochain=%6.3fd expect=%3.9f\n", pminer->nPrimorialMultiplier, nRoundTests, nRoundPrimesHit, (unsigned int) nRoundTime, dPrimeProbability, ((dTimeExpected/1000000.0))/86400.0, dRoundChainExpected);

                    // Bitcoin: update time and nonce
                    pblock->nTime = max(pblock->nTime, (unsigned int) GetAdjustedTime());
                    while (++(pblock->nNonce) < 0xffff0000)
                    {
                        uint256 hashBlockHeader = pblock->GetHeaderHash();

                        if (hashBlockHeader < hashBlockHeaderLimit)
                            continue; // must meet minimum requirement
                        if (ProbablePrimalityTestWithTrialDivision(CBigNum(hashBlockHeader), 1000))
                            break;
                    }
                    if (pblock->nNonce >= 0xffff0000)
                        break;

                    // Bitcoin: reset sieve+primality round timer
                    nRoundTests = 0;
                    nRoundPrimesHit = 0;
                    nPrimeTimerStart = GetTimeMicros();
                    if (dTimeExpected > dTimeExpectedPrev)
                        fIncrementPrimorial = !fIncrementPrimorial;
                    dTimeExpectedPrev = dTimeExpected;

                    // Bitcoin: dynamic adjustment of primorial multiplier
                    if (fIncrementPrimorial)
                    {
                        if (!PrimeTableGetNextPrime(pminer->nPrimorialMultiplier))
                            error("BitcoinMiner() : primorial increment overflow");
                    }
                    else if (pminer->nPrimorialMultiplier > nPrimorialMultiplierMin)
                    {
                        if (!PrimeTableGetPreviousPrime(pminer->nPrimorialMultiplier))
                            error("BitcoinMiner() : primorial decrement overflow");
                    }

                    Primorial(pminer->nPrimorialMultiplier, bnPrimorial);
                }
            }
        } 
    }
    catch (boost::thread_interrupted)
    {
        LogPrintf("BitcoinMiner terminated\n");
        throw;
    }
}

void PrimeCoin::GenerateBitcoins(bool fGenerate, CWallet* pwallet, int nThreads)
{
    static boost::thread_group* minerThreads = NULL;

    if (nThreads < 0) {
        // In regtest threads defaults to 1
        if (Params().DefaultMinerThreads())
            nThreads = Params().DefaultMinerThreads();
        else
            nThreads = boost::thread::hardware_concurrency();
    }

    if (minerThreads != NULL)
    {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = NULL;
    }

    if (nThreads == 0 || !fGenerate)
        return;

    minerThreads = new boost::thread_group();
    for (int i = 0; i < nThreads; i++)
        minerThreads->create_thread(boost::bind(&PrimeMiner, pwallet));
}
**/
