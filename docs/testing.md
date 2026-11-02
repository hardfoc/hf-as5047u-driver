# Testing and CI

How the driver is checked: host unit tests against a model of the chip, compiled documentation, ESP32 builds and
static analysis, on every pull request.

## Host unit tests

The tests build the real driver against a model of the AS5047U SPI slave
([`tests/mock_as5047u.hpp`](../tests/mock_as5047u.hpp)): the frame length selects the format, the reply to a
command arrives during the next frame, write data frames return the old and then the new content, wrong CRCs set
ERRFL, and ERRFL clears when read. Every MOSI frame is logged, so tests check the exact bytes on the bus, and the
model can fail a transfer, corrupt a reply on MISO or set the reply's warning and error bits on demand, to check
that bus and reply errors reach the caller.

```bash
cmake -S . -B build-host -DHF_AS5047U_BUILD_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

| File | Covers |
| --- | --- |
| [`tests/test_frames.cpp`](../tests/test_frames.cpp) | CRC-8 against an independent implementation for every input; the bytes of 16-, 24- and 32-bit reads; writes promoted to 24 bit; reply CRC checks and the reply warning and error bits |
| [`tests/test_driver.cpp`](../tests/test_driver.cpp) | Velocity sign and scale, zero position split, ABIRES codes, outputs, direction, filter presets, write verification, sticky flags, angle output source, bus-error propagation, `void` legacy ports |
| [`tests/mock_as5047u.hpp`](../tests/mock_as5047u.hpp) | The chip model |
| [`tests/test_harness.hpp`](../tests/test_harness.hpp) | A dependency-free test registry and `CHECK` macros |

## Documentation snippets

Every C++ block on this site is included from [`tests/docs/`](../tests/docs/my_spi.hpp), which the host build
compiles (target `hf_as5047u_doc_snippets`) with the driver explicitly instantiated over the documented port, so
every member of `AS5047U<>` is compiled. A change to the API that breaks a page breaks the build first.

## CI workflows

| Workflow | What it checks |
| --- | --- |
| Host unit tests | GCC and Clang build and run the tests and the doc snippets |
| ESP32 examples build | The apps in `examples/esp32/app_config.yml` with `ci_enabled`, for each listed ESP-IDF version and build type |
| C++ lint, C++ analysis | clang-format, clang-tidy and cppcheck |
| Docs | Strict site build, figure and generated-table drift, Doxygen, link check, Lighthouse budget |
| Markdown lint, YAML lint, link check | Repository text files |

The workflows call the shared, versioned workflows of
[hf-general-ci-tools](https://github.com/hardfoc/hf-general-ci-tools) and
[hf-espidf-ci-tools](https://github.com/hardfoc/hf-espidf-ci-tools).

## On hardware

The [ESP32 examples](examples.md) are the hardware test suite: `driver_integration_test` runs the API against a
connected sensor and prints a pass / fail summary over the serial monitor.
