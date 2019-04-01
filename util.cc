/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#include <unistd.h>
#include <time.h>
#include <sys/times.h>

#include "util.h"

namespace tlbsim {

double get_cputime() noexcept {
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

}
