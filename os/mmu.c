#include <stdint.h>
#include "pmm.h"

#define DESC_BLOCK 0x1ULL
#define DESC_PAGE 0x3ULL
#define DESC_TABLE 0x3ULL
#define DESC_AF (1ULL << 10)
#define DESC_SH_INNER (3ULL << 8)
#define DESC_AP_RW_EL1 (0ULL << 6)
#define DESC_ATTRINDX(n) ((uint64_t)(n) << 2)
#define DESC_PXN (1ULL << 53)
#define DESC_UXN (1ULL << 54)

__attribute__((aligned(4096))) static uint64_t l1_table[512];
__attribute__((aligned(4096))) static uint64_t l2_low[512];
__attribute__((aligned(4096))) static uint64_t l2_ram[512];
__attribute__((aligned(4096))) static uint64_t l2_vm[512];
__attribute__((aligned(4096))) static uint64_t l3_vm[512];

void uart_puts_public(const char *);

static void vm_tlb_flush(uintptr_t va) {
    uint64_t operand = (uint64_t)(va >> 12);
    __asm__ volatile("dsb ishst\ntlbi vae1is, %0\ndsb ish\nisb" :: "r"(operand) : "memory");
}

int vm_map_page(uintptr_t va, uintptr_t pa) {
    if ((va & 0xfffULL) || (pa & 0xfffULL)) return 0;
    if (va < 0x80000000ULL || va >= 0xc0000000ULL) return 0;

    uint32_t l1i = (uint32_t)((va >> 30) & 0x1ff);
    uint32_t l2i = (uint32_t)((va >> 21) & 0x1ff);
    uint32_t l3i = (uint32_t)((va >> 12) & 0x1ff);

    uint64_t *l1 = l1_table;
    if (!(l1[l1i] & DESC_TABLE)) return 0;
    uint64_t *l2 = (uint64_t *)(uintptr_t)(l1[l1i] & ~0xfffULL);

    if (!(l2[l2i] & DESC_TABLE)) {
        uintptr_t page = pmm_alloc_page();
        if (!page) return 0;
        uint64_t *l3 = (uint64_t *)page;
        for (uint32_t i = 0; i < 512; ++i) l3[i] = 0;
        l2[l2i] = ((uint64_t)l3) | DESC_TABLE;
    }

    uint64_t *l3 = (uint64_t *)(uintptr_t)(l2[l2i] & ~0xfffULL);
    l3[l3i] = (uint64_t)pa | DESC_PAGE | DESC_AF | DESC_AP_RW_EL1 |
              DESC_ATTRINDX(0) | DESC_SH_INNER;
    vm_tlb_flush(va);
    return 1;
}

uintptr_t vm_get_pa(uintptr_t va) {
    if ((va & 0xfffULL) || va < 0x80000000ULL || va >= 0xc0000000ULL) return 0;
    uint32_t l1i = (uint32_t)((va >> 30) & 0x1ff);
    uint32_t l2i = (uint32_t)((va >> 21) & 0x1ff);
    uint32_t l3i = (uint32_t)((va >> 12) & 0x1ff);
    if (!(l1_table[l1i] & DESC_TABLE)) return 0;
    uint64_t *l2 = (uint64_t *)(uintptr_t)(l1_table[l1i] & ~0xfffULL);
    if (!(l2[l2i] & DESC_TABLE)) return 0;
    uint64_t *l3 = (uint64_t *)(uintptr_t)(l2[l2i] & ~0xfffULL);
    if (!(l3[l3i] & DESC_PAGE)) return 0;
    return (uintptr_t)(l3[l3i] & ~0xfffULL);
}

int vm_unmap_page(uintptr_t va) {
    if ((va & 0xfffULL) || va < 0x80000000ULL || va >= 0xc0000000ULL) return 0;
    uint32_t l1i = (uint32_t)((va >> 30) & 0x1ff);
    uint32_t l2i = (uint32_t)((va >> 21) & 0x1ff);
    uint32_t l3i = (uint32_t)((va >> 12) & 0x1ff);
    if (!(l1_table[l1i] & DESC_TABLE)) return 0;
    uint64_t *l2 = (uint64_t *)(uintptr_t)(l1_table[l1i] & ~0xfffULL);
    if (!(l2[l2i] & DESC_TABLE)) return 0;
    uint64_t *l3 = (uint64_t *)(uintptr_t)(l2[l2i] & ~0xfffULL);
    if (!(l3[l3i] & DESC_PAGE)) return 0;
    l3[l3i] = 0;
    vm_tlb_flush(va);
    return 1;
}

void mmu_init(void) {
    for (uint32_t i = 0; i < 512; ++i) l1_table[i] = 0;

    for (uint32_t i = 0; i < 512; ++i) {
        uint64_t pa = (uint64_t)i * 0x200000ULL;
        uint64_t attr = DESC_AF | DESC_AP_RW_EL1;

        /* Keep QEMU virt's GIC and UART MMIO ranges as Device memory. */
        if (pa >= 0x08000000ULL && pa < 0x0a000000ULL)
            attr |= DESC_ATTRINDX(1) | DESC_PXN | DESC_UXN;
        else
            attr |= DESC_ATTRINDX(0) | DESC_SH_INNER;

        /* Kernel RAM is executable so the identity-mapped image can continue. */
        if (pa < 0x40000000ULL)
            attr |= DESC_PXN | DESC_UXN;

        l2_low[i] = pa | DESC_BLOCK | attr;
    }

    l1_table[0] = ((uint64_t)l2_low) | DESC_TABLE;

    /* VA 1GB..2GB -> PA 1GB..2GB, where the kernel and RAM live. */
    for (uint32_t i = 0; i < 512; ++i) {
        uint64_t pa = 0x40000000ULL + (uint64_t)i * 0x200000ULL;
        l2_ram[i] = pa | DESC_BLOCK | DESC_AF | DESC_AP_RW_EL1 |
                    DESC_ATTRINDX(0) | DESC_SH_INNER;
    }
    l1_table[1] = ((uint64_t)l2_ram) | DESC_TABLE;

    /* VA 0x80000000 -> L2 table -> L3 page table for a 4 KiB mapping. */
    for (uint32_t i = 0; i < 512; ++i) { l2_vm[i] = 0; l3_vm[i] = 0; }
    l1_table[2] = ((uint64_t)l2_vm) | DESC_TABLE;
    l2_vm[0] = ((uint64_t)l3_vm) | DESC_TABLE;
    l3_vm[0] = 0x40300000ULL | DESC_PAGE | DESC_AF | DESC_AP_RW_EL1 |
               DESC_ATTRINDX(0) | DESC_SH_INNER;

    uint64_t mair = 0x04ULL << 8 | 0xffULL;
    uint64_t tcr = 32ULL | (1ULL << 8) | (1ULL << 10) |
                   (3ULL << 12) | (5ULL << 32);

    __asm__ volatile("msr mair_el1, %0" :: "r"(mair) : "memory");
    __asm__ volatile("msr tcr_el1, %0" :: "r"(tcr) : "memory");
    __asm__ volatile("msr ttbr0_el1, %0" :: "r"(l1_table) : "memory");
    __asm__ volatile("dsb sy\ntlbi vmalle1\ndsb sy\nisb" ::: "memory");

    uint64_t sctlr;
    __asm__ volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    sctlr |= (1ULL << 0) | (1ULL << 2) | (1ULL << 12);
    __asm__ volatile("msr sctlr_el1, %0\nisb" :: "r"(sctlr) : "memory");
}

void mmu_test(void) {
    volatile uint64_t *ram = (volatile uint64_t *)0x40100000ULL;
    const uint64_t pattern = 0x5241495a594f5341ULL;
    *ram = pattern;
    if (*ram == pattern)
        uart_puts_public("MMU: identity-mapped RAM read/write test PASSED.\n");

    volatile uint64_t *mapped = (volatile uint64_t *)0x80000000ULL;
    *mapped = 0x5241495a594f564dULL;
    if (*mapped == 0x5241495a594f564dULL)
        uart_puts_public("VM: 4 KiB page mapping read/write test PASSED.\n");
}
