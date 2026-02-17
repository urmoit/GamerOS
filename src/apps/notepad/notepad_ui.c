#include "notepad_ui.h"

const char* notepad_ui_title(void) {
    return "Notepad";
}

const char* notepad_ui_status_saved(void) {
    return "Saved (PgUp Save, PgDn Reload)";
}

const char* notepad_ui_status_modified(void) {
    return "Modified (PgUp Save, PgDn Reload)";
}

const char* notepad_ui_toolbar_hint(void) {
    return "File  Edit  View";
}
