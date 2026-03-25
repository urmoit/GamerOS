# GamerOS File Structure Reorganization Plan

## Current Problems
1. **`/impl/` is too flat** - Contains drivers, filesystem, graphics, kernel, UI all mixed together
2. **Scattered components** - Executive managers are in different folders, kernel code split across locations
3. **App organization** - Apps folder has loose structure
4. **Hardware abstraction** - Driver code and device-specific code not clearly separated
5. **Build artifacts mixed with source** - No clear separation

## Proposed New Structure

```
GamerOS/
├── src/
│   ├── core/                      # Core OS kernel and executive
│   │   ├── kernel/                # Kernel implementation (currently impl/kernel/)
│   │   ├── executive/             # Executive layer (all managers)
│   │   │   ├── filesystem_manager/
│   │   │   ├── object_manager/
│   │   │   ├── process_manager/
│   │   │   ├── ipc_manager/
│   │   │   ├── security_manager/
│   │   │   ├── power_manager/
│   │   │   └── executive.c
│   │   └── hal/                   # Hardware abstraction layer
│   │       ├── cpu/
│   │       ├── memory/
│   │       └── interrupts/
│   │
│   ├── drivers/                   # All device drivers (from impl/drivers/)
│   │   ├── storage/               # ATA, USB storage
│   │   ├── input/                 # Keyboard, mouse
│   │   ├── network/               # VMware, etc.
│   │   ├── bus/                   # PCI, USB controllers
│   │   └── virtualization/        # VMware tools
│   │
│   ├── graphics/                  # Graphics subsystem (from impl/graphics/)
│   │   ├── vga/
│   │   ├── fonts/
│   │   ├── cursors/
│   │   └── graphics_api/
│   │
│   ├── ui/                        # UI system and applications
│   │   ├── system/                # Core UI system (from impl/ui_system/)
│   │   ├── widgets/               # UI widgets
│   │   └── apps/                  # Applications
│   │       ├── explorer/
│   │       ├── notepad/
│   │       ├── settings/
│   │       ├── about/
│   │       └── system/
│   │
│   ├── filesystem/                # Filesystem implementation (from impl/filesystem/)
│   │
│   ├── user-mode/                 # User mode subsystems
│   │   └── (contents from user_mode/)
│   │
│   ├── arch/                      # Architecture-specific code
│   │   └── x86_64/                # x86-64 specific (from impl/x86_64/)
│   │
│   └── include/                   # Public headers (rename from intf/)
│       └── (all .h files)
│
├── build/                         # Build artifacts
│   └── x86_64/
│       └── iso/
│
├── config/                        # Build and system config
│   ├── targets/                   # Linker scripts, GRUB config
│   └── buildenv/                  # Docker build environment
│
├── resources/                     # Media and resources
│   ├── cursors/
│   └── wallpapers/
│
├── docs/                          # Documentation
│   ├── user_mode_design.md
│   ├── executive_layer_design.md
│   ├── changelogs/
│   └── walkthroughs/
│
└── tools/                         # Build scripts and tools
    ├── build.bat
    ├── build-iso.bat
    ├── run-qemu.bat
    ├── clean.bat
    ├── convert_cursor.py
    └── Makefile files
```

## Migration Steps

### Phase 1: Create new structure
- [ ] Create new directories (all proposed folders above)
- [ ] Update all include paths in source code

### Phase 2: Move core components
- [ ] Move `src/impl/kernel/` → `src/core/kernel/`
- [ ] Move `src/executive/` → `src/core/executive/`
- [ ] Move HAL-related code to `src/core/hal/`

### Phase 3: Reorganize drivers
- [ ] Move `src/impl/drivers/` → `src/drivers/` with subcategories
- [ ] Categorize drivers by type (storage, input, bus, etc.)

### Phase 4: Reorganize graphics and UI
- [ ] Move `src/impl/graphics/` → `src/graphics/`
- [ ] Move `src/impl/ui_system/` → `src/ui/system/`
- [ ] Reorganize `src/apps/` → `src/ui/apps/`

### Phase 5: Reorganize utilities
- [ ] Move `src/intf/` → `src/include/`
- [ ] Move `targets/` → `config/targets/`
- [ ] Move `buildenv/` → `config/buildenv/`
- [ ] Create `tools/` and create build scripts

### Phase 6: Documentation and configuration
- [ ] Move `.md` files to `docs/`
- [ ] Update Makefiles with new paths
- [ ] Update `build.bat` and other scripts
- [ ] Update linker scripts with new paths

## Files Affected by Path Changes

These need updating:
- `Makefile` and `Makefile.wsl` - include paths
- `build.bat`, `build-iso.bat`, `clean.bat` - paths
- `debug.bat` - paths
- `run-qemu.bat` - paths
- All `#include` statements in `.c` and `.h` files
- `targets/x86_64/linker.ld` - object file paths
- `targets/x86_64/grub.cfg` - boot paths

## Benefits of This Organization

✅ **Clearer separation of concerns** - Drivers, graphics, UI are in separate top-level folders  
✅ **Easier navigation** - Similar code grouped together logically  
✅ **Scalability** - Room to add more subsystems without cluttering  
✅ **Standard OS structure** - Follows typical kernel project layout  
✅ **Better CI/CD** - Can build subsystems independently if needed  

## Risks & Mitigations

⚠️ **Risk**: Breaking the build  
→ **Mitigation**: Update all build configs and include paths carefully

⚠️ **Risk**: Losing track of file locations  
→ **Mitigation**: Create a file mapping document before moving

⚠️ **Risk**: Version control issues  
→ **Mitigation**: Do moves in systematic batches

---

**Next Steps**: Once you review this plan, I can:
1. Create a detailed file mapping document
2. Start implementation phase by phase
3. Update all build configurations
4. Update all include paths automatically
