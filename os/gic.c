#include <stdint.h>
#include "gic.h"

static uintptr_t gicd_base = 0x08000000UL;
static uintptr_t gicr_base = 0x080A0000UL

#define GICD_CTLR 0x0000
#define GICR_WAKER 0x0014
#define GICR_SGI_BASE 0x10000
#define GICR_IGROUPR0 (GICR_SGI_BASE + 0x0080)
#define GICR_ISENABLER0 (GICR_SGI_BASE + 0x0100)
#define GICR_IPRIORITYR0 (GICR_SGI_BASE + 0x0400)

#define GICR_WAKER_PROCESSOR_SLEEP (1u << 1)
#define GICR_WAKER_CHILDREN_ASLEEP (1u << 2)
#define GICD_CTLR_ENABLE_G1NS (1u << 1)


static inline void mmio_write32(uintptr_t addr, uint32_t value) {
    *(volatile uint32_t *)addr = value;
}

static inline uint32_t mmio_read32(uintptr_t addr) {
    return *(volatile uint32_t *)addr;
}

void gic_init(uintptr_t distributor_base, uintptr_t redistributor_base, uint32_t ppi) {
    gicd_base = distributor_base;
    gicr_base = redistributor_base;
    timer_ppi = ppi;
    uintptr_t r = gicr_base;

    uint32_t waker = mmio_read32(r + GICR_WAKER);
    waker &= ~GICR_WAKER_PROCESSOR_SLEEP;
    mmio_write32(r + GICR_WAKER, waker);

    while (mmio_read32(r + GICR_WAKER) & GICR_WAKER_CHILDREN_ASLEEP) {}

    /* SGIs and PPIs are Group 1 Non-secure. */
    mmio_write32(r + GICR_IGROUPR0, 0xffffffffu);

    /* Use a priority accepted by the CPU interface. */
    for (uint32_t i = 0; i < 32; i += 4)
        mmio_write32(r + GICR_IPRIORITYR0 + i, 0xa0a0a0a0u);

    /* Enable the physical timer PPI, INTID 30. */
    mmio_write32(r + GICR_ISENABLER0, 1u << timer_ppi);

    /* Enable Group 1 at the distributor. */
    uint32_t ctlr = mmio_read32(gicd_base + GICD_CTLR);
    ctlr |= GICD_CTLR_ENABLE_G1NS;
    mmio_write32(GICD_BASE + GICD_CTLR, ctlr);

    /* Enable the GICv3 system-register CPU interface. */
    uint64_t sre = 7;
    __asm__ volatile("msr icc_sre_el1, %0\nisb" :: "r"(sre) : "memory");

    uint64_t pmr = 0xff;
    __asm__ volatile("msr icc_pmr_el1, %0" :: "r"(pmr) : "memory");

    uint64_t igrpen = 1;
    __asm__ volatile("msr icc_igrpen1_el1, %0\nisb" :: "r"(igrpen) : "memory");
}

void gic_enable_ppi(uint32_t intid) {
    if (intid < 16 || intid > 31) return;
    mmio_write32(gicr_base + GICR_ISENABLER0, 1u << intid);
}

void gic_ack(uint32_t *intid) {
    uint64_t value;
    __asm__ volatile("mrs %0, icc_iar1_el1" : "=r"(value));
    *intid = (uint32_t)value;
}

void gic_eoi(uint32_t intid) {
    uint64_t value = intid;
    __asm__ volatile("msr icc_eoir1_el1, %0\nisb" :: "r"(value) : "memory");
}
