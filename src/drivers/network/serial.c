#include "serial.h"
#include "ports.h"

#define SERIAL_PORT 0x3F8

// Every wait below is bounded. On machines without a real UART (or when the
// port is not wired to anything) the LSR never reports readiness, and an
// unbounded spin would hang the kernel before it ever reaches the shell.
#define SERIAL_WAIT_LIMIT 100000

void serial_init(void) {
    outb(SERIAL_PORT + 1, 0x00); // Disable interrupts
    outb(SERIAL_PORT + 3, 0x80); // Enable DLAB
    outb(SERIAL_PORT + 0, 0x03); // Divisor = 3 (38400 baud)
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7); // Enable FIFO
    outb(SERIAL_PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set
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
    return 0;
}

int serial_is_transmit_empty(void) {
    return inb(SERIAL_PORT + 5) & 0x20;
}

void serial_putchar(char c) {
    // Give up rather than block forever if the transmitter never drains.
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