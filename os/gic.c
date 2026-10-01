#include <stdint.h>
#include "gic.h"

#define GICD_BASE 0x08000000UL
#define GICR_BASE 0x080A0000UL
#define GICR_STRIDE 0x20000UL

#define GICD_CTLR 0x0000
#define GICD_ISENABLER 0x0100
#define GICD_IPRIORITYR 0x0400

#define GICR_WAKER 0x0014
#define GICR_IGROUPR0 0x0080
#define GICR_ISENABLER0 0x0100
#define GICR_IPRIORITYR 0x0400

#define GICR_WAKER_PROCESSOR_SLEEP (1u << 1)
#define GICR_WAKER_CHILDREN_ASLEEP (1u << 2)

#define GICD_CTLR_ENABLE_G1NS (1u << 1)

static inline void mmio_write32(uintptr_t addr, uint32_t value) {
    *(volatile uint32_t *)addr = value;
}
static inline uint32_t mmio_read32(uintptr_t addr) {
    return *(volatile uint32_t *)addr;
}

static uintptr_t gicr(void) {
    return GICR_BASE;
}

void gic_init(void) {
    uintptr_t r = gicr();

    uint32_t waker = mmio_read32(r + GICR_WAKER);
    waker &= ~GICR_WAKER_PROCESSOR_SLEEP;
    mmio_write32(r + GICR_WAKER, waker);

    for (;;) {
        if (!(mmio_read32(r + GICR_WAKER) & GICR_WAKER_CHILDREN_ASLEEP))
            break;
    }

    /* PPI 0..15 and SGI 0..15 are Group 1 Non-secure. */
    mmio_write32(r + GICR_IGROUPR0, 0xffffffffu);

    /* Default PPI priority: 0xa0. */
    for (uint32_t i = 0; i < 32; i += 4)
        mmio_write32(r + GICR_IPRIORITYR + i, 0xa0a0a0a0u);

    /* Enable the timer PPI (INTID 30). */
    mmio_write32(r + GICR_ISENABLER0, 1u << 30);

    /* Enable Group 1 interrupts in the distributor. */
    uint32_t ctlr = mmio_read32(GICD_BASE + GICD_CTLR);
    ctlr |= GICD_CTLR_ENABLE_G1NS;
    mmio_write32(GICD_BASE + GICD_CTLR, ctlr);

    /*
     * ICC_SRE_EL1: permit system-register interface.
     * ICC_PMR_EL1: accept priorities numerically <= 0xff.
     * ICC_IGRPEN1_EL1: enable Group 1 delivery.
     */
    uint64_t sre = 7;
    __asm__ volatile("msr icc_sre_el1, %0\nisb" :: "r"(sre) : "memory");
    uint64_t pmr = 0xff;
    __asm__ volatile("msr icc_pmr_el1, %0" :: "r"(pmr));
    uint64_t igrpen = 1;
    __asm__ volatile("msr icc_igrpen1_el1, %0\nisb" :: "r"(igrpen) : "memory");
}

void gic_enable_ppi(uint32_t intid) {
    if (intid < 16 || intid > 31) return;
    mmio_write32(gicr() + GICR_ISENABLER0, 1u << intid);
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
