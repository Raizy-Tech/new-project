#ifndef UART_H
#define UART_H
#include <stdint.h>

void uart_set_base(uintptr_t base);
void uart_putc_public(char c);
void uart_puts_public(const char *s);
char uart_getc(void);

#endif
