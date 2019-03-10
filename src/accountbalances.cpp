#include "accountbalances.h"

#include <iostream>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

void ensureExists(const CBitcoinAddress& address, AccountBalances& accountBalances)
{
    if (accountBalances.count(address) == 0)
    {
        accountBalances[address] = 0;
    }
}

BalanceData calculateAccountBalances()
{
    AccountBalances accountBalances;
    int height = 0;
    for (std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi)
    {
        CBlockIndex* pindex = mi->second;
        height = pindex->nHeight;
        CBlock block;
        block.ReadFromDisk(pindex);
        for (const auto& transaction : block.vtx)
        {
            mempool.addUnchecked(transaction.GetHash(), transaction);
            for(const auto& in : transaction.vin)
            {
                if(mempool.exists(in.prevout.hash) == 0)
                {
                    std::cout << "Could not find transaction with hash " << in.prevout.hash.ToString() << "\n";
                    continue;
                }
                std::cout << "Found transaction with hash " << in.prevout.hash.ToString() << "\n";
                const auto& outTransaction = mempool.lookup(in.prevout.hash);
                const auto& out = outTransaction.vout[in.prevout.n];
                CTxDestination destinationAddress;
                ExtractDestination(out.scriptPubKey, destinationAddress);
                CBitcoinAddress address(destinationAddress);
                ensureExists(address, accountBalances);
                accountBalances[address] -= out.nValue;
            }
            for(const auto& out : transaction.vout)
            {
                CTxDestination destinationAddress;
                ExtractDestination(out.scriptPubKey, destinationAddress);
                CBitcoinAddress address(destinationAddress);
                ensureExists(address, accountBalances);
                accountBalances[address] += out.nValue;
            }
        }
    }
    return BalanceData(height, accountBalances);
}

std::string accountsToJson(const BalanceData& balanceData)
{
    using boost::property_tree::ptree;
    using boost::property_tree::write_json;
    ptree balanceTree;
    for (auto& balance : balanceData.balances)
        balanceTree.put(balance.first.ToString(), balance.second);
    ptree dataTree;
    dataTree.put("height", balanceData.height);
    dataTree.add_child("balances", balanceTree);
    std::ostringstream buf; 
    write_json(buf, dataTree);
    return buf.str();
}

void dumpAccounts(const std::string& dumpFile)
{
    auto accountBalances = calculateAccountBalances();
    
    std::ofstream output;
    output.open (dumpFile);
    output << accountsToJson(accountBalances);
    output.close();
}
