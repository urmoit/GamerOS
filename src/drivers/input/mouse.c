#include <mouse.h>
#include <ports.h>
#include <graphics.h>
#include <pic.h>

#define MOUSE_DATA_PORT    0x60
#define MOUSE_STATUS_PORT  0x64
#define MOUSE_CMD_PORT     0x64

#define MOUSE_IO_TIMEOUT   100000

static int32_t mouse_x = 160;
static int32_t mouse_y = 100;
static uint8_t mouse_buttons = 0;
static int8_t mouse_wheel_delta = 0;
static uint8_t mouse_has_wheel = 0;
static uint8_t mouse_cycle = 0;
static uint8_t mouse_present = 0;
static uint8_t mouse_packet[4];

int mouse_is_present(void);

static void mouse_apply_motion(int16_t dx, int16_t dy) {
    int32_t nx = (int32_t)mouse_x + dx;
    int32_t ny = (int32_t)mouse_y + dy;

    if (nx < 0) nx = 0;
    if (ny < 0) ny = 0;
    if (nx >= (int32_t)current_width - CURSOR_SIZE) nx = (int32_t)current_width - CURSOR_SIZE - 1;
    if (ny >= (int32_t)current_height - CURSOR_SIZE) ny = (int32_t)current_height - CURSOR_SIZE - 1;

    mouse_x = nx;
    mouse_y = ny;
}

static inline int16_t clamp_mouse_delta(int16_t v) {
    if (v > 12) return 12;
    if (v < -12) return -12;
    return v;
}

static uint8_t mouse_read_data_timeout(uint8_t* out) {
    for (int i = 0; i < MOUSE_IO_TIMEOUT; i++) {
        if (inb(MOUSE_STATUS_PORT) & 0x01) {
            if (out) {
                *out = inb(MOUSE_DATA_PORT);
            } else {
                (void)inb(MOUSE_DATA_PORT);
            }
            return 1;
        }
    }
    return 0;
}

void mouse_send_cmd(uint8_t cmd) {
    for (int i = 0; i < MOUSE_IO_TIMEOUT; i++) {
        if (!(inb(MOUSE_STATUS_PORT) & 0x02)) break;
    }
    outb(MOUSE_CMD_PORT, 0xD4);
    for (int i = 0; i < MOUSE_IO_TIMEOUT; i++) {
        if (!(inb(MOUSE_STATUS_PORT) & 0x02)) break;
    }
    outb(MOUSE_DATA_PORT, cmd);
}

static void mouse_drain_ack(void) {
    for (int i = 0; i < 4; i++) {
        if (!mouse_read_data_timeout(0)) break;
    }
}

void mouse_init(void) {
    mouse_x = current_width / 2;
    mouse_y = current_height / 2;
    mouse_buttons = 0;
    mouse_wheel_delta = 0;
    mouse_has_wheel = 0;
    mouse_cycle = 0;

    outb(MOUSE_CMD_PORT, 0xA8); // Enable auxiliary device
    io_wait();

    outb(MOUSE_CMD_PORT, 0x20); // Read command byte
    io_wait();
    uint8_t status = inb(MOUSE_DATA_PORT);
    if (status == 0xFF || status == 0x00) {
        mouse_present = 0;
        return;
    }

    status |= 2; // Enable IRQ12
    outb(MOUSE_CMD_PORT, 0x60); // Write command byte
    io_wait();
    outb(MOUSE_DATA_PORT, status);
    io_wait();

    mouse_present = 1;

    mouse_send_cmd(0xF6); // Set defaults
    mouse_drain_ack();

    // Enable IntelliMouse wheel packets (4-byte PS/2 packet).
    mouse_send_cmd(0xF3);
    mouse_send_cmd(200);
    mouse_drain_ack();
    mouse_send_cmd(0xF3);
    mouse_send_cmd(100);
    mouse_drain_ack();
    mouse_send_cmd(0xF3);
    mouse_send_cmd(80);
    mouse_drain_ack();

    mouse_send_cmd(0xF2); // Get mouse ID
    uint8_t b = 0;
    uint8_t id = 0;
    if (mouse_read_data_timeout(&b)) {   // ACK (usually 0xFA)
        if (mouse_read_data_timeout(&id)) {
            mouse_has_wheel = (id == 0x03 || id == 0x04) ? 1 : 0;
        }
    }

    mouse_send_cmd(0xF4); // Enable streaming
    mouse_drain_ack();

    // Unmask IRQ2 (cascade) on master PIC and IRQ12 on slave PIC
    uint8_t master_mask = inb(PIC1_DATA);
    master_mask &= ~(1 << 2);
    outb(PIC1_DATA, master_mask);

    uint8_t slave_mask = inb(PIC2_DATA);
    slave_mask &= ~(1 << 4); // Unmask IRQ12 (mouse is IRQ12 - bit 4 on slave)
    outb(PIC2_DATA, slave_mask);
}

void mouse_handler(void) {
    if (!mouse_present) return;
    uint8_t status = inb(MOUSE_STATUS_PORT);
    if (!(status & 0x01)) return;
    // Bit 5 clear = keyboard byte; drain it, not a mouse packet.
    if (!(status & 0x20)) { inb(MOUSE_DATA_PORT); return; }
    
    uint8_t data = inb(MOUSE_DATA_PORT);
    
    switch (mouse_cycle) {
        case 0:
            if (data & 0x08) { // Valid packet start
                mouse_packet[0] = data;
                mouse_cycle = 1;
            }
            break;
        case 1:
            mouse_packet[1] = data;
            mouse_cycle = 2;
            break;
        case 2:
            mouse_packet[2] = data;
            if (mouse_has_wheel) {
                mouse_cycle = 3;
            } else {
                if (mouse_packet[0] & 0xC0) {
                    mouse_cycle = 0;
                    break;
                }
                mouse_buttons = mouse_packet[0] & 0x07;

                // PS/2 dy is positive when the mouse moves up; negate it.
                mouse_apply_motion(clamp_mouse_delta((int8_t)mouse_packet[1]),
                                   (int16_t)(-clamp_mouse_delta((int8_t)mouse_packet[2])));

                mouse_cycle = 0;
            }
            break;
        case 3:
            mouse_packet[3] = data;
            if (mouse_packet[0] & 0xC0) {
                mouse_cycle = 0;
                break;
            }
            mouse_buttons = mouse_packet[0] & 0x07;

            {
                int16_t dx = clamp_mouse_delta((int8_t)mouse_packet[1]);
                int16_t dy = clamp_mouse_delta((int8_t)mouse_packet[2]);

                mouse_apply_motion(dx, (int16_t)-dy);
            }

            {
                int8_t wheel = (int8_t)mouse_packet[3];
                if (wheel != 0) {
                    int16_t acc = (int16_t)mouse_wheel_delta + wheel;
                    if (acc > 64) acc = 64;
                    if (acc < -64) acc = -64;
                    mouse_wheel_delta = (int8_t)acc;
                }
            }
            mouse_cycle = 0;
            break;
    }
}

void mouse_poll(void) {
    int safety = 64;
    while (safety-- > 0) {
        uint8_t status = inb(MOUSE_STATUS_PORT);
        if (!(status & 0x01)) break;
        // Bit 5 clear = keyboard byte; leave it in place.
        if (!(status & 0x20)) break;
        mouse_handler();
    }
}

int32_t mouse_get_x(void) { return mouse_x; }
int32_t mouse_get_y(void) { return mouse_y; }
uint8_t mouse_get_buttons(void) { return mouse_buttons; }
mouse_state_t mouse_get_state(void) {
    mouse_state_t state;
    state.x = mouse_x;
    state.y = mouse_y;
    state.buttons = mouse_buttons;
    return state;
}
int8_t mouse_get_wheel_delta(void) {
    int8_t d = mouse_wheel_delta;
    mouse_wheel_delta = 0;
    return d;
}

void mouse_clear_buttons(void) { mouse_buttons = 0; }

int mouse_is_present(void) { return mouse_present ? 1 : 0; }

// --- Non-PS/2 input sources ---

void mouse_set_position(int32_t x, int32_t y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (int32_t)current_width - CURSOR_SIZE) x = (int32_t)current_width - CURSOR_SIZE - 1;
    if (y >= (int32_t)current_height - CURSOR_SIZE) y = (int32_t)current_height - CURSOR_SIZE - 1;
    mouse_x = x;
    mouse_y = y;
}

void mouse_inject_relative(int16_t dx, int16_t dy, uint8_t buttons) {
    mouse_buttons = buttons;
    mouse_apply_motion(clamp_mouse_delta(dx), clamp_mouse_delta(dy));
}

void mouse_set_buttons(uint8_t buttons) {
    mouse_buttons = buttons;
}

// Feed the shared accumulator the PS/2 wheel uses.
void mouse_inject_wheel(int8_t wheel) {
    if (wheel == 0) return;
    int16_t acc = (int16_t)mouse_wheel_delta + wheel;
    if (acc > 64) acc = 64;
    if (acc < -64) acc = -64;
    mouse_wheel_delta = (int8_t)acc;
}

// TODO: Add runtime-configurable pointer sensitivity and acceleration profiles.
// TODO: Add optional movement smoothing filter for high-jitter PS/2 devices.
