cmake_minimum_required(VERSION 3.28)

project(VexOS LANGUAGES C CXX ASM_NASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

find_program(CLANG_BIN NAMES clang HINTS "C:/Program Files/LLVM/bin" REQUIRED)
find_program(CLANGXX_BIN NAMES clang++ HINTS "C:/Program Files/LLVM/bin" REQUIRED)
find_program(LLD_BIN NAMES ld.lld lld-link HINTS "C:/Program Files/LLVM/bin" REQUIRED)
find_program(LLD_LINK_BIN NAMES lld-link HINTS "C:/Program Files/LLVM/bin" REQUIRED)
find_program(LLVM_OBJCOPY_BIN NAMES llvm-objcopy HINTS "C:/Program Files/LLVM/bin" REQUIRED)
find_program(NASM_BIN NAMES nasm REQUIRED)
find_program(CARGO_BIN NAMES cargo REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(QEMU_BIN "V:/tools/qemu/app/qemu-system-x86_64.exe" CACHE FILEPATH "QEMU x86_64 binary")
set(OVMF_CODE "V:/tools/qemu/app/share/edk2-x86_64-code.fd" CACHE FILEPATH "OVMF code firmware")
set(OVMF_VARS "V:/tools/qemu/app/share/edk2-i386-vars.fd" CACHE FILEPATH "OVMF vars firmware")

set(BUILD_ROOT "${CMAKE_BINARY_DIR}/out")
set(EFI_ROOT "${BUILD_ROOT}/efi_root")
set(KERNEL_ELF "${BUILD_ROOT}/VEXKERN.ELF")
set(BOOT_EFI "${BUILD_ROOT}/BOOTX64.EFI")
set(INIT_VEX "${BUILD_ROOT}/INIT.VEX")
set(INIT_PAYLOAD_BIN "${CMAKE_BINARY_DIR}/apps/init/payload.bin")
set(INIT_WALLPAPER_BIN "${CMAKE_BINARY_DIR}/apps/init/wallpaper.bin")
set(INIT_WALLPAPER_OBJECT "${CMAKE_BINARY_DIR}/apps/init/wallpaper.o")
set(DIAG_VEX "${BUILD_ROOT}/DIAG.VEX")
set(DIAG_PAYLOAD_BIN "${CMAKE_BINARY_DIR}/apps/diagnostics/payload.bin")
set(TESTS_VEX "${BUILD_ROOT}/TESTS.VEX")
set(TESTS_PAYLOAD_BIN "${CMAKE_BINARY_DIR}/apps/testcenter/payload.bin")
set(SERVICES_VEX "${BUILD_ROOT}/SERVICES.VEX")
set(SERVICES_PAYLOAD_BIN "${CMAKE_BINARY_DIR}/apps/services/payload.bin")
set(TERMINAL_VEX "${BUILD_ROOT}/TERMINAL.VEX")
set(TERMINAL_PAYLOAD_BIN "${CMAKE_BINARY_DIR}/apps/terminal/payload.bin")
set(TERMINAL2_VEX "${BUILD_ROOT}/TERMINAL2.VEX")
set(TERMINAL2_PAYLOAD_BIN "${CMAKE_BINARY_DIR}/apps/terminal2/payload.bin")
set(COMPOSITOR_VEX "${BUILD_ROOT}/COMPOSITOR.VEX")
set(COMPOSITOR_PAYLOAD_BIN "${CMAKE_BINARY_DIR}/apps/compositor/payload.bin")
set(GPU_VEX "${BUILD_ROOT}/GPU.VEX")
set(GPU_PAYLOAD_BIN "${CMAKE_BINARY_DIR}/apps/gpu/payload.bin")
set(VARS_COPY "${BUILD_ROOT}/ovmf-vars.fd")
file(TO_NATIVE_PATH "${BOOT_EFI}" BOOT_EFI_NATIVE)

file(MAKE_DIRECTORY "${BUILD_ROOT}")

set(KERNEL_SOURCES
    ${CMAKE_SOURCE_DIR}/kernel/core/kernel.c
    ${CMAKE_SOURCE_DIR}/kernel/core/console.c
    ${CMAKE_SOURCE_DIR}/kernel/core/memory.c
    ${CMAKE_SOURCE_DIR}/kernel/core/address_space.c
    ${CMAKE_SOURCE_DIR}/kernel/core/acpi.c
    ${CMAKE_SOURCE_DIR}/kernel/core/scheduler.c
    ${CMAKE_SOURCE_DIR}/kernel/core/ipc.c
    ${CMAKE_SOURCE_DIR}/kernel/core/graphics.c
    ${CMAKE_SOURCE_DIR}/kernel/core/syscall.c
    ${CMAKE_SOURCE_DIR}/kernel/core/manifest.c
    ${CMAKE_SOURCE_DIR}/kernel/core/process.c
    ${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/execution.c
    ${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/interrupts.c
    ${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/serial.c
    ${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/framebuffer.c
    ${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/timer.c
)

set(KERNEL_OBJECTS "")
foreach(source_file IN LISTS KERNEL_SOURCES)
    get_filename_component(source_name "${source_file}" NAME_WE)
    set(object_file "${CMAKE_BINARY_DIR}/kernel/${source_name}.o")
    add_custom_command(
        OUTPUT "${object_file}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/kernel"
        COMMAND "${CLANG_BIN}"
                --target=x86_64-unknown-none-elf
                -ffreestanding
                -fno-stack-protector
                -fno-pic
                -fno-omit-frame-pointer
                -m64
                -mno-red-zone
                -mcmodel=kernel
                -Wall
                -Wextra
                -Werror
                -I"${CMAKE_SOURCE_DIR}/kernel/include"
                -c "${source_file}"
                -o "${object_file}"
        DEPENDS "${source_file}"
        VERBATIM
    )
    list(APPEND KERNEL_OBJECTS "${object_file}")
endforeach()

set(KERNEL_ENTRY_OBJECT "${CMAKE_BINARY_DIR}/kernel/entry.o")
add_custom_command(
    OUTPUT "${KERNEL_ENTRY_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/kernel"
    COMMAND "${NASM_BIN}" -f elf64 "${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/entry.asm" -o "${KERNEL_ENTRY_OBJECT}"
    DEPENDS "${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/entry.asm"
    VERBATIM
)

set(KERNEL_INTERRUPTS_OBJECT "${CMAKE_BINARY_DIR}/kernel/interrupts_asm.o")
add_custom_command(
    OUTPUT "${KERNEL_INTERRUPTS_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/kernel"
    COMMAND "${NASM_BIN}" -f elf64 "${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/interrupts.asm" -o "${KERNEL_INTERRUPTS_OBJECT}"
    DEPENDS "${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/interrupts.asm"
    VERBATIM
)

set(KERNEL_SYSCALL_ENTRY_OBJECT "${CMAKE_BINARY_DIR}/kernel/syscall_entry.o")
add_custom_command(
    OUTPUT "${KERNEL_SYSCALL_ENTRY_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/kernel"
    COMMAND "${NASM_BIN}" -f elf64 "${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/syscall_entry.asm" -o "${KERNEL_SYSCALL_ENTRY_OBJECT}"
    DEPENDS "${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/syscall_entry.asm"
    VERBATIM
)

add_custom_command(
    OUTPUT "${KERNEL_ELF}"
    COMMAND "${LLD_BIN}"
            -T "${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/linker.ld"
            -nostdlib
            -static
            -o "${KERNEL_ELF}"
            "${KERNEL_ENTRY_OBJECT}"
            "${KERNEL_INTERRUPTS_OBJECT}"
            "${KERNEL_SYSCALL_ENTRY_OBJECT}"
            ${KERNEL_OBJECTS}
    DEPENDS ${KERNEL_OBJECTS} "${KERNEL_ENTRY_OBJECT}" "${KERNEL_INTERRUPTS_OBJECT}" "${KERNEL_SYSCALL_ENTRY_OBJECT}" "${CMAKE_SOURCE_DIR}/kernel/arch/x86_64/linker.ld"
    VERBATIM
)
add_custom_target(kernel ALL DEPENDS "${KERNEL_ELF}")

set(BOOT_KERNEL_BLOB_SOURCE "${CMAKE_BINARY_DIR}/boot/kernel_blob.c")
add_custom_command(
    OUTPUT "${BOOT_KERNEL_BLOB_SOURCE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/boot"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
            --input "${KERNEL_ELF}"
            --output "${BOOT_KERNEL_BLOB_SOURCE}"
            --symbol vex_kernel_blob
    DEPENDS "${KERNEL_ELF}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
    VERBATIM
)

set(BOOT_SOURCES
    ${CMAKE_SOURCE_DIR}/boot/loader/src/common.c
    ${CMAKE_SOURCE_DIR}/boot/loader/src/sha256.c
    ${CMAKE_SOURCE_DIR}/boot/loader/src/elf.c
    ${CMAKE_SOURCE_DIR}/boot/loader/src/package.c
    ${CMAKE_SOURCE_DIR}/boot/loader/src/main.c
    ${CMAKE_SOURCE_DIR}/boot/loader/src/tweetnacl.c
)

set(BOOT_OBJECTS "")
foreach(source_file IN LISTS BOOT_SOURCES)
    get_filename_component(source_name "${source_file}" NAME_WE)
    set(object_file "${CMAKE_BINARY_DIR}/boot/${source_name}.obj")
    add_custom_command(
        OUTPUT "${object_file}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/boot"
        COMMAND "${CLANG_BIN}"
                --target=x86_64-pc-windows-msvc
                -ffreestanding
                -fshort-wchar
                -fno-stack-protector
                -fno-builtin
                -fno-pic
                -m64
                -mno-red-zone
                -Wall
                -Wextra
                -Werror
                -I"${CMAKE_SOURCE_DIR}/boot/loader/include"
                -I"${CMAKE_SOURCE_DIR}/kernel/include"
                -c "${source_file}"
                -o "${object_file}"
        DEPENDS "${source_file}"
        VERBATIM
    )
    list(APPEND BOOT_OBJECTS "${object_file}")
endforeach()

set(BOOT_BLOB_SOURCES
    "${BOOT_KERNEL_BLOB_SOURCE}"
)
set(BOOT_INIT_BLOB_SOURCE "${CMAKE_BINARY_DIR}/boot/init_blob.c")

add_custom_command(
    OUTPUT "${BOOT_INIT_BLOB_SOURCE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/boot"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
            --input "${INIT_VEX}"
            --output "${BOOT_INIT_BLOB_SOURCE}"
            --symbol vex_init_blob
    DEPENDS "${INIT_VEX}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
    VERBATIM
)
list(APPEND BOOT_BLOB_SOURCES "${BOOT_INIT_BLOB_SOURCE}")

set(BOOT_DIAG_BLOB_SOURCE "${CMAKE_BINARY_DIR}/boot/diag_blob.c")
add_custom_command(
    OUTPUT "${BOOT_DIAG_BLOB_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
            --input "${DIAG_VEX}"
            --output "${BOOT_DIAG_BLOB_SOURCE}"
            --symbol vex_diag_blob
    DEPENDS "${DIAG_VEX}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
    VERBATIM
)
list(APPEND BOOT_BLOB_SOURCES "${BOOT_DIAG_BLOB_SOURCE}")

set(BOOT_TESTS_BLOB_SOURCE "${CMAKE_BINARY_DIR}/boot/tests_blob.c")
add_custom_command(
    OUTPUT "${BOOT_TESTS_BLOB_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
            --input "${TESTS_VEX}"
            --output "${BOOT_TESTS_BLOB_SOURCE}"
            --symbol vex_tests_blob
    DEPENDS "${TESTS_VEX}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
    VERBATIM
)
list(APPEND BOOT_BLOB_SOURCES "${BOOT_TESTS_BLOB_SOURCE}")

set(BOOT_SERVICES_BLOB_SOURCE "${CMAKE_BINARY_DIR}/boot/services_blob.c")
add_custom_command(
    OUTPUT "${BOOT_SERVICES_BLOB_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
            --input "${SERVICES_VEX}"
            --output "${BOOT_SERVICES_BLOB_SOURCE}"
            --symbol vex_services_blob
    DEPENDS "${SERVICES_VEX}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
    VERBATIM
)
list(APPEND BOOT_BLOB_SOURCES "${BOOT_SERVICES_BLOB_SOURCE}")

set(BOOT_TERMINAL_BLOB_SOURCE "${CMAKE_BINARY_DIR}/boot/terminal_blob.c")
add_custom_command(
    OUTPUT "${BOOT_TERMINAL_BLOB_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
            --input "${TERMINAL_VEX}"
            --output "${BOOT_TERMINAL_BLOB_SOURCE}"
            --symbol vex_terminal_blob
    DEPENDS "${TERMINAL_VEX}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
    VERBATIM
)
list(APPEND BOOT_BLOB_SOURCES "${BOOT_TERMINAL_BLOB_SOURCE}")

set(BOOT_TERMINAL2_BLOB_SOURCE "${CMAKE_BINARY_DIR}/boot/terminal2_blob.c")
add_custom_command(
    OUTPUT "${BOOT_TERMINAL2_BLOB_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
            --input "${TERMINAL2_VEX}"
            --output "${BOOT_TERMINAL2_BLOB_SOURCE}"
            --symbol vex_terminal2_blob
    DEPENDS "${TERMINAL2_VEX}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
    VERBATIM
)
list(APPEND BOOT_BLOB_SOURCES "${BOOT_TERMINAL2_BLOB_SOURCE}")

set(BOOT_COMPOSITOR_BLOB_SOURCE "${CMAKE_BINARY_DIR}/boot/compositor_blob.c")
add_custom_command(
    OUTPUT "${BOOT_COMPOSITOR_BLOB_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
            --input "${COMPOSITOR_VEX}"
            --output "${BOOT_COMPOSITOR_BLOB_SOURCE}"
            --symbol vex_compositor_blob
    DEPENDS "${COMPOSITOR_VEX}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
    VERBATIM
)
list(APPEND BOOT_BLOB_SOURCES "${BOOT_COMPOSITOR_BLOB_SOURCE}")

set(BOOT_GPU_BLOB_SOURCE "${CMAKE_BINARY_DIR}/boot/gpu_blob.c")
add_custom_command(
    OUTPUT "${BOOT_GPU_BLOB_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
            --input "${GPU_VEX}"
            --output "${BOOT_GPU_BLOB_SOURCE}"
            --symbol vex_gpu_blob
    DEPENDS "${GPU_VEX}" "${CMAKE_SOURCE_DIR}/tools/generate_blob_source.py"
    VERBATIM
)
list(APPEND BOOT_BLOB_SOURCES "${BOOT_GPU_BLOB_SOURCE}")

foreach(source_file IN LISTS BOOT_BLOB_SOURCES)
    get_filename_component(source_name "${source_file}" NAME_WE)
    set(object_file "${CMAKE_BINARY_DIR}/boot/${source_name}.obj")
    add_custom_command(
        OUTPUT "${object_file}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/boot"
        COMMAND "${CLANG_BIN}"
                --target=x86_64-pc-windows-msvc
                -ffreestanding
                -fshort-wchar
                -fno-stack-protector
                -fno-builtin
                -fno-pic
                -m64
                -mno-red-zone
                -Wall
                -Wextra
                -Werror
                -c "${source_file}"
                -o "${object_file}"
        DEPENDS "${source_file}"
        VERBATIM
    )
    list(APPEND BOOT_OBJECTS "${object_file}")
endforeach()

set(INIT_OBJECT "${CMAKE_BINARY_DIR}/apps/init/main.o")
set(INIT_ANIMATION_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_animation.o")
set(INIT_APP_RUNTIME_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_app_runtime.o")
set(INIT_CATALOG_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_catalog.o")
set(INIT_DOMAIN_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_domain.o")
set(INIT_INPUT_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_input.o")
set(INIT_LAYOUT_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_layout.o")
set(INIT_LAUNCHER_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_launcher.o")
set(INIT_POINTER_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_pointer.o")
set(INIT_PS2_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_ps2.o")
set(INIT_REGISTRY_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_registry.o")
set(INIT_RTC_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_rtc.o")
set(INIT_RUNTIME_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_runtime.o")
set(INIT_SESSION_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_session.o")
set(INIT_SHELL_UI_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_shell_ui.o")
set(INIT_STAGE_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_stage.o")
set(INIT_VIEWS_OBJECT "${CMAKE_BINARY_DIR}/apps/init/desktop_views.o")
set(INIT_ELF "${CMAKE_BINARY_DIR}/apps/init/INITAPP.ELF")
add_custom_command(
    OUTPUT "${INIT_WALLPAPER_BIN}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_wallpaper_asset.py"
            --input "${CMAKE_SOURCE_DIR}/imgdesk/back.jpg"
            --output "${INIT_WALLPAPER_BIN}"
            --width 1280
            --height 800
    DEPENDS "${CMAKE_SOURCE_DIR}/imgdesk/back.jpg" "${CMAKE_SOURCE_DIR}/tools/generate_wallpaper_asset.py"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_WALLPAPER_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CMAKE_COMMAND}" -E chdir "${CMAKE_BINARY_DIR}"
            "${LLVM_OBJCOPY_BIN}"
            -I binary
            -O elf64-x86-64
            -B i386:x86-64
            "apps/init/wallpaper.bin"
            "apps/init/wallpaper.o"
    DEPENDS "${INIT_WALLPAPER_BIN}"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_APP_RUNTIME_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_app_runtime.c"
            -o "${INIT_APP_RUNTIME_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_app_runtime.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_app_runtime.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_domain.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_views.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_CATALOG_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_catalog.c"
            -o "${INIT_CATALOG_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_catalog.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_catalog.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_domain.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_registry.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_ANIMATION_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_animation.c"
            -o "${INIT_ANIMATION_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_animation.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_animation.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_DOMAIN_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_domain.c"
            -o "${INIT_DOMAIN_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_domain.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_domain.h"
        "${CMAKE_SOURCE_DIR}/apps/init/vex_boot_info.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_REGISTRY_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_registry.c"
            -o "${INIT_REGISTRY_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_registry.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_registry.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_catalog.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_domain.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_INPUT_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_input.c"
            -o "${INIT_INPUT_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_input.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_input.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_session.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_launcher.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_LAUNCHER_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_launcher.c"
            -o "${INIT_LAUNCHER_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_launcher.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_launcher.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_catalog.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_LAYOUT_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_layout.c"
            -o "${INIT_LAYOUT_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_layout.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_layout.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_session.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_POINTER_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_pointer.c"
            -o "${INIT_POINTER_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_pointer.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_pointer.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_session.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_launcher.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_PS2_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_ps2.c"
            -o "${INIT_PS2_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_ps2.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_ps2.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_input.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_RTC_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_rtc.c"
            -o "${INIT_RTC_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_rtc.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_rtc.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_ps2.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_RUNTIME_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_runtime.c"
            -o "${INIT_RUNTIME_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_runtime.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_runtime.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_session.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_SHELL_UI_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_shell_ui.c"
            -o "${INIT_SHELL_UI_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_shell_ui.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_shell_ui.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_domain.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_layout.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_rtc.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_STAGE_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_stage.c"
            -o "${INIT_STAGE_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_stage.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_stage.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_layout.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_VIEWS_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_views.c"
            -o "${INIT_VIEWS_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_views.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_app_runtime.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_views.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_domain.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_layout.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_rtc.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_SESSION_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/desktop_session.c"
            -o "${INIT_SESSION_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_session.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_session.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_catalog.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/init"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/init/main.c"
            -o "${INIT_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/init/main.c"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_animation.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_app_runtime.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_catalog.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_domain.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_input.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_layout.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_launcher.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_pointer.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_ps2.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_registry.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_rtc.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_runtime.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_shell_ui.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_session.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_stage.h"
        "${CMAKE_SOURCE_DIR}/apps/init/desktop_views.h"
        "${CMAKE_SOURCE_DIR}/apps/init/vex_boot_info.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_PAYLOAD_BIN}"
    COMMAND "${LLD_BIN}"
            -T "${CMAKE_SOURCE_DIR}/apps/init/linker.ld"
            -nostdlib
            -static
            -o "${INIT_ELF}"
            "${INIT_OBJECT}"
            "${INIT_ANIMATION_OBJECT}"
            "${INIT_APP_RUNTIME_OBJECT}"
            "${INIT_CATALOG_OBJECT}"
            "${INIT_DOMAIN_OBJECT}"
            "${INIT_INPUT_OBJECT}"
            "${INIT_LAYOUT_OBJECT}"
            "${INIT_LAUNCHER_OBJECT}"
            "${INIT_POINTER_OBJECT}"
            "${INIT_PS2_OBJECT}"
            "${INIT_REGISTRY_OBJECT}"
            "${INIT_RTC_OBJECT}"
            "${INIT_RUNTIME_OBJECT}"
            "${INIT_SHELL_UI_OBJECT}"
            "${INIT_SESSION_OBJECT}"
            "${INIT_STAGE_OBJECT}"
            "${INIT_VIEWS_OBJECT}"
            "${INIT_WALLPAPER_OBJECT}"
    COMMAND "${LLVM_OBJCOPY_BIN}" -O binary "${INIT_ELF}" "${INIT_PAYLOAD_BIN}"
    DEPENDS "${INIT_OBJECT}" "${INIT_ANIMATION_OBJECT}" "${INIT_APP_RUNTIME_OBJECT}" "${INIT_CATALOG_OBJECT}" "${INIT_DOMAIN_OBJECT}" "${INIT_INPUT_OBJECT}" "${INIT_LAYOUT_OBJECT}" "${INIT_LAUNCHER_OBJECT}" "${INIT_POINTER_OBJECT}" "${INIT_PS2_OBJECT}" "${INIT_REGISTRY_OBJECT}" "${INIT_RTC_OBJECT}" "${INIT_RUNTIME_OBJECT}" "${INIT_SHELL_UI_OBJECT}" "${INIT_SESSION_OBJECT}" "${INIT_STAGE_OBJECT}" "${INIT_VIEWS_OBJECT}" "${INIT_WALLPAPER_OBJECT}" "${CMAKE_SOURCE_DIR}/apps/init/linker.ld"
    VERBATIM
)

add_custom_command(
    OUTPUT "${INIT_VEX}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${BUILD_ROOT}"
    COMMAND "${CMAKE_SOURCE_DIR}/tools/vex/target/release/vex.exe"
            package
            --manifest "${CMAKE_SOURCE_DIR}/apps/init/manifest.json"
            --binary "${INIT_PAYLOAD_BIN}"
            --out "${INIT_VEX}"
            --signing-key "${CMAKE_SOURCE_DIR}/apps/init/signing.key"
    DEPENDS
        vex-tool
        "${CMAKE_SOURCE_DIR}/apps/init/manifest.json"
        "${INIT_PAYLOAD_BIN}"
        "${CMAKE_SOURCE_DIR}/apps/init/signing.key"
    VERBATIM
)
add_custom_target(init-package ALL DEPENDS "${INIT_VEX}")

set(DIAG_OBJECT "${CMAKE_BINARY_DIR}/apps/diagnostics/main.o")
set(DIAG_ELF "${CMAKE_BINARY_DIR}/apps/diagnostics/DIAGAPP.ELF")
add_custom_command(
    OUTPUT "${DIAG_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/diagnostics"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/diagnostics/main.c"
            -o "${DIAG_OBJECT}"
    DEPENDS "${CMAKE_SOURCE_DIR}/apps/diagnostics/main.c"
    VERBATIM
)

add_custom_command(
    OUTPUT "${DIAG_PAYLOAD_BIN}"
    COMMAND "${LLD_BIN}"
            -T "${CMAKE_SOURCE_DIR}/apps/diagnostics/linker.ld"
            -nostdlib
            -static
            -o "${DIAG_ELF}"
            "${DIAG_OBJECT}"
    COMMAND "${LLVM_OBJCOPY_BIN}" -O binary "${DIAG_ELF}" "${DIAG_PAYLOAD_BIN}"
    DEPENDS "${DIAG_OBJECT}" "${CMAKE_SOURCE_DIR}/apps/diagnostics/linker.ld"
    VERBATIM
)

add_custom_command(
    OUTPUT "${DIAG_VEX}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${BUILD_ROOT}"
    COMMAND "${CMAKE_SOURCE_DIR}/tools/vex/target/release/vex.exe"
            package
            --manifest "${CMAKE_SOURCE_DIR}/apps/diagnostics/manifest.json"
            --binary "${DIAG_PAYLOAD_BIN}"
            --out "${DIAG_VEX}"
            --signing-key "${CMAKE_SOURCE_DIR}/apps/diagnostics/signing.key"
    DEPENDS
        vex-tool
        "${CMAKE_SOURCE_DIR}/apps/diagnostics/manifest.json"
        "${DIAG_PAYLOAD_BIN}"
        "${CMAKE_SOURCE_DIR}/apps/diagnostics/signing.key"
    VERBATIM
)
add_custom_target(diagnostics-package ALL DEPENDS "${DIAG_VEX}")

set(TESTS_OBJECT "${CMAKE_BINARY_DIR}/apps/testcenter/main.o")
set(TESTS_ELF "${CMAKE_BINARY_DIR}/apps/testcenter/TESTCENTER.ELF")
add_custom_command(
    OUTPUT "${TESTS_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/testcenter"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/testcenter/main.c"
            -o "${TESTS_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/testcenter/main.c"
        "${CMAKE_SOURCE_DIR}/apps/init/vex_boot_info.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${TESTS_PAYLOAD_BIN}"
    COMMAND "${LLD_BIN}"
            -T "${CMAKE_SOURCE_DIR}/apps/testcenter/linker.ld"
            -nostdlib
            -static
            -o "${TESTS_ELF}"
            "${TESTS_OBJECT}"
    COMMAND "${LLVM_OBJCOPY_BIN}" -O binary "${TESTS_ELF}" "${TESTS_PAYLOAD_BIN}"
    DEPENDS "${TESTS_OBJECT}" "${CMAKE_SOURCE_DIR}/apps/testcenter/linker.ld"
    VERBATIM
)

add_custom_command(
    OUTPUT "${TESTS_VEX}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${BUILD_ROOT}"
    COMMAND "${CMAKE_SOURCE_DIR}/tools/vex/target/release/vex.exe"
            package
            --manifest "${CMAKE_SOURCE_DIR}/apps/testcenter/manifest.json"
            --binary "${TESTS_PAYLOAD_BIN}"
            --out "${TESTS_VEX}"
            --signing-key "${CMAKE_SOURCE_DIR}/apps/testcenter/signing.key"
    DEPENDS
        vex-tool
        "${CMAKE_SOURCE_DIR}/apps/testcenter/manifest.json"
        "${TESTS_PAYLOAD_BIN}"
        "${CMAKE_SOURCE_DIR}/apps/testcenter/signing.key"
    VERBATIM
)
add_custom_target(testcenter-package ALL DEPENDS "${TESTS_VEX}")

set(SERVICES_OBJECT "${CMAKE_BINARY_DIR}/apps/services/main.o")
set(SERVICES_ELF "${CMAKE_BINARY_DIR}/apps/services/SERVICESAPP.ELF")
add_custom_command(
    OUTPUT "${SERVICES_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/services"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/services/main.c"
            -o "${SERVICES_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/services/main.c"
        "${CMAKE_SOURCE_DIR}/apps/init/vex_boot_info.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${SERVICES_PAYLOAD_BIN}"
    COMMAND "${LLD_BIN}"
            -T "${CMAKE_SOURCE_DIR}/apps/services/linker.ld"
            -nostdlib
            -static
            -o "${SERVICES_ELF}"
            "${SERVICES_OBJECT}"
    COMMAND "${LLVM_OBJCOPY_BIN}" -O binary "${SERVICES_ELF}" "${SERVICES_PAYLOAD_BIN}"
    DEPENDS "${SERVICES_OBJECT}" "${CMAKE_SOURCE_DIR}/apps/services/linker.ld"
    VERBATIM
)

add_custom_command(
    OUTPUT "${SERVICES_VEX}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${BUILD_ROOT}"
    COMMAND "${CMAKE_SOURCE_DIR}/tools/vex/target/release/vex.exe"
            package
            --manifest "${CMAKE_SOURCE_DIR}/apps/services/manifest.json"
            --binary "${SERVICES_PAYLOAD_BIN}"
            --out "${SERVICES_VEX}"
            --signing-key "${CMAKE_SOURCE_DIR}/apps/services/signing.key"
    DEPENDS
        vex-tool
        "${CMAKE_SOURCE_DIR}/apps/services/manifest.json"
        "${SERVICES_PAYLOAD_BIN}"
        "${CMAKE_SOURCE_DIR}/apps/services/signing.key"
    VERBATIM
)
add_custom_target(services-package ALL DEPENDS "${SERVICES_VEX}")

set(TERMINAL_OBJECT "${CMAKE_BINARY_DIR}/apps/terminal/main.o")
set(TERMINAL_ELF "${CMAKE_BINARY_DIR}/apps/terminal/TERMINALAPP.ELF")
add_custom_command(
    OUTPUT "${TERMINAL_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/terminal"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/terminal/main.c"
            -o "${TERMINAL_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/terminal/main.c"
        "${CMAKE_SOURCE_DIR}/apps/init/vex_boot_info.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${TERMINAL_PAYLOAD_BIN}"
    COMMAND "${LLD_BIN}"
            -T "${CMAKE_SOURCE_DIR}/apps/terminal/linker.ld"
            -nostdlib
            -static
            -o "${TERMINAL_ELF}"
            "${TERMINAL_OBJECT}"
    COMMAND "${LLVM_OBJCOPY_BIN}" -O binary "${TERMINAL_ELF}" "${TERMINAL_PAYLOAD_BIN}"
    DEPENDS "${TERMINAL_OBJECT}" "${CMAKE_SOURCE_DIR}/apps/terminal/linker.ld"
    VERBATIM
)

add_custom_command(
    OUTPUT "${TERMINAL_VEX}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${BUILD_ROOT}"
    COMMAND "${CMAKE_SOURCE_DIR}/tools/vex/target/release/vex.exe"
            package
            --manifest "${CMAKE_SOURCE_DIR}/apps/terminal/manifest.json"
            --binary "${TERMINAL_PAYLOAD_BIN}"
            --out "${TERMINAL_VEX}"
            --signing-key "${CMAKE_SOURCE_DIR}/apps/terminal/signing.key"
    DEPENDS
        vex-tool
        "${CMAKE_SOURCE_DIR}/apps/terminal/manifest.json"
        "${TERMINAL_PAYLOAD_BIN}"
        "${CMAKE_SOURCE_DIR}/apps/terminal/signing.key"
    VERBATIM
)
add_custom_target(terminal-package ALL DEPENDS "${TERMINAL_VEX}")

set(TERMINAL2_OBJECT "${CMAKE_BINARY_DIR}/apps/terminal2/main.o")
set(TERMINAL2_ELF "${CMAKE_BINARY_DIR}/apps/terminal2/TERMINAL2APP.ELF")
add_custom_command(
    OUTPUT "${TERMINAL2_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/terminal2"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/terminal2/main.c"
            -o "${TERMINAL2_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/terminal2/main.c"
        "${CMAKE_SOURCE_DIR}/apps/init/vex_boot_info.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${TERMINAL2_PAYLOAD_BIN}"
    COMMAND "${LLD_BIN}"
            -T "${CMAKE_SOURCE_DIR}/apps/terminal2/linker.ld"
            -nostdlib
            -static
            -o "${TERMINAL2_ELF}"
            "${TERMINAL2_OBJECT}"
    COMMAND "${LLVM_OBJCOPY_BIN}" -O binary "${TERMINAL2_ELF}" "${TERMINAL2_PAYLOAD_BIN}"
    DEPENDS "${TERMINAL2_OBJECT}" "${CMAKE_SOURCE_DIR}/apps/terminal2/linker.ld"
    VERBATIM
)

add_custom_command(
    OUTPUT "${TERMINAL2_VEX}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${BUILD_ROOT}"
    COMMAND "${CMAKE_SOURCE_DIR}/tools/vex/target/release/vex.exe"
            package
            --manifest "${CMAKE_SOURCE_DIR}/apps/terminal2/manifest.json"
            --binary "${TERMINAL2_PAYLOAD_BIN}"
            --out "${TERMINAL2_VEX}"
            --signing-key "${CMAKE_SOURCE_DIR}/apps/terminal2/signing.key"
    DEPENDS
        vex-tool
        "${CMAKE_SOURCE_DIR}/apps/terminal2/manifest.json"
        "${TERMINAL2_PAYLOAD_BIN}"
        "${CMAKE_SOURCE_DIR}/apps/terminal2/signing.key"
    VERBATIM
)
add_custom_target(terminal2-package ALL DEPENDS "${TERMINAL2_VEX}")

set(COMPOSITOR_OBJECT "${CMAKE_BINARY_DIR}/apps/compositor/main.o")
set(COMPOSITOR_ELF "${CMAKE_BINARY_DIR}/apps/compositor/COMPOSITORAPP.ELF")
add_custom_command(
    OUTPUT "${COMPOSITOR_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/compositor"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/compositor/main.c"
            -o "${COMPOSITOR_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/compositor/main.c"
        "${CMAKE_SOURCE_DIR}/apps/init/vex_boot_info.h"
        "${CMAKE_SOURCE_DIR}/apps/init/vex_ui_proto.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${COMPOSITOR_PAYLOAD_BIN}"
    COMMAND "${LLD_BIN}"
            -T "${CMAKE_SOURCE_DIR}/apps/compositor/linker.ld"
            -nostdlib
            -static
            -o "${COMPOSITOR_ELF}"
            "${COMPOSITOR_OBJECT}"
    COMMAND "${LLVM_OBJCOPY_BIN}" -O binary "${COMPOSITOR_ELF}" "${COMPOSITOR_PAYLOAD_BIN}"
    DEPENDS "${COMPOSITOR_OBJECT}" "${CMAKE_SOURCE_DIR}/apps/compositor/linker.ld"
    VERBATIM
)

add_custom_command(
    OUTPUT "${COMPOSITOR_VEX}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${BUILD_ROOT}"
    COMMAND "${CMAKE_SOURCE_DIR}/tools/vex/target/release/vex.exe"
            package
            --manifest "${CMAKE_SOURCE_DIR}/apps/compositor/manifest.json"
            --binary "${COMPOSITOR_PAYLOAD_BIN}"
            --out "${COMPOSITOR_VEX}"
            --signing-key "${CMAKE_SOURCE_DIR}/apps/compositor/signing.key"
    DEPENDS
        vex-tool
        "${CMAKE_SOURCE_DIR}/apps/compositor/manifest.json"
        "${COMPOSITOR_PAYLOAD_BIN}"
        "${CMAKE_SOURCE_DIR}/apps/compositor/signing.key"
    VERBATIM
)
add_custom_target(compositor-package ALL DEPENDS "${COMPOSITOR_VEX}")

set(GPU_OBJECT "${CMAKE_BINARY_DIR}/apps/gpu/main.o")
set(GPU_ELF "${CMAKE_BINARY_DIR}/apps/gpu/GPUAPP.ELF")
add_custom_command(
    OUTPUT "${GPU_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apps/gpu"
    COMMAND "${CLANG_BIN}"
            --target=x86_64-unknown-none-elf
            -ffreestanding
            -fno-stack-protector
            -fno-builtin
            -fno-pic
            -m64
            -mno-red-zone
            -O2
            -Wall
            -Wextra
            -Werror
            -c "${CMAKE_SOURCE_DIR}/apps/gpu/main.c"
            -o "${GPU_OBJECT}"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/apps/gpu/main.c"
        "${CMAKE_SOURCE_DIR}/apps/init/vex_boot_info.h"
        "${CMAKE_SOURCE_DIR}/apps/init/vex_ui_proto.h"
    VERBATIM
)

add_custom_command(
    OUTPUT "${GPU_PAYLOAD_BIN}"
    COMMAND "${LLD_BIN}"
            -T "${CMAKE_SOURCE_DIR}/apps/gpu/linker.ld"
            -nostdlib
            -static
            -o "${GPU_ELF}"
            "${GPU_OBJECT}"
    COMMAND "${LLVM_OBJCOPY_BIN}" -O binary "${GPU_ELF}" "${GPU_PAYLOAD_BIN}"
    DEPENDS "${GPU_OBJECT}" "${CMAKE_SOURCE_DIR}/apps/gpu/linker.ld"
    VERBATIM
)

add_custom_command(
    OUTPUT "${GPU_VEX}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${BUILD_ROOT}"
    COMMAND "${CMAKE_SOURCE_DIR}/tools/vex/target/release/vex.exe"
            package
            --manifest "${CMAKE_SOURCE_DIR}/apps/gpu/manifest.json"
            --binary "${GPU_PAYLOAD_BIN}"
            --out "${GPU_VEX}"
            --signing-key "${CMAKE_SOURCE_DIR}/apps/gpu/signing.key"
    DEPENDS
        vex-tool
        "${CMAKE_SOURCE_DIR}/apps/gpu/manifest.json"
        "${GPU_PAYLOAD_BIN}"
        "${CMAKE_SOURCE_DIR}/apps/gpu/signing.key"
    VERBATIM
)
add_custom_target(gpu-package ALL DEPENDS "${GPU_VEX}")

add_custom_command(
    OUTPUT "${BOOT_EFI}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${BUILD_ROOT}"
    COMMAND "${LLD_LINK_BIN}"
            /subsystem:efi_application
            /nodefaultlib
            /entry:efi_main
            /machine:x64
            /base:0x100000
            /fixed:no
            /opt:ref
            /opt:icf
            /Brepro
            /out:${BOOT_EFI_NATIVE}
            ${BOOT_OBJECTS}
    DEPENDS ${BOOT_OBJECTS}
    VERBATIM
)
add_custom_target(bootloader ALL DEPENDS "${BOOT_EFI}")

add_custom_target(efi-root ALL
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${EFI_ROOT}/EFI/BOOT"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BOOT_EFI}" "${EFI_ROOT}/EFI/BOOT/BOOTX64.EFI"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${KERNEL_ELF}" "${EFI_ROOT}/EFI/BOOT/VEXKERN.ELF"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${INIT_VEX}" "${EFI_ROOT}/EFI/BOOT/INIT.VEX"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DIAG_VEX}" "${EFI_ROOT}/EFI/BOOT/DIAG.VEX"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${TESTS_VEX}" "${EFI_ROOT}/EFI/BOOT/TESTS.VEX"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SERVICES_VEX}" "${EFI_ROOT}/EFI/BOOT/SERVICES.VEX"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${TERMINAL_VEX}" "${EFI_ROOT}/EFI/BOOT/TERMINAL.VEX"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${TERMINAL2_VEX}" "${EFI_ROOT}/EFI/BOOT/TERMINAL2.VEX"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${COMPOSITOR_VEX}" "${EFI_ROOT}/EFI/BOOT/COMPOSITOR.VEX"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${GPU_VEX}" "${EFI_ROOT}/EFI/BOOT/GPU.VEX"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${OVMF_VARS}" "${VARS_COPY}"
    DEPENDS bootloader kernel init-package diagnostics-package testcenter-package services-package terminal-package terminal2-package compositor-package gpu-package
    VERBATIM
)

add_custom_target(vex-tool ALL
    COMMAND "${CARGO_BIN}" build --manifest-path "${CMAKE_SOURCE_DIR}/tools/vex/Cargo.toml" --release
    DEPENDS
        "${CMAKE_SOURCE_DIR}/tools/vex/Cargo.toml"
        "${CMAKE_SOURCE_DIR}/tools/vex/src/main.rs"
    VERBATIM
)

add_test(
    NAME boot-smoke
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/smoke_boot.py"
            --qemu "${QEMU_BIN}"
            --ovmf-code "${OVMF_CODE}"
            --ovmf-vars "${VARS_COPY}"
            --efi-root "${EFI_ROOT}"
)
set_tests_properties(boot-smoke PROPERTIES DEPENDS efi-root TIMEOUT 45)

add_test(
    NAME vex-package-tooling
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/package_tooling.py"
            --vex "${CMAKE_SOURCE_DIR}/tools/vex/target/release/vex.exe"
            --manifest "${CMAKE_SOURCE_DIR}/tests/fixtures/demo-app/manifest.json"
            --binary "${CMAKE_SOURCE_DIR}/tests/fixtures/demo-app/payload.bin"
)
set_tests_properties(vex-package-tooling PROPERTIES DEPENDS vex-tool TIMEOUT 20)

add_custom_target(run-qemu
    COMMAND "${QEMU_BIN}"
            -machine q35
            -m 256M
            -cpu qemu64
            -vga std
            -display none
            -serial stdio
            -monitor none
            -drive if=pflash,format=raw,readonly=on,file="${OVMF_CODE}"
            -drive if=pflash,format=raw,file="${VARS_COPY}"
            -drive format=raw,file=fat:rw:"${EFI_ROOT}"
    DEPENDS efi-root
    USES_TERMINAL
    VERBATIM
)
