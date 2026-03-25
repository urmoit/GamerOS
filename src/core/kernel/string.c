#include <string.h>

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

size_t strlen(const char* str) {
    if (!str) return 0; // NULL check
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    if (!dest || !src) return dest; // NULL checks
    char* d = dest;
    while ((*d++ = *src++));
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

int strncmp(const char* str1, const char* str2, size_t count) {
    if (count == 0) return 0;
    if (!str1 || !str2) return str1 ? 1 : (str2 ? -1 : 0);
    for (size_t i = 0; i < count; i++) {
        unsigned char c1 = (unsigned char)str1[i];
        unsigned char c2 = (unsigned char)str2[i];
        if (c1 != c2) return (int)c1 - (int)c2;
        if (c1 == '\0') return 0;
    }
    return 0;
}

char* strstr(const char* haystack, const char* needle) {
    if (!haystack || !needle) return 0;
    if (*needle == '\0') return (char*)haystack;
    size_t needle_len = strlen(needle);
    if (needle_len == 0) return (char*)haystack;

    for (size_t i = 0; haystack[i] != '\0'; i++) {
        if (haystack[i] == needle[0]) {
            if (strncmp(&haystack[i], needle, needle_len) == 0) {
                return (char*)&haystack[i];
            }
        }
    }
    return 0;
}

static int append_char(char** out, char c) {
    **out = c;
    (*out)++;
    return 1;
}

static int append_cstr(char** out, const char* s) {
    int n = 0;
    if (!s) s = "(null)";
    while (*s) {
        **out = *s++;
        (*out)++;
        n++;
    }
    return n;
}

static int append_uint(char** out, uint32_t v, uint32_t base, int uppercase) {
    char tmp[16];
    int idx = 0;
    int n = 0;
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (base < 2 || base > 16) return 0;
    if (v == 0) {
        **out = '0';
        (*out)++;
        return 1;
    }

    while (v > 0 && idx < (int)sizeof(tmp)) {
        tmp[idx++] = digits[v % base];
        v /= base;
    }
    while (idx > 0) {
        **out = tmp[--idx];
        (*out)++;
        n++;
    }
    return n;
}

int sprintf(char* dest, const char* fmt, ...) {
    if (!dest || !fmt) return 0;

    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    char* out = dest;
    int written = 0;

    while (*fmt) {
        if (*fmt != '%') {
            written += append_char(&out, *fmt++);
            continue;
        }
        fmt++;
        if (*fmt == '\0') break;

        switch (*fmt) {
            case '%':
                written += append_char(&out, '%');
                break;
            case 'c': {
                int ch = __builtin_va_arg(args, int);
                written += append_char(&out, (char)ch);
                break;
            }
            case 's': {
                const char* s = __builtin_va_arg(args, const char*);
                written += append_cstr(&out, s);
                break;
            }
            case 'd':
            case 'i': {
                int v = __builtin_va_arg(args, int);
                if (v < 0) {
                    written += append_char(&out, '-');
                    written += append_uint(&out, (uint32_t)(-v), 10, 0);
                } else {
                    written += append_uint(&out, (uint32_t)v, 10, 0);
                }
                break;
            }
            case 'u': {
                uint32_t v = __builtin_va_arg(args, uint32_t);
                written += append_uint(&out, v, 10, 0);
                break;
            }
            case 'x': {
                uint32_t v = __builtin_va_arg(args, uint32_t);
                written += append_uint(&out, v, 16, 0);
                break;
            }
            case 'X': {
                uint32_t v = __builtin_va_arg(args, uint32_t);
                written += append_uint(&out, v, 16, 1);
                break;
            }
            default:
                written += append_char(&out, '%');
                written += append_char(&out, *fmt);
                break;
        }
        fmt++;
    }

    *out = '\0';
    __builtin_va_end(args);
    return written;
}

// TODO: Implement memory-safe string operations
// TODO: Add wide character string support
