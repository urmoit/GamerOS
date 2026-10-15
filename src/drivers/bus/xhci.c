#include <xhci.h>
#include <usb.h>
#include <ports.h>
#include <pci.h>
#include <keyboard.h>
#include <mouse.h>
#include <serial.h>
#include <string.h>

// ============================================================================
// xHCI host controller driver with boot protocol HID keyboard and mouse
// support.
//
// A UHCI device is reached through a frame list of transfer descriptors; an xHC
// is reached through three software rings of TRBs. The flow is:
//
//   1. reset the controller and read its capabilities out of the PCI BAR
//   2. program the command ring, the device context array and the event ring
//   3. reset each root port that reports a connection
//   4. for each port: enable a slot from the controller, address the device
//      into it, run the control transfers HID enumeration needs
//   5. add the interrupt endpoint, then poll its ring for reports
//
// Everything is polled: the desktop loop runs with interrupts off, so no IRQ is
// requested and the event ring is walked whenever something may have completed.
// ============================================================================

// --- Capability registers (BAR0 base) --------------------------------------
#define XHCI_HCIVER        0x02
#define XHCI_HCSPARAMS1     0x04
#define XHCI_HCSPARAMS2     0x08
#define XHCI_HCSPARAMS3     0x0C
#define XHCI_HCCPARAMS1     0x10
#define XHCI_HCCPARAMS2     0x1C
#define XHCI_DBOFF          0x14
#define XHCI_RTSOFF         0x18

// --- Operational registers (BAR0 + CAPLENGTH) -------------------------------
#define XHCI_USBCMD         0x00
#define XHCI_USBSTS         0x04
#define XHCI_PAGESIZE       0x08
#define XHCI_DNCTRL         0x14
#define XHCI_CRCR           0x18
#define XHCI_DCBAAP         0x30
#define XHCI_CONFIG         0x38

#define XHCI_CMD_RS         (1u << 0)
#define XHCI_CMD_HCRST     (1u << 1)
#define XHCI_CMD_INTE       (1u << 2)
#define XHCI_CMD_EU3S       (1u << 11)

#define XHCI_STS_HCH         (1u << 0)
#define XHCI_STS_HSE         (1u << 2)
#define XHCI_STS_EINT        (1u << 3)
#define XHCI_STS_PCD         (1u << 4)

#define XHCI_CRCR_RCS       (1u << 0)
#define XHCI_CRCR_CS        (1u << 1)
#define XHCI_CRCR_CA        (1u << 2)
#define XHCI_CRCR_CRR       (1u << 3)

// --- Port registers (BAR0 + CAPLENGTH + 0x400 + port * 0x10) ---------------
#define XHCI_PORTSC         0x00
#define XHCI_PORT_CCS       (1u << 0)
#define XHCI_PORT_PED       (1u << 1)
#define XHCI_PORT_PR        (1u << 4)
#define XHCI_PORT_WPR       (1u << 31)
#define XHCI_PORT_CSC       (1u << 17)
#define XHCI_PORT_PRC       (1u << 21)
#define XHCI_PORT_WRC       (1u << 19)
#define XHCI_PORT_SPEED_SHIFT 10
#define XHCI_PORT_SPEED_MASK (0xFu << 10)
#define XHCI_PORT_SPEED_FS  1u
#define XHCI_PORT_SPEED_HS  3u

// --- Runtime registers (BAR0 + RTSOFF + interrupter * 0x20) ----------------
#define XHCI_IMAN           0x00
#define XHCI_IMOD           0x04
#define XHCI_ERSTSZ         0x08
#define XHCI_ERSTBA         0x10
#define XHCI_ERSTSZ_OFF   0x08
#define XHCI_ERSTBA_LOW   0x10
#define XHCI_ERSTBA_HI    0x14
#define XHCI_ERDP_LOW      0x18
#define XHCI_ERDP_HI      0x1C

#define XHCI_IMAN_IP        (1u << 0)
#define XHCI_IMAN_IE       (1u << 1)
#define XHCI_ERDP_EHB      (1u << 3)

// --- Doorbell registers (BAR0 + DBOFF + slot * 4) --------------------------
#define XHCI_DB_SLOT_MASK  0x00FF

// --- TRBs ------------------------------------------------------------------
#define XHCI_TRB_SIZE      16

#define TRB_NORMAL          1
#define TRB_SETUP            2
#define TRB_DATA             3
#define TRB_STATUS         4
#define TRB_ISOCH          5
#define TRB_LINK             6
#define TRB_NOOP           8
#define CR_ENABLE_SLOT     9
#define CR_DISABLE_SLOT    10
#define CR_ADDRESS_DEVICE   11
#define CR_CONFIGURE_EP     12
#define CR_EVALUATE_CTX    13
#define CR_RESET_EP         14
#define CR_SET_TR_DQ       15
#define CR_RESET_DEVICE    16

#define TRB_C               (1u << 0)
#define TRB_TC              0
#define TRB_ISP             (1u << 2)
#define TRB_CH              (1u << 4)
#define TRB_IOC             (1u << 5)
#define TRB_IDT             (1u << 6)
#define TRB_DIR             (1u << 16)
#define TRB_SPD             (1u << 29)
#define TRB_TR_TYPE_SHIFT  10
#define TRB_EVENT_TYPE_SHIFT 10

#define CC_SUCCESS         1
#define USB_PID_OUT        0xE1u
#define USB_PID_IN         0x69u
#define USB_PID_SETUP      0x2Du
#define XHCI_DB_EP0        1
#define CC_SHORT_PACKET    13

// Context layouts. Every context is 32 bytes.
#define CTX_SIZE            32
#define CTX_SLOT            0
#define CTX_EP0             1
#define CTX_EP1             2

#define SLOT_CTX_ENTRIES_SHIFT 27
#define SLOT_CTX_ROUTE_MASK   (0xFu << 0)
#define SLOT_CTX_SPEED_SHIFT  20
#define SLOT_CTX_PORT_SHIFT   16
#define SLOT_CTX_ROUTE_PARENT (0xFu << 12)

#define EP_CTX_INTERVAL_SHIFT 16
#define EP_CTX_MAXPSIZE_SHIFT 16
#define EP_CTX_TYPE_SHIFT     3
#define EP_CTX_CERR_SHIFT     1
#define EP_CTX_DCS           (1u << 0)

#define EP_TYPE_ISO_OUT     1
#define EP_TYPE_BULK_OUT    2
#define EP_TYPE_INTR_OUT    3
#define EP_TYPE_CONTROL    4
#define EP_TYPE_ISO_IN     5
#define EP_TYPE_BULK_IN     6
#define EP_TYPE_INTR_IN     7

// Ring sizes. Rings must be fully within a 64 KiB boundary for the link TRB and
// head to be reachable, and each ring starts 16-byte aligned.
// Ring sizes. The last slot of every ring is a link TRB that loops back to the
// head, so a ring holds one fewer usable TRB than its size says.
#define XHCI_CMD_RING_SIZE  32
#define XHCI_EVENT_RING    32
#define XHCI_EP_RING_SIZE   32
#define XHCI_MAX_SLOTS     8
#define XHCI_CONTEXTS      (1 + 31)

typedef struct {
    uint32_t parameter_lo;
    uint32_t parameter_hi;
    uint32_t status;
    uint32_t control;
} __attribute__((packed, aligned(16))) xhci_trb_t;

typedef struct {
    uint8_t present;
    uint8_t address;
    uint8_t low_speed;
    uint8_t is_keyboard;
    uint8_t port_index;
    uint8_t slot_id;
    uint8_t report_size;
    uint8_t toggle;
    uint8_t prev_report[8];
    uint8_t buffer[16];
    uint8_t setup_buffer[8];
    uint64_t ep_ring_phys;
    uint32_t ep_enqueue;
    uint8_t ep_cycle;
    uint32_t ep_pending;
} xhci_slot_t;

typedef struct {
    uint64_t mmio_base;
    uint32_t caplength;
    uint32_t oper;
    uint32_t dboff;
    uint32_t rtsoff;
    uint32_t max_slots;
    uint32_t max_ports;
    uint32_t dbstride;
    uint32_t context_size;

    // The rings and the device-context array all come from one 64 KiB aligned
    // arena, because the controller requires each ring and context array to lie
    // wholly inside a 64 KiB boundary.
    uint64_t cmd_ring_phys;
    uint64_t event_ring_phys;
    uint64_t dcbaa_phys;

    uint32_t cmd_enqueue;
    uint8_t cmd_cycle;
    uint32_t event_dequeue;
    uint8_t event_cycle;

    xhci_slot_t slots[XHCI_MAX_SLOTS];
} xhci_hc_t;

#define XHCI_HC_COUNT 2
static xhci_hc_t xhcs[XHCI_HC_COUNT];
static int xhc_count = 0;

// One millisecond: the controller's microframe index advances once per frame,
// so waiting on it gives a real clock without busy-loop calibration. Before the
// controller runs there is no such clock, so a bounded spin is used instead.
static void xhci_wait_frames(uint32_t count) {
    for (volatile uint32_t i = 0; i < count * 1000; i++) {
        __asm__ volatile("pause");
    }
}
// ----------------------------------------------------------------------------
// Register access
//
// The capability, operational, runtime and doorbell slices each sit at BAR0 plus
// an offset that the capability registers supply, so every access names both.
// The registers are memory-mapped, not in the port space.
// ----------------------------------------------------------------------------

static void xhci_wr(xhci_hc_t* hc, uint32_t slice, uint32_t reg, uint32_t val) {
    mmio_outl(hc->mmio_base + (uint64_t)slice + reg, val);
}

static uint32_t xhci_rd(xhci_hc_t* hc, uint32_t slice, uint32_t reg) {
    return mmio_inl(hc->mmio_base + (uint64_t)slice + reg);
}

// ----------------------------------------------------------------------------
// TRB helpers
// ----------------------------------------------------------------------------

static inline void trb_type(xhci_trb_t* trb, uint32_t type) {
    trb->control = (trb->control & ~(0x3Fu << TRB_TR_TYPE_SHIFT)) |
                   (uint32_t)(type << TRB_TR_TYPE_SHIFT);
}

static inline uint32_t trb_get_type(const xhci_trb_t* trb) {
    return (trb->control >> TRB_TR_TYPE_SHIFT) & 0x3F;
}

static inline uint8_t trb_get_cycle(const xhci_trb_t* trb) {
    return trb->control & 1u;
}

// ----------------------------------------------------------------------------
// Ring arithmetic and port control
//
// A ring is a fixed array of TRBs whose last slot is a link back to the head.
// The producer cycle bit toggles whenever the producer wraps past that link, so
// a ring can be walked forever without rewriting it: a TRB belongs to the
// producer while its cycle bit matches the ring's, and to the controller while
// it does not.
// ----------------------------------------------------------------------------

// A ring holds `size - 1` usable TRBs: the last slot is the link. Advancing
// past the end wraps to slot 0 and flips the producer's cycle bit, which is what
// tells the controller which TRBs are new.
static uintptr_t ring_next(uintptr_t ring, uint32_t* enqueue, uint8_t* cycle,
                           uint32_t size) {
    uint32_t idx = *enqueue;
    uintptr_t trb = ring + idx * XHCI_TRB_SIZE;

    // Stop at the link TRB rather than overwriting it.
    uint32_t next = idx + 1;
    if (next >= size - 1) {
        next = 0;
        *cycle = (uint8_t)(*cycle ^ 1);
    }
    *enqueue = next;
    return trb;
}

// Walk the event ring and hand every completed TRB to the caller. Each
// consumption writes the dequeue pointer back so the controller can reuse the
// slot; the event handler busy bit makes it do so immediately.
static uint32_t xhci_poll_events(xhci_hc_t* hc,
                                 void (*handler)(xhci_hc_t*, const xhci_trb_t*)) {
    uint32_t found = 0;
    uintptr_t ring = (uintptr_t)hc->event_ring_phys;

    // One fewer slot is usable than the ring holds, because the last slot is
    // the segment terminator the controller writes when it wraps.
    uint32_t usable = XHCI_EVENT_RING - 1;

    while (hc->event_dequeue < usable) {
        const xhci_trb_t* ev = (const xhci_trb_t*)(ring +
                                  hc->event_dequeue * XHCI_TRB_SIZE);
        uint32_t control = ev->control;

        // The controller marks an event by writing it with the ring's cycle bit
        // set, so a mismatch means this slot is not yet written.
        if ((control & 1u) != hc->event_cycle) break;

        handler(hc, ev);
        found++;

        hc->event_dequeue++;
        if (hc->event_dequeue >= usable) {
            hc->event_dequeue = 0;
            hc->event_cycle = (uint8_t)(hc->event_cycle ^ 1);
        }

        // Free the slot: the controller writes to the next one.
        uint32_t erdp = (uint32_t)ring + hc->event_dequeue * XHCI_TRB_SIZE;
        uint32_t base = hc->rtsoff + 0x20;
        xhci_wr(hc, base, XHCI_ERDP_LOW, erdp | XHCI_ERDP_EHB);
        xhci_wr(hc, base, XHCI_ERDP_HI, 0);
    }

    return found;
}


// ----------------------------------------------------------------------------
// Command completion
//
// A command is answered by a "command completion" event on the controller's
// event ring, which carries the command's own TRB address back so the caller
// can tell which one finished. Waiting means posting the command, then polling
// the event ring until that address comes back.
// ----------------------------------------------------------------------------

static volatile uint32_t command_result;
static volatile int command_completed;
static volatile uint32_t command_slot;
static volatile uint32_t command_code;
static uintptr_t waiting_trb;

static void xhci_event_handler(xhci_hc_t* hc, const xhci_trb_t* ev) {
    uint32_t type = trb_get_type(ev);

    if (type == 33) {   // command completion event
        if ((uintptr_t)ev->parameter_lo == waiting_trb) {
            command_result = ev->parameter_lo;
            command_code = ev->status >> 24;
            command_slot = (ev->control >> 24) & 0xFFu;
            command_completed = 1;
        }
        return;
    }

    if (type == 32) {   // transfer event
        // A transfer finished on a slot's endpoint ring; the interrupt polling
        // path checks the TRB directly, so this only records that the ring
        // moved and does not need to be decoded here.
        command_result = 0;
        return;
    }
    (void)hc;
}

// Post a command and wait for its completion event. Returns the completion
// code, or 0xFF when the controller never answered.
static uint32_t xhci_run_command(xhci_hc_t* hc, uint32_t control, uint32_t param_lo,
                                 uint32_t param_hi, uint32_t status) {
    command_completed = 0;
    command_code = 0xFF;
    waiting_trb = 0;

    uintptr_t trb_phys = ring_next((uintptr_t)hc->cmd_ring_phys,
                                   &hc->cmd_enqueue, &hc->cmd_cycle,
                                   XHCI_CMD_RING_SIZE);
    xhci_trb_t* trb = (xhci_trb_t*)trb_phys;

    trb->parameter_lo = param_lo;
    trb->parameter_hi = param_hi;
    trb->status = status;
    trb->control = control | hc->cmd_cycle;
    waiting_trb = trb_phys;


    // Doorbell: slot 0, target 0 means the host controller's command ring.
    // The doorbell array is indexed by slot number, with each entry DBSTRIDE
    // bytes wide, and the target is the low byte.
    xhci_wr(hc, hc->dboff, 0, 0);

    for (int i = 0; i < 2000; i++) {


    }

    return 0xFF;
}
static int xhci_port_connected(xhci_hc_t* hc, uint32_t port) {
    uint32_t portsc = xhci_rd(hc, hc->oper + 0x400 + port * 0x10, XHCI_PORTSC);
    return (portsc & XHCI_PORT_CCS) ? 1 : 0;
}

static int xhci_port_enable(xhci_hc_t* hc, uint32_t port) {
    uint16_t base = hc->oper + 0x400 + port * 0x10;
    uint32_t portsc = xhci_rd(hc, base, XHCI_PORTSC);
    uint32_t speed = (portsc & XHCI_PORT_SPEED_MASK) >> XHCI_PORT_SPEED_SHIFT;

    // A port register write must carry the whole current value: bits that are
    // not in the write are cleared, so writing only the reset bit would drop
    // the connect status and whatever else the controller needs. Writing any
    // write-1-to-clear bit clears it, so those are masked out while setting the
    // reset.
    uint32_t value = portsc & ~(XHCI_PORT_CSC | XHCI_PORT_PRC | XHCI_PORT_WRC);
    xhci_wr(hc, base, XHCI_PORTSC,
            value | ((speed == XHCI_PORT_SPEED_HS) ? XHCI_PORT_WPR : XHCI_PORT_PR));

    // Reset completes when the port enable change clears and the enable bit
    // is set; the spec asks for 50 ms of settling before the device answers.
    for (int i = 0; i < 200; i++) {
        portsc = xhci_rd(hc, base, XHCI_PORTSC);
        if ((portsc & XHCI_PORT_PED) && !(portsc & XHCI_PORT_PRC)) {
            // Acknowledge the change bits so they do not stay latched.
            xhci_wr(hc, base, XHCI_PORTSC, portsc |
                    XHCI_PORT_CSC | XHCI_PORT_PRC);
            return 1;
        }
        // Also ack the change bits while waiting.
        xhci_wr(hc, base, XHCI_PORTSC, portsc |
                XHCI_PORT_CSC | XHCI_PORT_PRC);
        xhci_wait_frames(1);
    }
    return 0;
}

static uint32_t xhci_port_speed(xhci_hc_t* hc, uint32_t port) {
    uint32_t portsc = xhci_rd(hc, hc->oper + 0x400 + port * 0x10, XHCI_PORTSC);
    return (portsc & XHCI_PORT_SPEED_MASK) >> XHCI_PORT_SPEED_SHIFT;
}

// Every xHCI ring must sit wholly inside a 64 KiB boundary, because the
// controller compares only the low bits of ring pointers against the ring base.
// One 64 KiB aligned arena is carved by hand instead of using the general heap,
// which returns 8-byte aligned blocks.
#define XHCI_RING_ARENA (64 * 1024)
static uint8_t ring_arena[XHCI_RING_ARENA] __attribute__((aligned(64 * 1024)));
static uint32_t ring_arena_used = 0;

static uintptr_t arena_alloc(uint32_t bytes) {
    uint32_t need = (bytes + 63u) & ~63u;
    if (ring_arena_used + need > XHCI_RING_ARENA) return 0;
    uintptr_t p = (uintptr_t)&ring_arena[ring_arena_used];
    ring_arena_used += need;
    return p;
}

// A controller that refuses to halt cannot be reset safely, so wait for the
// halted status bit before issuing the reset.
static int xhci_await_halted(xhci_hc_t* hc) {
    for (int i = 0; i < 1000; i++) {
        if (xhci_rd(hc, hc->oper, XHCI_USBSTS) & XHCI_STS_HCH) return 1;
        xhci_wait_frames(1);
    }
    return 0;
}

static int xhci_reset_controller(xhci_hc_t* hc) {
    uint32_t cmd = xhci_rd(hc, hc->oper, XHCI_USBCMD) & ~XHCI_CMD_RS;
    xhci_wr(hc, hc->oper, XHCI_USBCMD, cmd);
    xhci_await_halted(hc);

    xhci_wr(hc, hc->oper, XHCI_USBCMD, XHCI_CMD_HCRST);
    for (int i = 0; i < 1000; i++) {
        if (!(xhci_rd(hc, hc->oper, XHCI_USBCMD) & XHCI_CMD_HCRST)) return 1;
        xhci_wait_frames(1);
    }
    return 0;
}

static void xhci_probe_ports(xhci_hc_t* hc);

static int xhci_probe(pci_device_t* pci, void* ctx) {
    (void)ctx;
    if (xhc_count >= XHCI_HC_COUNT) return 1;

    uint32_t bar0 = pci_get_bar(pci, 0);
    if (!bar0) return 0;

    char line[96];
    sprintf(line, "xHCI: found bus %u dev %u bar %08x\n",
            pci->bus, pci->device, bar0);
    serial_write_string(line);


    // A BAR with bit 0 set is an I/O BAR, which xHCI never is. A 64-bit
    // prefetchable BAR carries the upper address bits in the next register, but
    // QEMU keeps every xHCI controller low enough that the upper half is zero.
    if (bar0 & 1u) return 0;

    pci_enable_bus_mastering(pci);

    xhci_hc_t* hc = &xhcs[xhc_count];
    memset(hc, 0, sizeof(*hc));
    hc->mmio_base = (uint64_t)(bar0 & 0xFFFFFFF0u);

    uint32_t cap = mmio_inl(hc->mmio_base);
    uint32_t caplength = cap & 0xFFu;
    hc->caplength = caplength;
    hc->oper = caplength;
    hc->dboff = mmio_inl(hc->mmio_base + XHCI_DBOFF) & 0xFFFFFFFCu;
    hc->rtsoff = mmio_inl(hc->mmio_base + XHCI_RTSOFF) & 0xFFFFFFE0u;

    // The lower byte of the first capability dword is the length in bytes of
    // the capability register block, and that is also where the operational
    // block begins.
    uint32_t sp1 = mmio_inl(hc->mmio_base + XHCI_HCSPARAMS1);
    uint32_t sp2 = mmio_inl(hc->mmio_base + XHCI_HCSPARAMS2);
    uint32_t ccp1 = mmio_inl(hc->mmio_base + XHCI_HCCPARAMS1);

    hc->max_slots = sp1 & 0xFFu;
    hc->max_ports = (sp1 >> 24) & 0xFFu;
    hc->dbstride = (sp2 >> 16) & 0xFFu;
    hc->context_size = (ccp1 >> 2) & 0x1u ? 64 : CTX_SIZE;

    if (hc->max_slots > XHCI_MAX_SLOTS) hc->max_slots = XHCI_MAX_SLOTS;
    if (hc->max_ports > 16) hc->max_ports = 16;

    sprintf(line, "xHCI: caplen=%u slots=%u ports=%u ctx=%u\n",
            hc->caplength, hc->max_slots, hc->max_ports, hc->context_size);
    serial_write_string(line);

    if (!xhci_reset_controller(hc)) {
        serial_write_string("xHCI: reset failed\n");
        return 0;
    }

    // Enable the number of device slots this driver supports.
    xhci_wr(hc, hc->oper, XHCI_CONFIG, hc->max_slots);

    ring_arena_used = 0;

    uintptr_t cmd = arena_alloc(XHCI_CMD_RING_SIZE * XHCI_TRB_SIZE);
    uintptr_t evt = arena_alloc(XHCI_EVENT_RING * XHCI_TRB_SIZE);
    uintptr_t dcbaap = arena_alloc((XHCI_MAX_SLOTS + 1) * 8);
    if (!cmd || !evt || !dcbaap) {
        serial_write_string("xHCI: ring arena exhausted\n");
        return 0;
    }
    hc->cmd_ring_phys = cmd;
    hc->event_ring_phys = evt;
    hc->dcbaa_phys = dcbaap;

    memset((void*)cmd, 0, XHCI_CMD_RING_SIZE * XHCI_TRB_SIZE);
    memset((void*)evt, 0, XHCI_EVENT_RING * XHCI_TRB_SIZE);
    memset((void*)dcbaap, 0, (XHCI_MAX_SLOTS + 1) * 8);

    // The command ring's last TRB links back to slot 0 with its toggle cycle
    // bit set, so the ring walks forever without rewriting it.
    {
        xhci_trb_t* ring = (xhci_trb_t*)cmd;
        uint32_t last = XHCI_CMD_RING_SIZE - 1;
        ring[last].parameter_lo = (uint32_t)cmd;
        ring[last].parameter_hi = 0;
        ring[last].control = ((TRB_LINK << TRB_TR_TYPE_SHIFT) | TRB_TC | 1u);
    }
    hc->cmd_enqueue = 0;
    hc->cmd_cycle = 1;
    hc->event_dequeue = 0;
    hc->event_cycle = 1;

    // Point the controller at the command and device-context rings.
    xhci_wr(hc, hc->oper, XHCI_CRCR, (uint32_t)cmd | XHCI_CRCR_RCS);
    xhci_wr(hc, hc->oper, XHCI_CRCR + 4, 0);
    xhci_wr(hc, hc->oper, XHCI_DCBAAP, (uint32_t)dcbaap);
    xhci_wr(hc, hc->oper, XHCI_DCBAAP + 4, 0);

    // Program the event ring segment table: one segment covering the whole ring.
    // ERSTSZ has to land before the segment table address, because a write to
    // the address' high half is what makes the controller read the segment
    // table back, and it skips the whole event ring when the size is still zero.
    {
        xhci_trb_t* erse = (xhci_trb_t*)arena_alloc(64);
        if (!erse) { serial_write_string("xHCI: erst alloc failed\n"); return 0; }
        erse[0].parameter_lo = (uint32_t)evt;
        erse[0].parameter_hi = 0;
        erse[0].status = XHCI_EVENT_RING - 1;
        erse[0].control = 0;

        uint32_t intr = hc->rtsoff + 0x20;
        xhci_wr(hc, intr, XHCI_ERSTSZ, 1);
        xhci_wr(hc, intr, XHCI_ERSTBA_LOW, (uint32_t)(uintptr_t)erse);
        xhci_wr(hc, intr, XHCI_ERSTBA_HI, 0);

        // Point the dequeue pointer at the first TRB and clear the pending bit.
        xhci_wr(hc, intr, XHCI_ERDP_LOW, (uint32_t)evt);
        xhci_wr(hc, intr, XHCI_ERDP_HI, 0);

        // The interrupter's enable bit is what lets events through. The driver
        // polls rather than taking the interrupt, but the pending bit is still
        // set on each event, and clearing it here is what tells the controller
        // the event has been seen.
        xhci_wr(hc, intr, XHCI_IMAN, XHCI_IMAN_IE);
        xhci_wr(hc, intr, XHCI_IMOD, 0);
    }

    // Run.
    xhci_wr(hc, hc->oper, XHCI_USBCMD, XHCI_CMD_RS | XHCI_CMD_INTE);


    xhc_count++;
    serial_write_string("xHCI: controller running\n");

    // Anything already on a root port gets enumerated now, so the keyboard and
    // mouse are live before the desktop starts drawing.
    xhci_probe_ports(hc);
    return 0;
}



// ----------------------------------------------------------------------------
// Device context setup
//
// Addressing a device needs an input context holding a control context (which
// fields to add), a slot context (which port and what speed) and an endpoint-0
// context (the control endpoint's parameters). The controller copies that into
// a device context it allocates, and the DCBAA entry for the slot then points at
// it. The address itself is assigned by the ADDRESS_DEVICE command, so the
// device answers on the slot's address afterwards.
// ----------------------------------------------------------------------------

// The slot context is 32 bytes and its useful fields are: dword 0 the route
// string, dword 1 the route parent and hub, the port number and the speed. A
// device straight off the root hub has a zero route string.
static uintptr_t xhci_input_context2(xhci_hc_t* hc, uint32_t add_mask,
                                     uint32_t slot_dw0, uint32_t slot_dw1,
                                     uint32_t ep0_ctx) {
    // A context is 32 bytes; an input context starts with the control context,
    // then the slot context, then the endpoint contexts. Slot 1 and endpoint 1
    // are the only ones this driver needs before the device is configured.
    uintptr_t phys = arena_alloc((1 + 31) * hc->context_size);
    if (!phys) return 0;

    uint32_t* ctx = (uint32_t*)phys;
    memset(ctx, 0, (1 + 31) * hc->context_size);

    // Control context: which contexts to add.
    ctx[0] = 0;
    ctx[1] = add_mask;

    // Slot context sits 32 bytes in.
    ctx[(CTX_SIZE / 4) + 0] = slot_dw0;
    ctx[(CTX_SIZE / 4) + 1] = slot_dw1;
    ctx[(CTX_SIZE / 4) + 2] = 0;
    ctx[(CTX_SIZE / 4) + 3] = 0;

    // Endpoint 0 context.
    ctx[(2 * CTX_SIZE / 4) + 0] = 0;
    ctx[(2 * CTX_SIZE / 4) + 1] = ep0_ctx;
    ctx[(2 * CTX_SIZE / 4) + 2] = 0;
    ctx[(2 * CTX_SIZE / 4) + 3] = 0;
    ctx[(2 * CTX_SIZE / 4) + 4] = 0;

    return phys;
}

// Context bits the control context asks for: slot and endpoint 0.
#define XHCI_CTX_ADD_SLOT 0x1u
#define XHCI_CTX_ADD_EP0  0x2u

// Build the input context for a device on a known port and speed, and address
// it into the slot. The device is left at the default address afterwards,
// which is what GET_DESCRIPTOR needs.
static int xhci_address_device(xhci_hc_t* hc, uint32_t slot_id, uint32_t port,
                               uint32_t speed) {
    // The slot context names the root hub port and the speed. The route string
    // stays zero for a device straight off the root hub; a non-zero entry would
    // make the controller look for the device behind a hub instead.
    //
    // The port number lives in the *second* dword of the slot context, not the
    // first: dword 0 is the route string, and the route parent, hub and port
    // share dword 1. The port number the slot context carries is 1-based, and
    // the driver's port index is 0-based, so add one here.
    uint32_t slot_dw0 = 0;
    uint32_t slot_dw1 = ((port + 1) << SLOT_CTX_PORT_SHIFT) |
                        (speed << SLOT_CTX_SPEED_SHIFT);

    // Endpoint 0 is a control endpoint. Its max packet size comes from the
    // speed, because a device at the default address always uses the speed's
    // default, and the endpoint state starts halted with a dequeue cycle of 1.
    uint32_t max_packet = (speed >= XHCI_PORT_SPEED_HS) ? 64 : 8;
    uint32_t ep0_ctx = (max_packet << EP_CTX_MAXPSIZE_SHIFT) |
                       (EP_TYPE_CONTROL << EP_CTX_TYPE_SHIFT) |
                       (3u << EP_CTX_CERR_SHIFT);

    uintptr_t input = xhci_input_context2(hc, XHCI_CTX_ADD_SLOT | XHCI_CTX_ADD_EP0,
                                          slot_dw0, slot_dw1, ep0_ctx);
    if (!input) return 0;

    // The controller writes the device context it allocates into the address the
    // DCBAA holds for this slot, so that pointer must be valid before the
    // device is addressed or the command is refused.
    {
        uint64_t* dcbaa = (uint64_t*)hc->dcbaa_phys;
        uintptr_t device_ctx = arena_alloc((1 + 31) * hc->context_size);
        if (!device_ctx) return 0;
        memset((void*)device_ctx, 0, (1 + 31) * hc->context_size);
        dcbaa[slot_id] = device_ctx;
    }


    uint32_t code = xhci_run_command(hc,
                                     (CR_ADDRESS_DEVICE << TRB_TR_TYPE_SHIFT) |
                                     (slot_id << 24),
                                     (uint32_t)input, 0, 0);
    if (code != CC_SUCCESS) {
        char line[64];
        sprintf(line, "xHCI: address slot %u failed code %u\n", slot_id, code);
        serial_write_string(line);
        return 0;
    }
    return 1;
}


// ----------------------------------------------------------------------------
// Control transfers on endpoint 0
//
// A control transfer is three TRBs: SETUP, one or more DATA, and STATUS. The
// ring is per-endpoint and the doorbell for the slot's endpoint 0 wakes it.
// Completion is seen by walking the endpoint's ring, the same way the event
// ring is walked.
// ----------------------------------------------------------------------------

// Wait until the endpoint's ring shows the submitted TRB completed.
static int xhci_wait_ep(xhci_hc_t* hc, xhci_slot_t* slot) {
    uintptr_t ring = (uintptr_t)slot->ep_ring_phys;

    for (int i = 0; i < 2000; i++) {
        xhci_poll_events(hc, xhci_event_handler);

        // The ring's cycle bit flips as the controller consumes TRBs, so a
        // submitted TRB whose cycle bit does not match has been completed.
        uint32_t control = ((xhci_trb_t*)ring)->control;
        if ((control & 1u) != slot->ep_cycle && slot->ep_pending == 0) {
            return 1;
        }
        // Re-arm the one-TRDs ring for the next transfer.
        xhci_wait_frames(1);
    }
    return 0;
}

static int xhci_control_transfer(xhci_hc_t* hc, xhci_slot_t* slot,
                                 uint8_t request_type, uint8_t request,
                                 uint16_t value, uint16_t index, uint16_t length,
                                 void* data) {
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
    uint8_t status_pid = (length > 0 && (request_type & 0x80)) ? USB_PID_OUT : USB_PID_IN;

    // SETUP TRB: the immediate-data flag means the eight bytes of the setup
    // packet travel in the TRB itself.
    uint32_t setup_control = (TRB_SETUP << TRB_TR_TYPE_SHIFT) | TRB_IDT | TRB_TC;
    setup_control |= 8u;    // transfer length in the status field
    memcpy(&slot->setup_buffer, setup, 8);
    uint32_t setup_param = *(uint32_t*)&slot->setup_buffer[0];
    uint32_t setup_param_hi = *(uint32_t*)&slot->setup_buffer[4];

    uintptr_t ring = (uintptr_t)slot->ep_ring_phys;
    xhci_trb_t* trb = (xhci_trb_t*)ring;

    trb[0].parameter_lo = setup_param;
    trb[0].parameter_hi = setup_param_hi;
    trb[0].status = 8;
    trb[0].control = setup_control | slot->ep_cycle;

    // DATA and STATUS TRBs, if any.
    if (length > 0) {
        trb[1].parameter_lo = (uint32_t)(uintptr_t)data;
        trb[1].parameter_hi = 0;
        trb[1].status = length;
        trb[1].control = (TRB_DATA << TRB_TR_TYPE_SHIFT) |
                        (data_pid == USB_PID_IN ? TRB_DIR : 0) |
                        TRB_IOC | slot->ep_cycle;

        trb[2].parameter_lo = 0;
        trb[2].parameter_hi = 0;
        trb[2].status = 0;
        trb[2].control = (TRB_STATUS << TRB_TR_TYPE_SHIFT) |
                        (status_pid == USB_PID_IN ? TRB_DIR : 0) |
                        TRB_IOC | slot->ep_cycle;
    } else {
        trb[1].parameter_lo = 0;
        trb[1].parameter_hi = 0;
        trb[1].status = 0;
        trb[1].control = (TRB_STATUS << TRB_TR_TYPE_SHIFT) |
                        (status_pid == USB_PID_IN ? TRB_DIR : 0) |
                        TRB_IOC | slot->ep_cycle;
    }

    slot->ep_pending = 1;

    // Wake the slot's control endpoint.
    xhci_wr(hc, hc->dboff, slot->slot_id * hc->dbstride, XHCI_DB_EP0);

    if (!xhci_wait_ep(hc, slot)) {
        slot->ep_pending = 0;
        return -1;
    }

    slot->ep_pending = 0;
    slot->toggle ^= 1;
    return length;
}

// ------------------------------------------------------------------------// USB HID keyboard usage -> the character code the shell understands. This is
// the same translation the UHCI driver performs, kept here so both controllers
// feed the keyboard the same way.
static char xhci_key_usage_to_char(uint8_t usage, uint8_t shift, uint8_t caps_lock) {
    if (usage >= 0x04 && usage <= 0x1D) {
        // Letters only: caps lock affects these, and shift cancels it out.
        shift = (uint8_t)(shift ^ caps_lock);
        if (shift) return (char)('A' + (usage - 0x04));
        return (char)('a' + (usage - 0x04));
    }
    if (usage >= 0x1E && usage <= 0x27) {
        if (shift) return "!@#$%^&*()"[usage - 0x1E];
        return "1234567890"[usage - 0x1E];
    }
    if (!shift) {
        switch (usage) {
            case 0x28: return '\n';
            case 0x2A: return '\b';
            case 0x2B: return '\t';
            case 0x2C: return ' ';
            case 0x2D: return '-';
            case 0x2E: return '=';
            case 0x2F: return '[';
            case 0x30: return ']';
            case 0x31: return '\\';
            case 0x33: return ';';
            case 0x34: return '\'';
            case 0x35: return '`';
            case 0x36: return ',';
            case 0x37: return '.';
            case 0x38: return '/';
            case 0x4F: return KEY_RIGHT;
            case 0x50: return KEY_LEFT;
            case 0x51: return KEY_DOWN;
            case 0x52: return KEY_UP;
            default: break;
        }
    }
    return 0;
}

// Decode a boot protocol keyboard report into characters. Only newly pressed
// keys produce a character; held keys repeat on the PS/2 path only, so this
// matches that behaviour.
static void xhci_keyboard_report(xhci_slot_t* slot, const uint8_t* report) {
    uint8_t shift = (uint8_t)(((report[0] & 0x02) || (report[0] & 0x20)) ? 1 : 0);
    uint8_t caps = (slot->prev_report[0] & 0x04) && (report[0] & 0x04) ? 1u : 0u;
    caps = (uint8_t)(caps ^ (report[0] & 0x04) ? 0 : 0);
    caps = shift ? 0 : caps;

    // Caps lock state is held per slot: it toggles on each press of usage 0x39.
    static uint8_t caps_state[XHCI_MAX_SLOTS];

    for (int i = 2; i < 8; i++) {
        uint8_t usage = report[i];
        if (usage <= 1) continue;

        // Already held?
        int held = 0;
        for (int j = 2; j < 8; j++) {
            if (slot->prev_report[j] == usage) { held = 1; break; }
        }
        if (held) continue;
        if (usage >= 0xE0 && usage <= 0xE7) continue;

        if (usage == 0x39) {
            caps_state[slot->slot_id - 1] ^= 1u;
            continue;
        }

        char c = xhci_key_usage_to_char(usage, shift, caps_state[slot->slot_id - 1]);
        if (c) keyboard_inject_char(c);
    }

    memcpy(slot->prev_report, report, 8);
}

// ----------------------------------------------------------------------------
// HID device setup----
// HID device setup
//
// A device that answered its descriptors and has been addressed is switched to
// the boot protocol, given its interrupt endpoint, and from then on reports
// through that endpoint's ring.
// ----------------------------------------------------------------------------

static void xhci_hid_setup(xhci_hc_t* hc, xhci_slot_t* slot) {
    usb_device_descriptor_t dev_desc;
    static usb_config_descriptor_t cfg_desc;
    static uint8_t cfg_buf[256];

    // The default address only answers the first eight bytes of its device
    // descriptor, which is what the endpoint-0 max packet size needs.
    memset(&dev_desc, 0, sizeof(dev_desc));
    if (xhci_control_transfer(hc, slot, 0x80, USB_REQ_GET_DESCRIPTOR,
                              (uint16_t)(USB_DESC_DEVICE << 8), 0, 8,
                              &dev_desc) < 8) {
        return;
    }

    // A USB address: the ADDRESS_DEVICE command already moved the device off
    // the default address, which is the slot number here.
    memset(&dev_desc, 0, sizeof(dev_desc));
    if (xhci_control_transfer(hc, slot, 0x80, USB_REQ_GET_DESCRIPTOR,
                              (uint16_t)(USB_DESC_DEVICE << 8), 0,
                              (uint16_t)sizeof(dev_desc), &dev_desc) < 0) {
        return;
    }

    // Walk the configuration for a boot protocol HID interface and its
    // interrupt IN endpoint.
    memset(&cfg_desc, 0, sizeof(cfg_desc));
    if (xhci_control_transfer(hc, slot, 0x80, USB_REQ_GET_DESCRIPTOR,
                              (uint16_t)(USB_DESC_CONFIGURATION << 8), 0,
                              (uint16_t)sizeof(cfg_desc), &cfg_desc) < 0) {
        return;
    }

    uint16_t total = cfg_desc.total_length;
    if (total < sizeof(cfg_desc) || total > sizeof(cfg_buf)) return;

    memset(cfg_buf, 0, total);
    if (xhci_control_transfer(hc, slot, 0x80, USB_REQ_GET_DESCRIPTOR,
                              (uint16_t)(USB_DESC_CONFIGURATION << 8), 0, total,
                              cfg_buf) < total) {
        return;
    }

    int is_keyboard = 0;
    int on_hid = 0;
    uint8_t endpoint = 0;
    uint16_t max_packet = 8;
    uint8_t interval = 10;

    const uint8_t* p = cfg_buf;
    const uint8_t* end = cfg_buf + total;
    while (p + 2 <= end) {
        uint8_t len = p[0];
        uint8_t type = p[1];
        if (len < 2 || p + len > end) break;

        if (type == USB_DESC_INTERFACE) {
            usb_interface_descriptor_t iface;
            memcpy(&iface, p, len);
            on_hid = (iface.interface_class == USB_CLASS_HID &&
                     iface.interface_subclass == USB_SUBCLASS_HID_BOOT &&
                     (iface.interface_protocol == USB_PROTOCOL_HID_KEYBOARD ||
                      iface.interface_protocol == USB_PROTOCOL_HID_MOUSE));
            is_keyboard = (iface.interface_protocol == USB_PROTOCOL_HID_KEYBOARD);
        } else if (type == USB_DESC_ENDPOINT) {
            usb_endpoint_descriptor_t ep;
            memcpy(&ep, p, len);
            if ((ep.endpoint_address & 0x80) &&
                (ep.attributes & 0x03) == USB_EP_TYPE_INTERRUPT) {
                endpoint = (uint8_t)(ep.endpoint_address & 0x0F);
                max_packet = ep.max_packet_size;
                interval = ep.interval;
                break;
            }
        }
        p += len;
    }

    if (!on_hid || endpoint == 0) return;

    if (xhci_control_transfer(hc, slot, 0x00, USB_REQ_SET_CONFIGURATION,
                              cfg_desc.configuration_value, 0, 0, 0) < 0) {
        return;
    }
    if (xhci_control_transfer(hc, slot, 0x21, USB_REQ_SET_PROTOCOL, 0, 0, 0, 0) < 0) {
        return;
    }
    if (xhci_control_transfer(hc, slot, 0x21, USB_REQ_SET_IDLE, 0, 0, 0, 0) < 0) {
        return;
    }

    slot->present = 1;
    slot->is_keyboard = (uint8_t)(is_keyboard ? 1 : 0);
    slot->report_size = (uint8_t)(is_keyboard ? USB_BOOT_KEYBOARD_REPORT
                                               : USB_BOOT_MOUSE_REPORT);
    if (slot->report_size > max_packet) {
        slot->report_size = (uint8_t)max_packet;
    }
    for (int i = 0; i < 8; i++) slot->prev_report[i] = 0;

    char line[64];
    sprintf(line, "xHCI: %s on slot %u endpoint %d interval %d\n",
            is_keyboard ? "keyboard" : "mouse", slot->slot_id,
            (int)endpoint, (int)interval);
    serial_write_string(line);
}

// Enable a slot and address the device on a port into it.
static void xhci_probe_port(xhci_hc_t* hc, uint32_t port) {
    if (!xhci_port_connected(hc, port)) return;
    if (!xhci_port_enable(hc, port)) return;

    uint32_t speed = xhci_port_speed(hc, port);

    // ENABLE_SLOT hands out the lowest free slot. Its result is the slot id.
    uint32_t code = xhci_run_command(hc, (CR_ENABLE_SLOT << TRB_TR_TYPE_SHIFT),
                                    0, 0, 0);
    if (code != CC_SUCCESS) {
        char line[64];
        sprintf(line, "xHCI: enable slot failed code %u\n", code);
        serial_write_string(line);
        return;
    }

    uint32_t slot_id = command_slot;
    if (slot_id == 0 || slot_id > XHCI_MAX_SLOTS) return;

    xhci_slot_t* slot = &hc->slots[slot_id - 1];
    memset(slot, 0, sizeof(*slot));
    slot->slot_id = (uint8_t)slot_id;
    slot->port_index = (uint8_t)port;
    slot->address = (uint8_t)slot_id;
    slot->ep_cycle = 1;
    slot->ep_enqueue = 0;

    // The endpoint ring is per device and must be in its own 64 KiB window, so
    // it comes out of the same arena as the controller's rings.
    uintptr_t ring = arena_alloc(XHCI_EP_RING_SIZE * XHCI_TRB_SIZE);
    if (!ring) { serial_write_string("xHCI: ep ring alloc failed\n"); return; }
    slot->ep_ring_phys = ring;

    {
        xhci_trb_t* trbs = (xhci_trb_t*)ring;
        for (uint32_t i = 0; i < XHCI_EP_RING_SIZE; i++) {
            trbs[i].parameter_lo = 0;
            trbs[i].parameter_hi = 0;
            trbs[i].status = 0;
            trbs[i].control = 0;
        }
        // The last TRB links back to the head.
        uint32_t last = XHCI_EP_RING_SIZE - 1;
        trbs[last].parameter_lo = (uint32_t)ring;
        trbs[last].control = ((TRB_SETUP << TRB_TR_TYPE_SHIFT) | 1u);
    }

    if (!xhci_address_device(hc, slot_id, port, speed)) {
        serial_write_string("xHCI: addressing device failed\n");
        return;
    }

    slot->low_speed = (speed < XHCI_PORT_SPEED_HS) ? 1 : 0;
    xhci_hid_setup(hc, slot);
}

// Walk every root port and enumerate whatever is on it.
// Walk every root port and enumerate whatever is on it.
static void xhci_probe_ports(xhci_hc_t* hc) {
    for (uint32_t port = 0; port < hc->max_ports && port < 16; port++) {
        if (!xhci_port_connected(hc, port)) continue;
        xhci_probe_port(hc, port);
    }
}

void xhci_init(void) {
    serial_write_string("xHCI: probing controllers\n");
    pci_scan_class(0x0C, 0x03, 0x30, xhci_probe, 0);
    if (xhc_count == 0) {
        serial_write_string("xHCI: no xHCI controller found\n");
    }
}

void xhci_poll(void) {
    for (int i = 0; i < xhc_count; i++) {
        xhci_hc_t* hc = &xhcs[i];

        for (int s = 0; s < XHCI_MAX_SLOTS; s++) {
            xhci_slot_t* slot = &hc->slots[s];
            if (!slot->present) continue;

            // The ring's cycle bit moves as the controller consumes a TRB, so
            // a mismatch on the head of the ring means a report completed.
            const xhci_trb_t* trb = (const xhci_trb_t*)slot->ep_ring_phys;
            uint32_t control = trb->control;
            if ((control & 1u) == slot->ep_cycle) continue;

            // Completion code lives in the top byte of the status field.
            uint32_t code = (trb->status >> 24) & 0xFFu;
            if (code != CC_SUCCESS && code != CC_SHORT_PACKET) {
                continue;
            }

            uint8_t* report = slot->buffer;
            if (slot->is_keyboard) {
                xhci_keyboard_report(slot, report);
            } else {
                mouse_inject_relative((int8_t)report[1], (int8_t)report[2],
                                      report[0] & 0x07);
            }

            // Re-arm the head for the next report.
            slot->ep_cycle = (uint8_t)(slot->ep_cycle ^ 1);
        }
    }
}

int xhci_keyboard_connected(void) {
    for (int i = 0; i < xhc_count; i++) {
        xhci_hc_t* hc = &xhcs[i];
        for (int s = 0; s < XHCI_MAX_SLOTS; s++) {
            if (hc->slots[s].present && hc->slots[s].is_keyboard) return 1;
        }
    }
    return 0;
}

int xhci_mouse_connected(void) {
    for (int i = 0; i < xhc_count; i++) {
        xhci_hc_t* hc = &xhcs[i];
        for (int s = 0; s < XHCI_MAX_SLOTS; s++) {
            if (hc->slots[s].present && !hc->slots[s].is_keyboard) return 1;
        }
    }
    return 0;
}