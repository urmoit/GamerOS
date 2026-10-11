#ifndef SHELL_API_H
#define SHELL_API_H

// The interface between standalone apps and the kernel shell.
//
// Apps are separate translation units: they must not reach into the kernel's
// internals, so everything they legitimately need from the shell is declared
// here. The shell (window manager, taskbar, desktop, input pump) stays in
// main.c and implements these.

#include <stdint.h>
#include <fs.h>

#include <graphics.h>

// ---- Shell metrics ----------------------------------------------------------
// Text is the canonical 8x8 font at its natural advance.
#define FONT_CW 8
#define FONT_CH 8
#define TASKBAR_HEIGHT 20
#define DESKTOP_ICON_W   20
#define DESKTOP_ICON_H   20
#define DESKTOP_ICON_HIT_W 28
#define DESKTOP_ICON_HIT_H 32

// ---- Text and shape primitives ---------------------------------------------
void draw_compact_string(int x, int y, const char* str, uint8_t color);
void draw_compact_string_clipped(int x, int y, int max_chars, const char* str, uint8_t color);
int  compact_text_width(const char* str);
void append_string(char* dest, size_t dest_cap, const char* src);
void append_text(char* out, int out_size, int* pos, const char* s);
void append_unsigned(char* out, int out_size, int* pos, uint64_t v);
void fill_chamfer_rect(int x, int y, int w, int h, uint8_t color);
void draw_chamfer_rect(int x, int y, int w, int h, uint8_t color);
void draw_tool_button(int x, int y, int w, int h, const char* label, int enabled);

// ---- Shell state an app configures -----------------------------------------
// Debug overlay, shown by the Settings System page's "Debug window" toggle.
int  shell_debug_overlay_enabled(void);
void shell_set_debug_overlay(int enabled);

// Current desktop UI scale, as a whole multiplier.
int  shell_ui_scale(void);

// Display profiles the Settings System page lists and applies.
int         shell_display_profile_count(void);
int         shell_display_profile_available(int idx);
const char* shell_display_profile_label(int idx);
int         shell_display_profile_current(void);
void        shell_set_display_profile(int idx);

// ---- Paths ------------------------------------------------------------------
const char* path_basename(const char* path);
int  path_has_extension(const char* path, const char* ext);

// ---- Storage ----------------------------------------------------------------
// Apps address user data through GOS: paths rather than raw C:/ paths.
void     shell_ensure_storage_initialized(void);
uint8_t  shell_storage_ready(void);
file_t*  shell_fs_open(const char* gos_path);
file_t*  shell_fs_create(const char* gos_path);

// ---- Shell services ---------------------------------------------------------
void shell_raise_error(const char* title, const char* message);

#endif
