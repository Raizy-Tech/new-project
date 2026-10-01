# RaizyOS

Personal ARM64 operating-system project.

Target: AArch64 on QEMU virt. Language: C + AArch64 assembly.

Milestone 1: boot directly into an ARM64 kernel and print a message through the QEMU PL011 UART.

Build with an AArch64 bare-metal GCC toolchain and QEMU:

    make
    make run

Roadmap: boot -> exceptions -> timer -> MMU -> memory manager -> scheduler -> processes -> syscalls -> drivers -> filesystem -> networking -> userland -> optional AI service.

The AI subsystem will remain outside the kernel so the OS remains usable without Internet access or an LLM provider.
