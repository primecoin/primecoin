#ifndef BITCOIN_ACCOUNTBALANCES_H
#define BITCOIN_ACCOUNTBALANCES_H

#include "base58.h"
#include "main.h"

using AccountBalances = std::map<CBitcoinAddress, int64>;

struct BalanceData {
    int height;
    AccountBalances balances;

    BalanceData(const int& height, const AccountBalances& balances)
        :height(height), balances(balances)
    {}
};

BalanceData calculateAccountBalances();
std::string accountsToJson(const BalanceData& balances);
void dumpAccounts(const std::string& dumpFile);

#endif