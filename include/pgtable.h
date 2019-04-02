/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 */

#ifndef PGTABLE_H
#define PGTABLE_H

#define PTE_V 0x01
#define PTE_R 0x02
#define PTE_W 0x04
#define PTE_X 0x08
#define PTE_U 0x10
#define PTE_G 0x20
#define PTE_A 0x40
#define PTE_D 0x80

#define SATP_MODE 0xF000000000000000ULL
#define SATP_ASID 0x0FFFF00000000000ULL
#define SATP_PPN  0x00000FFFFFFFFFFFULL

#define SATP_MODE_NONE 0ULL
#define SATP_MODE_SV39 0x8000000000000000ULL
#define SATP_MODE_SV48 0x9000000000000000ULL

#endif // PGTABLE_H
