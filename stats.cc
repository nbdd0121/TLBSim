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

#include <cstdio>

#include "api.h"
#include "config.h"
#include "stats.h"

__attribute__((visibility("default")))
uint64_t tlbsim_instret;
__attribute__((visibility("default")))
uint64_t tlbsim_minstret;

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
