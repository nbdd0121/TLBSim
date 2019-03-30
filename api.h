/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019, Gary Guo
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
