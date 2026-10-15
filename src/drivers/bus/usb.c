#include <usb.h>
#include <ports.h>
#include <pci.h>
#include <keyboard.h>
#include <mouse.h>
#include <serial.h>
#include <string.h>

// ============================================================================
// USB HID stack: UHCI host controller, device enumeration and boot protocol
// keyboards and mice.
//
// The PS/2 drivers cover everything wired to the 8042, but a machine with USB
// keyboards (or a VM with usb-kbd / usb-mouse on the virtual UHCI controller)
// has no 8042 device to talk to. This module covers that: it brings up each
// UHCI controller, walks its root hub ports, enumerates HID devices, switches
// them to the boot protocol, and reports their 8-byte keyboard reports and
// 4-byte mouse reports into the same shared input state the shell reads.
//
// Interrupts are disabled while the desktop loop runs (the shell polls input),
// so the interrupt transfers are polled too: a transfer descriptor stops being
// "active" when the host controller finishes it, and the buffer it wrote is the
// report. Control transfers are synchronous for the same reason.
//
// Scope: UHCI only. EHCI covers high-speed bulk traffic, which HID boot devices
// (full or low speed) do not use, and xHCI needs its own TRB machinery, so the
// PCI probe skips those controllers rather than half-driving them.
// ============================================================================

// ----------------------------------------------------------------------------
// UHCI register map (offsets from the controller's I/O base in BAR4)
// ----------------------------------------------------------------------------
#define UHCI_USBCMD        0x00
#define UHCI_USBSTS        0x02
#define UHCI_USBINTR       0x04
#define UHCI_FRNUM         0x06
#define UHCI_FRBASEADD     0x08
#define UHCI_PORTSC(port)  (0x10 + (port) * 2)

// Port status and control bits.
#define UHCI_PORT_CCS      0x0001
#define UHCI_PORT_CSC      0x0002
#define UHCI_PORT_PE       0x0004
#define UHCI_PORT_PEC      0x0008
#define UHCI_PORT_LSDA     0x0100
#define UHCI_PORT_PR       0x0200

#define UHCI_CMD_RS        0x0001
#define UHCI_CMD_HCRESET   0x0002
#define UHCI_CMD_GRESET    0x0004
#define UHCI_CMD_CF        0x0040
#define UHCI_CMD_MAXP      0x0080

// Queue head / transfer descriptor link pointer flags.
#define LINK_TERMINATE     0x00000001u
#define LINK_QH            0x00000002u
#define LINK_DEPTH         0x00000004u

// Transfer descriptor control and status. UHCI keeps the status bits at the top
// of the word and the actual transferred length in the bottom 11 bits, stored as
// length - 1.
#define TD_CTRL_CRCTIMEO   (1u << 18)
#define TD_CTRL_SPD        (1u << 29)   // short packet detect
#define TD_CTRL_BABBLE     (1u << 20)
#define TD_CTRL_DBUFERR    (1u << 21)
#define TD_CTRL_STALLED    (1u << 22)
#define TD_CTRL_ACTIVE     (1u << 23)
#define TD_CTRL_IOC        (1u << 24)
#define TD_CTRL_LS         (1u << 26)
#define TD_CTRL_C_ERR3     (3u << 27)
#define TD_ACTLEN(td)      (((td)->status & 0x7FF) + 1)

// USB packet PIDs. A UHCI token keeps the whole 8-bit PID byte in its low byte,
// with the device address at bits 8-14, the endpoint at 15-18, the data toggle
// at 19 and the maximum length in the top 11 bits. Linux's uhci-hcd.h and
// QEMU's UHCI model both read the token this way, so this is the on-the-wire
// encoding, not a simplification.
#define USB_PID_OUT        0xE1u
#define USB_PID_IN         0x69u
#define USB_PID_SETUP      0x2Du
#define TD_TOKEN_TOGGLE    (1u << 19)

// Expected length is stored as (length - 1). Length zero stores 0x7FF, which
// decodes back to zero, so a zero-length status packet works.
#define TD_TOKEN_MAXLEN(l) ((((uint32_t)(l) - 1u) & 0x7FFu) << 21)
#define TD_TOKEN_DEVADDR(a) (((uint32_t)(a) & 0x7Fu) << 8)
#define TD_TOKEN_EP(e)     (((uint32_t)(e) & 0x0Fu) << 15)

typedef struct {
    uint32_t link;    // next queue in the schedule
    uint32_t elem;    // first transfer descriptor of this queue
} __attribute__((packed, aligned(16))) uhci_qh_t;

typedef struct {
    uint32_t link;
    uint32_t status;  // control and status
    uint32_t token;
    uint32_t buffer;  // address of the data buffer
} __attribute__((packed, aligned(16))) uhci_td_t;

// A HID device or a hub attached to a port.
//
// Only one interrupt transfer descriptor is queued per device at a time. A ring
// of descriptors would leave several IN transfers outstanding at once, and when
// a report arrives the controller completes the whole ring with the same bytes,
// applying one mouse move several times over.
//
// A device either hangs off the root hub of a controller (parent < 0) or off a
// port of one of the hubs in the same controller's list (parent is the hub's
// slot index), which is what makes devices behind a hub reachable. Same
// full-speed devices behind a full-speed hub need no split transfers, so the
// queue is shared: a device behind a hub is addressed exactly like a root
// device, and the parent only matters for the requests that manage the port.
typedef struct {
    uint8_t present;
    uint8_t is_keyboard;
    uint8_t is_hub;
    uint8_t address;      // USB device address, 1..127
    uint8_t low_speed;
    uint8_t endpoint;     // interrupt IN endpoint number
    uint16_t max_packet;
    uint8_t report_size;  // HID: report length in bytes
    uint8_t toggle;       // next data toggle for the interrupt endpoint
    uint8_t shift;        // shift or caps lock, keyboard reports only
    uint8_t caps_lock;
    uint8_t parent;       // slot index of the parent hub, 0xFF for the root hub
    uint8_t port_index;   // 0-based port on the root hub or the parent hub
    uint8_t nports;       // hubs only: number of downstream ports
    uint8_t prev_report[USB_BOOT_KEYBOARD_REPORT];
    uint8_t prev_change[8];   // hubs only: last change bitmap, for edge detection
    uhci_qh_t qh;
    uhci_td_t td;
    uint8_t buffer[USB_BOOT_KEYBOARD_REPORT];
} __attribute__((aligned(16))) usb_hid_dev_t;

#define USB_DEV_PER_HC 8
#define USB_PARENT_ROOT 0xFF

typedef struct {
    uint16_t io_base;
    uint8_t initialized;
    uint32_t frame_list[1024] __attribute__((aligned(4096)));
    uhci_qh_t control_qh;
    uhci_td_t control_tds[5];
    usb_hid_dev_t devices[USB_DEV_PER_HC];
} uhci_hc_t;

#define USB_HC_COUNT 4
static uhci_hc_t hcs[USB_HC_COUNT];
static int hc_count = 0;

// Physical addresses: the kernel identity-maps the first 4 GiB, and every
// descriptor, buffer and queue lives inside that range, so the kernel virtual
// address is also the bus address.
static uint32_t usb_phys(void* p) {
    return (uint32_t)(uintptr_t)p;
}

// Compiler barrier. The host controller and this code share one core, and x86
// already orders stores, so the barrier only has to stop the compiler from
// moving descriptor writes across the schedule update.
static void usb_barrier(void) {
    __asm__ volatile("" ::: "memory");
}

// ----------------------------------------------------------------------------
// Timing
//
// The UHCI frame counter advances once per 1 ms frame while the schedule runs,
// which is a portable millisecond clock that needs no busy-loop calibration.
// ----------------------------------------------------------------------------

static uint16_t uhci_frame(uint16_t io_base) {
    return inw(io_base + UHCI_FRNUM) & 0x3FF;
}

static void uhci_wait_ms(uint16_t io_base, uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        uint16_t start = uhci_frame(io_base);
        // Spin until the counter leaves the frame it started in. Snapshots are
        // compared, never accumulated, so the 1024-frame wrap is harmless. The
        // iteration bound is what keeps a stopped schedule from spinning here
        // forever: the counter never advances when the controller is halted.
        uint16_t now = start;
        for (uint32_t spin = 0; now == start && spin < 200000; spin++) {
            now = uhci_frame(io_base);
        }
    }
}

// Bounded spin for the delays that happen before the schedule (and therefore the
// frame counter) is running.
static void uhci_wait_loops(uint32_t iterations) {
    for (volatile uint32_t i = 0; i < iterations; i++) {
        __asm__ volatile("pause");
    }
}

// Wait for a transfer descriptor to stop being active. Returns 1 on completion
// and 0 on timeout.
static int uhci_wait_td(uhci_td_t* td, uint16_t io_base, uint32_t timeout_ms) {
    for (uint32_t i = 0; i < timeout_ms; i++) {
        usb_barrier();
        if (!(td->status & TD_CTRL_ACTIVE)) {
            return 1;
        }
        uhci_wait_ms(io_base, 1);
    }
    return 0;
}

// ----------------------------------------------------------------------------
// Host controller startup and port reset
// ----------------------------------------------------------------------------

static int uhci_hc_init(uhci_hc_t* hc, pci_device_t* pci) {
    uint32_t bar4 = pci_get_bar(pci, 4);
    uint16_t io_base = (uint16_t)(bar4 & 0xFFFE);
    if (!io_base) {
        return 0;
    }

    pci_enable_bus_mastering(pci);

    hc->io_base = io_base;
    for (int i = 0; i < USB_DEV_PER_HC; i++) {
        hc->devices[i].present = 0;
    }

    // Stop the controller before touching the schedule: it may be running a
    // frame list left behind by the previous loader. Wait for it to report
    // halted, otherwise the resets below can race with a frame still in flight.
    outw(io_base + UHCI_USBCMD, 0);
    for (int i = 0; i < 100000; i++) {
        if (inw(io_base + UHCI_USBSTS) & 0x0020) break;   // Host Controller Halted
        uhci_wait_loops(200);
    }
    outw(io_base + UHCI_USBINTR, 0);      // no interrupt line: input is polled
    outw(io_base + UHCI_USBSTS, 0xFFFF);

    // Global reset of the root hubs, then a controller reset. Both take real
    // time: the spec wants the global reset held for tens of milliseconds, and
    // the controller reset bit clears itself when it completes.
    outw(io_base + UHCI_USBCMD, UHCI_CMD_GRESET);
    uhci_wait_loops(500000);
    outw(io_base + UHCI_USBCMD, 0);
    uhci_wait_loops(100000);

    outw(io_base + UHCI_USBCMD, UHCI_CMD_HCRESET);
    int reset_ok = 0;
    for (int i = 0; i < 500; i++) {
        if (!(inw(io_base + UHCI_USBCMD) & UHCI_CMD_HCRESET)) {
            reset_ok = 1;
            break;
        }
        uhci_wait_loops(2000);
    }
    if (!reset_ok) {
        serial_write_string("USB: hc reset failed\n");
        return 0;
    }

    // Idle schedule: one control queue at the head. Its element pointer is
    // terminated, so an idle frame is over as soon as the controller reaches it.
    for (int i = 0; i < 1024; i++) {
        hc->frame_list[i] = usb_phys(&hc->control_qh) | LINK_QH;
    }
    hc->control_qh.link = LINK_TERMINATE | LINK_QH;
    hc->control_qh.elem = LINK_TERMINATE;

    outw(io_base + UHCI_USBSTS, 0xFFFF);
    outl(io_base + UHCI_FRBASEADD, usb_phys(hc->frame_list));
    outw(io_base + UHCI_FRNUM, 0);
    // Run, mark the schedule configured, and declare 64-byte maximum packets.
    outw(io_base + UHCI_USBCMD, UHCI_CMD_RS | UHCI_CMD_CF | UHCI_CMD_MAXP);

    hc->initialized = 1;
    return 1;
}

// Acknowledge any latched change bits on a port without disturbing the bits
// this driver configured. A port register write keeps only the read-only bits
// of the current value, so writing a change mask alone would also clear the
// enable and reset bits; a read-modify-write preserves them.
static void uhci_port_ack(uint16_t io_base, int port_index) {
    uint16_t reg = io_base + UHCI_PORTSC(port_index);
    uint16_t cur = inw(reg);
    outw(reg, cur | UHCI_PORT_CSC | UHCI_PORT_PEC);
}

// Reset and enable one root hub port. Returns 1 when a device is connected and
// enabled, 0 when the port is empty or the device never enabled, and reports the
// device speed through the out parameter.
static int uhci_port_ready(uhci_hc_t* hc, int port_index, uint8_t* low_speed) {
    uint16_t io_base = hc->io_base;
    uint16_t reg = io_base + UHCI_PORTSC(port_index);
    uint16_t status = inw(reg);

    *low_speed = 0;
    if (!(status & UHCI_PORT_CCS)) {
        return 0;
    }

    // Hold the reset for at least 10 ms (50 ms covers the devices that need the
    // longer recovery window). The reset signal itself deasserts on hardware;
    // the write that follows drops any state this driver added.
    outw(reg, UHCI_PORT_PR);
    uhci_wait_ms(io_base, 50);
    outw(reg, 0);

    // Enable the port explicitly. Some controllers re-enable it themselves once
    // the reset completes, others never do, and a low-speed device needs it
    // written by hand either way. Poll for the enable bit rather than trusting
    // a fixed delay.
    int enabled = 0;
    for (int i = 0; i < 100 && !enabled; i++) {
        outw(reg, UHCI_PORT_PE);
        for (int spin = 0; spin < 1000; spin++) {
            status = inw(reg);
            if (status & UHCI_PORT_PE) {
                enabled = 1;
                break;
            }
            uhci_wait_loops(200);
        }
    }
    *low_speed = (status & UHCI_PORT_LSDA) ? 1 : 0;

    uhci_port_ack(io_base, port_index);
    return enabled ? 1 : 0;
}

// ----------------------------------------------------------------------------
// Transfers
// ----------------------------------------------------------------------------

// Build and run a control transfer through the controller's control queue.
// Returns the number of bytes an IN request actually transferred, 0 for a
// no-data request, and -1 when the transfer failed.
static int uhci_control(uhci_hc_t* hc, usb_hid_dev_t* dev,
                        uint8_t request_type, uint8_t request,
                        uint16_t value, uint16_t index, uint16_t length,
                        void* data) {
    if (!hc->initialized) return -1;

    // The setup packet is a local because the transfer is fully waited for
    // before this frame returns.
    uint8_t setup[8];
    setup[0] = request_type;
    setup[1] = request;
    setup[2] = (uint8_t)(value & 0xFF);
    setup[3] = (uint8_t)((value >> 8) & 0xFF);
    setup[4] = (uint8_t)(index & 0xFF);
    setup[5] = (uint8_t)((index >> 8) & 0xFF);
    setup[6] = (uint8_t)(length & 0xFF);
    setup[7] = (uint8_t)((length >> 8) & 0xFF);

    uint8_t data_pid = (request_type & 0x80) ? USB_PID_IN : USB_PID_OUT;
    // The status stage always travels opposite to the data stage.
    uint8_t status_pid = (length > 0 && (request_type & 0x80)) ? USB_PID_OUT : USB_PID_IN;
    int dir_in = (request_type & 0x80) ? 1 : 0;

    uhci_td_t* td = hc->control_tds;
    uint32_t dest = TD_TOKEN_DEVADDR(dev->address) | TD_TOKEN_EP(0);
    uint32_t status_flags = TD_CTRL_C_ERR3 | (dev->low_speed ? TD_CTRL_LS : 0);

    // SETUP stage: DATA0, no short packet detect, and eight bytes of packet.
    td[0].link = usb_phys(&td[1]);
    td[0].status = TD_CTRL_ACTIVE | status_flags;
    td[0].token = dest | USB_PID_SETUP | TD_TOKEN_MAXLEN(8);
    td[0].buffer = usb_phys(setup);

    // The data stage travels in the request's direction and starts on DATA1. A
    // short packet completes the stage, which lets a device that knows it has
    // nothing more to send end the transfer early.
    uint32_t data_status = status_flags;
    if (dir_in && length > 0) {
        data_status |= TD_CTRL_SPD;
    }

    if (length > 0) {
        td[1].link = usb_phys(&td[2]);
        td[1].status = TD_CTRL_ACTIVE | data_status;
        td[1].token = dest | data_pid | TD_TOKEN_TOGGLE | TD_TOKEN_MAXLEN(length);
        td[1].buffer = usb_phys(data);

        // The status stage reverses direction and always uses DATA1.
        td[2].link = LINK_TERMINATE;
        td[2].status = TD_CTRL_ACTIVE | TD_CTRL_IOC | status_flags;
        td[2].token = dest | status_pid | TD_TOKEN_TOGGLE | TD_TOKEN_MAXLEN(0);
        td[2].buffer = 0;
    } else {
        // No data stage: the status packet still uses DATA1.
        td[1].link = LINK_TERMINATE;
        td[1].status = TD_CTRL_ACTIVE | TD_CTRL_IOC | status_flags;
        td[1].token = dest | status_pid | TD_TOKEN_TOGGLE | TD_TOKEN_MAXLEN(0);
        td[1].buffer = 0;
    }
    usb_barrier();

    hc->control_qh.elem = usb_phys(&td[0]);
    usb_barrier();

    int status_index = (length > 0) ? 2 : 1;
    if (!uhci_wait_td(&td[status_index], hc->io_base, 1000)) {
        // Abort: the caller expects a finished chain, and leaving descriptors
        // active under a queue the interrupt ring shares would corrupt it.
        for (int i = 0; i <= status_index; i++) {
            td[i].status = 0;
        }
        hc->control_qh.elem = LINK_TERMINATE;
        usb_barrier();
        return -1;
    }

    int32_t transferred = 0;
    if (length > 0 && dir_in) {
        transferred = (int32_t)TD_ACTLEN(&td[1]);
        if (transferred > length) transferred = length;
        if (td[1].status & (TD_CTRL_STALLED | TD_CTRL_DBUFERR | TD_CTRL_BABBLE)) {
            transferred = -1;
        }
    }

    hc->control_qh.elem = LINK_TERMINATE;
    usb_barrier();
    return transferred;
}

// Arm the interrupt transfer descriptor. It is re-armed only after a report has
// been consumed, which keeps exactly one request in flight: the controller moves
// on to the next queue while this one is inactive, and watches for it to become
// active again.
static void hid_rearm(usb_hid_dev_t* dev) {
    uhci_td_t* td = &dev->td;

    td->status = TD_CTRL_ACTIVE | TD_CTRL_C_ERR3 | (dev->low_speed ? TD_CTRL_LS : 0);
    td->token = TD_TOKEN_MAXLEN(dev->report_size) | USB_PID_IN |
                (TD_TOKEN_TOGGLE * dev->toggle) |
                TD_TOKEN_DEVADDR(dev->address) | TD_TOKEN_EP(dev->endpoint);
    td->buffer = usb_phys(dev->buffer);
    usb_barrier();

    // Data toggle alternates for every completed transfer, so the device's
    // sequence stays in step with the host's request.
    dev->toggle = (uint8_t)(dev->toggle ^ 1);
}

static void hid_start_interrupt(usb_hid_dev_t* dev) {
    dev->toggle = 0;

    dev->td.link = LINK_TERMINATE;
    hid_rearm(dev);

    dev->qh.link = LINK_TERMINATE | LINK_QH;
    dev->qh.elem = usb_phys(&dev->td);
    usb_barrier();
}

// ----------------------------------------------------------------------------
// HID report handling
// ----------------------------------------------------------------------------

// USB HID key usage -> character, unshifted. Usage IDs come from the HID Usage
// Tables keyboard page: 0x04-0x1D are a-z, 0x1E-0x27 are 1-9 and 0, and the rest
// of the printable keys follow the physical layout.
static char hid_usage_ascii(uint8_t usage) {
    if (usage >= 0x04 && usage <= 0x1D) {
        return (char)('a' + (usage - 0x04));
    }
    if (usage >= 0x1E && usage <= 0x27) {
        return "1234567890"[usage - 0x1E];
    }
    switch (usage) {
        case 0x28: return '\n';   // Enter
        case 0x29: return 27;     // Escape
        case 0x2A: return '\b';   // Backspace
        case 0x2B: return '\t';   // Tab
        case 0x2C: return ' ';
        case 0x2D: return '-';
        case 0x2E: return '=';
        case 0x2F: return '[';
        case 0x30: return ']';
        case 0x31: return '\\';
        case 0x32: return '#';
        case 0x33: return ';';
        case 0x34: return '\'';
        case 0x35: return '`';
        case 0x36: return ',';
        case 0x37: return '.';
        case 0x38: return '/';
        // Keypad keys. 0x59-0x62 are 1-9 and then 0.
        case 0x54: return '/';
        case 0x55: return '*';
        case 0x56: return '-';
        case 0x57: return '+';
        case 0x58: return '\n';
        case 0x63: return '.';
        default: break;
    }
    if (usage >= 0x59 && usage <= 0x62) {
        return (char)('1' + (usage - 0x59));
    }
    return 0;
}

static char hid_usage_ascii_shifted(uint8_t usage) {
    if (usage >= 0x04 && usage <= 0x1D) {
        return (char)('A' + (usage - 0x04));
    }
    if (usage >= 0x1E && usage <= 0x27) {
        return "!@#$%^&*()"[usage - 0x1E];
    }
    switch (usage) {
        case 0x2D: return '_';
        case 0x2E: return '+';
        case 0x2F: return '{';
        case 0x30: return '}';
        case 0x31: return '|';
        case 0x32: return '~';
        case 0x33: return ':';
        case 0x34: return '"';
        case 0x35: return '~';
        case 0x36: return '<';
        case 0x37: return '>';
        case 0x38: return '?';
        case 0x54: return '/';
        case 0x55: return '*';
        case 0x56: return '-';
        case 0x57: return '+';
        case 0x58: return '\n';
        case 0x63: return '.';
        default: break;
    }
    if (usage >= 0x59 && usage <= 0x62) {
        return (char)('1' + (usage - 0x59));
    }
    return 0;
}

// USB HID key usage -> the special key codes the shell already understands.
static char hid_usage_special(uint8_t usage) {
    switch (usage) {
        case 0x49: return KEY_INS;
        case 0x4A: return KEY_HOME;
        case 0x4B: return KEY_PGUP;
        case 0x4C: return KEY_DEL;
        case 0x4D: return KEY_END;
        case 0x4E: return KEY_PGDN;
        case 0x4F: return KEY_RIGHT;
        case 0x50: return KEY_LEFT;
        case 0x51: return KEY_DOWN;
        case 0x52: return KEY_UP;
        default: break;
    }
    return 0;
}

static char hid_usage_to_char(uint8_t usage, uint8_t shift, uint8_t caps_lock) {
    // A special key is not a character, so shift does not apply to it.
    char special = hid_usage_special(usage);
    if (special) return special;

    if (usage >= 0x04 && usage <= 0x1D) {
        // Caps lock affects letters only, and shift cancels it out.
        shift = (uint8_t)(shift ^ caps_lock);
    }
    return shift ? hid_usage_ascii_shifted(usage) : hid_usage_ascii(usage);
}

// Whether a usage code was already held in the previous boot protocol report.
static int hid_report_has(const uint8_t* report, uint8_t usage) {
    if (usage == 0) return 1;
    for (int i = 2; i < USB_BOOT_KEYBOARD_REPORT; i++) {
        if (report[i] == usage) return 1;
    }
    return 0;
}

static void hid_keyboard_report(usb_hid_dev_t* dev, const uint8_t* report) {
    // Modifier bits: 0x02 left shift, 0x20 right shift.
    dev->shift = (uint8_t)(((report[0] & 0x02) || (report[0] & 0x20)) ? 1 : 0);

    for (int i = 2; i < USB_BOOT_KEYBOARD_REPORT; i++) {
        uint8_t usage = report[i];
        if (usage <= 1) continue;                          // no key / rollover error
        if (hid_report_has(dev->prev_report, usage)) continue;   // still held
        if (usage >= 0xE0 && usage <= 0xE7) continue;      // modifiers, tracked above
        if (usage == 0x39) {                                // Caps Lock toggles on press
            dev->caps_lock = (uint8_t)(dev->caps_lock ^ 1);
            continue;
        }
        char c = hid_usage_to_char(usage, dev->shift, dev->caps_lock);
        if (c) {
            keyboard_inject_char(c);
        }
    }

    for (int i = 0; i < USB_BOOT_KEYBOARD_REPORT; i++) {
        dev->prev_report[i] = report[i];
    }
}

static void hid_mouse_report(usb_hid_dev_t* dev, const uint8_t* report) {
    uint8_t buttons = (uint8_t)(report[0] & 0x07);
    int16_t dx = (int8_t)report[1];
    // A boot protocol mouse reports Y in screen coordinates: positive is down.
    int16_t dy = (int8_t)report[2];

    mouse_inject_relative(dx, dy, buttons);
    if (dev->report_size >= 4) {
        int8_t wheel = (int8_t)report[3];
        if (wheel != 0) {
            mouse_inject_wheel(wheel);
        }
    }
}

static int usb_setup_device(uhci_hc_t* hc, usb_hid_dev_t* dev, int slot,
                            usb_hid_dev_t* parent, uint8_t port_index);
static void usb_hub_prime(uhci_hc_t* hc, usb_hid_dev_t* hub);
static void usb_hub_changed(uhci_hc_t* hc, usb_hid_dev_t* hub,
                            const uint8_t* report);
static int usb_setup_device_on_port(uhci_hc_t* hc, usb_hid_dev_t* parent,
                                    uint8_t port_index, int slot);

static void hid_device_report(uhci_hc_t* hc, usb_hid_dev_t* dev, const uint8_t* report) {
    if (dev->is_hub) {
        // A hub's change bitmap is not an input event, so it never reaches the
        // shell: it is a request to re-read the hub's ports.
        usb_hub_changed(hc, dev, report);
        return;
    }
    if (dev->is_keyboard) {
        hid_keyboard_report(dev, report);
    } else {
        hid_mouse_report(dev, report);
    }
}

// ----------------------------------------------------------------------------
// Enumeration
//
// Devices are enumerated top-down: each port of the root hub, then each port of
// any hub found on a port, recursively. The slot that comes back carries the
// device, and its parent and port index say where it hangs. Everything below
// talks to one device at a time through the shared control queue, so the walk
// is blocking: a hub's ports are probed one after another.
// ----------------------------------------------------------------------------

// USB addresses are per controller, so the slot index is a valid address that
// is unique, small and far from the 127-device limit.
static uint8_t usb_slot_address(int slot) {
    return (uint8_t)(slot + 1);
}

// Link a device's interrupt queue into the controller's chain, behind the
// control queue. The head of the chain hangs off the control queue, which is
// already in every frame of the schedule.
static void usb_append_queue(uhci_hc_t* hc, usb_hid_dev_t* dev) {
    usb_hid_dev_t* last = 0;
    for (int i = 0; i < USB_DEV_PER_HC; i++) {
        if (&hc->devices[i] != dev && hc->devices[i].present) {
            last = &hc->devices[i];
        }
    }
    if (last) {
        last->qh.link = usb_phys(&dev->qh) | LINK_QH | LINK_DEPTH;
    } else {
        hc->control_qh.link = usb_phys(&dev->qh) | LINK_QH | LINK_DEPTH;
    }
    usb_barrier();
}

// Class-specific hub descriptor, and the features a hub port reports.
#define USB_CLASS_HUB            0x09
#define USB_DESC_HUB             0x29
#define USB_PORT_FEATURE_POWER   0x0008
#define USB_PORT_FEATURE_RESET   0x0004

#define HUB_PORT_STATUS_CONNECTION  0x0001
#define HUB_PORT_STATUS_ENABLE      0x0002
#define HUB_PORT_STATUS_RESET       0x0010
#define HUB_PORT_STATUS_POWER       0x0100
#define HUB_PORT_STATUS_LOW_SPEED   0x0200
#define HUB_PORT_CHANGE_RESET   0x0010

// --- device setup -----------------------------------------------------------

static int usb_setup_device(uhci_hc_t* hc, usb_hid_dev_t* dev, int slot,
                            usb_hid_dev_t* parent, uint8_t port_index) {
    usb_device_descriptor_t dev_desc;
    usb_config_descriptor_t config_desc;
    static uint8_t config_buf[256];

    dev->address = 0;
    dev->max_packet = 8;
    dev->is_hub = 0;
    dev->parent = (parent == 0) ? USB_PARENT_ROOT : (uint8_t)(parent - hc->devices);
    dev->port_index = port_index;

    // The default address 0 control endpoint answers only the first 8 bytes of
    // its descriptor, which is all that is needed to learn the packet size.
    memset(&dev_desc, 0, sizeof(dev_desc));
    if (uhci_control(hc, dev, 0x80, USB_REQ_GET_DESCRIPTOR,
                     (uint16_t)(USB_DESC_DEVICE << 8), 0, 8, &dev_desc) < 8) {
        return 0;
    }
    dev->max_packet = dev_desc.max_packet_size0;

    // Give the device a real address now that its packet size is known. The
    // request itself is addressed to the default address 0, because the new
    // address only takes effect once the device accepts it.
    uint8_t new_address = usb_slot_address(slot);
    dev->address = 0;
    if (uhci_control(hc, dev, 0x00, USB_REQ_SET_ADDRESS, new_address, 0, 0, 0) < 0) {
        return 0;
    }
    dev->address = new_address;
    uhci_wait_ms(hc->io_base, 10);   // the device needs time to change address

    memset(&dev_desc, 0, sizeof(dev_desc));
    if (uhci_control(hc, dev, 0x80, USB_REQ_GET_DESCRIPTOR,
                     (uint16_t)(USB_DESC_DEVICE << 8), 0,
                     (uint16_t)sizeof(dev_desc), &dev_desc) < 0) {
        return 0;
    }

    // Read the configuration: every interface of the device lives in it, so the
    // walk below can tell a boot protocol HID from a hub.
    memset(&config_desc, 0, sizeof(config_desc));
    if (uhci_control(hc, dev, 0x80, USB_REQ_GET_DESCRIPTOR,
                     (uint16_t)(USB_DESC_CONFIGURATION << 8), 0,
                     (uint16_t)sizeof(config_desc), &config_desc) < 0) {
        return 0;
    }

    uint16_t total_length = config_desc.total_length;
    if (total_length < sizeof(config_desc) || total_length > sizeof(config_buf)) {
        return 0;
    }
    memset(config_buf, 0, total_length);
    if (uhci_control(hc, dev, 0x80, USB_REQ_GET_DESCRIPTOR,
                     (uint16_t)(USB_DESC_CONFIGURATION << 8), 0, total_length,
                     config_buf) < total_length) {
        return 0;
    }

    // Walk the interface descriptors, keeping the first appendable endpoint of
    // the interface we care about. A boot protocol HID is class 3, subclass 1
    // and protocol 1 (keyboard) or 2 (mouse); a hub is class 9.
    int is_hub = 0;
    int is_keyboard = 0;
    int on_hid_interface = 0;
    uint8_t endpoint = 0;
    uint16_t max_packet = 8;
    uint8_t hub_endpoint = 0;
    uint16_t hub_max_packet = 8;

    const uint8_t* p = config_buf;
    const uint8_t* end = config_buf + total_length;
    while (p + 2 <= end) {
        uint8_t len = p[0];
        uint8_t type = p[1];
        if (len < 2 || p + len > end) break;

        if (type == USB_DESC_INTERFACE) {
            usb_interface_descriptor_t iface;
            memcpy(&iface, p, len);
            is_hub = (iface.interface_class == USB_CLASS_HUB) ? 1 : 0;
            is_keyboard = (iface.interface_protocol == USB_PROTOCOL_HID_KEYBOARD);
            on_hid_interface = (iface.interface_class == USB_CLASS_HID &&
                               iface.interface_subclass == USB_SUBCLASS_HID_BOOT &&
                               (iface.interface_protocol == USB_PROTOCOL_HID_KEYBOARD ||
                                iface.interface_protocol == USB_PROTOCOL_HID_MOUSE));
        } else if (type == USB_DESC_ENDPOINT) {
            usb_endpoint_descriptor_t ep;
            memcpy(&ep, p, len);
            if ((ep.endpoint_address & 0x80) &&
                (ep.attributes & 0x03) == USB_EP_TYPE_INTERRUPT) {
                if (on_hid_interface && endpoint == 0) {
                    endpoint = (uint8_t)(ep.endpoint_address & 0x0F);
                    max_packet = ep.max_packet_size;
                }
                if (is_hub && hub_endpoint == 0) {
                    hub_endpoint = (uint8_t)(ep.endpoint_address & 0x0F);
                    hub_max_packet = ep.max_packet_size;
                }
            }
        }
        p += len;
    }

    if (uhci_control(hc, dev, 0x00, USB_REQ_SET_CONFIGURATION,
                     config_desc.configuration_value, 0, 0, 0) < 0) {
        return 0;
    }

    if (is_hub && hub_endpoint != 0) {
        // A hub reports on a status change endpoint, where the report is a
        // bitmap with one bit per port; no protocol request is involved. The
        // queue is armed only after its children, because the control queue
        // and this queue share the controller's schedule and the walk below
        // must finish first.
        uint8_t hub_desc[8];
        memset(hub_desc, 0, sizeof(hub_desc));
        if (uhci_control(hc, dev, 0xA0, USB_REQ_GET_DESCRIPTOR,
                         (uint16_t)(USB_DESC_HUB << 8), 0,
                         (uint16_t)sizeof(hub_desc), hub_desc) < 0) {
            return 0;
        }

        dev->present = 1;
        dev->is_hub = 1;
        dev->is_keyboard = 0;
        dev->endpoint = hub_endpoint;
        dev->max_packet = hub_max_packet;
        dev->report_size = 1;
        if (dev->report_size > dev->max_packet) dev->report_size = (uint8_t)dev->max_packet;
        dev->nports = hub_desc[2];
        for (int i = 0; i < 8 && i < dev->report_size; i++) dev->prev_change[i] = 0;

        serial_write_string("USB: hub on port (no split transactions: children "
                            "behind it cannot be reached)\n");
        for (uint8_t hp = 0; hp < dev->nports; hp++) {
            usb_setup_device_on_port(hc, dev, hp, -1);
        }

        hid_start_interrupt(dev);
        usb_append_queue(hc, dev);
        usb_hub_prime(hc, dev);
        return 1;
    }

    if (!on_hid_interface || endpoint == 0) {
        return 0;   // not a boot protocol HID with an interrupt endpoint
    }

    // Boot protocol first: the fixed 8-byte keyboard and 4-byte mouse reports
    // mean no report descriptor has to be parsed.
    if (uhci_control(hc, dev, 0x21, USB_REQ_SET_PROTOCOL, 0, 0, 0, 0) < 0) {
        return 0;
    }
    // Idle forever: a report only arrives when something changes, which is
    // exactly what the poll loop wants.
    if (uhci_control(hc, dev, 0x21, USB_REQ_SET_IDLE, 0, 0, 0, 0) < 0) {
        return 0;
    }

    dev->present = 1;
    dev->is_hub = 0;
    dev->is_keyboard = (uint8_t)(is_keyboard ? 1 : 0);
    dev->endpoint = endpoint;
    dev->max_packet = max_packet;
    dev->report_size = (uint8_t)(is_keyboard ? USB_BOOT_KEYBOARD_REPORT : USB_BOOT_MOUSE_REPORT);
    if (dev->report_size > USB_BOOT_KEYBOARD_REPORT) dev->report_size = USB_BOOT_KEYBOARD_REPORT;
    if (dev->report_size > dev->max_packet) dev->report_size = (uint8_t)dev->max_packet;
    for (int i = 0; i < USB_BOOT_KEYBOARD_REPORT; i++) dev->prev_report[i] = 0;
    dev->shift = 0;
    dev->caps_lock = 0;

    hid_start_interrupt(dev);
    usb_append_queue(hc, dev);

    char msg[80];
    sprintf(msg, "USB: %s on port (address %d, endpoint %d)\n",
            is_keyboard ? "keyboard" : "mouse", (int)dev->address, (int)endpoint);
    serial_write_string(msg);
    return 1;
}

// --- hub port management ----------------------------------------------------

// Read a hub port's status and change words. The request is addressed to the
// port (recipient "other"), not the interface, so the direction and recipient
// bits of the request type read 0xA3.
static int usb_hub_port_status(uhci_hc_t* hc, usb_hid_dev_t* hub,
                               uint8_t port, uint16_t* status,
                               uint16_t* changed) {
    uint16_t words[2];
    memset(words, 0, sizeof(words));
    int r = uhci_control(hc, hub, 0xA3, USB_REQ_GET_STATUS, 0, (uint16_t)port,
                        4, words);
    *status = words[0];
    if (changed) *changed = words[1];
    return r;
}

// Ask a hub to set a feature on one of its ports, such as power or reset.
static int usb_hub_port_feature(uhci_hc_t* hc, usb_hid_dev_t* hub,
                                uint8_t port, uint16_t feature) {
    return uhci_control(hc, hub, 0x23, USB_REQ_SET_FEATURE, feature,
                       (uint16_t)port, 0, 0);
}

// Find a free slot for a device that just appeared.
static int usb_alloc_slot(uhci_hc_t* hc) {
    for (int i = 0; i < USB_DEV_PER_HC; i++) {
        if (!hc->devices[i].present) return i;
    }
    return -1;
}

// Drop whatever was on a hub port: it went away.
static void usb_hub_port_empty(uhci_hc_t* hc, usb_hid_dev_t* hub, uint8_t port) {
    for (int i = 0; i < USB_DEV_PER_HC; i++) {
        usb_hid_dev_t* d = &hc->devices[i];
        if (d->present && d->parent == (uint8_t)(hub - hc->devices) &&
            d->port_index == port) {
            d->present = 0;
        }
    }
}

// Wait for the hub's first status change report so its initial state is known
// before anything is powered up. The report also clears the change bits.
static void usb_hub_prime(uhci_hc_t* hc, usb_hid_dev_t* hub) {
    for (int i = 0; i < 200; i++) {
        usb_barrier();
        if (!(hub->td.status & TD_CTRL_ACTIVE)) break;
        uhci_wait_ms(hc->io_base, 1);
    }
    for (int i = 0; i < 8 && i < hub->report_size; i++) {
        hub->prev_change[i] = hub->buffer[i];
    }
    hid_rearm(hub);
}

// Power, reset and enumerate whatever is on a hub port.
static int usb_setup_device_on_port(uhci_hc_t* hc, usb_hid_dev_t* parent,
                                    uint8_t port_index, int slot) {
    uint8_t port = (uint8_t)(port_index + 1);
    uint16_t status = 0;

    if (usb_hub_port_status(hc, parent, port, &status, 0) < 4) {
        return 0;
    }
    if (!(status & HUB_PORT_STATUS_CONNECTION)) {
        usb_hub_port_empty(hc, parent, port_index);
        return 0;
    }

    if (usb_hub_port_feature(hc, parent, port, USB_PORT_FEATURE_POWER) < 0) {
        return 0;
    }
    // Port power settling: the spec asks for 10 ms of power good delay.
    uhci_wait_ms(hc->io_base, 20);

    int rp = usb_hub_port_feature(hc, parent, port, USB_PORT_FEATURE_POWER);
    int rr = usb_hub_port_feature(hc, parent, port, USB_PORT_FEATURE_RESET);
    (void)rp;
    if (rr < 0) {
        return 0;
    }

    // Wait for the reset to finish: the hub clears its reset bit and then
    // enables the port, and only then does the device answer on address 0. The
    // change bit reports completion; the status bit reports the port is live.
    uint16_t changed = 0;
    for (int i = 0; i < 100; i++) {
        if (usb_hub_port_status(hc, parent, port, &status, &changed) < 4) return 0;
        if ((changed & HUB_PORT_CHANGE_RESET) &&
            (status & HUB_PORT_STATUS_ENABLE)) break;
        uhci_wait_ms(hc->io_base, 2);
    }

    if (slot < 0) {
        slot = usb_alloc_slot(hc);
    }
    if (slot < 0) {
        return 0;
    }

    usb_hid_dev_t* dev = &hc->devices[slot];
    dev->present = 0;
    dev->parent = (uint8_t)(parent - hc->devices);
    dev->port_index = port_index;
    dev->low_speed = (status & HUB_PORT_STATUS_LOW_SPEED) ? 1 : 0;
    return usb_setup_device(hc, dev, slot, parent, port_index);
}

// --- hub change reports -----------------------------------------------------

// A hub reports which of its ports changed, one bit per port, with port 1 at
// bit 0 of byte 0. For every port that changed, a fresh read of the port says
// whether a device appeared or disappeared.
static void usb_hub_changed(uhci_hc_t* hc, usb_hid_dev_t* hub,
                            const uint8_t* report) {
    for (uint8_t port = 0; port < hub->nports && port < 8 * USB_BOOT_KEYBOARD_REPORT; port++) {
        int bit = port % 8;
        uint8_t mask = (uint8_t)(1u << bit);
        if (!(report[port / 8] & mask)) continue;

        uint16_t status = 0;
        if (usb_hub_port_status(hc, hub, (uint8_t)(port + 1), &status, 0) < 4) continue;
        if (status & HUB_PORT_STATUS_CONNECTION) {
            usb_setup_device_on_port(hc, hub, port, -1);
        } else {
            usb_hub_port_empty(hc, hub, port);
        }
    }
}

// --- root hub ---------------------------------------------------------------

// Power and reset one root hub port, then enumerate whatever answered.
static int usb_root_probe_port(uhci_hc_t* hc, int port_index) {
    uint16_t reg = hc->io_base + UHCI_PORTSC(port_index);
    if (!(inw(reg) & UHCI_PORT_CCS)) return 0;

    uint8_t low_speed = 0;
    if (!uhci_port_ready(hc, port_index, &low_speed)) return 0;

    int slot = usb_alloc_slot(hc);
    if (slot < 0) return 0;

    usb_hid_dev_t* dev = &hc->devices[slot];
    dev->present = 0;
    dev->parent = USB_PARENT_ROOT;
    dev->port_index = (uint8_t)port_index;
    dev->low_speed = low_speed;
    return usb_setup_device(hc, dev, slot, 0, (uint8_t)port_index);
}

static void uhci_hc_probe(uhci_hc_t* hc, pci_device_t* pci) {
    if (!uhci_hc_init(hc, pci)) {
        serial_write_string("USB: controller reset failed\n");
        return;
    }

    // A UHCI root hub has two ports.
    for (int port_index = 0; port_index < 2; port_index++) {
        usb_root_probe_port(hc, port_index);
    }
}

static int usb_probe_controller(pci_device_t* pci, void* ctx) {
    (void)ctx;
    if (hc_count >= USB_HC_COUNT) return 1;   // stop scanning
    uhci_hc_probe(&hcs[hc_count], pci);
    hc_count++;
    return 0;
}

// ----------------------------------------------------------------------------
// Public interface
// ----------------------------------------------------------------------------

void usb_init(void) {
    serial_write_string("USB: probing UHCI controllers\n");
    pci_scan_class(0x0C, 0x03, 0x00, usb_probe_controller, 0);
    if (hc_count == 0) {
        serial_write_string("USB: no UHCI controller found\n");
    }
}

void usb_poll(void) {
    for (int hc_index = 0; hc_index < hc_count; hc_index++) {
        uhci_hc_t* hc = &hcs[hc_index];
        if (!hc->initialized) continue;

        for (int dev_index = 0; dev_index < USB_DEV_PER_HC; dev_index++) {
            usb_hid_dev_t* dev = &hc->devices[dev_index];
            if (!dev->present) continue;

            uhci_td_t* td = &dev->td;
            usb_barrier();
            if (td->status & TD_CTRL_ACTIVE) continue;

            const uint8_t* report = dev->buffer;
            hid_device_report(hc, dev, report);
            hid_rearm(dev);
        }
    }
}

int usb_keyboard_connected(void) {
    for (int hc_index = 0; hc_index < hc_count; hc_index++) {
        uhci_hc_t* hc = &hcs[hc_index];
        for (int i = 0; i < USB_DEV_PER_HC; i++) {
            if (hc->devices[i].present &&
                !hc->devices[i].is_hub && hc->devices[i].is_keyboard) return 1;
        }
    }
    return 0;
}

int usb_mouse_connected(void) {
    for (int hc_index = 0; hc_index < hc_count; hc_index++) {
        uhci_hc_t* hc = &hcs[hc_index];
        for (int i = 0; i < USB_DEV_PER_HC; i++) {
            if (hc->devices[i].present &&
                !hc->devices[i].is_hub && !hc->devices[i].is_keyboard) return 1;
        }
    }
    return 0;
}