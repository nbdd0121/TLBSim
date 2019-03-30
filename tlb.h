/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#ifndef TLBSIM_TLB_H
#define TLBSIM_TLB_H

#include "pgtable.h"
#include "api.h"

// Represent an TLB entry.
// This is the internally-used exchange formats between all different types of TLB.
struct tlb_entry_t {
    uint64_t vpn;
    uint64_t ppn;
    uint64_t pte;
    // -1 indicates this TLB entry is not valid
    // When not -1, bits (..16) are isolation ID (realms) and bits (15..0) are ASID.
    int asid;
    int granularity;

    // Check whether an ASID match
    bool asid_match(int asid) {
        // In different realm
        if ((this->asid &~ 0xffff) != (asid &~ 0xffff)) return false;
        // Global page always match
        if (this->pte & PTE_G) return true;
        // Otherwise need ASID match
        return (this->asid & 0xffff) == (asid & 0xffff);
    }

    // Check whether an ASID match when flushing
    bool asid_match_flush(int asid) {
        // In different realm
        if ((this->asid &~ 0xffff) != (asid &~ 0xffff)) return false;
        // Want global flush
        if ((asid & 0xffff) == 0) return true;
        // Global page never flushes if not global flush
        if (this->pte & PTE_G) return false;
        // Otherwise need ASID match
        return (this->asid & 0xffff) == (asid & 0xffff);
    }
};

/*
 * Check if a request is permitted given a PTE.
 *
 * <0 -> Fail
 *  0 -> Success
 * >0 -> Need update PTE's accessed/dirty bits, the mask to be ORed is returned.
 */
int pte_permission_check(int pte, const tlbsim_req_t& req);

int walk_page(tlb_entry_t& search, const tlbsim_req_t& req);

#endif // TLBSIM_TLB_H