#ifndef SCHED_H
#define SCHED_H
#include <stdint.h>

typedef void (*thread_entry_t)(void);

void sched_init(void);
int sched_create(thread_entry_t entry, uint64_t stack_size);
void sched_yield(void);
uint64_t sched_current_id(void);
uint64_t sched_switches(void);

#endif
