#include <stdint.h>
#include "dtb.h"
#define FDT_MAGIC 0xd00dfeedU
#define FDT_BEGIN_NODE 1U
#define FDT_END_NODE 2U
#define FDT_PROP 3U
#define FDT_NOP 4U
#define FDT_END 9U
#define NODE_OTHER 0
#define NODE_MEMORY 1
#define NODE_UART 2
#define NODE_GIC 3
#define NODE_TIMER 4
struct fdt_header { uint32_t magic, totalsize, off_dt_struct, off_dt_strings, off_mem_rsvmap;
    uint32_t version, last_comp_version, boot_cpuid_phys, size_dt_strings, size_dt_struct; };
static uint32_t be32(uint32_t x){return((x&0xff000000U)>>24)|((x&0x00ff0000U)>>8)|((x&0x0000ff00U)<<8)|((x&0x000000ffU)<<24);}
static uintptr_t align4(uintptr_t p){return(p+3U)&~(uintptr_t)3U;}
static int streq(const char*a,const char*b){if(!a||!b)return 0;while(*a&&*b&&*a==*b){++a;++b;}return *a==*b;}
static int compat_has(const uint8_t*p,uint32_t len,const char*target){
    uint32_t i=0; while(i<len){const char*s=(const char*)(p+i);uint32_t n=0;while(i+n<len&&s[n])++n;
        if(streq(s,target))return 1; i+=n+1;} return 0;
}
static uint64_t cells(const uint32_t*p,uint32_t n){uint64_t v=0;for(uint32_t i=0;i<n;++i)v=(v<<32)|be32(p[i]);return v;}
int dtb_init(uintptr_t a,struct dtb_info*i){
    if(!i||!a)return 0; const struct fdt_header*h=(const struct fdt_header*)a;
    if(be32(h->magic)!=FDT_MAGIC)return 0;
    uint32_t total=be32(h->totalsize),so=be32(h->off_dt_struct),stro=be32(h->off_dt_strings),ss=be32(h->size_dt_struct),sts=be32(h->size_dt_strings);
    if(total<sizeof(*h)||so>=total||stro>=total||ss>total-so||sts>total-stro)return 0;
    const uint8_t*base=(const uint8_t*)a,*p=base+so,*end=p+ss,*strings=base+stro;
    uint32_t ac=2,sc=1; int depth=-1,node=NODE_OTHER,md=-1; uintptr_t rb=0,ub=0,gb=0,gr=0;uint32_t timer_ppi=0;uint64_t rs=0;int have_ram=0;
    while(p<end){
        uint32_t tag=be32(*(const uint32_t*)p);p+=4;
        if(tag==FDT_BEGIN_NODE){
            const char*n=(const char*)p;uintptr_t q=(uintptr_t)p;while(q<(uintptr_t)end&&*(const char*)q)++q;
            p=(const uint8_t*)align4(q+1);++depth;node=NODE_OTHER;
            if(depth==1){if(streq(n,"memory")||(n[0]=='m'&&n[1]=='e'&&n[2]=='m'&&n[3]=='o'&&n[4]=='r'&&n[5]=='y'&&n[6]=='@')){node=NODE_MEMORY;md=depth;}}
            continue;
        }
        if(tag==FDT_END_NODE){--depth;node=NODE_OTHER;continue;}
        if(tag==FDT_PROP){
            if(p+8>end)return 0;uint32_t len=be32(*(const uint32_t*)p),no=be32(*(const uint32_t*)(p+4));p+=8;if(p+len>end)return 0;
            const char*name=(no<sts)?(const char*)(strings+no):0;
            if(depth==0&&streq(name,"#address-cells")&&len>=4)ac=be32(*(const uint32_t*)p);
            else if(depth==0&&streq(name,"#size-cells")&&len>=4)sc=be32(*(const uint32_t*)p);
            else if(depth==1&&node==NODE_MEMORY&&streq(name,"reg")&&(ac==2||ac==1)&&(sc==2||sc==1)&&len>=(ac+sc)*4){
                const uint32_t*v=(const uint32_t*)p;rb=(uintptr_t)cells(v,ac);rs=cells(v+ac,sc);have_ram=1;
            } else if(depth>=1&&streq(name,"compatible")){
                if(compat_has(p,len,"arm,pl011"))node=NODE_UART;
                else if(compat_has(p,len,"arm,gic-v3"))node=NODE_GIC;
                else if(compat_has(p,len,"arm,armv8-timer"))node=NODE_TIMER;
            } else if(node==NODE_UART&&streq(name,"reg")&&(ac==2||ac==1)&&len>= (ac+1)*4){
                ub=(uintptr_t)cells((const uint32_t*)p,ac);
            } else if(node==NODE_GIC&&streq(name,"reg")&&(ac==2||ac==1)&&sc>=1&&len>=(ac+sc)*4){
                const uint32_t*v=(const uint32_t*)p;gb=(uintptr_t)cells(v,ac);
                if(len>=(ac+sc)*8)gr=(uintptr_t)cells(v+ac+sc,ac);
            }
            else if(node==NODE_TIMER&&streq(name,"interrupts")&&len>=24){
                const uint32_t*v=(const uint32_t*)p;
                timer_ppi=be32(v+4)+16;
            }
            p=(const uint8_t*)align4((uintptr_t)(p+len));continue;
        }
        if(tag==FDT_NOP)continue;if(tag==FDT_END)break;return 0;
    }
    i->address=a;i->total_size=total;i->ram_base=rb;i->ram_size=rs;i->uart_base=ub;i->gicd_base=gb;i->gicr_base=gr;i->timer_ppi=timer_ppi;i->valid=have_ram&&ub&&gb&&gr&&timer_ppi;
    return i->valid;
}
static void print_hex(uint64_t v){extern void uart_putc(char);static const char h[]="0123456789abcdef";for(int i=15;i>=0;--i)uart_putc(h[(v>>(i*4))&15]);}
void dtb_print_info(const struct dtb_info*i){extern void uart_puts_public(const char*);
    uart_puts_public("DTB: valid flattened device tree.\nDTB: address = 0x");print_hex(i->address);
    uart_puts_public("\nDTB: RAM base = 0x");print_hex(i->ram_base);
    uart_puts_public("\nDTB: RAM size = 0x");print_hex(i->ram_size);
    uart_puts_public("\nDTB: UART base = 0x");print_hex(i->uart_base);
    uart_puts_public("\nDTB: GICD base = 0x");print_hex(i->gicd_base);
    uart_puts_public("\nDTB: GICR base = 0x");print_hex(i->gicr_base);
    uart_puts_public("\nDTB: timer PPI = 0x");print_hex(i->timer_ppi);uart_puts_public("\n");
}
