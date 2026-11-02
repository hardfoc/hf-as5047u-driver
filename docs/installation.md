# Installation

Add the driver to a CMake or ESP-IDF project. It is a header-only C++20 library with no dependencies.

## Requirements

- A C++20 compiler: GCC 10 or later, Clang 12 or later. The host CI builds with GCC and Clang on Ubuntu 24.04.
- No RTOS, heap or exceptions. The only output is `DumpStatus()`, which prints through `printf` when you call it.

## Get the source

A git submodule keeps the driver pinned to a version you chose:

```bash
git submodule add https://github.com/hardfoc/hf-as5047u-driver.git external/hf-as5047u-driver
git submodule update --init external/hf-as5047u-driver
```

Only `inc/`, `src/`, `cmake/` and `CMakeLists.txt` are needed to build. The examples have their own submodule
(`examples/esp32/scripts`); you do not need it to use the driver.

## CMake

```cmake
add_subdirectory(external/hf-as5047u-driver)
target_link_libraries(my_app PRIVATE hf_as5047u)   # or the alias hf::as5047u
```

`hf_as5047u` is an `INTERFACE` target: it carries the include directories (including the generated version
header) and requires C++20 (`cxx_std_20`), so your target does not need to set either.

An installed copy works the same way. `cmake --install` puts the headers, the template implementation
`as5047u.ipp` and the generated `as5047u_version.h` in one include directory and exports the package
`hf_as5047u`:

```cmake
find_package(hf_as5047u CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE hf::as5047u)
```

| Option | Default | Effect |
| --- | --- | --- |
| `HF_AS5047U_ENABLE_WARNINGS` | `OFF` | `-Wall -Wextra -Wpedantic` for targets that link the driver |
| `HF_AS5047U_BUILD_TESTS` | `OFF` | Builds the host unit tests in `tests/` ([Testing and CI](testing.md)) |

## ESP-IDF

The ESP-IDF component wrapper is [`examples/esp32/components/hf_as5047u/`](../examples/esp32/components/hf_as5047u/CMakeLists.txt).
Copy that folder into your project's `components/` (or point `EXTRA_COMPONENT_DIRS` at it) and set the
`HF_AS5047U_ROOT` cache variable to the driver checkout; the wrapper stops with a clear message when the path is
wrong. Your component then declares `REQUIRES hf_as5047u`. The wrapper itself requires `driver` and `freertos`.

## How the build is organized

| Layer | File | Role |
| --- | --- | --- |
| Settings | [`cmake/hf_as5047u_build_settings.cmake`](../cmake/hf_as5047u_build_settings.cmake) | Target name, include directories, version: the single source of truth |
| Library | [`CMakeLists.txt`](../CMakeLists.txt) | Interface library, install and package export |
| ESP-IDF | [`examples/esp32/components/hf_as5047u/CMakeLists.txt`](../examples/esp32/components/hf_as5047u/CMakeLists.txt) | IDF component wrapper over the same settings |

The version is set once in the settings file and written into `as5047u_version.h` at configure time. A build
that does not go through CMake must generate that header from
[`inc/as5047u_version.h.in`](../inc/as5047u_version.h.in) itself. Read the version at run time:

```cpp
const char* v = as5047u::AS5047U<MySpi>::GetDriverVersion();   // "1.0.0"
```

## Compile-time switches

| Define | Effect |
| --- | --- |
| `CONFIG_AS5047U_FRAME_24`, `CONFIG_AS5047U_FRAME_32` | Default frame format of the constructor (otherwise `SPI_16`) |
| `CONFIG_AS5047U_CRC_RETRIES=n` | Default number of retries of every call (otherwise 0) |

## Next

[Quickstart](quickstart.md) wires the sensor and reads an angle; [Porting](porting.md) writes the SPI interface
for your MCU.
