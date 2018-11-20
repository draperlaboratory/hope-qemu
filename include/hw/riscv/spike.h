/*
 * Spike machine interface
 *
 * Copyright (c) 2017 SiFive, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HW_RISCV_SPIKE_H
#define HW_RISCV_SPIKE_H

typedef struct {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    RISCVHartArrayState soc;
    DeviceState *plic;
    void *fdt;
    int fdt_size;
} SpikeState;

enum {
    SPIKE_MROM,
    SPIKE_CLINT,
    SPIKE_PLIC,
    SPIKE_DRAM
};

enum {
    SPIKE_CLOCK_FREQ = 1000000000
};

#define SPIKE_PLIC_HART_CONFIG "MS"
#define SPIKE_PLIC_NUM_SOURCES 127
#define SPIKE_PLIC_NUM_PRIORITIES 7
#define SPIKE_PLIC_PRIORITY_BASE 0x0
#define SPIKE_PLIC_PENDING_BASE 0x1000
#define SPIKE_PLIC_ENABLE_BASE 0x2000
#define SPIKE_PLIC_ENABLE_STRIDE 0x80
#define SPIKE_PLIC_CONTEXT_BASE 0x200000
#define SPIKE_PLIC_CONTEXT_STRIDE 0x1000
#if defined(TARGET_RISCV32)
#define SPIKE_V1_09_1_CPU TYPE_RISCV_CPU_RV32GCSU_V1_09_1
#define SPIKE_V1_10_0_CPU TYPE_RISCV_CPU_RV32GCSU_V1_10_0
#elif defined(TARGET_RISCV64)
#define SPIKE_V1_09_1_CPU TYPE_RISCV_CPU_RV64GCSU_V1_09_1
#define SPIKE_V1_10_0_CPU TYPE_RISCV_CPU_RV64GCSU_V1_10_0
#endif

#endif
