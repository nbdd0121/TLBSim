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
#include "dyn_array.h"

namespace tlbsim {

struct FIFOSet {
    DynArray<tlb_entry_t> entries;
    int ptr = 0;
    int insert_ptr = 0;

    FIFOSet(int size): entries(size) {}

    bool find(tlb_entry_t& search) {
        insert_ptr = -1;
        int associativity = entries.size();
        for (int i = 0; i < associativity; i++) {
            auto& entry = entries[i];
            if (!entry.valid()) {
                if (insert_ptr == -1) insert_ptr = i;
                continue;
            }
            if (entry.vpn != search.vpn) continue;
            if (!entry.asid_match(search.asid)) continue;
            search.ppn = entry.ppn;
            search.pte = entry.pte;
            insert_ptr = i;
            return true;
        }
        if (insert_ptr == -1) {
            insert_ptr = ptr;
        }
        return false;
    }

    void insert(const tlb_entry_t& insert, TLB& tlb) {
        if (ptr == insert_ptr) {
            int associativity = entries.size();
            ptr = ptr == associativity - 1 ? 0 : ptr + 1;
        }

        auto& entry = entries[insert_ptr];
        if (entry.valid()) {
            ++tlb.stats->evict;
            if (tlb.hartid != -1) {
                tlbsim_client.invalidate_l0(&tlbsim_client, tlb.hartid, entry.vpn);
            }
        }

        entry = insert;
    }

    void flush(int asid, uint64_t vpn, uint64_t& num_flush) {
        for (auto& entry: entries) {
            if (!entry.valid()) continue;
            if (vpn != 0 && entry.vpn != vpn) continue;
            if (!entry.asid_match_flush(asid)) continue;
            entry.invalidate();
            num_flush++;
        }
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

    void flush_local(int asid, uint64_t vpn) override {
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
public:
    SetAssocTLB(TLB* parent, tlb_stats_t* stats, int hartid, size_t size, int associativity):
        TLB(parent, stats, hartid), maps(size / associativity, set_t(associativity)) {

        idx_bits = ilog2(size / associativity);
    }

    bool find_and_lock(tlb_entry_t& search) override {
        size_t set_index = (search.vpn ^ (search.asid >> 16 << (idx_bits - 3))) & ((1 << idx_bits) - 1);
        auto& set = maps[set_index];
        set.lock.lock();
        return set.set.find(search);
    }

    void unlock(const tlb_entry_t& entry) override {
        size_t set_index = (entry.vpn ^ (entry.asid >> 16 << (idx_bits - 3))) & ((1 << idx_bits) - 1);
        auto& set = maps[set_index];
        set.lock.unlock();
    }

    void insert_and_unlock(const tlb_entry_t& insert) override {
        size_t set_index = (insert.vpn ^ (insert.asid >> 16 << (idx_bits - 3))) & ((1 << idx_bits) - 1);
        auto& set = maps[set_index];
        set.set.insert(insert, *this);
        set.lock.unlock();
    }

    void flush_local(int asid, uint64_t vpn) override {
        uint64_t num_flush = 0;
        if (vpn == 0) {
            for (auto& set: maps) {
                set.lock.lock();
                set.set.flush(asid, 0, num_flush);
                set.lock.unlock();
            }
        } else {
            size_t set_index = (vpn ^ (asid >> 16 << (idx_bits - 3))) & ((1 << idx_bits) - 1);
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
