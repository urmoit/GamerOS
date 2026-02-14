# Walkthrough - `src` Bug Fix Pass (2026-02-15)

This document now tracks only the latest additions from the 2026-02-15 update.

## Fixes Applied

### 1. Added storage-backed filesystem integration for desktop shell

Files:
- `src/intf/fs.h`
- `src/impl/filesystem/fs.c`
- `Makefile`
- `src/impl/kernel/main.c`

Changes:
- Expanded FS limits and path support (`MAX_FILES`, `MAX_FILENAME_LEN`).
- Added basic directory support and existence checks:
  - `fs_create_directory(...)`
  - `fs_directory_exists(...)`
- Added file metadata/list helpers:
  - `fs_file_exists(...)`
  - `fs_get_file_size(...)`
  - `fs_list_entries(...)`
- Wired `fs.o` into build/link graph in `Makefile`.
- Initialized storage layout at boot with Windows-style paths:
  - `C:/Windows`
  - `C:/Users/Admin`
  - `C:/Apps/System`
- Seeded baseline system/app files under storage paths.

Reason:
- Requested “real filesystem with storage usage” and Windows-like folder layout for apps/system files.

### 2. Added File Explorer app and upgraded Settings to dedicated configurable app

File:
- `src/impl/kernel/main.c`

Changes:
- Added `WIN_EXPLORER` window type with desktop and Start menu launch entries.
- Implemented in-window explorer navigation:
  - Current path bar
  - Parent navigation (`[..]`)
  - Directory/file listing from `fs_list_entries(...)`
  - Folder click-to-open behavior
- Added dedicated Settings app behavior (not tied to Notepad):
  - Runtime toggle controls:
    - Taskbar compact mode
    - Desktop glow
    - Clock seconds display
  - Live UI updates based on selected settings.
- Updated taskbar width/time rendering paths to consume Settings state.

Reason:
- Requested full Settings app and a File Explorer backed by storage/folder structure.

### 3. Reduced filesystem memory footprint for VMware runtime stability

Files:
- `src/intf/fs.h`
- `src/impl/filesystem/fs.c`

Changes:
- Reduced filesystem table/path sizes:
  - `MAX_FILES`: `64` -> `32`
  - `MAX_FILENAME_LEN`: `48` -> `40`
- Reduced simulated disk buffer size:
  - `DISK_SECTOR_SIZE`: `512` -> `256`
  - `MAX_DISK_SECTORS`: `1024` -> `128`

Reason:
- Lower static memory pressure in early boot/runtime to mitigate VMware virtual CPU shutdown faults after filesystem/explorer integration.

### 4. Reworked app launch layout and converted Settings to tabbed UI

File:
- `src/impl/kernel/main.c`

Changes:
- Removed Notepad and About from desktop/start-menu launch surface.
- Moved About information into Settings (new `About` tab).
- Converted Settings window to a tabbed layout with left navigation:
  - `System`
  - `Personalization`
  - `Accounts`
  - `About`
- Wired tab click handling in Settings.
- Kept runtime system toggles under `System` tab:
  - Taskbar compact mode
  - Desktop glow
  - Clock seconds
- Updated desktop icons to:
  - `Settings`
  - `Explorer`
- Updated Start menu items to:
  - `Settings`
  - `File Explorer`
  - `Shut Down`

Reason:
- Requested removal of standalone Notepad/About apps and a Windows-11-style tabbed Settings experience.

### 5. Restored full Notepad app and migrated storage namespace to `GamerOS`

Files:
- `src/impl/kernel/main.c`
- `src/intf/fs.h`
- `src/impl/filesystem/fs.c`

Changes:
- Restored Notepad as a first-class app in desktop and Start menu.
- Upgraded Notepad behavior with storage-backed persistence:
  - Loads from `C:/Users/Admin/NOTEPAD.TXT` on open
  - Saves to disk with `PgUp`
  - Reloads from disk with `PgDn`
  - Auto-saves when closing if modified
  - Added status line showing save state
- Renamed filesystem OS folder path from `Windows` to `GamerOS`:
  - `C:/GamerOS`
  - `C:/GamerOS/System32`
- Moved app/system registration files under `C:/GamerOS/System32`:
  - `NOTEPAD.APP`
  - `SETTINGS.APP`
  - `EXPLORER.APP`
  - `KERNEL.SYS`
  - `SHELL32.DLL`
- Added simulated multi-device storage capability metadata in FS layer:
  - HDD, SSD, NVMe, USB, CD-ROM, RAM disk
  - Exposed via `fs_storage_get_device_count()` and `fs_storage_get_device(...)`

Reason:
- Requested Notepad restoration, `GamerOS` naming in explorer/storage paths, and broader disk/storage support modeling.

### 6. Hardened Settings open/click path for VMware stability

Files:
- `src/impl/kernel/main.c`
- `src/impl/graphics/vga_graphics.c`

Changes:
- Fixed repeated mouse click-trigger execution by ensuring `last_buttons` is updated before all early returns in `process_mouse(...)`.
- Added open-window position/size clamping in `open_window(...)` to keep windows within visible desktop bounds.
- Fixed `draw_rect(...)` left-border write guard to validate x upper bound and prevent out-of-bounds backbuffer writes.

Reason:
- Addressed VMware virtual CPU shutdown fault observed when opening Settings after recent UI/storage changes.

### 7. Added secondary Settings-render fail-safe guards

Files:
- `src/impl/kernel/main.c`

Changes:
- Added strict per-window geometry clamping at draw time before rendering.
- Prevented rendering when a window resolves to invalid dimensions after clamp.
- Simplified Settings system-tab runtime rendering path to avoid per-frame device-struct expansion and use a safer summarized storage display.

Reason:
- Follow-up hardening for persistent VMware CPU-disabled popup when opening Settings.

### 8. Added Windows-style "This PC" drive view in File Explorer

Files:
- `src/impl/kernel/main.c`

Changes:
- Added `This PC` mode for Explorer root view.
- Explorer now lists drives (`C:`, `D:`, `E:`...) with device labels/types.
- Clicking a drive opens that drive root path.
- Added `[This PC]` navigation entry when browsing inside a drive/folder.
- Initialized drive-root directories from detected storage profile count during storage layout setup.

Reason:
- Requested Windows-like File Explorer behavior with a drive-centric "This PC" entry point.

### 9. Hardened Explorer open path to avoid VMware CPU-disabled popup

File:
- `src/impl/kernel/main.c`

Changes:
- Removed runtime storage enumeration calls from Explorer draw/click paths.
- Added cached `This PC` drive entries (`explorer_build_this_pc_entries()`), built at initialization/open.
- Explorer now renders and navigates using cached drive labels/paths only.

Reason:
- Mitigate persistent VMware crash popup when opening File Explorer after recent storage/drive UI updates.

### 10. Moved storage initialization out of early boot (lazy init)

File:
- `src/impl/kernel/main.c`

Changes:
- Removed `initialize_storage_layout()` from startup path in `kernel_main()`.
- Added lazy storage initialization (`ensure_storage_initialized()`) triggered when opening storage-dependent apps (`Notepad`, `File Explorer`).
- Added safe default Explorer drive label/path at boot (`C: Local Disk`) so desktop can render without storage subsystem initialization.

Reason:
- System was failing before desktop startup with VMware CPU-disabled popup; this isolates heavy storage setup from boot and restores startup stability.

### 11. Added Explorer-safe open mode to stop VMware crash on launch

File:
- `src/impl/kernel/main.c`

Changes:
- Removed `ensure_storage_initialized()` from Explorer window open path.
- Added a guarded click behavior in `handle_explorer_click(...)`:
  - if storage is not initialized, Explorer stays in stable `This PC` mode and does not enter drive-root FS traversal.

Reason:
- Prevent CPU-disabled VMware popup when opening/clicking File Explorer while storage init path is still being stabilized.

### 12. Emergency Explorer safe-mode rollback for startup/runtime stability

File:
- `src/impl/kernel/main.c`

Changes:
- Replaced dynamic Explorer rendering with a static safe-mode panel (`This PC` placeholder view).
- Disabled Explorer click actions in safe mode to avoid triggering unstable runtime paths.
- Kept Explorer window launchable and visible while isolating VMware crash source.

Reason:
- Explorer path still triggered CPU-disabled popup; this rollback keeps system usable while fault isolation continues.

### 13. Emergency Notepad safe-mode rollback for open-path stability

File:
- `src/impl/kernel/main.c`

Changes:
- Removed storage initialization from Notepad open path.
- Notepad now opens in in-memory safe mode (`notepad_clear()` on open).
- Guarded Notepad storage actions so save/load/autosave run only when storage is initialized.

Reason:
- Notepad open path still triggered VMware CPU-disabled popup; this keeps Notepad launch stable while storage init path is being debugged.

### 14. Increased usable desktop workspace and app window sizes

File:
- `src/impl/kernel/main.c`

Changes:
- Reduced taskbar height (`20` -> `16`) to free more vertical desktop space.
- Updated default Notepad/Settings/Explorer open geometry to near-maximized sizes based on current screen dimensions.
- Added default top-left positioning for larger windows so they open fully visible.

Reason:
- Requested better graphics usability so app windows can be larger within the current stable video mode.

### 15. Added higher-resolution VGA runtime path (640x480)

Files:
- `src/impl/x86_64/boot.asm`
- `src/impl/graphics/vga_graphics.c`

Changes:
- Boot stage now sets VGA mode 12h (640x480x16) before entering long mode.
- Graphics backend defaults switched to 640x480 runtime dimensions.
- Implemented planar VGA mode-12h present path in `swap_buffers()`:
  - Converts 8-bit backbuffer pixels to 4-bit planar output
  - Writes all 4 VGA planes via sequencer map-mask registers.
- Updated long-mode mode selection path to use pre-set 640x480 runtime state without BIOS interrupts.

Reason:
- Requested better resolution support so the UI is no longer constrained to very small 320x200 rendering.

### 16. Fixed mode mismatch by aligning boot and kernel to VGA mode 12h

Files:
- `src/impl/x86_64/boot.asm`
- `src/impl/kernel/main.c`

Changes:
- Boot path switched back to `set_vga_mode12h` before long-mode transition.
- Kernel startup now explicitly requests 640x480 runtime mode (`set_video_mode(MODE_VESA_640x480)`), matching the mode-12h planar present path.

Reason:
- Black screen occurred when boot mode and runtime present path were out of sync. This re-aligns the graphics pipeline so mode 12h can render correctly.

### 17. Added explicit VGA planar-write register programming for mode 12h

File:
- `src/impl/graphics/vga_graphics.c`

Changes:
- Added `vga_prepare_mode12_planar_writes()` to program sequencer/graphics-controller registers required for planar writes:
  - Sequencer memory mode (`0x3C4/0x3C5`)
  - Graphics controller write/mapping/bitmask (`0x3CE/0x3CF`)
- Called planar-write setup during mode selection and before frame present.
- Restored sequencer map-mask to all planes after swap.

Reason:
- Continue full mode-12h support; enforce correct VGA planar write state every frame to address persistent black output.

### 18. Fixed mode-12h UI stripe artifacts with strict per-byte planar present

File:
- `src/impl/graphics/vga_graphics.c`

Changes:
- Expanded mode-12h graphics-controller setup to deterministic write-mode state (set/reset, rotate, read-map, misc, bitmask).
- Replaced plane present `memcpy` with explicit byte-by-byte writes to VGA memory per plane while map-mask is selected.

Reason:
- Removed vertical stripe corruption caused by unstable/non-deterministic planar present behavior in 640x480 mode 12h.

### 19. Tightened per-plane VGA state and volatile VRAM writes for mode 12h

File:
- `src/impl/graphics/vga_graphics.c`

Changes:
- Switched mode-12h VRAM target pointer to `volatile uint8_t*` for strict byte write semantics.
- Added per-plane GC setup inside present loop:
  - Read-map select (`GC index 0x04`)
  - Bitmask reassert (`GC index 0x08 = 0xFF`)
- Kept explicit map-mask selection per plane before writes.

Reason:
- Further hardening for persistent 1-pixel vertical stripe artifacts in VMware mode-12h rendering.

### 20. Implemented full runtime VGA mode-12h register programming

File:
- `src/impl/graphics/vga_graphics.c`

Changes:
- Added full in-kernel mode setup routine `vga_set_mode_12h_runtime()`:
  - Programs Misc output register.
  - Programs full Sequencer register set with reset/unreset flow.
  - Unprotects and writes full 25-entry CRTC timing table for 640x480.
  - Programs full Graphics Controller register set for planar A000 mode.
  - Programs Attribute Controller palette/control registers and reenables display.
  - Clears visible VRAM after mode switch.
- Updated runtime mode entry points to call full setup:
  - `vga_set_mode_13h()` now performs mode-12h runtime programming.
  - `vesa_set_mode(0x101)` now performs mode-12h runtime programming.

Reason:
- Previous mode-12h path only partially configured runtime VGA state; that can leave register state inconsistent and produce persistent vertical stripe artifacts on VMware.

### 21. Added Settings changelog tab, generated release changelog file, and bumped build to 1.200

Files:
- `src/impl/kernel/main.c`
- `changelog_2026-02-15.md`

Changes:
- Added new `Changelog` tab to Settings navigation.
- Added changelog content panel in Settings with latest update highlights.
- Increased Settings tab count and click handling to include the new tab.
- Updated build metadata strings from `1.100` to `1.200`:
  - Settings About tab
  - `C:/GamerOS/GAMEROS.INI` content generated at storage init
- Updated About graphics descriptor to current runtime mode (`VGA 640x480x16`).
- Added repository release document: `changelog_2026-02-15.md` summarizing post-alpha changes.

Reason:
- Requested in-OS changelog visibility, a standalone 2026-02-15 changelog markdown file, and build bump to `1.200`.

### 22. Improved Settings layout and added mouse-wheel scrolling for Settings/Notepad

Files:
- `src/impl/kernel/main.c`
- `src/intf/mouse.h`
- `src/impl/drivers/mouse.c`

Changes:
- Reworked Settings left navigation into a clearer panel/button layout with a header.
- Added text clipping for Settings content (`draw_compact_string_clipped(...)`) so long lines stay inside content bounds.
- Added wheel-scrollable changelog view in Settings (`Changelog` tab) with internal scroll state.
- Added PS/2 mouse wheel delta support in driver:
  - IntelliMouse wheel enable/init sequence
  - 4-byte packet parsing path
  - `mouse_get_wheel_delta()` API
- Integrated wheel input into shell mouse processing (`process_mouse(..., wheel_delta)`).
- Added Notepad wheel scrolling and viewport tracking (`notepad_view_top`).
- Expanded Notepad buffer for practical scrolling:
  - `NOTEPAD_MAX_LINES`: `20` -> `160`
  - `NOTEPAD_MAX_COLS`: `40` -> `96`
- Added cursor viewport auto-follow during keyboard editing.

Reason:
- Requested improved Settings tab layout, prevention of content overflow into window frame, and mouse-wheel scrolling support in both Settings and Notepad.

### 23. Centered taskbar text/buttons and synchronized taskbar hitboxes

File:
- `src/impl/kernel/main.c`

Changes:
- Reworked taskbar draw path to compute shared vertical geometry for:
  - taskbar baseline
  - Start button
  - task buttons
  - clock panel
- Centered Start label text inside the Start button using compact-font width metrics.
- Centered task button titles vertically/horizontally using computed button dimensions.
- Added a dedicated right-side clock box and centered clock text for both `HH:MM` and `HH:MM:SS` modes.
- Added taskbar overflow guard so task buttons stop before entering the clock region.
- Updated mouse hit-testing to match the new centered taskbar geometry:
  - Start button click bounds
  - task button click bounds
  - taskbar clickable region respects reserved clock area
- Removed dependence on fixed `+3/+4/+6/+7` taskbar text/click offsets in favor of geometry-derived placement.

Reason:
- Requested immediate visual cleanup for off-center taskbar text/buttons and matching interaction alignment.

### 24. Smoothed cursor movement, fixed cursor trail artifacts, and recentered startup screen

Files:
- `src/impl/kernel/main.c`
- `src/impl/graphics/vga_graphics.c`
- `src/intf/graphics.h`
- `src/impl/drivers/mouse.c`

Changes:
- Added partial present API (`present_rect(...)`) to update only changed screen regions instead of always presenting a full frame.
- Added cursor overlay composition path in shell loop:
  - save background under cursor
  - restore previous cursor region
  - draw cursor at new position
  - present only union of old/new cursor rects on move-only frames
- Kept full redraw path for state-changing events (clicks, drag, wheel, keyboard).
- Fixed mode-12h partial present byte handling so each affected VGA byte is rebuilt fully from backbuffer.
  - This removes black pixel trails/artifacts during cursor movement.
- Added mouse packet delta clamp (`-12..12`) to reduce sudden pointer jumps from spiky packets.
- Reworked startup animation layout for 640x480:
  - centered title/subtitle based on current resolution
  - larger centered progress bar
  - progress fill size derived from runtime width
- Updated in-OS Settings `Changelog` tab content with:
  - taskbar centering/hitbox sync
  - cursor smooth path
  - cursor black-trail artifact fix

Reason:
- Requested less laggy/non-teleporting cursor behavior, removal of new black cursor artifacts, and a larger centered loading screen for current resolution.

### 25. Fixed inverted mouse-wheel scroll direction

File:
- `src/impl/kernel/main.c`

Changes:
- Normalized wheel input sign at shell input boundary:
  - `wheel_delta = -mouse_get_wheel_delta()`
- This applies consistently to existing wheel consumers:
  - Notepad viewport scrolling
  - Settings changelog scrolling

Reason:
- Requested fix for inverted wheel behavior where scroll direction felt backwards in OS UI.

### 26. Sped up startup loading animation

File:
- `src/impl/kernel/main.c`

Changes:
- Reduced startup progress animation work in `startup_animation()`:
  - Progress bar now advances in larger steps instead of 1-pixel increments.
  - Per-frame delay loop reduced (`12000` -> `2500`).
- Kept the visual progress effect while cutting total startup animation time.

Reason:
- Requested faster loading screen because startup animation felt too slow.

### 27. Added resizable windows with mouse drag handle

File:
- `src/impl/kernel/main.c`

Changes:
- Added bottom-right resize grip rendering for desktop windows.
- Added left-click resize interaction from window corner:
  - Press on grip to start resize mode.
  - Drag to resize width/height in real time.
  - Release mouse to end resize mode.
- Added per-window min size constraints and desktop/taskbar max bounds.
- Kept existing drag-to-move behavior for title bar unchanged.

Reason:
- Requested resizable windows in desktop shell UI.

### 28. Added GamerOS desktop watermark text

File:
- `src/impl/kernel/main.c`

Changes:
- Added desktop watermark rendering helper (`draw_desktop_watermark()`).
- Displays two right-aligned lines above the taskbar:
  - `GamerOS 00m1 Preview`
  - `Evaluation copy. Build 1.200`
- Integrated watermark draw call into desktop render flow so it appears consistently on desktop.

Reason:
- Requested Windows-preview-style informational text, but branded for GamerOS.

