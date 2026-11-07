# plat.c

OS abstraction layer library providing synchronization and multiprocessing primitives, helpful macros and utilities.

## Requirements

- `clang` in PATH
- `cmake` in PATH
- `ninja` in PATH
- `clang-format` in PATH

## Building

This project is configured and built with CMake.

```bash
cmake --preset Debug  # Configure the build, run once per preset
cmake --build -j $(nproc) --preset Debug  # Build for that preset
cmake --build -j $(nproc) --preset Debug -t plat  # Build different target, eg. plat-format
```
