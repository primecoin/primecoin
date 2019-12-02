#ifndef ADDRINDEX_H
#define ADDRINDEX_H

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

bool ReadTransaction(CTransactionRef& tx, const CDiskTxPos &pos, uint256 &hashBlock);

bool FindTransactionsByDestination(const CTxDestination &dest, std::set<CExtDiskTxPos> &setpos);

void BuildAddrIndex(const CScript &script, const CExtDiskTxPos &pos, std::vector<std::pair<uint160, CExtDiskTxPos> > &out);

bool EraseTxIndexDataForBlock(std::vector<std::pair<uint256, CDiskTxPos> > &vPosTxid,std::vector<std::pair<uint160, CExtDiskTxPos> > &vPosAddrid);

#endif // ADDRINDEX_H
