#include <stdint.h>
#include "gic.h"
#include "timer.h"

void exception_vectors(void);
void mmu_init(void);
void mmu_test(void);

#define UART0_BASE 0x09000000UL
#define UARTDR (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UARTFR (*(volatile uint32_t *)(UART0_BASE + 0x18))
#define UARTFR_TXFF (1u << 5)

void uart_putc(char c) {
    while (UARTFR & UARTFR_TXFF) {}
    UARTDR = (uint32_t)c;
}
void uart_puts_public(const char *s) {
    while (*s) { if (*s == '\n') uart_putc('\r'); uart_putc(*s++); }
}
static void uart_puts(const char *s) { uart_puts_public(s); }

static void exception_init(void) {
    __asm__ volatile("msr vbar_el1, %0\nisb" :: "r"(exception_vectors) : "memory");
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

    gic_init();
    timer_init(0);
    uart_puts("Milestone 3: GICv3 + generic timer ONLINE\n");
    uart_puts("Milestone 4: timer IRQs ONLINE\n");
    irq_enable();

    uart_puts("Milestone 5: enabling MMU...\n");
    mmu_init();
    uart_puts("MMU: enabled with identity mappings.\n");
    mmu_test();
    uart_puts("MMU: kernel execution continues after translation.\n");

    for (;;) __asm__ volatile("wfi");
}
