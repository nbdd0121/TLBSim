/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 *
 * This header defines two types of TLBs that uses associative sets. This includes fully
 * associative and set associative TLBs.
 */

#ifndef TLBSIM_ASSOC_H
#define TLBSIM_ASSOC_H

#include "tlb.h"
#include "stats.h"
#include "dyn_array.h"
#include "dyn_bitset.h"

namespace tlbsim {

template<typename T>
struct FIFOCache {
    DynArray<T> entries;
    DynBitset valid;
    int ptr = 0;
    int insert_ptr = 0;

    FIFOCache(int size): entries(size), valid(size) {}

    template<typename Matcher>
    T* find(Matcher matcher) {
        insert_ptr = -1;
        int associativity = entries.size();
        for (int i = 0; i < associativity; i++) {
            if (!valid[i]) {
                if (insert_ptr == -1) insert_ptr = i;
                continue;
            }
            auto& entry = entries[i];
            if (!matcher(entry)) continue;
            insert_ptr = i;
            return &entry;
        }
        if (insert_ptr == -1) {
            insert_ptr = ptr;
        }
        return nullptr;
    }

    template<typename Evicter>
    T* insert(const T& insert, Evicter evicter) {
        if (ptr == insert_ptr) {
            int associativity = entries.size();
            ptr = ptr == associativity - 1 ? 0 : ptr + 1;
        }

        auto& entry = entries[insert_ptr];
        if (valid[insert_ptr]) {
            evicter(entry);
        }

        entry = insert;
        valid[insert_ptr] = true;
        return &entry;
    }

    template<typename Filter>
    void filter(Filter filter) {
        int associativity = entries.size();
        for (int i = 0; i < associativity; i++) {
            if (!valid[i]) continue;
            auto& entry = entries[i];
            if (!filter(entry)) continue;
            valid[i] = false;
        }
    }
};

struct FIFOSet {
    FIFOCache<tlb_entry_t> cache;
    FIFOSet(int size): cache(size) {}

    bool find(tlb_entry_t& search) {
        auto ptr = cache.find([&](auto& entry) {
            if (entry.vpn != search.vpn) return false;
            if (!entry.asid.match(search.asid)) return false;
            return true;
        });
        if (!ptr) return false;
        search = *ptr;
        return true;
    }

    void insert(const tlb_entry_t& insert, TLB& tlb) {
        cache.insert(insert, [&](auto& entry) {
            ++tlb.stats->evict;
            if (tlb.hartid != -1) {
                tlbsim_client.invalidate_l0(&tlbsim_client, tlb.hartid, entry.vpn, 3);
            }
        });
    }

    void flush(int asid, uint64_t vpn, uint64_t& num_flush) {
        cache.filter([&](auto& entry) {
            if (vpn != 0 && entry.vpn != vpn) return false;
            if (!entry.asid.match_flush(asid)) return false;
            num_flush++;
            return true;
        });
    }
};

template<typename Set = FIFOSet>
class AssocTLB: public TLB {
private:
    Set set;
    Spinlock lock;
public:
    AssocTLB(TLB* parent, tlb_stats_t* stats, int hartid, int size): TLB(parent, stats, hartid), set(size) {}
    bool find_and_lock(tlb_entry_t& search) override {
        lock.lock();
        return set.find(search);
    }

    void unlock(const tlb_entry_t&) override {
        lock.unlock();
    }

    void insert_and_unlock(const tlb_entry_t& insert) override {
        set.insert(insert, *this);
        lock.unlock();
    }

    void flush_local(asid_t asid, uint64_t vpn) override {
        lock.lock();
        uint64_t num_flush = 0;
        set.flush(asid, vpn, num_flush);
        lock.unlock();
        stats->flush += num_flush;
    }
};

template<typename Set = FIFOSet>
class SetAssocTLB: public TLB {
private:
    struct set_t {
        Set set;
        Spinlock lock;

        set_t(int size): set(size) {}
        set_t(const set_t& other): set(other.set) {}
    };
    DynArray<set_t> maps;
    int idx_bits;
private:
    inline size_t index(asid_t asid, uint64_t vpn) const {
        // Due to the existence of global pages, we either need to treat them differently, or
        // we cannot use ASID bits in set index. As we mostly use an associativity of 8, and we
        // usually have no more than 8 cores, this choice shouldn't be too bad.
        // Of course a separate global/non-global page TLB can be easily implemented with our
        // framework.

        // We would like to also include realm id in calculation.
        // We assume bits of realm id are equally important and least significant bits are used
        // first.
        size_t realm = bswap32(asid.realm()) >> (32 - idx_bits);
        size_t set_index = (vpn & ((1 << idx_bits) - 1)) ^ realm;
        return set_index;
    }

public:
    SetAssocTLB(TLB* parent, tlb_stats_t* stats, int hartid, size_t size, int associativity):
        TLB(parent, stats, hartid), maps(size / associativity, set_t(associativity)) {

        idx_bits = ilog2(size / associativity);
    }

    bool find_and_lock(tlb_entry_t& search) override {
        size_t set_index = index(search.asid, search.vpn);
        auto& set = maps[set_index];
        set.lock.lock();
        return set.set.find(search);
    }

    void unlock(const tlb_entry_t& entry) override {
        size_t set_index = index(entry.asid, entry.vpn);
        auto& set = maps[set_index];
        set.lock.unlock();
    }

    void insert_and_unlock(const tlb_entry_t& insert) override {
        size_t set_index = index(insert.asid, insert.vpn);
        auto& set = maps[set_index];
        set.set.insert(insert, *this);
        set.lock.unlock();
    }

    void flush_local(asid_t asid, uint64_t vpn) override {
        uint64_t num_flush = 0;
        if (vpn == 0) {
            for (auto& set: maps) {
                set.lock.lock();
                set.set.flush(asid, 0, num_flush);
                set.lock.unlock();
            }
        } else {
            size_t set_index = index(asid, vpn);
            auto& set = maps[set_index];
            set.lock.lock();
            set.set.flush(asid, vpn, num_flush);
            set.lock.unlock();
        }
        stats->flush += num_flush;
    }
};

}

#endif
