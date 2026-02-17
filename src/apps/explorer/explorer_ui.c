#include "explorer_ui.h"

static const char* g_safe_lines[] = {
    "Explorer Safe Mode",
    "Drive view temporarily",
    "limited for VMware stability.",
    "",
    "C: Local Disk",
    "D: Data"
};

const char* explorer_ui_title(void) {
    return "This PC";
}

int explorer_ui_safe_line_count(void) {
    return (int)(sizeof(g_safe_lines) / sizeof(g_safe_lines[0]));
}

const char* explorer_ui_safe_line(int idx) {
    if (idx < 0 || idx >= explorer_ui_safe_line_count()) return "";
    return g_safe_lines[idx];
}
