#include <stdint.h>
#include "pmm.h"
#define PAGE_SIZE 0x1000ULL
#define MAX_RAM_SIZE (1ULL << 30)
#define MAX_PAGE_COUNT (MAX_RAM_SIZE / PAGE_SIZE)
#define BITMAP_WORDS (MAX_PAGE_COUNT / 64ULL)
extern char __kernel_end;
static uint64_t page_bitmap[BITMAP_WORDS];
static uintptr_t g_ram_base, g_ram_end;
static uint64_t g_page_count, free_pages;
static inline void bitmap_set(uint64_t p){page_bitmap[p>>6]|=1ULL<<(p&63);}
static inline void bitmap_clear(uint64_t p){page_bitmap[p>>6]&=~(1ULL<<(p&63));}
static inline int bitmap_test(uint64_t p){return (page_bitmap[p>>6]>>(p&63))&1;}
void pmm_init(uintptr_t ram_base,uint64_t ram_size){
    for(uint64_t i=0;i<BITMAP_WORDS;++i)page_bitmap[i]=0;
    if(ram_size>MAX_RAM_SIZE)ram_size=MAX_RAM_SIZE;
    g_ram_base=(ram_base+PAGE_SIZE-1)&~(PAGE_SIZE-1);
    g_ram_end=(ram_base+ram_size)&~(PAGE_SIZE-1);
    if(g_ram_end<=g_ram_base){g_page_count=0;free_pages=0;return;}
    g_page_count=(g_ram_end-g_ram_base)/PAGE_SIZE;
    uintptr_t first_free=((uintptr_t)&__kernel_end+PAGE_SIZE-1)&~(PAGE_SIZE-1);
    if(first_free<g_ram_base)first_free=g_ram_base;
    if(first_free>g_ram_end)first_free=g_ram_end;
    for(uintptr_t a=first_free;a<g_ram_end;a+=PAGE_SIZE)bitmap_set((a-g_ram_base)/PAGE_SIZE);
    free_pages=(g_ram_end-first_free)/PAGE_SIZE;
}
uintptr_t pmm_alloc_page(void){
    if(!free_pages)return 0;
    uint64_t words=(g_page_count+63)/64;
    for(uint64_t w=0;w<words;++w){uint64_t bits=page_bitmap[w];
        if(!bits)continue;
        uint64_t bit=(uint64_t)__builtin_ctzll(bits),p=w*64+bit;
        if(p>=g_page_count)continue;
        bitmap_clear(p);--free_pages;return g_ram_base+p*PAGE_SIZE;
    }
    return 0;
}
void pmm_free_page(uintptr_t a){
    if(a<g_ram_base||a>=g_ram_end||(a&(PAGE_SIZE-1)))return;
    uint64_t p=(a-g_ram_base)/PAGE_SIZE;
    if(bitmap_test(p))return;
    bitmap_set(p);++free_pages;
}
uint64_t pmm_free_count(void){return free_pages;}
