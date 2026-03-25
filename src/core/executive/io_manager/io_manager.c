#include "io_manager.h"
#include <memory.h>
#include <string.h>

#define MAX_DRIVERS 32
#define MAX_IO_REQUESTS 256

static device_driver_t* registered_drivers[MAX_DRIVERS];
static uint32_t driver_count = 0;

static io_request_t io_requests[MAX_IO_REQUESTS];
static uint8_t request_used[MAX_IO_REQUESTS];
static uint32_t next_request_id = 0;

static int allocate_request_slot(void) {
    for (uint32_t i = 0; i < MAX_IO_REQUESTS; i++) {
        if (!request_used[i]) {
            request_used[i] = 1;
            return (int)i;
        }
    }
    return -1;
}

void io_manager_init(void) {
    memset(registered_drivers, 0, sizeof(registered_drivers));
    memset(io_requests, 0, sizeof(io_requests));
    memset(request_used, 0, sizeof(request_used));
    driver_count = 0;
    next_request_id = 0;
}

int io_register_driver(device_driver_t* driver) {
    if (!driver || driver_count >= MAX_DRIVERS) {
        return -1;
    }

    // Check if driver already registered
    for (uint32_t i = 0; i < driver_count; i++) {
        if (strcmp(registered_drivers[i]->name, driver->name) == 0) {
            return -2;  // Already registered
        }
    }

    registered_drivers[driver_count++] = driver;

    // Initialize the driver
    if (driver->init) {
        return driver->init();
    }

    return 0;
}

int io_unregister_driver(const char* name) {
    for (uint32_t i = 0; i < driver_count; i++) {
        if (strcmp(registered_drivers[i]->name, name) == 0) {
            // Shutdown driver
            if (registered_drivers[i]->shutdown) {
                registered_drivers[i]->shutdown();
            }

            // Remove from array
            for (uint32_t j = i; j < driver_count - 1; j++) {
                registered_drivers[j] = registered_drivers[j + 1];
            }
            driver_count--;
            return 0;
        }
    }
    return -1;  // Not found
}

object_handle_t io_open_device(const char* name) {
    for (uint32_t i = 0; i < driver_count; i++) {
        if (strcmp(registered_drivers[i]->name, name) == 0) {
            // Create device object
            object_attributes_t attrs = {
                .type = OBJECT_TYPE_DEVICE,
                .context = registered_drivers[i]
            };
            return object_create(OBJECT_TYPE_DEVICE, &attrs);
        }
    }
    return INVALID_HANDLE;
}

int io_close_device(object_handle_t handle) {
    if (object_get_type(handle) != OBJECT_TYPE_DEVICE) {
        return -1;
    }
    return object_destroy(handle);
}

static device_driver_t* get_driver_from_device(object_handle_t device) {
    void* context = object_get_context(device);
    return (device_driver_t*)context;
}

int io_read(object_handle_t device, void* buffer, uint32_t size) {
    device_driver_t* driver = get_driver_from_device(device);
    if (!driver || !driver->read) {
        return -1;
    }
    return driver->read(device, buffer, size, 0);
}

int io_write(object_handle_t device, const void* buffer, uint32_t size) {
    device_driver_t* driver = get_driver_from_device(device);
    if (!driver || !driver->write) {
        return -1;
    }
    return driver->write(device, buffer, size, 0);
}

int io_ioctl(object_handle_t device, uint32_t code, void* data) {
    device_driver_t* driver = get_driver_from_device(device);
    if (!driver || !driver->ioctl) {
        return -1;
    }
    return driver->ioctl(device, code, data);
}

int io_read_async(object_handle_t device, void* buffer, uint32_t size,
                  void (*callback)(io_request_t*), void* context) {
    int slot = allocate_request_slot();
    if (slot < 0) {
        return -1;
    }

    next_request_id++;
    io_requests[slot].type = IO_REQUEST_READ;
    io_requests[slot].device_handle = device;
    io_requests[slot].buffer = buffer;
    io_requests[slot].size = size;
    io_requests[slot].offset = 0;
    io_requests[slot].ioctl_code = 0;
    io_requests[slot].ioctl_data = 0;
    io_requests[slot].status = IO_STATUS_PENDING;
    io_requests[slot].bytes_transferred = 0;
    io_requests[slot].completion_callback = callback;
    io_requests[slot].context = context;

    return (int)next_request_id;
}

// TODO: Add timeout support for I/O operations
// TODO: Implement I/O request cancellation
// TODO: Add priority queuing for I/O requests
// TODO: Implement scatter-gather I/O operations

int io_write_async(object_handle_t device, const void* buffer, uint32_t size,
                   void (*callback)(io_request_t*), void* context) {
    int slot = allocate_request_slot();
    if (slot < 0) {
        return -1;
    }

    next_request_id++;
    io_requests[slot].type = IO_REQUEST_WRITE;
    io_requests[slot].device_handle = device;
    io_requests[slot].buffer = (void*)buffer;
    io_requests[slot].size = size;
    io_requests[slot].offset = 0;
    io_requests[slot].ioctl_code = 0;
    io_requests[slot].ioctl_data = 0;
    io_requests[slot].status = IO_STATUS_PENDING;
    io_requests[slot].bytes_transferred = 0;
    io_requests[slot].completion_callback = callback;
    io_requests[slot].context = context;

    return (int)next_request_id;
}

int io_process_async_requests(uint32_t max_requests) {
    uint32_t processed = 0;
    for (uint32_t i = 0; i < MAX_IO_REQUESTS; i++) {
        if (max_requests && processed >= max_requests) {
            break;
        }
        if (!request_used[i] || io_requests[i].status != IO_STATUS_PENDING) {
            continue;
        }

        int result;
        if (io_requests[i].type == IO_REQUEST_READ) {
            result = io_read(io_requests[i].device_handle, io_requests[i].buffer, io_requests[i].size);
        } else if (io_requests[i].type == IO_REQUEST_WRITE) {
            result = io_write(io_requests[i].device_handle, io_requests[i].buffer, io_requests[i].size);
        } else {
            result = -1;
        }

        io_requests[i].status = (result >= 0) ? IO_STATUS_COMPLETED : IO_STATUS_FAILED;
        io_requests[i].bytes_transferred = (result >= 0) ? (uint32_t)result : 0;

        if (io_requests[i].completion_callback) {
            io_requests[i].completion_callback(&io_requests[i]);
        }

        request_used[i] = 0;
        processed++;
    }
    return (int)processed;
}
