#include <stdint.h>
#include "ramfs.h"

static const char readme[] =
    "RaizyOS RAM filesystem\n"
    "Read-only files are embedded in the kernel image.\n";

static const char motd[] =
    "Welcome to RaizyOS.\n"
    "ARM64 kernel + userspace + scheduler + shell.\n";

static const char about[] =
    "RaizyOS is a personal AArch64 operating-system project.\n"
    "Platform: QEMU virt / GICv3 / PL011.\n";

static const struct ramfs_file files[] = {
    { "README.TXT", readme, sizeof(readme) - 1 },
    { "MOTD.TXT", motd, sizeof(motd) - 1 },
    { "ABOUT.TXT", about, sizeof(about) - 1 }
};

void ramfs_init(void) {}

uint32_t ramfs_count(void) {
    return (uint32_t)(sizeof(files) / sizeof(files[0]));
}

const struct ramfs_file *ramfs_file_at(uint32_t index) {
    return index < ramfs_count() ? &files[index] : 0;
}

static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a++ != *b++) return 0;
    }
    return *a == *b;
}

const struct ramfs_file *ramfs_find(const char *name) {
    for (uint32_t i = 0; i < ramfs_count(); ++i)
        if (str_eq(files[i].name, name)) return &files[i];
    return 0;
}
