# GamerOS Changelog (2026-10-12)

> **WORK IN PROGRESS — DO NOT SHIP.** Build 1.570 is unfinished, and some of
> the entries below are only compile-verified. Treat this as a running log.

## Release

- Version: `00m2`
- Build: `1.560`
- Status: Unreleased. Work in progress; still being verified.

<!-- New entries go below this line. Keep entries one bullet per change. -->


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