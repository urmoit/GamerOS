# GamerOS Changelog (2026-04-03)

This changelog tracks work for the next release cycle.

## Release
- Version: `00m1`
- Build: `1.500`
- Date: `2026-04-03`

## Latest Updates (Stability and Reliability Hardening)

### Kernel Boot Robustness
- Fixed persistent NASM warnings from `boot.asm` BSS section initialization (zero warnings now).
- Moved all initialized data from `.bss` into `.data` section to eliminate undefined behavior.
- Added compile-time page table alignment assertions for boot paging structures.
- Added boot-time page table self-check validating identity mapping coverage.
- Reordered kernel init sequence: Serial Debug → HAL → Filesystem → Input → Graphics → Shell.
- Serial debug output now initializes first so all boot stages can report milestones.
- Filesystem initializes before graphics so assets can load from storage.
- Replaced jump-based `io_wait()` with standard port 0x80 delay for consistent timing.
- Added explicit 4K BSS alignment in linker script to prevent kernel/data overlap.

### Graphics Subsystem Stability
- Added bounds validation to all VGA planar write paths in `swap_buffers()`.
- Added explicit memory barrier after plane writes for VGA hardware update visibility.
- Extended true-color framebuffer validation with pitch overflow and alignment checks.
- Framebuffer rejection now logs to serial debug for diagnostics.
- Cursor rendering now validates coordinates and clips to screen bounds.
- Cursor underlay capture now checks graphics mode before using RGB path.
- Font rendering functions now validate bounds and handle NULL inputs gracefully.
- Resolution switch function validates against native framebuffer and back buffer capacity.

### Input Driver Stabilization
- Keyboard extended key prefix (`0xE0`) now has timeout to prevent stuck state.
- Keyboard buffer overflow handling documented and improved with count helper.
- Unknown scancodes now logged to serial instead of being silently ignored.
- Mouse packet sync validation added (first byte must have bit 3 set).
- Mouse packet cycle reset timeout added to resync if packets are lost.
- Wheel delta clamping improved with saturation at ±127 instead of wrapping.
- Mouse init now verifies AUX device presence before configuration.
- PIC EOI sequencing corrected for IRQ8-15 cascade handling.
- Added `pic_mask_irq()` and `pic_unmask_irq()` helpers.
- Added spurious interrupt handler for IRQ7 and IRQ15.
- All input polling functions now have bounded iteration counts.

### Filesystem and Storage Reliability
- All filesystem functions now validate input parameters (NULL checks, bounds).
- Path comparison handles edge cases (empty paths, trailing slashes, case sensitivity).
- Directory listing validates output array bounds before writing.
- ATA PIO driver operations now have timeout counters to prevent infinite hangs.
- ATA device detection has explicit timeout for each wait phase.
- IDENTIFY command failure no longer blocks boot, gracefully reports unavailable.
- Cache flush after writes now has timeout and error check.
- RAM-backed storage properly zeroed on init with sector bounds validation.
- Superblock validation checks magic string before trusting metadata.
- Filesystem bootstrap creates default directories only if metadata load fails.

### Shell and Window Manager Stability
- Main input polling loop has explicit safety bounds (max 64 iterations per device).
- Window drag/resize operations validate coordinates and maintain minimum dimensions.
- Drag/resize state resets on mouse button release (no stuck drag state).
- Window creation validates count, title length, position, and size bounds.
- Window focus management handles all-closed case cleanly (focus set to -1).
- Window list array properly maintained with no gaps on window close.
- All UI drawing functions validate coordinates before rendering.
- Start menu rendering validates menu state before drawing.
- Taskbar clock handles RTC read failures gracefully.
- Widget creation validates parameters (NULL text, dimensions, parent-child).
- Widget destruction properly frees all allocated memory recursively.
- Event handling validates widget state (enabled/visible) before dispatching.

### Driver Subsystem Hardening
- RTC reads preserve NMI enable bit to prevent spurious Non-Maskable Interrupts.
- Time read retries if values change during read (RTC update in progress).
- Serial I/O functions now have timeouts to prevent hangs on absent UART.
- Serial init validates UART presence before configuration.
- VMware backdoor access now has exception handler for port access.
- VMware mouse operations return gracefully instead of hanging when disabled.
- USB driver stubs documented with clear "not yet implemented" markers.

### Executive Layer and User Mode
- Executive init validates each subsystem before proceeding to next.
- Error state tracked with explicit error codes for diagnostics.
- Object manager init validated before dependent services start.
- User mode process table reset on init (no stale entries).
- PID generation wraps safely without overflow.
- Process access validation handles edge cases (caller equals target).
- GOSAPP executable format validated before loading.
- Task slot allocation handles full table gracefully.

### Build System Reliability
- Build targets have explicit directory creation dependencies.
- Clean target properly removes all generated files.
- ISO staging directory cleaned before each build.
- Dockerfile uses pinned Ubuntu 22.04 base for reproducible builds.
- Build scripts check tool availability before attempting operations.
- Build failures reported with error codes instead of silent failures.
- Git ignore updated to exclude all build artifacts and generated files.

## Added
- `keyboard_buffer_count()` helper for debugging keyboard buffer state.
- `pic_mask_irq()` and `pic_unmask_irq()` for safe interrupt enable/disable.
- `pic_handle_spurious()` for IRQ7/IRQ15 spurious interrupt handling.
- `input_poll_all()` helper for safe sequential keyboard and mouse polling.
- `graphics_get_bpp()` API for runtime graphics capability reporting.
- Serial debug logging for framebuffer rejection reasons.
- Boot milestone reporting for each init stage via serial output.
- Page table self-check validation during early boot.
- Widget event validation for enabled/visible state before dispatch.
- Process executable format validation in GOSAPP loader.
- Build script tool availability checks (Docker, GCC, NASM, QEMU).

## Changed
- Kernel init sequence reordered for safer dependency ordering.
- `io_wait()` implementation changed from jump chain to port 0x80 delay.
- Keyboard extended key handling now has timeout instead of permanent state.
- Mouse packet handling now validates sync byte before processing.
- Mouse wheel delta clamping changed from wrap to saturation behavior.
- ATA PIO driver operations now timeout instead of waiting indefinitely.
- Filesystem parameter validation applied to all public API functions.
- Window manager array maintenance changed to compact on close (no gaps).
- RTC register read preserves NMI enable bit (bit 7) during access.
- Serial I/O functions timeout on absent hardware instead of hanging.
- Executive init validates each subsystem before proceeding.
- Build system targets have explicit directory creation dependencies.
- Docker build environment pinned to Ubuntu 22.04 for reproducibility.
- Git ignore expanded to cover all build artifacts and IDE files.

## Fixed
- Fixed hundreds of NASM warnings from boot.asm BSS section initialization.
- Fixed potential page table misalignment on memory-constrained VMs.
- Fixed boot sequence ordering that could drop input events during startup.
- Fixed I/O wait timing inconsistency that could be optimized out by compilers.
- Fixed VGA planar write paths that could corrupt memory on slow hardware.
- Fixed true-color framebuffer validation that missed pitch overflow cases.
- Fixed cursor rendering that could write out-of-bounds on screen edges.
- Fixed font rendering that could access invalid glyph indices.
- Fixed resolution switching that could exceed back buffer capacity.
- Fixed keyboard extended key prefix getting stuck without following scancode.
- Fixed mouse packet desync causing freeze on VMware with noisy PS/2.
- Fixed mouse wheel delta wrapping instead of saturating at limits.
- Fixed PIC EOI sequencing for IRQ8-15 cascade interrupts.
- Fixed infinite poll loops in keyboard and mouse polling functions.
- Fixed filesystem crashes from NULL or oversized path parameters.
- Fixed ATA driver hang when device is absent or unresponsive.
- Fixed RAM disk storage not properly zeroed on initialization.
- Fixed filesystem bootstrap creating duplicate directories.
- Fixed shell input loop that could freeze from unbounded polling.
- Fixed window manager leaving gaps in window array after close.
- Fixed UI rendering that could corrupt memory at screen edges.
- Fixed widget memory leaks on destruction of complex widgets.
- Fixed RTC reads triggering spurious NMIs from bit 7 corruption.
- Fixed serial I/O hanging on hardware without UART.
- Fixed VMware backdoor crashes on non-VMware hardware.
- Fixed executive layer cascading failures from subsystem init errors.
- Fixed process model crashes from malformed GOSAPP executables.
- Fixed build system failures from stale artifacts in repeated builds.
- Fixed Docker build non-reproducibility from unpinned base image.
- Fixed build scripts silently failing without error messages.

## Notes
- Release date is finalized.
- This is a stability-focused release with no new user-visible features.
- All changes are backward compatible with existing apps and configurations.
- Build numbers incremented to 1.500 to reflect significant internal improvements.

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
 - PIT/HPET timer interrupt implementation for preemptive multitasking.
 - Proper `kmalloc`/`kfree` heap memory management implementation.
 - USB driver stack completion for mouse/keyboard/device support.
 - Basic TCP/IP networking implementation for internet connectivity.
 - AC'97 or HD Audio driver for system audio support.
 - ACPI implementation for power management and hardware discovery.
 - Cross-toolchain integration (`i686-elf-gcc`) so C components can move back into the kernel cleanly.
