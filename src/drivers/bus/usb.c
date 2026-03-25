#include "../../intf/usb.h"
#include "../../intf/graphics.h"

static usb_device_t* devices = 0;

void usb_handle_device(usb_device_t* dev) {
    // 1. Log device detection
    char log[128];
    vga_draw_string(10, 50, "USB Device Detected!", XP_COLOR_WHITE);
    
    // 2. Identify class
    if (dev->descriptor.device_class == 0x00) {
        // Special case: check interface descriptors (Not implemented yet)
    }

    if (dev->descriptor.device_class == 0x09) {
         vga_draw_string(10, 70, "Type: USB Hub", XP_COLOR_WHITE);
    } else if (dev->descriptor.device_class == 0x03) {
         vga_draw_string(10, 70, "Type: HID (Keyboard/Mouse)", XP_COLOR_WHITE);
    }
}

void usb_init(void) {
    // High level initialization
}
