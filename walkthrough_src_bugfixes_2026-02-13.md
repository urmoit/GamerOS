# Walkthrough - `src` Bug Fix Pass (2026-02-13)

This document captures everything fixed during the `src` stabilization pass, plus the validation results and known blockers.
It now includes additional fix waves applied after repeated VMware/runtime testing and UI iteration.

## Fixes Applied

### 1. Corrected core integer typedefs

File:
- `src/intf/stdint.h`

Changes:
- `uint64_t` -> `unsigned long long`
- `int64_t` -> `signed long long`
- `size_t`, `uintptr_t`, `intptr_t` mapped to 64-bit-compatible typedefs in this environment.

Reason:
- Prevent width mismatch and shift-size bugs in 64-bit-related code paths.

### 2. Unified and extended graphics interface

File:
- `src/intf/graphics.h`

Changes:
- Added compatibility alias:
  - `typedef video_mode_t vga_mode_t;`
- Added legacy mode macros:
  - `VGA_MODE_13H`, `VGA_MODE_12H`
- Added `vga_set_mode(...)` declaration.
- Added compatibility declarations for legacy callers:
  - `vga_clear`, `vga_set_pixel`, `vga_get_pixel`,
  - `vga_fill_rect`, `vga_draw_rect`, `vga_draw_line`,
  - `vga_draw_circle`, `vga_fill_circle`, `vga_draw_string`,
  - `vga_blit_buffer`, `vga_draw_bitmap_cursor`,
  - `vga_draw_shadow`, `vga_fill_rounded_rect`, `vga_draw_rounded_rect`.
- Added compatibility aliases/macros:
  - `current_vga_width`, `current_vga_height`, `vga_framebuffer`, `current_color_depth`
- Added missing color alias:
  - `XP_COLOR_WHITE`

Reason:
- Large parts of `src` still rely on historical `vga_*` symbol names.

### 3. Implemented missing graphics compatibility functions

File:
- `src/impl/graphics/vga_graphics.c`

Changes:
- Added implementations for all newly declared compatibility APIs.
- Added:
  - `init_graphics()`
  - `vga_set_mode(...)`
  - circle draw/fill, buffer blit, bitmap cursor draw
  - shadow/rounded-rect fallback helpers

Reason:
- Removed implicit-function-declaration failures and made legacy callers compile without invasive rewrites.

### 4. Fixed GDI module compile breaks

File:
- `src/executive/gdi/gdi.c`

Changes:
- Switched allocator include from `memory.h` to `mm.h` for `kmalloc`/`kfree`.
- Removed reliance on undefined runtime symbol by setting color depth explicitly to 8 in this path.

Reason:
- `memory.h` did not expose allocator APIs used by this file.

### 5. Resolved UI signature conflicts

Files:
- `src/intf/ui.h`
- `src/impl/ui_system/ui.c`

Changes:
- Unified `draw_string` declaration/definition to `int x, int y, ...`.
- Renamed local static helper `draw_char` -> `ui_draw_char` in `ui.c` to avoid symbol/type conflict with global prototypes.

Reason:
- Removed conflicting declarations between `ui.h` and `graphics.h`.

### 6. Added missing mouse state API used by desktop code

Files:
- `src/intf/mouse.h`
- `src/impl/drivers/mouse.c`

Changes:
- Added `mouse_state_t` struct.
- Added `mouse_get_state()` declaration and implementation.

Reason:
- `gui_app.c` and `desktop_manager.c` used this API but it was not defined in the shared interface.

### 7. Fixed window rendering conflicts/warnings

File:
- `src/impl/ui_system/window.c`

Changes:
- Removed local color macro redefinitions that conflicted with `ui.h`.
- Used existing shared color macros.
- Updated `draw_string` call to match unified signature.

Reason:
- Eliminated macro redefinition warnings and signature mismatch risk.

### 8. Fixed missing include in widget renderer

File:
- `src/impl/ui_system/ui_widgets.c`

Changes:
- Added `#include "../../intf/graphics.h"`.

Reason:
- File used `vga_*` rendering helpers without including the interface that declares them.

### 9. Hardened interrupt return path for VMware stability

File:
- `src/impl/kernel_mode/hal/interrupts/isr.asm`

Changes:
- Removed `sti` immediately before `iretq` in ISR/IRQ stubs.

Reason:
- Re-enabling interrupts in the stub epilogue can re-enter before state is fully restored and is a known source of instability/triple-fault behavior in some VMs.

### 10. Fixed VESA buffer present path

File:
- `src/impl/graphics/vga_graphics.c`

Changes:
- Updated `swap_buffers()` to copy the full active frame to `framebuffer` when available, with VGA memory fallback.

Reason:
- Previous code truncated VESA copies to 320x200 in one path.

### 11. Replaced IPC stubs with a working baseline queue

File:
- `src/impl/kernel_mode/microkernel/ipc.c`

Changes:
- Implemented fixed-size in-kernel message queue.
- Added payload allocation/copy on send and consume/free on receive.
- Added queue initialization logic in `ipc_init()`.

Reason:
- Removes “always fail” IPC behavior and provides a basic functional IPC path.

### 12. Reduced allocator fragmentation in `kfree`

File:
- `src/impl/kernel_mode/microkernel/memory.c`

Changes:
- Added previous-block coalescing in addition to next-block coalescing.

Reason:
- Prevents avoidable heap fragmentation over time.

### 13. Removed object manager static-pool bottleneck

File:
- `src/executive/object_manager/object_manager.c`

Changes:
- Switched object allocation from fixed 4KB static pool to `kmalloc`.
- Added `kfree` on destroy and handle-allocation failure cleanup.
- Corrected allocator header include to `mm.h`.

Reason:
- Eliminates hard object-capacity cap caused by tiny static pool.

### 14. Enabled user mode subsystem initialization

File:
- `src/user_mode/user_mode.c`

Changes:
- Un-commented and wired subsystem init/shutdown calls (workstation, server service, security, Win32/POSIX/OS2, Windows9x/MSDOS compatibility layers).

Reason:
- Previously declared user mode initialization path did not actually initialize subsystems.

### 15. Implemented missing window manager operations

File:
- `src/user_mode/integral_subsystems/workstation/window_manager.c`

Changes:
- Added ID-to-window-slot mapping.
- Implemented `show/hide/move/resize/destroy/minimize/maximize/restore`.
- Implemented focus/front/back behavior and hit-testing (`window_get_at_position`).

Reason:
- Removed major functional stubs in window management.

### 16. Added start menu interaction and real clock in kernel desktop loop

File:
- `src/impl/kernel/main.c`

Changes:
- Added start menu open/close toggle via Start button.
- Added clickable menu items for launching windows.
- Replaced static `"12:00"` with RTC-backed time string using `get_time(...)`.
- Updated keyboard processing to return “changed” state for cleaner redraw triggering.

Reason:
- Addressed medium-priority desktop interactivity/runtime behavior gaps.

### 17. Completed executive service startup wiring

File:
- `src/executive/executive.c`

Changes:
- Added `gdi_init()` during executive startup.
- Registered baseline input drivers (`keyboard`, `mouse`) with I/O manager.
- Created baseline filesystem entries (`README.TXT`, `NOTEPAD.TXT`).

Reason:
- Addressed incomplete executive initialization and provided usable default services on boot.

### 18. Added Notepad cursor navigation keys

File:
- `src/impl/kernel/main.c`

Changes:
- Added handling for `KEY_LEFT`, `KEY_RIGHT`, `KEY_UP`, `KEY_DOWN`, `KEY_HOME`, and `KEY_END`.
- Cursor now moves within/among lines instead of only append/backspace editing.

Reason:
- Removed the primary editing limitation flagged in bug tracking.

### 19. Implemented UI framework event and frame loop behavior

File:
- `src/user_mode/integral_subsystems/workstation/ui_framework.c`

Changes:
- Added an internal event queue.
- Added keyboard/mouse polling and event dispatch in `ui_process_events()`.
- Updated `ui_render_frame()` to run desktop update + cursor draw + `swap_buffers()`.
- Added cursor visibility state support.

Reason:
- Replaced “skeleton-only” UI framework behavior with a working processing/render flow.

### 20. Hardened exception handling to prevent recursive fault escalation

File:
- `src/impl/kernel_mode/hal/interrupts/isr.c`

Changes:
- Added recursive-exception guard (`exception_in_progress`).
- Converted exception path to fail-stop (`cli` + `hlt` loop) instead of trying to continue from CPU faults.

Reason:
- Attempting to continue from faults (especially GPF/page-fault in unstable state) can cascade into double/triple-fault behavior.

### 21. Implemented queued asynchronous I/O processing

Files:
- `src/executive/io_manager/io_manager.c`
- `src/executive/io_manager/io_manager.h`

Changes:
- Added internal async request slot tracking.
- Changed `io_read_async`/`io_write_async` to queue pending operations.
- Added `io_process_async_requests(uint32_t max_requests)` to execute queued operations and fire callbacks.

Reason:
- Replaced synchronous “fake async” behavior with real queued async flow.

### 22. Fixed IDT null-entry present-bit bug (VMware hardening)

File:
- `src/impl/kernel_mode/hal/interrupts/idt.c`

Changes:
- Updated `set_idt_entry()` so `handler == 0` creates a fully zeroed, not-present gate.
- Previously, zero handlers were still marked present (`0x8E`) and pointed to address `0`, which is unsafe.

Reason:
- Spurious/unexpected vectors hitting a present gate at address `0` can cascade into fault storms and possible triple fault behavior.
- This is a targeted hardening fix for VMware instability and needs runtime validation in VMware.

### 23. Fixed RTC link failure in ISO build

Files:
- `Makefile`

Changes:
- Added RTC driver compilation/linking:
  - `src/impl/drivers/rtc.c`
  - `build/x86_64/rtc.o`
- Added explicit build rule for `rtc.o`.

Reason:
- Kernel linked with unresolved `get_time` after clock integration in shell.

### 24. Removed invalid BIOS video interrupts from long-mode runtime

Files:
- `src/impl/graphics/vga_graphics.c`
- `src/impl/x86_64/boot.asm`

Changes:
- Stopped invoking BIOS `int 0x10` from 64-bit C runtime.
- Forced VGA mode setup in 32-bit boot stage (`set_vga_mode13h`) before entering long mode.
- Made `vesa_set_mode(...)` return failure in long mode path.

Reason:
- BIOS services are not callable from long mode and were a crash vector on VMware.

### 25. Fixed multiboot video-mode mismatch causing black screen

File:
- `src/impl/x86_64/boot.asm`

Changes:
- Removed multiboot `VIDEO_MODE` flag request from header (`0x07` -> `0x03`).
- Updated multiboot checksum and neutralized now-unused graphics request fields.

Reason:
- Boot requested VBE 32-bit mode while shell rendered to VGA mode 13h memory (`0xA0000`), producing a black display.

### 26. Switched text rendering to real font table

Files:
- `src/impl/graphics/vga_graphics.c`
- `Makefile`
- `src/impl/graphics/font.c`

Changes:
- `draw_char()` now uses shared `font_8x8` glyph table from `font.c`.
- Added `font.o` to build/link graph.

Reason:
- Replaced inconsistent inline glyph rendering with stable ASCII font output.

### 27. Hardened IRQ call ABI and compiler settings for interrupt safety

Files:
- `src/impl/kernel_mode/hal/interrupts/isr.asm`
- `Makefile`

Changes:
- Added ABI stack alignment around ISR/IRQ C handler calls (`sub rsp, 8` / `add rsp, 8`).
- Added `-mno-red-zone` to kernel CFLAGS.

Reason:
- Prevent stack corruption and red-zone clobbering in interrupt context.

### 28. Improved IRQ dispatch ordering and click-path safety

Files:
- `src/impl/kernel_mode/hal/interrupts/isr.c`
- `src/impl/kernel/main.c`

Changes:
- Moved `pic_eoi(...)` to end of IRQ handling and added IRQ vector guard.
- Avoided window-order mutation while iterating click hit-tests.

Reason:
- Reduced re-entrant IRQ pressure and in-loop state mutation hazards under drag/click workloads.

### 29. Added polling-only stability mode during runtime

File:
- `src/impl/kernel/main.c`

Changes:
- Disabled IRQ-driven runtime input handling via `cli` after initialization.
- Continued using `mouse_poll()` / `keyboard_poll()` in main loop.

Reason:
- Temporary VMware stability mode to avoid drag-induced fault storms while preserving interactivity.

### 30. Desktop shell redesign toward compact XP look

File:
- `src/impl/kernel/main.c`

Changes:
- Removed desktop “My PC” icon.
- Reduced icon/taskbar/button sizing.
- Added custom Notepad icon drawing.
- Added compact text renderer for labels/taskbar.
- Centered and scaled desktop/taskbar labels.
- Improved Start menu layout and visual hierarchy.
- Added startup splash/progress animation.
- Added basic z-order bring-to-front behavior.

Reason:
- Requested UX cleanup and closer XP-like visual behavior in constrained 320x200 mode.

### 31. Added About app with version/build/latest-changes info

File:
- `src/impl/kernel/main.c`

Changes:
- Added `WIN_ABOUT` app window.
- Added desktop and Start menu launch entry for About.
- Added about content panel with:
  - Version `00m1`
  - Build `1.100`
  - latest changes and system details.

Reason:
- Added in-OS version/build/change visibility requested for release identity.

### 32. Implemented Start menu Shut Down action without VMware CPU-disabled popup path

File:
- `src/impl/kernel/main.c`

Changes:
- Added `shutdown_os()` routine.
- Wired Start menu “Shut Down” click region to shutdown routine.
- Attempted VM poweroff through common ACPI/APM ports.
- Replaced prior `cli + hlt` shutdown loop with non-crashing shutdown-screen state.

Reason:
- Requested shutdown action while avoiding the explicit CPU-disabled popup behavior.

### 33. Fixed keyboard/mouse cross-talk and mouse jump decoding

Files:
- `src/impl/drivers/keyboard.c`
- `src/impl/drivers/mouse.c`

Changes:
- Keyboard ISR now ignores auxiliary (mouse) bytes on controller status bit.
- Mouse decoder now drops overflow packets and uses correct signed PS/2 delta interpretation.

Reason:
- Eliminated “typing by itself” artifacts and cursor teleporting behavior.

### 34. Added storage-backed filesystem integration for desktop shell

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

### 35. Added File Explorer app and upgraded Settings to dedicated configurable app

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

## Validation Performed

### Syntax validation across all C files in `src`

Command pattern used:

```powershell
gcc -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Isrc/intf -fsyntax-only <file.c>
```

Result:
- **No syntax errors remaining** across `src/*.c` after iterative fix waves.

## Environment Blocker (Not Code)

Full `make` build is still blocked because cross-toolchain binaries are missing:

- Missing: `x86_64-linux-gnu-gcc` (and related cross-compile toolchain components).

So this pass guarantees local source-level syntax consistency, but not full link/boot validation yet.

## Files Touched In This Pass

- `src/intf/stdint.h`
- `src/intf/graphics.h`
- `src/intf/ui.h`
- `src/intf/mouse.h`
- `src/impl/graphics/vga_graphics.c`
- `src/executive/gdi/gdi.c`
- `src/impl/ui_system/ui.c`
- `src/impl/ui_system/window.c`
- `src/impl/ui_system/ui_widgets.c`
- `src/impl/drivers/mouse.c`
- `src/impl/kernel_mode/hal/interrupts/isr.asm`
- `src/impl/kernel_mode/microkernel/ipc.c`
- `src/impl/kernel_mode/microkernel/memory.c`
- `src/executive/object_manager/object_manager.c`
- `src/user_mode/user_mode.c`
- `src/user_mode/integral_subsystems/workstation/window_manager.c`
- `src/impl/kernel/main.c`
- `src/executive/executive.c`
- `src/user_mode/integral_subsystems/workstation/ui_framework.c`
- `src/impl/kernel_mode/hal/interrupts/isr.c`
- `src/executive/io_manager/io_manager.c`
- `src/executive/io_manager/io_manager.h`
- `src/impl/kernel_mode/hal/interrupts/idt.c`
- `src/impl/x86_64/boot.asm`
- `src/impl/graphics/font.c`
- `src/impl/drivers/keyboard.c`
- `Makefile`
- `src/intf/ports.h`
- `src/intf/fs.h`
- `src/impl/filesystem/fs.c`

## Notes

- This pass was intentionally compatibility-first to get `src` compiling cleanly with minimal behavioral churn.
- A future cleanup can remove compatibility wrappers and migrate all callers to a single modern graphics API.
- VMware runtime stability has improved but still requires continued validation under drag/click heavy interaction.
