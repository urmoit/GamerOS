# GamerOS Changelog (TBD)

This changelog tracks work for the next release cycle.

## Release
- Version: `00m1`
- Build: `1.400`
- Date: `TBD`

## Added
- True-color framebuffer path added (24/32bpp linear framebuffer support).
- New graphics API helpers for RGB drawing:
  - `draw_pixel_rgb(...)`
  - `clear_screen_rgb(...)`
- Added explicit video mode enum target `MODE_VESA_1920x1080`.

## Changed
- Build metadata updated from `1.300` to `1.400`.
- Desktop bottom accent above taskbar changed from dual blue glow bands to a subtle neutral shadow for cleaner wallpaper blending.
- Startup/loading screen fully redesigned with modern card layout, rounded/chamfered corners, and updated progress rail treatment.
- Branding text updated to remove Windows-style wording in favor of GamerOS-native naming.
- Core shell UI modernized to match loading screen direction:
  - window chrome and controls updated with rounded/chamfered surfaces
  - Start menu restyled with modern panel/button treatment
  - taskbar, task buttons, and clock updated to modernized geometry
  - Settings theme label updated from legacy XP naming to GamerOS Modern
- Boot flow now prefers bootloader-provided framebuffer and keeps that mode into kernel startup.
- Graphics backend now safely activates true-color mode whenever a valid bootloader framebuffer is provided.
- Graphics pipeline compatibility pass completed so legacy UI color IDs and cursor composition behave correctly on both indexed VGA and true-color framebuffer paths.
- Boot header now explicitly requests `1920x1080x32` graphics mode again for Full HD true-color startup when supported.
- Settings `Gaming`/`About` now display live graphics backend mode, bpp, and resolution (including RGBA indicator).
- Start menu layout refreshed for clarity with cleaner cards, search strip, and stronger action hierarchy.
- Loading screen background reworked to a smoother gradient-style presentation.
- Loading indicator switched from linear bar to a circular progress loader.
- Completed full-shell modern visual pass with a bright unified style:
  - startup card and spinner recolored for a clean contemporary boot look
  - Start menu restyled with profile chip, light surfaces, and neutral shutdown action
  - taskbar, Start button, task buttons, and clock moved to light modern treatment
  - desktop icon labels and watermark tuned for better readability on bright backgrounds

## Fixed
- Fixed wallpaper/taskbar transition mismatch caused by the two blue desktop glow strips.
- Fixed stale About build text (now shows `1.400`).
- Fixed hard lock to legacy 16-color planar rendering path by adding true-color framebuffer output path when available.
- Fixed VMware startup crash path by hardening multiboot framebuffer validation and forcing VGA fallback when framebuffer memory/type/range is unsafe.
- Fixed true-color color mapping bug where non-`0x0F` palette IDs were truncated.
- Fixed cursor background restore artifacts on true-color backend by switching cursor underlay capture/restore to RGB-aware path.
- Fixed boot-time triple-fault by relocating early page tables away from kernel image/BSS memory range.
- Fixed early paging initialization robustness by allocating P4/P3/P2 tables as page-aligned kernel BSS symbols instead of fixed physical addresses.
- Fixed ultra-dark desktop/UI rendering in true-color mode by applying shell palette mappings during non-VGA palette initialization.
- Fixed near-black desktop visibility when wallpaper asset is too dark by adding adaptive bright modern wallpaper fallback rendering.

## Notes
- Release date is not finalized yet.
