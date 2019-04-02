/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#ifndef TLBSIM_API_H
#define TLBSIM_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct tlbsim_client_t {
    // Load a machine word from physical address.
    uint64_t (*phys_load)(struct tlbsim_client_t* self, uint64_t);
    // Update a machine word
    bool (*phys_cmpxchg)(struct tlbsim_client_t *self, uint64_t, uint64_t, uint64_t);
    // Routine to invalidate L0 TLB to maintain inclusive property
    void (*invalidate_l0)(struct tlbsim_client_t* self, int hartid, uint64_t vpn);
} tlbsim_client_t;

// Provided by the client (user).
extern tlbsim_client_t tlbsim_client;

typedef struct {
    uint64_t satp;
    uint64_t vpn;
    // This is redundant with SATP, but we did this for convience.
    unsigned asid;
    unsigned hartid;
    unsigned ifetch: 1;
    unsigned write: 1;
    unsigned supervisor: 1;
    unsigned sum: 1;
    unsigned mxr: 1;
} tlbsim_req_t;

typedef struct {
    uint64_t ppn;
    uint64_t pte;
    unsigned granularity: 2;
    // If this is 0, it means permission check failed
    unsigned perm: 1;
} tlbsim_resp_t;

// Must be incremented atomically.
extern uint64_t tlbsim_instret;
extern uint64_t tlbsim_minstret;

// If these are false, then tlbsim_*instret counters are not needed.
extern bool tlbsim_need_instret;
extern bool tlbsim_need_minstret;

tlbsim_resp_t tlbsim_access(tlbsim_req_t* req);
void tlbsim_flush(int hartid, int asid, uint64_t vpn);

// Reset counters. If print is true, the old value is printed out.
void tlbsim_reset_counters(bool print);

#ifdef __cplusplus
}
#endif

#endif // TLBSIM_API_H
