#ifndef USB_H
#define USB_H

#include "stdint.h"

// USB Requests
typedef struct {
    uint8_t  request_type;
    uint8_t  request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} __attribute__((packed)) usb_setup_packet_t;

#define USB_REQ_GET_DESCRIPTOR 0x06
#define USB_DEVICE_DESCRIPTOR 0x01

// USB Speed
typedef enum {
    USB_SPEED_LOW,
    USB_SPEED_FULL,
    USB_SPEED_HIGH,
    USB_SPEED_SUPER
} usb_speed_t;

// USB Device Descriptor
typedef struct {
    uint8_t  length;
    uint8_t  descriptor_type;
    uint16_t usb_version;
    uint8_t  device_class;
    uint8_t  device_subclass;
    uint8_t  device_protocol;
    uint8_t  max_packet_size;
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t device_version;
    uint8_t  manufacturer_string_idx;
    uint8_t  product_string_idx;
    uint8_t  serial_number_string_idx;
    uint8_t  num_configurations;
} __attribute__((packed)) usb_device_descriptor_t;

typedef struct usb_device {
    uint8_t address;
    usb_speed_t speed;
    usb_device_descriptor_t descriptor;
    void* controller_data;
    struct usb_device* next;
} usb_device_t;

void usb_init(void);
void usb_handle_device(usb_device_t* dev);

#endif
