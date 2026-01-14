#include "../../../intf/stdint.h"

// Basic IPC stubs - to be implemented later

int send_message(int pid, void* message, size_t size) {
    // Stub: always fail
    return -1;
}

int receive_message(int* sender_pid, void* buffer, size_t size) {
    // Stub: no messages
    return -1;
}

void ipc_init() {
    // Stub: nothing to initialize
}