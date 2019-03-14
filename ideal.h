/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 *
 * This header defines an ideal TLB, which has infinite memory. Ideal TLB never evicts entries.
 * This is similar to associative TLBs with very large size, but has better performance by using
 * hashmaps.
 */

#ifndef TLBSIM_IDEAL_H
#define TLBSIM_IDEAL_H

#include "tlb.h"

namespace tlbsim {

class IdealTLB: public TLB {
private:
    std::unordered_map<uint64_t, tlb_entry_t> map;
    std::unordered_map<uint64_t, tlb_entry_t> g_map;
    Spinlock lock;
public:
    IdealTLB(TLB* parent, tlb_stats_t* stats): TLB(parent, stats, -1) {}
    bool find_and_lock(tlb_entry_t& search) override {
        lock.lock();
        uint64_t key = (search.vpn << 24) | search.asid;
        auto iter = g_map.find(key &~ 0xffff);
        if (iter != g_map.end()) {
            search.ppn = iter->second.ppn;
            search.pte = iter->second.pte;
            return true;
        }
        auto iter2 = map.find(key);
        if (iter2 != map.end()) {
            search.ppn = iter2->second.ppn;
            search.pte = iter2->second.pte;
            return true;
        }
        return false;
    }

    void unlock(const tlb_entry_t&) override {
        lock.unlock();
    }

    void insert_and_unlock(const tlb_entry_t& insert) override {
        uint64_t key = (insert.vpn << 24) | insert.asid;
        if (insert.pte & PTE_G) {
            g_map[key &~ 0xffff] = insert;
        } else {
            map[key] = insert;
        }
        lock.unlock();
    }

    void flush_local(int asid, uint64_t vpn) override {
        lock.lock();
        uint64_t num_flush = 0;
        if (vpn == 0) {
            if (asid == -1) {
                for (auto iter = g_map.begin(); iter != g_map.end(); ) {
                    if ((iter->second.asid &~ 0xffff) == (asid &~ 0xffff)) {
                        num_flush++;
                        iter = g_map.erase(iter);
                    } else {
                        ++iter;
                    }
                }
            }
            for (auto iter = map.begin(); iter != map.end(); ) {
                if ((iter->second.asid &~ 0xffff) == (asid &~ 0xffff)) {
                    num_flush++;
                    iter = map.erase(iter);
                } else {
                    ++iter;
                }
            }
        } else {
            uint64_t key = (vpn << 24) | asid;
            if (asid == -1) {
                key &=~ 0xffff;
                auto iter = g_map.find(key);
                if (iter != g_map.end()) {
                    num_flush++;
                    g_map.erase(iter);
                }
                for (auto iter = map.begin(); iter != map.end(); ) {
                    if ((iter->first &~ 0xffff) == key) {
                        num_flush++;
                        iter = map.erase(iter);
                    } else {
                        ++iter;
                    }
                }
            } else {
                auto iter = map.find(key);
                if (iter != map.end()) {
                    num_flush++;
                    map.erase(iter);
                }
            }
        }
        lock.unlock();
        stats->flush += num_flush;
    }
};

}

#endif
