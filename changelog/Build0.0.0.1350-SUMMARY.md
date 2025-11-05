This build focuses on improving the developer experience by introducing a new set of Windows-native scripts for building, running, and debugging the OS. The project structure has been simplified by removing obsolete files and consolidating documentation into the main `README.md` file.

**Key changes:**
-   **New Setup Scripts:** Introduced `build.bat`, `clean.bat`, `run-qemu.bat`, `run-vmware.bat`, and `debug.bat` for easier project management on Windows.
-   **Simplified Project Structure:** Removed several outdated markdown files and scripts.
-   **Improved Documentation:** The `README.md` file has been updated to reflect the new project structure and workflows.
-   **Enhanced Error Handling:** The new scripts include checks for dependencies like QEMU and VMware.