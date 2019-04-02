/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#ifndef TLBSIM_UTIL_H
#define TLBSIM_UTIL_H

#include <atomic>

namespace tlbsim {

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

double get_cputime() noexcept;

static constexpr int ilog2(int x) {
    return sizeof(int) * 8 - 1 - __builtin_clz(x);
}

static constexpr uint32_t bswap32(uint32_t value) {
    return __builtin_bswap32(value);
}

}

#endif // TLBSIM_UTIL_H
