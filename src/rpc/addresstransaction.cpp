#include <base58.h>
#include <chain.h>
#include <coins.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <init.h>
#include <keystore.h>
#include <validation.h>
#include <addressindex.h>
#include <validationinterface.h>
#include <merkleblock.h>
#include <net.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <primitives/transaction.h>
#include <rpc/safemode.h>
#include <rpc/server.h>
#include <rpc/addresstransaction.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/standard.h>
#include <txmempool.h>
#include <uint256.h>
#include <utilstrencodings.h>
#ifdef ENABLE_WALLET
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#endif

#include <future>
#include <stdint.h>

#include <univalue.h>


extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry);

//UniValue searchrawtransactions(const UniValue& params, bool fHelp)
UniValue searchrawtransactions(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 4)
        throw std::runtime_error("searchrawtransactions <address> [verbose=1] [skip=0] [count=100]\n");

    if (!fAddrIndex || !fTxIndex)
        throw JSONRPCError(RPC_MISC_ERROR, "Address index or tx index not enabled");
	
	CTxDestination destination = DecodeDestination(request.params[0].get_str());
    if (!IsValidDestination(destination)) {
        throw std::runtime_error("invalid TX output address");
    }

    std::set<CExtDiskTxPos> setpos;
    if (!FindTransactionsByDestination(destination, setpos))
        throw JSONRPCError(RPC_DATABASE_ERROR, "Cannot search for address");

    int nSkip = 0;
    int nCount = 100;
    bool fVerbose = true;
    if (request.params.size() > 1)
        fVerbose = (request.params[1].get_int() != 0);
    if (request.params.size() > 2)
        nSkip = request.params[2].get_int();
    if (request.params.size() > 3)
        nCount = request.params[3].get_int();

    if (nSkip < 0)
        nSkip += setpos.size();
    if (nSkip < 0)
        nSkip = 0;
    if (nCount < 0)
        nCount = 0;

    std::set<CExtDiskTxPos>::const_iterator it = setpos.begin();
    while (it != setpos.end() && nSkip--) it++;

    UniValue result(UniValue::VARR);
    while (it != setpos.end() && nCount--) {
        CTransactionRef ptx;
        uint256 hashBlock;
        if (!ReadTransaction(ptx, *it, hashBlock))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Cannot read transaction from disk");
        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        CMutableTransaction mtx(*ptx);
        ssTx << mtx;
        std::string strHex = HexStr(ssTx.begin(), ssTx.end());
        if (fVerbose) {
            UniValue object(UniValue::VOBJ);
            TxToJSON(*ptx, hashBlock, object);
            object.push_back(Pair("hex", strHex));
            result.push_back(object);
        } else {
            result.push_back(strHex);
        }
        it++;
    }
    return result;
}

UniValue getaddressbalance(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1)
    {
        throw std::runtime_error(
            "getaddressbalance\n"
            "\nReturns the balance for an address(es) (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "}\n"
            "\nResult:\n"
            "{\n"
            "  \"balance\"  (string) The current balance in satoshis\n"
            "  \"received\"  (string) The total number of satoshis received (including change)\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressbalance", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddressbalance", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
        );
	}
        
    if (!fAddrIndex || !fTxIndex)
        throw JSONRPCError(RPC_MISC_ERROR, "Address index or tx index not enabled");
	
	// get addres
	UniValue oparam(UniValue::VOBJ);
	oparam.read(request.params[0].get_str());

	UniValue addres = NullUniValue;
	addres = find_value(oparam, "addresses");
	if (addres.isNull() || (!addres.isArray()))
	{
	    throw std::runtime_error(
            "getaddressbalance need addresses param or param not array\n"
        );
	}
	CAmount balance = 0;
	CAmount received = 0;
	
    for (unsigned int i = 0; i < addres.size(); ++i) {
       LogPrintf("process address %s\n", addres[i].get_str());
	   
	   CTxDestination destination = DecodeDestination(addres[i].get_str());
	   if (!IsValidDestination(destination)) {
		   throw std::runtime_error("invalid TX output address");
	   }

	   std::set<CExtDiskTxPos> setpos;
	   if (!FindTransactionsByDestination(destination, setpos))
		   throw JSONRPCError(RPC_DATABASE_ERROR, "Cannot search for address");
	   
	   std::set<CExtDiskTxPos>::const_iterator it = setpos.begin();
	   while (it != setpos.end()) {
		   CTransactionRef ptx;
		   uint256 hashBlock;
		   if (!ReadTransaction(ptx, *it, hashBlock))
			   throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Cannot read transaction from disk");
		   
		   for(const CTxOut& itOut:ptx->vout)
		   {
			   CTxDestination addressIn;
			   if (!ExtractDestination(itOut.scriptPubKey, addressIn))
				   continue;
			   if (addressIn == destination)
			   {
				   balance += itOut.nValue;
				   if (itOut.nValue > 0) {
					   received += itOut.nValue;
				   }
			   }
		   }
		   /*
				minus spented 
				fetch vout thought txid
		   */
		   for(const CTxIn& itIn:ptx->vin)
		   {
			   CBlockIndex* blockindex = nullptr;
			   CTransactionRef tx;
			   uint256 hash_block;
			   if (!GetTransaction(itIn.prevout.hash, tx, Params().GetConsensus(), hash_block, true, blockindex)) {
				   std::string errmsg;
				   if (blockindex) {
					   if (!(blockindex->nStatus & BLOCK_HAVE_DATA)) {
						   throw JSONRPCError(RPC_MISC_ERROR, "Block not available");
					   }
					   errmsg = "No such transaction found in the provided block";
				   } else {
					   errmsg = fTxIndex
								? "No such mempool or blockchain transaction"
								: "No such mempool transaction. Use -txindex to enable blockchain transaction queries";
				   }
				   LogPrintf(". Use gettransaction for wallet transactions. %s\n",errmsg );
				   continue;
			   }
			   
			   CTxDestination addressPreout;
			   if (!ExtractDestination(tx->vout[itIn.prevout.n].scriptPubKey, addressPreout))
			   {
				   LogPrintf("rawtransation 231:get destination from script pub key failed.\n");
				   continue;
			   }
			   if (addressPreout == destination)
			   {
				   balance -= tx->vout[itIn.prevout.n].nValue;
			   }
		   }
		   
		   it++;
	   }
    }

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("balance", balance));
    result.push_back(Pair("received", received));
	
    return result;
}
