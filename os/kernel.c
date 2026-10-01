#include <stdint.h>
#include "gic.h"
#include "timer.h"
void exception_vectors(void);
#define UART0_BASE 0x09000000UL
#define UARTDR (*(volatile uint32_t *)(UART0_BASE+0x00))
#define UARTFR (*(volatile uint32_t *)(UART0_BASE+0x18))
#define UARTFR_TXFF (1u<<5)
static void uart_putc(char c){while(UARTFR&UARTFR_TXFF){}UARTDR=(uint32_t)c;}
static void uart_puts(const char*s){while(*s){if(*s=='\n')uart_putc('\r');uart_putc(*s++);}}
static void exception_init(void){__asm__ volatile("msr vbar_el1,%0\nisb"::"r"(exception_vectors):"memory");}
int kernel_main(void){
 uart_puts("\n========================================\n");
 uart_puts("              RaizyOS ARM64             \n");
 uart_puts("========================================\n");
 uart_puts("Kernel booted successfully.\n");
 uart_puts("Architecture: AArch64\n");
 uart_puts("Platform: QEMU virt\n");
 exception_init();
 uart_puts("Milestone 2: exception vectors ONLINE\n");
 __asm__ volatile("svc #0");
 uart_puts("SVC returned successfully.\n");
 uart_puts("Initializing GICv3 + generic timer...\n");
 gic_init();
 timer_init(0);
 __asm__ volatile("msr daifclr, #2\nisb");
 uart_puts("Milestone 3: TIMER INTERRUPTS ONLINE\n");
 uart_puts("Next: MMU -> memory manager -> scheduler\n");
 for(;;)__asm__ volatile("wfi");
 return 0;
}
