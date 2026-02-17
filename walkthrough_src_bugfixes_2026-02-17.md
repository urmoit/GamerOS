# Walkthrough - `src` Bug Fix Pass (2026-02-17)

This document tracks the latest changes for build `1.300`.

## Fixes Applied

### 1. Initialized build 1.300 walkthrough

Files:
- `walkthrough_src_bugfixes_2026-02-17.md`

Changes:
- Created a new dated walkthrough file for the next update cycle.
- Set baseline note for build `1.300`.

Reason:
- Start tracking new implementation and bug-fix work separately from the 2026-02-15 log.

### 2. Added `src/apps` application framework and executable registry

Files:
- `src/intf/apps.h`
- `src/apps/apps.c`
- `src/apps/README.md`
- `src/apps/notepad/NOTEPAD.EXE.manifest`
- `src/apps/settings/SETTINGS.EXE.manifest`
- `src/apps/explorer/EXPLORER.EXE.manifest`
- `src/apps/notepad/README.md`
- `src/apps/settings/README.md`
- `src/apps/explorer/README.md`
- `Makefile`

Changes:
- Added a dedicated app framework folder under `src` for built-in application system organization.
- Introduced a centralized app registry (`apps.c`) that maps executable names to shell window entry points.
- Added app interface (`apps.h`) for executable lookup and launch resolution.
- Added per-app folders and source-side manifests for built-in executables (Notepad, Settings, Explorer).
- Wired the new module into the build graph by adding `apps.o` compile/link rules.

Reason:
- Required cleaner application architecture with a dedicated source folder and executable-oriented app metadata.

### 3. Switched shell app launch flow from direct window IDs to `.EXE` resolution

Files:
- `src/impl/kernel/main.c`

Changes:
- Added `launch_application_exe(...)` shell helper that resolves and launches built-in executables.
- Updated Start menu launch actions to use executable targets:
  - `NOTEPAD.EXE`
  - `SETTINGS.EXE`
  - `EXPLORER.EXE`
- Updated desktop icon launch actions to use executable targets via app registry.
- Updated storage seeding to create `.EXE` stubs under `C:/GamerOS/System32`.
- Added app-root storage directories:
  - `C:/GamerOS/Apps`
  - `C:/GamerOS/Apps/BuiltIn`

Reason:
- Needed Windows-style executable launch behavior and better structured built-in app bootstrapping.

### 4. Improved UI scalability and perceived responsiveness baseline

Files:
- `src/impl/kernel/main.c`

Changes:
- Added adaptive UI scale profile selection from runtime resolution.
- Added shared Start menu metric calculation for draw/input consistency across different screen sizes.
- Added dynamic desktop icon layout spacing based on UI scale profile.
- Updated Settings `Personalization`/`About` to display live runtime resolution and graphics metadata.
- Updated startup animation to eased timing for smoother visual progression.
- Refined taskbar rendering to a two-tone style for a more modern and polished look.
- Bumped in-OS build strings and watermark to build `1.300`.

Reason:
- Requested stronger UI support with better scaling behavior, smoother UX, and a cleaner modern presentation baseline.

### 5. Moved app-specific UI content into per-app folders and refreshed app visuals

Files:
- `src/apps/notepad/notepad_ui.h`
- `src/apps/notepad/notepad_ui.c`
- `src/apps/settings/settings_ui.h`
- `src/apps/settings/settings_ui.c`
- `src/apps/explorer/explorer_ui.h`
- `src/apps/explorer/explorer_ui.c`
- `src/impl/kernel/main.c`
- `Makefile`
- `src/apps/README.md`
- `src/apps/notepad/README.md`
- `src/apps/settings/README.md`
- `src/apps/explorer/README.md`
- `changelog_2026-02-17.md`

Changes:
- Added app-owned UI content modules so each built-in app keeps its own text/content in its own `src/apps/<app>` folder.
- Wired new app UI modules into the build graph.
- Replaced hardcoded Settings tabs/changelog strings in kernel shell with calls into `settings_ui.c`.
- Replaced Notepad status/toolbar strings with app-owned text from `notepad_ui.c`.
- Replaced Explorer safe-mode content strings with app-owned text from `explorer_ui.c`.
- Updated desktop draw order so watermark is rendered under app icons/windows.
- Refreshed in-window visuals:
  - Notepad: toolbar strip + improved status bar styling.
  - Settings: modern content header strip + module-driven tab/changelog content.
  - Explorer: clearer left navigation + right content pane layout.
- Updated in-Settings changelog content to the 2026-02-17 release summary.

Reason:
- Requested per-app folder ownership for future growth, better app-specific UI, and watermark placement beneath app content.

### 6. Hardened Start menu stability path and restored Explorer interactive storage mode

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-02-17.md`

Changes:
- Reworked Start menu metrics calculation to a fixed stable geometry path for VMware.
- Fixed Settings content/header overlap by introducing body layout offset under the top header strip.
- Restored Explorer from static safe-mode text to functional storage-backed interaction:
  - `This PC` drive list rendering
  - drive click-to-open
  - directory listing rendering
  - parent navigation (`[Up]`)
  - quick-access return to `This PC`
- Added Explorer storage initialization when Explorer window opens so filesystem data is available.
- Updated in-Settings changelog lines to include Start menu stability and Explorer restoration notes.

Reason:
- Requested fix for VMware CPU popup on Start menu open, better Settings UI readability, and fully working Explorer with storage support.

### 7. Expanded Settings categories and moved changelog under GamerOS Update

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-02-17.md`

Changes:
- Expanded Settings tab model to include full category list:
  - System
  - Bluetooth & devices
  - Network & internet
  - Personalization
  - Apps
  - Accounts
  - Time & language
  - Gaming
  - Accessibility
  - Privacy & security
  - GamerOS Update
  - About
- Removed standalone `Changelog` tab behavior.
- Moved release notes/changelog rendering under `GamerOS Update` section.
- Updated wheel scroll handling to scroll update notes in `GamerOS Update` tab.
- Ensured Settings naming and update labeling uses GamerOS branding.

Reason:
- Requested Windows-style Settings category breadth, but branded as GamerOS, with changelog moved under GamerOS Update.

### 8. Applied unified modern Windows XP-style theming across all built-in app windows

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-02-17.md`

Changes:
- Reworked shared window chrome for app windows with stronger XP-like visual hierarchy:
  - layered title bar treatment
  - beveled window frame/border depth
  - improved close button styling
- Updated Notepad window body with cleaner framed editor surface and XP-style toolbar/status bar integration.
- Updated Settings window body with clearer card-like navigation/content surfaces and stronger tab-state contrast.
- Updated Explorer window body with consistent XP-style panel framing and control treatment.
- Added corresponding note to GamerOS Update content in Settings.

Reason:
- Requested that every application use a modernized Windows XP visual theme consistently.

### 9. Fixed Explorer VMware popup path and improved Notepad fast-typing reliability

Files:
- `src/impl/kernel/main.c`
- `src/impl/drivers/keyboard.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-02-17.md`

Changes:
- Removed immediate storage initialization from Explorer window-open path.
- Explorer now initializes storage lazily when user interacts with drive view (`This PC`/drive open), reducing open-path risk in VMware.
- Increased keyboard ring buffer size from `64` to `256` for better burst-input handling.
- Changed keyboard buffer overflow behavior to drop oldest byte instead of newest, preserving current typing input.
- Updated in-Settings GamerOS Update notes and markdown changelog entries for these fixes.

Reason:
- Requested fix for Explorer popup in VMware and faster/more reliable Notepad typing/key registration.

### 10. Refreshed watermark text and expanded Start menu actions

Files:
- `src/impl/kernel/main.c`
- `src/apps/settings/settings_ui.c`
- `changelog_2026-02-17.md`

Changes:
- Updated desktop watermark copy to a clearer GamerOS build identity:
  - `GamerOS 00m1 XP Shell`
  - `Build 1.300 - Internal Preview`
- Expanded Start menu entries beyond app launch basics:
  - Notepad
  - Settings
  - File Explorer
  - GamerOS Update
  - About GamerOS
- Added Start menu action handlers for new entries:
  - `GamerOS Update` opens Settings directly on update tab
  - `About GamerOS` opens Settings directly on about tab
- Added matching note in in-OS GamerOS Update text.

Reason:
- Requested watermark info adjustment and new Start menu content.

### 11. Synced Settings GamerOS Update content to markdown changelog layout

Files:
- `src/apps/settings/settings_ui.c`
- `src/impl/kernel/main.c`
- `changelog_2026-02-17.md`

Changes:
- Replaced in-Settings update text block with markdown-style lines that mirror `changelog_2026-02-17.md` structure and items:
  - `#` title line
  - `##` section headings
  - bullet lists and nested bullet-style entries
- Removed extra custom `GamerOS Update` heading line in renderer so displayed layout matches markdown ordering.
- Kept wheel scroll behavior for long markdown content.

Reason:
- Requested same markdown-like layout and same items in Settings as `changelog_2026-02-17.md` without divergence.

### 12. Added markdown viewer support and file-backed changelog source in Settings

Files:
- `src/intf/fs.h`
- `src/apps/settings/settings_ui.h`
- `src/apps/settings/settings_ui.c`
- `src/impl/kernel/main.c`
- `changelog_2026-02-17.md`

Changes:
- Added markdown source API for Settings update page:
  - changelog markdown file path getter
  - changelog markdown text getter
- Added file seeding for changelog markdown during storage initialization:
  - `C:/GamerOS/Apps/Settings/CHANGELOG_2026-02-17.MD`
- Replaced old line-array rendering path with markdown-aware viewer in Settings:
  - heading parsing (`#`, `##`, `###`)
  - list parsing (`-`, nested `  -`, ordered list style)
  - basic code-fence support (` ``` ` blocks)
  - inline code marker normalization (backticks shown safely in viewer)
- Added markdown parse cache and scroll integration for `GamerOS Update` tab.
- Increased FS `MAX_FILE_SIZE` to support full changelog markdown content in storage.
- Moved large Notepad file IO buffer from stack to static storage for safety with larger file capacity.
- Relocated changelog runtime markdown path into apps storage scope:
  - `C:/GamerOS/Apps/Settings/CHANGELOG_2026-02-17.MD`
- Added app-owned source markdown file:
  - `src/apps/settings/changelog.md`

Reason:
- Requested full markdown format support and changelog content sourced from a markdown file, not divergent hardcoded items.

### 13. Applied Windows 7-style global UI theme pass with modern elements

Files:
- `src/impl/kernel/main.c`
- `changelog_2026-02-17.md`

Changes:
- Refreshed desktop shell visuals toward a Windows 7-inspired style:
  - multi-band Aero-like desktop backdrop
  - stronger desktop glow treatment near taskbar
  - glossy dark taskbar with top highlight line
- Updated Start menu visuals with a more modern shell treatment:
  - dark framed menu body
  - highlighted profile/header panes
  - beveled item rows and shutdown action bar
- Updated shared window chrome across apps:
  - stronger title bar highlight bands
  - refined frame borders and depth cues
- Updated desktop watermark copy to match the new UI direction.

Reason:
- Requested whole OS UI to look like Windows 7 with modern UI elements.

### 14. Remade startup loading screen to match Windows 7-style shell direction

Files:
- `src/impl/kernel/main.c`
- `changelog_2026-02-17.md`

Changes:
- Replaced previous startup animation implementation with a new visual sequence:
  - Aero-like multi-band background
  - centered framed startup panel
  - updated startup subtitle text
  - redesigned progress rail with smoother eased fill
  - moving highlight sweep over progress bar
- Tuned animation pacing for a more modern and polished boot feel.

Reason:
- Requested loading screen remake aligned with the new Windows 7-inspired UI direction.

### 15. Added scalable Aero wallpaper and small runtime smoothness tuning

Files:
- `src/impl/kernel/main.c`
- `changelog_2026-02-17.md`

Changes:
- Added dedicated desktop wallpaper renderer (`draw_desktop_wallpaper`) with a Windows 7-inspired Aero look.
- Wallpaper rendering is resolution-aware and uses current runtime dimensions.
- Reduced startup animation duration/timing for faster perceived boot.
- Added a tiny idle-frame pause in main loop when nothing changes to reduce VM jitter and improve smoothness.

Reason:
- Requested small tweaks for smoother/faster feel and addition of wallpaper.

### 16. Integrated external `Background.png` as live desktop wallpaper

Files:
- `src/resources/wallpapers/Background.png`
- `src/resources/wallpapers/background_wallpaper.h`
- `src/resources/wallpapers/background_wallpaper.c`
- `src/impl/kernel/main.c`
- `Makefile`
- `changelog_2026-02-17.md`

Changes:
- Moved user-provided wallpaper image into source tree under `src/resources/wallpapers`.
- Generated compiled 16-color wallpaper asset from PNG for kernel runtime rendering.
- Wired wallpaper asset source into build graph.
- Replaced procedural desktop wallpaper path with image-backed renderer using the new wallpaper asset.
- Kept scaling logic so wallpaper can render to current runtime desktop dimensions.

Reason:
- Requested using provided `Background.png` as actual OS desktop background.

### 17. Added taskbar date next to time and aligned taskbar layout math

Files:
- `src/impl/kernel/main.c`
- `changelog_2026-02-17.md`

Changes:
- Updated taskbar clock rendering to include date alongside time:
  - format: `HH:MM[:SS] DD/MM/YYYY`
- Added shared clock width helper so draw path and mouse hitbox path use the same reserved width.
- Updated task button right-limit calculation to prevent overlap with expanded clock area.
- Added matching release-note entries in changelog.

Reason:
- Requested showing date on the taskbar where time is displayed, without breaking taskbar app interactions.
