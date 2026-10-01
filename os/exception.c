#include <stdint.h>
#include "gic.h"
#include "timer.h"

static volatile uint32_t *const UARTDR = (volatile uint32_t *)0x09000000UL;
static volatile uint32_t *const UARTFR = (volatile uint32_t *)0x09000018UL;
#define UARTFR_TXFF (1u << 5)

static void uart_putc(char c) {
    while (*UARTFR & UARTFR_TXFF) {}
    *UARTDR = (uint32_t)c;
}

static void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

static void uart_hex64(uint64_t v) {
    static const char d[] = "0123456789abcdef";
    uart_puts("0x");
    for (int s = 60; s >= 0; s -= 4)
        uart_putc(d[(v >> s) & 0xf]);
}

void exception_sync_handler(uint64_t esr, uint64_t elr, uint64_t spsr, uint64_t far) {
    uint32_t ec = (uint32_t)(esr >> 26);
    uart_puts("\n[EXCEPTION] synchronous exception\n");
    uart_puts("  ESR_EL1 = "); uart_hex64(esr); uart_puts("\n");
    uart_puts("  ELR_EL1 = "); uart_hex64(elr); uart_puts("\n");
    uart_puts("  SPSR_EL1 = "); uart_hex64(spsr); uart_puts("\n");
    uart_puts("  FAR_EL1 = "); uart_hex64(far); uart_puts("\n");
    uart_puts("  EC = "); uart_hex64(ec); uart_puts("\n");
    if (ec == 0x15) uart_puts("  Cause: SVC instruction from AArch64.\n");
    else uart_puts("  Cause: unclassified synchronous exception.\n");
}

void exception_irq_handler(void) {
    uint32_t intid = 0;
    gic_ack(&intid);

    if (intid == 30) {
        timer_irq_handler();
        uint64_t ticks = timer_ticks();
        if ((ticks % 10) == 0) {
            uart_puts("[TIMER] tick ");
            uart_hex64(ticks);
            uart_puts("\n");
        }
    } else if (intid < 1020) {
        uart_puts("[IRQ] INTID = ");
        uart_hex64(intid);
        uart_puts("\n");
    }

    if (intid < 1020)
        gic_eoi(intid);
}

void exception_unhandled_handler(void) {
    uart_puts("\n[FATAL] unhandled exception vector\n");
    for (;;) __asm__ volatile("wfe");
}
