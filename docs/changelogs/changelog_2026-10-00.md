# GamerOS Changelog (2026-10-00)

> **WORK IN PROGRESS — DO NOT SHIP.** Build 1.570 is unfinished, and some of
> the entries below are only compile-verified. Treat this as a running log.

## Release

- Version: `00m2`
- Build: `1.570`
- Status: Unreleased. Work in progress; still being verified.

<!-- New entries go below this line. Keep entries one bullet per change. -->


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