#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>

struct irq_frame;

enum process_state {
    PROCESS_UNUSED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_ZOMBIE
};

struct process {
    uint64_t pid;
    enum process_state state;
    uintptr_t page_table;
    uintptr_t user_text;
    uintptr_t user_stack;
    uintptr_t stack_pa;
    uintptr_t user_pages[64];
    uint32_t user_page_count;
    uint64_t syscalls;
};

void process_init(void);
void process_destroy(void);
int process_start_user(void);
uint64_t process_current_pid(void);
uint64_t process_syscalls(void);
struct irq_frame *process_syscall_dispatch(struct irq_frame *frame);

#endif
