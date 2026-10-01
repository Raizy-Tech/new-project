CROSS ?= aarch64-none-elf-
CC := $(CROSS)gcc
LD := $(CROSS)ld
CFLAGS := -ffreestanding -fno-builtin -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -O2 -mcpu=cortex-a53 -mgeneral-regs-only
LDFLAGS := -T linker.ld -nostdlib
BUILD := build
all: $(BUILD)/kernel.elf
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
$(BUILD)/dtb.o: dtb.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.elf: $(BUILD)/boot.o $(BUILD)/exceptions.o $(BUILD)/kernel.o $(BUILD)/exception.o $(BUILD)/gic.o $(BUILD)/timer.o $(BUILD)/mmu.o $(BUILD)/pmm.o $(BUILD)/dtb.o $(BUILD)/dtb.o linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD)/boot.o $(BUILD)/exceptions.o $(BUILD)/kernel.o $(BUILD)/exception.o $(BUILD)/gic.o $(BUILD)/timer.o $(BUILD)/mmu.o $(BUILD)/pmm.o
run: $(BUILD)/kernel.elf
	qemu-system-aarch64 -M virt,gic-version=3,highmem-redists=off -cpu cortex-a53 -m 128M -nographic -monitor none -kernel $(BUILD)/kernel.elf
clean:
	rm -rf $(BUILD)
.PHONY: all run clean
