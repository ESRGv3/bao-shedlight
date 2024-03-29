/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Sandro Pinto <sandro.pinto@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#ifndef __VM_H__
#define __VM_H__

#include <bao.h>
#include <arch/vm.h>

#include <platform.h>
#include <mem.h>
#include <cpu.h>
#include <spinlock.h>
#include <config.h>
#include <emul.h>
#include <objcache.h>
#include <interrupts.h>
#include <bitmap.h>
#include <iommu.h>
#include <ipc.h>

struct vm {
    vmid_t id;

    const struct vm_config* config;

    spinlock_t lock;
    struct cpu_synctoken sync;
    cpuid_t master;

    struct list vcpu_list;
    size_t cpu_num;
    cpumap_t cpus;

    struct addr_space as;

    struct vm_arch arch;

    struct list emul_list;
    struct objcache emul_oc;

    struct iommu_vm iommu;

    BITMAP_ALLOC(interrupt_bitmap, MAX_INTERRUPTS);

    size_t ipc_num;
    struct ipc *ipcs;
};

struct vcpu {
    node_t node;

    struct arch_regs* regs;
    struct vcpu_arch arch;

    vcpuid_t id;
    cpuid_t phys_id;
    bool active;

    struct vm* vm;

    uint8_t stack[STACK_SIZE] __attribute__((aligned(STACK_SIZE)));
};

extern struct vm vm;
extern struct config* vm_config_ptr;

void vm_init(struct vm* vm, const struct vm_config* config, bool master, vmid_t vm_id);
void vm_start(struct vm* vm, vaddr_t entry);
struct vcpu* vm_get_vcpu(struct vm* vm, vcpuid_t vcpuid);
void vm_emul_add_mem(struct vm* vm, struct emul_mem* emu);
void vm_emul_add_reg(struct vm* vm, struct emul_reg* emu);
emul_handler_t vm_emul_get_mem(struct vm* vm, vaddr_t addr);
emul_handler_t vm_emul_get_reg(struct vm* vm, vaddr_t addr);
void vcpu_init(struct vcpu* vcpu, struct vm* vm, vaddr_t entry);
void vm_msg_broadcast(struct vm* vm, struct cpu_msg* msg);
cpumap_t vm_translate_to_pcpu_mask(struct vm* vm, cpumap_t mask, size_t len);
cpumap_t vm_translate_to_vcpu_mask(struct vm* vm, cpumap_t mask, size_t len);

static inline cpuid_t vm_translate_to_pcpuid(struct vm* vm, vcpuid_t vcpuid)
{
    ssize_t i = bitmap_find_nth((bitmap_t*)&vm->cpus, sizeof(vm->cpus) * 8,
                           vcpuid + 1, 0, true);
    if(i < 0) {
        return INVALID_CPUID;
    } else {
        return (cpuid_t)i;
    }
}

static inline vcpuid_t vm_translate_to_vcpuid(struct vm* vm, cpuid_t pcpuid)
{
    if (vm->cpus & (1UL << pcpuid)) {
        return (cpuid_t)bitmap_count((bitmap_t*)&vm->cpus, 0, pcpuid, true);
    } else {
        return INVALID_CPUID;
    }
}

static inline bool vm_has_interrupt(struct vm* vm, irqid_t int_id)
{
    return !!bitmap_get(vm->interrupt_bitmap, int_id);
}

static inline void vcpu_inject_hw_irq(struct vcpu *vcpu, irqid_t id)
{
    vcpu_arch_inject_hw_irq(vcpu, id);
}

static inline void vcpu_inject_irq(struct vcpu *vcpu, irqid_t id)
{
    vcpu_arch_inject_irq(vcpu, id);
}

/* ------------------------------------------------------------*/

void vm_arch_init(struct vm* vm, const struct vm_config* config);
void vcpu_arch_init(struct vcpu* vcpu, struct vm* vm);
void vcpu_run(struct vcpu* vcpu);
unsigned long vcpu_readreg(struct vcpu* vcpu, unsigned long reg);
void vcpu_writereg(struct vcpu* vcpu, unsigned long reg, unsigned long val);
unsigned long vcpu_readpc(struct vcpu* vcpu);
void vcpu_writepc(struct vcpu* vcpu, unsigned long pc);
void vcpu_arch_run(struct vcpu* vcpu);
void vcpu_arch_reset(struct vcpu* vcpu, vaddr_t entry);

#endif /* __VM_H__ */
