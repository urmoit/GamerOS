# GamerOS Changelog (2026-02-15)

This changelog covers all major work completed after the first alpha changelog request.

## Release
- Version: `00m1`
- Build: `1.200`
- Date: `2026-02-15`

## Added
- Storage-backed filesystem integration in shell and runtime.
- Directory support and listing helpers in filesystem layer.
- File Explorer app with Windows-style `This PC` drive view.
- Settings app as dedicated tabbed app (System, Personalization, Accounts, About, Changelog).
- About information moved into Settings.
- Notepad restored as full app with keyboard editing.
- Mouse wheel input API for shell/UI (`mouse_get_wheel_delta()`).
- Notepad storage actions:
  - Load from `C:/Users/Admin/NOTEPAD.TXT`
  - Save with `PgUp`
  - Reload with `PgDn`
  - Auto-save on close when modified
- GamerOS storage namespace:
  - `C:/GamerOS`
  - `C:/GamerOS/System32`
  - App/system entries in System32 (`NOTEPAD.APP`, `SETTINGS.APP`, `EXPLORER.APP`, `KERNEL.SYS`, `SHELL32.DLL`)

## Changed
- Build metadata updated from `1.100` to `1.200`.
- Settings About panel updated to reflect new build and graphics mode.
- Runtime graphics path aligned to VGA mode `12h` (640x480x16 planar).
- Desktop/window sizing adjusted for larger workspace in 640x480 mode.
- Startup/storage flow switched to safer lazy-init patterns for stability.
- Settings layout refreshed with clearer left navigation panel and tab-button styling.
- Settings content rendering now clips long lines to content width.
- Notepad document capacity increased for usable long-form editing:
  - `NOTEPAD_MAX_LINES`: `20 -> 160`
  - `NOTEPAD_MAX_COLS`: `40 -> 96`
- Notepad now uses viewport-based rendering with cursor auto-follow.
- Taskbar layout now uses shared centered geometry for Start button, task buttons, and clock panel.
- Start label, task labels, and clock text are centered from font metrics instead of fixed pixel offsets.
- Taskbar button layout now reserves clock space explicitly to prevent overlap.
- Shell render loop now uses cursor-only partial presents on move-only frames for smoother pointer motion.
- Mouse-wheel direction handling is normalized so UI scrolling matches expected direction.
- Startup/loading animation is now centered and scaled for the active 640x480 runtime layout.
- Startup/loading animation runtime was shortened by reducing progress-frame count and per-frame delay.
- Desktop windows now support mouse resizing from a bottom-right grip with min/max bounds.
- Added GamerOS-branded desktop evaluation watermark text near the lower-right desktop area.

## Fixed
- VMware popup/crash hardening across Settings, Explorer, and Notepad open/click paths.
- Multiple window/input safety issues:
  - click-edge state handling
  - geometry clamping
  - safer rendering bounds checks
- Black-screen mode mismatch between boot mode and runtime renderer.
- Planar present instability in mode `12h` with multiple hardening passes:
  - explicit VGA register preparation for planar writes
  - deterministic per-plane byte writes
  - volatile VRAM writes and tighter per-plane register setup
  - full runtime mode-12h register programming (SEQ/CRTC/GC/AC)
- Missing mouse-wheel scrolling in core apps:
  - Settings `Changelog` tab now scrolls with wheel.
  - Notepad content now scrolls with wheel.
- Settings text overflow into window/frame area resolved by clipped content drawing.
- Off-center taskbar text/button rendering in shell UI.
- Taskbar interaction mismatch where visual button position and click-hitbox position could drift.
- Cursor teleport-like jumpiness reduced via lighter mouse-move render path and per-packet delta clamping.
- Cursor black trail artifacts introduced by partial present byte masking in mode `12h`.

## Notes
- File Explorer currently runs with safety-first behavior in VMware-focused paths while deeper storage interactions are stabilized.
- Mode `12h` rendering has received full runtime programming support and multiple stripe-fix iterations.
