---
layout: default
title: "HardFOC AS5047U Driver"
description: "Portable C++20 driver for the AS5047U magnetic encoder from ams with SPI interface, CRC protection, DAEC, and DFS™"
nav_order: 1
permalink: /
---

# HF-AS5047U Driver

**Portable C++20 driver for the AS5047U magnetic encoder from ams with SPI interface, CRC protection, DAEC, and DFS™**

[![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![CI](https://github.com/hardfoc/hf-as5047u-driver/actions/workflows/esp32-examples-build-ci.yml/badge.svg?branch=main)](https://github.com/hardfoc/hf-as5047u-driver/actions/workflows/esp32-examples-build-ci.yml)
[![Docs](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://hardfoc.github.io/hf-as5047u-driver/)

## 📚 Table of Contents
1. [Overview](#-overview)
2. [Features](#-features)
3. [Quick Start](#-quick-start)
4. [Installation](#-installation)
5. [API Reference](#-api-reference)
6. [Examples](#-examples)
7. [Documentation](#-documentation)
8. [References](#-references)
9. [Contributing](#-contributing)
10. [License](#-license)

## 📦 Overview

> **📖 [📚🌐 Live Complete Documentation](https://hardfoc.github.io/hf-as5047u-driver/)** - 
> Interactive guides, examples, and step-by-step tutorials

**HF-AS5047U** is a portable C++20 driver for the **AS5047U** magnetic encoder from ams. It delivers fast 14‑bit absolute angle readings over SPI, optional CRC protection and advanced features like Dynamic Angle Error Compensation (DAEC) and an adaptive Dynamic Filter System (DFS™). The sensor can also output incremental (A/B/I) and 3‑phase commutation (UVW) signals or a PWM encoded angle, making it a drop‑in replacement for optical encoders in high-performance motor control and robotics.

The driver uses a CRTP-based `SpiInterface` for hardware abstraction, allowing it to run on any platform (ESP32, STM32, Arduino, etc.) with zero runtime overhead. It implements all major features from the AS5047U datasheet including absolute angle readout with/without DAEC, velocity measurement, AGC and magnetic field diagnostics, ABI/UVW/PWM interface configuration, error/status flag handling, and full OTP programming sequence.

![AS5047U magnetic encoder interfaces — SPI / ABI / UVW / PWM](docs/assets/as5047u-interfaces.svg)

## ✨ Features

- ✅ **14-bit absolute angle** with optional CRC check
- ✅ **DAEC** (Dynamic Angle Error Compensation) for low-latency angle correction
- ✅ **DFS™** (Dynamic Filter System) adaptive noise filtering
- ✅ **Stray-field immunity** for robust operation
- ✅ **Incremental ABI outputs** up to 4096 PPR
- ✅ **UVW commutation** with programmable pole pairs
- ✅ **PWM output** option
- ✅ **Diagnostics** for AGC, magnitude and error flags
- ✅ **OTP memory** for permanent configuration
- ✅ **Hardware Agnostic**: SPI interface for platform independence
- ✅ **Modern C++**: C++20 with CRTP-based design
- ✅ **Zero Overhead**: CRTP-based design for compile-time polymorphism

## 🚀 Quick Start

```cpp
#include "as5047u.hpp"

// 1. Implement the SPI interface (see platform_integration.md)
class MySpi : public as5047u::SpiInterface<MySpi> {
public:
    void transfer(const uint8_t* tx, uint8_t* rx, size_t len) {
        // Your SPI transfer implementation
    }
};

// 2. Create driver instance
MySpi spi;
as5047u::AS5047U encoder(spi, FrameFormat::SPI_24); // 24-bit frames with CRC

// 3. Read angle / velocity (unit-selected API)
float angle_deg = encoder.GetAngle(as5047u::AngleUnit::Degrees);      // 0..360
float angle_rad = encoder.GetAngle(as5047u::AngleUnit::Radians);      // 0..2π
float vel_rpm = encoder.GetVelocity(as5047u::VelocityUnit::Rpm);
float vel_dps = encoder.GetVelocity(as5047u::VelocityUnit::DegPerSec);

// 4. Diagnostics
uint8_t agc = encoder.GetAGC();            // automatic gain control
uint16_t mag = encoder.GetMagnitude();     // magnetic magnitude
uint16_t errors = encoder.GetErrorFlags(); // current error flags
```

For detailed setup, see [Installation](docs/installation.md) and [Quick Start Guide](docs/quickstart.md).

## 🔧 Installation

1. **Clone or copy** the driver files into your project
2. **Implement the SPI interface** for your platform (see [Platform Integration](docs/platform_integration.md))
3. **Include the header** in your code:
   ```cpp
   #include "as5047u.hpp"
   ```
4. Compile with a **C++20** or newer compiler

For detailed installation instructions, see [docs/installation.md](docs/installation.md).

## 📖 API Reference

| Method | Description |
|--------|-------------|
| `GetAngle(AngleUnit)` | Read absolute angle in selected unit (LSB/deg/rad) |
| `GetVelocity(VelocityUnit)` | Read velocity in selected unit (LSB/deg/s/rad/s/RPM) |
| `GetAngle()` | Read 14-bit compensated absolute angle |
| `GetRawAngle()` | Read 14-bit raw absolute angle |
| `GetVelocity()` | Read signed 14-bit velocity (LSB units) |
| `GetVelocityDegPerSec()` | Velocity in degrees/sec |
| `GetVelocityRPM()` | Velocity in revolutions per minute |
| `GetAGC()` | Read AGC (0–255) value |
| `GetMagnitude()` | Read magnetic field magnitude |
| `GetErrorFlags()` | Read and clear error/status flags |
| `SetZeroPosition()` | Set new zero offset |
| `SetDirection()` | Set rotation direction (CW or CCW) |
| `SetABIResolution()` | Set ABI output resolution (10–14 bits) |
| `SetUVWPolePairs()` | Set UVW pole pairs (1–7) |
| `ConfigureInterface()` | Enable/disable ABI, UVW, PWM |
| `SetDynamicAngleCompensation()` | Enable/disable DAEC |
| `SetAdaptiveFilter()` | Enable/disable adaptive filter (DFS) |
| `SetFilterPreset()` | Set filter from preset (LowNoise, Balanced, HighBandwidth) |
| `GetAdaptiveFilterEnabled()` | Read whether adaptive filter is enabled |
| `GetFilterParameters()` | Read current K_min and K_max register codes |
| `ProgramOTP()` | Program current settings into OTP |

For complete API documentation, see [docs/api_reference.md](docs/api_reference.md).

Compatibility note: the unit-specific helpers (`GetAngleDegrees()`, `GetVelocityRPM()`, etc.) remain available for existing code, but new code should prefer the unit-enum overloads.

## 📊 Examples

For ESP32 examples and integration tests, see the [examples/esp32](examples/esp32/) directory.

Detailed example walkthroughs are available in [docs/examples.md](docs/examples.md).

## 📚 Documentation

For complete documentation, see the [docs directory](docs/index.md).

## 🔗 References

| Resource | Link |
|----------|------|
| ams AS5047U product page | <https://ams.com/as5047u> |
| AS5047U datasheet (ams) | <https://ams.com/documents/20143/36005/AS5047U_DS000637_2-00.pdf> |
| DAEC application note | <https://ams.com/documents/20143/36005/AS5047_AN000288_1-00.pdf> |
| ESP-IDF SPI master | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/spi_master.html> |
| C++20 language reference | <https://en.cppreference.com/w/cpp/20> |

## 🤝 Contributing

Pull requests and suggestions are welcome! Please follow the existing code style and include tests for new features.

## 📄 License

This project is licensed under the **GNU General Public License v3.0**.
See the [LICENSE](LICENSE) file for details.

