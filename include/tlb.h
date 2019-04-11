/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#ifndef TLBSIM_TLB_H
#define TLBSIM_TLB_H

#include "pgtable.h"
#include "api.h"

namespace tlbsim {

// Represent a packed structure of realm ID + ASID.
// In the future this may be expanded to 64-bits to include VMID.
struct asid_t {
    // Bit  31    : global indication
    // Bit  30    : Always 0
    // Bits 29..16: isolation ID (realms), 14 bits
    // Bits 15..0 : ASID, 16 bits
    int32_t _value;

    asid_t() = default;
    // Castable from/to int32_t
    asid_t(int32_t asid) noexcept : _value{asid} {};
    constexpr operator int32_t() const noexcept { return _value; }

    constexpr bool invalid() const noexcept { return _value == -1; }
    constexpr bool valid() const noexcept { return _value != -1; }

    constexpr bool global() const noexcept { return _value < 0; }
    void global(bool set) noexcept {
        _value = (_value & 0x7fffffff) | ((int32_t)set << 31);
    }

    constexpr int realm() const noexcept { return (_value >> 16) & 0x3fff; }
    void realm(int set) noexcept {
        _value = (_value & 0xc000ffff) | ((set & 0x3fff) << 16);
    }

    constexpr int asid() const noexcept { return _value & 0xffff; }
    void asid(int asid) noexcept {
        _value = (_value & 0xffff0000) | (asid & 0xffff);
    }

    constexpr int realm_asid() const noexcept { return _value & 0x7fffffff; }

    // Check if ASID matches in lookup.
    // this  : entry in TLB
    // search: entry to match
    bool match(asid_t search) const noexcept {
        // In different realm
        if (realm() != search.realm()) return false;
        // Global page always match
        if (global()) return true;
        // Otherwise need ASID match
        return asid() == search.asid();
    }

    // Check if ASID matches when flushing.
    // this : entry in TLB
    // flush: entry to flush
    bool match_flush(asid_t flush) const noexcept {
        // In different realm
        if (realm() != flush.realm()) return false;
        // Want global flush
        if (flush.global()) return true;
        // Global page never flushes if not global flush
        if (global()) return false;
        // Otherwise need ASID match
        return asid() == flush.asid();
    }
};

// Represent an TLB entry.
// This is the internally-used exchange formats between all different types of TLB.
struct tlb_entry_t {
    uint64_t vpn;
    uint64_t ppn;
    uint64_t pte;
    // -1 indicates this TLB entry is not valid
    // When not -1, bits (..16) are isolation ID (realms) and bits (15..0) are ASID.
    asid_t asid;
    int granularity;

    bool valid() const noexcept {
        return asid.valid();
    }

    void invalidate() noexcept {
        asid = -1;
    }
};

/*
 * Check if a request is permitted given a PTE.
 *
 * <0 -> Fail
 *  0 -> Success
 * >0 -> Need update PTE's accessed/dirty bits, the mask to be ORed is returned.
 */
int pte_permission_check(int pte, const tlbsim_req_t& req);

struct tlb_stats_t;

class TLB {
public:
    TLB* parent;
    tlb_stats_t* stats;
    // Associated hart ID. Only used for L1 cache to enforce L0 inclusion policy.
    // -1 should be used for non-L1 caches.
    int hartid;

    TLB(TLB* parent, tlb_stats_t* stats, int hartid): parent{parent}, stats{stats}, hartid{hartid} {}

    // Find an entry, and acquire a (possibly) fine-grained lock that prevents
    // any race to the entry.
    virtual bool find_and_lock(tlb_entry_t &entry) { return false; }

    // Relase a (possibly) find-grained lock for an entry.
    virtual void unlock(const tlb_entry_t &entry) {}

    // Insert an entry, and unlock.
    virtual void insert_and_unlock(const tlb_entry_t &entry) {}

    virtual void flush_local(asid_t asid, uint64_t vpn) {}

    virtual int access(tlb_entry_t &search, const tlbsim_req_t& req);

    virtual void flush(asid_t asid, uint64_t vpn) {
        flush_local(asid, vpn);
        parent->flush(asid, vpn);
    }
};

extern class PageWalker final: public TLB {
public:
    PageWalker(): TLB(nullptr, nullptr, -1) {}
    int access(tlb_entry_t &search, const tlbsim_req_t& req) override;
    void flush(asid_t asid, uint64_t vpn) override {}
} page_walker;

}

#endif // TLBSIM_TLB_H
