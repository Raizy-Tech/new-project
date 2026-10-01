# RaizyOS

Personal ARM64 operating-system project.

Target: AArch64 on QEMU virt. Language: C + AArch64 assembly.

Current tested milestones:
1. Direct ARM64 kernel boot with QEMU PL011 UART console.
2. Exception vectors through VBAR_EL1, synchronous exception handling, and SVC #0.
3. GICv3 Group-1 interrupt setup and ARM generic physical timer interrupts (PPI 30).

The CI workflow builds the cross-compiled kernel and boots it under QEMU, checking the UART console for the kernel, exception, and timer milestones.

Roadmap:
boot -> exceptions -> timer/GIC -> MMU -> physical memory allocator -> virtual memory -> scheduler -> processes -> syscalls -> drivers -> filesystem -> networking -> userland -> optional AI service.

The AI subsystem remains outside the kernel. The `awesome-free-llm-apis` resources can later support an optional user-space AI service without making the kernel dependent on Internet access or API keys.
