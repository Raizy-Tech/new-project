#include <stdint.h>
#include "pmm.h"
#define RAM_BASE 0x40000000ULL
#define RAM_SIZE (128ULL * 1024ULL * 1024ULL)
#define RAM_END (RAM_BASE + RAM_SIZE)
#define PAGE_SIZE 0x1000ULL
#define PAGE_COUNT (RAM_SIZE / PAGE_SIZE)
#define BITMAP_WORDS (PAGE_COUNT / 64ULL)
extern char __kernel_end;
static uint64_t page_bitmap[BITMAP_WORDS];
static uint64_t free_pages;
static inline void bitmap_set(uint64_t page) { page_bitmap[page >> 6] |= 1ULL << (page & 63); }
static inline void bitmap_clear(uint64_t page) { page_bitmap[page >> 6] &= ~(1ULL << (page & 63)); }
static inline int bitmap_test(uint64_t page) { return (page_bitmap[page >> 6] >> (page & 63)) & 1; }
void pmm_init(void) {
    for (uint64_t i = 0; i < BITMAP_WORDS; ++i) page_bitmap[i] = 0;
    uintptr_t first_free = ((uintptr_t)&__kernel_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    if (first_free < RAM_BASE) first_free = RAM_BASE;
    if (first_free > RAM_END) first_free = RAM_END;
    for (uintptr_t address = first_free; address < RAM_END; address += PAGE_SIZE)
        bitmap_set((address - RAM_BASE) / PAGE_SIZE);
    free_pages = (RAM_END - first_free) / PAGE_SIZE;
}
uintptr_t pmm_alloc_page(void) {
    if (free_pages == 0) return 0;
    for (uint64_t word = 0; word < BITMAP_WORDS; ++word) {
        uint64_t bits = page_bitmap[word];
        if (!bits) continue;
        uint64_t bit = (uint64_t)__builtin_ctzll(bits);
        uint64_t page = word * 64ULL + bit;
        bitmap_clear(page);
        --free_pages;
        return RAM_BASE + page * PAGE_SIZE;
    }
    return 0;
}
void pmm_free_page(uintptr_t address) {
    if (address < RAM_BASE || address >= RAM_END || (address & (PAGE_SIZE - 1)) != 0) return;
    uint64_t page = (address - RAM_BASE) / PAGE_SIZE;
    if (bitmap_test(page)) return;
    bitmap_set(page);
    ++free_pages;
}
uint64_t pmm_free_count(void) { return free_pages; }
