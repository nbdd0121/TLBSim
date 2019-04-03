/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#include <atomic>
#include <cstdio>
#include <cinttypes>
#include <exception>

#include "tlb.h"
#include "config.h"
#include "util.h"
#include "stats.h"

namespace tlbsim {

int pte_permission_check(int pte, const tlbsim_req_t& req) {
    if (!(pte & PTE_V)) {
        ++v_fault;
    } else if ((pte & PTE_U) && (req.supervisor && !req.sum)) {
        ++u_fault;
    } else if (!(pte & PTE_U) && !req.supervisor) {
        ++s_fault;
    } else if (!req.ifetch && !req.write && !((pte & PTE_R) || ((pte & PTE_X) && req.mxr))) {
        ++r_fault;
    } else if (req.write && !(pte & PTE_W)) {
        ++w_fault;
    } else if (req.ifetch && !(pte & PTE_X)) {
        ++x_fault;
    } else {
        int mask = PTE_A | (req.write ? PTE_D : 0);
        int update = mask &~ (pte & mask);
        if (update) {
            if ((update & PTE_D)) ++d_fault;
            else ++a_fault;
        }
        return update;
    }
    return -1;
}

PageWalker page_walker;

int PageWalker::access(tlb_entry_t& search, const tlbsim_req_t& req) {
    // Find out levels in total
    int levels;
    switch (req.satp & SATP_MODE) {
    case SATP_MODE_SV39: levels = 3; break;
    case SATP_MODE_SV48: levels = 4; break;
    default: std::terminate();
    }

    int vpn_bits = levels * 9;

    // Check if the address is canonical.
    uint64_t vpn = search.vpn;
    uint64_t canonical_vpn = (uint64_t)((int64_t)(vpn << (64 - vpn_bits)) >> (64 - vpn_bits - 12)) >> 12;
    if (canonical_vpn != vpn) {
        fprintf(stderr, "%" PRIx64 " is not canonical %" PRIx64 "\n", vpn, canonical_vpn);
        return -2;
    }

    uint64_t ppn = req.satp & SATP_PPN;

    for (int i = 0, bits_left = vpn_bits - 9; i < levels; i++, bits_left -= 9) {
        uint64_t index = (vpn >> bits_left) & 0x1ff;
        uint64_t pte_addr = (ppn << 12) + index * 8;
        uint64_t pte = tlbsim_client.phys_load(&tlbsim_client, pte_addr);
        ppn = pte >> 10;

        // Check for invalid PTE
        if (!(pte & PTE_V)) goto invalid;

        // Check for malformed PTEs
        if ((pte & (PTE_R | PTE_W | PTE_X)) == PTE_W) goto invalid;
        if ((pte & (PTE_R | PTE_W | PTE_X)) == (PTE_W | PTE_X)) goto invalid;

        // A global bit will cause the page to be global regardless if this is leaf.
        if ((pte & PTE_G)) search.asid.global(true);

        // Not leaf yet
        if (!(pte & (PTE_R | PTE_W | PTE_X))) continue;

        // Check for misaligned huge page
        if (ppn & ((1ULL << bits_left) - 1)) goto invalid;

        int perm = pte_permission_check(pte, req);
        if (config_update_pte && perm > 0) {
            uint64_t updated_pte = pte | perm;
            if (tlbsim_client.phys_cmpxchg(&tlbsim_client, pte_addr, pte, updated_pte)) {
                pte = updated_pte;
            }
        }

        // PPN is always filled as if this is a 4K page.
        search.ppn = ppn | (vpn & ((1L << bits_left) - 1));
        search.pte = pte;
        search.granularity = levels - 1 - i;
        return perm;
    }

invalid:
    search.ppn = 0;
    search.pte = 0;
    return pte_permission_check(0, req);
}

}
