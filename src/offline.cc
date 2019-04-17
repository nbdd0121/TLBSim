/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#include <cassert>

#include "offline.h"

namespace tlbsim {

struct packet_t {
    union {
        struct {
            tlbsim_req_t req;
            tlb_entry_t search;
        } access;
        struct {
            asid_t asid;
            uint64_t vpn;
        } flush;
    };
    enum {
        ACCESS,
        FLUSH
    } tag;
};

int AccessLogger::access(tlb_entry_t &search, const tlbsim_req_t& req) {
    lock.lock();
    int ret = parent->access(search, req);
    packet_t packet;
    packet.access.req = req;
    packet.access.search = search;
    packet.tag = packet_t::ACCESS;
    os.write(reinterpret_cast<const char*>(&packet), sizeof(packet_t));
    lock.unlock();
    return ret;
}

void AccessLogger::flush(asid_t asid, uint64_t vpn) {
    lock.lock();
    packet_t packet;
    packet.flush.asid = asid;
    packet.flush.vpn = vpn;
    packet.tag = packet_t::FLUSH;
    os.write(reinterpret_cast<char*>(&packet), sizeof(packet_t));
    lock.unlock();
}

int LogReplayer::access(tlb_entry_t &search, const tlbsim_req_t& req) {
    assert(&req == &req_ut);
    search = search_ut;
    return pte_permission_check(search.pte, req);
}

bool LogReplayer::replay_step(TLB* target) {
    if (!is) return false;
    packet_t packet;
    if (!is.read(reinterpret_cast<char*>(&packet), sizeof(packet_t))) return false;
    switch (packet.tag) {
        case packet_t::ACCESS: {
            req_ut = packet.access.req;
            search_ut = packet.access.search;
            tlb_entry_t entry;
            entry.asid = req_ut.asid;
            entry.vpn = req_ut.vpn;
            target->access(entry, req_ut);
            break;
        }
        case packet_t::FLUSH: {
            target->flush(packet.flush.asid, packet.flush.vpn);
            break;
        }
        default: assert(0);
    }
    return true;
}

}
