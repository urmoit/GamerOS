#include "io_manager.h"
#include "../../intf/memory.h"
#include "../../intf/string.h"

#define MAX_DRIVERS 32
#define MAX_IO_REQUESTS 256

static device_driver_t* registered_drivers[MAX_DRIVERS];
static uint32_t driver_count = 0;

static io_request_t io_requests[MAX_IO_REQUESTS];
static uint32_t next_request_id = 0;

void io_manager_init(void) {
    memset(registered_drivers, 0, sizeof(registered_drivers));
    memset(io_requests, 0, sizeof(io_requests));
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
    // For now, implement synchronously
    int result = io_read(device, buffer, size);
    if (callback) {
        io_request_t req = {
            .type = IO_REQUEST_READ,
            .device_handle = device,
            .buffer = buffer,
            .size = size,
            .status = result >= 0 ? IO_STATUS_COMPLETED : IO_STATUS_FAILED,
            .bytes_transferred = result >= 0 ? (uint32_t)result : 0,
            .completion_callback = callback,
            .context = context
        };
        callback(&req);
    }
    return result;
}

int io_write_async(object_handle_t device, const void* buffer, uint32_t size,
                   void (*callback)(io_request_t*), void* context) {
    // For now, implement synchronously
    int result = io_write(device, buffer, size);
    if (callback) {
        io_request_t req = {
            .type = IO_REQUEST_WRITE,
            .device_handle = device,
            .buffer = (void*)buffer,
            .size = size,
            .status = result >= 0 ? IO_STATUS_COMPLETED : IO_STATUS_FAILED,
            .bytes_transferred = result >= 0 ? (uint32_t)result : 0,
            .completion_callback = callback,
            .context = context
        };
        callback(&req);
    }
    return result;
}