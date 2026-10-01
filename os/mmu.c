#include <stdint.h>

#define DESC_BLOCK 0x1ULL
#define DESC_TABLE 0x3ULL
#define DESC_AF (1ULL << 10)
#define DESC_SH_INNER (3ULL << 8)
#define DESC_AP_RW_EL1 (0ULL << 6)
#define DESC_ATTRINDX(n) ((uint64_t)(n) << 2)
#define DESC_PXN (1ULL << 53)
#define DESC_UXN (1ULL << 54)

__attribute__((aligned(4096))) static uint64_t l1_table[512];
__attribute__((aligned(4096))) static uint64_t l2_low[512];

void uart_puts_public(const char *);

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

    uint64_t mair = 0x04ULL << 8 | 0xffULL;
    uint64_t tcr = 34ULL | (1ULL << 8) | (1ULL << 10) |
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
}
