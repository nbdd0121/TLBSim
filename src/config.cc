/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <json/json.h>

#include "api.h"
#include "config.h"

#include "stats.h"
#include "tlb.h"
#include "assoc.h"
#include "ideal.h"
#include "validator.h"
#include "offline.h"

namespace tlbsim {

bool config_cache_inv = false;
bool config_update_pte = true;
TLB* config_stlb;
TLB* config_ctlbs[32];
TLB* config_itlbs[32];
TLB* config_dtlbs[32];
LogReplayer* config_replayer;

static Json::Value itlb_template;
static Json::Value dtlb_template;
static Json::Value ctlb_template;

class HartIsolator: public TLB {
    int hartid;
public:
    HartIsolator(TLB* shared, int hartid): TLB(shared, nullptr, -1), hartid(hartid) {}

    int access(tlb_entry_t &search, const tlbsim_req_t& req) {
        search.asid = (int32_t)search.asid | (hartid << 16);
        int perm = parent->access(search, req);
        search.asid = (int32_t)search.asid & 0xc000ffff;
        return perm;
    }

    void flush(asid_t asid, uint64_t vpn) override {
        asid = asid | (hartid << 16);
        parent->flush(asid, vpn);
    }
};

static Json::Value read_json(const char *path) {
    Json::Value json;
    std::ifstream file(path);
    if (!file) {
        std::cerr << "TLBSim: File " << path << " does not exist\n";
        exit(1);
    }
    try {
        file >> json;
    } catch (std::exception&) {
        std::cerr << "TLBSim: Error parsing " << path << " as json\n";
        exit(1);
    }
    return json;
}

// Verify the validity of the template, and print out the configuration
static void validate_template(Json::Value& tmpl, bool shared) {
    auto type = tmpl["type"].asString();
    fprintf(stderr, "  - type: %s\n", type.c_str());
    if (type == "assoc") {
        int size = tmpl["size"].asInt();
        fprintf(stderr, "    size: %d\n", size);
        return;
    }
    if (type == "set") {
        int assoc = tmpl.get("assoc", 8).asInt();
        int size = tmpl["size"].asInt();
        fprintf(stderr, "    assoc: %d\n", assoc);
        fprintf(stderr, "    size: %d\n", size);
        return;
    }
    if (type == "isolate") {
        if (shared) {
            fprintf(stderr, "TLBSim: Hart isolator cannot be used in shared context\n");
            exit(1);
        }
        return;
    }
    if (type == "ideal") {
        return;
    }
    if (type == "validate") {
        return;
    }
    if (type == "log") {
        if (!shared) {
            fprintf(stderr, "TLBSim: Access logger can only be used in shared context\n");
            exit(1);
        }
        const char* file = tmpl["file"].asCString();
        fprintf(stderr, "    file: %s\n", file);
        return;
    }
    fprintf(stderr, "TLBSim: %s is not an accepted TLB type\n", type.c_str());
    exit(1);
}

static TLB* instantiate(const Json::Value& tmpl, TLB* parent, tlb_stats_t* stats, int hartid, bool inv) {
    auto type = tmpl["type"].asString();
    if (type == "assoc") {
        int size = tmpl["size"].asInt();
        return new AssocTLB<>(parent, stats, inv ? hartid : -1, size);
    }
    if (type == "set") {
        int assoc = tmpl.get("assoc", 8).asInt();
        int size = tmpl["size"].asInt();
        return new SetAssocTLB<>(parent, stats, inv ? hartid : -1, size, assoc);
    }
    if (type == "isolate") {
        return new HartIsolator(parent, hartid);
    }
    if (type == "ideal") {
        return new IdealTLB(parent, stats);
    }
    if (type == "validate") {
        return new ASIDValidator(new TLBValidator(parent, stats));
    }
    if (type == "log") {
        const char* file = tmpl["file"].asCString();
        return new AccessLogger(parent, std::ofstream(file));
    }
    // Not reachable
    return nullptr;
}

__attribute__((constructor))
static void setup_env2(void) {
    char *config_file = getenv("TLB_CONFIG");
    Json::Value config_json = read_json(config_file ? config_file : "tlbsim.config");
    fprintf(stderr, "TLB Configuration:\n");
    tlbsim_need_instret = config_json.get("need_instret", true).asBool();
    fprintf(stderr, "  need_instret: %s\n", tlbsim_need_instret ? "true" : "false");
    tlbsim_need_minstret = config_json.get("need_minstret", true).asBool();
    fprintf(stderr, "  need_minstret: %s\n", tlbsim_need_minstret ? "true" : "false");
    config_cache_inv = config_json.get("cache_invalidate_entries", false).asBool();
    fprintf(stderr, "  cache_invalidate_entries: %s\n", config_cache_inv ? "true" : "false");
    config_update_pte = config_json.get("hardware_pte_update", true).asBool();
    fprintf(stderr, "  hardware_pte_update: %s\n", config_update_pte ? "true" : "false");

    auto& replay = config_json["replay"];
    if (replay.isString()) {
        config_replayer = new LogReplayer(std::ifstream(replay.asCString()));
        fprintf(stderr, "  replay: \"%s\"\n", replay.asCString());
    }

    auto validate = [&config_json](Json::Value& tmpl, const char* key) {
        tmpl.swap(config_json[key]);
        if (!tmpl.isArray()) {
            tmpl = Json::arrayValue;
        }
        auto size = tmpl.size();
        if (size == 0) {
            fprintf(stderr, "  %s: []\n", key);
        } else {
            fprintf(stderr, "  %s:\n", key);
            auto size = tmpl.size();
            for (int i = size - 1; i >= 0; i--) {
                validate_template(tmpl[i], false);
            }
        }
    };

    Json::Value stlb_template;
    validate(stlb_template, "stlb");
    validate(ctlb_template, "ctlb");
    validate(itlb_template, "itlb");
    validate(dtlb_template, "dtlb");

    // Instantiate shared eagerly
    config_stlb = config_replayer ? (TLB*)config_replayer : &page_walker;
    auto size = stlb_template.size();
    for (int i = size - 1; i >= 0; i--) {
        config_stlb = instantiate(stlb_template[i], config_stlb, &stlb_stats, -1, false);
    }
}

void setup_private_tlb(int hartid) {
    TLB *ctlb = config_stlb;
    auto size = ctlb_template.size();
    for (int i = size - 1; i >= 0; i--) {
        ctlb = instantiate(ctlb_template[i], ctlb, &ctlb_stats, hartid, i == 0 && itlb_template.empty() && dtlb_template.empty());
    }

    TLB *itlb = ctlb;
    size = itlb_template.size();
    for (int i = size - 1; i >= 0; i--) {
        itlb = instantiate(itlb_template[i], itlb, &itlb_stats, hartid, i == 0);
    }

    TLB *dtlb = ctlb;
    size = dtlb_template.size();
    for (int i = size - 1; i >= 0; i--) {
        dtlb = instantiate(dtlb_template[i], dtlb, &dtlb_stats, hartid, i == 0);
    }

    config_ctlbs[hartid] = ctlb;
    config_itlbs[hartid] = itlb;
    config_dtlbs[hartid] = dtlb;
}


}
