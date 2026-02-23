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
    "- TBD\n"
    "\n"
    "## Changed\n"
    "- Build metadata updated from `1.300` to `1.400`.\n"
    "\n"
    "## Fixed\n"
    "- TBD\n"
    "\n"
    "## Notes\n"
    "- Release date is not finalized yet.\n";

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
