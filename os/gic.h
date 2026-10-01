#pragma once
#include <stdint.h>

void gic_init(void);
void gic_enable_ppi(uint32_t intid);
void gic_ack(uint32_t *intid);
void gic_eoi(uint32_t intid);
