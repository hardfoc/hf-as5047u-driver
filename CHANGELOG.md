# Changelog

All notable changes to this driver are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Host unit tests (`-DHF_AS5047U_BUILD_TESTS=ON`) against a model of the AS5047U SPI slave, and a CI workflow.

### Changed

- **Breaking:** `GetStickyErrorFlags()` no longer clears the flags; the new `ClearStickyErrorFlags()` clears them
  and returns what it cleared. The measurement getters no longer consume the flags to decide on a retry (they use
  the ERRFL of their own read), so the flags of every sample stay visible until the caller clears them. Datasheet
  Figure 33: ERRFL clears when read, so the driver's cache is the only copy. Code that relied on a get to clear
  must call `ClearStickyErrorFlags()` instead.
- Documentation moved to the HardFOC MkDocs platform: new page set (quickstart, hardware, architecture, SPI
  protocol, OTP programming, API guide, porting, testing), code snippets compiled by the host build from
  `tests/docs/`, Doxygen under `/api/`, versioned with mike. The Jekyll site files and the doxygen-awesome
  submodule are gone.

### Fixed

- Every ERRFL value the driver reads reaches the sticky flags: `GetErrorFlags()` and `ReadReg<ERRFL>()` used to
  read ERRFL a second time for the sticky update, which returned 0 and dropped the flags (datasheet Figure 33).
  `GetErrorFlags()` no longer re-reads ERRFL until it reads 0.
- Every member of `AS5047U<>` compiles on current GCC and Clang: the `template` keyword is no longer used without a
  template argument list, and `GetFilterParameters()` no longer binds bit-fields to references.

## [1.0.0]

The version set in `cmake/hf_as5047u_build_settings.cmake`; not tagged yet.

### Added

- `AS5047U<SpiType>` driver over a CRTP SPI interface: angle, velocity, AGC, magnitude, diagnostics, 16-, 24- and
  32-bit frames with CRC, zero position, direction, DAEC, adaptive filter presets, ABI / UVW / PWM outputs,
  hysteresis, OTP programming, typed register access.
- ESP-IDF component wrapper and ESP32 example applications.
