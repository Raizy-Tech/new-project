#include <stdint.h>
#include "gic.h"
#include "timer.h"
#include "sched.h"
#include "process.h"

void process_user_return(void);

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

struct irq_frame *exception_sync_handler(struct irq_frame *frame) {
    uint64_t esr;
    __asm__ volatile("mrs %0, esr_el1" : "=r"(esr));
    uint32_t ec = (uint32_t)(esr >> 26);

    if (ec == 0x15) {
        uint64_t nr = esr & 0xffffULL;
        if (nr == 0x1 && frame->spsr == 0) {
            process_syscall(nr, frame->x[0]);
            uart_puts("[SYSCALL] user SVC #1 handled.\\n");
            frame->x[0] = 0x5241495a594f4b31ULL;
        } else if (nr == 0x2 && frame->spsr == 0) {
            uart_puts("[PROCESS] user process exit -> EL1.\\n");
            frame->elr = (uint64_t)process_user_return;
            frame->spsr = 0x5ULL;
        } else {
            uart_puts("\n[EXCEPTION] synchronous exception\n");
            uart_puts("  Cause: SVC instruction.\n");
        }
        return frame;
    }

    uart_puts("\n[EXCEPTION] synchronous exception\n");
    uart_puts("  ESR_EL1 = "); uart_hex64(esr); uart_puts("\n");
    uart_puts("  ELR_EL1 = "); uart_hex64(frame->elr); uart_puts("\n");
    uart_puts("  SPSR_EL1 = "); uart_hex64(frame->spsr); uart_puts("\n");
    uart_puts("  FAR_EL1 = "); uart_hex64(0); uart_puts("\n");
    uart_puts("  EC = "); uart_hex64(ec); uart_puts("\n");
    uart_puts("  Cause: unclassified synchronous exception.\n");
    return frame;
}

struct irq_frame *exception_irq_handler(struct irq_frame *frame) {
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

    if (intid == 30)
        return sched_preempt(frame);
    return frame;
}

void exception_unhandled_handler(void) {
    uart_puts("\n[FATAL] unhandled exception vector\n");
    for (;;) __asm__ volatile("wfe");
}
