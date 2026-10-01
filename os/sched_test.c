#include <stdint.h>
#include "sched.h"

void uart_puts_public(const char *);

static volatile uint64_t thread1_count;
static volatile uint64_t thread2_count;

static void thread1(void) {
    for (;;) {
        ++thread1_count;
        if ((thread1_count % 1000) == 0)
            uart_puts_public("[SCHED] thread 1 running\n");
        sched_yield();
    }
}

static void thread2(void) {
    for (;;) {
        ++thread2_count;
        if ((thread2_count % 1000) == 0)
            uart_puts_public("[SCHED] thread 2 running\n");
        sched_yield();
    }
}

void sched_test_start(void) {
    if (!sched_create(thread1, 16384) || !sched_create(thread2, 16384)) {
        uart_puts_public("Scheduler: thread creation FAILED.\n");
        return;
    }

    uart_puts_public("Scheduler: 2 kernel threads created.\n");
    sched_yield();

    for (;;) {
        if (thread1_count >= 1000 && thread2_count >= 1000) {
            uart_puts_public("Scheduler: round-robin context switching PASSED.\n");
            uart_puts_public("Scheduler: kernel threads ONLINE.\n");
            for (;;) __asm__ volatile("wfi");
        }
        sched_yield();
    }
}
