# GamerOS Changelog (2026-02-17)

This changelog tracks work for the next release cycle.

## Release
- Version: `00m1`
- Build: `1.300`
- Date: `2026-02-17`

## Added
- New application framework module at `src/apps` with centralized built-in app registry.
- Per-app source folders under `src/apps` for modular app growth:
  - `src/apps/notepad`
  - `src/apps/settings`
  - `src/apps/explorer`
- New source-side app manifests now stored inside each app folder:
  - `src/apps/notepad/NOTEPAD.EXE.manifest`
  - `src/apps/settings/SETTINGS.EXE.manifest`
  - `src/apps/explorer/EXPLORER.EXE.manifest`
- App UI content modules added inside each app folder:
  - `src/apps/notepad/notepad_ui.c`
  - `src/apps/settings/settings_ui.c`
  - `src/apps/explorer/explorer_ui.c`
- New public app-launch interface `src/intf/apps.h` for executable resolution.

## Changed
- Build metadata updated from `1.200` to `1.300`.
- Build system now compiles and links `src/apps/apps.c`.
- Desktop and Start menu launch flow now routes through `.EXE` app descriptors (`NOTEPAD.EXE`, `SETTINGS.EXE`, `EXPLORER.EXE`).
- System32 app records switched from `.APP` entries to `.EXE` stubs.
- Storage initialization now seeds app paths:
  - `C:/GamerOS/Apps`
  - `C:/GamerOS/Apps/BuiltIn`
- UI scaling groundwork added with runtime scale profile and adaptive Start menu metrics.
- Settings UI now shows live runtime resolution and scale profile.
- Startup animation motion updated with eased progress timing for smoother feel.
- Taskbar visual style updated to a two-tone bar for a more polished desktop look.
- Desktop watermark draw order updated so it appears behind app windows/icons instead of over app content.
- Notepad UI refreshed with menu-strip hint and stronger status bar treatment.
- Settings UI refreshed with modernized content header and app-module-driven tab/changelog content.
- Explorer UI refreshed with clearer two-pane layout (left quick access + right content panel).
- Start menu metrics path simplified/hardened for VMware stability.
- Explorer app restored from safe-static mode to interactive storage-backed navigation.
- Settings navigation expanded to GamerOS-style categories:
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
- Changelog view moved under `GamerOS Update` (Windows-style naming removed; now GamerOS branding).
- All built-in app windows now use a unified modernized Windows XP-style theme (title bars, borders, panels, and controls).
- Explorer storage initialization moved off immediate app-open path (now deferred to first drive interaction).
- Desktop watermark text updated to new GamerOS build identity wording.
- Start menu expanded with additional entries:
  - GamerOS Update
  - About GamerOS
- Settings `GamerOS Update` page now mirrors markdown-style structure and item content from `changelog_2026-02-17.md`.
- Settings `GamerOS Update` now uses markdown-aware rendering (headings, bullet lists, nested bullets, code-style lines).
- Changelog content now lives in a markdown file in GamerOS storage: `C:/GamerOS/Apps/Settings/CHANGELOG_2026-02-17.MD` (seeded at init).
- Added source markdown changelog file under app module: `src/apps/settings/changelog.md`.
- Applied Windows 7-inspired global shell theme refresh with modernized visuals:
  - Aero-like desktop color bands
  - glossy dark taskbar treatment
  - updated Start menu shell styling
  - refined window chrome across built-in apps
- Startup/loading screen fully remade with a Windows 7-inspired presentation:
  - Aero-like background bands
  - centered startup panel
  - refined progress rail with smoother eased animation
  - moving highlight sweep across progress fill
- Desktop now uses a dedicated procedural wallpaper renderer (Windows 7-inspired Aero style) that scales to runtime resolution.
- Small smoothness/performance tuning:
  - faster startup animation timing profile
  - tiny idle-frame pause to reduce VM jitter on unchanged frames
- Integrated custom wallpaper asset from PNG into source/runtime path:
  - source image: `src/resources/wallpapers/Background.png`
  - compiled asset: `src/resources/wallpapers/background_wallpaper.c/.h`
  - desktop now renders this wallpaper image as live background
- Taskbar clock now shows both time and date in a single compact display (`HH:MM[:SS] DD/MM/YYYY`).

## Fixed
- Notepad launch path now consistently loads storage-backed document content when launched via `.EXE` entry flow.
- Fixed Settings content/header overlap so changelog and section content are readable.
- Fixed VMware crash path seen when opening Start menu in latest UI pass.
- Fixed Explorer popup regression in VMware when opening Explorer.
- Improved Notepad typing responsiveness under fast key bursts by expanding keyboard buffer and avoiding newest-key drops.
- Fixed taskbar app-button reservation/hitbox alignment after clock width increase for date display.

## Notes
- This release introduces executable-style app descriptors and app-folder organization as the base for broader `.EXE` app support.
- Explorer now supports `This PC` drive open, folder traversal, and parent navigation using current storage model.
