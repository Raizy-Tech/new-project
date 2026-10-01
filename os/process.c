#include <stdint.h>
#include "process.h"
#include "pmm.h"
#include "sched.h"

void sched_test_start(void);

#define USER_TEXT_VA  0x81000000ULL
#define USER_STACK_VA 0x82000000ULL
#define PAGE_SIZE     0x1000ULL

extern char user_program_start;
extern char user_program_end;

int vm_map_user_page(uintptr_t va, uintptr_t pa, int writable, int executable);
uintptr_t vm_create_address_space(void);
void vm_switch_address_space(uintptr_t root_pa);
int vm_map_user_page_in(uintptr_t root_pa, uintptr_t va, uintptr_t pa, int writable, int executable);

static struct process proc;
static uint64_t next_pid = 1;

void process_init(void) {
    proc.pid = 0;
    proc.state = PROCESS_UNUSED;
    proc.page_table = 0;
    proc.user_text = 0;
    proc.user_stack = 0;
    proc.syscalls = 0;
}

uint64_t process_current_pid(void) {
    return proc.pid;
}

uint64_t process_syscalls(void) {
    return proc.syscalls;
}

static int syscall_write_test(struct irq_frame *frame) {
    ++proc.syscalls;
    frame->x[0] = 0x5241495a594f4b31ULL;
    return 0;
}

static int syscall_exit(struct irq_frame *frame) {
    (void)frame;
    proc.state = PROCESS_ZOMBIE;
    return 1;
}

void process_syscall_dispatch(struct irq_frame *frame) {
    /*
     * AArch64 user ABI: x8 contains the syscall number, x0-x5 arguments.
     * The initial RaizyOS ABI currently implements:
     *   x8=1: test syscall, returns a fixed success token in x0
     *   x8=2: process exit
     */
    switch (frame->x[8]) {
    case 1:
        syscall_write_test(frame);
        break;
    case 2:
        if (syscall_exit(frame)) {
            extern void process_user_return(void);
            frame->elr = (uint64_t)process_user_return;
            frame->spsr = 0x5ULL;
        }
        break;
    default:
        frame->x[0] = (uint64_t)-1;
        break;
    }
}

int process_start_user(void) {
    uintptr_t code_pa = pmm_alloc_page();
    uintptr_t stack_pa = pmm_alloc_page();
    if (!code_pa || !stack_pa) return 0;

    uint64_t code_size = (uint64_t)(&user_program_end - &user_program_start);
    if (code_size > PAGE_SIZE) return 0;

    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
        ((volatile uint8_t *)code_pa)[i] = 0;
    for (uint64_t i = 0; i < code_size; ++i)
        ((volatile uint8_t *)code_pa)[i] =
            ((const uint8_t *)&user_program_start)[i];

    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
        ((volatile uint8_t *)stack_pa)[i] = 0;

    proc.page_table = vm_create_address_space();
    if (!proc.page_table) return 0;

    if (!vm_map_user_page_in(proc.page_table, USER_TEXT_VA, code_pa, 0, 1) ||
        !vm_map_user_page_in(proc.page_table, USER_STACK_VA, stack_pa, 1, 0))
        return 0;

    proc.pid = next_pid++;
    proc.state = PROCESS_RUNNING;
    proc.user_text = USER_TEXT_VA;
    proc.user_stack = USER_STACK_VA;
    proc.syscalls = 0;

    vm_switch_address_space(proc.page_table);
    uintptr_t user_sp = USER_STACK_VA + PAGE_SIZE;

    __asm__ volatile(
        "msr sp_el0, %0\n"
        "msr elr_el1, %1\n"
        "msr spsr_el1, xzr\n"
        "isb\n"
        "eret\n"
        :: "r"(user_sp), "r"(USER_TEXT_VA)
        : "memory");

    __builtin_unreachable();
}

void process_user_return(void) {
    proc.state = PROCESS_ZOMBIE;
    sched_test_start();
    for (;;) __asm__ volatile("wfi");
}
