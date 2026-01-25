#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "stdint.h"

void keyboard_init();
char keyboard_read_char();
int keyboard_has_char();

#endif
