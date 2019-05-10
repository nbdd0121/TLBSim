/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#ifndef TLBSIM_STATS_H
#define TLBSIM_STATS_H

#include "util.h"

namespace tlbsim {

// Global page fault statistics
extern atomic_u64_t v_fault;
extern atomic_u64_t u_fault;
extern atomic_u64_t s_fault;
extern atomic_u64_t r_fault;
extern atomic_u64_t w_fault;
extern atomic_u64_t x_fault;
extern atomic_u64_t a_fault;
extern atomic_u64_t d_fault;

// Global TLB flush statistics
extern atomic_u64_t flush_full;
extern atomic_u64_t flush_gpage;
extern atomic_u64_t flush_asid;
extern atomic_u64_t flush_page;

// Per-TLB statistics
struct tlb_stats_t {
    atomic_u64_t miss;
    atomic_u64_t evict;
    atomic_u64_t flush;

    void reset() {
        miss = 0;
        evict = 0;
        flush = 0;
    }

    void print(const char* name);
};

extern tlb_stats_t itlb_stats;
extern tlb_stats_t dtlb_stats;
extern tlb_stats_t ctlb_stats;
extern tlb_stats_t stlb_stats;

void print_instrets();
void print_faults();
void print_flushes();

}

#endif // TLBSIM_STATS_H
