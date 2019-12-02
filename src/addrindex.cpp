#include <addrindex.h>
#include <validation.h>
#include <chainparams.h>
#include <hash.h>
#include <random.h>
#include <pow.h>
#include "base58.h"
#include "random.h"
#include <uint256.h>
#include <util.h>
#include <ui_interface.h>
#include <init.h>
#include <prime/prime.h>

#include <stdint.h>
#include <boost/thread.hpp>
#include <core_io.h>

bool ReadTransaction(CTransactionRef& tx, const CDiskTxPos &pos, uint256 &hashBlock) {
    CAutoFile file(OpenBlockFile(pos, true), SER_DISK, CLIENT_VERSION);
    if (file.IsNull())
        return error("%s: OpenBlockFile failed", __func__);
    CBlockHeader header;
    try {
        file >> header;
		fseek(file.Get(), pos.nTxOffset, SEEK_CUR);
		file >> tx;
    } catch (std::exception &e) {
        LogPrintf("Upgrading txindex database... [%s]\n", e.what());
        return error("%s() : deserialize or I/O error", __PRETTY_FUNCTION__);
    }
    hashBlock = header.GetHash();
    return true;
}

bool FindTransactionsByDestination(const CTxDestination &dest, std::set<CExtDiskTxPos> &setpos) {
    uint160 addrid;
    const CKeyID *pkeyid = boost::get<CKeyID>(&dest);
    if (pkeyid)
        addrid = static_cast<uint160>(*pkeyid);
    if (addrid.IsNull()) {
        const CScriptID *pscriptid = boost::get<CScriptID>(&dest);
        if (pscriptid)
            addrid = static_cast<uint160>(*pscriptid);
        }
    if (addrid.IsNull())
        return false;

    LOCK(cs_main);
    if (!fAddrIndex)
        return false;
    std::vector<CExtDiskTxPos> vPos;
    if (!pblocktree->ReadAddrIndex(addrid, vPos))
        return false;
    setpos.insert(vPos.begin(), vPos.end());
    return true;
}

// Index either: a) every data push >=8 bytes,  b) if no such pushes, the entire script
void BuildAddrIndex(const CScript &script, const CExtDiskTxPos &pos, std::vector<std::pair<uint160, CExtDiskTxPos> > &out)
{
    int outSize = out.size();
    CScript::const_iterator pc = script.begin();
    CScript::const_iterator pend = script.end();
    std::vector<unsigned char> data;
    opcodetype opcode;
    bool fHaveData = false;
    while (pc < pend) {
        script.GetOp(pc, opcode, data);
        if (0 <= opcode && opcode <= OP_PUSHDATA4 && data.size() >= 8) { // data element
            uint160 addrid;
            if (data.size() <= 20) {
                memcpy(&addrid, &data[0], data.size());
            } else {
                addrid = Hash160(data);
            }
            //LogPrintf("BuildAddrIndex: add index ===== %s\n", addrid.GetHex());
            out.push_back(std::make_pair(addrid, pos));
            fHaveData = true;
        }
    }
    if (!fHaveData) {
        uint160 addrid = Hash160(script);
        //LogPrintf("BuildAddrIndex: add index %s\n", addrid.GetHex());
        out.push_back(std::make_pair(addrid, pos));
    }
	
	if (outSize == out.size())
	{
		LogPrintf("BuildAddrIndex:get address from scriptPubkey failed. %s===========\n", ScriptToAsmStr(script));
	}
}

bool EraseTxIndexDataForBlock(std::vector<std::pair<uint256, CDiskTxPos> > &vPosTxid,std::vector<std::pair<uint160, CExtDiskTxPos> > &vPosAddrid)
{
    if (!fTxIndex || !fAddrIndex) return true;

    if (!pblocktree->EraseTxIndex(vPosTxid)) {
        return false;
    }
	if (!pblocktree->EraseAddrIndex(vPosAddrid)){
		return false;
	}
    return true;
}
