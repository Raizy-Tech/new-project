#include <stdint.h>
#include "process.h"
#include "pmm.h"
#include "heap.h"

#define USER_TEXT_VA  0x81000000ULL
#define USER_STACK_VA 0x82000000ULL
#define PAGE_SIZE     0x1000ULL

extern char user_program_start;
extern char user_program_end;

int vm_map_user_page(uintptr_t va, uintptr_t pa, int writable, int executable);

static uint64_t syscall_count;

void sched_test_start(void);

uint64_t process_syscalls(void) {
    return syscall_count;
}

void process_start_user(void) {
    uintptr_t code_pa = pmm_alloc_page();
    uintptr_t stack_pa = pmm_alloc_page();
    if (!code_pa || !stack_pa) return;

    uint64_t code_size = (uint64_t)(&user_program_end - &user_program_start);
    if (code_size > PAGE_SIZE) return;

    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
        ((volatile uint8_t *)code_pa)[i] = 0;
    for (uint64_t i = 0; i < code_size; ++i)
        ((volatile uint8_t *)code_pa)[i] =
            ((const uint8_t *)&user_program_start)[i];

    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
        ((volatile uint8_t *)stack_pa)[i] = 0;

    if (!vm_map_user_page(USER_TEXT_VA, code_pa, 0, 1) ||
        !vm_map_user_page(USER_STACK_VA, stack_pa, 1, 0))
        return;

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

void process_syscall(uint64_t nr, uint64_t arg0) {
    if (nr == 1) {
        ++syscall_count;
        (void)arg0;
    }
}

void process_user_return(void) {
    sched_test_start();
    for (;;) __asm__ volatile("wfi");
}
