#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include "../../intf/stdint.h"
#include "../object_manager/object_manager.h"  // For object_handle_t

// I/O request types
typedef enum {
    IO_REQUEST_READ,
    IO_REQUEST_WRITE,
    IO_REQUEST_IOCTL
} io_request_type_t;

// I/O status
typedef enum {
    IO_STATUS_PENDING,
    IO_STATUS_COMPLETED,
    IO_STATUS_FAILED,
    IO_STATUS_CANCELLED
} io_status_t;

// I/O request structure
typedef struct io_request {
    io_request_type_t type;
    object_handle_t device_handle;
    void* buffer;
    uint32_t size;
    uint32_t offset;
    uint32_t ioctl_code;
    void* ioctl_data;
    io_status_t status;
    uint32_t bytes_transferred;
    void (*completion_callback)(struct io_request* request);
    void* context;
} io_request_t;

// Device driver interface
typedef struct device_driver {
    char* name;
    uint32_t type;
    int (*init)(void);
    int (*read)(object_handle_t device, void* buffer, uint32_t size, uint32_t offset);
    int (*write)(object_handle_t device, const void* buffer, uint32_t size, uint32_t offset);
    int (*ioctl)(object_handle_t device, uint32_t code, void* data);
    int (*shutdown)(void);
} device_driver_t;

// I/O Manager functions
int io_register_driver(device_driver_t* driver);
int io_unregister_driver(const char* name);

object_handle_t io_open_device(const char* name);
int io_close_device(object_handle_t handle);

int io_read(object_handle_t device, void* buffer, uint32_t size);
int io_write(object_handle_t device, const void* buffer, uint32_t size);
int io_ioctl(object_handle_t device, uint32_t code, void* data);

// Asynchronous I/O
int io_read_async(object_handle_t device, void* buffer, uint32_t size,
                  void (*callback)(io_request_t*), void* context);
int io_write_async(object_handle_t device, const void* buffer, uint32_t size,
                   void (*callback)(io_request_t*), void* context);

// Initialization
void io_manager_init(void);

#endif