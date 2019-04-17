/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 *
 * This header provides utility for offline simulation.
 */

#ifndef TLBSIM_OFFLINE_H
#define TLBSIM_OFFLINE_H

#include <fstream>

#include "tlb.h"
#include "util.h"

namespace tlbsim {

// The TLB architecture when using AccessLogger:
//     ISASim --> L1 --> AccessLogger --> PageWalker
class AccessLogger: public TLB {
private:
    std::ofstream os;
    Spinlock lock;
public:
    AccessLogger(TLB* parent, std::ofstream&& os): TLB(parent, NULL, -1), os(std::move(os)) {}
    int access(tlb_entry_t &search, const tlbsim_req_t& req) override;
    void flush(asid_t asid, uint64_t vpn) override;
};

// The TLB architecture when using AccessLogger:
//     LogReplayer (initiater) --> DUT --> LogReplayer (replayer)
class LogReplayer: public TLB {
private:
    std::ifstream is;
    tlb_entry_t search_ut;
    tlbsim_req_t req_ut;
public:
    LogReplayer(std::ifstream&& is): TLB(NULL, NULL, -1), is(std::move(is)) {}
    int access(tlb_entry_t &search, const tlbsim_req_t& req) override;
    void flush(asid_t asid, uint64_t vpn) override {}

    // Read and replay one single entry from input stream.
    bool replay_step(TLB* target);
};

}

#endif
