// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addressindex.h>

#include <chainparams.h>
#include <hash.h>
#include <random.h>
#include <pow.h>
#include "random.h"
#include <uint256.h>
#include <util.h>
#include <ui_interface.h>
#include <init.h>
#include <prime/prime.h>

#include <stdint.h>

#include <boost/thread.hpp>


#include <validation.h>

#include <arith_uint256.h>
#include "base58.h"
#include <chain.h>
#include <chainparams.h>
#include <checkpoints.h>
#include <checkqueue.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <cuckoocache.h>
#include <hash.h>
#include <init.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <prime/parameters.h>
#include <random.h>
#include <reverse_iterator.h>
#include <script/script.h>
#include <script/sigcache.h>
#include <script/standard.h>
#include <timedata.h>
#include <tinyformat.h>
#include <txdb.h>
#include <addressindex.h>
#include <txmempool.h>
#include <ui_interface.h>
#include <undo.h>
#include <util.h>
#include <utilmoneystr.h>
#include <utilstrencodings.h>
#include <validationinterface.h>
#include <warnings.h>

#include <future>
#include <sstream>
#include <core_io.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/thread.hpp>

CAddrIndexBlockTreeDB::CAddrIndexBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe) : CBlockTreeDB(nCacheSize, fMemory, fWipe) {
    if (!Read('S', salt)) {
        salt = GetRandHash();
        Write('S', salt);
    }
}

bool CAddrIndexBlockTreeDB::ReadAddrIndex(uint160 addrid, std::vector<CExtDiskTxPos> &list) {
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    uint64_t lookupid;
    {
        CHashWriter ss(SER_GETHASH, 0);
        ss << salt;
        ss << addrid;
        lookupid = UintToArith256(ss.GetHash()).GetLow64();
    }

    pcursor->Seek(std::make_pair('a', lookupid));

    while (pcursor->Valid()) {
        std::pair<std::pair<char, uint64_t>, CExtDiskTxPos> key;
        if (pcursor->GetKey(key) && key.first.first == 'a' && key.first.second == lookupid) {
            list.push_back(key.second);
        } else {
            break;
        }
        pcursor->Next();
    }
    return true;
}

bool CAddrIndexBlockTreeDB::AddAddrIndex(const std::vector<std::pair<uint160, CExtDiskTxPos> > &list) {
    unsigned char foo[0];
    CDBBatch batch(*this);
    for (std::vector<std::pair<uint160, CExtDiskTxPos> >::const_iterator it=list.begin(); it!=list.end(); it++) {
        CHashWriter ss(SER_GETHASH, 0);
        ss << salt;
        ss << it->first;
        batch.Write(std::make_pair(std::make_pair('a', UintToArith256(ss.GetHash()).GetLow64()), it->second), FLATDATA(foo));
    }
    return WriteBatch(batch, true);
}

bool CAddrIndexBlockTreeDB::EraseAddrIndex(const std::vector<std::pair<uint160, CExtDiskTxPos> > &list) {
    unsigned char foo[0];
    CDBBatch batch(*this);
    for (std::vector<std::pair<uint160, CExtDiskTxPos> >::const_iterator it=list.begin(); it!=list.end(); it++) {
        CHashWriter ss(SER_GETHASH, 0);
        ss << salt;
        ss << it->first;
        batch.Erase(std::make_pair(std::make_pair('a', UintToArith256(ss.GetHash()).GetLow64()), it->second));
    }
    return WriteBatch(batch, true);
}

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
