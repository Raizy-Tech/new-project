CROSS ?= aarch64-none-elf-
CC := $(CROSS)gcc
LD := $(CROSS)ld
CFLAGS := -ffreestanding -fno-builtin -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -O2 -mcpu=cortex-a53 -mgeneral-regs-only
LDFLAGS := -T linker.ld -nostdlib
BUILD := build
all: $(BUILD)/kernel.elf
USER_LDFLAGS := -T user.ld -nostdlib
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/boot.o: boot.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/exceptions.o: exceptions.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.o: kernel.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/exception.o: exception.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/gic.o: gic.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/timer.o: timer.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/mmu.o: mmu.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/pmm.o: pmm.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/heap.o: heap.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/context.o: context.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/sched.o: sched.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/sched_test.o: sched_test.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/process.o: process.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/uart.o: uart.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/ramfs.o: ramfs.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/virtio_blk.o: virtio_blk.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/shell.o: shell.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/user.o: user.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/user.elf: $(BUILD)/user.o user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(BUILD)/user.o

$(BUILD)/user.bin: $(BUILD)/user.elf
	$(CROSS)objcopy -O binary $< $@

$(BUILD)/user_blob.o: $(BUILD)/user.elf
	$(LD) -r -b binary -o $@ $<
$(BUILD)/dtb.o: dtb.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.elf: $(BUILD)/boot.o $(BUILD)/exceptions.o $(BUILD)/kernel.o $(BUILD)/exception.o $(BUILD)/gic.o $(BUILD)/timer.o $(BUILD)/mmu.o $(BUILD)/pmm.o $(BUILD)/heap.o $(BUILD)/context.o $(BUILD)/sched.o $(BUILD)/sched_test.o $(BUILD)/process.o $(BUILD)/uart.o $(BUILD)/ramfs.o $(BUILD)/shell.o $(BUILD)/virtio_blk.o $(BUILD)/user_blob.o $(BUILD)/dtb.o linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD)/boot.o $(BUILD)/exceptions.o $(BUILD)/kernel.o $(BUILD)/exception.o $(BUILD)/gic.o $(BUILD)/timer.o $(BUILD)/mmu.o $(BUILD)/pmm.o $(BUILD)/heap.o $(BUILD)/context.o $(BUILD)/sched.o $(BUILD)/sched_test.o $(BUILD)/process.o $(BUILD)/uart.o $(BUILD)/ramfs.o $(BUILD)/shell.o $(BUILD)/virtio_blk.o $(BUILD)/user_blob.o $(BUILD)/dtb.o
run: $(BUILD)/kernel.elf
	qemu-system-aarch64 -M virt,gic-version=3,highmem-redists=off -cpu cortex-a53 -m 128M -nographic -monitor none -kernel $(BUILD)/kernel.elf
clean:
	rm -rf $(BUILD)
.PHONY: all run clean
