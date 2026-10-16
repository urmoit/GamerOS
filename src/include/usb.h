#ifndef USB_H
#define USB_H

#include "stdint.h"

// USB standard requests.
#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_CLEAR_FEATURE     0x01
#define USB_REQ_SET_FEATURE       0x02
#define USB_REQ_SET_ADDRESS       0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_DESCRIPTOR    0x07
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09

// USB class requests (HID).
#define USB_REQ_GET_REPORT        0x01
#define USB_REQ_SET_IDLE          0x0A
#define USB_REQ_SET_PROTOCOL      0x0B

// Descriptor types.
#define USB_DESC_DEVICE          0x01
#define USB_DESC_CONFIGURATION   0x02
#define USB_DESC_STRING          0x03
#define USB_DESC_INTERFACE       0x04
#define USB_DESC_ENDPOINT        0x05

// HID descriptor types.
#define USB_DESC_HID             0x21
#define USB_DESC_HID_REPORT      0x22

// Classes, subclasses and the boot protocol identifiers.
#define USB_CLASS_HID            0x03
#define USB_SUBCLASS_HID_BOOT    0x01
#define USB_PROTOCOL_HID_KEYBOARD 0x01
#define USB_PROTOCOL_HID_MOUSE   0x02

// Endpoint transfer types.
#define USB_EP_TYPE_CONTROL       0x00
#define USB_EP_TYPE_ISOCHRONOUS   0x01
#define USB_EP_TYPE_BULK          0x02
#define USB_EP_TYPE_INTERRUPT     0x03

// HID boot protocol report sizes.
#define USB_BOOT_KEYBOARD_REPORT 8
#define USB_BOOT_MOUSE_REPORT    4

typedef struct {
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} __attribute__((packed)) usb_setup_packet_t;

typedef struct {
    uint8_t length;
    uint8_t descriptor_type;
    uint16_t usb_version;
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    uint8_t max_packet_size0;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t device_version;
    uint8_t manufacturer_string_idx;
    uint8_t product_string_idx;
    uint8_t serial_number_string_idx;
    uint8_t num_configurations;
} __attribute__((packed)) usb_device_descriptor_t;

typedef struct {
    uint8_t length;
    uint8_t descriptor_type;
    uint16_t total_length;
    uint8_t num_interfaces;
    uint8_t configuration_value;
    uint8_t configuration_str;
    uint8_t attributes;
    uint8_t max_power;
} __attribute__((packed)) usb_config_descriptor_t;

typedef struct {
    uint8_t length;
    uint8_t descriptor_type;
    uint8_t interface_number;
    uint8_t alternate_setting;
    uint8_t num_endpoints;
    uint8_t interface_class;
    uint8_t interface_subclass;
    uint8_t interface_protocol;
    uint8_t interface_str;
} __attribute__((packed)) usb_interface_descriptor_t;

typedef struct {
    uint8_t length;
    uint8_t descriptor_type;
    uint8_t endpoint_address;
    uint8_t attributes;
    uint16_t max_packet_size;
    uint8_t interval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

void usb_init(void);

void usb_poll(void);

int usb_keyboard_connected(void);
int usb_mouse_connected(void);

#endif
