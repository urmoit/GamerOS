#include <settings_ui.h>

#include <graphics.h>
#include <mouse.h>
#include <fs.h>
#include <rtc.h>
#include <serial.h>
#include <string.h>
#include <timezone.h>
#include <markdown.h>
#include <shell_api.h>

// ---------------------------------------------------------------------------
// State
//
// Every setting the app exposes lives here. The shell reads what it needs
// through the accessors at the bottom of this file (settings_app_show_seconds,
// settings_app_taskbar_*), so the taskbar and the clock follow these choices
// without either side reaching into the other's statics.
// ---------------------------------------------------------------------------

static uint8_t setting_show_seconds = 0;
static uint8_t setting_compact_mode = 1;
static int setting_taskbar_style = TASKBAR_STYLE_MACOS_DOCK;
static int settings_tab = 0;

// Toggles exposed on the non-System pages.
static uint8_t setting_bluetooth = 0;
static uint8_t setting_network = 0;
static uint8_t setting_high_contrast = 0;
static uint8_t setting_large_text = 0;

static int settings_scroll_top[SETTINGS_TAB_COUNT] = {0};
// The changelog is parsed and drawn by src/ui/markdown/markdown.c.
static uint8_t settings_md_loaded = 0;

typedef enum {
    SETTINGS_UPDATE_VIEW_CHANGELOG = 0,
    SETTINGS_UPDATE_VIEW_ROADMAP = 1
} settings_update_view_t;

static int settings_update_view = SETTINGS_UPDATE_VIEW_CHANGELOG;

// Hit-box state published by settings_app_draw_content() so the click handler
// tests the exact rectangles that were rendered, instead of duplicating the
// layout constants.
#define SETTINGS_MAX_TOGGLES 8

typedef enum {
    SETTINGS_TOG_COMPACT = 0,
    SETTINGS_TOG_SECONDS,
    SETTINGS_TOG_DEBUG,
    SETTINGS_TOG_BLUETOOTH,
    SETTINGS_TOG_NETWORK,
    SETTINGS_TOG_CONTRAST,
    SETTINGS_TOG_LARGE_TEXT
} settings_toggle_id_t;

typedef struct {
    const char* label;
    int is_on;
    int id;
} settings_toggle_desc_t;

static int settings_tog_count = 0;
static int settings_tog_hit_x = 0;
static int settings_tog_hit_w = 0;
static int settings_tog_hit_y[SETTINGS_MAX_TOGGLES];
static int settings_tog_id[SETTINGS_MAX_TOGGLES];

static int settings_res_hit_x = 0;
static int settings_res_hit_y[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static int settings_res_hit_count = 0;
static int settings_res_hit_h = 0;

// Sub-button hit boxes for the GamerOS Update view switch.
static int settings_view_hit_x = 0;
static int settings_view_hit_y = 0;
static int settings_view_hit_w = 0;
static int settings_view_hit_h = 0;
static int settings_view_hit_gap = 0;

// Hit boxes for the Customization page's rows, which open another page the way
// the Windows Settings sub-pages do.
#define SETTINGS_MAX_NAV_ROWS 4
static int settings_nav_hit_x = 0;
static int settings_nav_hit_w = 0;
static int settings_nav_hit_y[SETTINGS_MAX_NAV_ROWS];
static int settings_nav_hit_tab[SETTINGS_MAX_NAV_ROWS];
static int settings_nav_hit_count = 0;

// Geometry of the pane and its two columns, recorded by the renderer so the
// click and wheel handlers test the rectangles that are actually on screen.
static int settings_pane_x = 0;
static int settings_pane_y = 0;
static int settings_pane_w = 0;
static int settings_pane_h = 0;
static int settings_nav_x = 0;
static int settings_nav_y = 0;
static int settings_nav_w = 124;
static int settings_content_x = 0;
static int settings_content_y = 0;
static int settings_content_w = 0;
static int settings_content_h = 0;

// ---------------------------------------------------------------------------
// Page index
// ---------------------------------------------------------------------------

const char* settings_app_title(void) {
    return "Settings";
}

static const char* settings_ui_tab_name(int tab_idx) {
    static const char* tabs[SETTINGS_TAB_COUNT] = {
        "System",
        "Devices",
        "Network",
        "Personalization",
        "Customization",
        "Taskbar",
        "Apps",
        "Accounts",
        "Time & Language",
        "Gaming",
        "Accessibility",
        "Privacy",
        "GamerOS Update"
    };
    if (tab_idx < 0 || tab_idx >= SETTINGS_TAB_COUNT) return "Unknown";
    return tabs[tab_idx];
}

// ---------------------------------------------------------------------------
// Row primitives
// ---------------------------------------------------------------------------

// Draw a horizontal on/off switch.
//
// Renders a chamfered pill with a sliding knob: the track turns accent blue when
// enabled, and the knob moves from the left (off) to the right (on).
static void draw_toggle_switch(int x, int y, int is_on) {
    const int tw = 20;
    const int th = 9;
    if (x < 0 || y < 0) return;

    // Track.
    uint8_t track = is_on ? XP_LBLUE : XP_LGRAY;
    fill_chamfer_rect(x, y, tw, th, track);
    draw_chamfer_rect(x, y, tw, th, XP_DGRAY);
    // Highlight along the top edge for a slightly raised look.
    fill_rect(x + 1, y + 1, tw - 2, 1, is_on ? XP_LBLUE : XP_WHITE);

    // Knob slides across the track.
    const int knob_w = 7;
    const int knob_h = 5;
    const int inset = 2;
    int knob_x = is_on ? (x + tw - knob_w - inset) : (x + inset);
    int knob_y = y + (th - knob_h) / 2;
    fill_chamfer_rect(knob_x, knob_y, knob_w, knob_h, XP_WHITE);
    draw_chamfer_rect(knob_x, knob_y, knob_w, knob_h, XP_DGRAY);
}

// Draw one labelled settings toggle row: label on the left, state text and a
// right-aligned switch on the right. Returns the height consumed.
static int draw_settings_toggle(int content_x, int content_w, int row_y,
                                const char* label, int is_on, int max_chars) {
    const int toggle_w = 20;
    // Right-align the switch inside the content pane.
    int toggle_x = content_x + content_w - toggle_w - 6;
    if (toggle_x < content_x) toggle_x = content_x;

    draw_compact_string_clipped(content_x + 6, row_y + 1, max_chars, label, XP_BLACK);

    // State text sits just left of the switch.
    const char* state = is_on ? "On" : "Off";
    int state_x = toggle_x - 6 - ((int)strlen(state) * FONT_CW);
    if (state_x > content_x + 6) {
        draw_compact_string(state_x, row_y + 1, state, is_on ? XP_BLUE : XP_DGRAY);
    }

    draw_toggle_switch(toggle_x, row_y, is_on);
    return 12;
}

// Section heading: coloured label plus a short accent tick and a faint rule.
// Returns the Y at which the section body may start.
static int draw_settings_section(int content_x, int content_w, int y, const char* title) {
    if (y < 0) return y;
    draw_compact_string_clipped(content_x + 6, y, (content_w - 12) / FONT_CW, title, XP_BLUE);
    int rule_y = y + 9;
    fill_rect(content_x + 6, rule_y, 12, 1, XP_LBLUE);
    fill_rect(content_x + 20, rule_y, content_w - 26, 1, XP_LGRAY);
    return rule_y + 4;
}

// Quieter sub-heading for groups inside a section.
static int draw_settings_subheader(int content_x, int content_w, int y, const char* title) {
    if (y < 0) return y;
    draw_compact_string_clipped(content_x + 6, y, (content_w - 12) / FONT_CW, title, XP_DGRAY);
    return y + 11;
}

// Label on the left, value right-aligned. Returns the height consumed.
static int draw_settings_kv(int content_x, int content_w, int y,
                            const char* key, const char* value) {
    if (y < 0 || !key || !value) return 11;
    draw_compact_string_clipped(content_x + 6, y, (content_w - 12) / FONT_CW, key, XP_DGRAY);

    int vw = (int)strlen(value) * FONT_CW;
    int vx = content_x + content_w - 6 - vw;
    int kx = content_x + 6;
    if (vx > kx + 54) {
        draw_compact_string(vx, y, value, XP_BLACK);
    } else {
        // Not enough room to right-align; place after the key instead.
        draw_compact_string_clipped(content_x + 50, y, (content_w - 56) / FONT_CW, value, XP_BLACK);
    }
    return 11;
}

// Backing card used to group related rows.
static void draw_settings_card(int content_x, int content_w, int y, int height) {
    if (height <= 0) return;
    int cx = content_x + 4;
    int cw = content_w - 12;
    if (cw <= 4) return;
    fill_chamfer_rect(cx, y, cw, height, XP_LGRAY);
    draw_chamfer_rect(cx, y, cw, height, XP_DGRAY);
    // Accent edge down the left side ties the group together.
    fill_rect(cx + 1, y + 1, 2, height - 2, XP_LBLUE);
}

// Height of one Customization navigation row.
#define SETTINGS_NAV_ROW_H 16

// A row that opens another Settings page: label on the left, a chevron on the
// right. The clickable rectangle is published for settings_app_handle_click()
// the same way the toggles publish theirs. Returns the height consumed.
static int draw_settings_nav_row(int content_x, int content_w, int y,
                                 const char* label, int target_tab) {
    int rx = content_x + 4;
    int rw = content_w - 12;
    if (rw <= 8) return SETTINGS_NAV_ROW_H + 6;

    fill_chamfer_rect(rx, y, rw, SETTINGS_NAV_ROW_H, XP_LGRAY);
    draw_chamfer_rect(rx, y, rw, SETTINGS_NAV_ROW_H, XP_DGRAY);
    fill_rect(rx + 1, y + 1, 2, SETTINGS_NAV_ROW_H - 2, XP_LBLUE);
    draw_compact_string_clipped(rx + 8, y + 4, (rw - 26) / FONT_CW, label, XP_BLACK);

    // Right-pointing triangle, drawn because the 8x8 font has no such glyph.
    int tip_x = rx + rw - 11;
    int mid_y = y + SETTINGS_NAV_ROW_H / 2;
    for (int i = 0; i < 5; i++) {
        int height = 5 - i;
        fill_rect(tip_x - i, mid_y - height / 2, 1, height, XP_DGRAY);
    }

    if (settings_nav_hit_count < SETTINGS_MAX_NAV_ROWS) {
        settings_nav_hit_x = rx;
        settings_nav_hit_w = rw;
        settings_nav_hit_y[settings_nav_hit_count] = y;
        settings_nav_hit_tab[settings_nav_hit_count] = target_tab;
        settings_nav_hit_count++;
    }
    return SETTINGS_NAV_ROW_H + 6;
}

// Publish a toggle's clickable rectangle for settings_app_handle_click().
static void settings_register_toggle(int x, int w, int y, int id) {
    if (settings_tog_count >= SETTINGS_MAX_TOGGLES) return;
    settings_tog_hit_x = x;
    settings_tog_hit_w = w;
    settings_tog_hit_y[settings_tog_count] = y;
    settings_tog_id[settings_tog_count] = id;
    settings_tog_count++;
}

// Render a group of toggles inside a card, registering each hit box.
// Returns the vertical space consumed.
static int draw_settings_toggle_group(int content_x, int content_w, int y,
                                      const settings_toggle_desc_t* items, int count) {
    const int ROW_H = 12;
    if (count <= 0) return 0;
    if (count > SETTINGS_MAX_TOGGLES) count = SETTINGS_MAX_TOGGLES;

    int card_h = 6 + count * ROW_H;
    draw_settings_card(content_x, content_w, y, card_h);

    int tx = content_x + 10;
    int tw = content_w - 22;
    int label_chars = (content_w - 60) / FONT_CW;
    if (label_chars < 4) label_chars = 4;

    for (int i = 0; i < count; i++) {
        int ry = y + 3 + i * ROW_H;
        settings_register_toggle(content_x, content_w - 16, ry, items[i].id);
        draw_settings_toggle(tx, tw, ry, items[i].label, items[i].is_on, label_chars);
    }
    return card_h + 8;
}

// Apply a toggle by id. The debug overlay lives in the shell, so that one goes
// through the shell service rather than a shared static.
static void settings_apply_toggle(int id) {
    switch (id) {
        case SETTINGS_TOG_COMPACT:    setting_compact_mode = (uint8_t)!setting_compact_mode; break;
        case SETTINGS_TOG_SECONDS:    setting_show_seconds = (uint8_t)!setting_show_seconds; break;
        case SETTINGS_TOG_DEBUG:      shell_set_debug_overlay(!shell_debug_overlay_enabled()); break;
        case SETTINGS_TOG_BLUETOOTH:  setting_bluetooth = (uint8_t)!setting_bluetooth; break;
        case SETTINGS_TOG_NETWORK:    setting_network = (uint8_t)!setting_network; break;
        case SETTINGS_TOG_CONTRAST:   setting_high_contrast = (uint8_t)!setting_high_contrast; break;
        case SETTINGS_TOG_LARGE_TEXT: setting_large_text = (uint8_t)!setting_large_text; break;
        default: break;
    }
}

// Row pitch for the navigation rail, derived from the pane height so every page
// stays on screen and clickable. A fixed 20px pitch let the later rows fall past
// the bottom of the rail, where they still consumed a nav index the click
// handler could no longer reach.
static int settings_nav_row_h(int pane_h) {
    int usable = (pane_h - 4) - 32;   // Rail height minus header and padding.
    int row_h = 20;
    if (usable > 0) {
        int fit = usable / SETTINGS_TAB_COUNT;
        if (fit < row_h) row_h = fit;
    }
    if (row_h < 11) row_h = 11;
    return row_h;
}

// ---------------------------------------------------------------------------
// Time zone dropdown
//
// A combo box: one row showing the current selection with a drop arrow, which
// expands to a scrolling list on click. The list is longer than the pane, so the
// mouse wheel scrolls it. Row geometry is published for the click and wheel
// handlers rather than recomputed there, which is how the other Settings
// controls stay aligned.
// ---------------------------------------------------------------------------

#define TZ_LIST_VISIBLE 8
#define TZ_ROW_H       11
#define TZ_LIST_BTN    11
#define TZ_COMBO_H     13

static int settings_tz_scroll = 0;
static int settings_tz_open = 0;
static int settings_tz_list_x = 0;
static int settings_tz_list_w = 0;
static int settings_tz_list_y = 0;
static int settings_tz_list_bottom = 0;
static int settings_tz_combo_y = 0;
static int settings_tz_up_x = 0;
static int settings_tz_up_y = 0;
static int settings_tz_down_x = 0;
static int settings_tz_down_y = 0;
static int settings_tz_btn = TZ_LIST_BTN;

// Short label for a zone, with the "UTC+05:30  " prefix stripped for readability.
static const char* settings_tz_selected_label(int index) {
    const tz_info_t* z = tz_get(index);
    if (!z) return "Unknown";
    const char* n = z->name;
    while (*n == 'U' || *n == 'T' || *n == 'C' || *n == '+' || *n == '-' ||
           (*n >= '0' && *n <= '9') || *n == ':') n++;
    while (*n == ' ') n++;
    return n;
}

// Solid triangle used for the drop arrow and the scroll buttons. The 8x8 font has
// no such glyph, so it is drawn rather than printed.
static void draw_tz_chevron_down(int x, int y, uint8_t color) {
    for (int row = 0; row < 4; row++) {
        int w = row * 2 + 1;
        fill_rect(x - w / 2, y + row, w, 1, color);
    }
}

static void draw_tz_chevron_up(int x, int y, uint8_t color) {
    for (int row = 0; row < 4; row++) {
        int w = (3 - row) * 2 + 1;
        fill_rect(x - w / 2, y + row, w, 1, color);
    }
}

static void settings_tz_clamp_scroll(int selected) {
    int total = tz_count();
    int max_scroll = total - TZ_LIST_VISIBLE;
    if (max_scroll < 0) max_scroll = 0;
    if (settings_tz_scroll < 0) settings_tz_scroll = 0;
    if (settings_tz_scroll > max_scroll) settings_tz_scroll = max_scroll;
    if (settings_tz_scroll > selected - 1) settings_tz_scroll = selected - 1;
    if (settings_tz_scroll < selected - TZ_LIST_VISIBLE + 2) {
        settings_tz_scroll = selected - TZ_LIST_VISIBLE + 2;
    }
    if (settings_tz_scroll < 0) settings_tz_scroll = 0;
    if (settings_tz_scroll > max_scroll) settings_tz_scroll = max_scroll;
}

static void settings_tz_scroll_by(int delta) {
    int total = tz_count();
    int max_scroll = total - TZ_LIST_VISIBLE;
    if (max_scroll < 0) max_scroll = 0;
    settings_tz_scroll += delta;
    if (settings_tz_scroll < 0) settings_tz_scroll = 0;
    if (settings_tz_scroll > max_scroll) settings_tz_scroll = max_scroll;
}

// Draws the combo row, plus the list when open. Returns the height consumed.
static int draw_settings_timezone_list(int cx, int cw, int y) {
    int selected = tz_current_index();
    settings_tz_clamp_scroll(selected);

    int total = tz_count();
    settings_tz_list_x = cx;
    settings_tz_list_w = cw;
    settings_tz_btn = TZ_LIST_BTN;
    settings_tz_combo_y = y;
    settings_tz_list_y = y + TZ_COMBO_H + 2;
    settings_tz_list_bottom = settings_tz_list_y;

    // ---- Combo row ---------------------------------------------------------
    fill_chamfer_rect(cx, y, cw, TZ_COMBO_H, settings_tz_open ? XP_LBLUE : XP_WHITE);
    draw_chamfer_rect(cx, y, cw, TZ_COMBO_H, XP_DGRAY);
    fill_rect(cx + 1, y + 1, cw - 2, 2, XP_LBLUE);

    char selected_buf[72];
    selected_buf[0] = 0;
    {
        int lp = 0;
        append_text(selected_buf, (int)sizeof(selected_buf), &lp,
                    settings_tz_selected_label(selected));
        append_text(selected_buf, (int)sizeof(selected_buf), &lp, "   (");
        append_unsigned(selected_buf, (int)sizeof(selected_buf), &lp, total);
        append_text(selected_buf, (int)sizeof(selected_buf), &lp, " zones)");
    }
    draw_compact_string_clipped(cx + 6, y + 3, (cw - 26) / FONT_CW, selected_buf, XP_BLACK);
    draw_tz_chevron_down(cx + cw - 8, y + 4, XP_DGRAY);

    if (!settings_tz_open) return TZ_COMBO_H + 2;

    // ---- Expanded list -----------------------------------------------------
    settings_tz_up_x = cx + cw - settings_tz_btn - 4;
    settings_tz_up_y = settings_tz_list_y;
    settings_tz_down_x = settings_tz_up_x;
    settings_tz_down_y = settings_tz_list_y + settings_tz_btn + 2;

    int list_y = settings_tz_list_y + settings_tz_btn + 4;
    int list_h = TZ_LIST_VISIBLE * TZ_ROW_H + 4;
    int frame_h = list_h + settings_tz_btn + 6;

    fill_chamfer_rect(cx, settings_tz_list_y, cw, frame_h, XP_WHITE);
    draw_chamfer_rect(cx, settings_tz_list_y, cw, frame_h, XP_DGRAY);

    // Scroll buttons, only when there is somewhere to scroll.
    if (total > TZ_LIST_VISIBLE) {
        int bx = settings_tz_up_x + settings_tz_btn / 2;
        fill_chamfer_rect(settings_tz_up_x, settings_tz_up_y, settings_tz_btn, settings_tz_btn, XP_LGRAY);
        draw_chamfer_rect(settings_tz_up_x, settings_tz_up_y, settings_tz_btn, settings_tz_btn, XP_DGRAY);
        draw_tz_chevron_up(bx, settings_tz_up_y + 3, XP_BLACK);

        fill_chamfer_rect(settings_tz_down_x, settings_tz_down_y, settings_tz_btn, settings_tz_btn, XP_LGRAY);
        draw_chamfer_rect(settings_tz_down_x, settings_tz_down_y, settings_tz_btn, settings_tz_btn, XP_DGRAY);
        draw_tz_chevron_down(bx, settings_tz_down_y + 4, XP_BLACK);
    }

    fill_chamfer_rect(cx + 3, list_y, cw - 6, list_h, XP_WHITE);
    draw_chamfer_rect(cx + 3, list_y, cw - 6, list_h, XP_DGRAY);

    for (int i = 0; i < TZ_LIST_VISIBLE; i++) {
        int idx = settings_tz_scroll + i;
        if (idx >= total) break;
        int ry = list_y + 2 + i * TZ_ROW_H;
        if (idx == selected) {
            fill_rect(cx + 4, ry, cw - 8, TZ_ROW_H - 1, XP_LBLUE);
        }
        draw_compact_string_clipped(cx + 7, ry + 1, (cw - 16) / FONT_CW,
                                    settings_tz_selected_label(idx), XP_BLACK);
    }

    settings_tz_list_bottom = settings_tz_list_y + frame_h;
    return (settings_tz_list_bottom - y) + 4;
}

// True when the cursor is over the combo row, or over the list while open.
static int settings_tz_hit(int mx, int my) {
    if (settings_tz_list_w <= 0) return 0;
    if (mx < settings_tz_list_x || mx >= settings_tz_list_x + settings_tz_list_w) return 0;
    if (my >= settings_tz_combo_y && my < settings_tz_combo_y + TZ_COMBO_H) return 1;
    if (settings_tz_open && my >= settings_tz_list_y && my < settings_tz_list_bottom) return 1;
    return 0;
}

static void settings_tz_close(void) {
    settings_tz_open = 0;
}

// Wheel scrolling over the open list.
static int handle_settings_tz_wheel(int mx, int my, int8_t wheel_delta) {
    if (!settings_tz_open || wheel_delta == 0) return 0;
    if (!settings_tz_hit(mx, my)) return 0;
    settings_tz_scroll_by(-(int)wheel_delta);
    return 1;
}

// Click handling, published by the renderer above.
static int handle_settings_tz_click(int mx, int my) {
    if (settings_tz_list_w <= 0) return 0;
    int total = tz_count();
    int selected = tz_current_index();

    int in_combo = (mx >= settings_tz_list_x &&
                    mx < settings_tz_list_x + settings_tz_list_w &&
                    my >= settings_tz_combo_y &&
                    my < settings_tz_combo_y + TZ_COMBO_H);
    if (in_combo) {
        settings_tz_open = (uint8_t)!settings_tz_open;
        settings_tz_clamp_scroll(tz_current_index());
        return 1;
    }

    if (!settings_tz_open) return 0;
    if (mx < settings_tz_list_x || mx >= settings_tz_list_x + settings_tz_list_w) return 0;
    if (my < settings_tz_list_y || my >= settings_tz_list_bottom) return 0;

    if (total > TZ_LIST_VISIBLE) {
        if (mx >= settings_tz_up_x && mx < settings_tz_up_x + settings_tz_btn &&
            my >= settings_tz_up_y && my < settings_tz_up_y + settings_tz_btn) {
            settings_tz_scroll_by(-1);
            return 1;
        }
        if (mx >= settings_tz_down_x && mx < settings_tz_down_x + settings_tz_btn &&
            my >= settings_tz_down_y && my < settings_tz_down_y + settings_tz_btn) {
            settings_tz_scroll_by(1);
            return 1;
        }
    }

    int list_y = settings_tz_list_y + settings_tz_btn + 6;
    if (my < list_y) return 1;
    int rel = (my - list_y) / TZ_ROW_H;
    int idx = settings_tz_scroll + rel;
    if (idx < 0 || idx >= total) return 1;

    if (idx != selected) tz_set_current_index(idx);
    settings_tz_open = 0;   // picking an entry closes the list
    return 1;
}

// ---------------------------------------------------------------------------
// Taskbar style combo box
//
// Two entries, so the list needs no scroll buttons: it follows the time zone
// combo's published-geometry pattern instead of inventing a second one.
// ---------------------------------------------------------------------------

#define TASKBAR_STYLE_COUNT 2
#define TASKBAR_STYLE_ROW_H 12

static const char* taskbar_style_label(int style) {
    switch (style) {
        case TASKBAR_STYLE_WINDOWS:    return "Windows";
        case TASKBAR_STYLE_MACOS_DOCK: return "macOS Dock";
        default:                       return "Unknown";
    }
}

static int taskbar_style_combo_x = 0;
static int taskbar_style_combo_y = 0;
static int taskbar_style_combo_w = 0;
static int taskbar_style_list_y = 0;
static int taskbar_style_list_bottom = 0;
static uint8_t taskbar_style_open = 0;

static void taskbar_style_close(void) {
    taskbar_style_open = 0;
}

// Draws the combo row, plus the list while open. Returns the height consumed.
static int draw_taskbar_style_combo(int cx, int cw, int y) {
    taskbar_style_combo_x = cx;
    taskbar_style_combo_y = y;
    taskbar_style_combo_w = cw;
    taskbar_style_list_y = y + TZ_COMBO_H + 2;
    taskbar_style_list_bottom = y + TZ_COMBO_H;

    fill_chamfer_rect(cx, y, cw, TZ_COMBO_H, taskbar_style_open ? XP_LBLUE : XP_WHITE);
    draw_chamfer_rect(cx, y, cw, TZ_COMBO_H, XP_DGRAY);
    fill_rect(cx + 1, y + 1, cw - 2, 2, XP_LBLUE);
    draw_compact_string_clipped(cx + 6, y + 3, (cw - 26) / FONT_CW,
                                taskbar_style_label(setting_taskbar_style), XP_BLACK);
    draw_tz_chevron_down(cx + cw - 8, y + 4, XP_DGRAY);

    if (!taskbar_style_open) return TZ_COMBO_H + 4;

    int list_h = TASKBAR_STYLE_COUNT * TASKBAR_STYLE_ROW_H + 4;
    taskbar_style_list_bottom = taskbar_style_list_y + list_h;

    fill_chamfer_rect(cx, taskbar_style_list_y, cw, list_h, XP_WHITE);
    draw_chamfer_rect(cx, taskbar_style_list_y, cw, list_h, XP_DGRAY);
    for (int i = 0; i < TASKBAR_STYLE_COUNT; i++) {
        int ry = taskbar_style_list_y + 2 + i * TASKBAR_STYLE_ROW_H;
        if (i == setting_taskbar_style) {
            fill_rect(cx + 3, ry, cw - 6, TASKBAR_STYLE_ROW_H - 1, XP_LBLUE);
        }
        draw_compact_string_clipped(cx + 7, ry + 2, (cw - 16) / FONT_CW,
                                    taskbar_style_label(i), XP_BLACK);
    }
    return (taskbar_style_list_bottom - y) + 4;
}

static int handle_taskbar_style_click(int mx, int my) {
    if (taskbar_style_combo_w <= 0) return 0;

    int in_combo = (mx >= taskbar_style_combo_x &&
                    mx < taskbar_style_combo_x + taskbar_style_combo_w &&
                    my >= taskbar_style_combo_y &&
                    my < taskbar_style_combo_y + TZ_COMBO_H);
    if (in_combo) {
        taskbar_style_open = (uint8_t)!taskbar_style_open;
        return 1;
    }

    if (!taskbar_style_open) return 0;
    if (mx < taskbar_style_combo_x ||
        mx >= taskbar_style_combo_x + taskbar_style_combo_w) return 0;
    if (my < taskbar_style_list_y || my >= taskbar_style_list_bottom) return 0;

    int rel = (my - (taskbar_style_list_y + 2)) / TASKBAR_STYLE_ROW_H;
    if (rel < 0 || rel >= TASKBAR_STYLE_COUNT) return 1;
    setting_taskbar_style = rel;
    taskbar_style_open = 0;   // picking an entry closes the list
    return 1;
}

// ---------------------------------------------------------------------------
// Page rendering
// ---------------------------------------------------------------------------

// Defined with the changelog section below; forward-declared because the
// GamerOS Update page draws it.
static void settings_md_ensure_loaded(void);

void settings_app_draw_icon(int x, int y) {
    fill_chamfer_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_WHITE);
    draw_chamfer_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, XP_DGRAY);
    fill_rect(x + 2, y + 2, 3, DESKTOP_ICON_H - 4, XP_LBLUE);
    fill_rect(x + 8, y + 4, 4, 12, XP_DGRAY);
    fill_rect(x + 4, y + 8, 12, 4, XP_DGRAY);
    fill_rect(x + 6, y + 6, 8, 8, XP_LGRAY);
    fill_rect(x + 8, y + 8, 4, 4, XP_WHITE);
}

void settings_app_draw_content(int x, int y, int w, int h, int is_active) {
    (void)is_active;
    if (w < 24 || h < 24) return;

    // Pane background, then the two columns: a navigation rail on the left and
    // the page body on the right.
    fill_chamfer_rect(x + 0, y + 0, w, h, XP_WHITE);
    draw_chamfer_rect(x + 0, y + 0, w, h, XP_DGRAY);

    settings_pane_x = x;
    settings_pane_y = y;
    settings_pane_w = w;
    settings_pane_h = h;

    int nav_x = x + 2;
    int nav_y = y + 2;
    int nav_w = 124;
    int nav_h = h - 4;
    settings_nav_x = nav_x;
    settings_nav_y = nav_y;
    settings_nav_w = nav_w;

    fill_chamfer_rect(nav_x, nav_y, nav_w, nav_h, XP_LGRAY);
    draw_chamfer_rect(nav_x, nav_y, nav_w, nav_h, XP_DGRAY);
    fill_chamfer_rect(nav_x + 1, nav_y + 1, nav_w - 2, 18, XP_WHITE);
    fill_rect(nav_x + 2, nav_y + 2, nav_w - 4, 2, XP_LBLUE);
    draw_compact_string(nav_x + 8, nav_y + 7, settings_app_title(), XP_BLACK);

    int content_x = nav_x + nav_w + 8;
    int content_y = y + 2;
    int content_w = w - (content_x - x) - 8;
    int content_h = h - 4;
    settings_content_x = content_x;
    settings_content_y = content_y;
    settings_content_w = content_w;
    settings_content_h = content_h;

    int body_y = content_y + 24;
    fill_chamfer_rect(content_x, content_y, content_w, content_h, XP_WHITE);
    draw_chamfer_rect(content_x, content_y, content_w, content_h, XP_DGRAY);
    fill_chamfer_rect(content_x + 1, content_y + 1, content_w - 2, 20, XP_LGRAY);
    fill_rect(content_x + 2, content_y + 2, content_w - 4, 2, XP_LBLUE);
    draw_compact_string_clipped(content_x + 8, content_y + 7, (content_w - 16) / FONT_CW, "Modern Control Center", XP_BLACK);
    int content_chars = (content_w - 12) / FONT_CW;
    if (content_chars < 1) content_chars = 1;

    int nav_bottom = nav_y + nav_h;
    int nav_row_h = settings_nav_row_h(h);
    int nav_row_box = nav_row_h - 2;
    for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
        int ty = nav_y + 28 + i * nav_row_h;
        if (ty + nav_row_box > nav_bottom) break;  // Pane too short: stop drawing rows.
        fill_chamfer_rect(nav_x + 5, ty - 1, nav_w - 10, nav_row_box, (settings_tab == i) ? XP_LBLUE : XP_WHITE);
        draw_chamfer_rect(nav_x + 5, ty - 1, nav_w - 10, nav_row_box, XP_DGRAY);
        if (settings_tab == i) {
            fill_rect(nav_x + 7, ty + 1, 3, nav_row_box - 4, XP_WHITE);
        }
        draw_compact_string_clipped(nav_x + 14, ty + (nav_row_box - FONT_CH) / 2,
                                    (nav_w - 24) / FONT_CW, settings_ui_tab_name(i), XP_BLACK);
    }

    // Reset per-frame hit-box registries. Only the active page renders, so
    // whatever registers below is exactly what is clickable this frame.
    settings_tog_count = 0;
    settings_res_hit_count = 0;
    settings_nav_hit_count = 0;

    if (settings_tab == SETTINGS_TAB_SYSTEM) {
        // ---- System toggles -------------------------------------------
        // The taskbar toggle lives on the Taskbar page now.
        settings_toggle_desc_t sys_toggles[] = {
            { "Clock seconds",   setting_show_seconds, SETTINGS_TOG_SECONDS },
            { "Debug window",    shell_debug_overlay_enabled(), SETTINGS_TOG_DEBUG }
        };

        int ry = draw_settings_section(content_x, content_w, body_y, "System");
        ry += draw_settings_toggle_group(content_x, content_w, ry, sys_toggles, 2);

        // ---- Display info ---------------------------------------------
        {
            char cur[40];
            char nat[40];
            char be[48];
            sprintf(cur, "%ux%u", (unsigned int)current_width, (unsigned int)current_height);
            sprintf(nat, "%ux%u", (unsigned int)graphics_get_native_width(), (unsigned int)graphics_get_native_height());
            sprintf(be, "%s", graphics_is_truecolor() ? "Framebuffer" : "VGA Safe");

            ry = draw_settings_section(content_x, content_w, ry, "Display");
            ry += draw_settings_kv(content_x, content_w, ry, "Resolution", cur);
            ry += draw_settings_kv(content_x, content_w, ry, "Native", nat);
            ry += draw_settings_kv(content_x, content_w, ry, "Backend", be);
            ry += 6;
        }

        // ---- Resolution list ------------------------------------------
        ry = draw_settings_subheader(content_x, content_w, ry, "Resolution");
        {
            int rx = content_x + 6;
            int rw = content_w - 16;
            int selected = shell_display_profile_current();
            settings_res_hit_x = rx;
            settings_res_hit_h = 12;
            for (int i = 0; i < shell_display_profile_count(); i++) {
                if (ry + 12 > content_y + content_h) break;
                if (i >= 8) break;
                int usable = shell_display_profile_available(i);
                uint8_t bg = selected ? XP_LBLUE : XP_WHITE;
                fill_chamfer_rect(rx, ry, rw, 12, bg);
                draw_chamfer_rect(rx, ry, rw, 12, XP_DGRAY);
                // Selection marker.
                if (selected) fill_rect(rx + 2, ry + 2, 2, 8, XP_WHITE);
                draw_compact_string_clipped(rx + 7, ry + 2, (rw - 12) / FONT_CW,
                                            shell_display_profile_label(i),
                                            usable ? XP_BLACK : XP_DGRAY);
                settings_res_hit_y[settings_res_hit_count++] = ry;
                ry += 13;
            }
            ry += 6;
        }

        // ---- Storage ---------------------------------------------------
        ry = draw_settings_section(content_x, content_w, ry, "Storage");
        {
            int sc = fs_storage_get_device_count();
            char cnt[8];
            cnt[0] = (char)('0' + (sc > 9 ? 9 : sc));
            cnt[1] = 0;
            ry += draw_settings_kv(content_x, content_w, ry, "Devices", cnt);
            ry += draw_settings_kv(content_x, content_w, ry, "Volume", "System Disk");
        }
    } else if (settings_tab == SETTINGS_TAB_PERSONALIZATION) {
        char res_line[40];
        char scale_line[40];
        sprintf(res_line, "%ux%u", (unsigned int)current_width, (unsigned int)current_height);
        sprintf(scale_line, "x%d", shell_ui_scale());

        int ry = draw_settings_section(content_x, content_w, body_y, "Theme");
        ry += draw_settings_kv(content_x, content_w, ry, "Theme", "GamerOS Modern");
        ry += draw_settings_kv(content_x, content_w, ry, "Color profile", "Blue");
        ry += 10;

        ry = draw_settings_section(content_x, content_w, ry, "Layout");
        ry += draw_settings_kv(content_x, content_w, ry, "Desktop", res_line);
        ry += draw_settings_kv(content_x, content_w, ry, "UI scale", scale_line);
        // The taskbar row moved to the Taskbar page, where it is editable.
    } else if (settings_tab == SETTINGS_TAB_CUSTOMIZATION) {
        int ry = draw_settings_section(content_x, content_w, body_y, "Shell");

        ry += draw_settings_nav_row(content_x, content_w, ry, "Taskbar",
                                    SETTINGS_TAB_TASKBAR);

        ry = draw_settings_section(content_x, content_w, ry, "Current");
        ry += draw_settings_kv(content_x, content_w, ry, "Style",
                               taskbar_style_label(setting_taskbar_style));
        ry += draw_settings_kv(content_x, content_w, ry, "Task buttons",
                               setting_compact_mode ? "Compact" : "Full");
    } else if (settings_tab == SETTINGS_TAB_TASKBAR) {
        settings_toggle_desc_t tb_toggles[] = {
            { "Compact task buttons", setting_compact_mode, SETTINGS_TOG_COMPACT }
        };

        int ry = draw_settings_section(content_x, content_w, body_y, "Taskbar style");
        ry += draw_taskbar_style_combo(content_x, content_w, ry);
        ry += draw_settings_kv(content_x, content_w, ry, "Layout",
                               setting_taskbar_style == TASKBAR_STYLE_WINDOWS
                                   ? "Full width bar" : "Floating dock");
        ry += 6;

        ry = draw_settings_section(content_x, content_w, ry, "Size");
        ry += draw_settings_toggle_group(content_x, content_w, ry, tb_toggles, 1);
    } else if (settings_tab == SETTINGS_TAB_BLUETOOTH_DEVICES) {
        settings_toggle_desc_t dev_toggles[] = {
            { "Bluetooth", setting_bluetooth, SETTINGS_TOG_BLUETOOTH }
        };

        int ry = draw_settings_section(content_x, content_w, body_y, "Wireless");
        ry += draw_settings_toggle_group(content_x, content_w, ry, dev_toggles, 1);
        ry = draw_settings_section(content_x, content_w, ry, "Input");
        ry += draw_settings_kv(content_x, content_w, ry, "Mouse", mouse_is_present() ? "PS/2 Compatible" : "Not detected");
        ry += draw_settings_kv(content_x, content_w, ry, "Keyboard", "PS/2 Standard");
        ry += 10;

        ry = draw_settings_section(content_x, content_w, ry, "Display");
        ry += draw_settings_kv(content_x, content_w, ry, "Output", "VGA / Framebuffer");
    } else if (settings_tab == SETTINGS_TAB_NETWORK_INTERNET) {
        settings_toggle_desc_t net_toggles[] = {
            { "Wi-Fi", setting_network, SETTINGS_TOG_NETWORK }
        };

        int ry = draw_settings_section(content_x, content_w, body_y, "Connection");
        ry += draw_settings_toggle_group(content_x, content_w, ry, net_toggles, 1);
        ry = draw_settings_section(content_x, content_w, ry, "Status");
        ry += draw_settings_kv(content_x, content_w, ry, "Network stack", setting_network ? "Enabled" : "Not enabled");
        ry += draw_settings_kv(content_x, content_w, ry, "Adapter", setting_network ? "Connecting" : "Virtual profile");
        ry += draw_settings_kv(content_x, content_w, ry, "IP address", setting_network ? "Pending" : "None");
    } else if (settings_tab == SETTINGS_TAB_APPS) {
        int ry = draw_settings_section(content_x, content_w, body_y, "Installed");
        {
            static const char* app_names[] = {
                "Notepad", "Settings", "File Explorer", "About GamerOS"
            };
            int count = (int)(sizeof(app_names) / sizeof(app_names[0]));
            int card_h = 6 + count * 12;
            draw_settings_card(content_x, content_w, ry, card_h);
            for (int i = 0; i < count; i++) {
                int ary = ry + 3 + i * 12;
                // Bullet marker, then the display name.
                fill_rect(content_x + 9, ary + 4, 2, 2, XP_LBLUE);
                draw_compact_string_clipped(content_x + 15, ary + 2,
                                            (content_w - 60) / FONT_CW, app_names[i], XP_BLACK);
            }
            ry += card_h + 10;
        }
        ry = draw_settings_section(content_x, content_w, ry, "Paths");
        ry += draw_settings_kv(content_x, content_w, ry, "App root", "C:/GamerOS/Apps");
        ry += draw_settings_kv(content_x, content_w, ry, "Manifests", "C:/GamerOS/Apps/Manifests");
    } else if (settings_tab == SETTINGS_TAB_ACCOUNTS) {
        int ry = draw_settings_section(content_x, content_w, body_y, "Account");
        ry += draw_settings_kv(content_x, content_w, ry, "User", "Admin");
        ry += draw_settings_kv(content_x, content_w, ry, "Profile", "C:/Users/Admin");
        ry += 10;

        ry = draw_settings_section(content_x, content_w, ry, "Session");
        ry += draw_settings_kv(content_x, content_w, ry, "Mode", "Local");
        ry += draw_settings_kv(content_x, content_w, ry, "Security level", "System");
        ry += draw_settings_kv(content_x, content_w, ry, "Isolation", "Active");
    } else if (settings_tab == SETTINGS_TAB_TIME_LANGUAGE) {
        // Time section first, then language. Local time comes from the
        // selected time zone rather than straight off the RTC.
        uint32_t now_utc = get_unix_time();
        int tz_idx = tz_current_index();
        int tz_dst = (tz_offset_minutes_at(tz_idx, now_utc) !=
                      (tz_get(tz_idx) ? tz_get(tz_idx)->std_offset_min : 0));

        uint16_t ly = 0; uint8_t lmo = 1, ld = 1, lh = 0, lmi = 0, ls = 0;
        tz_local_time(now_utc, &ly, &lmo, &ld, &lh, &lmi, &ls);

        char tm[24];
        sprintf(tm, "%02u:%02u:%02u", (unsigned int)lh, (unsigned int)lmi, (unsigned int)ls);
        char tzline[24];
        tz_format_offset(tzline, (int)sizeof(tzline), tz_idx, now_utc);

        int ry = draw_settings_section(content_x, content_w, body_y, "Time");
        ry += draw_settings_kv(content_x, content_w, ry, "Local time", tm);
        ry += draw_settings_kv(content_x, content_w, ry, "Time zone",
                               settings_tz_selected_label(tz_idx));
        ry += draw_settings_kv(content_x, content_w, ry, "UTC offset", tzline);
        ry += draw_settings_kv(content_x, content_w, ry, "Daylight saving",
                               tz_dst ? "In effect" : "Inactive");
        // The dropdown: combo row, expanding to a scrolling list.
        // Advance from the published bottom edge rather than the helper's
        // return value, so the sections below can never sit underneath the
        // expanded list.
        {
            int tz_top = ry;
            draw_settings_timezone_list(content_x, content_w, ry);
            int tz_floor = settings_tz_list_bottom;
            if (tz_floor <= tz_top) tz_floor = tz_top + TZ_COMBO_H + 2;
            ry = tz_floor + 10;
        }

        ry = draw_settings_section(content_x, content_w, ry, "Language");
        ry += draw_settings_kv(content_x, content_w, ry, "Display", "English (US)");
        ry += draw_settings_kv(content_x, content_w, ry, "Region", "United States");
    } else if (settings_tab == SETTINGS_TAB_GAMING) {
        char gfx_line[48];
        char res_line[40];
        uint8_t bpp = graphics_get_bpp();
        sprintf(gfx_line, "%s %u-bpp",
                graphics_is_truecolor() ? "True color" : "VGA indexed",
                (unsigned int)bpp);
        sprintf(res_line, "%ux%u", (unsigned int)current_width, (unsigned int)current_height);

        int ry = draw_settings_section(content_x, content_w, body_y, "Graphics");
        ry += draw_settings_kv(content_x, content_w, ry, "Mode", gfx_line);
        ry += draw_settings_kv(content_x, content_w, ry, "Resolution", res_line);
        ry += draw_settings_kv(content_x, content_w, ry, "Backend",
                               graphics_is_truecolor() ? "Framebuffer" : "VGA Safe");
        ry += 10;

        ry = draw_settings_section(content_x, content_w, ry, "Input");
        ry += draw_settings_kv(content_x, content_w, ry, "Latency mode", "Balanced");
        ry += draw_settings_kv(content_x, content_w, ry, "Polling", "Bounded (64 max)");
    } else if (settings_tab == SETTINGS_TAB_ACCESSIBILITY) {
        settings_toggle_desc_t a11y_toggles[] = {
            { "High contrast", setting_high_contrast, SETTINGS_TOG_CONTRAST },
            { "Large text",     setting_large_text,    SETTINGS_TOG_LARGE_TEXT }
        };

        int ry = draw_settings_section(content_x, content_w, body_y, "Display");
        ry += draw_settings_toggle_group(content_x, content_w, ry, a11y_toggles, 2);
        ry = draw_settings_section(content_x, content_w, ry, "Text");
        ry += draw_settings_kv(content_x, content_w, ry, "Font size", setting_large_text ? "Large" : "Standard");
    } else if (settings_tab == SETTINGS_TAB_PRIVACY_SECURITY) {
        int ry = draw_settings_section(content_x, content_w, body_y, "Security");
        ry += draw_settings_kv(content_x, content_w, ry, "Auth model", "Enabled");
        ry += draw_settings_kv(content_x, content_w, ry, "Isolation table", "Active");
        ry += draw_settings_kv(content_x, content_w, ry, "Policy", "Standard");
        ry += 10;

        ry = draw_settings_section(content_x, content_w, ry, "Data");
        ry += draw_settings_kv(content_x, content_w, ry, "User profile", "C:/Users/Admin");
        ry += draw_settings_kv(content_x, content_w, ry, "App data", "C:/Users/Admin/AppData");
        ry += draw_settings_kv(content_x, content_w, ry, "Telemetry", "None");
    } else if (settings_tab == SETTINGS_TAB_GAMEROS_UPDATE) {
        settings_md_ensure_loaded();
        int has_roadmap = (md_roadmap_start() >= 0);

        // Segmented control switching between the changelog and roadmap.
        int sub_x = content_x + 6;
        int sub_y = content_y + 5;
        int sub_gap = 6;
        int sub_w = (content_w - 12 - sub_gap) / 2;
        int sub_h = 15;
        if (sub_w < 56) sub_w = 56;
        settings_view_hit_x = sub_x;
        settings_view_hit_y = sub_y;
        settings_view_hit_w = sub_w;
        settings_view_hit_h = sub_h;
        settings_view_hit_gap = sub_gap;

        {
            const char* seg_labels[2] = { "Changelog", "Roadmap" };
            for (int i = 0; i < 2; i++) {
                int bx = sub_x + i * (sub_w + sub_gap);
                int active = (settings_update_view == i);
                fill_chamfer_rect(bx, sub_y, sub_w, sub_h, active ? XP_LBLUE : XP_LGRAY);
                draw_chamfer_rect(bx, sub_y, sub_w, sub_h, XP_DGRAY);
                if (active) fill_rect(bx + 1, sub_y + 1, sub_w - 2, 2, XP_WHITE);
                int chars = (sub_w - 10) / FONT_CW;
                draw_compact_string_clipped(bx + 5, sub_y + 4, chars, seg_labels[i], XP_BLACK);
            }
        }

        // Markdown viewer body. The module owns wrapping, block styling and
        // the scroll clamp, so the shell only supplies the box.
        int view_y = sub_y + sub_h + 5;
        int view_h = content_y + content_h - view_y - 12;
        if (view_h < 20) view_h = 20;
        draw_settings_card(content_x, content_w, view_y, view_h);

        int text_top = view_y + 4;
        int body_h = view_h - 8;
        if (body_h < 20) body_h = 20;

        int scroll = settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE];
        md_draw(content_x, text_top, content_w - 8, body_h,
                settings_update_view, &scroll);
        settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] = scroll;

        if (!has_roadmap && settings_update_view == MD_VIEW_ROADMAP) {
            // No roadmap section in this changelog. Say so rather than
            // duplicating the changelog, which made the toggle look broken.
            draw_compact_string_clipped(content_x + 12, text_top + 2,
                                        (content_w - 24) / FONT_CW,
                                        "No roadmap in this changelog.", XP_DGRAY);
        }

        // Footer hint, pinned to the bottom of the pane.
        draw_compact_string_clipped(content_x + 8, content_y + content_h - 10,
                                    content_chars, "Scroll with mouse wheel", XP_DGRAY);
    }
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

int settings_app_handle_click(int mx, int my) {
    if (settings_pane_w <= 0) return 0;
    if (mx < settings_pane_x || mx >= settings_pane_x + settings_pane_w ||
        my < settings_pane_y || my >= settings_pane_y + settings_pane_h) {
        return 0;
    }

    // Time zone dropdown, only meaningful on the Time & Language page.
    if (settings_tab == SETTINGS_TAB_TIME_LANGUAGE) {
        if (handle_settings_tz_click(mx, my)) return 1;
        // A click anywhere else in the content pane dismisses the list, the
        // way a combo box behaves.
        settings_tz_close();
    }

    // Taskbar style dropdown, only meaningful on the Taskbar page.
    if (settings_tab == SETTINGS_TAB_TASKBAR) {
        if (handle_taskbar_style_click(mx, my)) return 1;
        taskbar_style_close();
    }

    // Navigation rail. Must match the renderer: the first row sits at
    // nav_y + 28, on the same height-derived pitch.
    int nav_row_h = settings_nav_row_h(settings_pane_h);
    int nav_start_y = settings_nav_y + 28;
    if (mx >= settings_nav_x + 5 && mx < settings_nav_x + settings_nav_w - 5 &&
        my >= nav_start_y && my < nav_start_y + SETTINGS_TAB_COUNT * nav_row_h) {
        int tab_idx = (my - nav_start_y) / nav_row_h;
        if (tab_idx >= 0 && tab_idx < SETTINGS_TAB_COUNT) {
            settings_tab = tab_idx;
            return 1;
        }
    }

    // Toggles are registered by whichever page is on screen, so a single loop
    // covers System, Devices, Network, Taskbar and Accessibility without
    // per-page geometry duplicated here.
    for (int i = 0; i < settings_tog_count; i++) {
        int ty = settings_tog_hit_y[i];
        if (mx >= settings_tog_hit_x && mx < settings_tog_hit_x + settings_tog_hit_w &&
            my >= ty && my < ty + 12) {
            settings_apply_toggle(settings_tog_id[i]);
            return 1;
        }
    }

    // Customization's rows jump to another page, using the rectangles the
    // renderer published.
    for (int i = 0; i < settings_nav_hit_count; i++) {
        if (mx >= settings_nav_hit_x && mx < settings_nav_hit_x + settings_nav_hit_w &&
            my >= settings_nav_hit_y[i] && my < settings_nav_hit_y[i] + SETTINGS_NAV_ROW_H) {
            settings_tab = settings_nav_hit_tab[i];
            settings_scroll_top[settings_tab] = 0;
            return 1;
        }
    }

    // Segmented control inside GamerOS Update, using the published geometry.
    if (settings_tab == SETTINGS_TAB_GAMEROS_UPDATE && settings_view_hit_h > 0) {
        if (my >= settings_view_hit_y && my < settings_view_hit_y + settings_view_hit_h) {
            for (int seg = 0; seg < 2; seg++) {
                int bx = settings_view_hit_x + seg * (settings_view_hit_w + settings_view_hit_gap);
                if (mx >= bx && mx < bx + settings_view_hit_w) {
                    if (settings_update_view != seg) {
                        settings_update_view = seg;
                        settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] = 0;
                    }
                    return 1;
                }
            }
        }
    }

    if (settings_tab == SETTINGS_TAB_SYSTEM) {
        int list_w = settings_pane_w - (settings_res_hit_x - settings_pane_x) - 8;
        for (int i = 0; i < settings_res_hit_count; i++) {
            int ry = settings_res_hit_y[i];
            if (mx >= settings_res_hit_x && mx < settings_res_hit_x + list_w &&
                my >= ry && my < ry + settings_res_hit_h) {
                if (!shell_display_profile_available(i)) {
                    shell_raise_error("Display Error", "This profile needs framebuffer mode.");
                    return 1;
                }
                shell_set_display_profile(i);
                return 1;
            }
        }
    }
    return 0;
}

int settings_app_handle_wheel(int mx, int my, int8_t wheel_delta) {
    if (settings_content_w <= 0) return 0;
    if (mx < settings_content_x || mx >= settings_content_x + settings_content_w ||
        my < settings_content_y || my >= settings_content_y + settings_content_h) {
        return 0;
    }

    // The time zone combo scrolls under the wheel before the page tries
    // anything else, so the list moves instead of the page.
    if (settings_tab == SETTINGS_TAB_TIME_LANGUAGE) {
        if (handle_settings_tz_wheel(mx, my, wheel_delta)) return 1;
    }

    if (settings_tab == SETTINGS_TAB_GAMEROS_UPDATE) {
        settings_md_ensure_loaded();
        // Same box the renderer uses, so the clamp cannot drift from the pixels.
        int sub_y = settings_content_y + 5;
        int sub_h = 15;
        int view_y = sub_y + sub_h + 5;
        int view_h = settings_content_y + settings_content_h - view_y - 12;
        if (view_h < 20) view_h = 20;
        int body_h = view_h - 8;
        if (body_h < 20) body_h = 20;

        int scroll = settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE];
        int next = md_scroll_by(settings_content_w - 8, settings_update_view, scroll, -wheel_delta);
        if (next != scroll) {
            settings_scroll_top[SETTINGS_TAB_GAMEROS_UPDATE] = next;
            return 1;
        }
    }
    return 0;
}

void settings_app_select_tab(int tab_idx) {
    if (tab_idx >= 0 && tab_idx < SETTINGS_TAB_COUNT) {
        settings_tab = tab_idx;
    }
}

void settings_app_reset_scroll(void) {
    for (int i = 0; i < SETTINGS_TAB_COUNT; i++) settings_scroll_top[i] = 0;
}

// ---------------------------------------------------------------------------
// Changelog
// ---------------------------------------------------------------------------

static void settings_md_parse(const char* markdown) {
    md_parse(markdown);
    if (md_was_truncated()) {
        serial_write_string("Settings changelog: document truncated\n");
    }
}

// Drop the parsed document so storage is read again after it comes up.
void settings_app_reload_changelog(void) {
    settings_md_loaded = 0;
}

const char* settings_app_changelog_path(void) {
    return "C:/GamerOS/CHANGELOG.md";
}

static void settings_md_ensure_loaded(void) {
    if (settings_md_loaded) return;
    settings_md_loaded = 1;

    if (!shell_storage_ready()) {
        settings_md_parse(settings_app_changelog_text());
        return;
    }

    file_t* md_file = fs_open_file(settings_app_changelog_path());
    if (!md_file || md_file->size == 0) {
        settings_md_parse(settings_app_changelog_text());
        return;
    }

    static uint8_t md_buffer[MAX_FILE_SIZE + 1];
    memset(md_buffer, 0, sizeof(md_buffer));
    fs_read_file(md_file, md_buffer, MAX_FILE_SIZE);
    settings_md_parse((const char*)md_buffer);
}

// ---------------------------------------------------------------------------
// Accessors the shell reads
// ---------------------------------------------------------------------------

int settings_app_show_seconds(void) {
    return setting_show_seconds ? 1 : 0;
}

int settings_app_taskbar_compact(void) {
    return setting_compact_mode ? 1 : 0;
}

int settings_app_taskbar_style(void) {
    return setting_taskbar_style;
}

const char* settings_app_changelog_text(void) {
    return            "# GamerOS Changelog (2026-10-12)\n"
           "\n"
           "> **WORK IN PROGRESS — DO NOT SHIP.** Build 1.570 is unfinished, and some of\n"
           "> the entries below are only compile-verified. Treat this as a running log.\n"
           "\n"
           "## Release\n"
           "\n"
           "- Version: `00m2`\n"
           "- Build: `1.570`\n"
           "- Status: Unreleased. Work in progress; still being verified.\n"
           "\n"
           "\n"
           "\n"
           "## Customization\n"
           "\n"
           "- **Added a Customization page to Settings.** It is the hub for shell\n"
           "  appearance: a Taskbar row opens the new Taskbar page, and the current style\n"
           "  and button size are listed underneath so the state is readable without\n"
           "  navigating. The row publishes its click rectangle from the renderer the same\n"
           "  way the toggles do, so the hit box cannot drift away from the pixels.\n"
           "- **Added a Taskbar page.** The \"Taskbar compact\" toggle moved off System and\n"
           "  the read-only Taskbar row moved off Personalization, so every taskbar setting\n"
           "  is now in one place instead of being split across two pages, one of which\n"
           "  could display it but not change it.\n"
           "- **Added a taskbar style dropdown** with two entries. `Windows` renders the\n"
           "  classic full-width bar flush with the bottom edge, and `macOS Dock` keeps the\n"
           "  floating rounded panel. `dock_compute_layout()` branches on the choice but\n"
           "  keeps the item order identical, so Start stays on the left and the clock\n"
           "  stays in the tray in both styles; the renderer swaps the rounded panel,\n"
           "  shadow and inset bevel for a square bar with a flat bevel. The dock and the\n"
           "  bar share one layout call, so the click targets stay aligned with the pixels\n"
           "  either way.\n"
           "- **Made the Settings navigation rail adapt to the window height.** A fixed\n"
           "  20px row pitch let the later rows fall past the bottom of the rail, where\n"
           "  they still consumed a nav index the click handler could no longer reach.\n"
           "  The pitch is now derived from the window height in one helper that both the\n"
           "  renderer and `handle_settings_click()` call, and all thirteen rows remain\n"
           "  visible and clickable even in a short window.\n"
           "- **Bumped the build number from `1.560` to `1.570`** in `GAMEROS.INI`, the\n"
           "  desktop watermark and the Settings About page, and repointed\n"
           "  `tools/embed_changelog.py` at this file so the in-OS changelog viewer shows\n"
           "  the running 1.570 log instead of the 1.550 release notes.\n"
           "\n"
           "\n"
           "## Fixes\n"
           "\n"
           "- **Fixed the Windows taskbar drawing its items at the top of the screen.**\n"
           "  `dock_compute_layout()` set `start_y`, `taskbtn_y` and `clock_y` to the\n"
           "  *relative* `DOCK_PAD_Y` while the dock branch converted them to absolute\n"
           "  screen coordinates further down. The Windows branch returned before that\n"
           "  conversion, so Start, the task buttons and the clock rendered at y=5 above\n"
           "  an empty bar glued to the bottom edge. Each branch now writes the absolute\n"
           "  `y + DOCK_PAD_Y` itself, so the two cannot drift. Checked with a standalone\n"
           "  harness over 5 resolutions x 2 styles x compact/full x 0-6 windows, asserting\n"
           "  that every item lands inside its panel and that no two overlap.\n"
           "- **Fixed the desktop watermark ignoring the taskbar style.** It was anchored\n"
           "  to `DOCK_RESERVED_H`, the *tallest* the taskbar can be, so it floated 7px\n"
           "  higher than needed with the Windows bar. It now derives its position from the\n"
           "  live dock geometry, keeping the same 10px gap above whatever is drawn, and it\n"
           "  drops 6px further in the floating-dock style, which leaves room underneath.\n"
           "\n"
           "\n"
           "## Modularisation\n"
           "\n"
           "- **Moved every Settings component out of `main.c` into its own app module.**\n"
           "  `src/ui/apps/settings/settings_ui.{c,h}` now owns all thirteen pages, the\n"
           "  navigation rail, both dropdowns, every row primitive, the hit-box registries,\n"
           "  the input handlers and the embedded changelog — roughly 1,150 lines that were\n"
           "  interleaved with the window manager. The shell keeps only window plumbing and\n"
           "  reads user choices back through accessors.\n"
           "- **The two sides now talk through a declared interface.** `settings_ui.h`\n"
           "  exposes the shell-facing API (title, draw, click, wheel, page selection and\n"
           "  the taskbar preferences), and `shell_api.h` gained the shell-side services\n"
           "  Settings needs: the debug-overlay switch, the UI scale, the display profile\n"
           "  list and `append_text`/`append_unsigned`. No app reaches into a kernel static\n"
           "  any more, which is the same contract Notepad already follows.\n"
           "- `main.c` shrank from ~3,100 to ~2,860 lines and is down to the shell's own\n"
           "  concerns. `tools/Makefile` and `tools/Makefile.wsl` both compile the module and\n"
           "  resolve its header; both entry points now produce the same image.\n"
           "\n"
           "\n"
           "## Planned\n"
           "\n"
           "### Kernel\n"
           "\n"
           "- **Populate a real IDT with exception handlers.** `IDT` still reads back as\n"
           "  `0000000000000000 00000000`, so hardware interrupts work only because the\n"
           "  firmware descriptor happens to be in place, and any fault triple-faults\n"
           "  instead of reaching a handler. This is why the `CR4.OSFXSR` bug presented as\n"
           "  an instant shutdown with no diagnostic at all.\n"
           "- Add a panic path that prints the fault vector, error code and the registers\n"
           "  QEMU captured, so a crash is diagnosable from the serial log.\n"
           "- Replace the polled input loop with PIT/HPET timer-driven frame pacing.\n"
           "- Add keyboard shortcuts: Alt+Tab window cycling, a Windows key, and Ctrl+Alt+Del.\n"
           "\n"
           "### Storage\n"
           "\n"
           "- Implement an AHCI (SATA) driver so VMware VMs without a legacy IDE controller\n"
           "  can mount a real volume.\n"
           "- Implement an NVMe driver over PCI for the same reason.\n"
           "- Add a free-sector map so deleted space is reclaimed instead of leaked, and\n"
           "  address the fragmentation the append-only allocator produces over time.\n"
           "- Add a journal so an interrupted metadata write cannot corrupt the volume.\n"
           "- Stop rewriting the whole file table on every write. `fs_save_metadata()`\n"
           "  serialises all 256 records on each `fs_write_file()`, so saving a file costs\n"
           "  the same whether the volume holds three files or three hundred.\n"
           "- Add rename and move, and make `fs_delete_directory()` refuse non-empty\n"
           "  directories instead of deleting the entry and orphaning its children.\n"
           "- Raise the 24 KiB per-file cap in `MAX_FILE_SIZE`, and handle paths that\n"
           "  exceed the 64-character `MAX_FILENAME_LEN` rather than rejecting them.\n"
           "- Add a BIOS-style POST screen so storage detection and capacity are visible\n"
           "  before the desktop loads, instead of only on the serial log.\n"
           "\n"
           "### Shell\n"
           "\n"
           "- Add Explorer sorting by column. The details view draws Name, Date modified,\n"
           "  Type and Size, but only the zone dropdown is interactive so far; the reference\n"
           "  layout shows a sort indicator on the header that does nothing yet.\n"
           "- Expose create, rename and delete in the Explorer UI. The filesystem already\n"
           "  has `fs_create_file`, `fs_delete_file` and `fs_delete_directory`; nothing\n"
           "  calls them.\n"
           "- Give Notepad open/save dialogs instead of the single fixed file path.\n"
           "- Taskbar: window previews on hover, overflow grouping, and show-desktop.\n"
           "- Visually confirm the loading screen; the desktop, taskbar, watermark, Start\n"
           "  menu and Explorer have all been captured as rendered frames.\n"
           "\n"
           "### Settings and time\n"
           "\n"
           "- Persist Settings state so toggles survive a reboot instead of resetting.\n"
           "- Allow the clock to be set. The RTC is read-only today, so the time zone can\n"
           "  only shift the displayed time; nothing corrects a clock that has drifted.\n"
           "- Cover the remaining daylight-saving rules. Iran, Chile, and Lord Howe Island\n"
           "  are approximated by one of the four implemented rules rather than their real\n"
           "  transitions.\n"
           "- Remember each Settings tab's scroll position between visits.\n";
}
