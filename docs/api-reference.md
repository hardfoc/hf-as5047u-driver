# API Reference

Complete reference documentation for all public methods and types in the AS5047U driver.

## Source Code

- **Main Header**: [`inc/as5047u.hpp`](../inc/as5047u.hpp)
- **SPI Interface**: [`inc/as5047u_spi_interface.hpp`](../inc/as5047u_spi_interface.hpp)
- **Implementation**: [`src/as5047u.ipp`](../src/as5047u.ipp)
- **Registers**: [`inc/as5047u_registers.hpp`](../inc/as5047u_registers.hpp)

## Core Class

### `AS5047U<SpiType>`

Main driver class for interfacing with the AS5047U magnetic encoder.

**Template Parameter**: `SpiType` - Type implementing `as5047u::SpiInterface<SpiType>`

**Location**: [`inc/as5047u.hpp#L78`](../inc/as5047u.hpp#L78)

**Constructor:**
```cpp
explicit AS5047U(SpiType& bus, FrameFormat format = AS5047U_CFG::DEFAULT_FRAME_FORMAT) noexcept;
```

**Location**: [`inc/as5047u.hpp#L91`](../inc/as5047u.hpp#L91)

## Methods

### Frame Format

| Method | Signature | Location |
|--------|-----------|----------|
| `SetFrameFormat()` | `void SetFrameFormat(FrameFormat format) noexcept` | [`src/as5047u.ipp#L16`](../src/as5047u.ipp#L16) |

### Angle Reading

| Method | Signature | Location |
|--------|-----------|----------|
| `GetAngle(AngleUnit)` | `float GetAngle(AngleUnit unit, uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp`](../src/as5047u.ipp) |
| `GetAngle()` | `uint16_t GetAngle(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp#L33`](../src/as5047u.ipp#L33) |
| `GetRawAngle()` | `uint16_t GetRawAngle(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp#L48`](../src/as5047u.ipp#L48) |

### Velocity Reading

| Method | Signature | Location |
|--------|-----------|----------|
| `GetVelocity(VelocityUnit)` | `float GetVelocity(VelocityUnit unit, uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp`](../src/as5047u.ipp) |
| `GetVelocity()` | `int16_t GetVelocity(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp#L63`](../src/as5047u.ipp#L63) |
| `GetVelocityDegPerSec()` | `float GetVelocityDegPerSec(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp#L79`](../src/as5047u.ipp#L79) |
| `GetVelocityRadPerSec()` | `float GetVelocityRadPerSec(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp#L84`](../src/as5047u.ipp#L84) |
| `GetVelocityRPM()` | `float GetVelocityRPM(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp#L89`](../src/as5047u.ipp#L89) |

### Diagnostics

| Method | Signature | Location |
|--------|-----------|----------|
| `GetAGC()` | `uint8_t GetAGC(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp#L94`](../src/as5047u.ipp#L94) |
| `GetMagnitude()` | `uint16_t GetMagnitude(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp#L109`](../src/as5047u.ipp#L109) |
| `GetErrorFlags()` | `uint16_t GetErrorFlags(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp#L124`](../src/as5047u.ipp#L124) |
| `GetStickyErrorFlags()` | `AS5047U_Error GetStickyErrorFlags() const` | [`inc/as5047u.hpp#L384`](../inc/as5047u.hpp#L384) |
| `DumpStatus()` | `void DumpStatus() const` | [`src/as5047u.ipp#L597`](../src/as5047u.ipp#L597) |
| `GetDiagnostics()` | `AS5047U_REG::DIA GetDiagnostics() const` | [`src/as5047u.ipp#L393`](../src/as5047u.ipp#L393) |

### Configuration

| Method | Signature | Location |
|--------|-----------|----------|
| `GetZeroPosition()` | `uint16_t GetZeroPosition(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp#L135`](../src/as5047u.ipp#L135) |
| `SetZeroPosition()` | `bool SetZeroPosition(uint16_t angle_lsb, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp#L162`](../src/as5047u.ipp#L162) |
| `SetDirection()` | `bool SetDirection(bool clockwise, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`inc/as5047u.hpp#L214`](../inc/as5047u.hpp#L214) |
| `SetABIResolution()` | `bool SetABIResolution(uint8_t resolution_bits, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp#L171`](../src/as5047u.ipp#L171) |
| `SetUVWPolePairs()` | `bool SetUVWPolePairs(uint8_t pairs, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp#L179`](../src/as5047u.ipp#L179) |
| `SetIndexPulseLength()` | `bool SetIndexPulseLength(uint8_t lsb_len, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp#L187`](../src/as5047u.ipp#L187) |
| `ConfigureInterface()` | `bool ConfigureInterface(bool abi, bool uvw, bool pwm, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp#L205`](../src/as5047u.ipp#L205) |
| `SetDynamicAngleCompensation()` | `bool SetDynamicAngleCompensation(bool enable, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp#L225`](../src/as5047u.ipp#L225) |
| `SetAdaptiveFilter()` | `bool SetAdaptiveFilter(bool enable, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp`](../src/as5047u.ipp) |
| `SetFilterPreset()` | `bool SetFilterPreset(FilterPreset preset, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp`](../src/as5047u.ipp) |
| `SetFilterParameters()` | `bool SetFilterParameters(uint8_t k_min, uint8_t k_max, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp`](../src/as5047u.ipp) |
| `GetAdaptiveFilterEnabled()` | `bool GetAdaptiveFilterEnabled(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp`](../src/as5047u.ipp) |
| `GetFilterParameters()` | `std::pair<uint8_t, uint8_t> GetFilterParameters(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const` | [`src/as5047u.ipp`](../src/as5047u.ipp) |
| `Set150CTemperatureMode()` | `bool Set150CTemperatureMode(bool enable, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp#L250`](../src/as5047u.ipp#L250) |
| `SetHysteresis()` | `bool SetHysteresis(AS5047U_REG::SETTINGS3::Hysteresis hysteresis, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp#L315`](../src/as5047u.ipp#L315) |
| `GetHysteresis()` | `AS5047U_REG::SETTINGS3::Hysteresis GetHysteresis() const` | [`src/as5047u.ipp#L322`](../src/as5047u.ipp#L322) |
| `SetAngleOutputSource()` | `bool SetAngleOutputSource(AS5047U_REG::SETTINGS2::AngleOutputSource source, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`src/as5047u.ipp#L333`](../src/as5047u.ipp#L333) |
| `GetAngleOutputSource()` | `AS5047U_REG::SETTINGS2::AngleOutputSource GetAngleOutputSource() const` | [`src/as5047u.ipp#L340`](../src/as5047u.ipp#L340) |

### OTP Programming

| Method | Signature | Location |
|--------|-----------|----------|
| `ProgramOTP()` | `bool ProgramOTP()` | [`src/as5047u.ipp#L257`](../src/as5047u.ipp#L257) |

### Utility

| Method | Signature | Location |
|--------|-----------|----------|
| `SetPad()` | `void SetPad(uint8_t pad) noexcept` | [`src/as5047u.ipp#L302`](../src/as5047u.ipp#L302) |
| `ComputeCRC8()` | `static constexpr uint8_t ComputeCRC8(uint16_t data16)` | [`inc/as5047u.hpp#L96`](../inc/as5047u.hpp#L96) |
| `ReadReg()` | `template<typename RegT> RegT ReadReg() const` | [`inc/as5047u.hpp#L360`](../inc/as5047u.hpp#L360) |
| `WriteReg()` | `template<typename RegT> bool WriteReg(const RegT& reg, uint8_t retries = AS5047U_CFG::CRC_RETRIES)` | [`inc/as5047u.hpp#L376`](../inc/as5047u.hpp#L376) |

## Types

### Enumerations

| Type | Values | Location |
|------|--------|----------|
| `FrameFormat` | `SPI_16`, `SPI_24`, `SPI_32` | [`inc/as5047u_types.hpp#L15`](../inc/as5047u_types.hpp#L15) |
| `AngleUnit` | `Lsb`, `Degrees`, `Radians` | [`inc/as5047u.hpp`](../inc/as5047u.hpp) |
| `VelocityUnit` | `Lsb`, `DegPerSec`, `RadPerSec`, `Rpm` | [`inc/as5047u.hpp`](../inc/as5047u.hpp) |
| `AS5047U_Error` | `None`, `AgcWarning`, `MagHalf`, `P2ramWarning`, `P2ramError`, `FramingError`, `CommandError`, `CrcError`, `WatchdogError`, `OffCompError`, `CordicOverflow` | [`inc/as5047u.hpp#L32`](../inc/as5047u.hpp#L32) |

### Structures

| Type | Description | Location |
|------|-------------|----------|
| `AS5047U_REG::DIA` | Diagnostic register structure | [`inc/as5047u_registers.hpp`](../inc/as5047u_registers.hpp) |
| `AS5047U_REG::SETTINGS2::AngleOutputSource` | Angle output source enumeration | [`inc/as5047u_registers.hpp`](../inc/as5047u_registers.hpp) |
| `AS5047U_REG::SETTINGS3::Hysteresis` | Hysteresis enumeration | [`inc/as5047u_registers.hpp`](../inc/as5047u_registers.hpp) |

---

**Navigation**
⬅️ [Configuration](configuration.md) | [Next: Examples ➡️](examples.md) | [Back to Index](index.md)
