/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#ifndef TLBSIM_CONFIG_H
#define TLBSIM_CONFIG_H

namespace tlbsim {

class TLB;
class LogReplayer;

//
// Global configurations
//

// Whether invalidate entries should be cached.
extern bool config_cache_inv;

// Whether a non-accessed or dirty PTE should be updated or faulted.
extern bool config_update_pte;

// Globally shared TLBs
extern TLB* config_stlb;
extern TLB* config_ctlbs[32];
extern TLB* config_itlbs[32];
extern TLB* config_dtlbs[32];
extern LogReplayer* config_replayer;

void setup_private_tlb(int hartid);

}

#endif // TLBSIM_CONFIG_H
