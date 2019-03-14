/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#include "tlb.h"
#include "stats.h"
#include "config.h"

namespace tlbsim {

int TLB::access(tlb_entry_t &search, const tlbsim_req_t& req) {
    int perm;
    if (find_and_lock(search)) {
        perm = pte_permission_check(search.pte, req);
        if (perm <= 0 || !config_update_pte) goto unlock;
    }

    ++stats->miss;

    perm = parent->access(search, req);
    if (!config_cache_inv && perm != 0) goto unlock;

    insert_and_unlock(search);
    return perm;

unlock:
    unlock(search);
    return perm;
}

}
