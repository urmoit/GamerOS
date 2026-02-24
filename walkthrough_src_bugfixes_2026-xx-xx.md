# Walkthrough - `src` Bug Fix Pass (TBD)

This document tracks the latest changes for build `1.400`.

## Fixes Applied

### 1. Initialized build 1.400 walkthrough

Files:
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Created a new walkthrough file for the next update cycle.
- Set baseline note for build `1.400`.
- Marked release date as `TBD` until finalized.

Reason:
- Start tracking new implementation and bug-fix work for the `1.400` cycle.

### 2. Fixed wallpaper-to-taskbar transition styling

Files:
- `src/impl/kernel/main.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Replaced the two blue desktop glow bands above taskbar with a subtle neutral shadow strip.
- Kept wallpaper fully visible without mismatched blue overlays.
- Updated build `1.400` changelog with matching changed/fixed notes.

Reason:
- Requested removal of the two blue color bands that did not fit with the wallpaper look.

### 3. Modernized loading screen and removed Windows-style branding text

Files:
- `src/impl/kernel/main.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Updated startup subtitle copy to GamerOS-native wording.
- Refreshed startup animation color treatment to a more modern layered look while keeping centered panel/progress behavior.
- Updated desktop watermark copy to remove legacy platform-style naming and use GamerOS modern shell branding.
- Added matching release notes entries in the build `1.400` changelog.

Reason:
- Requested modern loading menu visuals and replacement of legacy platform-style naming without removing the affected text lines.

### 4. Redesigned loading screen and core shell UI to modern visual system

Files:
- `src/impl/kernel/main.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Added shared chamfered/rounded-style drawing helpers for consistent modern geometry.
- Reworked startup screen from retro framed bars into a modern centered card layout:
  - updated subtitle copy
  - refined layered background
  - redesigned rounded/chamfered progress rail and highlight sweep
- Modernized main shell surfaces to match startup design:
  - window chrome/body panels updated to rounded/chamfered treatment
  - close button and content surfaces restyled
  - Start menu panel/header/items/shutdown button restyled
  - taskbar strip, Start button, task buttons, and clock restyled
- Updated in-Settings text from `Theme: XP Classic` to `Theme: GamerOS Modern`.
- Updated About page build string from `1.300` to `1.400`.

Reason:
- Requested full loading-screen redesign with rounded modern visuals and matching modernization across the rest of the UI.

### 5. Added true-color framebuffer path and Full HD boot-mode request

Files:
- `src/intf/graphics.h`
- `src/impl/graphics/vga_graphics.c`
- `src/impl/kernel/main.c`
- `src/impl/x86_64/boot.asm`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Added multiboot framebuffer activation path in graphics backend for 24/32bpp linear framebuffer rendering.
- Expanded internal render buffers to support up to `1920x1080` and maintain compatibility with indexed-color callers.
- Added RGB drawing entry points:
  - `draw_pixel_rgb(...)`
  - `clear_screen_rgb(...)`
- Updated kernel startup to prefer multiboot framebuffer mode and only fallback to legacy VGA path when unavailable.
- Kept legacy VGA startup fallback for VM compatibility while preserving safe true-color activation when a valid framebuffer is present.
- Kept existing planar VGA renderer as fallback for environments without framebuffer support.

Reason:
- Requested full-color support with modern high resolution capability (up to Full HD 1080p).

### 6. Hardened framebuffer safety checks to prevent VMware boot crash

Files:
- `src/impl/graphics/vga_graphics.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Added strict multiboot framebuffer validation before enabling true-color path:
  - RGB framebuffer type required
  - width/height/bpp/pitch sanity checks
  - pitch lower-bound checks against resolution and bytes-per-pixel
  - address range guard to ensure framebuffer stays inside currently mapped first 4GB physical range
- If any validation fails, kernel now falls back to legacy VGA rendering path instead of using unsafe framebuffer state.
- Added matching fix note to build `1.400` changelog.

Reason:
- User reported VMware virtual CPU shutdown popup at boot after framebuffer updates; safety guards were required to avoid triple-fault style crashes on unsupported/unsafe framebuffer mappings.

### 7. Completed cross-backend graphics compatibility for whole shell

Files:
- `src/intf/graphics.h`
- `src/impl/graphics/vga_graphics.c`
- `src/impl/kernel/main.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Added graphics backend query helpers:
  - `graphics_is_truecolor()`
  - `graphics_get_pixel_rgb(...)`
- Reworked software color mapping to use full 256-entry palette conversion instead of low-4-bit truncation.
- Synced software palette map with runtime palette setup, including custom UI indices used by shell icons/theme colors.
- Updated cursor background capture/restore to RGB-aware handling when true-color backend is active.
- Re-enabled safe framebuffer auto-use in kernel startup with existing validation and VGA fallback guard.

Reason:
- Requested that the whole OS support the new graphics path consistently, not just partial rendering paths.

### 8. Fixed early paging-table overlap causing VMware triple-fault

Files:
- `src/impl/x86_64/boot.asm`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Moved early boot page-table physical addresses from low memory near kernel load region to high safe addresses:
  - `P4_TABLE` -> `0x4000000`
  - `P3_TABLE` -> `0x4001000`
  - `P2_TABLE` -> `0x4002000`
- Preserved identity-mapping logic and huge-page setup behavior.

Reason:
- VMware log showed guest triple-fault; with enlarged kernel BSS for new graphics buffers, previous fixed page-table addresses could overlap kernel image/BSS and corrupt paging structures during boot.

### 9. Reworked early page-table allocation to BSS-backed aligned symbols

Files:
- `src/impl/x86_64/boot.asm`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Replaced fixed-address boot page table pointers with page-aligned symbols in kernel BSS:
  - `p4_table`
  - `p3_table`
  - `p2_tables`
- Updated all paging init writes and CR3 load path to use those symbol addresses.
- Preserved identity-map size and huge-page mapping behavior.

Reason:
- VMware log still showed triple-fault; symbol-backed table allocation avoids fixed physical-address assumptions and prevents early boot paging writes into unexpected regions.

### 10. Enforced Full HD true-color request and added runtime graphics verification UI

Files:
- `src/impl/x86_64/boot.asm`
- `src/intf/graphics.h`
- `src/impl/graphics/vga_graphics.c`
- `src/impl/kernel/main.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Re-enabled multiboot video request fields for `1920x1080x32` (`VIDEO_MODE` flag).
- Added `graphics_get_bpp()` API for runtime graphics capability reporting.
- Updated Settings `Gaming` and `About` pages to show live mode details:
  - backend type (VESA true-color vs VGA indexed)
  - active bpp
  - active resolution
  - RGBA marker when true-color path is active
- Added boot-time serial log line reporting final graphics mode in use.

Reason:
- Requested confirmation that the OS runs on true-color/RGBA and 1080p where supported.

### 11. Fixed dark UI regression in true-color path

Files:
- `src/impl/graphics/vga_graphics.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Added shared shell palette application during palette bootstrap (not only VGA DAC init path).
- Ensured first 16 UI colors and custom shell palette entries are initialized for true-color mapping.
- Kept VGA DAC programming aligned with software palette mapping for consistency across both backends.

Reason:
- User reported near-black desktop/UI after true-color enablement; root cause was grayscale fallback palette being used before shell palette entries were applied.

### 12. Upgraded Start menu and loading screen presentation

Files:
- `src/impl/kernel/main.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Reworked Start menu styling for readability:
  - cleaner light panel
  - improved header/search strip
  - clearer app-row cards and shutdown action emphasis
- Rebuilt loading screen background to a smoother gradient-style visual treatment.
- Replaced old linear loading rail with circular progress loader animation.
- Updated release notes entries for these UI changes.

Reason:
- Requested better Start menu design and smoother loading screen visuals with a circular loading indicator.

### 13. Applied full-shell modern light theme and dark-wallpaper fallback

Files:
- `src/impl/kernel/main.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Finished a full visual consistency pass across shell surfaces with a brighter modern look:
  - startup screen switched to bright gradient + light card styling
  - circular loader recolored for clearer progression and contrast
  - Start menu rebuilt with light surfaces, profile chip, cleaner app rows, and neutral shutdown treatment
  - taskbar, Start button, task buttons, and clock updated to light modern palette
  - desktop watermark/icon-label colors adjusted for readability
- Added adaptive wallpaper safety path:
  - detects when embedded wallpaper asset is effectively too dark
  - uses a bright modern procedural backdrop fallback to keep desktop/apps visible
  - preserves wallpaper sampling when source brightness is acceptable
- Synced release notes for this style/visibility pass.

Reason:
- Requested a full modernized shell style and reported that the desktop became too dark to use.
