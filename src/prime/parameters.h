// Copyright (c) 2017 Ahmad A Kazi (Empinel/Plaxton) 
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIME_PARAMETERS_H
#define PRIME_PARAMETERS_H

#include "prime.h"
#include "bignum.h"

#include <string>

class PrimeCoin {

private:
	int64_t forkFromPrime = 0;
	std::string currencyName = "Primecoin";
	std::string tickerName = "XPM";

public:
	uint256 GetPrimeBlockProof(const CBlockIndex& block, const Consensus::Params& consensus_params);
	unsigned int GetPrimeWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& consensus_params);
	bool CheckPrimeProofs(uint256 hashBlockHeader, unsigned int nBits, const CBigNum& bnProbablePrime, unsigned int& nChainType, unsigned int& nChainLength, const Consensus::Params& params);
	CAmount GetPrimeBlockValue(int nBits, const Consensus::Params& consensus_params);
/*
	void static PrimeMiner(CWallet *pwallet);
	void GenerateBitcoins(bool fGenerate, CWallet* pwallet, int nThreads);
*/
	std::string getCurrencyName() { return currencyName; }
	std::string getSmallCurrencyName() { return currencyName; }
	std::string getLargeCurrencyName() { return currencyName; }
	std::string getTickerName() { return tickerName; }
	int64_t forkTime() { return forkFromPrime; }

};

extern PrimeCoin prime;

#endif // PRIME_PARAMETERS_H
