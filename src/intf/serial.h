#ifndef SERIAL_H
#define SERIAL_H

void serial_init(void);
int serial_received(void);
char serial_read(void);
int serial_is_transmit_empty(void);
void serial_putchar(char c);
void serial_write_string(const char* str);

#endif
