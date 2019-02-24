#ifndef BITCOIN_ACCOUNTBALANCES_H
#define BITCOIN_ACCOUNTBALANCES_H

#include "base58.h"
#include "main.h"

using AccountBalances = std::map<CBitcoinAddress, int64>;


AccountBalances calculateAccountBalances();
std::string accountsToJson(const AccountBalances& balances);
void dumpAccounts(const std::string& dumpFile);

#endif