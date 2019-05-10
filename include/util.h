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

struct atomic_u64_t {
    std::atomic<uint64_t> counter;

    atomic_u64_t& operator =(uint64_t value) {
        counter.store(value, std::memory_order_relaxed);
        return *this;
    }

    atomic_u64_t& operator ++() noexcept {
        counter.fetch_add(1, std::memory_order_relaxed);
        return *this;
    }

    atomic_u64_t& operator +=(uint64_t value) noexcept {
        counter.fetch_add(value, std::memory_order_relaxed);
        return *this;
    }

    // Shorthand for loading value from it
    uint64_t operator *() const noexcept {
        return counter.load(std::memory_order_relaxed);
    }
};

double get_cputime() noexcept;

static constexpr int ilog2(int x) {
    return sizeof(int) * 8 - 1 - __builtin_clz(x);
}

static constexpr uint32_t bswap32(uint32_t value) {
    return __builtin_bswap32(value);
}

}

#endif // TLBSIM_UTIL_H
