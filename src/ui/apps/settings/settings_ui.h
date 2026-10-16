#ifndef GAMEROS_SETTINGS_UI_H
#define GAMEROS_SETTINGS_UI_H

// Settings app: shell-facing API and shared page indices.

#include <stdint.h>

#define SETTINGS_TAB_SYSTEM 0
#define SETTINGS_TAB_BLUETOOTH_DEVICES 1
#define SETTINGS_TAB_NETWORK_INTERNET 2
#define SETTINGS_TAB_PERSONALIZATION 3
#define SETTINGS_TAB_CUSTOMIZATION 4
#define SETTINGS_TAB_TASKBAR 5
#define SETTINGS_TAB_APPS 6
#define SETTINGS_TAB_ACCOUNTS 7
#define SETTINGS_TAB_TIME_LANGUAGE 8
#define SETTINGS_TAB_GAMING 9
#define SETTINGS_TAB_ACCESSIBILITY 10
#define SETTINGS_TAB_PRIVACY_SECURITY 11
#define SETTINGS_TAB_GAMEROS_UPDATE 12
#define SETTINGS_TAB_COUNT 13

// Taskbar shapes the shell applies.
#define TASKBAR_STYLE_WINDOWS     0   // Full-width bar flush with the bottom edge.
#define TASKBAR_STYLE_MACOS_DOCK  1   // Centred floating panel.

const char* settings_app_title(void);

void settings_app_draw_content(int x, int y, int w, int h, int is_active);

void settings_app_draw_icon(int x, int y);

// Input. Each returns non-zero when it consumed the event.
int settings_app_handle_click(int mx, int my);
int settings_app_handle_wheel(int mx, int my, int8_t wheel_delta);

void settings_app_select_tab(int tab_idx);

void settings_app_reset_scroll(void);

void settings_app_reload_changelog(void);

int settings_app_show_seconds(void);
int settings_app_taskbar_compact(void);
int settings_app_taskbar_style(void);

const char* settings_app_changelog_path(void);
const char* settings_app_changelog_text(void);

#endif
