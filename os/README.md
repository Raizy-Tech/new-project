# RaizyOS

Personal ARM64 operating-system project.

Target: AArch64 on QEMU virt. Language: C + AArch64 assembly.

Current milestones:
1. Direct ARM64 kernel boot with QEMU PL011 UART console.
2. Exception vector table installed through VBAR_EL1, with synchronous-exception diagnostics and an SVC #0 test.
3. ARM generic timer initialized through CNTFRQ_EL0/CNTP_TVAL_EL1/CNTP_CTL_EL0.

Build with an AArch64 bare-metal GCC toolchain and QEMU:

    make
    make run

Roadmap:
boot -> exceptions -> generic timer -> GIC -> timer IRQ -> MMU -> physical memory allocator -> virtual memory -> scheduler -> processes -> syscalls -> drivers -> filesystem -> networking -> userland -> optional AI service.

The AI subsystem will remain outside the kernel so the OS remains usable without Internet access or an LLM provider. The `awesome-free-llm-apis` project can later be used by an optional user-space AI client/service; API keys must remain outside the kernel and out of source control.
