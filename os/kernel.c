#include <stdint.h>
#include "gic.h"
#include "timer.h"

void exception_vectors(void);

#define UART0_BASE 0x09000000UL
#define UARTDR (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UARTFR (*(volatile uint32_t *)(UART0_BASE + 0x18))
#define UARTFR_TXFF (1u << 5)

static void uart_putc(char c) {
    while (UARTFR & UARTFR_TXFF) {}
    UARTDR = (uint32_t)c;
}

static void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

static uint64_t read_cntfrq(void) {
    uint64_t value;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(value));
    return value;
}

static void exception_init(void) {
    __asm__ volatile(
        "msr vbar_el1, %0\n"
        "isb\n"
        :
        : "r"(exception_vectors)
        : "memory");
}

static void irq_enable(void) {
    __asm__ volatile("msr daifclr, #2" ::: "memory");
}

void kernel_main(void) {
    uart_puts("\n========================================\n");
    uart_puts("              RaizyOS ARM64             \n");
    uart_puts("========================================\n");
    uart_puts("Kernel booted successfully.\n");
    uart_puts("Architecture: AArch64\n");
    uart_puts("Platform: QEMU virt / GICv3\n");

    exception_init();
    uart_puts("Milestone 2: exception vectors ONLINE\n");
    uart_puts("Test: issuing SVC #0...\n");
    __asm__ volatile("svc #0");
    uart_puts("SVC returned successfully.\n");

    uart_puts("Milestone 3: ARM generic timer + GICv3\n");
    uart_puts("Counter frequency: ");
    uint64_t freq = read_cntfrq();
    char buf[24];
    int i = 0;
    if (freq == 0) {
        uart_puts("0\n");
    } else {
        while (freq && i < 23) {
            buf[i++] = (char)('0' + (freq % 10));
            freq /= 10;
        }
        while (i) uart_putc(buf[--i]);
        uart_putc('\n');
    }

    gic_init();
    timer_init(0);
    uart_puts("GIC: Group 1 + timer PPI 30 enabled.\n");
    uart_puts("Timer: 100 Hz periodic interrupt configured.\n");
    uart_puts("Milestone 4: waiting for real timer IRQs...\n");
    irq_enable();

    for (;;) {
        __asm__ volatile("wfi");
    }
}
