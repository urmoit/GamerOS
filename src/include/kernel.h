#ifndef KERNEL_H
#define KERNEL_H

struct multiboot_info;
void kernel_main(struct multiboot_info* mb_info);

void print(const char* str);
void print_char(char c);
void clear_screen(void);

#endif



