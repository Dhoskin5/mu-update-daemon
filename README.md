# mu-update-daemon

A lightweight D-Bus controlled update daemon for embedded Linux systems.

## Overview
This daemon listens on D-Bus (`org.mu.Update`) and securely applies updates after receiving verified triggers.

## Build Instructions
- Use `gdbus-codegen` to generate IPC code from `ipc/org.mu.Update.xml`.
- Implement core logic in `src/main.c`.


## Requirements
- GLib >= 2.60
- GIO >= 2.60
- CMake >= 3.10
- GCC or Clang


## License
This project is licensed under the MIT License.
