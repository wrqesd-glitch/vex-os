#ifndef VEX_BOOT_EFI_H
#define VEX_BOOT_EFI_H

#include "../../../kernel/include/vex/boot_info.h"

#define EFIAPI __attribute__((ms_abi))

typedef void* efi_handle_t;
typedef u64 efi_status_t;
typedef u64 efi_uintn_t;
typedef u64 efi_physical_address_t;
typedef void* efi_event_t;
typedef u16 efi_char16_t;

#define EFI_PAGE_SIZE 4096u

#define EFI_SUCCESS 0u
#define EFI_LOAD_ERROR 0x8000000000000001ull
#define EFI_BUFFER_TOO_SMALL 0x8000000000000005ull
#define EFI_INVALID_PARAMETER 0x8000000000000002ull
#define EFI_UNSUPPORTED 0x8000000000000003ull
#define EFI_OUT_OF_RESOURCES 0x8000000000000009ull
#define EFI_NOT_FOUND 0x8000000000000014ull
#define EFI_SECURITY_VIOLATION 0x800000000000001Aull
#define EFI_ABORTED 0x8000000000000015ull

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL 0x00000001u
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL 0x00000002u

#define EFI_FILE_MODE_READ 0x0000000000000001ull
#define EFI_FILE_READ_ONLY 0x0000000000000001ull
#define EFI_FILE_DIRECTORY 0x0000000000000010ull

#define EFI_ALLOCATE_ANY_PAGES 0u
#define EFI_ALLOCATE_MAX_ADDRESS 1u
#define EFI_ALLOCATE_ADDRESS 2u

#define EFI_RESERVED_MEMORY_TYPE 0u
#define EFI_LOADER_CODE 1u
#define EFI_LOADER_DATA 2u
#define EFI_BOOT_SERVICES_CODE 3u
#define EFI_BOOT_SERVICES_DATA 4u
#define EFI_CONVENTIONAL_MEMORY 7u

typedef struct efi_guid {
    u32 data1;
    u16 data2;
    u16 data3;
    u8 data4[8];
} efi_guid_t;

typedef struct efi_table_header {
    u64 signature;
    u32 revision;
    u32 header_size;
    u32 crc32;
    u32 reserved;
} efi_table_header_t;

typedef struct efi_memory_descriptor {
    u32 type;
    u32 padding;
    u64 physical_start;
    u64 virtual_start;
    u64 page_count;
    u64 attributes;
} efi_memory_descriptor_t;

typedef struct efi_configuration_table {
    efi_guid_t vendor_guid;
    void* vendor_table;
} efi_configuration_table_t;

typedef struct efi_simple_text_output_protocol {
    void* reset;
    efi_status_t(EFIAPI* output_string)(
        struct efi_simple_text_output_protocol* self,
        efi_char16_t* string
    );
} efi_simple_text_output_protocol_t;

struct efi_boot_services;
struct efi_runtime_services;
struct efi_file_protocol;
struct efi_graphics_output_protocol;

typedef struct efi_system_table {
    efi_table_header_t header;
    efi_char16_t* firmware_vendor;
    u32 firmware_revision;
    efi_handle_t console_in_handle;
    void* con_in;
    efi_handle_t console_out_handle;
    efi_simple_text_output_protocol_t* con_out;
    efi_handle_t standard_error_handle;
    efi_simple_text_output_protocol_t* std_err;
    struct efi_runtime_services* runtime_services;
    struct efi_boot_services* boot_services;
    efi_uintn_t number_of_table_entries;
    efi_configuration_table_t* configuration_table;
} efi_system_table_t;

typedef struct efi_loaded_image_protocol {
    u32 revision;
    efi_handle_t parent_handle;
    efi_system_table_t* system_table;
    efi_handle_t device_handle;
    void* file_path;
    void* reserved;
    u32 load_options_size;
    void* load_options;
    void* image_base;
    u64 image_size;
    u32 image_code_type;
    u32 image_data_type;
    efi_status_t(EFIAPI* unload)(efi_handle_t image_handle);
} efi_loaded_image_protocol_t;

typedef struct efi_simple_file_system_protocol {
    u64 revision;
    efi_status_t(EFIAPI* open_volume)(
        struct efi_simple_file_system_protocol* self,
        struct efi_file_protocol** root
    );
} efi_simple_file_system_protocol_t;

typedef struct efi_file_info {
    u64 size;
    u64 file_size;
    u64 physical_size;
    struct {
        u16 year;
        u8 month;
        u8 day;
        u8 hour;
        u8 minute;
        u8 second;
        u8 pad1;
        u32 nanosecond;
        short timezone;
        u8 daylight;
        u8 pad2;
    } create_time;
    struct {
        u16 year;
        u8 month;
        u8 day;
        u8 hour;
        u8 minute;
        u8 second;
        u8 pad1;
        u32 nanosecond;
        short timezone;
        u8 daylight;
        u8 pad2;
    } last_access_time;
    struct {
        u16 year;
        u8 month;
        u8 day;
        u8 hour;
        u8 minute;
        u8 second;
        u8 pad1;
        u32 nanosecond;
        short timezone;
        u8 daylight;
        u8 pad2;
    } modification_time;
    u64 attribute;
    efi_char16_t file_name[1];
} efi_file_info_t;

typedef struct efi_file_protocol {
    u64 revision;
    efi_status_t(EFIAPI* open)(
        struct efi_file_protocol* self,
        struct efi_file_protocol** new_handle,
        efi_char16_t* file_name,
        u64 open_mode,
        u64 attributes
    );
    efi_status_t(EFIAPI* close)(struct efi_file_protocol* self);
    efi_status_t(EFIAPI* delete_file)(struct efi_file_protocol* self);
    efi_status_t(EFIAPI* read)(
        struct efi_file_protocol* self,
        efi_uintn_t* buffer_size,
        void* buffer
    );
    efi_status_t(EFIAPI* write)(
        struct efi_file_protocol* self,
        efi_uintn_t* buffer_size,
        const void* buffer
    );
    efi_status_t(EFIAPI* get_position)(struct efi_file_protocol* self, u64* position);
    efi_status_t(EFIAPI* set_position)(struct efi_file_protocol* self, u64 position);
    efi_status_t(EFIAPI* get_info)(
        struct efi_file_protocol* self,
        efi_guid_t* information_type,
        efi_uintn_t* buffer_size,
        void* buffer
    );
    efi_status_t(EFIAPI* set_info)(
        struct efi_file_protocol* self,
        efi_guid_t* information_type,
        efi_uintn_t buffer_size,
        void* buffer
    );
    efi_status_t(EFIAPI* flush)(struct efi_file_protocol* self);
} efi_file_protocol_t;

typedef struct efi_pixel_bitmask {
    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
    u32 reserved_mask;
} efi_pixel_bitmask_t;

typedef struct efi_graphics_output_mode_information {
    u32 version;
    u32 horizontal_resolution;
    u32 vertical_resolution;
    u32 pixel_format;
    efi_pixel_bitmask_t pixel_information;
    u32 pixels_per_scan_line;
} efi_graphics_output_mode_information_t;

typedef struct efi_graphics_output_protocol_mode {
    u32 max_mode;
    u32 mode;
    efi_graphics_output_mode_information_t* info;
    efi_uintn_t size_of_info;
    efi_physical_address_t frame_buffer_base;
    efi_uintn_t frame_buffer_size;
} efi_graphics_output_protocol_mode_t;

typedef struct efi_graphics_output_protocol {
    efi_status_t(EFIAPI* query_mode)(
        struct efi_graphics_output_protocol* self,
        u32 mode_number,
        efi_uintn_t* size_of_info,
        efi_graphics_output_mode_information_t** info
    );
    efi_status_t(EFIAPI* set_mode)(struct efi_graphics_output_protocol* self, u32 mode_number);
    efi_status_t(EFIAPI* blt)(
        struct efi_graphics_output_protocol* self,
        void* blt_buffer,
        u32 blt_operation,
        efi_uintn_t source_x,
        efi_uintn_t source_y,
        efi_uintn_t destination_x,
        efi_uintn_t destination_y,
        efi_uintn_t width,
        efi_uintn_t height,
        efi_uintn_t delta
    );
    efi_graphics_output_protocol_mode_t* mode;
} efi_graphics_output_protocol_t;

typedef struct efi_boot_services {
    efi_table_header_t header;
    void* raise_tpl;
    void* restore_tpl;
    efi_status_t(EFIAPI* allocate_pages)(
        u32 type,
        u32 memory_type,
        efi_uintn_t pages,
        efi_physical_address_t* memory
    );
    efi_status_t(EFIAPI* free_pages)(
        efi_physical_address_t memory,
        efi_uintn_t pages
    );
    efi_status_t(EFIAPI* get_memory_map)(
        efi_uintn_t* memory_map_size,
        efi_memory_descriptor_t* memory_map,
        efi_uintn_t* map_key,
        efi_uintn_t* descriptor_size,
        u32* descriptor_version
    );
    efi_status_t(EFIAPI* allocate_pool)(
        u32 pool_type,
        efi_uintn_t size,
        void** buffer
    );
    efi_status_t(EFIAPI* free_pool)(void* buffer);
    void* create_event;
    void* set_timer;
    void* wait_for_event;
    void* signal_event;
    void* close_event;
    void* check_event;
    void* install_protocol_interface;
    void* reinstall_protocol_interface;
    void* uninstall_protocol_interface;
    efi_status_t(EFIAPI* handle_protocol)(
        efi_handle_t handle,
        efi_guid_t* protocol,
        void** interface
    );
    void* reserved;
    void* register_protocol_notify;
    void* locate_handle;
    void* locate_device_path;
    void* install_configuration_table;
    void* load_image;
    void* start_image;
    void* exit;
    void* unload_image;
    efi_status_t(EFIAPI* exit_boot_services)(efi_handle_t image_handle, efi_uintn_t map_key);
    void* get_next_monotonic_count;
    efi_status_t(EFIAPI* stall)(efi_uintn_t microseconds);
    efi_status_t(EFIAPI* set_watchdog_timer)(
        efi_uintn_t timeout,
        u64 watchdog_code,
        efi_uintn_t data_size,
        efi_char16_t* watchdog_data
    );
    void* connect_controller;
    void* disconnect_controller;
    efi_status_t(EFIAPI* open_protocol)(
        efi_handle_t handle,
        efi_guid_t* protocol,
        void** interface,
        efi_handle_t agent_handle,
        efi_handle_t controller_handle,
        u32 attributes
    );
    efi_status_t(EFIAPI* close_protocol)(
        efi_handle_t handle,
        efi_guid_t* protocol,
        efi_handle_t agent_handle,
        efi_handle_t controller_handle
    );
    void* open_protocol_information;
    void* protocols_per_handle;
    void* locate_handle_buffer;
    efi_status_t(EFIAPI* locate_protocol)(
        efi_guid_t* protocol,
        void* registration,
        void** interface
    );
    void* install_multiple_protocol_interfaces;
    void* uninstall_multiple_protocol_interfaces;
    void* calculate_crc32;
    void* copy_mem;
    void* set_mem;
    void* create_event_ex;
} efi_boot_services_t;

extern const efi_guid_t EFI_LOADED_IMAGE_PROTOCOL_GUID_VALUE;
extern const efi_guid_t EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_VALUE;
extern const efi_guid_t EFI_FILE_INFO_GUID_VALUE;
extern const efi_guid_t EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID_VALUE;
extern const efi_guid_t EFI_ACPI_20_TABLE_GUID_VALUE;
extern const efi_guid_t EFI_ACPI_TABLE_GUID_VALUE;

#endif
