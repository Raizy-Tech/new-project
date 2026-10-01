#include <stdint.h>

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

static uint64_t read_cntpct(void) {
    uint64_t value;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(value));
    return value;
}

static void timer_init(void) {
    uint64_t freq = read_cntfrq();
    uint64_t now = read_cntpct();
    uint64_t ticks = freq / 10; /* 100 ms demonstration period. */
    uint64_t deadline = now + ticks;

    __asm__ volatile(
        "msr cntp_tval_el1, %0\n"
        "mov x1, #1\n"
        "msr cntp_ctl_el0, x1\n"
        "isb\n"
        :
        : "r"(ticks)
        : "x1", "memory");

    uart_puts("Timer: ");
    uart_puts("ARM generic timer configured for 100 ms.\n");
    (void)deadline;
}

static void exception_init(void) {
    __asm__ volatile(
        "msr vbar_el1, %0\n"
        "isb\n"
        :
        : "r"(exception_vectors)
        : "memory");
}

void kernel_main(void) {
    uart_puts("\n========================================\n");
    uart_puts("              RaizyOS ARM64             \n");
    uart_puts("========================================\n");
    uart_puts("Kernel booted successfully.\n");
    uart_puts("Architecture: AArch64\n");
    uart_puts("Platform: QEMU virt\n");

    exception_init();
    uart_puts("Milestone 2: exception vectors ONLINE\n");
    uart_puts("Test: issuing SVC #0...\n");
    __asm__ volatile("svc #0");
    uart_puts("SVC returned successfully.\n");

    uart_puts("Milestone 3: generic timer initialization\n");
    uart_puts("Counter frequency: ");
    uint64_t freq = read_cntfrq();
    static const char digits[] = "0123456789";
    char buf[24];
    int i = 0;
    if (freq == 0) {
        uart_puts("0\n");
    } else {
        while (freq && i < 23) {
            buf[i++] = digits[freq % 10];
            freq /= 10;
        }
        while (i) uart_putc(buf[--i]);
        uart_putc('\n');
    }
    timer_init();

    uart_puts("Next: GIC interrupt routing -> timer IRQ -> MMU\n");
    for (;;) __asm__ volatile("wfe");
}
