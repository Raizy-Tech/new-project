#include <stdint.h>
#include "heap.h"
#include "pmm.h"

#define HEAP_BASE 0x90000000ULL
#define HEAP_LIMIT 0xa0000000ULL
#define PAGE_SIZE 0x1000ULL
#define MAX_ALLOCS 256
#define HEAP_MAGIC 0x5241495a48454150ULL

struct heap_alloc { uintptr_t va; uint32_t pages; uint8_t used; };
static struct heap_alloc allocs[MAX_ALLOCS];

int vm_map_page(uintptr_t va, uintptr_t pa);
int vm_unmap_page(uintptr_t va);
uintptr_t vm_get_pa(uintptr_t va);

static uintptr_t align_page(uintptr_t v) { return (v + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); }

void heap_init(void) {
    for (uint32_t i = 0; i < MAX_ALLOCS; ++i) allocs[i].used = 0;
}

void *kmalloc(uint64_t size) {
    if (!size) return 0;
    uint64_t total = size + sizeof(uint64_t) + PAGE_SIZE - 1;
    uint32_t pages = (uint32_t)(total / PAGE_SIZE);
    if (!pages) return 0;

    for (uint32_t slot = 0; slot < MAX_ALLOCS; ++slot) {
        if (allocs[slot].used) continue;

        uintptr_t va = HEAP_BASE;
        for (uint32_t i = 0; i < MAX_ALLOCS; ++i) {
            if (allocs[i].used) {
                uintptr_t end = allocs[i].va + (uintptr_t)allocs[i].pages * PAGE_SIZE;
                if (end > va) va = end;
            }
        }
        va = align_page(va);
        if (va + (uintptr_t)pages * PAGE_SIZE > HEAP_LIMIT) return 0;

        uint32_t mapped = 0;
        for (; mapped < pages; ++mapped) {
            uintptr_t pa = pmm_alloc_page();
            if (!pa || !vm_map_page(va + (uintptr_t)mapped * PAGE_SIZE, pa)) {
                if (pa) pmm_free_page(pa);
                break;
            }
        }

        if (mapped != pages) {
            for (uint32_t i = 0; i < mapped; ++i) {
                uintptr_t page_va = va + (uintptr_t)i * PAGE_SIZE;
                uintptr_t pa = vm_get_pa(page_va);
                vm_unmap_page(page_va);
                if (pa) pmm_free_page(pa);
            }
            return 0;
        }

        allocs[slot].va = va;
        allocs[slot].pages = pages;
        allocs[slot].used = 1;
        *(volatile uint64_t *)va = HEAP_MAGIC;
        return (void *)(va + sizeof(uint64_t));
    }
    return 0;
}

void kfree(void *ptr) {
    if (!ptr) return;
    uintptr_t va = (uintptr_t)ptr - sizeof(uint64_t);
    if (*(volatile uint64_t *)va != HEAP_MAGIC) return;

    for (uint32_t slot = 0; slot < MAX_ALLOCS; ++slot) {
        if (!allocs[slot].used || allocs[slot].va != va) continue;
        for (uint32_t i = 0; i < allocs[slot].pages; ++i) {
            uintptr_t page_va = va + (uintptr_t)i * PAGE_SIZE;
            uintptr_t pa = vm_get_pa(page_va);
            vm_unmap_page(page_va);
            if (pa) pmm_free_page(pa);
        }
        *(volatile uint64_t *)va = 0;
        allocs[slot].used = 0;
        return;
    }
}
