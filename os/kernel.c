#include <stdint.h>
#define UART0_BASE 0x09000000UL
#define UARTDR (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UARTFR (*(volatile uint32_t *)(UART0_BASE + 0x18))
#define UARTFR_TXFF (1u << 5)
static void uart_putc(char c) { while (UARTFR & UARTFR_TXFF) {} UARTDR = (uint32_t)c; }
static void uart_puts(const char *s) { while (*s) { if (*s == '\n') uart_putc('\r'); uart_putc(*s++); } }
void kernel_main(void) {
 uart_puts("\n========================================\n");
 uart_puts("              RaizyOS ARM64             \n");
 uart_puts("========================================\n");
 uart_puts("Kernel booted successfully.\n");
 uart_puts("Architecture: AArch64\n");
 uart_puts("Platform: QEMU virt\n");
 uart_puts("Milestone 1: UART console ONLINE\n");
 uart_puts("\nNext: exception vectors -> timer -> MMU\n");
 for (;;) __asm__ volatile("wfe");
}
