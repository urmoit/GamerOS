#include "notepad_ui.h"

const char* notepad_ui_title(void) {
    return "Notepad";
}

const char* notepad_ui_status_saved(void) {
    return "All changes saved  |  PgUp Save  PgDn Reload";
}

const char* notepad_ui_status_modified(void) {
    return "Unsaved changes  |  PgUp Save  PgDn Reload";
}

const char* notepad_ui_toolbar_hint(void) {
    return "File  Edit  Format  View  Help";
}
