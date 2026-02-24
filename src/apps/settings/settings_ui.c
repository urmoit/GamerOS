#include "settings_ui.h"

static const char* g_tabs[] = {
    "System",
    "Bluetooth & devices",
    "Network & internet",
    "Personalization",
    "Apps",
    "Accounts",
    "Time & language",
    "Gaming",
    "Accessibility",
    "Privacy & security",
    "GamerOS Update",
    "About",
};

static const char* g_changelog_md_path = "C:/GamerOS/Apps/Settings/CHANGELOG_2026-XX-XX.MD";

static const char* g_changelog_md_text =
    "# GamerOS Changelog (TBD)\n"
    "\n"
    "This changelog tracks work for the next release cycle.\n"
    "\n"
    "## Release\n"
    "- Version: `00m1`\n"
    "- Build: `1.400`\n"
    "- Date: `TBD`\n"
    "\n"
    "## Added\n"
    "- True-color framebuffer path added (24/32bpp linear framebuffer support).\n"
    "- New graphics API helpers for RGB drawing:\n"
    "  - `draw_pixel_rgb(...)`\n"
    "  - `clear_screen_rgb(...)`\n"
    "- Added explicit video mode enum target `MODE_VESA_1920x1080`.\n"
    "\n"
    "## Changed\n"
    "- Build metadata updated from `1.300` to `1.400`.\n"
    "- Desktop bottom accent above taskbar changed from dual blue glow bands to a subtle neutral shadow for cleaner wallpaper blending.\n"
    "- Startup/loading screen fully redesigned with modern card layout, rounded/chamfered corners, and updated progress rail treatment.\n"
    "- Branding text updated to remove Windows-style wording in favor of GamerOS-native naming.\n"
    "- Core shell UI modernized to match loading screen direction:\n"
    "  - window chrome and controls updated with rounded/chamfered surfaces\n"
    "  - Start menu restyled with modern panel/button treatment\n"
    "  - taskbar, task buttons, and clock updated to modernized geometry\n"
    "  - Settings theme label updated from legacy XP naming to GamerOS Modern\n"
    "- Boot flow now prefers bootloader-provided framebuffer and keeps that mode into kernel startup.\n"
    "- Graphics backend now safely activates true-color mode whenever a valid bootloader framebuffer is provided.\n"
    "- Graphics pipeline compatibility pass completed so legacy UI color IDs and cursor composition behave correctly on both indexed VGA and true-color framebuffer paths.\n"
    "- Boot header now explicitly requests `1920x1080x32` graphics mode again for Full HD true-color startup when supported.\n"
    "- Settings `Gaming`/`About` now display live graphics backend mode, bpp, and resolution (including RGBA indicator).\n"
    "- Start menu layout refreshed for clarity with cleaner cards, search strip, and stronger action hierarchy.\n"
    "- Loading screen background reworked to a smoother gradient-style presentation.\n"
    "- Loading indicator switched from linear bar to a circular progress loader.\n"
    "- Completed full-shell modern visual pass with a bright unified style:\n"
    "  - startup card and spinner recolored for a clean contemporary boot look\n"
    "  - Start menu restyled with profile chip, light surfaces, and neutral shutdown action\n"
    "  - taskbar, Start button, task buttons, and clock moved to light modern treatment\n"
    "  - desktop icon labels and watermark tuned for better readability on bright backgrounds\n"
    "\n"
    "## Fixed\n"
    "- Fixed wallpaper/taskbar transition mismatch caused by the two blue desktop glow strips.\n"
    "- Fixed stale About build text (now shows `1.400`).\n"
    "- Fixed hard lock to legacy 16-color planar rendering path by adding true-color framebuffer output path when available.\n"
    "- Fixed VMware startup crash path by hardening multiboot framebuffer validation and forcing VGA fallback when framebuffer memory/type/range is unsafe.\n"
    "- Fixed true-color color mapping bug where non-`0x0F` palette IDs were truncated.\n"
    "- Fixed cursor background restore artifacts on true-color backend by switching cursor underlay capture/restore to RGB-aware path.\n"
    "- Fixed boot-time triple-fault by relocating early page tables away from kernel image/BSS memory range.\n"
    "- Fixed early paging initialization robustness by allocating P4/P3/P2 tables as page-aligned kernel BSS symbols instead of fixed physical addresses.\n"
    "- Fixed ultra-dark desktop/UI rendering in true-color mode by applying shell palette mappings during non-VGA palette initialization.\n"
    "- Fixed near-black desktop visibility when wallpaper asset is too dark by adding adaptive bright modern wallpaper fallback rendering.\n"
    "\n"
    "## Notes\n"
    "- Release date is not finalized yet.\n"
    "\n"
;

int settings_ui_tab_count(void) {
    return (int)(sizeof(g_tabs) / sizeof(g_tabs[0]));
}

const char* settings_ui_tab_name(int idx) {
    if (idx < 0 || idx >= settings_ui_tab_count()) return "";
    return g_tabs[idx];
}

const char* settings_ui_panel_title(void) {
    return "Settings";
}

const char* settings_ui_changelog_md_path(void) {
    return g_changelog_md_path;
}

const char* settings_ui_changelog_md_text(void) {
    return g_changelog_md_text;
}

