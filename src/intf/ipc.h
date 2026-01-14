#ifndef IPC_H
#define IPC_H

#include "stdint.h"

// Basic IPC message structure
typedef struct {
    int sender_pid;
    int receiver_pid;
    size_t size;
    void* data;
} message_t;

// IPC functions
int send_message(int pid, void* message, size_t size);
int receive_message(int* sender_pid, void* buffer, size_t size);
void ipc_init();

#endif