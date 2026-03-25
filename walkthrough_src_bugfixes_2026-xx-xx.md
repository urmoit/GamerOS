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

### 14. Added basic app lifecycle tracking and virtual storage paths

Files:
- `src/impl/kernel/main.c`
- `src/apps/apps.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Introduced a small runtime table that tracks each built-in app via its `app_descriptor_t` and maintains a simple lifecycle state (stopped/running/suspended).
- Updated the shell’s app launcher to mark apps as running when their window is opened and as stopped when their top-level window is closed.
- Added a GamerOS virtual storage path scheme (`GOS:/User`, `GOS:/System`, `GOS:/Apps`) with a helper that resolves these into concrete filesystem paths on C:.
- Switched Notepad’s document path to `GOS:/User/Notepad/NOTEPAD.TXT` and routed its load/save routines through the new GOS path helpers.
- Extended the storage layout with per-app user folders (Notepad, Settings, Explorer, Saves) and a `C:/GamerOS/Apps/Manifests` area for app metadata.
- Created initial `.gosapp` manifest files for Notepad, Settings, and Explorer describing IDs, EXE paths, entry symbols, categories, and basic permissions.
- Seeded a couple of placeholder save-slot files in the user `Saves` folder to support future game/launcher integrations.

Reason:
- Requested that apps feel more like real, first-class GamerOS applications with explicit lifecycle state and a cleaner, more future-proof storage model.

### 15. Improved Settings changelog markdown layout

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Updated the `GamerOS Update` tab in Settings to continue using the existing markdown parser but render section headings with the larger standard font instead of the compact text style.
- Kept list and body content on the compact font for density while giving `#` / `##` headings a clearer visual hierarchy.
- Confirmed that the in-Settings changelog text matches the main `changelog_2026-xx-xx.md` contents so both views stay in sync.

Reason:
- Requested that the Settings changelog view better respect markdown formatting and present the document with a clearer, more readable layout.

### 16. Enlarged window headers, taskbar, and app UI layouts

Files:
- `src/impl/kernel/main.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Increased the window title bar height and close button size to make app headers easier to see and click, while expanding the draggable caption area.
- Raised the global taskbar height and widened task buttons and the Start button so they feel less cramped at 320x200 and at higher framebuffer resolutions.
- Adjusted Notepad’s toolbar/status bar and text viewport positions to align with the taller header, preserving the readable text area.
- Refined the Settings panel layout with a slightly wider navigation rail and content area, plus updated spacing, so each tab’s information breathes more and reads more like a modern control center.
- Updated the Explorer window chrome, toolbar, and side/list panels to match the new window header proportions and provide larger hit targets for path controls and quick-access entries.

Reason:
- Requested that headers, taskbar elements, and Settings/app UI controls be larger and visually cleaner so the whole shell feels more like a modern desktop environment.

### 17. Enlarged Settings buttons and polished GamerOS Update split view

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Increased Settings left-nav width and row height so tab buttons are easier to click and tab labels are less cramped.
- Rebalanced Settings content/header spacing to keep a cleaner modern layout while preserving usable content area.
- Enlarged `GamerOS Update` top sub-buttons and synchronized click hitboxes with rendered geometry so button text fits and targets feel reliable.
- Kept markdown heading rendering with larger font and preserved split behavior:
  - `Build 1.400 changelog` shows release/add/changed/fixed/notes
  - `Roadmap` starts from `## Planned` onward
- Synced the same notes in both top-level changelog markdown and the in-Settings embedded changelog text.

Reason:
- Requested additional Settings UI improvements with bigger buttons, especially where button labels were too large for prior button sizes.

### 18. Split About into standalone app and added desktop right-click menu

Files:
- `src/impl/kernel/main.c`
- `src/apps/apps.c`
- `src/apps/about/ABOUT.EXE.manifest`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Removed `About` from Settings tabs and kept Settings focused on system controls/update views.
- Moved About rendering into its own top-level `About GamerOS` app window (`WIN_ABOUT`) with build/kernel/graphics/system info.
- Registered standalone `ABOUT.EXE` in app descriptors, system app list, and app manifests so it launches like other first-class apps.
- Updated Start menu `About GamerOS` action to open `ABOUT.EXE` directly.
- Added desktop right-click context menu with actions:
  - `About GamerOS`
  - `Settings`
  - `Refresh Desktop`
- Expanded `## Planned` roadmap entries in changelog so the Settings `Roadmap` sub-view shows the new planned items too.

Reason:
- Requested that About be its own app, removed from Settings, plus a desktop right-click options menu including About, and synced changelog/roadmap updates.

### 19. Hardened desktop right-click input handling for VMware stability

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Added defensive mouse coordinate clamping in `process_mouse(...)` before UI hit-testing.
- Masked mouse button state to supported left/right bits only for desktop shell input handling.
- Ignored ambiguous dual-button packets to avoid unstable right-click context-menu transitions in VM input edge cases.
- Restricted primary click path to pure left-click (no simultaneous right button) for safer action dispatch.
- Synced release note text in top-level changelog and Settings embedded changelog.

Reason:
- User reported VMware guest halt popup when opening the desktop context menu via right click.

### 20. Clamped raw mouse coordinates before cursor rendering

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Added coordinate clamping immediately after `mouse_get_x()` / `mouse_get_y()` in the main loop.
- Ensured cursor composition and draw paths never receive negative or out-of-bounds mouse coordinates.
- Kept prior right-click packet hardening in `process_mouse(...)` and synced release notes text.

Reason:
- Right-click still triggered VMware guest halt popup; raw polled coordinates could still bypass earlier click-handler-only clamping and crash during render/input paths.

### 21. Added in-OS runtime error popup for easier fault diagnosis

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Added a modal in-OS runtime error popup (`Error` window style with `OK`) rendered on top of desktop UI.
- Added `raise_runtime_error(...)` helper to surface detectable runtime anomalies in both UI and serial logs.
- Wired popup triggers for:
  - ambiguous dual-button mouse packets (`L+R` together),
  - out-of-bounds raw polled mouse coordinates.
- Made the popup modal for input handling so accidental background actions do not hide the error condition.

Reason:
- Requested visible in-OS error handling so failures are easier to detect and diagnose than VM-level generic popup messages.

### 22. Modernized Start menu layout and aligned click geometry

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Increased Start menu panel width/height and retained modern light card styling with cleaner spacing.
- Reworked Start menu internal layout (profile rail, header, search strip, action rows) to reduce cramped text and clipping.
- Ensured `About GamerOS` is a clear dedicated Start menu entry and remains mapped to `ABOUT.EXE`.
- Updated Start menu click hit-testing to use the new row geometry so visual rows and interactive rows match exactly.

Reason:
- Requested a more modern Start menu and explicit About app access from Start menu, with the screenshot showing cramped/truncated entries.

### 23. Expanded roadmap with storage, process model, and cross-toolchain goals

Files:
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Added new `## Planned` roadmap item for real persistent disk I/O using ATA PIO instead of in-memory-only file handling.
- Added new `## Planned` roadmap item for a proper process/task model and defined app loader format.
- Added new `## Planned` roadmap item for integrating an `i686-elf-gcc` cross-toolchain to move C components back into the kernel cleanly.
- Synced the same three roadmap entries in both top-level changelog markdown and the Settings app embedded changelog text.

Reason:
- Requested explicit roadmap additions covering persistent ATA PIO storage, process/task loader architecture, and cross-toolchain support, with changelog synchronization across docs and Settings.

### 24. Implemented ATA PIO persistence and GOSAPP process loader model

Files:
- `src/intf/ata_pio.h`
- `src/impl/drivers/ata_pio.c`
- `src/impl/filesystem/fs.c`
- `src/intf/process_model.h`
- `src/impl/kernel/process_model.c`
- `src/impl/kernel/main.c`
- `Makefile`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`
- `src/apps/settings/settings_ui.c`

Changes:
- Added a real ATA PIO disk driver for primary IDE I/O (`ata_pio_init`, single-sector read/write, cache flush).
- Reworked filesystem persistence path to use sector I/O:
  - disk-backed metadata (superblock, file table, directory table)
  - reserved sector ranges per file for stable writes
  - metadata sync on file/directory creation and write updates
  - fallback RAM storage path retained when ATA device is unavailable.
- Added a concrete process/task model with executable loader:
  - task table with PID/state tracking (`READY`, `RUNNING`, `TERMINATED`)
  - `GOSAPP` loader format parsing (`Name=`, `Entry=`)
  - launch flow now validates/loads executable task image before window open
  - window close path terminates the associated task by window type.
- Converted built-in executable payloads (`NOTEPAD.EXE`, `SETTINGS.EXE`, `EXPLORER.EXE`, `ABOUT.EXE`) from ad-hoc `MZ` placeholder text to the new `GOSAPP` format.
- Updated top-level changelog and in-Settings changelog text to move ATA/process-loader items from roadmap to implemented release notes.

Build:
- Ran `make build-x86_64`: failed because `x86_64-linux-gnu-gcc` is not installed in this host environment.
- Ran `build-iso.bat`: failed because Docker daemon is not running (`dockerDesktopLinuxEngine` pipe missing).

Reason:
- Requested implementation of real persistent ATA PIO storage and a proper process/task model with loader format, then ISO build and synced release documentation.

### 25. Expanded installed-app storage layout and synced Settings roadmap

Files:
- `src/impl/kernel/main.c`
- `src/apps/apps.c`
- `src/intf/apps.h`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`
- `.gitignore`

Changes:
- Expanded the storage bootstrap so built-in apps now have clearer real filesystem presence:
  - install roots under `C:/GamerOS/Apps/<AppName>`
  - per-app `AppData` folders under `C:/Users/Admin/AppData/<AppName>`
  - registry snapshots under `C:/GamerOS/Registry`
  - seeded user folders such as `Documents` and `Downloads`
- Extended `app_descriptor_t` with manifest path, install root, and user data root so the shell tracks apps as installed system objects instead of only EXE names.
- Switched bootstrap file writes to open-or-create behavior so core metadata files can be refreshed instead of only being created once.
- Updated Explorer interaction so clicking `.EXE` entries in the file list launches the real registered app from the filesystem tree.
- Synced the embedded Settings changelog with the top-level `1.400` changelog additions and adjusted the `## Planned` roadmap so it focuses on features that still do not exist in the OS.
- Added `.gitignore` entries for generated build output and common binary/log artifacts.

Reason:
- Requested fuller storage/app realism for build `1.400`, changelog sync inside Settings, and a roadmap that only advertises genuinely future work rather than features that already landed.

### 26. Added display settings with runtime resolution switching and fixed Docker ISO staging

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `src/intf/graphics.h`
- `src/impl/graphics/vga_graphics.c`
- `Makefile`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Added a dedicated `Display` tab to Settings and inserted it into the Settings navigation list.
- Added display profiles for:
  - `640 x 480 (Safe)`
  - `800 x 600`
  - `1280 x 720`
  - `Native`
- Wired Settings clicks to runtime resolution changes through new graphics helpers:
  - `graphics_set_resolution(...)`
  - `graphics_get_native_width()`
  - `graphics_get_native_height()`
- Limited higher-resolution profiles to true-color framebuffer sessions so VGA users only see the safe supported mode.
- Re-clamped open windows after a resolution change so UI surfaces stay on-screen after the desktop size changes.
- Fixed ISO staging in `Makefile` by forcing file overwrites during copy steps and clearing old temporary ISO outputs before regeneration.
- Re-ran the Docker build successfully after the packaging fix; build completion now reaches ISO generation instead of failing on existing staged files.

Build:
- Ran `build.bat` with Docker available.
- Build now completes successfully and produces the ISO.
- Remaining output still includes repeated NASM warnings from `src/impl/x86_64/boot.asm` about attempted initialization in `.bss`, but there were no build-stopping errors.

Reason:
- Requested a real display settings surface in Settings where users can change resolution, plus a clean build validation pass without the prior Docker packaging failure.

### 27. Added on-screen boot debug overlay and narrowed VMware startup failures

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Added a persistent top-left debug overlay that keeps recent boot/runtime messages visible on-screen.
- Added boot milestone tracing for:
  - kernel entry
  - graphics backend selection
  - keyboard/mouse init
  - window init
  - startup animation completion
  - storage init begin/complete
  - main-loop handoff
- Routed runtime popup errors into the same debug log so visible UI errors and boot diagnostics share one trace surface.

Reason:
- Requested visible on-screen error text because VMware was halting before serial-only diagnostics were practical to inspect.

### 28. Hardened VMware shell handoff and disabled unsafe ATA probing

Files:
- `src/impl/kernel/main.c`
- `src/impl/drivers/mouse.c`
- `src/impl/drivers/keyboard.c`
- `src/impl/drivers/ata_pio.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Changed boot handoff so GamerOS paints one desktop frame before the first steady-state input polling cycle.
- Added bounded safety limits to `mouse_poll()` and `keyboard_poll()` so VMware cannot get trapped in infinite PS/2 drain loops.
- Disabled the current ATA PIO probe path and forced filesystem startup onto the RAM-backed fallback while VMware crash investigation continues.
- Kept the debug overlay active across desktop rendering so black-screen and shell-handoff failures remain visible after the loading screen.

Build:
- Ran `build.bat` successfully after the boot/input hardening changes.
- ISO generation completed.

Reason:
- Booting in VMware still stalled or halted after the loading screen, so the shell handoff and storage probe paths needed to be made safer before app-launch debugging could continue.

### 29. Reduced app-launch loader risk and added loader tracing

Files:
- `src/impl/kernel/process_model.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-xx-xx.md`
- `walkthrough_src_bugfixes_2026-xx-xx.md`

Changes:
- Moved the executable loader read buffer out of the per-launch kernel stack frame into static storage.
- Added serial loader tracing for:
  - `Loader: spawn request`
  - `Loader: open exe`
  - `Loader: read exe`
  - `Loader: validate header`
  - `Loader: allocate task slot`
  - `Loader: spawn success`
- Left the process/task model intact while removing the strongest immediate stack-overflow candidate from the launch path.

Build:
- Ran `build.bat` successfully after the loader change.
- Remaining build output still only includes the pre-existing NASM `.bss` warnings from `src/impl/x86_64/boot.asm`.

Reason:
- After boot was stabilized enough to reach storage initialization, opening an app still black-screened the VM; the next likely fault source was the stack-heavy loader path in `process_spawn_from_exe(...)`.
