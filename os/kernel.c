#include <stdint.h>
#include "gic.h"
#include "timer.h"
#include "pmm.h"
#include "dtb.h"
#include "heap.h"
#include "sched.h"
#include "process.h"

void exception_vectors(void);
void sched_test_start(void);
void mmu_init(void);
void mmu_test(void);
int vm_map_page(uintptr_t va, uintptr_t pa);
int vm_unmap_page(uintptr_t va);

static uintptr_t find_dtb(void) {
    const uint32_t magic = 0xedfe0dd0U;
    for (uintptr_t p = 0x40000000ULL; p < 0x48000000ULL; p += 4) {
        if (*(volatile uint32_t *)p == magic) return p;
    }
    return 0;
}

static uintptr_t uart_base = 0x09000000UL;
#define UARTDR (*(volatile uint32_t *)(uart_base + 0x00))
#define UARTFR (*(volatile uint32_t *)(uart_base + 0x18))
#define UARTFR_TXFF (1u << 5)

void uart_set_base(uintptr_t base) { if (base) uart_base = base; }

void uart_putc(char c) {
    while (UARTFR & UARTFR_TXFF) {}
    UARTDR = (uint32_t)c;
}
void uart_puts_public(const char *s) {
    while (*s) { if (*s == '\n') uart_putc('\r'); uart_putc(*s++); }
}
static void uart_puts(const char *s) { uart_puts_public(s); }
static void uart_puthex(uint64_t value) {
    static const char digits[] = "0123456789abcdef";
    uart_puts("0x");
    for (int i = 15; i >= 0; --i) uart_putc(digits[(value >> (i * 4)) & 0xf]);
}
static void exception_init(void) {
    __asm__ volatile("msr vbar_el1, %0\nisb" :: "r"(exception_vectors) : "memory");
}
static void irq_enable(void) {
    __asm__ volatile("msr daifclr, #2" ::: "memory");
}
static void pmm_test(void) {
    uint64_t before = pmm_free_count();
    uintptr_t page1 = pmm_alloc_page();
    uintptr_t page2 = pmm_alloc_page();
    uintptr_t page3 = pmm_alloc_page();

    if (!page1 || !page2 || !page3 || page1 == page2 || page1 == page3 || page2 == page3) {
        uart_puts("PMM: allocation test FAILED.\n");
        return;
    }

    *(volatile uint64_t *)page1 = 0x5241495a594f3031ULL;
    *(volatile uint64_t *)page2 = 0x5241495a594f3032ULL;
    *(volatile uint64_t *)page3 = 0x5241495a594f3033ULL;

    if (*(volatile uint64_t *)page1 != 0x5241495a594f3031ULL ||
        *(volatile uint64_t *)page2 != 0x5241495a594f3032ULL ||
        *(volatile uint64_t *)page3 != 0x5241495a594f3033ULL) {
        uart_puts("PMM: page read/write test FAILED.\n");
        return;
    }

    pmm_free_page(page1);
    pmm_free_page(page2);
    pmm_free_page(page3);

    if (pmm_free_count() != before) {
        uart_puts("PMM: free/reclaim test FAILED.\n");
        return;
    }

    uart_puts("PMM: 4 KiB page allocator ONLINE.\n");
    uart_puts("PMM: allocated pages: ");
    uart_puthex(page1); uart_puts(", "); uart_puthex(page2); uart_puts(", "); uart_puthex(page3);
    uart_puts("\n");
    uart_puts("PMM: page read/write + reclaim test PASSED.\n");
    uart_puts("PMM: free pages = "); uart_puthex(before); uart_puts("\n");
}
void kernel_main(uintptr_t dtb_address) {
    uart_puts("\n========================================\n");
    uart_puts("              RaizyOS ARM64             \n");
    uart_puts("========================================\n");
    uart_puts("Kernel booted successfully.\n");
    uart_puts("Architecture: AArch64\n");
    uart_puts("Platform: QEMU virt / GICv3\n");

    struct dtb_info dtb;
    if (dtb_address == 0 || *(volatile uint32_t *)dtb_address != 0xedfe0dd0U)
        dtb_address = find_dtb();
    if (!dtb_address || !dtb_init(dtb_address, &dtb)) {
        uart_puts("DTB: discovery FAILED.\n");
        for (;;) __asm__ volatile("wfi");
    }
    dtb_print_info(&dtb);
    uart_puts("Milestone 7: DTB hardware discovery ONLINE\n");

    exception_init();
    uart_puts("Milestone 2: exception vectors ONLINE\n");
    uart_puts("Test: issuing SVC #0...\n");
    __asm__ volatile("svc #0");
    uart_puts("SVC returned successfully.\n");

    uart_set_base(dtb.uart_base);
    gic_init(dtb.gicd_base, dtb.gicr_base, dtb.timer_ppi);
    timer_init(0);
    uart_puts("Milestone 3: GICv3 + generic timer ONLINE\n");
    uart_puts("Milestone 4: timer IRQs ONLINE\n");
    irq_enable();

    uart_puts("Milestone 5: enabling MMU...\n");
    mmu_init();
    uart_puts("MMU: enabled with identity mappings.\n");
    mmu_test();
    uart_puts("MMU: kernel execution continues after translation.\n");

    uart_puts("Milestone 6: physical memory manager...\n");
    uart_puts("PMM: using DTB-discovered RAM.\n");
    pmm_init(dtb.ram_base, dtb.ram_size);
    pmm_test();

    uart_puts("VM: dynamic PMM-backed mapping test...\n");
    uint64_t vm_before = pmm_free_count();
    uintptr_t vm_page = pmm_alloc_page();
    uintptr_t vm_va = 0x80400000ULL;
    if (!vm_page || !vm_map_page(vm_va, vm_page)) {
        uart_puts("VM: dynamic mapping FAILED.\n");
    } else {
        volatile uint64_t *vm_ptr = (volatile uint64_t *)vm_va;
        *vm_ptr = 0x5241495a594f444dULL;
        if (*vm_ptr == 0x5241495a594f444dULL && vm_unmap_page(vm_va)) {
            pmm_free_page(vm_page);
            if (pmm_free_count() == vm_before)
                uart_puts("VM: dynamic PMM-backed mapping PASSED.\n");
            else
                uart_puts("VM: dynamic mapping reclaim FAILED.\n");
        } else {
            uart_puts("VM: dynamic mapping read/write FAILED.\n");
        }
    }

    heap_init();
    uart_puts("Heap: page-backed kernel heap test...\n");
    uint64_t heap_before = pmm_free_count();
    uint8_t *a = (uint8_t *)kmalloc(64);
    uint8_t *b = (uint8_t *)kmalloc(5000);
    if (!a || !b) {
        uart_puts("Heap: allocation FAILED.\n");
    } else {
        for (uint32_t i = 0; i < 64; ++i) a[i] = (uint8_t)i;
        for (uint32_t i = 0; i < 5000; ++i) b[i] = (uint8_t)(i ^ 0xa5);
        uint32_t ok = 1;
        for (uint32_t i = 0; i < 64; ++i) if (a[i] != (uint8_t)i) ok = 0;
        for (uint32_t i = 0; i < 5000; ++i) if (b[i] != (uint8_t)(i ^ 0xa5)) ok = 0;
        kfree(a);
        kfree(b);
        if (ok && pmm_free_count() == heap_before)
            uart_puts("Heap: allocation/read-write/free test PASSED.\n");
        else
            uart_puts("Heap: reclaim test FAILED.\n");
    }

    uart_puts("Milestone 8: kernel scheduler...\n");
    sched_init();
    sched_test_start();

    process_init();
    uart_puts("Milestone 9: user process + syscall boundary...\n");
    process_start_user();

    for (;;) __asm__ volatile("wfi");
}
