#ifndef PROCESS_MODEL_H
#define PROCESS_MODEL_H

#include "stdint.h"

#define MAX_TASKS 32

typedef enum {
    TASK_STATE_EMPTY = 0,
    TASK_STATE_READY = 1,
    TASK_STATE_RUNNING = 2,
    TASK_STATE_TERMINATED = 3
} task_state_t;

typedef struct {
    int pid;
    char name[32];
    char exe_path[40];
    int window_type;
    task_state_t state;
} task_t;

void process_model_init(void);
int process_spawn_from_exe(const char* exe_path, int fallback_window_type);
int process_terminate_by_window_type(int window_type);
int process_mark_running(int pid);
int process_task_count(void);
const task_t* process_task_get(int idx);

#endif
