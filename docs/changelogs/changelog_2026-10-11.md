# GamerOS Changelog (2026-10-11)

> **WORK IN PROGRESS — DO NOT SHIP.** Build 1.550 is unfinished, and some of
> the entries below are only compile-verified. Treat this as a running log.

Build 1.550 takes the shell rendered by 1.520 and repairs how text is drawn, then
rebuilds the four always-visible surfaces — taskbar, Start menu, desktop watermark
and boot screen — on top of that fix.

## Release
- Version: `00m2`
- Build: `1.550` (in progress)
- Date: `2026-10-11`
- Status: **Unreleased.** Work in progress; still being verified.

## Critical — Triple Fault Opening the Start Menu

- **Fixed a triple fault that killed the guest the moment the Start button was
  clicked.** `boot.asm` enabled `CR4.PAE` but never set `CR4.OSFXSR` (bit 9) or
  `CR4.OSXMMEXCPT` (bit 10) before entering long mode. In IA-32e, executing an
  SSE instruction with `OSFXSR` clear raises `#UD`. Because the IDT is not
  populated for these faults, that `#UD` became `#GP` -> `#DF` -> **triple
  fault**, and the CPU shut down. This is what VMware was reporting as
  "a fault has occurred causing a virtual CPU to enter the shutdown state".
- **This was a latent landmine, not a Start-menu bug.** Any GCC-generated SSE
  opcode anywhere in the kernel would have killed it. `-O2` emits `pxor` +
  `movaps` for any initialisation wider than a 64-bit store, so the moment a
  struct crossed 16 bytes the kernel would die. Growing
  `start_menu_metrics_t` from 5 to 10 fields pushed it to 40 bytes and tipped GCC
  into emitting `pxor %xmm0,%xmm0` at the top of `draw_start_menu()` — which is
  why the crash appeared to belong to the Start menu.
- **Diagnosed from QEMU's exception trace**, not guessed: `-d int,cpu_reset` gave
  the sequence `#UD -> #GP(0x32) -> #DF -> Triple fault` at `RIP=0x109e00`, which
  `addr2line` resolved to `draw_start_menu()` at `main.c:2663`, and `info
  registers` showed `CR4=0x20` (PAE only, `OSFXSR` clear) against a `pxor`
  instruction that cannot fault for any other reason.
- **Verified fixed under QEMU 11.0.1:** opening the Start menu now raises **zero
  exceptions and zero triple faults**, and the VM stays `running`.

## Build Verification

- Bumped the build number from `1.520` to `1.550`, keeping version `00m2`.
  Updated in the `GAMEROS.INI` written at boot, the Settings About page, the
  desktop watermark, and `README.md`.
- Compile-verified: zero warnings, zero errors, zero undefined symbols.
- Booted under QEMU and confirmed the desktop shell renders, including the
  desktop icons, taskbar, and watermark.

## UI

- **Fixed the UI text renderer, which was shearing every letter.** `draw_compact_string()`
  sampled only the **left 6 of the 8 columns** of each glyph
  (`row_data & (0x80 >> col)` for `col` 0-5) while advancing the pen by only 6px.
  The right side of every glyph was discarded and the following glyph then
  overdrew 2px of it, so compact text rendered as vertical smears across the
  whole shell — most visibly on the desktop watermark. It now samples all 8
  columns and advances 8px, the font's natural width.
- **Converted all 43 hardcoded 6px text-width computations** to `FONT_CW`. Any
  site left at 6 would have overflowed its panel by a third once the renderer
  started advancing 8. The one non-text use of `/ 6` — the wallpaper gradient
  band height — was deliberately left alone.
- **Rebuilt the taskbar.** The Start button was 52px wide, but its label needs
  40px plus the flag gutter, so the text spilled past the button border. It is
  now wide enough, with the label centred in the space left by the flag. The task
  strip moved off a hardcoded `x = 56` to a centred run between the Start button
  and the clock, which removes the large dead gap on wide displays. Task labels
  are now clamped to the button width instead of a fixed 8 characters, and the
  Start button shows a pressed state while the menu is open.
- **Rebuilt the Start menu.** The profile rail was 42px wide, but "Admin" needs
  40px at the corrected advance, so the label bled into the content column. The
  rail is now 56px, the panel is wider, and the avatar plus both labels are
  centred on the rail's own centre line. "Launchpad" is placed after the brand
  word using a measured gap rather than a magic `x + 74`.
- **Centralised the Start menu layout.** `rail_w`, `panel_x`, `panel_w`,
  `items_top` and the item count were duplicated between the renderer and the
  click handler — the same duplication that previously let click targets drift
  away from the pixels they were drawn at. They now come from a single
  `get_start_menu_metrics()`. The taskbar strip got the same treatment via
  `taskbar_strip_start()`.
- **Rebuilt the desktop watermark.** The build badge is right-aligned on the
  lower row, and the card is sized from measured text instead of hand-padded
  spaces that no longer lined up once the advance changed.
- **Rebuilt the loading screen.** The title was centred on the *screen* while
  being drawn inside a much narrower panel, so it sat outside the panel on any
  wide display; it is now anchored to the panel's content column. The "Loading"
  label was drawn across the spinner ring at the same `y` and overwrote the dots;
  it now sits centred beneath the ring. The panel was also enlarged and centred
  rather than positioned from the title's `y`.

## Build and Tooling

- **Raised the changelog viewer's line budget from 256 to 320** in `main.c`, and
  moved the matching cap in `tools/embed_changelog.py` to the same value. The
  two were separate hardcoded literals, so raising only one would have left the
  generator warning about a truncation the viewer no longer had.

## Documentation

- **Split the changelog per release.** Build `1.520` (2026-10-10) stays in
  `changelog_2026-10-10.md`; this file records build `1.550` (2026-10-11).
  `tools/embed_changelog.py` was repointed at this file so the in-OS viewer keeps
  showing the newest release.

## Notes

- The 1.520 notes recorded that the ISO had never been booted. It has now: the
  desktop shell, taskbar, and watermark were confirmed rendering under QEMU.
- The font fix is global. Every compact string in the OS is 33% wider than it
  was, which is why the four surfaces above were rebuilt rather than patched.
- Start-menu and taskbar hit boxes are now derived from the same metrics the
  renderer uses, so a future layout change cannot silently break clicking.

## Planned
- Visually confirm the Start menu and loading screen under QEMU; the Start menu
  is now verified, and the desktop, taskbar, watermark and Start menu have all
  been captured as rendered frames.
- Persist Settings state so toggles survive a reboot instead of resetting.
- Replace the polled input loop with PIT/HPET timer-driven frame pacing.
- Complete the USB driver stack and implement ATA PIO properly; the driver is
  currently disabled at boot and the filesystem runs on the RAM disk.
