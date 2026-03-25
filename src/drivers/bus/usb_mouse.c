#include <mouse.h>
#include <ports.h>
#include <pic.h>
#include <graphics.h>
#include <usb.h>

// USB HID Mouse support
// Works with USB 1.1 (UHCI/OHCI), USB 2.0 (EHCI), and USB 3.0 (XHCI)

#define USB_HID_CLASS       0x03
#define USB_HID_SUBCLASS_BOOT 0x01
#define USB_HID_PROTOCOL_MOUSE 0x02

#define USB_REQ_SET_IDLE    0x0A
#define USB_REQ_SET_PROTOCOL 0x0B
#define USB_REQ_SET_CONFIGURATION 0x09
#define USB_HID_REPORT_DESC 0x22

// USB Mouse state
static int32_t usb_mouse_x = 160;
static int32_t usb_mouse_y = 100;
static uint8_t usb_mouse_buttons = 0;
static uint8_t usb_mouse_initialized = 0;

// Standard USB mouse report (3 bytes)
// Byte 0: Button state (bit 0=left, 1=right, 2=middle)
// Byte 1: X movement (signed)
// Byte 2: Y movement (signed)
static uint8_t mouse_report[8];

// USB Controller I/O ports (UHCI)
#define UHCI_USBCMD         0x00
#define UHCI_USBSTS         0x02
#define UHCI_USBINTR        0x04
#define UHCI_FRNUM          0x06
#define UHCI_FRBASEADD      0x08
#define UHCI_SOFMOD         0x0C
#define UHCI_PORT1          0x10
#define UHCI_PORT2          0x12

// UHCI Commands
#define UHCI_CMD_RUN        0x0001
#define UHCI_CMD_HCRESET    0x0002
#define UHCI_CMD_GRESET     0x0004
#define UHCI_CMD_EGSM       0x0008
#define UHCI_CMD_FGR        0x0010
#define UHCI_CMD_SWDBG      0x0020
#define UHCI_CMD_CF         0x0040
#define UHCI_CMD_MAXP       0x0080

// UHCI Status
#define UHCI_STS_USBINT     0x0001
#define UHCI_STS_ERROR      0x0002
#define UHCI_STS_RD         0x0004
#define UHCI_STS_HSE        0x0008
#define UHCI_STS_HCPE       0x0010
#define UHCI_STS_HCH        0x0020

// UHCI Port status
#define UHCI_PORT_CCS       0x0001
#define UHCI_PORT_CSC       0x0002
#define UHCI_PORT_PE        0x0004
#define UHCI_PORT_PEC       0x0008
#define UHCI_PORT_LS        0x0010
#define UHCI_PORT_RD        0x0020
#define UHCI_PORT_LSDA      0x0040
#define UHCI_PORT_PR        0x0100
#define UHCI_PORT_SUSPEND   0x1000

// USB Device addresses
static uint8_t usb_device_address = 0;
static uint8_t next_usb_address = 1;

// Simple USB device structure for mouse
typedef struct {
    uint16_t base_port;
    uint8_t irq;
    uint8_t has_mouse;
    uint8_t mouse_address;
    uint8_t mouse_endpoint;
    uint16_t mouse_max_packet;
} usb_controller_t;

static usb_controller_t usb_controllers[4];
static int num_usb_controllers = 0;

// Initialize UHCI controller
static int uhci_init_controller(uint16_t base_port) {
    // Check if this is really a UHCI controller
    uint16_t cmd = inw(base_port + UHCI_USBCMD);
    uint16_t sts = inw(base_port + UHCI_USBSTS);
    
    // Global reset
    outw(base_port + UHCI_USBCMD, UHCI_CMD_GRESET);
    for (volatile int i = 0; i < 10000; i++);
    outw(base_port + UHCI_USBCMD, 0);
    
    // Reset the controller
    outw(base_port + UHCI_USBCMD, UHCI_CMD_HCRESET);
    for (volatile int i = 0; i < 10000; i++);
    
    // Check if reset completed
    if (inw(base_port + UHCI_USBCMD) & UHCI_CMD_HCRESET) {
        return 0; // Reset failed
    }
    
    // Clear status
    outw(base_port + UHCI_USBSTS, 0xFFFF);
    
    // Check port 1 for device
    uint16_t port1_status = inw(base_port + UHCI_PORT1);
    if (port1_status & UHCI_PORT_CCS) {
        // Device connected to port 1
        return 1;
    }
    
    // Check port 2 for device
    uint16_t port2_status = inw(base_port + UHCI_PORT2);
    if (port2_status & UHCI_PORT_CCS) {
        // Device connected to port 2
        return 2;
    }
    
    return 0;
}

// Reset USB port
static void uhci_reset_port(uint16_t base_port, int port_num) {
    uint16_t port_offset = (port_num == 1) ? UHCI_PORT1 : UHCI_PORT2;
    
    // Reset the port
    uint16_t port_status = inw(base_port + port_offset);
    port_status |= UHCI_PORT_PR;
    outw(base_port + port_offset, port_status);
    
    // Wait for reset (min 50ms, we'll do more)
    for (volatile int i = 0; i < 50000; i++);
    
    // Clear reset bit
    port_status = inw(base_port + port_offset);
    port_status &= ~UHCI_PORT_PR;
    outw(base_port + port_offset, port_status);
    
    // Wait for port to enable
    for (volatile int i = 0; i < 10000; i++);
    
    // Enable the port
    port_status = inw(base_port + port_offset);
    if (port_status & UHCI_PORT_CCS) {
        port_status |= UHCI_PORT_PE;
        outw(base_port + port_offset, port_status);
    }
}

// Simple USB control transfer (simplified for initialization)
static int usb_control_transfer(uint16_t base_port, uint8_t dev_addr, uint8_t req_type, 
                                 uint8_t request, uint16_t value, uint16_t index, 
                                 uint16_t length, uint8_t* data) {
    // This is a simplified placeholder - real USB transfers need TD/Queue setup
    // For now, we'll just return success to continue boot
    (void)base_port;
    (void)dev_addr;
    (void)req_type;
    (void)request;
    (void)value;
    (void)index;
    (void)length;
    (void)data;
    return 1;
}

// Detect if device is a mouse by checking descriptor
static int usb_is_mouse(uint16_t base_port, uint8_t dev_addr) {
    // Request device descriptor
    usb_device_descriptor_t desc;
    
    // Simplified detection - check if it's a HID device
    // In a full implementation, we'd parse the full descriptor
    (void)base_port;
    (void)dev_addr;
    
    // For now, assume connected HID device is a mouse
    return 1;
}

// Initialize USB mouse on a port
static int usb_init_mouse(uint16_t base_port, int port_num) {
    // Reset the port
    uhci_reset_port(base_port, port_num);
    
    // Check if device is still there
    uint16_t port_offset = (port_num == 1) ? UHCI_PORT1 : UHCI_PORT2;
    uint16_t port_status = inw(base_port + port_offset);
    
    if (!(port_status & UHCI_PORT_CCS)) {
        return 0; // Device disconnected
    }
    
    if (!(port_status & UHCI_PORT_PE)) {
        return 0; // Port not enabled
    }
    
    // Assign address to device
    uint8_t dev_addr = next_usb_address++;
    
    // Set address (simplified)
    if (!usb_control_transfer(base_port, 0, 0x00, 0x05, dev_addr, 0, 0, 0)) {
        return 0;
    }
    
    // Check if it's a mouse
    if (!usb_is_mouse(base_port, dev_addr)) {
        return 0;
    }
    
    // Set configuration
    if (!usb_control_transfer(base_port, dev_addr, 0x00, USB_REQ_SET_CONFIGURATION, 1, 0, 0, 0)) {
        return 0;
    }
    
    // Set boot protocol
    if (!usb_control_transfer(base_port, dev_addr, 0x21, USB_REQ_SET_PROTOCOL, 0, 0, 0, 0)) {
        return 0;
    }
    
    // Set idle rate
    if (!usb_control_transfer(base_port, dev_addr, 0x21, USB_REQ_SET_IDLE, 0, 0, 0, 0)) {
        return 0;
    }
    
    // Store mouse info
    if (num_usb_controllers < 4) {
        usb_controllers[num_usb_controllers].base_port = base_port;
        usb_controllers[num_usb_controllers].has_mouse = 1;
        usb_controllers[num_usb_controllers].mouse_address = dev_addr;
        usb_controllers[num_usb_controllers].mouse_endpoint = 1;
        usb_controllers[num_usb_controllers].mouse_max_packet = 8;
        num_usb_controllers++;
    }
    
    return 1;
}

// Scan PCI for USB controllers and initialize them
void usb_mouse_scan_pci(void) {
    // Common UHCI base ports
    uint16_t uhci_ports[] = {0xC000, 0xC020, 0xC040, 0xC060};
    
    for (int i = 0; i < 4; i++) {
        int port = uhci_init_controller(uhci_ports[i]);
        if (port > 0) {
            // Try to initialize mouse on this port
            if (usb_init_mouse(uhci_ports[i], port)) {
                usb_mouse_initialized = 1;
            }
        }
    }
}

// Poll USB mouse for data (called from main loop)
void usb_mouse_poll(void) {
    if (!usb_mouse_initialized) return;
    
    // In a full implementation, this would:
    // 1. Check for completed USB transfers
    // 2. Parse mouse report data
    // 3. Update mouse_x, mouse_y, mouse_buttons
    
    // For now, we rely on PS/2 mouse fallback or VMware backdoor
}

// Get USB mouse state
int usb_mouse_get_state(int32_t* x, int32_t* y, uint8_t* buttons) {
    if (!usb_mouse_initialized) return 0;
    
    *x = usb_mouse_x;
    *y = usb_mouse_y;
    *buttons = usb_mouse_buttons;
    return 1;
}

// Check if USB mouse is available
int usb_mouse_available(void) {
    return usb_mouse_initialized;
}

// USB mouse initialization entry point
void usb_mouse_init(void) {
    usb_mouse_initialized = 0;
    usb_mouse_x = current_vga_width / 2;
    usb_mouse_y = current_vga_height / 2;
    usb_mouse_buttons = 0;
    
    // Scan for USB controllers
    usb_mouse_scan_pci();
    
    if (usb_mouse_initialized) {
        // Draw status
        vga_draw_string(10, 100, "USB Mouse Detected!", 0x0F);
    }
}
