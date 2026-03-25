#include "settings_ui.h"

static const char* g_tabs[] = {
    "System",
    "Display",
    "Bluetooth & devices",
    "Network & internet",
    "Personalization",
    "Apps",
    "Accounts",
    "Time & language",
    "Gaming",
    "Accessibility",
    "Privacy & security",
    "GamerOS Update",
};

static const char* g_changelog_md_path = "C:/GamerOS/Apps/Settings/CHANGELOG_2026-XX-XX.MD";

static const char* g_changelog_md_text =
    "# GamerOS Changelog (TBD)\n"
    "\n"
    "This changelog tracks work for the next release cycle.\n"
    "\n"
    "## Release\n"
    "- Version: `00m1`\n"
    "- Build: `1.400`\n"
    "- Date: `TBD`\n"
    "\n"
    "## Added\n"
    "- True-color framebuffer path added (24/32bpp linear framebuffer support).\n"
    "- New graphics API helpers for RGB drawing:\n"
    "  - `draw_pixel_rgb(...)`\n"
    "  - `clear_screen_rgb(...)`\n"
    "- Added explicit video mode enum target `MODE_VESA_1920x1080`.\n"
    " - Introduced a simple app lifecycle tracker so built-in desktop apps behave like first-class GamerOS apps with explicit running/stopped state.\n"
    " - Added a GamerOS virtual path system (`GOS:/User`, `GOS:/System`, `GOS:/Apps`) that maps to concrete filesystem locations.\n"
    " - Wired Notepad to use `GOS:/User/Notepad/NOTEPAD.TXT` so its data lives in a per-app user folder instead of a hardcoded C: path.\n"
    " - Created dedicated per-app and user storage folders (Notepad, Settings, Explorer, Saves) to keep app data organized.\n"
    " - Added basic app manifest files (`*.gosapp`) for built-in apps (Notepad, Settings, Explorer) describing ID, display name, EXE path, entry point, category, and permissions.\n"
    " - Initialized save-slot style files under the user `Saves` folder to prepare for future game/launcher integration.\n"
    " - Added standalone `ABOUT.EXE` system app with app metadata/manifest so About is launchable outside Settings.\n"
    " - Added desktop right-click context menu with quick actions (`About GamerOS`, `Settings`, `Refresh Desktop`).\n"
    " - Added in-OS runtime error popup dialog (modal with `OK`) so detectable failures show directly inside GamerOS.\n"
    " - Added real ATA PIO disk driver (`0x1F0` primary IDE path) for sector-level read/write operations.\n"
    " - Added persistent filesystem metadata layout (superblock + file table + directory table) stored on disk sectors.\n"
    " - Added `GOSAPP` executable loader format (`GOSAPP`, `Name=...`, `Entry=...`) and task table for spawned app processes.\n"
    " - Expanded the virtual storage layout with real app install roots, `AppData` folders, registry snapshots, Documents/Downloads roots, and persisted preference files for built-in apps.\n"
    " - Added shell-visible installed app registry and storage registry files under `C:/GamerOS/Registry`.\n"
    " - Added a dedicated `Display` Settings tab with clickable resolution profiles and backend-aware availability checks.\n"
    " - Added runtime display resolution helpers so the shell can switch render size without rebooting when framebuffer mode is active.\n"
    " - Added an always-visible top-left debug overlay that mirrors recent boot/runtime fault messages on-screen.\n"
    " - Added staged boot progress tracing so startup now reports graphics, input, storage, and shell handoff milestones.\n"
    "\n"
    "## Changed\n"
    "- Build metadata updated from `1.300` to `1.400`.\n"
    "- Desktop bottom accent above taskbar changed from dual blue glow bands to a subtle neutral shadow for cleaner wallpaper blending.\n"
    "- Startup/loading screen fully redesigned with modern card layout, rounded/chamfered corners, and updated progress rail treatment.\n"
    "- Branding text updated to remove Windows-style wording in favor of GamerOS-native naming.\n"
    "- Core shell UI modernized to match loading screen direction:\n"
    "  - window chrome and controls updated with rounded/chamfered surfaces\n"
    "  - Start menu restyled with modern panel/button treatment\n"
    "  - taskbar, task buttons, and clock updated to modernized geometry\n"
    "  - Settings theme label updated from legacy XP naming to GamerOS Modern\n"
    "- Boot flow now prefers bootloader-provided framebuffer and keeps that mode into kernel startup.\n"
    "- Graphics backend now safely activates true-color mode whenever a valid bootloader framebuffer is provided.\n"
    "- Graphics pipeline compatibility pass completed so legacy UI color IDs and cursor composition behave correctly on both indexed VGA and true-color framebuffer paths.\n"
    "- Boot header now explicitly requests `1920x1080x32` graphics mode again for Full HD true-color startup when supported.\n"
    "- Settings `Gaming` now displays live graphics backend mode, bpp, and resolution (including RGBA indicator).\n"
    "- About details moved out of Settings into the dedicated `About GamerOS` app window.\n"
    "- Start menu layout refreshed for clarity with cleaner cards, search strip, and stronger action hierarchy.\n"
    "- Loading screen background reworked to a smoother gradient-style presentation.\n"
    "- Loading indicator switched from linear bar to a circular progress loader.\n"
    "- Completed full-shell modern visual pass with a bright unified style:\n"
    "  - startup card and spinner recolored for a clean contemporary boot look\n"
    "  - Start menu restyled with profile chip, light surfaces, and neutral shutdown action\n"
    "  - taskbar, Start button, task buttons, and clock moved to light modern treatment\n"
    "  - desktop icon labels and watermark tuned for better readability on bright backgrounds\n"
    "- Settings `GamerOS Update` changelog viewer now renders markdown headings with a larger font for clearer section hierarchy.\n"
    "- Refined Settings layout again with larger left-nav rows, a wider navigation rail, and more content spacing so controls are easier to scan and click.\n"
    "- Enlarged `GamerOS Update` sub-buttons and widened their hit targets so `Build 1.400 changelog` and `Roadmap` labels fit cleanly.\n"
    "- Kept `## Planned` in markdown source but mapped it to the `Roadmap` sub-view so the main build changelog view only shows release/add/changed/fixed/notes.\n"
    "- Start menu `About GamerOS` action now launches the standalone About app instead of jumping to a Settings tab.\n"
    "- Start menu redesigned with wider modern layout, cleaner spacing, and improved label fit so entries like `File Explorer`, `GamerOS Update`, and `About GamerOS` are fully readable.\n"
    "- Start menu hit-testing updated to match new row geometry, improving click reliability on all menu entries.\n"
    "- Built-in app executables now use the new loader format (`GOSAPP`) instead of ad-hoc `MZ` text placeholders.\n"
    "- App launch path now creates/marks runtime tasks via loader validation before opening the target window.\n"
    "- File Explorer now browses real filesystem entries from GamerOS system/user folders and can launch `.EXE` files directly from the file list.\n"
    "- Built-in app metadata now includes manifest path, install root, and per-app user data root so apps are represented as real installed applications in the storage tree.\n"
    "- Settings navigation now includes a dedicated `Display` page instead of keeping display information buried under other tabs.\n"
    "- Docker build packaging was hardened so repeated ISO builds overwrite staged files cleanly instead of failing on existing outputs.\n"
    "- Boot handoff now paints one shell frame before entering steady-state input polling so VMware no longer sits on a black post-loader screen when PS/2 state is noisy.\n"
    "- Input polling was moved onto bounded safety loops instead of unbounded controller drains.\n"
    "\n"
    "## Fixed\n"
    "- Fixed wallpaper/taskbar transition mismatch caused by the two blue desktop glow strips.\n"
    "- Fixed stale About build text (now shows `1.400`).\n"
    "- Fixed hard lock to legacy 16-color planar rendering path by adding true-color framebuffer output path when available.\n"
    "- Fixed VMware startup crash path by hardening multiboot framebuffer validation and forcing VGA fallback when framebuffer memory/type/range is unsafe.\n"
    "- Fixed true-color color mapping bug where non-`0x0F` palette IDs were truncated.\n"
    "- Fixed cursor background restore artifacts on true-color backend by switching cursor underlay capture/restore to RGB-aware path.\n"
    "- Fixed boot-time triple-fault by relocating early page tables away from kernel image/BSS memory range.\n"
    "- Fixed early paging initialization robustness by allocating P4/P3/P2 tables as page-aligned kernel BSS symbols instead of fixed physical addresses.\n"
    "- Fixed ultra-dark desktop/UI rendering in true-color mode by applying shell palette mappings during non-VGA palette initialization.\n"
    "- Fixed near-black desktop visibility when wallpaper asset is too dark by adding adaptive bright modern wallpaper fallback rendering.\n"
    "- Fixed Settings information architecture by removing duplicated About content and keeping About in its own dedicated app surface.\n"
    "- Fixed VM instability path where desktop right-click/context-menu input packets could trigger unsafe mouse-state transitions and halt the guest.\n"
    "- Fixed VM crash path by clamping raw polled mouse coordinates before cursor composition/rendering, preventing out-of-range right-click packets from corrupting draw paths.\n"
    "- Fixed silent-debugability issue by surfacing unstable mouse packet/bounds anomalies in an in-OS popup and serial log.\n"
    "- Fixed storage bootstrap paths still writing stale `1.300` build metadata into the GamerOS system config.\n"
    "- Fixed repository noise by ignoring generated `build`/`dist` outputs and common binary/log artifacts via `.gitignore`.\n"
    "- Fixed repeated Docker ISO builds failing during staging because `cp` refused to overwrite existing `kernel.elf` and ISO outputs.\n"
    "- Fixed Settings missing a real display configuration surface by exposing resolution switching directly in the UI.\n"
    "- Fixed post-loading-screen black screen on VMware by presenting the desktop before first PS/2 polling and capping poll-loop drain counts.\n"
    "- Fixed a kernel-stack-heavy app loader path by moving the executable read buffer out of the launch-time stack frame.\n"
    "- Fixed boot/storage investigation visibility by surfacing storage/bootstrap milestones in the debug overlay instead of relying only on serial.\n"
    "- Fixed unstable VMware boot path further by disabling the unsafe ATA PIO probe and forcing the filesystem onto the RAM-backed storage fallback for now.\n"
    "\n"
    "## Notes\n"
    "- Release date is not finalized yet.\n"
    "\n"
    "## Planned\n"
    " - New app container model so desktop apps behave like first-class system windows (with minimize/maximize/close, focus handling, and z-order).\n"
    " - Extend the app lifecycle from the current running/stopped tracker into a richer controller that can also suspend, resume, and terminate or throttle background apps.\n"
    " - Standardized app chrome guidelines so built-in tools (Settings, File Manager, Media Player) visually align with the shell.\n"
    " - Interactive app registry browser in Settings or Explorer that shows manifests, install roots, permissions, and runtime state.\n"
    " - Extended `.gosapp` manifest format with versioning, dependency, and fine-grained permission fields.\n"
    " - Basic permissions model for apps (filesystem, network, settings, hardware) to prepare for future sandboxing.\n"
    " - Unified input dispatch layer that routes keyboard/mouse events to the active app with proper focus rules.\n"
    " - Initial window manager hooks to support multiple top-level app windows with proper stacking and activation.\n"
    " - App-aware taskbar integration so running apps appear with icons, titles, and hover previews.\n"
    " - Simple inter-app protocol for launching apps with arguments (e.g., open file from File Manager into Media Player).\n"
    " - High-level UI controls library (buttons, labels, checkboxes, sliders) for building consistent in-box apps.\n"
    " - Layout helpers for common app patterns (sidebar/content, toolbar/content, dialog with actions).\n"
    " - New `App Shell` sample app demonstrating the recommended window structure and toolbar patterns.\n"
    " - Multi-pane File Manager upgrade with breadcrumb navigation, details columns, and file properties/actions.\n"
    " - Basic storage abstraction layer that hides raw disk details behind a friendly volume and path model.\n"
    " - High-level storage API for apps to read/write user documents without touching low-level disk code.\n"
    " - Expand the `AppData` model into a first-class API (`GOS:/User/AppData/<AppId>`) for all current and future apps.\n"
    " - Live settings storage backend that actually loads persisted toggles, theme choice, and last-used graphics mode at startup.\n"
    " - Generic key/value persistence helper that apps can call instead of writing raw preference files directly.\n"
    " - First version of a save-game style data slot API for games and game launchers.\n"
    " - Read-only system volume layout for core OS assets, with a separate writable user area.\n"
    " - Bootstrap for a future installer experience that can place apps into the correct volume and register them with the shell.\n"
    " - Storage-aware About/Gaming pages that display detected volumes, free space, and backend information.\n"
    " - Internal diagnostics overlay app that can inspect current apps, windows, and storage mounts.\n"
    " - App-callable error and warning dialog API so apps can raise shell-native modal messages consistently.\n"
    " - Simple notification/toast API for apps to surface background events (e.g., file copy finished).\n"
    " - Background service concept for future features like update checks and indexing without blocking the shell.\n"
    " - Skeleton for an in-box text editor app that uses the new app framework and storage APIs.\n"
    " - Planned media player UI spec (play/pause, seek, volume) wired up to the new app container model.\n"
    " - Initial hooks for a plugin system that will later allow optional features and tools to be shipped as separate app bundles.\n"
    " - Expanded desktop context menu customization (pin actions, sort icons, wallpaper/profile actions).\n"
    " - Desktop icon arrangement tools (snap-to-grid toggle, align, auto-arrange, manual save/load layouts).\n"
    " - About app diagnostics extensions (build hash, boot mode, memory/storage snapshot, export report).\n"
    " - App install/uninstall flow in Settings with manifest validation and rollback on failure.\n"
    " - Update channel selection in GamerOS Update (`stable`, `preview`, `dev`) with per-channel roadmap sections.\n"
    " - Cross-toolchain integration (`i686-elf-gcc`) so C components can move back into the kernel cleanly.\n"
    ;

int settings_ui_tab_count(void) {
    return (int)(sizeof(g_tabs) / sizeof(g_tabs[0]));
}

const char* settings_ui_tab_name(int idx) {
    if (idx < 0 || idx >= settings_ui_tab_count()) return "";
    return g_tabs[idx];
}

const char* settings_ui_panel_title(void) {
    return "Settings";
}

const char* settings_ui_changelog_md_path(void) {
    return g_changelog_md_path;
}

const char* settings_ui_changelog_md_text(void) {
    return g_changelog_md_text;
}

