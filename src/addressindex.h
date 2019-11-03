#ifndef ADDRESSINDEX_H
#define ADDRESSINDEX_H

#include <txdb.h>

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

struct CExtDiskTxPos : public CDiskTxPos
{
    unsigned int nHeight;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
            READWRITE(*(CDiskTxPos*)this);
            READWRITE(VARINT(nHeight));
    }

    CExtDiskTxPos(const CDiskTxPos &pos, int nHeightIn) : CDiskTxPos(pos), nHeight(nHeightIn) {
    }

    CExtDiskTxPos() {
        SetNull();
    }

    void SetNull() {
        CDiskTxPos::SetNull();
        nHeight = 0;
    }

    friend bool operator==(const CExtDiskTxPos &a, const CExtDiskTxPos &b) {
        return (a.nHeight == b.nHeight && a.nFile == b.nFile && a.nPos == b.nPos && a.nTxOffset == b.nTxOffset);
    }

    friend bool operator!=(const CExtDiskTxPos &a, const CExtDiskTxPos &b) {
        return !(a == b);
    }

    friend bool operator<(const CExtDiskTxPos &a, const CExtDiskTxPos &b) {
        if (a.nHeight < b.nHeight) return true;
        if (a.nHeight > b.nHeight) return false;
        return ((const CDiskTxPos)a < (const CDiskTxPos)b);
    }
};

class CAddrIndexBlockTreeDB : public CBlockTreeDB
{
private:
    uint256 salt;
public:
    explicit CAddrIndexBlockTreeDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    CAddrIndexBlockTreeDB(const CBlockTreeDB&) = delete;
    CAddrIndexBlockTreeDB& operator=(const CBlockTreeDB&) = delete;
    
    bool ReadAddrIndex(uint160 addrid, std::vector<CExtDiskTxPos> &list);
    bool AddAddrIndex(const std::vector<std::pair<uint160, CExtDiskTxPos> > &list);
    bool EraseAddrIndex(const std::vector<std::pair<uint160, CExtDiskTxPos> > &list);
};

bool ReadTransaction(CTransactionRef& tx, const CDiskTxPos &pos, uint256 &hashBlock);

bool FindTransactionsByDestination(const CTxDestination &dest, std::set<CExtDiskTxPos> &setpos);

void BuildAddrIndex(const CScript &script, const CExtDiskTxPos &pos, std::vector<std::pair<uint160, CExtDiskTxPos> > &out);

bool EraseTxIndexDataForBlock(std::vector<std::pair<uint256, CDiskTxPos> > &vPosTxid,std::vector<std::pair<uint160, CExtDiskTxPos> > &vPosAddrid);

#endif // ADDRESSINDEX_H
