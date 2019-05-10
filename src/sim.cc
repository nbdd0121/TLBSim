/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#include <cstdio>

#include "api.h"
#include "tlb.h"
#include "config.h"
#include "stats.h"

using namespace tlbsim;


__attribute__((visibility("default")))
bool tlbsim_need_instret = true;
__attribute__((visibility("default")))
bool tlbsim_need_minstret = true;

static void print_counters() {
    print_instrets();
    itlb_stats.print("I-TLB");
    dtlb_stats.print("D-TLB");
    ctlb_stats.print("C-TLB");
    stlb_stats.print("S-TLB");
    print_faults();
    print_flushes();

    fprintf(stderr, "User Time: %lg\n", get_cputime());
}

static void reset_counters() {
    tlbsim_instret = 0;
    tlbsim_minstret = 0;
    itlb_stats.reset();
    dtlb_stats.reset();
    ctlb_stats.reset();
    stlb_stats.reset();
}

__attribute__((visibility("default")))
void tlbsim_reset_counters(bool print) {
    if (print) print_counters();
    reset_counters();
}

/* Display counters at exit */
__attribute__((destructor))
static void print_counters_at_exit(void) {
    print_counters();
}

__attribute__((visibility("default")))
tlbsim_resp_t tlbsim_access(tlbsim_req_t* req) {
    // Choose the TLB
    auto& tlb = (req->ifetch ? config_itlbs : config_dtlbs)[req->hartid];

    // Setup up TLB is not yet ready
    if (!tlb) {
        setup_private_tlb(req->hartid);
    }

    // ID-remapping
    if (req->asid == 0) req->asid = req->hartid;
    
    tlb_entry_t search;
    search.vpn = req->vpn;
    search.asid = req->asid;
    tlbsim_resp_t resp;
    resp.perm = tlb->access(search, *req) == 0;
    resp.ppn = search.ppn;
    resp.pte = search.pte;
    resp.granularity = 0;
    return resp;
}

__attribute__((visibility("default")))
void tlbsim_flush(int hartid, int asid, uint64_t vpn) {
    // First increment the statistics
    if (vpn == 0) {
        if (asid == -1) ++flush_full;
        else ++flush_asid;
    } else {
        if (asid == -1) ++flush_gpage;
        else ++flush_page;
    }

    // TLBs not setup yet.
    if (!config_itlbs[hartid]) return;

    asid_t asid_new = asid;

    // ID-remapping
    if (asid == -1) {
        asid_new = 0;
        asid_new.global(true);
    } else if (asid == 0) asid_new = hartid;

    config_itlbs[hartid]->flush_local(asid_new, vpn);
    config_dtlbs[hartid]->flush(asid_new, vpn);
}


