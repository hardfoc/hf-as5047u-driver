# Examples

Complete ESP32 applications that exercise the driver on real hardware, one feature area each.

They build for an ESP32-S3 with the sensor on SPI2; the pins are in
[`esp32_as5047u_test_config.hpp`](../examples/esp32/main/esp32_as5047u_test_config.hpp) and the port in
[`esp32_as5047u_bus.hpp`](../examples/esp32/main/esp32_as5047u_bus.hpp). CI builds the apps marked "yes" on each
pull request.

## The apps

<!-- generated: examples -->
| App | What it shows | Source | CI |
| --- | --- | --- | --- |
| `driver_integration_test` :material-star:{ title="featured" } | Comprehensive driver integration test suite (hardware required for full validation) | [`driver_integration_test.cpp`](../examples/esp32/main/driver_integration_test.cpp) | yes |
| `basic_angle_reading` :material-star:{ title="featured" } | Basic angle reading example with diagnostics | [`basic_angle_reading_example.cpp`](../examples/esp32/main/basic_angle_reading_example.cpp) | yes |
| `velocity_reading` :material-star:{ title="featured" } | Velocity reading example with unit conversions | [`velocity_reading_example.cpp`](../examples/esp32/main/velocity_reading_example.cpp) | yes |
| `configuration_example` :material-star:{ title="featured" } | Configuration example with ABI, UVW, zero position, and direction setup | [`configuration_example.cpp`](../examples/esp32/main/configuration_example.cpp) | yes |
| `full_features` :material-star:{ title="featured" } | Comprehensive example demonstrating core AS5047U features | [`full_features_example.cpp`](../examples/esp32/main/full_features_example.cpp) | yes |

Build and flash one (from `examples/esp32`, ESP-IDF environment loaded):

```bash
./scripts/build_app.sh driver_integration_test Release
./scripts/flash_app.sh flash_monitor driver_integration_test Release
```

Source of this table: [`examples/esp32/app_config.yml`](../examples/esp32/app_config.yml).
<!-- /generated -->

## Set up once

```bash
git submodule update --init examples/esp32/scripts   # the HardFOC ESP-IDF build scripts
cd examples/esp32
. $IDF_PATH/export.sh                                 # ESP-IDF v5.5
./scripts/build_app.sh list                           # every app and build type
```

The scripts read [`app_config.yml`](../examples/esp32/app_config.yml) (the same file this page's table comes
from) and build each app as its own firmware image.

## What to read in an example

| You want to | Read |
| --- | --- |
| Read angle, AGC and magnitude in a loop | `basic_angle_reading_example.cpp` |
| Read velocity in °/s, rad/s and rpm | `velocity_reading_example.cpp` |
| Set up ABI, UVW, zero position and direction | `configuration_example.cpp` |
| See every feature in one program | `full_features_example.cpp` |
| Run the whole API against a sensor, with pass / fail per test | `driver_integration_test.cpp` |

The [example folder's README](../examples/esp32/README.md) has the wiring and the expected serial output.
