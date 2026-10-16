#include "serial.h"
#include "ports.h"

#define SERIAL_PORT 0x3F8

// Bound the waits: without a real UART the LSR never reports ready.
#define SERIAL_WAIT_LIMIT 100000

void serial_init(void) {
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x80); // DLAB on
    outb(SERIAL_PORT + 0, 0x03); // Divisor = 3 (38400 baud)
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7); // FIFO on
    outb(SERIAL_PORT + 4, 0x0B); // IRQs on, RTS/DSR set
}

int serial_received(void) {
    return inb(SERIAL_PORT + 5) & 1;
}

char serial_read(void) {
    for (int i = 0; i < SERIAL_WAIT_LIMIT; i++) {
        if (serial_received()) {
            return inb(SERIAL_PORT);
        }
    }
    // FIXME: No real UART present; serial_read returns 0 on timeout.
    return 0;
}

int serial_is_transmit_empty(void) {
    return inb(SERIAL_PORT + 5) & 0x20;
}

void serial_putchar(char c) {
    for (int i = 0; i < SERIAL_WAIT_LIMIT; i++) {
        if (serial_is_transmit_empty()) {
            outb(SERIAL_PORT, c);
            return;
        }
    }
}

void serial_write_string(const char* str) {
    if (!str) return;
    while (*str) {
        serial_putchar(*str);
        str++;
    }
}