/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 *
 * This header declares validators.
 */

#ifndef TLBSIM_VALIDATOR_H
#define TLBSIM_VALIDATOR_H

#include <unordered_map>
#include "tlb.h"
#include "util.h"
#include "ideal.h"

namespace tlbsim {

class ASIDValidator: public TLB {
private:
    std::unordered_map<int, uint64_t> nonzero_asids;
    // The reverse mapping to nonzero_asids.
    // This mapping exists as we would like to enforce the property between ASIDs and address
    // spaces managed by the OS to be partial bijective.
    std::unordered_map<uint64_t, int> asid_revmap;
    uint64_t zero_asids[32] {};
    Spinlock lock;
public:
    ASIDValidator(TLB* parent): TLB(parent, NULL, -1) {}
    
    int access(tlb_entry_t &search, const tlbsim_req_t& req) override;

    void flush_local(asid_t asid, uint64_t vpn) override;
};


class TLBValidator: public IdealTLB {
public:
    TLBValidator(TLB* parent, tlb_stats_t* stats): IdealTLB(parent, stats) {
    }

    int access(tlb_entry_t &search, const tlbsim_req_t& req) override;
};

}

#endif
