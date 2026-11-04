# Troubleshooting

Symptoms you may see while bringing up an AS5047U, their causes and the fix for each, cheapest check first.

Start with `DumpStatus()`: it prints the angle, velocity, AGC, magnitude, ERRFL, every DIA bit and the settings
registers in one go, and tells transport problems (all zeros, all ones) apart from magnet problems (AGC at a limit).

## Every register reads 0 or 0x3FFF

| Cause | Check | Fix |
| --- | --- | --- |
| No supply, or TEST not grounded | 3.0 V to 3.6 V (3.3 V mode) or 4.5 V to 5.5 V (5 V mode) on VDD; TEST at GND | Fix the supply; ground TEST |
| MISO not connected, or floating | Probe MISO during a frame: it must toggle | Wire MISO; 0x3FFF everywhere means MISO stays high |
| Wrong SPI mode | Mode 1: CLK idles low, data sampled on the falling edge | Set mode 1; modes 0, 2 and 3 do not work |
| CSn held low across several frames | CSn must go high after each `transfer()` | One frame per chip-select period ([Porting](porting.md)) |
| 5 V mode with a 3.3 V MCU | V<sub>IH</sub> is 0.7 × VDD = 3.5 V at 5 V | Level-shift, or run the sensor in 3.3 V mode |

## CRC or framing errors

`CrcError` or `FramingError` in the sticky flags, in `GetErrorFlags()` or in a getter's `Result::error` (the
chip saw a bad MOSI frame), or `ReplyCrcError` (the driver saw a bad MISO frame). `BusError` in a getter's
`Result::error` means the port's `transfer()` returned `false`.

| Cause | Fix |
| --- | --- |
| Clock too fast for the wiring | Drop to 1 MHz; the limit is 10 MHz on a short, clean bus |
| CSn set-up or high time under 350 ns | Add chip-select set-up and hold cycles in your SPI driver |
| A frame of the wrong length | `transfer()` must clock exactly `len` bytes (2, 3 or 4) |
| Noise on CLK | Shorter traces, series resistor at the MCU, solid ground between MCU and sensor |

Pass a `retries` value (for example `GetAngle(2)`) to repeat a read the chip flagged. Retries hide occasional
errors; they do not fix a wiring problem.

## AGC at 0 or 255

| AGC | Meaning | Fix |
| --- | --- | --- |
| 255, `MagHalf` set | Field too weak: magnet too far, too small or not diametrically magnetized | Move it closer; the field must be 35 mT to 70 mT at the die |
| 0 | Field too strong | Move the magnet away |
| Changes with angle | Magnet off-center or tilted | Center it on the package axis |

## The angle is wrong or noisy

| Symptom | Cause | Fix |
| --- | --- | --- |
| Angle runs backwards | `DIR` | `SetDirection(true)` makes clockwise (seen from the top of the chip) increase |
| Zero is in the wrong place | Zero position not set, or lost at power-off | `SetZeroPosition()`, then [OTP programming](otp.md) to keep it |
| Noisy at standstill | Filter at high K, DAEC on | `SetFilterPreset(FilterPreset::LowNoise)`, `SetDynamicAngleCompensation(false)` |
| Lags at high speed | DAEC off, or reading ANGLEUNC | `SetDynamicAngleCompensation(true)`, `SetAngleOutputSource(UseANGLECOM)` |
| The ABI count lags by a step after a reversal | ABI hysteresis (1 LSB of 11 bit, 0.17°, by default) | `SetHysteresis(SETTINGS3::Hysteresis::NONE)` |

## Velocity is not zero at standstill

The velocity is a 14-bit two's-complement value in 24.141 °/s per LSB. At standstill the datasheet gives 5.8 °/s
rms noise at K = 0 and 244.9 °/s at K = 6 (Figure 17), so ±1 LSB with the magnet still is normal, and more is normal
with a high filter K. Detect "stopped" with a threshold, not with zero.

## Settings are gone after a power cycle

The settings registers are volatile until OTP is burned: this is expected. Burn them with `ProgramOTP()` once you
are sure of them ([OTP programming](otp.md)).

## A setter returns false

The value did not read back as written, or a frame of the write failed. The driver prints nothing; clear the sticky
flags, call the setter again and read them:

| Sticky flags after the call | Cause |
| --- | --- |
| `CrcError` | The write frame was corrupted on the bus: see CRC errors above |
| `FramingError` or `CommandError` | Frame length or chip select |
| `BusError` | The port's `transfer()` returned `false` |
| `ReplyCrcError` | A reply was corrupted on MISO: see CRC errors above |
| None | The register or field is read-only, or the value does not stick |

Call the setter with a `retries` value to retry automatically.
