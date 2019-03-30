/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#include <cstdio>

#include "api.h"
#include "config.h"
#include "stats.h"

__attribute__((visibility("default")))
uint64_t tlbsim_instret;
__attribute__((visibility("default")))
uint64_t tlbsim_minstret;

namespace tlbsim {

atomic_u64_t v_fault;
atomic_u64_t u_fault;
atomic_u64_t s_fault;
atomic_u64_t r_fault;
atomic_u64_t w_fault;
atomic_u64_t x_fault;
atomic_u64_t a_fault;
atomic_u64_t d_fault;

atomic_u64_t flush_full;
atomic_u64_t flush_gpage;
atomic_u64_t flush_asid;
atomic_u64_t flush_page;

void print_instrets() {
    fprintf(stderr, "Total instructions : %ld\n", tlbsim_instret);
    fprintf(stderr, "Memory Instructions: %ld\n", tlbsim_minstret);
}

void print_faults() {
    uint64_t pagefaults = *v_fault + *u_fault + *s_fault + *r_fault + *w_fault + *x_fault;
    if (!config_update_pte)
        pagefaults += *a_fault + *d_fault;
    fprintf(stderr, "Pagefaults:\n");
    fprintf(stderr, "  Total: %ld\n", pagefaults);
    fprintf(stderr, "  V    : %ld\n", *v_fault);
    fprintf(stderr, "  U    : %ld\n", *u_fault);
    fprintf(stderr, "  S    : %ld\n", *s_fault);
    fprintf(stderr, "  R    : %ld\n", *r_fault);
    fprintf(stderr, "  W    : %ld\n", *w_fault);
    fprintf(stderr, "  X    : %ld\n", *x_fault);
    fprintf(stderr, "  A    : %ld\n", *a_fault);
    fprintf(stderr, "  D    : %ld\n", *d_fault);
}

void print_flushes() {
    fprintf(stderr, "SFENCE.VMA:\n");
    fprintf(stderr, "  total         : %ld\n", *flush_full +  *flush_gpage + *flush_asid + *flush_page);
    fprintf(stderr, "  full          : %ld\n", *flush_full);
    fprintf(stderr, "  with      addr: %ld\n", *flush_gpage);
    fprintf(stderr, "  with asid     : %ld\n", *flush_asid);
    fprintf(stderr, "  with asid/addr: %ld\n", *flush_page);
}

void tlb_stats_t::print(const char* name) {
    fprintf(stderr, "%s:\n", name);
    fprintf(stderr, "  Miss    : %ld\n", *this->miss);
    fprintf(stderr, "  Eviction: %ld\n", *this->evict);
    fprintf(stderr, "  Flush   : %ld\n", *this->flush);
}

}
