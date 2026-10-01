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
};

static struct thread threads[MAX_THREADS];
static uint32_t current;
static uint64_t next_id = 1;
static uint64_t switch_count;

struct context;
extern void context_switch(struct context *old, struct context *next);
extern void thread_trampoline(void);

static void interrupts_disable(void) {
    __asm__ volatile("msr daifset, #2" ::: "memory");
}

static void interrupts_restore(void) {
    __asm__ volatile("msr daifclr, #2" ::: "memory");
}

void sched_init(void) {
    for (uint32_t i = 0; i < MAX_THREADS; ++i)
        threads[i].state = THREAD_UNUSED;

    threads[0].id = 0;
    threads[0].state = THREAD_RUNNING;
    threads[0].stack = 0;
    threads[0].stack_size = 0;
    current = 0;
    switch_count = 0;
}

int sched_create(thread_entry_t entry, uint64_t stack_size) {
    if (!entry) return 0;
    if (!stack_size) stack_size = DEFAULT_STACK_SIZE;
    stack_size = (stack_size + 0xfffULL) & ~0xfffULL;

    interrupts_disable();

    uint32_t slot;
    for (slot = 1; slot < MAX_THREADS; ++slot)
        if (threads[slot].state == THREAD_UNUSED) break;

    if (slot == MAX_THREADS) {
        interrupts_restore();
        return 0;
    }

    void *stack = kmalloc(stack_size);
    if (!stack) {
        interrupts_restore();
        return 0;
    }

    uintptr_t bottom = (uintptr_t)stack;
    uintptr_t top = (bottom + stack_size) & ~0xfULL;

    struct thread *t = &threads[slot];
    t->id = next_id++;
    t->state = THREAD_READY;
    t->stack = stack;
    t->stack_size = stack_size;

    t->ctx.x19 = (uint64_t)entry;
    t->ctx.x20 = 0;
    t->ctx.x21 = 0;
    t->ctx.x22 = 0;
    t->ctx.x23 = 0;
    t->ctx.x24 = 0;
    t->ctx.x25 = 0;
    t->ctx.x26 = 0;
    t->ctx.x27 = 0;
    t->ctx.x28 = 0;
    t->ctx.x29 = 0;
    t->ctx.x30 = (uint64_t)thread_trampoline;
    t->ctx.sp = top;

    interrupts_restore();
    return (int)t->id;
}

static uint32_t next_ready(void) {
    for (uint32_t step = 1; step < MAX_THREADS; ++step) {
        uint32_t i = (current + step) % MAX_THREADS;
        if (threads[i].state == THREAD_READY)
            return i;
    }
    return current;
}

void sched_yield(void) {
    interrupts_disable();

    uint32_t next = next_ready();
    if (next == current) {
        interrupts_restore();
        return;
    }

    uint32_t old = current;
    threads[old].state = THREAD_READY;
    threads[next].state = THREAD_RUNNING;
    current = next;
    ++switch_count;

    interrupts_restore();
    context_switch(&threads[old].ctx, &threads[next].ctx);
}

uint64_t sched_current_id(void) {
    return threads[current].id;
}

uint64_t sched_switches(void) {
    return switch_count;
}
