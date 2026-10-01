#include <stdint.h>
#include "virtio_blk.h"

#define VIRTIO_MMIO_MAGIC       0x000
#define VIRTIO_MMIO_VERSION     0x004
#define VIRTIO_MMIO_DEVICE_ID  0x008
#define VIRTIO_MMIO_VENDOR_ID  0x00c

#define VIRTIO_BLK_DEVICE_ID 2U
#define VIRTIO_MAGIC 0x74726976U

static uintptr_t g_base;
static uint32_t g_device_id;

static volatile uint32_t *reg32(uint32_t off) {
    return (volatile uint32_t *)(g_base + off);
}

int virtio_blk_probe(uintptr_t base) {
    g_base = base;
    g_device_id = 0;
    if (!base) return 0;

    uint32_t magic = *reg32(VIRTIO_MMIO_MAGIC);
    uint32_t version = *reg32(VIRTIO_MMIO_VERSION);
    uint32_t device = *reg32(VIRTIO_MMIO_DEVICE_ID);

    if (magic != VIRTIO_MAGIC || (version != 1U && version != 2U))
        return 0;
    if (device != VIRTIO_BLK_DEVICE_ID)
        return 0;

    g_device_id = device;
    return 1;
}

uint32_t virtio_blk_device_id(void) {
    return g_device_id;
}
