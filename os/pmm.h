#ifndef PMM_H
#define PMM_H
#include <stdint.h>
void pmm_init(uintptr_t ram_base, uint64_t ram_size);
uintptr_t pmm_alloc_page(void);
void pmm_free_page(uintptr_t address);
uint64_t pmm_free_count(void);
#endif
