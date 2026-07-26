BdsDxe: loading Boot0001 "UEFI QEMU HARDDISK QM00001 " from PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0xFFFF,0x0)
BdsDxe: starting Boot0001 "UEFI QEMU HARDDISK QM00001 " from PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0xFFFF,0x0)
Vex loader: kernel parse
Vex loader: kernel parsed
Vex loader: kernel load
Vex loader: kernel loaded
Vex loader: init verified
Vex loader: diag verified
Vex loader: tests verified
Vex loader: services verified
Vex loader: terminal verified
Vex loader: compositor verified
Vex loader: gpu verified
Vex loader: capture boot state
Vex loader: boot state captured
vex:kernel:enter
vex:phase:framebuffer_banner
vex:phase:framebuffer_console
vex:phase:memory_init
vex:phase:address_space_init
vex:as:kernel base=0x0000000004000000 size=122880
vex:as:bootinfo base=0x0000000009B69000 size=8192 apps=6
vex:as:init_pkg base=0x0000000009B76000 size=33456128
vex:as:app_pkg index=0 base=0x000000000BB5D000 size=12288
vex:as:app_pkg index=1 base=0x000000000BB5F000 size=12288
vex:as:app_pkg index=2 base=0x000000000BB61000 size=12288
vex:as:app_pkg index=3 base=0x000000000BB63000 size=8192
vex:as:app_pkg index=4 base=0x000000000BB64000 size=16384
vex:as:app_pkg index=5 base=0x000000000BB67000 size=20480
vex:as:map_pkg index=0 base=0x0000000009B76000 size=33456128
vex:as:map_pkg index=1 base=0x000000000BB5D000 size=12288
vex:as:map_pkg index=2 base=0x000000000BB5F000 size=12288
vex:as:map_pkg index=3 base=0x000000000BB61000 size=12288
vex:as:map_pkg index=4 base=0x000000000BB63000 size=8192
vex:as:map_pkg index=5 base=0x000000000BB64000 size=16384
vex:as:map_pkg index=6 base=0x000000000BB67000 size=20480
vex:phase:acpi_init
vex:phase:timer_init
vex:phase:execution_init
vex:phase:interrupts_init
vex:phase:scheduler_init
vex:phase:process_init
vex:phase:graphics_init

[boot]
rsdp: 0x000000000F77E000
fb.base: 0x0000000080000000
fb.size: 4096000
fb.width: 1280
fb.height: 800
fb.pitch: 1280

[memory]
regions: 99
usable_pages: 63939
allocated_pages: 20
kernel.cr3: 0x0000000000100000
kernel.mapped_pages: 8214

[platform]
acpi.rsdp: 0x000000000F77E000
acpi.xsdt: 0x0000000000000000
acpi.madt: 0x000000000F778000
acpi.cpus: 1
acpi.ioapics: 1
timer.kind: 0
timer.base: 0x0000000000000000
timer.period_fs: 0
timer.ready: 0
exec.gdt_loaded: 1
exec.tss: 0x000000000401BD98
exec.user_cs: 0x0000000000000023
exec.user_ss: 0x000000000000001B
gfx.surfaces: 0
gfx.fences: 0
vex:memory:regions=99 usable_pages=63939 allocated_pages=20
vex:vm:kernel_cr3=0x0000000000100000 mapped_pages=8214
vex:exec:gdt=1 tss=0x000000000401BD98 user_cs=0x0000000000000023 user_ss=0x000000000000001B
vex:gfx:surfaces=0 fences=0
selftest:PASS
vex:as:map_pkg index=0 base=0x0000000009B76000 size=33456128
vex:as:map_pkg index=1 base=0x000000000BB5D000 size=12288
vex:as:map_pkg index=2 base=0x000000000BB5F000 size=12288
vex:as:map_pkg index=3 base=0x000000000BB61000 size=12288
vex:as:map_pkg index=4 base=0x000000000BB63000 size=8192
vex:as:map_pkg index=5 base=0x000000000BB64000 size=16384
vex:as:map_pkg index=6 base=0x000000000BB67000 size=20480

[init]
domain: init-shell
entry: svc/init-shell
caps: 0x000000000000001F
services: 1
processes: 1
vex:init:domain=init-shell entry=svc/init-shell caps=0x000000000000001F
vex:services:count=1 processes=1
init.cr3: 0x0000000000113000
init.entry_va: 0x0000000040000000
init.stack_va: 0x0000000070000FF8
vex:init:cr3=0x0000000000113000 entry_va=0x0000000040000000 stack_va=0x0000000070000FF8
user.cs: 0x0000000000000023
user.ss: 0x000000000000001B
user.rip: 0x0000000040000000
user.rsp: 0x0000000070000FF8
vex:init:user_ctx cs=0x0000000000000023 ss=0x000000000000001B rip=0x0000000040000000 rsp=0x0000000070000FF8
vex:as:map_pkg index=0 base=0x0000000009B76000 size=33456128
vex:as:map_pkg index=1 base=0x000000000BB5D000 size=12288
vex:as:map_pkg index=2 base=0x000000000BB5F000 size=12288
vex:as:map_pkg index=3 base=0x000000000BB61000 size=12288
vex:as:map_pkg index=4 base=0x000000000BB63000 size=8192
vex:as:map_pkg index=5 base=0x000000000BB64000 size=16384
vex:as:map_pkg index=6 base=0x000000000BB67000 size=20480
vex:app:slot=0 verified=1 status=READY name=diagnostics-view entry=app/diagnostics-view caps=0x0000000000000015 surface=2 fence=3
vex:as:map_pkg index=0 base=0x0000000009B76000 size=33456128
vex:as:map_pkg index=1 base=0x000000000BB5D000 size=12288
vex:as:map_pkg index=2 base=0x000000000BB5F000 size=12288
vex:as:map_pkg index=3 base=0x000000000BB61000 size=12288
vex:as:map_pkg index=4 base=0x000000000BB63000 size=8192
vex:as:map_pkg index=5 base=0x000000000BB64000 size=16384
vex:as:map_pkg index=6 base=0x000000000BB67000 size=20480
vex:app:slot=1 verified=1 status=READY name=test-center entry=app/test-center caps=0x0000000000000017 surface=3 fence=4
vex:as:map_pkg index=0 base=0x0000000009B76000 size=33456128
vex:as:map_pkg index=1 base=0x000000000BB5D000 size=12288
vex:as:map_pkg index=2 base=0x000000000BB5F000 size=12288
vex:as:map_pkg index=3 base=0x000000000BB61000 size=12288
vex:as:map_pkg index=4 base=0x000000000BB63000 size=8192
vex:as:map_pkg index=5 base=0x000000000BB64000 size=16384
vex:as:map_pkg index=6 base=0x000000000BB67000 size=20480
vex:app:slot=2 verified=1 status=READY name=explorer-console entry=app/explorer-console caps=0x000000000000001F surface=4 fence=5
vex:as:map_pkg index=0 base=0x0000000009B76000 size=33456128
vex:as:map_pkg index=1 base=0x000000000BB5D000 size=12288
vex:as:map_pkg index=2 base=0x000000000BB5F000 size=12288
vex:as:map_pkg index=3 base=0x000000000BB61000 size=12288
vex:as:map_pkg index=4 base=0x000000000BB63000 size=8192
vex:as:map_pkg index=5 base=0x000000000BB64000 size=16384
vex:as:map_pkg index=6 base=0x000000000BB67000 size=20480
vex:app:slot=3 verified=1 status=READY name=terminal-console entry=app/terminal-console caps=0x000000000000001B surface=5 fence=6
vex:as:map_pkg index=0 base=0x0000000009B76000 size=33456128
vex:as:map_pkg index=1 base=0x000000000BB5D000 size=12288
vex:as:map_pkg index=2 base=0x000000000BB5F000 size=12288
vex:as:map_pkg index=3 base=0x000000000BB61000 size=12288
vex:as:map_pkg index=4 base=0x000000000BB63000 size=8192
vex:as:map_pkg index=5 base=0x000000000BB64000 size=16384
vex:as:map_pkg index=6 base=0x000000000BB67000 size=20480
vex:app:slot=4 verified=1 status=READY name=compositor-service entry=svc/compositor caps=0x0000000000000035 surface=6 fence=7
vex:as:map_pkg index=0 base=0x0000000009B76000 size=33456128
vex:as:map_pkg index=1 base=0x000000000BB5D000 size=12288
vex:as:map_pkg index=2 base=0x000000000BB5F000 size=12288
vex:as:map_pkg index=3 base=0x000000000BB61000 size=12288
vex:as:map_pkg index=4 base=0x000000000BB63000 size=8192
vex:as:map_pkg index=5 base=0x000000000BB64000 size=16384
vex:as:map_pkg index=6 base=0x000000000BB67000 size=20480
vex:app:slot=5 verified=1 status=READY name=gpu-service entry=svc/gpu caps=0x0000000000000035 surface=7 fence=8
services: 7
processes: 7

[status]
selftest: PASS
kernel: READY
vex:kernel:ready