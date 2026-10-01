#include <stdint.h>
#include "ramfs.h"

#define RAMFS_MAX_FILES 16

struct ramfs_entry {
    char name_buf[RAMFS_NAME_MAX + 1];
    char data_buf[RAMFS_DATA_MAX + 1];
    struct ramfs_file view;
    uint32_t size;
    uint8_t used;
    uint8_t builtin;
};

static struct ramfs_entry entries[RAMFS_MAX_FILES];

static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a++ != *b++) return 0;
    }
    return *a == *b;
}

static uint32_t str_len(const char *s) {
    uint32_t n = 0;
    while (s && *s && n < RAMFS_NAME_MAX) ++n, ++s;
    return n;
}

static int set_entry(uint32_t index, const char *name, const char *data, uint8_t builtin) {
    uint32_t n = str_len(name);
    if (!name || !n || n > RAMFS_NAME_MAX) return 0;
    for (uint32_t i = 0; i < n; ++i) entries[index].name_buf[i] = name[i];
    entries[index].name_buf[n] = 0;

    uint32_t d = 0;
    if (data) {
        while (data[d] && d < RAMFS_DATA_MAX) {
            entries[index].data_buf[d] = data[d];
            ++d;
        }
    }
    entries[index].data_buf[d] = 0;
    entries[index].size = d;
    entries[index].used = 1;
    entries[index].builtin = builtin;
    entries[index].view.name = entries[index].name_buf;
    entries[index].view.data = entries[index].data_buf;
    entries[index].view.size = d;
    return 1;
}

void ramfs_init(void) {
    static const char readme[] =
        "RaizyOS RAM filesystem\n"
        "Writable in memory; contents reset on reboot.\n";
    static const char motd[] =
        "Welcome to RaizyOS.\n"
        "ARM64 kernel + userspace + scheduler + shell.\n";
    static const char about[] =
        "RaizyOS is a personal AArch64 operating-system project.\n"
        "Platform: QEMU virt / GICv3 / PL011.\n";

    for (uint32_t i = 0; i < RAMFS_MAX_FILES; ++i) entries[i].used = 0;
    set_entry(0, "README.TXT", readme, 1);
    set_entry(1, "MOTD.TXT", motd, 1);
    set_entry(2, "ABOUT.TXT", about, 1);
}

uint32_t ramfs_count(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; ++i)
        if (entries[i].used) ++count;
    return count;
}

const struct ramfs_file *ramfs_file_at(uint32_t index) {
    uint32_t seen = 0;
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; ++i) {
        if (!entries[i].used) continue;
        if (seen++ == index) return &entries[i].view;
    }
    return 0;
}

const struct ramfs_file *ramfs_find(const char *name) {
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; ++i)
        if (entries[i].used && str_eq(entries[i].name_buf, name))
            return &entries[i].view;
    return 0;
}

int ramfs_create(const char *name) {
    uint32_t n = str_len(name);
    if (!name || !n || n > RAMFS_NAME_MAX || ramfs_find(name)) return 0;

    for (uint32_t i = 0; i < RAMFS_MAX_FILES; ++i) {
        if (!entries[i].used) return set_entry(i, name, "", 0);
    }
    return 0;
}

int ramfs_write(const char *name, const char *data) {
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; ++i) {
        if (!entries[i].used || !str_eq(entries[i].name_buf, name)) continue;
        uint32_t n = 0;
        if (data) {
            while (data[n] && n < RAMFS_DATA_MAX) {
                entries[i].data_buf[n] = data[n];
                ++n;
            }
        }
        entries[i].data_buf[n] = 0;
        entries[i].size = n;
        entries[i].view.data = entries[i].data_buf;
        entries[i].view.size = n;
        return 1;
    }
    return 0;
}

int ramfs_remove(const char *name) {
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; ++i) {
        if (!entries[i].used || !str_eq(entries[i].name_buf, name)) continue;
        if (entries[i].builtin) return 0;
        entries[i].used = 0;
        return 1;
    }
    return 0;
}
