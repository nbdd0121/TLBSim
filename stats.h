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

#ifndef TLBSIM_STATS_H
#define TLBSIM_STATS_H

#include "util.h"

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

void print_instrets();
void print_faults();
void print_flushes();

#endif // TLBSIM_STATS_H