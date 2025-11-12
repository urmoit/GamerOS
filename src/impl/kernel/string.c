7wasdw#include "../../intf/string.h"

void* memcpy(void* dest, const void* src, size_t count) {
    if (!dest || !src || count == 0) return dest; // NULL and zero checks
    char* dst8 = (char*)dest;
    char* src8 = (char*)src;
    // Use size_t for loop counter to avoid signed/unsigned comparison warnings
    size_t i = 0;
    while (i < count) {
        dst8[i] = src8[i];
        i++;
    }
    return dest;
}

void* memset(void* dest, int val, size_t count) {
    if (!dest || count == 0) return dest; // NULL and zero checks
    char* temp = (char*)dest;
    // Use size_t for loop counter to avoid signed/unsigned comparison warnings
    size_t i = 0;
    while (i < count) {
        temp[i] = (char)val;
        i++;
    }
    return dest;
}

int strcmp(const char* str1, const char* str2) {
    if (!str1 || !str2) return str1 ? 1 : (str2 ? -1 : 0); // NULL checks
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

char* strncpy(char* dest, const char* src, size_t count) {
    if (!dest || !src || count == 0) return dest; // NULL and zero checks
    char* d = dest;
    const char* s = src;
    // Use size_t for loop counter to avoid signed/unsigned comparison warnings
    size_t i = 0;
    while (i < count && *s != '\0') {
        *d++ = *s++;
        i++;
    }
    while (i < count) {
        *d++ = '\0';
        i++;
    }
    return dest;
}