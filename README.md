# VEX OS

> **Capability-Secured x86_64 Operating System**
>
> **VEX OS** is an experimental capability-secured operating system for x86_64 platforms built completely from scratch.
>
> The project focuses on secure process execution, capability-based access control, signed application packages, isolated virtual address spaces, and a modern desktop environment.

---

## Philosophy

Unlike traditional hobby operating systems that stop after displaying a framebuffer or launching a shell, VEX OS is designed as a complete operating system with its own execution model, desktop environment, package format, SDK and development tooling.

The long-term goal is to build a secure, modular operating system where applications communicate through well-defined interfaces instead of unrestricted kernel access.

---

# Current Project Status

**Development Stage**

> **Alpha / Active Development**

Current implementation already includes:

* Working UEFI boot chain
* ELF64 executable loading
* Package signature verification
* Physical & virtual memory management
* Process manager
* Priority scheduler
* IPC subsystem
* Capability-aware syscall dispatcher
* Desktop shell
* Window manager
* Rust host SDK
* Integration tests

Current limitations:

* Applications still execute in Ring 0
* No filesystem
* No networking stack
* No GPU acceleration
* Cooperative scheduling
* Single-core execution

---

# Features

## Boot

* UEFI bootloader
* ELF64 loader
* Signed application loading
* ed25519 package verification
* SHA-256 hashing
* Boot information handoff

---

## Kernel

* Freestanding C kernel
* Physical page allocator
* 4-level paging
* Per-process address spaces
* Process manager
* Cooperative priority scheduler
* Capability-aware syscall dispatch
* IPC channels
* Interrupt handling
* GDT
* TSS
* IDT
* ACPI parser
* CMOS RTC

---

## Graphics

* UEFI GOP framebuffer
* Software compositor
* Surface manager
* Fence synchronization
* Desktop renderer
* Window manager
* Taskbar
* Start menu
* Console renderer

---

## Input

* PS/2 keyboard
* PS/2 mouse
* Keyboard navigation
* Window dragging
* Focus switching

---

## Security

* Capability-based permissions
* Package verification
* ed25519 signatures
* SHA-256 integrity checks
* Manifest-based permissions

---

## Tooling

* Rust SDK (`vex`)
* Package builder
* Package verifier
* Package inspector
* Smoke tests
* Python tooling
* Asset generation

---

# Current Implementation

## Implemented

### Boot

* ✅ UEFI Bootloader
* ✅ ELF64 parser
* ✅ Kernel loader
* ✅ Package loader
* ✅ Package verification
* ✅ Boot information transfer

### Memory

* ✅ Physical page allocator
* ✅ Virtual memory manager
* ✅ 4-level paging
* ✅ Per-process CR3
* ✅ Address space management

### Execution

* ✅ GDT
* ✅ TSS
* ✅ IDT
* ✅ Interrupt handlers
* ✅ Syscall entry
* ✅ Capability-aware syscall dispatcher

### Process Management

* ✅ Process creation
* ✅ Process manager
* ✅ Priority scheduler
* ✅ IPC ring buffers

### Graphics

* ✅ Framebuffer initialization
* ✅ Surface manager
* ✅ Fence management
* ✅ Desktop renderer
* ✅ Window manager
* ✅ Console output

### Hardware

* ✅ PS/2 keyboard
* ✅ PS/2 mouse
* ✅ CMOS RTC
* ✅ ACPI (RSDP/XSDT/MADT)

### Applications

* ✅ Init Shell
* ✅ Desktop Environment
* ✅ Compositor Service
* ✅ GPU Service
* ✅ Terminal
* ✅ Diagnostics
* ✅ Test Center
* ✅ Explorer

### Host Tooling

* ✅ Rust SDK
* ✅ Build tooling
* ✅ Package tooling
* ✅ Smoke tests
* ✅ Package verification tests

---

# Work In Progress

* Ring 3 execution
* Preemptive scheduler
* Zero-copy IPC
* Versioned syscall ABI
* SMP support
* VFS
* User-space drivers
* Vulkan backend
* Wayland-like display protocol
* Reproducible builds

---

# Planned

* Filesystem
* Networking
* GPU acceleration
* Hot-reloadable drivers
* Kernel fault injection
* IPC benchmarks
* Scheduler benchmarks
* Capability validation
* CI/CD
* Unit testing

---

# Architecture

```text
                                HARDWARE
                   CPU • RAM • UEFI • Framebuffer
                              • PS/2 •

                                   │
                                   ▼

                           UEFI BOOTLOADER
──────────────────────────────────────────────────────────────
ELF64 Loader
Package Verification
ed25519
SHA-256
Memory Mapping

                                   │
                                   ▼

                              VEX KERNEL
──────────────────────────────────────────────────────────────
Memory Manager
Scheduler
Process Manager
Address Spaces
IPC
Syscalls
Graphics
Console
ACPI
RTC
Interrupts

                                   │
                                   ▼

                           SYSTEM SERVICES
──────────────────────────────────────────────────────────────
Init Shell
Compositor
GPU Service
Explorer
Diagnostics
Terminal
Test Center

                                   │
                                   ▼

                             APPLICATIONS
──────────────────────────────────────────────────────────────
Currently executing in Ring 0
(Ring 3 support in development)
```

---

# Repository Layout

```text
vex_os/

├── boot/           UEFI bootloader
├── kernel/         Kernel source
├── apps/           System services & applications
├── tools/          Rust SDK & development tools
├── tests/          Integration tests
├── docs/           Documentation
│
├── CMakeLists.txt
└── command-to-run.md
```

---

# Technologies

| Component         | Technology          |
| ----------------- | ------------------- |
| Kernel            | Freestanding C      |
| Bootloader        | C + x86_64 Assembly |
| SDK               | Rust                |
| Tooling           | Python              |
| Build System      | CMake               |
| Firmware          | UEFI (EDK2)         |
| Emulator          | QEMU + OVMF         |
| Cryptography      | ed25519 (TweetNaCl) |
| Hashing           | SHA-256             |
| Executable Format | ELF64               |
| Package Format    | VEXPKG2             |
| Configuration     | JSON                |
# Build Requirements

## Host Environment

The project can be built on Windows, Linux or macOS.

### Required Software

| Component    | Version       |
| ------------ | ------------- |
| CMake        | ≥ 3.16        |
| LLVM / Clang | Recommended   |
| Ninja        | Recommended   |
| Rust + Cargo | Latest Stable |
| Python       | ≥ 3.8         |
| QEMU         | ≥ 7.0         |
| OVMF (EDK2)  | Required      |

### Python Packages

```bash
pip install Pillow
```

---

# Building

Configure the build:

```bash
cmake -B build -G Ninja
```

Build:

```bash
cmake --build build
```

Generated output:

```text
build/out/

kernel.bin
bootloader.bin
efi_root/
ovmf-vars.fd
*.bin
```

---

# Running

The full QEMU command is available in:

```text
command-to-run.md
```

or

```text
docs/running.md
```

---

# Applications

The current VEX OS image includes the following applications and services.

| Application | Type        | Description                                    |
| ----------- | ----------- | ---------------------------------------------- |
| Init Shell  | Service     | Desktop session, taskbar, application launcher |
| Compositor  | Service     | Software compositor                            |
| GPU Service | Service     | Graphics abstraction layer                     |
| Terminal    | Application | Terminal emulator                              |
| Diagnostics | Application | System diagnostics                             |
| Test Center | Application | Integration test launcher                      |
| Explorer    | Application | Service / file explorer                        |
| Demo Shell  | Application | Development testing application                |

---

# System Components

## Process Manager

Responsible for:

* Process creation
* Process scheduling
* Address space ownership
* Capability assignment

---

## Scheduler

Current implementation:

* Cooperative
* Priority based
* Single-core

Future implementation:

* Preemptive
* Time slicing
* SMP aware

---

## Memory Manager

Features:

* Physical page allocator
* 4-level paging
* Virtual memory
* Per-process CR3
* Independent address spaces

---

## IPC

Current IPC implementation:

* Ring-buffer channels
* 16 entries per channel
* Kernel managed

Planned:

* Zero-copy transport
* Shared memory channels
* Higher throughput

---

## Graphics

Current renderer:

* Software framebuffer
* Surface management
* Fence synchronization
* Desktop renderer
* Window manager

Future renderer:

* Vulkan backend
* Hardware acceleration

---

## Security

Current security model:

* Capability based permissions
* Manifest validation
* Package verification
* ed25519 signatures
* SHA-256 hashing

---

# Development Tools

The project includes several host-side tools.

## Rust SDK

```text
tools/vex/
```

Capabilities:

* build
* package
* verify
* inspect
* run

---

## Python Tools

```text
tools/
```

Available utilities:

* Wallpaper asset generator
* Binary blob generator
* Build analyzer

---

# Testing

Current tests include:

* Boot smoke tests
* Package verification tests

Located in:

```text
tests/
```

---

# Documentation

Complete documentation is available inside the `docs/` directory.

Included documents:

* architecture.md
* build.md
* developer-guide.md
* package-format.md
* project-summary.md
* roadmap.md
* running.md

---

# Current Limitations

Although VEX OS already boots into a functional desktop environment, several major subsystems are still under development.

Current limitations include:

* No Ring 3 execution
* No filesystem
* No networking stack
* No GPU acceleration
* No SMP
* Cooperative scheduler
* No zero-copy IPC
* No reproducible builds
* No CI/CD
* No hardware drivers beyond the current platform support

---

# Roadmap

## Near-term

* Ring 3 execution
* Preemptive scheduler
* Zero-copy IPC
* Versioned syscall ABI
* Kernel validation tests
* IPC benchmarks
* Capability validation

---

## Mid-term

* Virtual filesystem
* User-space drivers
* Wayland-like protocol
* Vulkan backend
* SMP support

---

## Long-term

* Networking
* GPU acceleration
* Hot-reloadable drivers
* Deterministic builds
* Continuous Integration
* Complete SDK
* Stable ABI

---

# Project Goals

The primary objective of VEX OS is to build a modern operating system that combines:

* Capability-based security
* Modular architecture
* High performance
* Modern desktop environment
* Native SDK
* Secure application distribution
* Clean system interfaces

The project is intended as a long-term systems programming effort and serves as a platform for experimenting with operating system design, kernel architecture and application execution models.

---

# Project Status

Current development stage:

> **Alpha**

Current functionality:

* Kernel boots successfully
* Core kernel subsystems initialize correctly
* Desktop environment renders successfully
* System services start automatically
* Application packages are verified during boot
* Desktop interaction (keyboard & mouse) is operational
* Smoke tests pass successfully

---

# License

**No open-source license is currently provided.**

Unless explicitly stated otherwise, **all rights are reserved**.

The source code is published for reference purposes only and may not be copied, modified, redistributed or used in other projects without prior written permission from the author.

---

# Acknowledgements

VEX OS is an independent operating system project developed from scratch.

It incorporates publicly available standards and specifications where appropriate (UEFI, ELF64, ACPI, x86_64 architecture) while implementing its own kernel, boot process, package format, desktop environment and development tooling.

---

<div align="center">

**VEX OS**

*Capability-Secured Operating System for x86_64*

*Built from scratch.*

</div>
