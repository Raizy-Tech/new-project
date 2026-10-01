#ifndef RAMFS_H
#define RAMFS_H
#include <stdint.h>

#define RAMFS_NAME_MAX 31
#define RAMFS_DATA_MAX 255

struct ramfs_file {
    const char *name;
    const char *data;
    uint32_t size;
};

void ramfs_init(void);
uint32_t ramfs_count(void);
const struct ramfs_file *ramfs_file_at(uint32_t index);
const struct ramfs_file *ramfs_find(const char *name);
int ramfs_create(const char *name);
int ramfs_write(const char *name, const char *data);
int ramfs_remove(const char *name);

#endif
