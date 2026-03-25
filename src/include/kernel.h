#ifndef KERNEL_H
#define KERNEL_H

// Kernel entry point
struct multiboot_info;
void kernel_main(struct multiboot_info* mb_info);

// Basic I/O functions
void print(const char* str);
void print_char(char c);
void clear_screen(void);

#endif



