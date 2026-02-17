# GamerOS Applications

This folder contains built-in application metadata and launcher mapping for the GamerOS shell.

## Layout

- `apps.c`: Runtime app registry used by kernel shell launch flow.
- `notepad/`: Notepad app module files.
- `settings/`: Settings app module files.
- `explorer/`: File Explorer app module files.
- Each app folder now holds both system manifests and app-specific UI content code.

## Built-in Executables

- `NOTEPAD.EXE`
- `SETTINGS.EXE`
- `EXPLORER.EXE`

At boot, matching `.EXE` stubs are materialized into:

- `C:/GamerOS/System32`
- `C:/GamerOS/Apps/BuiltIn`

Each app now has its own folder so you can add more files per app over time without mixing modules.
