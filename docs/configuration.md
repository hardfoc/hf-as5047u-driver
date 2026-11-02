# Configuration

Every setting of the AS5047U, the call that changes it, its register and reset value, and two complete setups.

All settings live in the non-volatile register block (0x0015 to 0x001B). Writes go to the volatile copy and are
lost at power-off unless you burn them ([OTP programming](otp.md)). A chip that was never programmed starts with
every register at `0x0000`, which gives the defaults below. Every snippet here is compiled from
[`tests/docs/configuration.cpp`](../tests/docs/configuration.cpp).

## Compile-time defaults

Define these for the whole build (for example `target_compile_definitions(my_app PRIVATE CONFIG_AS5047U_FRAME_24)`,
or in your `sdkconfig.h`); [`inc/as5047u_config.hpp`](../inc/as5047u_config.hpp) reads them.

| Define | Effect | Without it |
| --- | --- | --- |
| `CONFIG_AS5047U_FRAME_24` | The constructor's default format is `SPI_24` | `SPI_16` |
| `CONFIG_AS5047U_FRAME_32` | The constructor's default format is `SPI_32` | `SPI_16` |
| `CONFIG_AS5047U_CRC_RETRIES=n` | Default `retries` argument of every call | `0` |

The repository has no Kconfig file of its own; an ESP-IDF project that wants a menu entry defines these symbols in
its own `Kconfig.projbuild`.

## Settings

| Setting (field) | Call | Reset value |
| --- | --- | --- |
| Zero position (`ZPOSM`, `ZPOSL`) | `SetZeroPosition()` | 0 |
| Direction (`SETTINGS2.DIR`) | `SetDirection()` | Clockwise counts up |
| DAEC (`SETTINGS2.DAECDIS`) | `SetDynamicAngleCompensation()` | On |
| What 0x3FFF returns (`SETTINGS2.Data_select`) | `SetAngleOutputSource()` | ANGLECOM |
| Adaptive filter (`DISABLE.FILTER_disable`) | `SetAdaptiveFilter()` | On |
| Filter coefficients (`SETTINGS1.K_min`, `K_max`) | `SetFilterPreset()` | K = 2 to 6 |
| 150 °C in 3.3 V mode (`SETTINGS2.NOISESET`) | `Set150CTemperatureMode()` | Off: up to 125 °C |
| ABI, UVW, PWM (`DISABLE`, `SETTINGS2`) | `ConfigureInterface()` | PWM off |
| ABI resolution (`SETTINGS3.ABIRES`) | `SetABIResolution()` | 1024 pulses |
| Index pulse width (`SETTINGS2.IWIDTH`) | `SetIndexPulseLength()` | 3 LSB |
| UVW pole pairs (`SETTINGS3.UVWPP`) | `SetUVWPolePairs()` | 1 |
| ABI hysteresis (`SETTINGS3.HYS`) | `SetHysteresis()` | 1 LSB (0.17°) |

DAEC is the dynamic angle error compensation; `SetFilterParameters()` sets the filter codes directly. ABI
and UVW are on after reset. Directions are seen from the top of the chip. The ABI decimal mode (`SETTINGS2.ABI_DEC`) has no setter; see
below.

### Filter presets

The adaptive filter picks a coefficient K between K_min and K_max depending on acceleration. Low K is less noise
and more latency.

| `FilterPreset` | K_min, K_max | Velocity noise (Figure 17) | Corner frequency |
| --- | --- | --- | --- |
| `LowNoise` | 0, 0 | 5.8 °/s rms | 48 Hz |
| `Balanced` | 2, 3 | 8.4 to 19.8 °/s rms | 194 to 387 Hz |
| `HighBandwidth` | 6, 6 | 244.9 °/s rms | 3095 Hz |

`SetFilterParameters()` takes the 3-bit register codes, not K: the mapping is not linear (Figure 13: K_min code
`000` is K = 2, `101` is K = 0; K_max code `000` is K = 6, `110` is K = 0). The presets write the right codes.

### ABI resolution

| `SetABIResolution()` | Steps per turn | Pulses per turn | ABIRES |
| --- | --- | --- | --- |
| 14 | 16384 | 4096 | `100` |
| 13 | 8192 | 2048 | `011` |
| 12 (default) | 4096 | 1024 | `000` |
| 11 | 2048 | 512 | `001` |
| 10 | 1024 | 256 | `010` |

With `ABI_DEC` = 1 the same codes give decimal counts from 1000 down to 25 pulses per turn; set it with
`WriteReg()` ([API guide](api-guide.md#use-any-register-directly)).

### Interface combinations

`ConfigureInterface(abi, uvw, pwm)`:

| `abi` | `uvw` | `pwm` | Outputs |
| --- | --- | --- | --- |
| true | false | false | A, B, I |
| true | false | true | A, B, I; PWM on W |
| false | true | false | U, V, W |
| false | true | true | U, V, W; PWM on I |
| true | true | any | A, B, I and U, V, W; `UVW_ABI` = 0, so PWM (if set) takes the W pin |
| false | false | true | PWM on W |
| false | false | false | ABI and UVW tristated |

## Setups

### Motor commutation and speed control

```cpp
--8<-- "tests/docs/configuration.cpp:commutation"
```

### Slow or static positioning

```cpp
--8<-- "tests/docs/configuration.cpp:positioning"
```

The datasheet recommends turning DAEC off under about 100 rpm: it removes 0.016° rms of noise and the latency it
compensates does not matter at low speed.

Next: [OTP programming](otp.md) to make a setup permanent.
