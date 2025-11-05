#include "../../intf/string.h"

void* memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    char* src8 = (char*)src;
    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
}

void* memset(void* dest, int val, size_t count) {
    char* temp = (char*)dest;
    while (count--) {
        *temp++ = (char)val;
    }
    return dest;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

char* strncpy(char* dest, const char* src, size_t count) {
    char* d = dest;
    const char* s = src;
    while (count-- > 0 && *s != '\0') {
        *d++ = *s++;
    }
    while (count-- > 0) {
        *d++ = '\0';
    }
    return dest;
}