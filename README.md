# BlackPeak OS

<div align="center">

### A 32-bit x86 monolithic operating system kernel built from scratch

**Higher-Half Kernel · Virtual Memory · Per-Process Address Spaces · Ring 3 · System Calls · Preemptive Multitasking · VFS · ramfs · tarfs**

</div>

---

## Overview

**BlackPeak OS** is a 32-bit x86 monolithic operating system developed from scratch in freestanding C and x86 assembly.

The project focuses on understanding and implementing the mechanisms that form the foundation of an operating system: processor protection, interrupt handling, virtual memory, physical memory management, address-space isolation, task scheduling, context switching, kernel memory allocation, user-mode execution, system calls, and filesystem abstraction.

Rather than relying on an existing kernel or operating-system framework, BlackPeak OS implements these mechanisms directly against the **i386 architecture** and validates them incrementally under emulation and low-level debugging tools.

The current `vfs-work` branch represents a substantial evolution from the original minimal bootable kernel. The kernel now provides:

- A higher-half kernel layout
- x86 protected-mode execution
- GDT, IDT, and TSS infrastructure
- Hardware interrupt handling
- Paging and recursive page-directory mapping
- Physical frame allocation
- Dynamic virtual memory mapping
- Per-process address spaces
- CR3 address-space switching
- Shared kernel mappings
- User/supervisor page permissions
- TLB invalidation
- A demand-paging foundation
- Kernel heap allocation with split and coalesce
- Preemptive multitasking
- Task and thread management
- Context switching
- Ring 3 user-mode execution
- TSS-based kernel stack switching
- `INT 0x80` system-call entry
- A Virtual File System (VFS) layer
- An in-memory writable filesystem (ramfs)
- A read-only embedded filesystem (tarfs)
- VGA and UART output
- PIC and PIT drivers
- Kernel panic handling

The project is intentionally developed incrementally: each subsystem is introduced, tested, debugged, and integrated into the larger kernel architecture.

---

## Engineering Focus

BlackPeak OS is primarily a systems-programming project.

The implementation explores:

- x86 protected-mode architecture
- CPU privilege levels
- Interrupt and exception handling
- Descriptor tables
- Hardware task-state mechanisms
- Physical memory management
- Virtual memory
- Page tables and page directories
- Address-space isolation
- Kernel/user memory permissions
- Context switching
- Timer-driven scheduling
- Kernel stack management
- System-call boundaries
- Filesystem abstraction
- Low-level device I/O
- Kernel heap management
- Debugging kernels without an existing operating system underneath

---

# Architecture

```text
                         User Applications
                              Ring 3
                                │
                                │ INT 0x80
                                │
                                ▼
┌───────────────────────────────────────────────────────┐
│                  BLACKPEAK KERNEL                     │
│                         Ring 0                        │
│                                                       │
│  Scheduler        Virtual Memory       System Calls   │
│  Tasks/Threads    Physical Memory      VFS            │
│  Context Switch   Kernel Heap          ramfs          │
│  Interrupts       Address Spaces       tarfs          │
│  Panic Handler    Drivers              Console        │
└───────────────────────────────────────────────────────┘
                                │
                                ▼
              GDT · TSS · IDT · PIC · PIT · Paging
                                │
                                ▼
                         x86 Hardware

Kernel organization

The kernel is structured around several cooperating subsystems:
text

Boot / Initialization
        │
        ├── GDT
        ├── TSS
        ├── IDT
        ├── PIC
        ├── PIT
        │
        ▼
Memory Subsystem
        │
        ├── Physical Frame Allocator
        ├── Paging
        ├── Virtual Page Mapping
        ├── Recursive Mapping
        ├── Address Spaces
        ├── CR3 Switching
        └── Demand Paging Foundation
        │
        ▼
Filesystem Subsystem
        │
        ├── VFS Layer
        ├── ramfs (writable, in-memory)
        └── tarfs (read-only, embedded)
        │
        ▼
Execution Subsystem
        │
        ├── Tasks
        ├── Threads
        ├── Context Management
        ├── Context Switching
        └── Preemptive Scheduler
        │
        ▼
User/Kernel Boundary
        │
        ├── Ring 3
        ├── User Address Spaces
        ├── TSS Kernel Stack Switching
        └── INT 0x80 System Calls

Feature Matrix
Boot and Kernel
Feature	Status
GRUB Multiboot boot	✅
BIOS boot path	✅
x86 assembly bootstrap	✅
32-bit i386 protected mode	✅
Freestanding C kernel	✅
Custom linker script	✅
Higher-half kernel layout	✅
Kernel initialization pipeline	✅
Monolithic kernel architecture	✅
CPU Protection and Descriptor Tables
Feature	Status
Global Descriptor Table (GDT)	✅
Kernel code/data segments	✅
User code/data segments	✅
Interrupt Descriptor Table (IDT)	✅
Task State Segment (TSS)	✅
Ring 0 execution	✅
Ring 3 execution	✅
Ring 0 ↔ Ring 3 transitions	✅
TSS-based kernel stack switching	✅
Interrupts and Exceptions
Feature	Status
IDT initialization	✅
ISR infrastructure	✅
Exception handling	✅
IRQ infrastructure	✅
Programmable Interrupt Controller (PIC)	✅
Programmable Interval Timer (PIT)	✅
Hardware timer interrupts (IRQ0)	✅
Interrupt-driven preemption	✅
Kernel panic handling	✅
Keyboard interrupt handler code	✅ (registered path present; IRQ1 currently masked)
Memory Management
Feature	Status
32-bit x86 paging	✅
Higher-half kernel mapping	✅
Recursive page-directory mapping	✅
Bitmap physical frame allocator	✅
Dynamic virtual page mapping (map_page)	✅
Scratch page-table window	✅
TLB invalidation (invlpg + CR3 reload)	✅
Per-process page directories	✅
CR3 address-space switching	✅
User/supervisor page permissions	✅
Read/write permission control	✅
Shared kernel mappings across processes	✅
Isolated user address space (PDE 2+)	✅
Demand-paging foundation	✅ (page-fault handler + lazy allocation hook)
Kernel Heap
Feature	Status
Heap initialization	✅
kmalloc / kfree	✅
Block splitting	✅
Block coalescing (forward + backward)	✅
Page-by-page heap growth	✅
4-byte size alignment	✅
Stress test (200 mixed allocations)	✅
Virtual File System (VFS)
Feature	Status
vfs_node_t abstraction	✅
vfs_ops_t (read/write/open/close/readdir/finddir)	✅
vfs_init	✅
vfs_mount	✅
vfs_root	✅
Path lookup (vfs_lookup)	✅
Multi-filesystem coexistence	✅
ramfs (Writable In-Memory Filesystem)
Feature	Status
Directory creation	✅
File creation	✅
File read	✅
File write	✅
readdir / finddir	✅
Directory attachment	✅
Up to 64 children per directory	✅
Up to 2048 bytes per file	✅
tarfs (Read-Only Embedded Filesystem)
Feature	Status
USTAR header parsing	✅
Octal size field parsing	✅
512-byte block walk	✅
Zero-block end detection	✅
File node creation	✅
finddir via readdir scan	✅
File read (tarfs_read)	✅
Mounted at /initrd	✅
Data embedded via .incbin	✅
Scheduling and Multitasking
Feature	Status
Round-robin scheduler	✅
PIT-driven preemption (100 Hz)	✅
Per-task kernel stacks	✅
Task states (UNUSED / READY / RUNNING)	✅
CR3 switching per task	✅
TSS esp0 update per task	✅
Assembly context save/restore	✅
Kernel task scheduling	✅
Ring 3 task scheduling	✅
Concurrent task execution verified (ABABAB)	✅
User Mode and Privilege Separation
Feature	Status
Ring 3 execution	✅
enter_user_mode_v2 IRET transition	✅
User code mapping	✅
User stack mapping	✅
User address space isolation	✅
TSS kernel stack switching	✅
Return to Ring 3 after syscall	✅
System Calls
Feature	Status
INT 0x80 gate (DPL 3)	✅
Syscall stub (context save/restore)	✅
Syscall handler	✅
File-related syscalls (open/read/write)	⬜ planned
Drivers and Hardware
Feature	Status
VGA text-mode console	✅
UART serial console (COM1)	✅
PIC remap (IRQ0–15 → 0x20–0x2F)	✅
PIT configuration (100 Hz)	✅
Keyboard scan-code driver code	✅
Keyboard IRQ actively dispatched	⬜ (IRQ1 currently masked)
Kernel panic handler	✅
Validation
Item	Status
Boot verified in QEMU	✅
Boot verified in Oracle VirtualBox	✅
Boot verified in Bochs	✅
VGA output verified	✅
UART output verified	✅
Scheduler verified via ABABAB stream	✅
Syscalls verified via continuous >>> SYSCALL FROM RING 3 <<<	✅
VFS verified via vfs_lookup	✅
ramfs verified via write + read round-trip	✅
tarfs verified via three successive tar-content changes	✅
Heap verified via 200-allocation stress test	✅
Boot Pipeline

The kernel initialization follows a staged low-level boot process:
text

GRUB
 │
 ▼
Multiboot Entry
 │
 ▼
x86 Assembly Bootstrap
 │
 ▼
Protected Mode
 │
 ▼
Higher-Half Kernel Entry
 │
 ▼
GDT
 │
 ▼
TSS
 │
 ▼
IDT
 │
 ▼
PIC
 │
 ▼
PIT
 │
 ▼
Heap Init
 │
 ▼
VFS Init
 │
 ▼
ramfs Init
 │
 ▼
tarfs Init
 │
 ▼
Scratch PDE / Recursive Setup
 │
 ▼
Page Directory Creation (pdA / pdB / pdUser)
 │
 ▼
Kernel Task A + Task B Creation
 │
 ▼
User Task Creation
 │
 ▼
Interrupts Enabled (`sti`)
 │
 ▼
Preemptive Scheduling Begins
 │
 ▼
Ring 3 Execution + INT 0x80 System Calls

The exact initialization dependencies are encoded in the kernel source and build system rather than being delegated to an existing operating-system runtime.
Memory Management Detail
Physical memory

    Bitmap allocator over 4096 frames

    Physical addresses start at 0x800000 (8 MB)

    Kernel's own physical range marked as used during boot

    Frames allocated on demand for page tables, page directories, and heap pages

Virtual memory layout
Virtual range	Purpose
0x00000000 – 0x007FFFFF	Identity mapped (bootstrap, kernel stack)
0x00800000 – 0x008FFFFF	User code and stack (PDE 2)
0xC0000000 – 0xC01FFFFF	Higher-half kernel code/data
0xC1000000	Scratch page-table window (PDE 772)
0xD0000000	Kernel heap start
0xFFC00000	Recursive page-table window
0xFFFFF000	Recursive page-directory
Address-space creation

create_process_address_space():

    Allocates a fresh physical frame for the new page directory

    Maps it temporarily through the scratch window (PDE 772)

    Zeroes entries 0–767 (private user space)

    Copies PDEs 0 and 1 (identity maps, so the low bootstrap stack survives CR3 switches)

    Copies PDEs 768–1022 (kernel space) except PDE 772 (private scratch)

    Sets PDE 1023 (recursive) to the new PD

    Returns the physical address, ready to be loaded into CR3

Demand-paging foundation

The page-fault path is wired into the interrupt subsystem. The handler detects user-mode, not-present faults, allocates a frame, maps it, zeroes it, and returns — a foundation for future lazy allocation. This is explicitly a foundation, not a fully realized demand-paging implementation.
Filesystem Detail
VFS

A thin node/ops abstraction that both concrete filesystems register against.

    vfs_node_t carries name, flags, size, ops pointer, and a filesystem-specific internal pointer.

    vfs_ops_t carries read, write, open, close, readdir, finddir.

    vfs_lookup(path) walks the tree component by component.

ramfs

An in-memory writable filesystem backed by the kernel heap.

    Files allocate their data at kmalloc time

    Directories hold up to 64 children

    Files up to 2048 bytes

    readdir and finddir implemented

    Attach helper exported to allow other filesystems to attach nodes

tarfs

A read-only filesystem that parses a USTAR archive embedded in the kernel binary.

    Source data is a tar blob linked via .incbin in the .initrd section

    Walks 512-byte blocks, parses octal size fields, stops at zero blocks

    Each regular file becomes a VFS node

    Mounted at /initrd

    Data pointer is set directly into the embedded archive (zero copy)

Build and Run
Requirements

A Linux development environment with:

    GCC (with -m32 support)

    GNU Binutils

    GRUB tools (grub2-mkrescue)

    xorriso

    QEMU (optional, for testing)

    Make

The kernel targets 32-bit x86 / i386.
Build
bash

make clean
make
cp kernel.elf iso_root/boot/kernel.elf
grub2-mkrescue -o kernel.iso iso_root

Run
bash

qemu-system-i386 -cdrom kernel.iso -serial stdio

The serial output is particularly useful for kernel diagnostics and debugging. VGA output reflects kprint calls; serial reflects both kprint and direct UART writes (scheduler ticks, syscalls).
Repository Structure
text

BlackPeakOS/
│
├── boot.S
├── kernel.c
├── linker.ld
├── link.ld
│
├── gdt.c
├── gdt.h
├── gdt_flush.s
│
├── idt.c
├── idt.h
├── idt_asm.s
│
├── tss.c
├── tss.h
├── tss_flush.s
│
├── isr.c
├── irq.c
├── interrupt.h
│
├── pic.c
├── pic.h
├── timer.c
├── timer.h
│
├── paging.c
├── paging.h
│
├── heap.c
├── heap.h
│
├── vfs.c
├── vfs.h
├── ramfs.c
├── ramfs.h
├── tarfs.c
├── tarfs.h
│
├── initrd.s
├── initrd.tar
├── initrd/
│   ├── hello.txt
│   └── readme.txt
│
├── scheduler.c
├── scheduler.h
├── task.c
├── task.h
├── thread.c
├── thread.h
├── context.c
├── context.h
├── context_switch.s
│
├── syscall.c
│
├── user.c
├── user.h
│
├── keyboard.c
├── keyboard.h
│
├── console.c
├── console.h
├── uart.c
├── uart.h
│
├── panic.c
├── panic.h
│
├── io.h
│
├── iso_root/
│   └── boot/
│       ├── kernel.elf
│       └── grub/
│           └── grub.cfg
│
├── docs/
│
├── Makefile
├── LICENSE
└── README.md

Testing and Validation

BlackPeak OS is developed and validated incrementally rather than treating the kernel as a single black box.

Testing and debugging have included:

    QEMU

    Oracle VirtualBox

    Bochs

    VGA diagnostics

    UART serial logging

    objdump

    nm

    GDB-compatible debugging workflows

    Boot-time diagnostics

    Interrupt-path debugging

    Memory-management validation

    Scheduler/context-switch validation

    Filesystem validation

The repository also contains testing artifacts documenting emulator-based validation.
Development Philosophy

The project follows a bottom-up systems-development approach.

Instead of immediately building user applications, the kernel establishes the mechanisms underneath them first:
text

CPU Protection
      ↓
Interrupts
      ↓
Physical Memory
      ↓
Virtual Memory
      ↓
Address Spaces
      ↓
Task / Thread Execution
      ↓
Preemptive Scheduling
      ↓
User Mode
      ↓
System Calls
      ↓
Filesystems
      ↓
Future User-Space Services

This approach makes each subsystem independently understandable while allowing the pieces to form a coherent operating-system architecture.
Current Status
Implemented

    ☑

    GRUB Multiboot boot
    ☑

    32-bit i386 protected mode
    ☑

    Freestanding C kernel
    ☑

    Custom linker layout with VMA/LMA alignment
    ☑

    Higher-half kernel
    ☑

    GDT
    ☑

    IDT
    ☑

    TSS
    ☑

    Ring 3 execution
    ☑

    Ring 0 ↔ Ring 3 transitions
    ☑

    Hardware interrupt infrastructure
    ☑

    PIC
    ☑

    PIT (100 Hz)
    ☑

    Timer-driven preemption
    ☑

    VGA console
    ☑

    UART serial debugging
    ☑

    Physical frame allocator
    ☑

    Paging
    ☑

    Higher-half paging
    ☑

    Recursive page-directory mapping
    ☑

    Scratch page-table window
    ☑

    Dynamic virtual page mapping
    ☑

    User/supervisor page permissions
    ☑

    TLB invalidation
    ☑

    CR3 switching
    ☑

    Per-process page directories
    ☑

    Shared kernel mappings
    ☑

    User address spaces
    ☑

    Demand-paging foundation
    ☑

    Kernel heap
    ☑

    Heap block splitting
    ☑

    Heap block coalescing
    ☑

    Heap growth on demand
    ☑

    Heap stress test (200 allocations)
    ☑

    VFS layer
    ☑

    ramfs (writable, in-memory)
    ☑

    tarfs (read-only, embedded)
    ☑

    Task management
    ☑

    Thread infrastructure
    ☑

    Context management
    ☑

    Assembly context switching
    ☑

    Round-robin scheduling
    ☑

    PIT-driven preemption
    ☑

    Kernel task scheduling
    ☑

    Ring 3 task scheduling
    ☑

    INT 0x80 system-call entry
    ☑

    System-call dispatcher
    ☑

    Kernel panic handling
    ☑

    Full integration test: heap stress + VFS + ramfs + tarfs + ABABAB + Ring-3 syscalls, in a single boot

Roadmap

The current kernel provides the foundation for higher-level operating-system functionality.
Next stages

    □

    File-related system calls (open, read, write, close) exposed to Ring 3
    □

    ELF executable loading
    □

    User-space program loading
    □

    Complete demand-paging behavior (lazy allocation + swap)
    □

    Persistent storage drivers (ATA / AHCI)
    □

    On-disk filesystems (FAT, ext2)
    □

    Additional device drivers
    □

    Richer user-space process model
    □

    Multiple concurrent user processes
    □

    fork / exec
    □

    Inter-process communication

The roadmap is intentionally layered on top of the existing kernel primitives rather than bypassing them.
Design Decisions

A few choices that shaped the architecture:

    Higher-half kernel so user space can occupy the full low 3 GB and the kernel lives above 3 GB, protected by supervisor page permissions.

    Recursive page-directory mapping (PDE 1023) so the kernel can edit page tables using ordinary pointer arithmetic, without needing a temporary mapping or identity-map assumptions.

    Scratch page-table window (PDE 772) so that page directories can be built without ever overwriting the master PD or the recursive window.

    Identity-map preservation (PDE 0 and 1) in every new process PD, so the low bootstrap stack and VGA remain accessible during CR3 switches.

    Bitmap frame allocator starting at 8 MB — simple, deterministic, easy to reason about during early development.

    Per-process page directories with shared kernel PDEs — standard monolithic-kernel approach: user space is private, kernel space is shared but protected by the supervisor bit.

    VFS as a node/ops layer so future filesystems can plug in without touching the VFS core.

Why This Project Matters

BlackPeak OS is not intended to compete with mature operating systems such as Linux or BSD.

Its purpose is different:

to implement and understand the mechanisms that make an operating system work.

The project requires reasoning about:

    CPU privilege transitions

    Page-table structures

    Physical-to-virtual address translation

    CR3 and address-space switching

    Interrupt entry and return paths

    Kernel stack management

    Context preservation

    Scheduler state

    User/kernel memory boundaries

    Hardware timer preemption

    System-call entry

    Filesystem abstraction

    Low-level device I/O

Working at this level provides practical experience with the boundary between software and hardware that is difficult to obtain through conventional application development alone.
References

    Intel® 64 and IA-32 Architectures Software Developer's Manual

    OSDev Wiki

    GRUB Multiboot Specification

    USTAR (POSIX tar) format specification

License

MIT License.
Author

Hariharan J

Systems Programming · Operating Systems · Kernel Development · x86 Architecture
<div align="center">
BlackPeak OS

From bootloader to Ring 3 — building the kernel from the hardware boundary upward.
</div> ```