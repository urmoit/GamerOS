# GamerOS Changelog (2026-10-00)

> **WORK IN PROGRESS — DO NOT SHIP.** Build 1.570 is unfinished. The Multiboot 2,
> UHCI USB and hypervisor pointer work is boot-verified under QEMU and is
> recorded in the Verification section; the rest is still being verified. Treat
> this as a running log.

## Release

- Version: `00m2`
- Build: `1.570`
- Status: Unreleased. Work in progress; still being verified.

<!-- New entries go below this line. Keep entries one bullet per change. -->

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
  port count is read from the hub descriptor, and each port is powered, reset
  and waited on. Devices attached directly to a root hub port enumerate and work.
- **Devices behind a hub do not enumerate, and that is a hardware gap, not a
  driver bug.** UHCI reaches a full-speed device behind a hub only with an SS/CS
  split pair, which is not implemented. QEMU's UHCI model does not implement
  them either — `hw/usb/dev-hub.c`'s `usb_hub_complete()` says *"If we ever
  implement usb 2.0 split transactions this will become a little more
  complicated"* — so the hub's port is enabled correctly and then the emulator
  clears the enable bit. This is why QEMU's auto-created hub holds a second USB
  device back, and why `run-qemu.bat` attaches the keyboard and mouse to root
  ports. A hub-port reset on QEMU was traced end to end: the reset is issued,
  the port reports enabled, and the status then regresses to connected-only.

## USB HIDs on xHCI — controller up, device addressing outstanding

- **An xHCI driver was added and brings the controller fully up.** Reset, the
  command ring and its link TRB, the event ring segment table, the DCBAA, the
  port registers, and `ENABLE_SLOT` all work: the kernel reports the controller
  running with 8 slots and 8 ports, enumerates its ports, and `ENABLE_SLOT`
  returns a slot id. Memory-mapped register access was added to `ports.c` for
  this, because xHCI registers live behind a BAR that port instructions cannot
  reach.
- **`ADDRESS_DEVICE` does not complete, so no device is usable through xHCI
  yet.** QEMU traces the command reaching the device lookup, but the completion
  code the guest reads back is `CC_TRB_ERROR`. The driver is complete enough to
  continue from — slot allocation, context building, the doorbell and the event
  ring are all exercised and correct — and the remaining work is in that final
  command. UHCI remains the working path, which is why `run-qemu.bat` uses a
  UHCI controller.

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
  Adding it restores the build: 27 objects link cleanly with no warnings under
  `-Wall -Wextra`, and the entry point is `0x101300`.
- The dead USB stubs (`usb_uhci.c`, `usb_ehci.c`, `usb_xhci.c`, `usb_mouse.c`,
  `usb_mouse.h`) were never in the build. They were removed; `usb.c` replaces
  them.

## Customization

- **Added a Customization page to Settings.** It is the hub for shell
  appearance: a Taskbar row opens the new Taskbar page, and the current style
  and button size are listed underneath so the state is readable without
  navigating. The row publishes its click rectangle from the renderer the same
  way the toggles do, so the hit box cannot drift away from the pixels.
- **Added a Taskbar page.** The "Taskbar compact" toggle moved off System and
  the read-only Taskbar row moved off Personalization, so every taskbar setting
  is now in one place instead of being split across two pages, one of which
  could display it but not change it.
- **Added a taskbar style dropdown** with two entries. `Windows` renders the
  classic full-width bar flush with the bottom edge, and `macOS Dock` keeps the
  floating rounded panel. `dock_compute_layout()` branches on the choice but
  keeps the item order identical, so Start stays on the left and the clock
  stays in the tray in both styles; the renderer swaps the rounded panel,
  shadow and inset bevel for a square bar with a flat bevel. The dock and the
  bar share one layout call, so the click targets stay aligned with the pixels
  either way.
- **Made the Settings navigation rail adapt to the window height.** A fixed
  20px row pitch let the later rows fall past the bottom of the rail, where
  they still consumed a nav index the click handler could no longer reach.
  The pitch is now derived from the window height in one helper that both the
  renderer and `handle_settings_click()` call, and all thirteen rows remain
  visible and clickable even in a short window.
- **Bumped the build number from `1.560` to `1.570`** in `GAMEROS.INI`, the
  desktop watermark and the Settings About page, and repointed
  `tools/embed_changelog.py` at this file so the in-OS changelog viewer shows
  the running 1.570 log instead of the 1.550 release notes.

## Fixes

- **Fixed the Windows taskbar drawing its items at the top of the screen.**
  `dock_compute_layout()` set `start_y`, `taskbtn_y` and `clock_y` to the
  *relative* `DOCK_PAD_Y` while the dock branch converted them to absolute
  screen coordinates further down. The Windows branch returned before that
  conversion, so Start, the task buttons and the clock rendered at y=5 above
  an empty bar glued to the bottom edge. Each branch now writes the absolute
  `y + DOCK_PAD_Y` itself, so the two cannot drift. Checked with a standalone
  harness over 5 resolutions x 2 styles x compact/full x 0-6 windows, asserting
  that every item lands inside its panel and that no two overlap.
- **Fixed the desktop watermark ignoring the taskbar style.** It was anchored
  to `DOCK_RESERVED_H`, the *tallest* the taskbar can be, so it floated 7px
  higher than needed with the Windows bar. It now derives its position from the
  live dock geometry, keeping the same 10px gap above whatever is drawn, and it
  drops 6px further in the floating-dock style, which leaves room underneath.

## Modularisation

- **Moved every Settings component out of `main.c` into its own app module.**
  `src/ui/apps/settings/settings_ui.{c,h}` now owns all thirteen pages, the
  navigation rail, both dropdowns, every row primitive, the hit-box registries,
  the input handlers and the embedded changelog — roughly 1,150 lines that were
  interleaved with the window manager. The shell keeps only window plumbing and
  reads user choices back through accessors.
- **The two sides now talk through a declared interface.** `settings_ui.h`
  exposes the shell-facing API (title, draw, click, wheel, page selection and
  the taskbar preferences), and `shell_api.h` gained the shell-side services
  Settings needs: the debug-overlay switch, the UI scale, the display profile
  list and `append_text`/`append_unsigned`. No app reaches into a kernel static
  any more, which is the same contract Notepad already follows.
- `main.c` shrank from ~3,100 to ~2,860 lines and is down to the shell's own
  concerns. `tools/Makefile` and `tools/Makefile.wsl` both compile the module and
  resolve its header; both entry points now produce the same image.

## Verification

All of the boot-verified work below was checked by booting the built ISO under
QEMU in a container and reading the guest's serial log back:

- `Boot protocol: Multiboot 2` prints, and the guest reaches the desktop main
  loop with no triple faults.
- `USB: keyboard on port (address 1, endpoint 1)` and
  `USB: mouse on port (address 1, endpoint 1)` print when one device is attached
  per UHCI controller, which is the configuration `run-qemu.bat` uses.
- `Boot protocol: Multiboot 1` still prints with the old loader command, so the
  legacy path is intact, and the same USB devices enumerate under it.
- The VMware detection change was confirmed by no longer printing
  `VMware: absolute pointer enabled` on QEMU, which is not running VMware.
- A regression matrix was run across six QEMU USB configurations (one device per
  controller, keyboard only, mouse only, no USB, xhci with both devices, xhci
  with keyboard only). Every one booted to the desktop loop with zero triple
  faults, and the two UHCI configurations that should find devices did.

The Customization, Fixes and Modularisation entries above are UI work that is
verified by render harnesses rather than by booting, and the changelog viewer
embedding has not been regenerated since those entries landed.

## Planned

### Kernel

- **Populate a real IDT with exception handlers.** `IDT` still reads back as
  `0000000000000000 00000000`, so hardware interrupts work only because the
  firmware descriptor happens to be in place, and any fault triple-faults
  instead of reaching a handler. This is why the `CR4.OSFXSR` bug presented as
  an instant shutdown with no diagnostic at all.
- Add a panic path that prints the fault vector, error code and the registers
  QEMU captured, so a crash is diagnosable from the serial log.
- Replace the polled input loop with PIT/HPET timer-driven frame pacing.
- Add keyboard shortcuts: Alt+Tab window cycling, a Windows key, and Ctrl+Alt+Del.

### Storage

- Implement an AHCI (SATA) driver so VMware VMs without a legacy IDE controller
  can mount a real volume.
- Implement an NVMe driver over PCI for the same reason.
- Add a free-sector map so deleted space is reclaimed instead of leaked, and
  address the fragmentation the append-only allocator produces over time.
- Add a journal so an interrupted metadata write cannot corrupt the volume.
- Stop rewriting the whole file table on every write. `fs_save_metadata()`
  serialises all 256 records on each `fs_write_file()`, so saving a file costs
  the same whether the volume holds three files or three hundred.
- Add rename and move, and make `fs_delete_directory()` refuse non-empty
  directories instead of deleting the entry and orphaning its children.
- Raise the 24 KiB per-file cap in `MAX_FILE_SIZE`, and handle paths that
  exceed the 64-character `MAX_FILENAME_LEN` rather than rejecting them.
- Add a BIOS-style POST screen so storage detection and capacity are visible
  before the desktop loads, instead of only on the serial log.

### Shell

- Add Explorer sorting by column. The details view draws Name, Date modified,
  Type and Size, but only the zone dropdown is interactive so far; the reference
  layout shows a sort indicator on the header that does nothing yet.
- Expose create, rename and delete in the Explorer UI. The filesystem already
  has `fs_create_file`, `fs_delete_file` and `fs_delete_directory`; nothing
  calls them.
- Give Notepad open/save dialogs instead of the single fixed file path.
- Taskbar: window previews on hover, overflow grouping, and show-desktop.
- Visually confirm the loading screen; the desktop, taskbar, watermark, Start
  menu and Explorer have all been captured as rendered frames.

### Settings and time

- Persist Settings state so toggles survive a reboot instead of resetting.
- Allow the clock to be set. The RTC is read-only today, so the time zone can
  only shift the displayed time; nothing corrects a clock that has drifted.
- Cover the remaining daylight-saving rules. Iran, Chile, and Lord Howe Island
  are approximated by one of the four implemented rules rather than their real
  transitions.
- Remember each Settings tab's scroll position between visits.

### USB

- Implement UHCI split transactions (SS/CS) so devices behind a hub enumerate.
  This is what currently keeps a second USB device off a single controller, and
  QEMU's UHCI model does not implement them either, so progress needs a
  different test host to confirm.
- Finish xHCI device addressing. The controller, rings, DCBAA, slot allocation
  and port enumeration all work; `ADDRESS_DEVICE` completes in QEMU but the
  guest reads back `CC_TRB_ERROR`, and the gap is in that last command.
