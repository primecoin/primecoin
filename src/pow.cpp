// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include "prime/parameters.h"

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    return prime.GetPrimeWorkRequired(pindexLast, pblock, params);
}

bool CheckProofOfWork(uint256 hashBlockHeader, unsigned int nBits, const CBigNum& bnPrimeChainMultiplier, unsigned int& nChainType, unsigned int& nChainLength, const Consensus::Params& consensus_params)
{
    if (!CheckPrimeProofOfWork(hashBlockHeader, nBits, bnPrimeChainMultiplier, nChainType, nChainLength, consensus_params))
        return error("CheckProofOfWork() : check failed for prime proof-of-work");
    
    return true;
}
