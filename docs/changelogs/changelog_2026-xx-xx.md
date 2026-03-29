# GamerOS Changelog (2026-xx-xx)

This changelog tracks work for the next release cycle.

## Release
- Version: `00m1`
- Build: `1.500`
- Date: `2026-xx-xx` (TBD)

## Added


## Changed


## Fixed


## Notes
- Release date is not finalized yet.

## Planned
 - New app container model so desktop apps behave like first-class system windows (with minimize/maximize/close, focus handling, and z-order).
 - Extend the app lifecycle from the current running/stopped tracker into a richer controller that can also suspend, resume, and terminate or throttle background apps.
 - Standardized app chrome guidelines so built-in tools (Settings, File Manager, Media Player) visually align with the shell.
 - Interactive app registry browser in Settings or Explorer that shows manifests, install roots, permissions, and runtime state.
 - Extended `.gosapp` manifest format with versioning, dependency, and fine-grained permission fields.
 - Basic permissions model for apps (filesystem, network, settings, hardware) to prepare for future sandboxing.
 - Unified input dispatch layer that routes keyboard/mouse events to the active app with proper focus rules.
 - Initial window manager hooks to support multiple top-level app windows with proper stacking and activation.
 - App-aware taskbar integration so running apps appear with icons, titles, and hover previews.
 - Simple inter-app protocol for launching apps with arguments (e.g., open file from File Manager into Media Player).
 - High-level UI controls library (buttons, labels, checkboxes, sliders) for building consistent in-box apps.
 - Layout helpers for common app patterns (sidebar/content, toolbar/content, dialog with actions).
 - New `App Shell` sample app demonstrating the recommended window structure and toolbar patterns.
 - Multi-pane File Manager upgrade with breadcrumb navigation, details columns, and file properties/actions.
 - Basic storage abstraction layer that hides raw disk details behind a friendly volume and path model.
 - High-level storage API for apps to read/write user documents without touching low-level disk code.
 - Expand the `AppData` model into a first-class API (`GOS:/User/AppData/<AppId>`) for all current and future apps.
 - Live settings storage backend that actually loads persisted toggles, theme choice, and last-used graphics mode at startup.
 - Generic key/value persistence helper that apps can call instead of writing raw preference files directly.
 - First version of a save-game style data slot API for games and game launchers.
 - Read-only system volume layout for core OS assets, with a separate writable user area.
 - Bootstrap for a future installer experience that can place apps into the correct volume and register them with the shell.
 - Storage-aware About/Gaming pages that display detected volumes, free space, and backend information.
 - Internal diagnostics overlay app that can inspect current apps, windows, and storage mounts.
 - App-callable error and warning dialog API so apps can raise shell-native modal messages consistently.
 - Simple notification/toast API for apps to surface background events (e.g., file copy finished).
 - Background service concept for future features like update checks and indexing without blocking the shell.
 - Skeleton for an in-box text editor app that uses the new app framework and storage APIs.
 - Planned media player UI spec (play/pause, seek, volume) wired up to the new app container model.
 - Initial hooks for a plugin system that will later allow optional features and tools to be shipped as separate app bundles.
 - Expanded desktop context menu customization (pin actions, sort icons, wallpaper/profile actions).
 - Desktop icon arrangement tools (snap-to-grid toggle, align, auto-arrange, manual save/load layouts).
 - About app diagnostics extensions (build hash, boot mode, memory/storage snapshot, export report).
 - App install/uninstall flow in Settings with manifest validation and rollback on failure.
 - Update channel selection in GamerOS Update (`stable`, `preview`, `dev`) with per-channel roadmap sections.
 - Cross-toolchain integration (`i686-elf-gcc`) so C components can move back into the kernel cleanly.
