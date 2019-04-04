/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 *
 * This file defines validators.
 */

#include "validator.h"

#define COLOR_ERR "\x1b[1;31m"
#define COLOR_RST "\x1b[0m"

// Check if two SATPs are consistent.
static inline bool consistent_satp(uint64_t a, uint64_t b) {
    // All fields other than ASID are required to be the same.
    return ((a ^ b) &~ (0xffffULL << 44)) == 0;
}

namespace tlbsim {

int ASIDValidator::access(tlb_entry_t &search, const tlbsim_req_t& req) {
    lock.lock();
    // We want the actual ASID in this use-case.
    uint64_t satp = req.satp;
    int asid = (satp >> 44) & 0xffff;
    if (asid == 0) {
        // ASID 0 may alias with all non-zero ASIDs
        for (auto iter = nonzero_asids.begin(); iter != nonzero_asids.end(); ) {
            uint64_t test = iter->second;
            if (!consistent_satp(satp, test)) {
                fprintf(
                    stderr,
                    COLOR_ERR "ASIDValidator: Hart %d uses ASID 0 (=%lx) while ASID %d (=%lx) is in-use\n" COLOR_RST,
                    req.hartid, req.satp, iter->first, test
                );
                // Erase the item to avoid duplicate errors.
                iter = nonzero_asids.erase(iter);
            } else {
                ++iter;
            }
        }
        uint64_t test = zero_asids[req.hartid];
        if (test && !consistent_satp(satp, test)) {
            fprintf(
                stderr,
                COLOR_ERR "ASIDValidator: Hart %d reuses ASID 0 (old=%lx, new=%lx) without flushing\n" COLOR_RST,
                req.hartid, test, satp
            );
        }
        zero_asids[req.hartid] = satp;
    } else {
        auto& ptr = nonzero_asids[asid];
        uint64_t test = ptr;
        if (test && !consistent_satp(satp, test)) {
            fprintf(
                stderr,
                COLOR_ERR "ASIDValidator: ASID %d reused (old=%lx, new=%lx) without flushing\n" COLOR_RST,
                asid, test, satp
            );
        }
        for (int i = 0; i < 32; i++) {
            uint64_t test = zero_asids[i];
            if (test && !consistent_satp(satp, test)) {
                fprintf(
                    stderr,
                    COLOR_ERR "ASIDValidator: ASID %d is used (=%lx) while hart %d still uses ASID 0 (=%lx)\n" COLOR_RST,
                    asid, test, i, satp
                );
                // Erase the item to avoid duplicate errors.
                zero_asids[i] = 0;
            }
        }
        ptr = satp;
    }

    lock.unlock();

    // As we only track ASIDs, leave the actual access to the parent.
    return parent->access(search, req);
}

void ASIDValidator::flush_local(asid_t asid, uint64_t vpn) {
    // We only track full ASIDs, so we don't care about page-level.
    if (vpn) return;
    lock.lock();
    if (asid.global()) {
        for (int i = 0; i < 32; i++) {
            zero_asids[i] = 0;
        }
        nonzero_asids.clear();
    } else {
        // We expect realm to be 0.
        if (asid < 32) {
            // The ASID we see here is already translated. We're unsure if this is a translated
            // ASID of zero, or an actual ASID is specified. We don't want to intrude the whole
            // TLB interface, so just be conservative.
            zero_asids[asid] = 0;
        }
        nonzero_asids.erase(asid);
    }
    lock.unlock();
}

}
