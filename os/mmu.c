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
    root[0] = l1_table[0];
    root[1] = l1_table[1];
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

