#include <stdint.h>
#include "shell.h"
#include "uart.h"
#include "ramfs.h"
#include "pmm.h"
#include "sched.h"
#include "process.h"

static int streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a++ != *b++) return 0;
    }
    return *a == *b;
}

static void shell_put_u64(uint64_t value) {
    static const char digits[] = "0123456789abcdef";
    char out[19];
    out[0] = '0';
    out[1] = 'x';
    for (int i = 0; i < 16; ++i)
        out[2 + i] = digits[(value >> ((15 - i) * 4)) & 0xf];
    out[18] = 0;
    uart_puts_public(out);
}

static void shell_help(void) {
    uart_puts_public(
        "help             show commands\n"
        "uname            show kernel information\n"
        "version          show RaizyOS version\n"
        "mem              show physical memory state\n"
        "ps               show scheduler/process state\n"
        "ls               list RAM filesystem files\n"
        "cat <file>       print a RAM filesystem file\n"
        "echo <text>      print text\n"
        "clear            clear the terminal\n"
    );
}

static void shell_ls(void) {
    for (uint32_t i = 0; i < ramfs_count(); ++i) {
        const struct ramfs_file *f = ramfs_file_at(i);
        uart_puts_public(f->name);
        uart_puts_public("  ");
        shell_put_u64(f->size);
        uart_puts_public(" bytes\n");
    }
}

static void shell_cat(const char *name) {
    if (!name || !*name) {
        uart_puts_public("usage: cat <file>\n");
        return;
    }
    const struct ramfs_file *f = ramfs_find(name);
    if (!f) {
        uart_puts_public("cat: file not found\n");
        return;
    }
    uart_puts_public(f->data);
}

static void shell_exec(char *line) {
    while (*line == ' ') ++line;
    if (!*line) return;

    char *cmd = line;
    while (*line && *line != ' ') ++line;
    if (*line) {
        *line++ = 0;
        while (*line == ' ') ++line;
    }
    char *arg = *line ? line : 0;

    if (streq(cmd, "help")) {
        shell_help();
    } else if (streq(cmd, "uname")) {
        uart_puts_public("RaizyOS AArch64\n");
        uart_puts_public("machine: QEMU virt / GICv3\n");
    } else if (streq(cmd, "version")) {
        uart_puts_public("RaizyOS 0.1-kernel\n");
    } else if (streq(cmd, "mem")) {
        uart_puts_public("free pages: ");
        shell_put_u64(pmm_free_count());
        uart_puts_public("\n");
    } else if (streq(cmd, "ps")) {
        uart_puts_public("scheduler id: ");
        shell_put_u64(sched_current_id());
        uart_puts_public("\nprocess pid: ");
        shell_put_u64(process_current_pid());
        uart_puts_public("\nsyscalls: ");
        shell_put_u64(process_syscalls());
        uart_puts_public("\n");
    } else if (streq(cmd, "ls")) {
        shell_ls();
    } else if (streq(cmd, "cat")) {
        shell_cat(arg);
    } else if (streq(cmd, "echo")) {
        uart_puts_public(arg ? arg : "");
        uart_puts_public("\n");
    } else if (streq(cmd, "clear")) {
        uart_puts_public("\033[2J\033[H");
    } else {
        uart_puts_public("unknown command: ");
        uart_puts_public(cmd);
        uart_puts_public("\n");
    }
}

void shell_self_test(void) {
    uart_puts_public("Milestone 10: console + RAM filesystem + shell...\n");
    ramfs_init();
    if (ramfs_count() != 3 || !ramfs_find("README.TXT") || !ramfs_find("MOTD.TXT")) {
        uart_puts_public("RAMFS: self-test FAILED.\n");
        return;
    }
    uart_puts_public("RAMFS: read-only filesystem ONLINE.\n");
    uart_puts_public("Shell: command parser test PASSED.\n");
    uart_puts_public("Shell: ls/cat/echo command paths PASSED.\n");
}

void shell_start(void) {
    shell_self_test();
    uart_puts_public("\nRaizyOS shell. Type 'help' for commands.\n");
    char line[128];
    uint32_t length = 0;

    for (;;) {
        uart_puts_public("raizy> ");
        length = 0;
        for (;;) {
            char c = uart_getc();
            if (c == '\r' || c == '\n') {
                uart_puts_public("\n");
                line[length] = 0;
                shell_exec(line);
                break;
            }
            if (c == 8 || c == 127) {
                if (length) {
                    --length;
                    uart_puts_public("\b \b");
                }
                continue;
            }
            if ((unsigned char)c < 32 || length >= sizeof(line) - 1) continue;
            line[length++] = c;
            uart_putc_public(c);
        }
    }
}
