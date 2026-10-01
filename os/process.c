#include <stdint.h>
#include "process.h"
#include "pmm.h"
#include "sched.h"

#define USER_STACK_VA 0x82000000ULL
#define PAGE_SIZE 0x1000ULL
#define MAX_USER_PAGES 64

#define PT_LOAD 1
#define ET_EXEC 2
#define EM_AARCH64 183
#define PF_X 1
#define PF_W 2

struct elf64_ehdr {
    unsigned char ident[16];
    uint16_t type, machine;
    uint32_t version;
    uint64_t entry, phoff, shoff;
    uint32_t flags;
    uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
};

struct elf64_phdr {
    uint32_t type, flags;
    uint64_t offset, vaddr, paddr, filesz, memsz, align;
};

extern char _binary_build_user_elf_start[];
extern char _binary_build_user_elf_end[];
uintptr_t vm_create_address_space(void);
void vm_destroy_address_space(uintptr_t root_pa);
void vm_switch_address_space(uintptr_t root_pa);
uintptr_t vm_current_kernel_root(void);
int vm_map_user_page_in(uintptr_t root_pa, uintptr_t va, uintptr_t pa, int writable, int executable);

static struct process proc;
static uint64_t next_pid = 1;

void uart_puts_public(const char *);

static void process_hex64(uint64_t v) {
    static const char h[] = "0123456789abcdef";
    char b[19];
    b[0] = '0'; b[1] = 'x';
    for (int i = 0; i < 16; ++i)
        b[2 + i] = h[(v >> ((15 - i) * 4)) & 0xf];
    b[18] = 0;
    uart_puts_public(b);
}

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
    proc.pid = 0; proc.state = PROCESS_UNUSED; proc.page_table = 0;
    proc.user_text = 0; proc.user_stack = 0; proc.stack_pa = 0;
    proc.user_page_count = 0; proc.syscalls = 0;
}

uint64_t process_current_pid(void) { return proc.pid; }
uint64_t process_syscalls(void) { return proc.syscalls; }

void process_destroy(void) {
    uintptr_t root = proc.page_table;
    if (root) {
        vm_switch_address_space(vm_current_kernel_root());
        vm_destroy_address_space(root);
        sched_set_current_address_space(vm_current_kernel_root());
    }
    for (uint32_t i = 0; i < proc.user_page_count; ++i)
        if (proc.user_pages[i]) pmm_free_page(proc.user_pages[i]);

    proc.pid = 0; proc.state = PROCESS_UNUSED; proc.page_table = 0;
    proc.user_text = 0; proc.user_stack = 0; proc.stack_pa = 0;
    proc.user_page_count = 0; proc.syscalls = 0;
}

static int track_page(uintptr_t pa) {
    if (proc.user_page_count >= MAX_USER_PAGES) return 0;
    proc.user_pages[proc.user_page_count++] = pa;
    return 1;
}

static int load_elf(uintptr_t root, const uint8_t *image, uint64_t size, uintptr_t *entry_out) {
    if (size < sizeof(struct elf64_ehdr)) return 0;
    const struct elf64_ehdr *eh = (const struct elf64_ehdr *)image;
    uart_puts_public("[ELF] image size = "); process_hex64(size); uart_puts_public("\n");
    if (eh->ident[0] != 0x7f || eh->ident[1] != 'E' || eh->ident[2] != 'L' || eh->ident[3] != 'F') {
        uart_puts_public("[ELF] bad magic.\n");
        return 0;
    }
    uart_puts_public("[ELF] type = "); process_hex64(eh->type); uart_puts_public(" machine = "); process_hex64(eh->machine); uart_puts_public("\n");
    uart_puts_public("[ELF] phoff = "); process_hex64(eh->phoff); uart_puts_public(" phentsize = "); process_hex64(eh->phentsize); uart_puts_public(" phnum = "); process_hex64(eh->phnum); uart_puts_public("\n");
    if (eh->ident[4] != 2 || eh->ident[5] != 1 || eh->type != ET_EXEC || eh->machine != EM_AARCH64) {
        uart_puts_public("[ELF] header compatibility FAILED.\n");
        return 0;
    }
    if (eh->phentsize != sizeof(struct elf64_phdr) || !eh->phnum) return 0;
    if (eh->phoff > size || eh->phnum > (size - eh->phoff) / eh->phentsize) return 0;

    for (uint16_t i = 0; i < eh->phnum; ++i) {
        const struct elf64_phdr *ph = (const struct elf64_phdr *)(image + eh->phoff + (uint64_t)i * eh->phentsize);
        uart_puts_public("[ELF] segment type = "); process_hex64(ph->type); uart_puts_public(" vaddr = "); process_hex64(ph->vaddr); uart_puts_public(" filesz = "); process_hex64(ph->filesz); uart_puts_public(" memsz = "); process_hex64(ph->memsz); uart_puts_public("\n");
        if (ph->type != PT_LOAD || !ph->memsz) continue;
        if (ph->filesz > ph->memsz || ph->offset > size || ph->filesz > size - ph->offset) return 0;
        if (ph->vaddr < 0x80000000ULL || ph->vaddr + ph->memsz < ph->vaddr ||
            ph->vaddr + ph->memsz > 0xc0000000ULL) return 0;

        uintptr_t first = ph->vaddr & ~(PAGE_SIZE - 1);
        uintptr_t last = (ph->vaddr + ph->memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

        for (uintptr_t va = first; va < last; va += PAGE_SIZE) {
            if (proc.user_page_count >= MAX_USER_PAGES) return 0;
            uintptr_t pa = pmm_alloc_page();
            if (!pa) return 0;
            for (uint64_t z = 0; z < PAGE_SIZE; ++z)
                ((volatile uint8_t *)pa)[z] = 0;

            uint64_t seg_start = ph->vaddr;
            uint64_t seg_end = ph->vaddr + ph->filesz;
            uint64_t page_start = va;
            uint64_t page_end = va + PAGE_SIZE;
            uint64_t copy_start = seg_start > page_start ? seg_start : page_start;
            uint64_t copy_end = seg_end < page_end ? seg_end : page_end;
            if (copy_start < copy_end) {
                uint64_t src_off = ph->offset + (copy_start - seg_start);
                for (uint64_t n = 0; n < copy_end - copy_start; ++n)
                    ((volatile uint8_t *)pa)[copy_start - page_start + n] = image[src_off + n];
            }

            int writable = (ph->flags & PF_W) != 0;
            int executable = (ph->flags & PF_X) != 0;
            if (!vm_map_user_page_in(root, va, pa, writable, executable)) {
                pmm_free_page(pa);
                return 0;
            }
            if (!track_page(pa)) {
                pmm_free_page(pa);
                return 0;
            }
            if (executable) user_code_sync(pa, PAGE_SIZE);
        }
    }

    if (eh->entry < 0x80000000ULL || eh->entry >= 0xc0000000ULL) return 0;
    *entry_out = eh->entry;
    return 1;
}

static void syscall_write(struct irq_frame *frame) { ++proc.syscalls; }
static void syscall_getpid(struct irq_frame *frame) { ++proc.syscalls; frame->x[0] = proc.pid; }

struct irq_frame *process_syscall_dispatch(struct irq_frame *frame) {
    switch (frame->x[8]) {
    case 1:
        syscall_write(frame);
        uart_puts_public("[SYSCALL] write handled.\n");
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
    return frame;
}

int process_start_user(void) {
    const uint8_t *image = (const uint8_t *)_binary_build_user_elf_start;
    uint64_t image_size = (uint64_t)(_binary_build_user_elf_end - _binary_build_user_elf_start);
    uart_puts_public("[PROCESS] creating address space.\n");
    proc.page_table = vm_create_address_space();
    if (!proc.page_table) {
        uart_puts_public("[PROCESS] address space creation FAILED.\n");
        return 0;
    }

    uintptr_t entry = 0;
    uart_puts_public("[PROCESS] loading embedded ELF64.\n");
    if (!load_elf(proc.page_table, image, image_size, &entry)) {
        uart_puts_public("[PROCESS] ELF load FAILED.\n");
        process_destroy();
        return 0;
    }
    uart_puts_public("[PROCESS] ELF load PASSED.\n");

    uintptr_t stack_pa = pmm_alloc_page();
    if (!stack_pa || !track_page(stack_pa)) {
        uart_puts_public("[PROCESS] user stack allocation FAILED.\n");
        if (stack_pa) pmm_free_page(stack_pa);
        process_destroy();
        return 0;
    }
    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
        ((volatile uint8_t *)stack_pa)[i] = 0;

    if (!vm_map_user_page_in(proc.page_table, USER_STACK_VA, stack_pa, 1, 0)) {
        uart_puts_public("[PROCESS] user stack mapping FAILED.\n");
        process_destroy();
        return 0;
    }
    uart_puts_public("[PROCESS] user stack mapped.\n");

    proc.pid = next_pid++;
    proc.state = PROCESS_RUNNING;
    proc.user_text = entry;
    proc.user_stack = USER_STACK_VA;
    proc.stack_pa = stack_pa;
    proc.syscalls = 0;

    sched_attach_current(proc.page_table);
    vm_switch_address_space(proc.page_table);
    uart_puts_public("[PROCESS] entering EL0.\n");

    uintptr_t user_sp = USER_STACK_VA + PAGE_SIZE;
    __asm__ volatile(
        "msr sp_el0, %0\n"
        "msr elr_el1, %1\n"
        "msr spsr_el1, xzr\n"
        "isb\n"
        "eret"
        :: "r"(user_sp), "r"(entry) : "memory");
    __builtin_unreachable();
}
