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

    // Castable from/to int32_t
    asid_t(int32_t asid) noexcept : _value{asid} {};
    constexpr operator int32_t() const noexcept { return _value; }

    constexpr bool invalid() const noexcept { return _value == -1; }
    constexpr bool valid() const noexcept { return _value != -1; }

    constexpr bool global() const noexcept { return _value < 0; }
    constexpr int realm() const noexcept { return (_value >> 16) & 0x3fff; }
    constexpr int asid() const noexcept { return _value & 0xffff; }
};

// Represent an TLB entry.
// This is the internally-used exchange formats between all different types of TLB.
struct tlb_entry_t {
    uint64_t vpn;
    uint64_t ppn;
    uint64_t pte;
    // -1 indicates this TLB entry is not valid
    // When not -1, bits (..16) are isolation ID (realms) and bits (15..0) are ASID.
    asid_t asid = -1;
    int granularity;

    bool valid() const noexcept {
        return asid.valid();
    }

    void invalidate() noexcept {
        asid = -1;
    }

    // Check whether an ASID match
    bool asid_match(asid_t asid) const noexcept {
        // In different realm
        if (this->asid.realm() != asid.realm()) return false;
        // Global page always match
        if (this->pte & PTE_G) return true;
        // Otherwise need ASID match
        return this->asid.asid() == asid.asid();
    }

    // Check whether an ASID match when flushing
    bool asid_match_flush(asid_t asid) const noexcept {
        // In different realm
        if (this->asid.realm() != asid.realm()) return false;
        // Want global flush
        if (asid.asid() == 0) return true;
        // Global page never flushes if not global flush
        if (this->pte & PTE_G) return false;
        // Otherwise need ASID match
        return this->asid.asid() == asid.asid();
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
