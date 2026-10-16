#include <vboxguest.h>
#include <pci.h>
#include <ports.h>
#include <graphics.h>
#include <mouse.h>
#include <serial.h>
#include <string.h>

// VirtualBox VMM device mouse integration driver.

#define VBOX_VENDOR_ID          0x80EE
#define VBOX_VMMDEV_DEVICE_ID   0xCAFE

// Request header, present at the start of every request type.
typedef struct {
    uint32_t size;            // size of the whole request, including this header
    uint32_t version;         // header version: 0x00010001
    uint32_t request_type;
    int32_t rc;               // set to -1 before sending; >= 0 (VINF_SUCCESS) after
    uint32_t reserved1;
    uint32_t reserved2;
} vbox_request_header_t;

#define VBOX_REQUEST_HEADER_VERSION 0x00010001u

// Request types used by this driver.
#define VMMDEVREQ_GET_MOUSE_STATUS 1
#define VMMDEVREQ_SET_MOUSE_STATUS 2

// Mouse capability bits, as reported in the response's feature mask.
#define VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE  (1u << 0)
#define VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE (1u << 1)

typedef struct {
    vbox_request_header_t header;
    uint32_t mouse_features;
    int32_t pointer_x_pos;
    int32_t pointer_y_pos;
} vbox_mouse_status_request_t;

static uint16_t vbox_port = 0;
static uint8_t vbox_request[512] __attribute__((aligned(4096)));
static int vbox_present = 0;

static vbox_mouse_status_request_t* vbox_req(void) {
    return (vbox_mouse_status_request_t*)vbox_request;
}

static void vbox_submit(void) {
    // One port write hands the block over; the request is synchronous.
    outl(vbox_port, (uint32_t)(uintptr_t)vbox_request);
}

void vboxguest_init(void) {
    pci_device_t pci;

    if (!pci_find_device(VBOX_VENDOR_ID, VBOX_VMMDEV_DEVICE_ID, &pci)) {
        return;
    }

    uint32_t bar0 = pci_get_bar(&pci, 0);
    uint16_t port = (uint16_t)(bar0 & 0xFFFFFFFCu);
    if (!port) {
        return;
    }

    pci_enable_bus_mastering(&pci);
    vbox_port = port;

    vbox_mouse_status_request_t* req = vbox_req();
    memset(req, 0, sizeof(*req));
    req->header.size = (uint32_t)sizeof(*req);
    req->header.version = VBOX_REQUEST_HEADER_VERSION;
    req->header.request_type = VMMDEVREQ_SET_MOUSE_STATUS;
    req->header.rc = -1;
    req->mouse_features = VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE;
    req->pointer_x_pos = 0;
    req->pointer_y_pos = 0;
    vbox_submit();

    vbox_present = 1;
    serial_write_string("VirtualBox: VMM device found, absolute mouse advertised\n");
}

int vboxguest_mouse_poll(void) {
    if (!vbox_present) return 0;

    vbox_mouse_status_request_t* req = vbox_req();

    memset(req, 0, sizeof(*req));
    req->header.size = (uint32_t)sizeof(*req);
    req->header.version = VBOX_REQUEST_HEADER_VERSION;
    req->header.request_type = VMMDEVREQ_GET_MOUSE_STATUS;
    req->header.rc = -1;
    vbox_submit();

    if (req->header.rc < 0) {
        return 0;   // request refused: nothing to read
    }
    // Trust the position only while the host wants absolute mode.
    if (!(req->mouse_features & VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE)) {
        return 0;
    }

    // Each axis spans 0..0xFFFF; read the screen size once.
    uint32_t screen_w = current_width;
    uint32_t screen_h = current_height;
    uint32_t raw_x = (uint32_t)req->pointer_x_pos & 0xFFFFu;
    uint32_t raw_y = (uint32_t)req->pointer_y_pos & 0xFFFFu;

    int32_t x = (int32_t)((raw_x * screen_w) / 0xFFFFu);
    int32_t y = (int32_t)((raw_y * screen_h) / 0xFFFFu);

    mouse_set_position(x, y);
    return 1;
}

int vboxguest_present(void) {
    return vbox_present;
}
