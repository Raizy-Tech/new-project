#include <stdint.h>
#include "pmm.h"

#define DESC_BLOCK 0x1ULL
#define DESC_PAGE 0x3ULL
#define DESC_TABLE 0x3ULL
#define DESC_AF (1ULL << 10)
#define DESC_SH_INNER (3ULL << 8)
#define DESC_AP_RW_EL1 (0ULL << 6)
#define DESC_AP_RO_EL1 (2ULL << 6)
#define DESC_AP_RW_EL0 (1ULL << 6)
#define DESC_AP_RO_EL0 (3ULL << 6)
#define DESC_ATTRINDX(n) ((uint64_t)(n) << 2)
#define DESC_PXN (1ULL << 53)
#define DESC_UXN (1ULL << 54)

__attribute__((aligned(4096))) static uint64_t l1_table[512];
__attribute__((aligned(4096))) static uint64_t l2_low[512];
__attribute__((aligned(4096))) static uint64_t l2_ram[512];
__attribute__((aligned(4096))) static uint64_t l2_vm[512];
__attribute__((aligned(4096))) static uint64_t l3_vm[512];

void uart_puts_public(const char *);

static void vm_zero_page(uintptr_t page) {
    uint64_t *p = (uint64_t *)page;
    for (uint32_t i = 0; i < 512; ++i) p[i] = 0;
}

static uint64_t *vm_l2(uint64_t *root, uint32_t l1i) {
    if (!(root[l1i] & DESC_TABLE)) return 0;
    return (uint64_t *)(uintptr_t)(root[l1i] & ~0xfffULL);
}

static uint64_t *vm_l3_create(uint64_t *l2, uint32_t l2i) {
    if (!(l2[l2i] & DESC_TABLE)) {
        uintptr_t page = pmm_alloc_page();
        if (!page) return 0;
        vm_zero_page(page);
        l2[l2i] = (uint64_t)page | DESC_TABLE;
    }
    return (uint64_t *)(uintptr_t)(l2[l2i] & ~0xfffULL);
}

uintptr_t vm_create_address_space(void) {
    uintptr_t root_pa = pmm_alloc_page();
    if (!root_pa) return 0;
    vm_zero_page(root_pa);
    uint64_t *root = (uint64_t *)root_pa;
    for (uint32_t i = 0; i < 512; ++i) root[i] = l1_table[i];
    return root_pa;
}

void vm_switch_address_space(uintptr_t root_pa) {
    if (!root_pa || (root_pa & 0xfffULL)) return;
    __asm__ volatile(
        "dsb sy\n"
        "msr ttbr0_el1, %0\n"
        "dsb sy\n"
        "tlbi vmalle1\n"
        "dsb sy\n"
        "isb" :: "r"(root_pa) : "memory");
}

uintptr_t vm_current_kernel_root(void) {
    return (uintptr_t)l1_table;
}

int vm_map_user_page_in(uintptr_t root_pa, uintptr_t va, uintptr_t pa, int writable, int executable) {
    if (!root_pa || (root_pa & 0xfffULL) || (va & 0xfffULL) || (pa & 0xfffULL)) return 0;
    if (va < 0x80000000ULL || va >= 0xc0000000ULL) return 0;

    uint64_t *root = (uint64_t *)root_pa;
    uint32_t l1i = (uint32_t)((va >> 30) & 0x1ff);
    uint32_t l2i = (uint32_t)((va >> 21) & 0x1ff);
    uint32_t l3i = (uint32_t)((va >> 12) & 0x1ff);
    uint64_t *l2 = vm_l2(root, l1i);

    if (!l2) {
        uintptr_t page = pmm_alloc_page();
        if (!page) return 0;
        vm_zero_page(page);
        root[l1i] = (uint64_t)page | DESC_TABLE;
        l2 = (uint64_t *)page;
    }

    uint64_t *l3 = vm_l3_create(l2, l2i);
    if (!l3) return 0;

    uint64_t ap = writable ? DESC_AP_RW_EL0 : DESC_AP_RO_EL0;
    uint64_t pxn = executable ? 0 : DESC_PXN;
    l3[l3i] = (uint64_t)pa | DESC_PAGE | DESC_AF | ap |
              DESC_ATTRINDX(0) | DESC_SH_INNER | pxn | DESC_UXN;
    if (executable) l3[l3i] &= ~DESC_UXN;
    return 1;
}


static void vm_tlb_flush(uintptr_t va) {
    uint64_t operand = (uint64_t)(va >> 12);
    __asm__ volatile("dsb ishst\ntlbi vae1is, %0\ndsb ish\nisb" :: "r"(operand) : "memory");
}

int vm_map_user_page(uintptr_t va, uintptr_t pa, int writable, int executable) {
    int ok = vm_map_user_page_in((uintptr_t)l1_table, va, pa, writable, executable);
    if (ok) vm_tlb_flush(va);
    return ok;
}


int vm_map_page(uintptr_t va, uintptr_t pa) {
    if ((va & 0xfffULL) || (pa & 0xfffULL)) return 0;
    if (va < 0x80000000ULL || va >= 0xc0000000ULL) return 0;
    uint32_t l1i = (uint32_t)((va >> 30) & 0x1ff);
    uint32_t l2i = (uint32_t)((va >> 21) & 0x1ff);
    uint32_t l3i = (uint32_t)((va >> 12) & 0x1ff);
    uint64_t *l2 = vm_l2(l1_table, l1i);
    if (!l2) return 0;
    uint64_t *l3 = vm_l3_create(l2, l2i);
    if (!l3) return 0;
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
    uint64_t *l2 = vm_l2(l1_table, l1i);
    if (!l2 || !(l2[l2i] & DESC_TABLE)) return 0;
    uint64_t *l3 = (uint64_t *)(uintptr_t)(l2[l2i] & ~0xfffULL);
    if (!(l3[l3i] & DESC_PAGE)) return 0;
    return (uintptr_t)(l3[l3i] & ~0xfffULL);
}

int vm_unmap_page(uintptr_t va) {
    if ((va & 0xfffULL) || va < 0x80000000ULL || va >= 0xc0000000ULL) return 0;
    uint32_t l1i = (uint32_t)((va >> 30) & 0x1ff);
    uint32_t l2i = (uint32_t)((va >> 21) & 0x1ff);
    uint32_t l3i = (uint32_t)((va >> 12) & 0x1ff);
    uint64_t *l2 = vm_l2(l1_table, l1i);
    if (!l2 || !(l2[l2i] & DESC_TABLE)) return 0;
    uint64_t *l3 = (uint64_t *)(uintptr_t)(l2[l2i] & ~0xfffULL);
    if (!(l3[l3i] & DESC_PAGE)) return 0;
    l3[l3i] = 0;
    vm_tlb_flush(va);
    if (l3 != l3_vm) {
        uint32_t empty = 1;
        for (uint32_t i = 0; i < 512; ++i)
            if (l3[i] & DESC_PAGE) { empty = 0; break; }
        if (empty) {
            l2[l2i] = 0;
            pmm_free_page((uintptr_t)l3);
        }
    }
    return 1;
}

void mmu_init(void) {
    for (uint32_t i = 0; i < 512; ++i) l1_table[i] = 0;
    for (uint32_t i = 0; i < 512; ++i) {
        uint64_t pa = (uint64_t)i * 0x200000ULL;
        uint64_t attr = DESC_AF | DESC_AP_RW_EL1;
        if (pa >= 0x08000000ULL && pa < 0x0a000000ULL)
            attr |= DESC_ATTRINDX(1) | DESC_PXN | DESC_UXN;
        else
            attr |= DESC_ATTRINDX(0) | DESC_SH_INNER;
        if (pa < 0x40000000ULL) attr |= DESC_PXN | DESC_UXN;
        l2_low[i] = pa | DESC_BLOCK | attr;
    }
    l1_table[0] = ((uint64_t)l2_low) | DESC_TABLE;

    for (uint32_t i = 0; i < 512; ++i) {
        uint64_t pa = 0x40000000ULL + (uint64_t)i * 0x200000ULL;
        l2_ram[i] = pa | DESC_BLOCK | DESC_AF | DESC_AP_RW_EL1 |
                    DESC_ATTRINDX(0) | DESC_SH_INNER;
    }
    l1_table[1] = ((uint64_t)l2_ram) | DESC_TABLE;

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
