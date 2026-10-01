#ifndef DTB_H
#define DTB_H
#include <stdint.h>
struct dtb_info {
    uintptr_t address;
    uint32_t total_size;
    uintptr_t ram_base;
    uint64_t ram_size;
    uintptr_t uart_base;
    uintptr_t gicd_base;
    uintptr_t gicr_base;
    uintptr_t virtio_base;
    uint32_t timer_ppi;
    int valid;
};
int dtb_init(uintptr_t address, struct dtb_info *info);
void dtb_print_info(const struct dtb_info *info);
#endif
