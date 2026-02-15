#ifndef STRING_H
#define STRING_H

#include "stdint.h"

void* memcpy(void* dest, const void* src, size_t count);
void* memset(void* dest, int val, size_t count);
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
int strcmp(const char* str1, const char* str2);
char* strncpy(char* dest, const char* src, size_t count);
int strncmp(const char* str1, const char* str2, size_t count);
char* strstr(const char* haystack, const char* needle);
int sprintf(char* dest, const char* fmt, ...);

// TODO: Add bounded formatting API (`snprintf`) for safer kernel string formatting.
// TODO: Add explicit integer conversion helpers (itoa/utoa) for non-format code paths.

#endif
