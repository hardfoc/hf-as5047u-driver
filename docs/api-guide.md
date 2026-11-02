# API guide

How to do the common jobs with the driver: create it, read angle and velocity, check the field, configure,
set up the outputs, use any register and handle errors.

Every snippet on this page is compiled by the host CI from
[`tests/docs/api_tasks.cpp`](../tests/docs/api_tasks.cpp), with `Encoder` standing for
`as5047u::AS5047U<MySpi>`. The [API reference](api-reference.md) lists every member.

## The shape of the API

One `AS5047U<SpiType>` object per sensor. It offers three levels of access:

| Level | Example | Use it for |
| --- | --- | --- |
| Measurements | `GetAngleDegrees()`, `GetVelocityRPM()`, `GetAGC()` | Reading the sensor in a loop |
| Setters | `SetZeroPosition()`, `SetABIResolution()`, `ConfigureInterface()` | One setting each, read-modify-write, verified |
| Typed registers | `ReadReg<AS5047U_REG::SETTINGS2>()`, `WriteReg(s2)` | Any field a setter does not cover |

| Group | Members |
| --- | --- |
| Angle | `GetAngle()`, `GetAngle(AngleUnit)`, `GetAngleDegrees()`, `GetAngleRadians()`, `GetRawAngle()` |
| Velocity | `GetVelocity()`, `GetVelocity(VelocityUnit)`, `GetVelocityDegPerSec()`, `GetVelocityRadPerSec()`, `GetVelocityRPM()` |
| Field and status | `GetAGC()`, `GetMagnitude()`, `GetDiagnostics()`, `GetErrorFlags()`, `GetStickyErrorFlags()`, `DumpStatus()` |
| Angle settings | `SetZeroPosition()`, `GetZeroPosition()`, `SetDirection()`, `SetDynamicAngleCompensation()`, `SetAngleOutputSource()` |
| Filter | `SetFilterPreset()`, `SetAdaptiveFilter()`, `SetFilterParameters()`, `GetFilterParameters()`, `GetAdaptiveFilterEnabled()` |
| Outputs | `ConfigureInterface()`, `SetABIResolution()`, `SetIndexPulseLength()`, `SetUVWPolePairs()`, `SetHysteresis()` |
| Chip | `Set150CTemperatureMode()`, `ProgramOTP()` |
| Bus | `SetFrameFormat()`, `SetPad()`, `ComputeCRC8()` |

Setters return `bool`: `false` means the value did not read back as written. Measurements return the value; their
errors are reported through the error flags (below). Nothing throws and nothing allocates.

Every call that takes a `retries` argument (default `AS5047U_CFG::CRC_RETRIES`, 0) repeats the access that many
times when the chip reports a CRC or framing error.

## Create the driver

```cpp
--8<-- "tests/docs/api_tasks.cpp:construct"
```

The constructor stores the port reference and the frame format; it sends nothing. Use `SPI_24` unless you need
the highest read rate (`SPI_16`) or share the bus in a daisy chain (`SPI_32` with `SetPad()`).
[SPI protocol](protocol.md) explains the formats.

## Read angle and velocity

```cpp
--8<-- "tests/docs/api_tasks.cpp:read"
```

| Value | Register | Range | Unit |
| --- | --- | --- | --- |
| `GetAngle()` | ANGLECOM (0x3FFF) | 0 to 16383 | 360° / 16384 = 0.022° per LSB |
| `GetRawAngle()` | ANGLEUNC (0x3FFE) | 0 to 16383 | Same, without dynamic angle error compensation |
| `GetVelocity()` | VEL (0x3FFC) | −8192 to 8191 | 24.141 °/s per LSB (4.02 rpm per LSB) |

ANGLECOM includes the zero position and the direction. Address 0x3FFF returns ANGLEUNC instead when
`SetAngleOutputSource(UseANGLEUNC)` is set. The datasheet specifies the velocity for −28000 rpm to 28000 rpm with
a total error of ±5 %.

## Check the magnetic field

```cpp
--8<-- "tests/docs/api_tasks.cpp:diagnostics"
```

| Reading | Healthy | Problem |
| --- | --- | --- |
| `GetAGC()` | Between 0 and 255, not at either end | 0: field too strong (magnet too close). 255: field too weak |
| `GetMagnitude()` | Steady while the AGC is in range | Falls when the AGC is at 255 |
| `dia.bits.LoopsFinished` | 1 | 0: the chip has not finished start-up |
| `dia.bits.MagHalf_flag` | 0 | 1: magnitude below half its target |
| `dia.bits.Comp_l`, `Comp_h` | 0 | AGC at its low or high limit |

## Configure

```cpp
--8<-- "tests/docs/api_tasks.cpp:configure"
```

These settings live in volatile registers until you burn them ([OTP programming](otp.md)).
[Configuration](configuration.md) lists every setting with its register and default, and two complete setups.

## Set up the outputs

```cpp
--8<-- "tests/docs/api_tasks.cpp:outputs"
```

`SetABIResolution()` takes 10 to 14 bits (clamped) and writes the binary ABIRES code;
`SetUVWPolePairs()` takes 1 to 7 (clamped). For decimal pulse counts, use the register directly (next section).

## Use any register directly

```cpp
--8<-- "tests/docs/api_tasks.cpp:raw"
```

Each struct in `AS5047U_REG` has `ADDRESS`, a 16-bit `value` and named `bits`. `ReadReg<T>()` reads it (and
ERRFL); `WriteReg(reg, retries)` writes `value` and checks the read-back. The datasheet's register tables are in
its "Volatile Registers" and "Non-Volatile Registers (OTP)" sections.

## Handle errors

The chip reports errors in ERRFL (0x0001), which clears when it is read. Every `readRegister()` inside the driver
reads ERRFL after the register and ORs its bits into a set of **sticky flags**; `GetStickyErrorFlags()` returns
them and clears them.

```cpp
--8<-- "tests/docs/api_tasks.cpp:errors"
```

!!! warning "The measurement getters consume the sticky flags"
    `GetAngle()`, `GetVelocity()`, `GetAGC()`, `GetMagnitude()` and the other retrying getters call
    `GetStickyErrorFlags()` themselves to decide whether to retry, so the flags of that read are gone when the call
    returns. To act on the flags of a sample, read it with `ReadReg<>()` as above. `GetErrorFlags()` reads ERRFL
    from the chip, which only shows what happened since the driver's last read of it.

| Flag (`AS5047U_Error`) | ERRFL bit | Meaning | What to do |
| --- | --- | --- | --- |
| `AgcWarning` | 0 | AGC at 0 or 255 | Check the magnet distance; read DIA for which end |
| `MagHalf` | 1 | AGC at 255 and the magnitude below half its target | Magnet too far or missing |
| `P2ramWarning` | 2 | ECC corrected one bit of the OTP settings | Note it; the value is still right |
| `P2ramError` | 3 | ECC found an uncorrectable OTP error | Settings are not reliable |
| `FramingError` | 4 | The chip could not frame the SPI transfer (for example a wrong number of clocks) | Check CSn per frame and the port's `len` |
| `CommandError` | 5 | Invalid command | Check frame format and wiring |
| `CrcError` | 6 | CRC of a MOSI frame was wrong | Check SPI mode 1, clock, signal integrity |
| `WatchdogError` | 7 | Internal oscillator or watchdog fault | Power-cycle; replace the part if it persists |
| `OffCompError` | 9 | Offset compensation not finished | Normal right after power-on; wait 10 ms |
| `CordicOverflow` | 10 | CORDIC overflow | The angle is not valid; check the field |

After any flag the datasheet asks for a read of DIA (`GetDiagnostics()`) for the details.
[Troubleshooting](troubleshooting.md) maps symptoms to causes.

## Version

```cpp
const char* v = as5047u::AS5047U<MySpi>::GetDriverVersion();   // "1.0.0"
```
