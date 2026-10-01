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

$(BUILD)/kernel.elf: $(BUILD)/boot.o $(BUILD)/exceptions.o $(BUILD)/kernel.o $(BUILD)/exception.o linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD)/boot.o $(BUILD)/exceptions.o $(BUILD)/kernel.o $(BUILD)/exception.o

run: $(BUILD)/kernel.elf
	qemu-system-aarch64 -M virt -cpu cortex-a53 -m 128M -nographic -kernel $(BUILD)/kernel.elf

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
