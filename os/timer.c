#include <stdint.h>
#include "timer.h"

static volatile uint64_t tick_count;

static inline uint64_t read_cntfrq(void) {
    uint64_t value;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(value));
    return value;
}

static inline void write_cntp_tval(uint64_t value) {
    __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(value));
}

static inline void write_cntp_ctl(uint64_t value) {
    __asm__ volatile("msr cntp_ctl_el0, %0" :: "r"(value));
}

void timer_init(uint32_t ticks) {
    uint64_t frequency = read_cntfrq();
    uint64_t interval = ticks ? ticks : frequency / 100;

    tick_count = 0;
    write_cntp_ctl(0);
    write_cntp_tval(interval);
    write_cntp_ctl(1);
    __asm__ volatile("isb");
}

void timer_irq_handler(void) {
    ++tick_count;
    write_cntp_tval(read_cntfrq() / 100);
}

uint64_t timer_ticks(void) {
    return tick_count;
}
