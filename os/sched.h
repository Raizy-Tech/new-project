#ifndef SCHED_H
#define SCHED_H
#include <stdint.h>

typedef void (*thread_entry_t)(void);

struct irq_frame {
    uint64_t x[31];
    uint64_t elr;
    uint64_t spsr;
    uint64_t reserved;
};

void sched_init(void);
int sched_create(thread_entry_t entry, uint64_t stack_size);
void sched_yield(void);
struct irq_frame *sched_preempt(struct irq_frame *frame);
uint64_t sched_current_id(void);
uint64_t sched_switches(void);

#endif
