#pragma once
#include <stdint.h>

void timer_init(uint32_t ticks);
void timer_irq_handler(void);
uint64_t timer_ticks(void);
