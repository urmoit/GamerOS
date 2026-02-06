#ifndef KEYBOARD_H
#define KEYBOARD_H

// Special key codes
#define KEY_LEFT    0x80
#define KEY_RIGHT   0x81
#define KEY_UP      0x82
#define KEY_DOWN    0x83
#define KEY_HOME    0x84
#define KEY_END     0x85
#define KEY_PGUP    0x86
#define KEY_PGDN    0x87
#define KEY_INS     0x88
#define KEY_DEL     0x89

void keyboard_init(void);
void keyboard_handler(void);
void keyboard_poll(void);
char keyboard_getchar(void);
int keyboard_has_input(void);

#endif
