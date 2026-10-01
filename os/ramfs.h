#ifndef RAMFS_H
#define RAMFS_H
#include <stdint.h>

struct ramfs_file {
    const char *name;
    const char *data;
    uint32_t size;
};

void ramfs_init(void);
uint32_t ramfs_count(void);
const struct ramfs_file *ramfs_file_at(uint32_t index);
const struct ramfs_file *ramfs_find(const char *name);

#endif
