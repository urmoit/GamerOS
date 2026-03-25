#include "../../intf/process_model.h"
#include "../../intf/fs.h"
#include "../../intf/string.h"
#include "../../intf/apps.h"
#include "../../intf/serial.h"

static task_t g_tasks[MAX_TASKS];
static int g_next_pid = 1;
static uint8_t g_loader_buffer[MAX_FILE_SIZE + 1];

static int parse_line_value(const char* text, const char* key, char* out, int out_cap) {
    if (!text || !key || !out || out_cap <= 1) return 0;
    const char* p = strstr(text, key);
    if (!p) return 0;
    p += strlen(key);
    int i = 0;
    while (p[i] && p[i] != '\n' && i < out_cap - 1) {
        out[i] = p[i];
        i++;
    }
    out[i] = 0;
    return i > 0;
}

static int parse_entry_window_type(const char* entry, int fallback_window_type) {
    if (!entry || !entry[0]) return fallback_window_type;
    if (strcmp(entry, "WIN_NOTEPAD") == 0) return APP_WINDOW_NOTEPAD;
    if (strcmp(entry, "WIN_SETTINGS") == 0) return APP_WINDOW_SETTINGS;
    if (strcmp(entry, "WIN_EXPLORER") == 0) return APP_WINDOW_EXPLORER;
    if (strcmp(entry, "WIN_ABOUT") == 0) return APP_WINDOW_ABOUT;
    return fallback_window_type;
}

void process_model_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        g_tasks[i].pid = 0;
        g_tasks[i].name[0] = 0;
        g_tasks[i].exe_path[0] = 0;
        g_tasks[i].window_type = APP_WINDOW_NOTEPAD;
        g_tasks[i].state = TASK_STATE_EMPTY;
    }
    g_next_pid = 1;
}

int process_spawn_from_exe(const char* exe_path, int fallback_window_type) {
    serial_write_string("Loader: spawn request\n");
    if (!exe_path || !exe_path[0]) return -1;
    serial_write_string("Loader: open exe\n");
    file_t* exe = fs_open_file(exe_path);
    if (!exe || exe->size == 0 || exe->size > MAX_FILE_SIZE) return -1;

    serial_write_string("Loader: read exe\n");
    memset(g_loader_buffer, 0, sizeof(g_loader_buffer));
    fs_read_file(exe, g_loader_buffer, exe->size);
    g_loader_buffer[exe->size] = 0;

    // Loader format:
    // GOSAPP\n
    // Name=...\n
    // Entry=WIN_...\n
    serial_write_string("Loader: validate header\n");
    if (strncmp((const char*)g_loader_buffer, "GOSAPP\n", 7) != 0) return -1;

    char task_name[32];
    char entry_name[32];
    if (!parse_line_value((const char*)g_loader_buffer, "Name=", task_name, sizeof(task_name))) {
        strncpy(task_name, "Unnamed", sizeof(task_name) - 1);
        task_name[sizeof(task_name) - 1] = 0;
    }
    if (!parse_line_value((const char*)g_loader_buffer, "Entry=", entry_name, sizeof(entry_name))) {
        entry_name[0] = 0;
    }

    serial_write_string("Loader: allocate task slot\n");
    int window_type = parse_entry_window_type(entry_name, fallback_window_type);

    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].state == TASK_STATE_EMPTY || g_tasks[i].state == TASK_STATE_TERMINATED) {
            g_tasks[i].pid = g_next_pid++;
            strncpy(g_tasks[i].name, task_name, sizeof(g_tasks[i].name) - 1);
            g_tasks[i].name[sizeof(g_tasks[i].name) - 1] = 0;
            strncpy(g_tasks[i].exe_path, exe_path, sizeof(g_tasks[i].exe_path) - 1);
            g_tasks[i].exe_path[sizeof(g_tasks[i].exe_path) - 1] = 0;
            g_tasks[i].window_type = window_type;
            g_tasks[i].state = TASK_STATE_READY;
            serial_write_string("Loader: spawn success\n");
            return g_tasks[i].pid;
        }
    }
    serial_write_string("Loader: no task slots\n");
    return -1;
}

int process_terminate_by_window_type(int window_type) {
    for (int i = MAX_TASKS - 1; i >= 0; i--) {
        if (g_tasks[i].state != TASK_STATE_EMPTY &&
            g_tasks[i].state != TASK_STATE_TERMINATED &&
            g_tasks[i].window_type == window_type) {
            g_tasks[i].state = TASK_STATE_TERMINATED;
            return 1;
        }
    }
    return 0;
}

int process_mark_running(int pid) {
    if (pid <= 0) return 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].pid == pid && g_tasks[i].state == TASK_STATE_READY) {
            g_tasks[i].state = TASK_STATE_RUNNING;
            return 1;
        }
    }
    return 0;
}

int process_task_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].state != TASK_STATE_EMPTY && g_tasks[i].state != TASK_STATE_TERMINATED) {
            count++;
        }
    }
    return count;
}

const task_t* process_task_get(int idx) {
    if (idx < 0) return 0;
    int seen = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].state != TASK_STATE_EMPTY && g_tasks[i].state != TASK_STATE_TERMINATED) {
            if (seen == idx) return &g_tasks[i];
            seen++;
        }
    }
    return 0;
}
