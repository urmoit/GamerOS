# GamerOS Changelog (2026-10-14)

Build 1.560 adds the input and boot plumbing the desktop was missing: a real USB
host driver so USB keyboards and mice work, Multiboot 2 handover support, and a
pointer abstraction that lets VMware, VirtualBox and USB report into the same
mouse state the shell reads. Everything below was verified under QEMU in a
container, with the guest's serial log read back to confirm behaviour.

## Release
- Version: `00m2`
- Build: `1.560`
- Date: `2026-10-14`
- Status: **Released.**

## Multiboot 2

- **The kernel now boots from a Multiboot 2 header as well as the Multiboot 1 one
  it always carried.** `boot.asm` emits both: the Multiboot 2 header first, then
  the Multiboot 1 header. The entry stub saves the boot magic from `EAX` and the
  information pointer from `EBX` before touching any register, and a new
  `multiboot2_build()` normalises either handover into the one `multiboot_info_t`
  layout the rest of the kernel consumes. `multiboot2` and `multiboot` both boot;
  the printed `Boot protocol: Multiboot 2` / `Multiboot 1` line confirms which.
- **This surfaced two boot bugs that had been invisible until now.** GRUB's
  Multiboot 2 loader was never reached, so its stricter behaviour exposed them:

  1. **A framebuffer tag in the Multiboot 2 header made GRUB refuse to load the
     image at all.** GRUB's `grub_multiboot2_load()` calls
     `grub_multiboot2_set_console()` with the requested width, height and depth
     and returns that error straight out of the command. When it cannot set the
     mode it does not fall back the way the Multiboot 1 path does, so the guest
     never started and the serial log was empty. The Multiboot 2 header now
     carries only the mandatory end tag and requests no console; `boot.asm` sets
     up its own VGA mode before entering long mode, so the graphics path does not
     depend on the loader.
  2. **The 64-bit handover stub folded the boot magic into the pointer.**
     `mov rsi, [multiboot_ptr]` is an 8-byte load and the two BSS slots sit next
     to each other, so `multiboot_magic` landed in the pointer's high half and
     `multiboot2_build()` walked a garbage address. Both slots are now loaded as
     32-bit words, which zero-extends correctly because the kernel image lives
     below 4 GiB. Verified by dumping `RBX=0x36d7628900100120` at the faulting
     instruction.

- **GRUB's `multiboot2` command is not in `grub-mkrescue`'s default module set**,
  so a config using it needs the module shipped inside the ISO. The ISO's
  `grub.cfg` still uses `multiboot` (which is present), and the loader command in
  `config/targets/x86_64/grub.cfg` records why.

## USB HID — UHCI host controller, boot protocol keyboard and mouse

- **USB keyboards and mice now work where they previously did nothing.** The old
  `usb_mouse.c` was a stub: every control transfer returned 1 immediately and no
  schedule was ever built. A real stack replaced it:

  - **`src/drivers/bus/usb.c`** — a UHCI host driver (frame list, queue head and
    transfer descriptor management, control and interrupt transfers), device
    enumeration, and boot protocol HID keyboards and mice.
  - **`src/drivers/bus/pci.c`** — a PCI helper with a class scan, so USB
    controllers, the VMM device and anything else can be found by class code
    instead of by fixed address.
- **Enumeration follows the USB spec**: read 8 bytes of the device descriptor to
  learn the control endpoint packet size, assign an address (the request itself
  is still addressed to the default address 0), read the full descriptors, then
  switch the interface to the boot protocol so no report descriptor parsing is
  needed. All of it is synchronous because the desktop loop runs with interrupts
  off and polls input.
- **Devices are enumerated top-down, including hubs.** A hub is detected, its
  port count is read from the hub descriptor, and each port is powered and reset.
  A device behind a hub is tracked with its parent and port index so a later hot
  plug can re-read just that port. Devices attached directly to a root hub port
  enumerate and work.
- **Known limitation — devices behind a hub need split transactions.** UHCI
  reaches a full-speed device behind a hub only with an SS/CS split pair. The hub
  itself is detected and its ports are powered, but a child device there will not
  enumerate. This is why QEMU's auto-created hub holds a second USB device back,
  and why `run-qemu.bat` attaches the keyboard and mouse to root ports.
- Scope is UHCI only. EHCI covers high-speed traffic HID boot devices do not use
  and xHCI needs its own TRB machinery, so the PCI probe skips both rather than
  half-driving them. QEMU's `-device qemu-xhci` therefore does not work with this
  stack.

## Shared pointer state

- **A PS/2 mouse, a USB HID mouse and the hypervisor absolute pointers now report
  into one place.** `mouse.c` keeps the clamping logic in one helper and exports
  `mouse_set_position`, `mouse_inject_relative`, `mouse_set_buttons` and
  `mouse_inject_wheel`, so a non-PS/2 device cannot drift out of step with the
  shell. `keyboard.c` exports `keyboard_inject_char()` on the same footing.
- **The USB boot keyboard maps its own usage table.** `hid_usage_to_char()` covers
  the USB HID keyboard usage IDs for letters, digits, punctuation, the keypad,
  shift, caps lock, and the special `KEY_LEFT/RIGHT/UP/DOWN/HOME/END/INS/DEL` codes
  the shell already understands.

## Hypervisor absolute pointers

- **VMware's absolute pointer protocol is now used.** The backdoor is probed with
  a `GET_VERSION` request and only taken at face value when the answer actually
  looks like VMware — the previous check passed on any machine, so the guest
  claimed an absolute pointer it never had. `vmware_enable_absolute()` verifies
  each command was accepted instead of assuming it. Buttons are translated from
  VMware's own bit encoding (0x20 left, 0x10 right, 0x08 middle) rather than
  passed through.
- **VirtualBox's VMM device is supported.** The VMM device sits on PCI at vendor
  `0x80EE`, device `0xCAFE`; a request block is submitted by writing its physical
  address to BAR0 and the host answers in place. The driver advertises
  "guest supports absolute" and reads the reported position when the host wants
  absolute coordinates, which is the condition where pointer capture is removed.
  Buttons keep arriving over PS/2 in VirtualBox, so they are left alone.

## Build

- **The build was broken and would not link.** `timezone.o` was referenced by
  `main.c` and `settings_ui.c` but was missing from `C_SRC` and `C_OBJ` in both
  Makefiles, so every build failed with `undefined reference to timezone_get_*`.
  Adding it restores the build, and the expected `kernel.elf` (0x166000 bytes,
  entry `0x101300`) links cleanly with no warnings under `-Wall -Wextra`.
- The dead USB stubs (`usb_uhci.c`, `usb_ehci.c`, `usb_xhci.c`, `usb_mouse.c`,
  `usb_mouse.h`) were never in the build. They were removed; `usb.c` replaces
  them.

## Verification

All of the above was checked by booting the built ISO under QEMU in a container
and reading the guest's serial log back:

- `Boot protocol: Multiboot 2` prints, and the guest reaches the desktop main loop.
- `USB: keyboard on port (address 1, endpoint 1)` and
  `USB: mouse on port (address 1, endpoint 1)` print when one device is attached
  per UHCI controller.
- `Boot protocol: Multiboot 1` still prints with the old loader command, so the
  legacy path is intact.
- The VMware detection change was confirmed by no longer printing
  `VMware: absolute pointer enabled` on QEMU, which is not running VMware.
