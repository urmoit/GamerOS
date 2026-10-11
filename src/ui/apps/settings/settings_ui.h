#ifndef GAMEROS_SETTINGS_UI_H
#define GAMEROS_SETTINGS_UI_H

// Settings, packaged as a standalone app rather than part of the kernel image.
//
// The shell (window manager, taskbar, desktop, input pump) lives in the kernel
// and calls into this module; this module owns every Settings page, all of its
// state and the changelog it renders. It reaches the kernel only through
// src/ui/shell/shell_api.h plus the driver headers it includes directly, the
// same contract Notepad follows.

#include <stdint.h>

// Page indices. The shell needs SETTINGS_TAB_GAMEROS_UPDATE so the Start menu
// can open the changelog directly.
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

// Taskbar shapes the shell applies. The default is the floating dock, which is
// how the shell looked before the style was configurable.
#define TASKBAR_STYLE_WINDOWS     0   // Full-width bar flush with the bottom edge.
#define TASKBAR_STYLE_MACOS_DOCK  1   // Centred floating panel.

// Title shown in the window frame and on the taskbar.
const char* settings_app_title(void);

// Draw the navigation rail and the active page into the given rect.
void settings_app_draw_content(int x, int y, int w, int h, int is_active);

// Desktop icon.
void settings_app_draw_icon(int x, int y);

// Input. Each returns non-zero when it consumed the event.
int settings_app_handle_click(int mx, int my);
int settings_app_handle_wheel(int mx, int my, int8_t wheel_delta);

// Open a specific page (used when launching Settings from the Start menu).
void settings_app_select_tab(int tab_idx);

// Forget the per-page scroll offsets; called when a Settings window opens.
void settings_app_reset_scroll(void);

// Drop the parsed changelog so it is read again from storage once it is up.
void settings_app_reload_changelog(void);

// State the shell reads so its own surfaces follow the Settings choices.
int settings_app_show_seconds(void);
int settings_app_taskbar_compact(void);
int settings_app_taskbar_style(void);

// The changelog the GamerOS Update page renders, and the boot-time file copy.
const char* settings_app_changelog_path(void);
const char* settings_app_changelog_text(void);

#endif
