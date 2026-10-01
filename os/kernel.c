#include <stdint.h>

void exception_vectors(void);
void exception_sync_handler(uint64_t esr, uint64_t elr, uint64_t spsr, uint64_t far);

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
    uart_puts("Next: generic timer -> GIC -> MMU\n");

    for (;;) __asm__ volatile("wfe");
}
