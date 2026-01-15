#ifndef STRING_H
#define STRING_H

#include "stdint.h"

void* memcpy(void* dest, const void* src, size_t count);
void* memset(void* dest, int val, size_t count);
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
int strcmp(const char* str1, const char* str2);
char* strncpy(char* dest, const char* src, size_t count);

#endif