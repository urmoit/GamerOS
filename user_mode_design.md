# User Mode Layer Design Document

## Overview

The User Mode layer sits atop the Executive Layer and provides the final interface for user applications. It consists of three main categories of subsystems: Integral Subsystems, Environment Subsystems, and Compatibility Layers.

## Architecture

### Layer Hierarchy
```
User Applications
    ↓
User Mode Layer
    ↓
Executive Layer
    ↓
Kernel Mode
```

### Component Categories

#### 1. Integral Subsystems
Core services that provide essential functionality to the OS:

- **Workstation**: Manages the desktop environment, windowing system, and user interface framework
- **Server Service**: Provides network services and server functionality
- **Security**: Handles user authentication, access control, and security policies

#### 2. Environment Subsystems
Provide compatibility with different operating system environments:

- **Win32**: Windows 32-bit API compatibility layer
- **POSIX**: POSIX-compliant API implementation
- **OS/2**: OS/2 API compatibility layer

#### 3. Compatibility Layers
Provide backward compatibility with legacy systems:

- **Windows 9x**: Windows 9x compatibility mode
- **MS-DOS**: MS-DOS compatibility and emulation

## Directory Structure

```
src/user_mode/
├── integral_subsystems/
│   ├── workstation/
│   │   ├── desktop_manager.c/h
│   │   ├── window_manager.c/h
│   │   ├── ui_framework.c/h
│   │   └── widget_system.c/h
│   ├── server_service/
│   │   ├── network_server.c/h
│   │   └── service_manager.c/h
│   └── security/
│       ├── authentication.c/h
│       ├── access_control.c/h
│       └── security_policy.c/h
├── environment_subsystems/
│   ├── win32/
│   │   ├── win32_api.c/h
│   │   └── win32_subsystem.c/h
│   ├── posix/
│   │   ├── posix_api.c/h
│   │   └── posix_subsystem.c/h
│   └── os2/
│       ├── os2_api.c/h
│       └── os2_subsystem.c/h
├── compatibility_layers/
│   ├── windows9x/
│   │   ├── win9x_compat.c/h
│   │   └── win9x_emulator.c/h
│   └── msdos/
│       ├── dos_compat.c/h
│       └── dos_emulator.c/h
└── interfaces/
    ├── user_mode.h
    └── subsystem_interfaces.h
```

## Subsystem Details

### Workstation Subsystem

**Purpose**: Provides the desktop environment and user interface framework.

**Components**:
- **Desktop Manager**: Manages desktop layout, wallpaper, icons, and desktop widgets
- **Window Manager**: Handles window creation, positioning, z-order, and window operations
- **UI Framework**: Provides the core UI rendering and event handling system
- **Widget System**: Implements UI widgets (buttons, textboxes, menus, etc.)

**Refactoring Source**: Current `src/impl/ui_system/` components

### Server Service Subsystem

**Purpose**: Provides network services and server functionality.

**Components**:
- **Network Server**: Handles network protocols and server operations
- **Service Manager**: Manages system services and their lifecycle

### Security Subsystem

**Purpose**: Manages security features and access control.

**Components**:
- **Authentication**: User login, password verification, and session management
- **Access Control**: File permissions, resource access control
- **Security Policy**: Security policies and enforcement

### Environment Subsystems

**Purpose**: Provide API compatibility for different operating environments.

Each environment subsystem implements the standard APIs for its respective platform:
- Win32: Windows API functions
- POSIX: Standard POSIX system calls
- OS/2: OS/2 API functions

### Compatibility Layers

**Purpose**: Provide backward compatibility with legacy systems.

- **Windows 9x**: Emulates Windows 9x environment for older applications
- **MS-DOS**: Provides DOS compatibility mode and command interpreter

## API Design

### User Mode Interface

The main interface (`user_mode.h`) provides:
- Initialization functions for subsystems
- Application entry points
- System call interfaces
- Environment selection

### Subsystem Interfaces

Each subsystem exposes a well-defined interface:
- Initialization/cleanup functions
- Core functionality APIs
- Event handling
- Configuration management

## Implementation Strategy

1. **Phase 1**: Create directory structure and interfaces
2. **Phase 2**: Implement Workstation subsystem (refactor existing UI code)
3. **Phase 3**: Implement stub versions of other subsystems
4. **Phase 4**: Integrate with Executive Layer APIs
5. **Phase 5**: Add full functionality to remaining subsystems

## Dependencies

- **Executive Layer**: All User Mode components depend on Executive Layer services
- **GDI**: Graphics rendering for UI components
- **IO Manager**: Input/output operations
- **Process Manager**: Process and thread management
- **Security Manager**: Low-level security services

## Future Extensions

- Additional environment subsystems (Linux, macOS compatibility)
- Enhanced compatibility layers
- Plugin architecture for third-party subsystems
- Advanced desktop features (virtual desktops, themes, etc.)