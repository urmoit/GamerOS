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

static const char* g_changelog_md_path = "C:/GamerOS/Apps/Settings/CHANGELOG_2026-02-17.MD";

static const char* g_changelog_md_text =
    "# GamerOS Changelog (2026-02-17)\n"
    "\n"
    "This changelog tracks work for the next release cycle.\n"
    "\n"
    "## Release\n"
    "- Version: `00m1`\n"
    "- Build: `1.300`\n"
    "- Date: `2026-02-17`\n"
    "\n"
    "## Added\n"
    "- New application framework module at `src/apps` with centralized built-in app registry.\n"
    "- Per-app source folders under `src/apps` for modular app growth:\n"
    "  - `src/apps/notepad`\n"
    "  - `src/apps/settings`\n"
    "  - `src/apps/explorer`\n"
    "- New source-side app manifests now stored inside each app folder:\n"
    "  - `src/apps/notepad/NOTEPAD.EXE.manifest`\n"
    "  - `src/apps/settings/SETTINGS.EXE.manifest`\n"
    "  - `src/apps/explorer/EXPLORER.EXE.manifest`\n"
    "- App UI content modules added inside each app folder:\n"
    "  - `src/apps/notepad/notepad_ui.c`\n"
    "  - `src/apps/settings/settings_ui.c`\n"
    "  - `src/apps/explorer/explorer_ui.c`\n"
    "- New public app-launch interface `src/intf/apps.h` for executable resolution.\n"
    "\n"
    "## Changed\n"
    "- Build metadata updated from `1.200` to `1.300`.\n"
    "- Build system now compiles and links `src/apps/apps.c`.\n"
    "- Desktop and Start menu launch flow now routes through `.EXE` app descriptors (`NOTEPAD.EXE`, `SETTINGS.EXE`, `EXPLORER.EXE`).\n"
    "- System32 app records switched from `.APP` entries to `.EXE` stubs.\n"
    "- Storage initialization now seeds app paths:\n"
    "  - `C:/GamerOS/Apps`\n"
    "  - `C:/GamerOS/Apps/BuiltIn`\n"
    "- UI scaling groundwork added with runtime scale profile and adaptive Start menu metrics.\n"
    "- Settings UI now shows live runtime resolution and scale profile.\n"
    "- Startup animation motion updated with eased progress timing for smoother feel.\n"
    "- Taskbar visual style updated to a two-tone bar for a more polished desktop look.\n"
    "- Desktop watermark draw order updated so it appears behind app windows/icons instead of over app content.\n"
    "- Notepad UI refreshed with menu-strip hint and stronger status bar treatment.\n"
    "- Settings UI refreshed with modernized content header and app-module-driven tab/changelog content.\n"
    "- Explorer UI refreshed with clearer two-pane layout (left quick access + right content panel).\n"
    "- Start menu metrics path simplified/hardened for VMware stability.\n"
    "- Explorer app restored from safe-static mode to interactive storage-backed navigation.\n"
    "- Settings navigation expanded to GamerOS-style categories:\n"
    "  - System\n"
    "  - Bluetooth & devices\n"
    "  - Network & internet\n"
    "  - Personalization\n"
    "  - Apps\n"
    "  - Accounts\n"
    "  - Time & language\n"
    "  - Gaming\n"
    "  - Accessibility\n"
    "  - Privacy & security\n"
    "  - GamerOS Update\n"
    "  - About\n"
    "- Changelog view moved under `GamerOS Update` (Windows-style naming removed; now GamerOS branding).\n"
    "- All built-in app windows now use a unified modernized Windows XP-style theme (title bars, borders, panels, and controls).\n"
    "- Explorer storage initialization moved off immediate app-open path (now deferred to first drive interaction).\n"
    "- Desktop watermark text updated to new GamerOS build identity wording.\n"
    "- Start menu expanded with additional entries:\n"
    "  - GamerOS Update\n"
    "  - About GamerOS\n"
    "- Settings `GamerOS Update` page now mirrors markdown-style structure and item content from `changelog_2026-02-17.md`.\n"
    "- Startup/loading screen fully remade with a Windows 7-inspired presentation:\n"
    "  - Aero-like background bands\n"
    "  - centered startup panel\n"
    "  - refined progress rail with smoother eased animation\n"
    "  - moving highlight sweep across progress fill\n"
    "- Integrated custom wallpaper asset from PNG into source/runtime path:\n"
    "  - source image: `src/resources/wallpapers/Background.png`\n"
    "  - compiled asset: `src/resources/wallpapers/background_wallpaper.c/.h`\n"
    "  - desktop now renders this wallpaper image as live background\n"
    "- Taskbar clock now shows both time and date in a single compact display (`HH:MM[:SS] DD/MM/YYYY`).\n"
    "\n"
    "## Fixed\n"
    "- Notepad launch path now consistently loads storage-backed document content when launched via `.EXE` entry flow.\n"
    "- Fixed Settings content/header overlap so changelog and section content are readable.\n"
    "- Fixed VMware crash path seen when opening Start menu in latest UI pass.\n"
    "- Fixed Explorer popup regression in VMware when opening Explorer.\n"
    "- Improved Notepad typing responsiveness under fast key bursts by expanding keyboard buffer and avoiding newest-key drops.\n"
    "- Fixed taskbar app-button reservation/hitbox alignment after clock width increase for date display.\n"
    "\n"
    "## Notes\n"
    "- This release introduces executable-style app descriptors and app-folder organization as the base for broader `.EXE` app support.\n"
    "- Explorer now supports `This PC` drive open, folder traversal, and parent navigation using current storage model.\n";

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
