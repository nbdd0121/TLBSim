#include <iostream>
#include "api.h"
#include "config.h"
#include "offline.h"
#include "stats.h"
#include "assoc.h"

using namespace tlbsim;

tlbsim_client_t tlbsim_client;

int main(int argc, char *argv[]) {
    while (config_replayer->replay_step(config_stlb)) {}
}

