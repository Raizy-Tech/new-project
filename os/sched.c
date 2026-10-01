#include <stdint.h>
#include "sched.h"
#include "heap.h"

#define MAX_THREADS 16
#define DEFAULT_STACK_SIZE 16384ULL

enum thread_state {
    THREAD_UNUSED = 0,
    THREAD_READY,
    THREAD_RUNNING
};

struct context {
    uint64_t x19, x20;
    uint64_t x21, x22;
    uint64_t x23, x24;
    uint64_t x25, x26;
    uint64_t x27, x28;
    uint64_t x29, x30;
    uint64_t sp;
};

struct thread {
    uint64_t id;
    enum thread_state state;
    struct context ctx;
    void *stack;
    uint64_t stack_size;
    struct irq_frame *frame;
    uintptr_t page_table;
};

static struct thread threads[MAX_THREADS];
static uint32_t current;
static uint64_t next_id = 1;
static uint64_t switch_count;
static uint64_t preempt_switch_count;

struct context;
extern void context_switch(struct context *old, struct context *next);
extern void thread_trampoline(void);

uintptr_t vm_current_kernel_root(void);
void vm_switch_address_space(uintptr_t root_pa);

static void interrupts_disable(void) {
    __asm__ volatile("msr daifset, #2" ::: "memory");
}

static void interrupts_restore(void) {
    __asm__ volatile("msr daifclr, #2" ::: "memory");
}

static void set_exception_stack(void *top) {
    __asm__ volatile("msr sp_el1, %0" :: "r"(top) : "memory");
}

void sched_init(void) {
    for (uint32_t i = 0; i < MAX_THREADS; ++i)
        threads[i].state = THREAD_UNUSED;

    threads[0].id = 0;
    threads[0].state = THREAD_RUNNING;
    threads[0].stack = 0;
    threads[0].stack_size = 0;
    threads[0].frame = 0;
    threads[0].page_table = vm_current_kernel_root();
    current = 0;
    switch_count = 0;
    preempt_switch_count = 0;
}

void sched_attach_current(uintptr_t page_table) {
    if (!page_table) page_table = vm_current_kernel_root();
    interrupts_disable();

    threads[0].page_table = page_table;
    interrupts_restore();
}

