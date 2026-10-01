#ifndef VIRTIO_BLK_H
#define VIRTIO_BLK_H
#include <stdint.h>

int virtio_blk_probe(uintptr_t base);
uint32_t virtio_blk_device_id(void);

#endif
