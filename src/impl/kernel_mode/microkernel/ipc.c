#include "../../../intf/stdint.h"
#include "../../../intf/mm.h"
#include "../../../intf/string.h"

typedef struct {
    int in_use;
    int sender_pid;
    int receiver_pid;
    size_t size;
    void* data;
} ipc_message_t;

#define IPC_MAX_MESSAGES 64
static ipc_message_t ipc_queue[IPC_MAX_MESSAGES];
static int ipc_initialized = 0;
static int current_sender_pid = 0;

int send_message(int pid, void* message, size_t size) {
    if (!ipc_initialized || pid < 0 || !message || size == 0) {
        return -1;
    }

    for (int i = 0; i < IPC_MAX_MESSAGES; i++) {
        if (!ipc_queue[i].in_use) {
            void* payload = kmalloc(size);
            if (!payload) {
                return -1;
            }
            memcpy(payload, message, size);

            ipc_queue[i].in_use = 1;
            ipc_queue[i].sender_pid = current_sender_pid;
            ipc_queue[i].receiver_pid = pid;
            ipc_queue[i].size = size;
            ipc_queue[i].data = payload;
            return 0;
        }
    }

    return -1;
}

int receive_message(int* sender_pid, void* buffer, size_t size) {
    if (!ipc_initialized || !buffer || size == 0) {
        return -1;
    }

    for (int i = 0; i < IPC_MAX_MESSAGES; i++) {
        if (ipc_queue[i].in_use) {
            if (size < ipc_queue[i].size) {
                return -1;
            }
            memcpy(buffer, ipc_queue[i].data, ipc_queue[i].size);
            if (sender_pid) {
                *sender_pid = ipc_queue[i].sender_pid;
            }
            kfree(ipc_queue[i].data);
            ipc_queue[i].data = 0;
            ipc_queue[i].in_use = 0;
            return (int)ipc_queue[i].size;
        }
    }

    return -1;
}

void ipc_init() {
    memset(ipc_queue, 0, sizeof(ipc_queue));
    ipc_initialized = 1;
}
