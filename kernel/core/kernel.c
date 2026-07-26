#include "../include/vex/kernel.h"

static void write_kv_hex(const char* key, u64 value) {
    console_write(key);
    console_write(": ");
    console_write_hex(value);
    console_write("\n");
}

static void write_kv_dec(const char* key, u64 value) {
    console_write(key);
    console_write(": ");
    console_write_u64(value);
    console_write("\n");
}

static void write_banner(const char* title) {
    console_write("\n[");
    console_write(title);
    console_write("]\n");
}

static void run_self_checks(void) {
    vex_process_t supervisor = { .pid = 1u, .capability_mask = VEX_CAP_LOG | VEX_CAP_IPC };
    vex_process_t guest = { .pid = 2u, .capability_mask = VEX_CAP_LOG };
    vex_process_t graphics_client = { .pid = 3u, .capability_mask = VEX_CAP_LOG | VEX_CAP_GRAPHICS };

    vex_thread_t* idle = scheduler_create_thread(1u, 0u, 4u);
    vex_thread_t* init = scheduler_create_thread(2u, 1u, 2u);
    scheduler_enqueue(idle);
    scheduler_enqueue(init);

    vex_channel_t channel;
    vex_channel_message_t outbound = { .id = 42u, .length = 12u, .payload = "hello-world" };
    vex_channel_message_t inbound;

    ipc_init(&channel);

    if (scheduler_ready_count() != 2u) {
        console_write_line("selftest:scheduler_ready_count=FAIL");
        return;
    }

    if (scheduler_tick() == 0 || scheduler_tick() == 0) {
        console_write_line("selftest:scheduler_tick=FAIL");
        return;
    }

    if (ipc_send(&channel, &supervisor, &outbound) != 0 || ipc_receive(&channel, &inbound) != 0) {
        console_write_line("selftest:ipc_loopback=FAIL");
        return;
    }

    if (inbound.id != outbound.id) {
        console_write_line("selftest:ipc_payload=FAIL");
        return;
    }

    if (ipc_send(&channel, &guest, &outbound) == 0) {
        console_write_line("selftest:capability_gate=FAIL");
        return;
    }

    if (syscall_dispatch(VEX_ABI_VERSION, 0u, 0u, 0u, 0u, &supervisor) != VEX_ABI_VERSION) {
        console_write_line("selftest:sys_get_abi=FAIL");
        return;
    }

    if (syscall_dispatch(99u, 0u, 0u, 0u, 0u, &supervisor) != 0xFFFF0001u) {
        console_write_line("selftest:sys_abi_reject=FAIL");
        return;
    }

    if (syscall_dispatch(VEX_ABI_VERSION, 2u, 0x1234u, 0u, 0u, &guest) != 0xFFFF0003u) {
        console_write_line("selftest:sys_capability_reject=FAIL");
        return;
    }

    const u64 fence_handle = graphics_create_fence(&graphics_client);
    if (fence_handle == 0u) {
        console_write_line("selftest:gfx_fence_create=FAIL");
        return;
    }
    if (graphics_signal_fence((u32)fence_handle, 7u, &graphics_client) != 7u) {
        console_write_line("selftest:gfx_fence_signal=FAIL");
        return;
    }
    if (graphics_wait_fence((u32)fence_handle, 7u, &graphics_client) == 0u) {
        console_write_line("selftest:gfx_fence_wait=FAIL");
        return;
    }

    console_write_line("selftest:PASS");
}

void kernel_main(const vex_boot_info_t* boot_info) {
    enum {
        VEX_DIAGNOSTICS_SLOT = 0u,
        VEX_TESTCENTER_SLOT = 1u,
        VEX_SERVICES_SLOT = 2u,
        VEX_TERMINAL_SLOT = 3u,
        VEX_COMPOSITOR_SLOT = 4u,
        VEX_GPU_SLOT = 5u
    };
    vex_boot_info_t* mutable_boot_info = (vex_boot_info_t*)(usize)boot_info;
    serial_init();
    console_write_line("vex:kernel:enter");
    console_write_line("vex:phase:framebuffer_banner");
    framebuffer_fill_banner(&boot_info->framebuffer);
    console_write_line("vex:phase:framebuffer_console");
    framebuffer_console_init(&boot_info->framebuffer);

    console_write_line("vex:phase:memory_init");
    memory_init(boot_info);
    console_write_line("vex:phase:address_space_init");
    address_space_init(boot_info);
    console_write_line("vex:phase:acpi_init");
    acpi_init(boot_info);
    console_write_line("vex:phase:timer_init");
    timer_init();
    console_write_line("vex:phase:execution_init");
    execution_init();
    console_write_line("vex:phase:interrupts_init");
    interrupts_init();
    console_write_line("vex:phase:scheduler_init");
    scheduler_init();
    console_write_line("vex:phase:process_init");
    process_init();
    console_write_line("vex:phase:graphics_init");
    graphics_init(boot_info);

    const vex_address_space_t* kernel_space = address_space_kernel();
    const vex_acpi_state_t* acpi = acpi_state();
    const vex_timer_state_t* timer = timer_state();
    const vex_execution_state_t* exec = execution_state();

    write_banner("boot");
    write_kv_hex("rsdp", boot_info->rsdp_address);
    write_kv_hex("fb.base", boot_info->framebuffer.base);
    write_kv_dec("fb.size", boot_info->framebuffer.size);
    write_kv_dec("fb.width", boot_info->framebuffer.width);
    write_kv_dec("fb.height", boot_info->framebuffer.height);
    write_kv_dec("fb.pitch", boot_info->framebuffer.pixels_per_scanline);

    write_banner("memory");
    write_kv_dec("regions", memory_region_count());
    write_kv_dec("usable_pages", memory_usable_pages());
    write_kv_dec("allocated_pages", memory_allocated_pages());
    write_kv_hex("kernel.cr3", kernel_space->cr3_phys);
    write_kv_dec("kernel.mapped_pages", address_space_page_count(kernel_space));

    write_banner("platform");
    write_kv_hex("acpi.rsdp", acpi->rsdp_address);
    write_kv_hex("acpi.xsdt", acpi->xsdt_address);
    write_kv_hex("acpi.madt", acpi->madt_address);
    write_kv_dec("acpi.cpus", acpi->cpu_count);
    write_kv_dec("acpi.ioapics", acpi->ioapic_count);
    write_kv_dec("timer.kind", timer->source_kind);
    write_kv_hex("timer.base", timer->counter_base);
    write_kv_dec("timer.period_fs", timer->counter_period_femtoseconds);
    write_kv_dec("timer.ready", timer->available);
    write_kv_dec("exec.gdt_loaded", exec->gdt_loaded);
    write_kv_hex("exec.tss", exec->tss_base);
    write_kv_hex("exec.user_cs", exec->user_code_selector);
    write_kv_hex("exec.user_ss", exec->user_data_selector);
    write_kv_dec("gfx.surfaces", graphics_surface_count());
    write_kv_dec("gfx.fences", graphics_fence_count());

    console_write("vex:memory:regions=");
    console_write_u64(memory_region_count());
    console_write(" usable_pages=");
    console_write_u64(memory_usable_pages());
    console_write(" allocated_pages=");
    console_write_u64(memory_allocated_pages());
    console_write("\n");
    console_write("vex:vm:kernel_cr3=");
    console_write_hex(kernel_space->cr3_phys);
    console_write(" mapped_pages=");
    console_write_u64(address_space_page_count(kernel_space));
    console_write("\n");
    console_write("vex:exec:gdt=");
    console_write_u64(exec->gdt_loaded);
    console_write(" tss=");
    console_write_hex(exec->tss_base);
    console_write(" user_cs=");
    console_write_hex(exec->user_code_selector);
    console_write(" user_ss=");
    console_write_hex(exec->user_data_selector);
    console_write(" syscall=");
    console_write_u64(exec->syscall_ready);
    console_write(" lstar=");
    console_write_hex(exec->syscall_entry);
    console_write("\n");
    console_write("vex:gfx:surfaces=");
    console_write_u64(graphics_surface_count());
    console_write(" fences=");
    console_write_u64(graphics_fence_count());
    console_write("\n");

    u64 kernel_phys = 0;
    if (address_space_translate(kernel_space, (u64)(usize)boot_info, &kernel_phys, 0) != 0 ||
        kernel_phys != (u64)(usize)boot_info) {
        console_write_line("selftest:kernel_as_translate=FAIL");
        for (;;) {
            __asm__ volatile ("hlt");
        }
    }

    run_self_checks();

    if (boot_info->init_image.verified == 0u) {
        console_write_line("vex:init:verify=FAIL");
    } else {
        vex_manifest_view_t init_manifest;
        const int bootstrap_status = bootstrap_init_domain(boot_info, &init_manifest);
        if (bootstrap_status != 0) {
            write_banner("init");
            console_write("vex:init:bootstrap=FAIL code=");
            console_write_u64((u64)(0 - bootstrap_status));
            console_write(" payload=");
            console_write_hex(boot_info->init_image.payload_base);
            console_write("\n");
        } else {
            write_banner("init");
            console_write("domain: ");
            console_write(init_manifest.name);
            console_write("\n");
            console_write("entry: ");
            console_write(init_manifest.entrypoint);
            console_write("\n");
            write_kv_hex("caps", init_manifest.capability_mask);
            write_kv_dec("services", service_count());
            write_kv_dec("processes", process_count());
            console_write("vex:init:domain=");
            console_write(init_manifest.name);
            console_write(" entry=");
            console_write(init_manifest.entrypoint);
            console_write(" caps=");
            console_write_hex(init_manifest.capability_mask);
            console_write("\n");
            console_write("vex:services:count=");
            console_write_u64(service_count());
            console_write(" processes=");
            console_write_u64(process_count());
            console_write("\n");
            const vex_service_t* service = service_get(0u);
            if (service != 0 && service->owner != 0) {
                write_kv_hex("init.cr3", service->owner->address_space_root);
                write_kv_hex("init.entry_va", service->owner->user_entrypoint);
                write_kv_hex("init.stack_va", service->owner->user_stack_pointer);
                console_write("vex:init:cr3=");
                console_write_hex(service->owner->address_space_root);
                console_write(" entry_va=");
                console_write_hex(service->owner->user_entrypoint);
                console_write(" stack_va=");
                console_write_hex(service->owner->user_stack_pointer);
                console_write("\n");
            }
            const vex_user_context_t* user_context = process_user_context(0u);
            if (user_context != 0) {
                write_kv_hex("user.cs", user_context->cs);
                write_kv_hex("user.ss", user_context->ss);
                write_kv_hex("user.rip", user_context->rip);
                write_kv_hex("user.rsp", user_context->rsp);
                console_write("vex:init:user_ctx cs=");
                console_write_hex(user_context->cs);
                console_write(" ss=");
                console_write_hex(user_context->ss);
                console_write(" rip=");
                console_write_hex(user_context->rip);
                console_write(" rsp=");
                console_write_hex(user_context->rsp);
                console_write("\n");
            }

            for (u32 index = 0; index < boot_info->app_image_count && index < VEX_MAX_APP_IMAGES; ++index) {
                const vex_package_image_info_t* image = &boot_info->app_images[index];
                vex_manifest_view_t app_manifest;
                const int app_status = bootstrap_package_domain(boot_info, image, 4u + index, &app_manifest);

                console_write("vex:app:slot=");
                console_write_u64(index);
                console_write(" verified=");
                console_write_u64(image->verified);

                if (image->verified == 0u) {
                    console_write(" status=SKIP\n");
                    continue;
                }

                if (app_status != 0) {
                    console_write(" status=FAIL code=");
                    console_write_u64((u64)(0 - app_status));
                    console_write(" payload=");
                    console_write_hex(image->payload_base);
                    console_write("\n");
                    continue;
                }

                console_write(" status=READY name=");
                console_write(app_manifest.name);
                console_write(" entry=");
                console_write(app_manifest.entrypoint);
                console_write(" caps=");
                console_write_hex(app_manifest.capability_mask);
                if (service_count() > index + 1u) {
                    const vex_service_t* app_service = service_get(index + 1u);
                    if (app_service != 0 && app_service->owner != 0 &&
                        app_service->owner->default_surface_handle != 0u) {
                        const vex_service_t* init_service = service_get(0u);
                        if (init_service != 0 &&
                            init_service->owner != 0 &&
                            index < VEX_MAX_SHARED_SURFACES) {
                            const u64 shared_mapping = graphics_share_surface(
                                app_service->owner->default_surface_handle,
                                init_service->owner,
                                process_address_space(init_service->owner),
                                VEX_USER_SHARED_SURFACE_BASE + (u64)index * VEX_USER_SHARED_SURFACE_STRIDE
                            );
                            if (shared_mapping != 0u) {
                                if (app_service->owner->default_mailbox_phys != 0u) {
                                    (void)address_space_map_user_range(
                                        process_address_space(init_service->owner),
                                        VEX_USER_SHARED_MAILBOX_BASE + (u64)index * 4096u,
                                        app_service->owner->default_mailbox_phys,
                                        4096u,
                                        1u,
                                        0u
                                    );
                                }
                                mutable_boot_info->shared_surfaces[index].owner_pid = app_service->owner->pid;
                                mutable_boot_info->shared_surfaces[index].surface_handle = app_service->owner->default_surface_handle;
                                mutable_boot_info->shared_surfaces[index].fence_handle = app_service->owner->default_fence_handle;
                                mutable_boot_info->shared_surfaces[index].width = (u32)graphics_query_surface(app_service->owner->default_surface_handle, VEX_SURFACE_QUERY_WIDTH, app_service->owner);
                                mutable_boot_info->shared_surfaces[index].height = (u32)graphics_query_surface(app_service->owner->default_surface_handle, VEX_SURFACE_QUERY_HEIGHT, app_service->owner);
                                mutable_boot_info->shared_surfaces[index].stride = (u32)graphics_query_surface(app_service->owner->default_surface_handle, VEX_SURFACE_QUERY_STRIDE, app_service->owner);
                                mutable_boot_info->shared_surfaces[index].buffer_count = (u32)graphics_query_surface(app_service->owner->default_surface_handle, VEX_SURFACE_QUERY_BUFFER_COUNT, app_service->owner);
                                mutable_boot_info->shared_surfaces[index].present_index = (u32)graphics_query_surface(app_service->owner->default_surface_handle, VEX_SURFACE_QUERY_PRESENT_INDEX, app_service->owner);
                                mutable_boot_info->shared_surfaces[index].private_mapping_base = app_service->owner->default_surface_va;
                                mutable_boot_info->shared_surfaces[index].shared_mapping_base = shared_mapping;
                                mutable_boot_info->shared_surfaces[index].private_mailbox_base = app_service->owner->default_mailbox_va;
                                mutable_boot_info->shared_surfaces[index].shared_mailbox_base = VEX_USER_SHARED_MAILBOX_BASE + (u64)index * 4096u;
                                mutable_boot_info->shared_surfaces[index].bytes_per_buffer = graphics_query_surface(app_service->owner->default_surface_handle, VEX_SURFACE_QUERY_BUFFER_BYTES, app_service->owner);
                            }
                        }
                        console_write(" surface=");
                        console_write_u64(app_service->owner->default_surface_handle);
                        console_write(" fence=");
                        console_write_u64(app_service->owner->default_fence_handle);
                    }
                }
                console_write("\n");
            }

            if (service_count() > (VEX_COMPOSITOR_SLOT + 1u)) {
                const vex_service_t* init_service = service_get(0u);
                const vex_service_t* compositor_service = service_get(VEX_COMPOSITOR_SLOT + 1u);
                const vex_service_t* gpu_service = service_count() > (VEX_GPU_SLOT + 1u)
                    ? service_get(VEX_GPU_SLOT + 1u)
                    : 0;

                if (init_service != 0 &&
                    init_service->owner != 0 &&
                    compositor_service != 0 &&
                    compositor_service->owner != 0) {
                    if (compositor_service->owner->default_mailbox_phys != 0u) {
                        if (address_space_map_user_range(
                                process_address_space(init_service->owner),
                                VEX_USER_INIT_COMPOSITOR_MAILBOX_BASE,
                                compositor_service->owner->default_mailbox_phys,
                                4096u,
                                1u,
                                0u
                            ) == 0) {
                            mutable_boot_info->compositor_scene_mailbox_base = VEX_USER_INIT_COMPOSITOR_MAILBOX_BASE;
                        }
                    }

                    if (gpu_service != 0 &&
                        gpu_service->owner != 0 &&
                        gpu_service->owner->default_mailbox_phys != 0u) {
                        if (address_space_map_user_range(
                                process_address_space(init_service->owner),
                                VEX_USER_INIT_GPU_MAILBOX_BASE,
                                gpu_service->owner->default_mailbox_phys,
                                4096u,
                                1u,
                                0u
                            ) == 0) {
                            mutable_boot_info->init_gpu_mailbox_base = VEX_USER_INIT_GPU_MAILBOX_BASE;
                        }
                        if (address_space_map_user_range(
                                process_address_space(compositor_service->owner),
                                VEX_USER_COMPOSITOR_GPU_MAILBOX_BASE,
                                gpu_service->owner->default_mailbox_phys,
                                4096u,
                                1u,
                                0u
                            ) == 0) {
                            mutable_boot_info->compositor_gpu_mailbox_base = VEX_USER_COMPOSITOR_GPU_MAILBOX_BASE;
                        }
                    }

                    for (u32 index = 0u; index < VEX_COMPOSITOR_SLOT; ++index) {
                        const vex_service_t* app_service = service_get(index + 1u);
                        if (app_service == 0 ||
                            app_service->owner == 0 ||
                            app_service->owner->default_surface_handle == 0u) {
                            continue;
                        }

                        const u64 compositor_mapping = graphics_share_surface(
                            app_service->owner->default_surface_handle,
                            compositor_service->owner,
                            process_address_space(compositor_service->owner),
                            VEX_USER_COMPOSITOR_SHARED_SURFACE_BASE + (u64)index * VEX_USER_SHARED_SURFACE_STRIDE
                        );
                        if (compositor_mapping == 0u) {
                            continue;
                        }

                        mutable_boot_info->shared_surfaces[index].compositor_mapping_base = compositor_mapping;
                        if (app_service->owner->default_mailbox_phys != 0u &&
                            address_space_map_user_range(
                                process_address_space(compositor_service->owner),
                                VEX_USER_COMPOSITOR_SHARED_MAILBOX_BASE + (u64)index * 4096u,
                                app_service->owner->default_mailbox_phys,
                                4096u,
                                1u,
                                0u
                            ) == 0) {
                            mutable_boot_info->shared_surfaces[index].compositor_mailbox_base =
                                VEX_USER_COMPOSITOR_SHARED_MAILBOX_BASE + (u64)index * 4096u;
                        }

                        if (gpu_service != 0 &&
                            gpu_service->owner != 0) {
                            const u64 gpu_mapping = graphics_share_surface(
                                app_service->owner->default_surface_handle,
                                gpu_service->owner,
                                process_address_space(gpu_service->owner),
                                VEX_USER_GPU_SHARED_SURFACE_BASE + (u64)index * VEX_USER_SHARED_SURFACE_STRIDE
                            );
                            if (gpu_mapping != 0u) {
                                mutable_boot_info->shared_surfaces[index].gpu_mapping_base = gpu_mapping;
                                if (app_service->owner->default_mailbox_phys != 0u &&
                                    address_space_map_user_range(
                                        process_address_space(gpu_service->owner),
                                        VEX_USER_GPU_SHARED_MAILBOX_BASE + (u64)index * 4096u,
                                        app_service->owner->default_mailbox_phys,
                                        4096u,
                                        1u,
                                        0u
                                    ) == 0) {
                                    mutable_boot_info->shared_surfaces[index].gpu_mailbox_base =
                                        VEX_USER_GPU_SHARED_MAILBOX_BASE + (u64)index * 4096u;
                                }
                            }
                        }
                    }

                    if (gpu_service != 0 &&
                        gpu_service->owner != 0 &&
                        compositor_service->owner->default_mailbox_phys != 0u) {
                        if (address_space_map_user_range(
                                process_address_space(gpu_service->owner),
                                VEX_USER_GPU_SCENE_MAILBOX_BASE,
                                compositor_service->owner->default_mailbox_phys,
                                4096u,
                                1u,
                                0u
                            ) == 0) {
                            mutable_boot_info->gpu_compositor_scene_mailbox_base = VEX_USER_GPU_SCENE_MAILBOX_BASE;
                        }
                    }
                }
            }

            write_kv_dec("services", service_count());
            write_kv_dec("processes", process_count());
        }
    }

    write_banner("status");
    console_write_line("selftest: PASS");
    console_write_line("kernel: READY");
    console_write_line("vex:kernel:ready");

    const vex_user_context_t* init_context = process_user_context(0u);
    if (init_context != 0) {
        execution_enter_user(init_context);
    }

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
