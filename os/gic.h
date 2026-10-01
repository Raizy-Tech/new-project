#pragma once
#include <stdint.h>

void gic_init(uintptr_t distributor_base, uintptr_t redistributor_base);
void gic_enable_ppi(uint32_t intid);
void gic_ack(uint32_t *intid);
void gic_eoi(uint32_t intid);
