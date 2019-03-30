/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#ifndef TLBSIM_UTIL_H
#define TLBSIM_UTIL_H

#include <atomic>

class Spinlock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire));
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }
};

typedef std::atomic<uint64_t> atomic_u64_t;

// Shorthand for loading value from atomic counters
static inline uint64_t operator *(atomic_u64_t& value) {
    return value.load(std::memory_order_relaxed);
}

#endif // TLBSIM_UTIL_H
