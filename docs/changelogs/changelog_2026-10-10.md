# GamerOS Changelog (2026-10-10)

This changelog records the build-system, correctness, and stability fixes applied
after build 1.500. Most of these were latent defects that only surfaced once the
project was actually compiled — the tree did not build at all before this cycle.

## Release
- Version: `00m2`
- Build: `1.520`
- Date: `2026-10-10`

## Build Verification
- First verified clean compile and ISO build using the containerized toolchain
  (`config/buildenv/Dockerfile`).
- Result: **zero errors, zero warnings, zero undefined symbols**.
- Output: `dist/x86_64/kernel.iso` (5.5 MB, Multiboot compatible).

## Critical — Build Blockers

- **Fixed the kernel failing to compile.** `back_buffer_rgb` was declared `static`
  in `vga_graphics.c`, but `main.c` dereferenced it directly in four places, so
  every translation unit failed. Added a bounds-checked `graphics_get_rgb_row(int y)`
  accessor and declared it in `graphics.h`, keeping the buffer private.
- **Fixed conflicting `draw_string` declarations.** `graphics.h` declared it with
  `int` coordinates while `ui.h` redeclared it with `uint32_t`, which is a hard
  compile error wherever both headers are included. Removed the duplicate
  declaration from `ui.h`.
- **Fixed `kmalloc`/`kfree` being declared but never defined.** The executive
  layer (`object_manager.c`, `gdi.c`) and the widget toolkit call them, so that
  code could never have linked. Implemented a real first-fit allocator in
  `src/core/kernel/mm.c` over a 1 MB static arena, with block validation and
  double-free rejection via a block magic value.
- **Fixed the Makefile pattern rule pointing at a deleted directory.** It
  referenced `$(SRC_DIR)/impl/`, which no longer exists. Repointed it at
  `$(SRC_DIR)/`.
- **Removed an empty `settings_ui.c`** (0 bytes) from the build; it produced a
  useless object file.
- **Repaired `Makefile.wsl`.** It referenced nine source files that do not exist
  in the tree (`core/hal/hal.c`, `cpu/gdt.c`, `interrupts/{idt,isr}.c`,
  `io/ports.c`, `interrupts/isr.asm`, `cpu/context_switch.asm`) and could not
  build at all. Rewritten to mirror the real, working source set.

## Memory Safety

- **Fixed an out-of-bounds read in `fs_write_file()`.** When the payload exceeded
  the reserved sector run, the function inflated `file->size` to the full
  reserved size regardless of the real data length. The trailing sector loop then
  `memcpy`'d past the caller's buffer. This was reachable from the shell's
  in-OS changelog viewer, which writes a ~11 KB string through this path.
- **Fixed a container teardown leak in `ui_widgets.c`.** `ui_destroy_widget()`
  freed the container but never its children, leaking the whole subtree on every
  window/panel destroy.
- **Fixed a division by zero in progress-bar rendering** when `max_value` equalled
  `min_value`. Also clamps the computed width so it cannot exceed the bar.
- **Fixed an unbounded textbox append.** Keyboard input wrote into a fixed 256-byte
  buffer with no real capacity check. Now performs a bounded append and honours
  the cursor position.
- **Fixed an out-of-bounds array read in the scaled 32bpp present path.** When the
  logical resolution differed from the native framebuffer, `swap_buffers()` copied
  a full native-width row from a narrower logical row, walking into the next row
  and past the buffer on the final row. Now scales horizontally as well.
- **Fixed `&windows[-1]` being formed** when `active_window` was `-1`.
- **Fixed `uint32_t`-to-`int` truncation** in the window drag clamp, which could
  assign a huge unsigned value to a signed coordinate.

## Boot Stability

- **Eliminated 4,080 spurious NASM warnings.** `align 4096` inside the `.bss`
  section emits fill bytes, which NASM rejects — once per padding byte. Switched
  the three page-table alignments to `alignb`, which needs no fill.
- **Fixed a VGA blanking bug in `boot.asm`.** Both graphics-controller setup loops
  wrote `0x00` to GC register 8, the plane bit mask, disabling all plane writes.
  Now writes `0xFF`.
- **Fixed a missing glyph in `font.inc`.** The Space character had no `db` data,
  shifting every glyph in the table by one. The table is now a correct 96 glyphs
  (32-127). Also gave `~` a real glyph instead of a blank one, in both the
  assembly and C font tables.
- **Made the boot font include path independent of the working directory.** It
  relied on being invoked from `tools/`. Now resolved through a `-I` include path.

## Driver Hardening (Boot Hangs)

- **Bounded the serial port waits.** `serial_putchar()` and `serial_read()` spun
  forever when no UART was present. Both now time out; `serial_putchar()` writes
  the byte and returns rather than blocking indefinitely.
- **Bounded the PS/2 mouse command waits.** `mouse_send_cmd()` contained two
  `while (inb(...) & 0x02);` loops that would hang forever with no controller.
- **Added real PS/2 mouse presence detection.** `mouse_init()` now probes the
  controller and records availability via a new `mouse_is_present()`; the handler
  returns early when absent. The previous code claimed to verify presence but did
  not, despite the changelog stating it did.
- **Added acknowledgement draining** between mouse commands so stale replies do not
  desynchronise the command stream.
- **Bounded the keyboard drain loop in `keyboard_init()`**, which could spin
  indefinitely on a noisy controller.
- **Added a timeout for the keyboard extended-key prefix.** A `0xE0` prefix whose
  follow-up scancode never arrived could leave the next real key misinterpreted
  as extended.
- **Added RTC update-in-progress handling.** The RTC now waits out an in-progress
  update and retries on unstable reads, preventing garbled clock values.
- **Fixed integer overflow in the VMware backdoor.** Coordinates were scaled with
  a 32-bit multiply that wrapped. Now scaled in 64-bit with proper clamping, and
  the absolute-pointer status/data reads follow a consistent protocol.

## Correctness

- **Fixed `graphics_get_bpp()` reporting the wrong colour depth.** It returned 16
  for the indexed fallback path, but the kernel programs VGA mode 12h
  (640x480x16 planar = 4bpp). Settings and About now report the true value.
- **Fixed the Explorer's click targets not matching what was drawn.** Hit testing
  used row offsets (`y+44`) that disagreed with the rendered rows (`y+58`), so
  clicks selected the wrong entry. Geometry re-derived from the draw code, and the
  highlight rectangle now aligns with the list pane.
- **Fixed three duplicated and conflicting Settings click handlers.** The System
  page had three separate blocks handling the same toggles with three different
  offset schemes, so behaviour depended on which block matched first. Consolidated
  into one handler whose geometry matches the rendered rows.
- **Fixed a nav-row offset drift in Settings.** Tabs were drawn at `nav_y + 28` but
  hit-tested at `nav_y + 22`, so every tab selection was off by one row.
- **Guarded Settings against short windows.** Nav rows and display-profile rows
  could overflow a small window and draw over the taskbar.
- **Fixed the PCI USB log rows drawing off-screen** and removed an unused variable.
  Row calculation no longer mixes bus and device indices incorrectly.
- **Fixed `ui.c` clipping to a hardcoded 320x200** while the shell runs at 640x480,
  truncating all text and gradients.
- **Made `ui.c` screen-size agnostic** throughout, and removed an underflow risk in
  the desktop hill renderer at small heights.

## Build and Tooling

- **Fixed the ISO staging path.** It wrote to `/tmp`, which is unreliable on the
  Windows/MSYS2 hosts the project targets. Now stages inside the build tree.
- **Fixed all batch scripts running `make` from the wrong directory**, so builds
  failed unless invoked from `tools/`. They now resolve their own location.
- **Fixed a stale `%errorlevel%` bug in `run-qemu.bat` and `debug.bat`.** Inside a
  parenthesised block, `%errorlevel%` expands at parse time, so the QEMU
  `where` check was testing a pre-existing value. Now uses `if not errorlevel 1`.
- **Added `mm.o` to the Makefile** for the new allocator.
- **Stopped including `io.h` alongside `ports.h`** in `main.c`; they duplicate the
  same port I/O prototypes.
- **Parenthesised `HEAP_SIZE`** in `mm.h` so the macro is safe in expressions.

## Executive Layer

- **Resolved a symbol collision.** The executive filesystem manager defined
  `fs_open_file`, `fs_read_file`, `fs_create_file`, and others with entirely
  different signatures from the real ones in `fs.h`. If both had ever been linked,
  this would have been a hard conflict. Renamed to `exec_fs_*`.
- **Fixed a duplicate `mouse_state_t` typedef** in the executive mouse driver by
  including the shared `<mouse.h>`.
- **Made `get_object()` reject `INVALID_HANDLE` explicitly** rather than relying
  on the range check alone.

## UI

- **Redesigned Notepad.** The old layout drew text across `y+54 .. y+h-26` while
  the white editor surface only covered `y+54 .. y+h-82`, so **text overflowed the
  editor and ran into the status bar**; the visible-line count was derived from
  the same mismatched bounds. Notepad now has a toolbar with a working Save
  button, a line-numbered gutter, a status bar showing `Ln/Col` and the file
  name, and a caret that cannot be drawn outside the editor.
- **Redesigned File Explorer.** Replaced the duplicate title, raw path text, and
  `[DIR] ` prefixes with a toolbar, breadcrumb, folder/file markers, a column
  header, selection highlighting, and a status bar showing the item count.
- **Made the Explorer actually navigable.** Back and Forward now work via a real
  navigation history, Up is disabled at the filesystem root, and the Quick Access
  entries for Documents and Downloads navigate instead of doing nothing.
  Previously only "This PC" responded and the other rows were inert text that
  looked clickable.
- Removed four UI string stubs the redesign made unused.
- **Fixed the GamerOS Update view switch appearing broken.** The changelog and
  roadmap views rendered identical content because the roadmap split is keyed off
  a `## Planned` heading, and the 1.520 changelog had none. Restored the heading,
  and the view switch now shows genuinely different documents.
- **Made the roadmap split resilient.** The parser now recognises `Planned`,
  `Roadmap`, or `Upcoming`, so renaming the heading cannot silently collapse the
  two views again.
- **Show an explicit notice** when a view has no entries, instead of silently
  duplicating the other document.
- **Raised `MAX_FILE_SIZE` from 12KB to 24KB.** The full release changelog no
  longer overflows the per-file cap. The generator now reads this constant from
  `fs.h` rather than hardcoding it, so the two cannot drift. Sector budget
  re-checked: 48 reserved sectors per file x 32 files = 1536 of 4096.
- **Rebuilt the entire Settings layout, all eleven tabs.** Previously only System
  had been converted; the other ten pages were still flat left-aligned text at
  hardcoded `body_y + N` offsets.
- Settings pages now share a common layout system built on a **running cursor**:
  section headings render a coloured label with an accent tick and rule, and each
  row returns its height so content flows instead of relying on fixed offsets.
- **Key/value rows are right-aligned** with a dimmed label, falling back to inline
  placement when the window is too narrow to reserve a value column.
- **Grouped rows render inside cards** with a blue accent edge, giving visual
  hierarchy without extra chrome.
- **Added interactive toggles to four more pages.** Devices gained a Bluetooth
  switch, Network a Wi-Fi switch, and Accessibility switches for High contrast and
  Large text. All seven toggles across the app are now real switches with a
  sliding knob, coloured state text, and full-row click targets.
- **Some values are now live rather than hardcoded.** Devices reports the actual
  mouse presence, and Network/Gaming reflect the toggle and framebuffer state.
- **GamerOS Update is now a segmented control** (Changelog / Roadmap) above a
  bordered viewer body with a pinned scroll hint, replacing the mismatched pair of
  standalone buttons.
- **Removed draw/hit-test geometry drift.** `draw_window()` publishes the exact
  rectangles it rendered — toggles, resolution rows, and the view switch — and the
  click handler reads those, rather than repeating layout constants that previously
  drifted out of sync. Scroll extent for the changelog viewer is published the same
  way, replacing a second, independently-computed viewport size.
- **Added `tools/embed_changelog.py`** so the in-OS changelog is generated from
  `docs/changelogs/` instead of being a hand-maintained copy that drifts. It
  refuses to embed content that would exceed the filesystem's file-size limit and
  warns when the viewer's line budget would be exceeded.

## Documentation

- **Fixed broken documentation links.** `changelog_2026-xx-xx.md`,
  `walkthrough_src_bugfixes_2026-xx-xx.md`, and `currentbugs.md` were all linked
  from the README but do not exist.
- **Fixed the documented project layout**, which described a `src/apps` /
  `src/impl` / `src/intf` structure that no longer exists.
- **Removed a duplicated "Advanced Options" section** in the README.
- **Corrected batch script paths** in the README to match their actual location.

## Notes
- The executive layer and user-mode subsystems remain **not wired into**
  `kernel_main`. They are now correct, consistent, and linkable, but still
  inactive. Enabling them is a deliberate follow-up decision.
- This changelog is the single source of truth. `tools/embed_changelog.py`
  generates the copy compiled into the in-OS viewer, so the two can no longer
  drift apart. Re-run it after editing this file.
- The generated ISO is compile-verified (zero warnings, zero errors, zero
  undefined symbols) but **not yet booted under QEMU**.

## Planned
- Boot the generated ISO under QEMU and confirm it reaches the desktop shell.
- Wire `executive_init()` and `user_mode_init()` into `kernel_main` now that both
  layers are linkable, and remove the linked-out `GOSAPP` shims.
- Raise `MAX_FILE_SIZE` in `fs.h`; the changelog viewer is close to its ceiling.
- Persist Settings state so toggles survive a reboot instead of resetting.
- Replace the polled input loop with PIT/HPET timer-driven frame pacing.
- Complete the USB driver stack and implement ATA PIO properly; the driver is
  currently disabled at boot and the filesystem runs on the RAM disk.