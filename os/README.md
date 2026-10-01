# RaizyOS

Personal ARM64 operating-system project.

Target: AArch64 on QEMU virt. Language: C + AArch64 assembly.

Current milestones:
1. Direct ARM64 kernel boot with QEMU PL011 UART console.
2. Exception vector table installed through VBAR_EL1, with synchronous-exception diagnostics and an SVC #0 test.
3. ARM generic timer initialized through CNTFRQ_EL0/CNTP_TVAL_EL1/CNTP_CTL_EL0.
4. GICv3 timer interrupts, MMU, PMM, heap, scheduler and preemptive kernel threads.
5. AArch64 user ELF loading, EL0 entry and basic syscalls.
6. PL011 console input/output, read-only embedded RAM filesystem and interactive shell.

Build with an AArch64 bare-metal GCC toolchain and QEMU:

    make
    make run

Roadmap:
boot -> exceptions -> timer/GIC -> MMU/PMM/heap -> scheduler -> processes/syscalls -> console/RAMFS/shell -> storage drivers -> persistent filesystem -> networking -> richer userland -> optional AI service.

The AI subsystem will remain outside the kernel so the OS remains usable without Internet access or an LLM provider. The `awesome-free-llm-apis` project can later be used by an optional user-space AI client/service; API keys must remain outside the kernel and out of source control.
