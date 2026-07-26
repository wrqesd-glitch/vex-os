# Diff Details

Date : 2026-07-09 03:31:23

Directory v:\\vex_os

Total : 79 files,  -20905 codes, 22 comments, -91 blanks, all -20974 lines

[Summary](results.md) / [Details](details.md) / [Diff Summary](diff.md) / Diff Details

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [apps/compositor/main.c](/apps/compositor/main.c) | C | 1 | 0 | 0 | 1 |
| [apps/gpu/main.c](/apps/gpu/main.c) | C | 33 | 0 | 2 | 35 |
| [apps/init/desktop\_app\_runtime.c](/apps/init/desktop_app_runtime.c) | C | 332 | 8 | 14 | 354 |
| [apps/init/desktop\_app\_runtime.h](/apps/init/desktop_app_runtime.h) | C++ | 21 | 0 | 3 | 24 |
| [apps/init/desktop\_catalog.h](/apps/init/desktop_catalog.h) | C++ | 0 | 7 | 1 | 8 |
| [apps/init/desktop\_registry.c](/apps/init/desktop_registry.c) | C | 1 | 0 | -1 | 0 |
| [apps/init/desktop\_views.c](/apps/init/desktop_views.c) | C | 152 | 0 | 11 | 163 |
| [apps/init/vex\_gpu\_proto.h](/apps/init/vex_gpu_proto.h) | C++ | 12 | 0 | 2 | 14 |
| [build/CMakeCache.txt](/build/CMakeCache.txt) | CMake Cache | -2 | 0 | -1 | -3 |
| [build/CMakeFiles/CMakeConfigureLog.yaml](/build/CMakeFiles/CMakeConfigureLog.yaml) | YAML | -179 | 0 | -4 | -183 |
| [build/Testing/Temporary/LastTest.log](/build/Testing/Temporary/LastTest.log) | Log | -31 | 0 | -3 | -34 |
| [build/Testing/Temporary/LastTestsFailed.log](/build/Testing/Temporary/LastTestsFailed.log) | Log | -1 | 0 | -1 | -2 |
| [build/boot/gpu\_blob.c](/build/boot/gpu_blob.c) | C | 57 | 0 | 0 | 57 |
| [build/boot/init\_blob.c](/build/boot/init_blob.c) | C | 1,375 | 0 | 0 | 1,375 |
| [build/gpu-boot-serial-reset.log](/build/gpu-boot-serial-reset.log) | Log | -2 | 0 | -1 | -3 |
| [build/gpu-boot-serial.log](/build/gpu-boot-serial.log) | Log | -2 | 0 | -1 | -3 |
| [build/out/desktop-check.log](/build/out/desktop-check.log) | Log | -62 | 0 | -7 | -69 |
| [build/out/desktop-check2.log](/build/out/desktop-check2.log) | Log | 0 | 0 | -1 | -1 |
| [build/out/desktop-check3.log](/build/out/desktop-check3.log) | Log | 0 | 0 | -1 | -1 |
| [build/out/desktop-gui.log](/build/out/desktop-gui.log) | Log | 0 | 0 | -1 | -1 |
| [build/out/desktop-gui4.log](/build/out/desktop-gui4.log) | Log | 0 | 0 | -1 | -1 |
| [build/out/embedded-boot.log](/build/out/embedded-boot.log) | Log | -2,006 | 0 | -20 | -2,026 |
| [build/out/embedded-serial.log](/build/out/embedded-serial.log) | Log | -16 | 0 | 0 | -16 |
| [build/out/gui-fresh.log](/build/out/gui-fresh.log) | Log | -61 | 0 | -6 | -67 |
| [build/out/headless-desktop-serial.log](/build/out/headless-desktop-serial.log) | Log | -62 | 0 | -7 | -69 |
| [build/out/headless-fix-serial.log](/build/out/headless-fix-serial.log) | Log | -62 | 0 | -7 | -69 |
| [build/out/headless-probe-serial.log](/build/out/headless-probe-serial.log) | Log | -62 | 0 | -7 | -69 |
| [build/out/headless-stage-serial.log](/build/out/headless-stage-serial.log) | Log | -62 | 0 | -7 | -69 |
| [build/out/headless-user.log](/build/out/headless-user.log) | Log | -55 | 0 | -6 | -61 |
| [build/out/headless.log](/build/out/headless.log) | Log | 0 | 0 | -1 | -1 |
| [build/out/isa-debug-402.log](/build/out/isa-debug-402.log) | Log | -1,426 | 0 | -1 | -1,427 |
| [build/out/isa-debug-current.log](/build/out/isa-debug-current.log) | Log | -1,426 | 0 | -1 | -1,427 |
| [build/out/isa-debug-e9.log](/build/out/isa-debug-e9.log) | Log | 0 | 0 | -1 | -1 |
| [build/out/isa-debug.log](/build/out/isa-debug.log) | Log | -702 | 0 | -1 | -703 |
| [build/out/manual-qemu-serial.log](/build/out/manual-qemu-serial.log) | Log | 0 | 0 | -1 | -1 |
| [build/out/python-boot-monitor.log](/build/out/python-boot-monitor.log) | Log | 0 | 0 | -1 | -1 |
| [build/out/python-boot-shot.log](/build/out/python-boot-shot.log) | Log | -61 | 0 | -5 | -66 |
| [build/out/qemu-serial.log](/build/out/qemu-serial.log) | Log | -55 | 0 | -6 | -61 |
| [build/out/stage-kernel-file.log](/build/out/stage-kernel-file.log) | Log | -2,005 | 0 | -20 | -2,025 |
| [build/out/stage-kernel-open.log](/build/out/stage-kernel-open.log) | Log | -2,005 | 0 | -20 | -2,025 |
| [build/out/stage-loader-file.log](/build/out/stage-loader-file.log) | Log | -2,005 | 0 | -20 | -2,025 |
| [build/out/stage-loader-relative.log](/build/out/stage-loader-relative.log) | Log | -2,005 | 0 | -20 | -2,025 |
| [build/out/stage-openroot.log](/build/out/stage-openroot.log) | Log | -2,005 | 0 | -20 | -2,025 |
| [build/out/stage-root.log](/build/out/stage-root.log) | Log | -2,005 | 0 | -20 | -2,025 |
| [build/out/stage-watchdog-fresh.log](/build/out/stage-watchdog-fresh.log) | Log | -2,378 | 0 | -20 | -2,398 |
| [build/out/stage-watchdog.log](/build/out/stage-watchdog.log) | Log | -1,431 | 0 | -1 | -1,432 |
| [build/out/verify-hub.log](/build/out/verify-hub.log) | Log | -61 | 0 | -5 | -66 |
| [build/qemu-debugcon.log](/build/qemu-debugcon.log) | Log | 0 | 0 | -1 | -1 |
| [build/qemu-debugcon2.log](/build/qemu-debugcon2.log) | Log | 0 | 0 | -1 | -1 |
| [build/qemu-debugcon3.log](/build/qemu-debugcon3.log) | Log | 0 | 0 | -1 | -1 |
| [build/qemu-desktop.log](/build/qemu-desktop.log) | Log | -64 | 0 | -6 | -70 |
| [build/qemu-final.log](/build/qemu-final.log) | Log | -61 | 0 | -6 | -67 |
| [build/qemu-gfx.log](/build/qemu-gfx.log) | Log | -64 | 0 | -6 | -70 |
| [build/qemu-gui-final-serial.log](/build/qemu-gui-final-serial.log) | Log | -55 | 0 | -6 | -61 |
| [build/qemu-gui-serial.log](/build/qemu-gui-serial.log) | Log | -1,045 | 0 | -96 | -1,141 |
| [build/qemu-gui-stderr.log](/build/qemu-gui-stderr.log) | Log | 0 | 0 | -1 | -1 |
| [build/qemu-gui-stdout.log](/build/qemu-gui-stdout.log) | Log | 0 | 0 | -1 | -1 |
| [build/qemu-headless-stderr.log](/build/qemu-headless-stderr.log) | Log | 0 | 0 | -1 | -1 |
| [build/qemu-headless-stdout.log](/build/qemu-headless-stdout.log) | Log | 0 | 0 | -1 | -1 |
| [build/qemu-live.log](/build/qemu-live.log) | Log | -56 | 0 | -7 | -63 |
| [build/qemu-noreboot-serial.log](/build/qemu-noreboot-serial.log) | Log | 0 | 0 | -1 | -1 |
| [build/qemu-run-debug.log](/build/qemu-run-debug.log) | Log | -2 | 0 | -3 | -5 |
| [build/qemu-serial-trace.log](/build/qemu-serial-trace.log) | Log | -55 | 0 | -6 | -61 |
| [build/qemu-serial2.log](/build/qemu-serial2.log) | Log | -55 | 0 | -6 | -61 |
| [build/qemu-serial3.log](/build/qemu-serial3.log) | Log | -55 | 0 | -6 | -61 |
| [build/qemu-services.log](/build/qemu-services.log) | Log | -2 | 0 | -1 | -3 |
| [build/qemu-shell-serial.log](/build/qemu-shell-serial.log) | Log | -55 | 0 | -6 | -61 |
| [build/qemu-ui-fix.log](/build/qemu-ui-fix.log) | Log | -64 | 0 | -6 | -70 |
| [build/qemu.log](/build/qemu.log) | Log | -125 | 0 | -6 | -131 |
| [command-to-run.md](/command-to-run.md) | Markdown | 11 | 0 | 0 | 11 |
| [docs/architecture-tree.md](/docs/architecture-tree.md) | Markdown | -30 | 0 | -7 | -37 |
| [docs/architecture.md](/docs/architecture.md) | Markdown | 239 | 0 | 64 | 303 |
| [docs/build.md](/docs/build.md) | Markdown | 108 | 0 | 39 | 147 |
| [docs/developer-guide.md](/docs/developer-guide.md) | Markdown | 134 | 0 | 47 | 181 |
| [docs/package-format.md](/docs/package-format.md) | Markdown | 123 | 0 | 44 | 167 |
| [docs/roadmap.md](/docs/roadmap.md) | Markdown | 104 | 0 | 35 | 139 |
| [docs/running.md](/docs/running.md) | Markdown | 105 | 0 | 36 | 141 |
| [loader\_pars.md](/loader_pars.md) | Markdown | 155 | 0 | 5 | 160 |
| [tools/analyze\_tests.py](/tools/analyze_tests.py) | Python | 155 | 7 | 35 | 197 |

[Summary](results.md) / [Details](details.md) / [Diff Summary](diff.md) / Diff Details