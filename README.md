# TLBSim
Fast TLB simulator for RISC-V systems

## pkg-config

To use with pkg-config, you can add a `tlbsim.pc` file to your pkgconfig's directory:
```
libdir=<path to TLBSim root directory here>

Name: TLBSim
Description: TLB Simulation Framework
Version: 0.0.1
Libs: -L${libdir} -ltlbsim
Cflags: -I${libdir}/api
```

## QEMU Integration

Currently integration with QEMU is provided. Checkout qemu/ for details.

## Usage

Set `TLB_CONFIG` environment to a config file. Config file needs to be valid json (comments are
accepted). Here are the options which can be configured in the config file:
* `need_instret`, `need_minstret`: whether instruction count is needed. Can be turned off to increase
  performance if you do not need it. `true` by default.
* `cache_invalidate_entries`: whether invalid translation can be cached in the TLB. Only needed for
  validation purposes. `false` by default.
* `hardware_pte_update`: whether dirty and access bits are updated by hardware. If set to false,
  if a page table entry needs update, a page fault is triggered. `true` by default.
* `stlb`, `ctlb`, `itlb`, `dtlb`: shared TLB, per-core TLB, per-core instruction TLB, per-core data
  TLB. Each should be an array of TLB descriptors. Each descriptor has a "type" field with optional
  parameters. Types could be:
  - assoc: Fully associative TLB. Has parameter `size`.
  - set: Set-associative TLB. Has parameter `assoc` for associativity and `size`.
  - ideal: An infinite sized TLB.

  There are other special purpose "TLB"s:
  - isolate: Can only be used in `ctlb`. It separate TLB accesses to different `realms` for
    different cores. It is used to simulate a shared TLB with non-global ASID space semantics.
  - validate: Check if use of virtual memory system is valid. Warning messages will be printed for
    possibly invalid usage.

You can find example config files in configs/ directory.
