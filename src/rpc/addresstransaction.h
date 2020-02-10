#ifndef ADDRESSTRANSACTION_H
#define ADDRESSTRANSACTION_H

class UniValue;
class JSONRPCRequest;

UniValue searchrawtransactions(const JSONRPCRequest& request);

UniValue getaddressbalance(const JSONRPCRequest& request);

UniValue searchutxos(const JSONRPCRequest& request);
#endif // ADDRESSTRANSACTION_H
