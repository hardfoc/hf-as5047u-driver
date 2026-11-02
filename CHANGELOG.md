# Changelog

All notable changes to this driver are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Host unit tests (`-DHF_AS5047U_BUILD_TESTS=ON`) against a model of the AS5047U SPI slave, and a CI workflow.

### Changed

- **Breaking (port contract):** the SPI port's `transfer()` returns `bool` (`true` = frame clocked, `false` = bus
  error) and every driver path propagates it: setters, `WriteReg()` and `ProgramOTP()` return `false`, a
  read-modify-write setter whose read fails writes nothing, and every failed frame sets the new
  `AS5047U_Error::BusError` sticky flag. A `void` `transfer()` still compiles and is treated as never failing; any
  other return type is rejected by `static_assert`. New `ReadReg(reg, retries)` overload returns whether the read
  worked. Datasheet "SPI Transaction", Figures 29 and 30: a read or write is a fixed sequence of frames, so a frame
  that was not clocked leaves the access incomplete.
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

- The write path no longer prints: a failed write verification used to `printf` a line from inside the driver.
  The cause is in the sticky flags, which the write path already fills from ERRFL. `DumpStatus()`, which exists to
  print, is unchanged.
- The non-standard `M_PI` is replaced by the constant `as5047u::kPi`, so the header builds where `M_PI` is not
  defined (MSVC without `_USE_MATH_DEFINES`, strict-ISO newlib); `<cmath>` is no longer included.
- `ProgramOTP()` follows the datasheet's step-by-step OTP procedure exactly (DS000637 page 41-42, Figure 60): it now
  clears the volatile registers before the guard-band test (step 15, which was skipped, so the guard-band
  comparison could pass on values still in the volatile registers), writes the PROG values of Figure 60 (`0x01`,
  `0x08`, `0x40`, `0x04`; it wrote `0x09`, `0x49`, `0x4D`, `0x49`, which set PROGOTP again after the burn),
  compares DISABLE..ECC (0x0015..0x001B) instead of 0x0016..0x001A, checks the result of every step and stops at
  the first failure (it ignored every write result and could burn after a failed write). New optional
  `OtpResult*` argument reports the failed step, whether the burn started and the burned image; new
  `VerifyOTP()` performs the power-on check of steps 21 and 22.
- The CRC of each 24- and 32-bit reply the driver uses is checked: a corrupted reply fails the access (retried
  when `retries` allows; a failed read returns 0) and sets the new `ReplyCrcError` sticky flag, instead of being
  returned as data. The warning and error bits of the replies are recorded as the new `ReplyWarning` and
  `ReplyError` sticky flags. Datasheet "SPI Transaction" Figures 22, 25 and 28 (MISO data frames) and "CRC
  Checksum" with Figure 31.
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
