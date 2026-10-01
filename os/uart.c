#include <stdint.h>
#include "uart.h"

static uintptr_t uart_base = 0x09000000ULL;

#define UARTDR (*(volatile uint32_t *)(uart_base + 0x00))
#define UARTFR (*(volatile uint32_t *)(uart_base + 0x18))
#define UARTFR_RXFE (1u << 4)
#define UARTFR_TXFF (1u << 5)

void uart_set_base(uintptr_t base) {
    if (base) uart_base = base;
}

void uart_putc_public(char c) {
    while (UARTFR & UARTFR_TXFF) {}
    UARTDR = (uint32_t)c;
}

void uart_puts_public(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc_public('\r');
        uart_putc_public(*s++);
    }
}

char uart_getc(void) {
    while (UARTFR & UARTFR_RXFE) {}
    return (char)(UARTDR & 0xff);
}
