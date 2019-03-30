/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#ifndef TLBSIM_CONFIG_H
#define TLBSIM_CONFIG_H

//
// Global configurations
//

// Whether invalidate entries should be cached.
extern bool config_cache_inv;

// Whether a non-accessed or dirty PTE should be updated or faulted.
extern bool config_update_pte;

#endif // TLBSIM_CONFIG_H