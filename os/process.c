#include <stdint.h>
#include "process.h"
#include "pmm.h"
#include "sched.h"

#define USER_TEXT_VA  0x81000000ULL
#define USER_STACK_VA 0x82000000ULL
#define PAGE_SIZE     0x1000ULL

extern char user_program_start;
extern char user_program_end;

int vm_map_user_page_in(uintptr_t root_pa, uintptr_t va, uintptr_t pa, int writable, int executable);
uintptr_t vm_create_address_space(void);
void vm_switch_address_space(uintptr_t root_pa);
uintptr_t vm_current_kernel_root(void);
void vm_destroy_address_space(uintptr_t root_pa);

static struct process proc;
static uint64_t next_pid = 1;

static void user_code_sync(uintptr_t start, uint64_t size) {
    uintptr_t end = (start + size + 63ULL) & ~63ULL;
    for (uintptr_t p = start & ~63ULL; p < end; p += 64ULL)
        __asm__ volatile("dc cvau, %0" :: "r"(p) : "memory");
    __asm__ volatile("dsb ish" ::: "memory");
    for (uintptr_t p = start & ~63ULL; p < end; p += 64ULL)
        __asm__ volatile("ic ivau, %0" :: "r"(p) : "memory");
    __asm__ volatile("dsb ish\nisb" ::: "memory");
}

void process_init(void) {
    proc.pid = 0;
    proc.state = PROCESS_UNUSED;
    proc.page_table = 0;
    proc.user_text = 0;
    proc.user_stack = 0;
    proc.code_pa = 0;
    proc.stack_pa = 0;
    proc.syscalls = 0;
}

uint64_t process_current_pid(void) { return proc.pid; }
uint64_t process_syscalls(void) { return proc.syscalls; }

void process_destroy(void) {
    uintptr_t root = proc.page_table;
    uintptr_t code = proc.code_pa;
    uintptr_t stack = proc.stack_pa;

    if (root) {
        vm_switch_address_space(vm_current_kernel_root());
        vm_destroy_address_space(root);
        sched_set_current_address_space(vm_current_kernel_root());
    }
    if (code) pmm_free_page(code);
    if (stack) pmm_free_page(stack);

    proc.pid = 0;
    proc.state = PROCESS_UNUSED;
    proc.page_table = 0;
    proc.user_text = 0;
    proc.user_stack = 0;
    proc.code_pa = 0;
    proc.stack_pa = 0;
    proc.syscalls = 0;
}

static void syscall_write(struct irq_frame *frame) {
    ++proc.syscalls;
    frame->x[0] = frame->x[0];
}

static void syscall_getpid(struct irq_frame *frame) {
    ++proc.syscalls;
    frame->x[0] = proc.pid;
}

void process_syscall_dispatch(struct irq_frame *frame) {
    switch (frame->x[8]) {
    case 1:
        syscall_write(frame);
        break;
    case 2:
        ++proc.syscalls;
        uart_puts_public("[SYSCALL] exit requested.\n");
        process_destroy();
        frame = sched_preempt(frame);
        break;
    case 3:
        syscall_getpid(frame);
        uart_puts_public("[SYSCALL] getpid handled.\n");
        break;
    case 4:
        ++proc.syscalls;
        frame->x[0] = 0;
        uart_puts_public("[SYSCALL] yield handled.\n");
        frame = sched_preempt(frame);
        break;
    default:
        ++proc.syscalls;
        frame->x[0] = (uint64_t)-1;
        break;
    }

    /*
     * The exception entry code consumes the returned frame pointer. The
     * scheduler may replace it with another task's saved frame.
     */
    if (frame) {
        __asm__ volatile("mov x19, %0" :: "r"(frame) : "x19");
    }
}

int process_start_user(void) {
    uintptr_t code_pa = pmm_alloc_page();
    uintptr_t stack_pa = pmm_alloc_page();
    if (!code_pa || !stack_pa) {
        if (code_pa) pmm_free_page(code_pa);
        if (stack_pa) pmm_free_page(stack_pa);
        return 0;
    }

    uint64_t code_size = (uint64_t)(&user_program_end - &user_program_start);
    if (code_size > PAGE_SIZE) {
        pmm_free_page(code_pa);
        pmm_free_page(stack_pa);
        return 0;
    }

    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
        ((volatile uint8_t *)code_pa)[i] = 0;
    for (uint64_t i = 0; i < code_size; ++i)
        ((volatile uint8_t *)code_pa)[i] =
            ((const uint8_t *)&user_program_start)[i];

    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
        ((volatile uint8_t *)stack_pa)[i] = 0;
    user_code_sync(code_pa, code_size);

    proc.page_table = vm_create_address_space();
    if (!proc.page_table) {
        pmm_free_page(code_pa);
        pmm_free_page(stack_pa);
        return 0;
    }

    if (!vm_map_user_page_in(proc.page_table, USER_TEXT_VA, code_pa, 0, 1) ||
        !vm_map_user_page_in(proc.page_table, USER_STACK_VA, stack_pa, 1, 0)) {
        vm_destroy_address_space(proc.page_table);
        proc.page_table = 0;
        pmm_free_page(code_pa);
        pmm_free_page(stack_pa);
        return 0;
    }

    proc.pid = next_pid++;
    proc.state = PROCESS_RUNNING;
    proc.user_text = USER_TEXT_VA;
    proc.user_stack = USER_STACK_VA;
    proc.code_pa = code_pa;
    proc.stack_pa = stack_pa;
    proc.syscalls = 0;

    sched_attach_current(proc.page_table);
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
