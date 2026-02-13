# GamerOS Bug Tracking List

## Summary
- **Total Bugs Found:** 25
- **Resolved:** 19
- **Critical Issues:** 1
- **High Priority:** 1
- **Medium Priority:** 0
- **Low Priority:** 8

## Latest Updates (2026-02-13)
- Marked **Incomplete executive services initialization** as resolved after wiring executive startup of GDI, keyboard/mouse drivers, and baseline FS files in `src/executive/executive.c`.
- Marked **Notepad text editing limitations** as resolved after adding arrow/home/end cursor movement support in `src/impl/kernel/main.c`.
- Marked **Incomplete UI framework implementation** as resolved after adding queued event processing, input polling, and render flow in `src/user_mode/integral_subsystems/workstation/ui_framework.c`.
- Added additional IDT hardening for VMware by ensuring unset vectors are **not-present** gates in `src/impl/kernel_mode/hal/interrupts/idt.c` (validation pending in VMware).
- Marked **I/O manager uses synchronous operations only** as resolved after adding queued async request processing in `src/executive/io_manager/io_manager.c`.

## Critical Issues (System Breaking)

### 🔴 CRITICAL: Triple Fault / CPU Disabled in VMware
- **Location:** [`src/impl/kernel_mode/hal/interrupts/isr.c`](src/impl/kernel_mode/hal/interrupts/isr.c:1)
- **Issue:** VMware shows "CPU has been disabled by the guest operating system"
- **Root Cause:** Interrupt handling issues - possible causes:
  1. ISR assembly handler `sti` instruction re-enabling interrupts before `iretq`
  2. Stack misalignment in interrupt handlers
  3. IDT entry IST field was non-zero (now fixed to 0)
  4. GDT reload in 64-bit mode without proper segment register updates
- **Status:** PARTIALLY FIXED
  - IST field now set to 0
  - GDT reload removed (boot GDT is used)
  - Still investigating remaining issues
- **Workaround:** Kernel boots in QEMU but not VMware

## Recently Resolved Issues

### ✅ RESOLVED: IST Field in IDT Entry
- **Location:** [`src/intf/idt.h`](src/intf/idt.h:10)
- **Issue:** IDT entry `reserved` field was actually IST (Interrupt Stack Table), non-zero values cause triple fault
- **Fix:** Renamed to `ist` and explicitly set to 0
- **Status:** RESOLVED

### ✅ RESOLVED: GDT Reload in 64-bit Mode
- **Location:** [`src/impl/kernel_mode/hal/cpu/gdt.c`](src/impl/kernel_mode/hal/cpu/gdt.c:6)
- **Issue:** Reloading GDT in 64-bit mode without reloading segment registers causes corruption
- **Fix:** Made `gdt_init()` a no-op since boot.asm already sets up correct GDT
- **Status:** RESOLVED

### ✅ RESOLVED: ISR Handler Pass-by-Value
- **Location:** [`src/impl/kernel_mode/hal/interrupts/isr.c`](src/impl/kernel_mode/hal/interrupts/isr.c:51)
- **Issue:** `common_isr_handler(registers_t regs)` passed 152-byte struct by value, corrupting stack
- **Fix:** Changed to `common_isr_handler(uint64_t* regs)` - pass by pointer
- **Status:** RESOLVED

### ✅ RESOLVED: Double Buffering Implementation
- **Location:** [`src/impl/graphics/vga_graphics.c`](src/impl/graphics/vga_graphics.c:11)
- **Issue:** Screen flickering and cursor trails
- **Fix:** Implemented 800x600 back buffer, all drawing to back buffer, `swap_buffers()` copies to VGA
- **Status:** RESOLVED

### ✅ RESOLVED: Mouse Button Detection
- **Location:** [`src/impl/drivers/mouse.c`](src/impl/drivers/mouse.c:81)
- **Issue:** Mouse buttons not properly detected
- **Fix:** Proper PS/2 packet decoding with button state in lower 3 bits
- **Status:** RESOLVED

### ✅ RESOLVED: Window Dragging
- **Location:** [`src/impl/kernel/main.c`](src/impl/kernel/main.c:258)
- **Issue:** Window dragging was jerky/non-functional
- **Fix:** Implemented proper drag state tracking with `drag_x`, `drag_y` offsets
- **Status:** RESOLVED

### ✅ RESOLVED: Keyboard Input Buffer
- **Location:** [`src/impl/drivers/keyboard.c`](src/impl/drivers/keyboard.c:9)
- **Issue:** No keyboard input buffering
- **Fix:** Added 64-character ring buffer with `keyboard_getchar()` and `keyboard_has_input()`
- **Status:** RESOLVED

### ✅ RESOLVED: VGA Mode 13h Initialization
- **Location:** [`src/impl/graphics/vga_graphics.c`](src/impl/graphics/vga_graphics.c:46)
- **Issue:** Dark screen, palette not initialized
- **Fix:** Proper BIOS INT 0x10 mode setting and palette initialization with XP blue color
- **Status:** RESOLVED

## High Priority (Major Functionality Impact)

- [ ] **CRITICAL: Triple fault in VMware**
  - **Location:** Interrupt handling system
  - **Impact:** Cannot run in VMware, only QEMU
  - **Status:** Under investigation (ISR return path + exception path hardened; IDT null-vector present-bit bug fixed, VMware re-test required)

- [x] **VESA mode functions incomplete**
  - **Location:** [`src/impl/graphics/vga_graphics.c`](src/impl/graphics/vga_graphics.c:64)
  - **Impact:** Cannot use 640x480 or 800x600 modes (fallback to 320x200 works)
  - **Status:** RESOLVED (back-buffer copy path corrected to use active framebuffer dimensions)
  - **Note:** `vesa_set_mode()` uses INT 0x10 but may not work in all environments

- [x] **IPC system is completely stubbed out**
  - **Location:** [`src/impl/kernel_mode/microkernel/ipc.c`](src/impl/kernel_mode/microkernel/ipc.c:1)
  - **Impact:** No inter-process communication possible
  - **Status:** RESOLVED (basic in-kernel message queue implemented)

- [x] **Incomplete executive services initialization**
  - **Location:** [`src/executive/executive.c`](src/executive/executive.c:17)
  - **Impact:** Missing critical OS services
  - **Status:** RESOLVED (GDI init, input driver registration, and baseline FS objects initialized)

## Medium Priority (Feature Limitations)

- [x] **VESA mode display output incomplete**
  - **Location:** [`src/impl/graphics/vga_graphics.c`](src/impl/graphics/vga_graphics.c:286)
  - **Impact:** `swap_buffers()` only copies up to 320x200 for VESA modes
  - **Status:** RESOLVED

- [x] **Notepad text editing limitations**
  - **Location:** [`src/impl/kernel/main.c`](src/impl/kernel/main.c:292)
  - **Impact:** No cursor movement with arrow keys, limited editing features
  - **Status:** RESOLVED (arrow/home/end cursor movement implemented)

- [x] **No real-time clock implementation**
  - **Location:** [`src/impl/kernel/main.c`](src/impl/kernel/main.c:200)
  - **Impact:** Taskbar shows static "12:00" time
  - **Status:** RESOLVED (taskbar clock now reads RTC)

- [x] **Start menu non-functional**
  - **Location:** [`src/impl/kernel/main.c`](src/impl/kernel/main.c:179)
  - **Impact:** Start button draws but menu doesn't open
  - **Status:** RESOLVED (toggle + item click handling implemented)

- [x] **Memory leak in kfree - only coalesces with next block**
  - **Location:** [`src/impl/kernel_mode/microkernel/memory.c`](src/impl/kernel_mode/microkernel/memory.c:58)
  - **Impact:** Memory fragmentation over time
  - **Status:** RESOLVED (coalesce with both next and previous blocks)

- [x] **Object manager uses static pool instead of kmalloc**
  - **Location:** [`src/executive/object_manager/object_manager.c`](src/executive/object_manager/object_manager.c:80)
  - **Impact:** Limited to 4096 bytes total for all objects
  - **Status:** RESOLVED (switched to kmalloc/kfree)

- [x] **User mode subsystems are commented out/not initialized**
  - **Location:** [`src/user_mode/user_mode.c`](src/user_mode/user_mode.c:27)
  - **Impact:** No user mode functionality available
  - **Status:** RESOLVED (subsystem init/shutdown paths enabled)

- [x] **Incomplete UI framework implementation**
  - **Location:** [`src/user_mode/integral_subsystems/workstation/ui_framework.c`](src/user_mode/integral_subsystems/workstation/ui_framework.c:30)
  - **Impact:** Broken UI event handling and rendering
  - **Status:** RESOLVED (event queue, input polling, desktop render flow, cursor visibility handling)

- [x] **Window manager incomplete**
  - **Location:** [`src/user_mode/integral_subsystems/workstation/window_manager.c`](src/user_mode/integral_subsystems/workstation/window_manager.c:24)
  - **Impact:** Window operations (minimize, maximize) not implemented
  - **Status:** RESOLVED (window operations and hit-testing implemented)

## Low Priority (Minor Issues)

- [ ] **Code style inconsistencies across files**
  - **Impact:** Code readability
  - **Status:** Open

- [ ] **Missing documentation comments**
  - **Impact:** Code maintainability
  - **Status:** Open

- [ ] **Hard-coded magic numbers**
  - **Location:** Multiple files (320, 200, 0xA0000, etc.)
  - **Impact:** Code maintainability
  - **Status:** Open

- [ ] **Font rendering uses simple 8x8 font**
  - **Location:** [`src/impl/graphics/vga_graphics.c`](src/impl/graphics/vga_graphics.c:178)
  - **Impact:** Limited character support, only basic ASCII
  - **Status:** Open

- [ ] **Compatibility layers (MSDOS, Windows9x) are stubs**
  - **Location:** [`src/user_mode/compatibility_layers/`](src/user_mode/compatibility_layers/)
  - **Impact:** No backward compatibility
  - **Status:** Open

- [ ] **Environment subsystems (Win32, POSIX, OS/2) are stubs**
  - **Location:** [`src/user_mode/environment_subsystems/`](src/user_mode/environment_subsystems/)
  - **Impact:** No application compatibility layers
  - **Status:** Open

- [ ] **Server service and security subsystems are stubs**
  - **Location:** [`src/user_mode/integral_subsystems/`](src/user_mode/integral_subsystems/)
  - **Impact:** No network services or security
  - **Status:** Open

- [ ] **Filesystem operations incomplete**
  - **Location:** [`src/executive/filesystem_manager/filesystem_manager.c`](src/executive/filesystem_manager/filesystem_manager.c:1)
  - **Impact:** Cannot save/load Notepad files
  - **Status:** Open

- [x] **I/O manager uses synchronous operations only**
  - **Location:** [`src/executive/io_manager/io_manager.c`](src/executive/io_manager/io_manager.c:115)
  - **Impact:** No async I/O support
  - **Status:** RESOLVED (queued async requests + explicit processing function implemented)

## Working Components (Verified)

### ✅ Graphics System
- VGA Mode 13h (320x200x256) initialization
- Double buffering with back buffer
- Drawing primitives: pixel, rect, line, text
- XP color palette with desktop blue

### ✅ Mouse Input
- PS/2 mouse detection and initialization
- 3-byte packet decoding
- X/Y coordinate tracking with bounds clamping
- Left/middle/right button detection
- Cursor rendering with arrow shape

### ✅ Keyboard Input
- PS/2 keyboard scancode reading
- US layout with shift key support
- 64-character ring buffer
- Extended keys (arrows, home, end)

### ✅ Window System
- Window creation and destruction
- Title bar rendering
- Close buttons
- Window dragging by title bar
- Active window highlighting
- Taskbar integration

### ✅ Notepad Application
- Text input and display
- Backspace support
- Enter/return for new lines
- Cursor rendering
- Multi-line text buffer (20 lines x 40 chars)

### ✅ Desktop Environment
- XP-style blue background
- Desktop icons (Notepad, My Computer)
- Taskbar with Start button
- Window list in taskbar

## Build System

### ✅ Working
- WSL-based cross-compilation
- `x86_64-linux-gnu-gcc` and `x86_64-linux-gnu-ld`
- ISO generation with GRUB
- 15 object files compile successfully

---

*Last Updated: February 13, 2026*
