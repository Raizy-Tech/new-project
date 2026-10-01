#include <stdint.h>
#include "virtio_blk.h"
#include "pmm.h"

#define VIRTIO_MMIO_MAGIC 0x000
#define VIRTIO_MMIO_VERSION 0x004
#define VIRTIO_MMIO_DEVICE_ID 0x008
#define VIRTIO_MMIO_STATUS 0x070
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_QUEUE_SEL 0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034
#define VIRTIO_MMIO_QUEUE_NUM 0x038
#define VIRTIO_MMIO_QUEUE_READY 0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW 0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH 0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW 0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH 0x0a4
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03c
#define VIRTIO_MMIO_CONFIG 0x100
#define VIRTIO_BLK_DEVICE_ID 2U
#define VIRTIO_MAGIC 0x74726976U
#define VIRTIO_STATUS_ACK 1U
#define VIRTIO_STATUS_DRIVER 2U
#define VIRTIO_STATUS_DRIVER_OK 4U
#define VIRTIO_STATUS_FEATURES_OK 8U
#define VIRTIO_F_VERSION_1 32U
#define VIRTIO_BLK_T_IN 0U
#define VIRTIO_BLK_T_OUT 1U
#define VRING_DESC_F_WRITE 2U

struct vring_desc { uint64_t addr; uint32_t len; uint16_t flags; uint16_t next; };
struct vring_avail { uint16_t flags; uint16_t idx; uint16_t ring[8]; };
struct vring_used_elem { uint32_t id; uint32_t len; };
struct vring_used { uint16_t flags; uint16_t idx; struct vring_used_elem ring[8]; };
struct blk_req { uint32_t type; uint32_t reserved; uint64_t sector; };
struct blk_queue {
    struct vring_desc *desc;
    struct vring_avail *avail;
    struct vring_used *used;
    struct blk_req *req;
    uint8_t *data;
    uint16_t last_used;
};

static uintptr_t g_base;
static uint32_t g_device_id;
static struct blk_queue q;

static volatile uint32_t *reg32(uint32_t off){return (volatile uint32_t *)(g_base+off);}
static void memzero(void *p,uint32_t n){uint8_t *b=p;while(n--)*b++=0;}
static void barrier(void){__asm__ volatile("dsb sy" ::: "memory");}

int virtio_blk_probe(uintptr_t base){
    g_base=base; g_device_id=0;
    if(!base)return 0;
    if(*reg32(VIRTIO_MMIO_MAGIC)!=VIRTIO_MAGIC)return 0;
    uint32_t v=*reg32(VIRTIO_MMIO_VERSION);
    if(v!=1U&&v!=2U)return 0;
    if(*reg32(VIRTIO_MMIO_DEVICE_ID)!=VIRTIO_BLK_DEVICE_ID)return 0;
    g_device_id=VIRTIO_BLK_DEVICE_ID;
    return 1;
}

static int setup_queue(void){
    *reg32(VIRTIO_MMIO_QUEUE_SEL)=0;
    uint32_t max=*reg32(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if(!max)return 0;
    uint32_t n=max>8?8:max;
    *reg32(VIRTIO_MMIO_QUEUE_NUM)=n;
    uintptr_t dp=pmm_alloc_page(), ap=pmm_alloc_page(), up=pmm_alloc_page();
    uintptr_t rp=pmm_alloc_page(), bp=pmm_alloc_page();
    if(!dp||!ap||!up||!rp||!bp)return 0;
    memzero((void*)dp,4096);memzero((void*)ap,4096);memzero((void*)up,4096);
    memzero((void*)rp,4096);memzero((void*)bp,4096);
    q.desc=(struct vring_desc*)dp;q.avail=(struct vring_avail*)ap;
    q.used=(struct vring_used*)up;q.req=(struct blk_req*)rp;q.data=(uint8_t*)bp;q.last_used=0;
    q.desc[0].addr=(uint64_t)rp;q.desc[0].len=16;q.desc[0].flags=0;q.desc[0].next=1;
    q.desc[1].addr=(uint64_t)bp;q.desc[1].len=512;q.desc[1].flags=VRING_DESC_F_WRITE;q.desc[1].next=2;
    q.desc[2].addr=(uint64_t)bp;q.desc[2].len=0;q.desc[2].flags=VRING_DESC_F_WRITE;
    uintptr_t d=(uintptr_t)q.desc,a=(uintptr_t)q.avail,u=(uintptr_t)q.used;
    *reg32(VIRTIO_MMIO_QUEUE_DESC_LOW)=(uint32_t)d;*reg32(VIRTIO_MMIO_QUEUE_DESC_HIGH)=(uint32_t)(d>>32);
    *reg32(VIRTIO_MMIO_QUEUE_AVAIL_LOW)=(uint32_t)a;*reg32(VIRTIO_MMIO_QUEUE_AVAIL_HIGH)=(uint32_t)(a>>32);
    *reg32(VIRTIO_MMIO_QUEUE_USED_LOW)=(uint32_t)u;*reg32(VIRTIO_MMIO_QUEUE_USED_HIGH)=(uint32_t)(u>>32);
    *reg32(VIRTIO_MMIO_QUEUE_READY)=1;
    return 1;
}

static int submit(uint32_t type,uint64_t sector){
    q.req->type=type;q.req->reserved=0;q.req->sector=sector;
    q.desc[1].flags=(type==VIRTIO_BLK_T_IN)?VRING_DESC_F_WRITE:0;
    q.desc[2].addr=(uint64_t)&q.req->reserved;q.desc[2].len=1;q.desc[2].flags=VRING_DESC_F_WRITE;
    q.avail->ring[q.avail->idx%8]=0;barrier();q.avail->idx++;barrier();
    *reg32(VIRTIO_MMIO_QUEUE_NOTIFY)=0;
    for(uint32_t i=0;i<1000000;i++){
        barrier();
        if(q.used->idx!=q.last_used){
            q.last_used=q.used->idx;
            return *(volatile uint8_t *)((uintptr_t)&q.req->reserved)==0;
        }
    }
    return 0;
}

static void set_status(uint32_t s){*reg32(VIRTIO_MMIO_STATUS)=s;barrier();}
static int device_init(void){
    set_status(0);set_status(VIRTIO_STATUS_ACK);set_status(VIRTIO_STATUS_ACK|VIRTIO_STATUS_DRIVER);
    uint32_t f=*reg32(VIRTIO_MMIO_DEVICE_FEATURES);
    uint32_t accepted=f&VIRTIO_F_VERSION_1;
    *reg32(VIRTIO_MMIO_DRIVER_FEATURES)=accepted;
    set_status(VIRTIO_STATUS_ACK|VIRTIO_STATUS_DRIVER|(accepted?VIRTIO_STATUS_FEATURES_OK:0));
    if(accepted&&(*reg32(VIRTIO_MMIO_STATUS)&VIRTIO_STATUS_FEATURES_OK)==0)return 0;
    if(!setup_queue())return 0;
    set_status(VIRTIO_STATUS_ACK|VIRTIO_STATUS_DRIVER|VIRTIO_STATUS_DRIVER_OK|(accepted?VIRTIO_STATUS_FEATURES_OK:0));
    return 1;
}

int virtio_blk_self_test(void){
    if(!g_device_id||!device_init())return 0;
    q.req->type=VIRTIO_BLK_T_IN;q.req->reserved=0;q.req->sector=0;
    if(!submit(VIRTIO_BLK_T_IN,0))return 0;
    volatile uint8_t *d=q.data;
    for(uint32_t i=0;i<512;i++)if(d[i]!=0)return 0;
    return 1;
}
uint32_t virtio_blk_device_id(void){return g_device_id;}
