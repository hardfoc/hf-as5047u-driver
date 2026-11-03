# Quickstart

Read an angle from your MCU: wire four SPI signals and the supply, write a one-method port class, call three
driver functions.

## 1. Wire it

| MCU | AS5047U pin | Notes |
| --- | --- | --- |
| SPI SCLK | 2 (CLK) | SPI mode 1 (CPOL = 0, CPHA = 1), up to 10 MHz; start at 1 MHz |
| SPI MOSI | 4 (MOSI) | The chip samples it on the falling clock edge |
| SPI MISO | 3 (MISO) | High-impedance while CSn is high |
| GPIO or hardware CS | 1 (CSn) | Active low; one frame per CSn low period |
| 3.3 V | 11 (VDD) and 12 (VDD3V3), tied | 3.3 V mode, 3.0 V to 3.6 V. For 5 V mode see [Hardware](hardware.md#power) |
| GND | 13 (GND) and 5 (TEST) | TEST must be grounded |

Put 100 nF from VDD to GND at the pins. Center a diametrically magnetized magnet over the package; the field must
be 35 mT to 70 mT at the die. The [Hardware](hardware.md) page has every pin, the timing and the 5 V wiring.

## 2. Add the driver

Add the repository as a submodule and link the `hf_as5047u` CMake target (or add `inc/` to your include path and
build with C++20). [Installation](installation.md) covers CMake and ESP-IDF.

```bash
git submodule add https://github.com/hardfoc/hf-as5047u-driver.git external/hf-as5047u-driver
```

## 3. Write the port

The port is the only platform code: one `transfer()` that sends `len` bytes and receives `len` bytes with CSn held
low, and returns `false` on a bus error. Replace the three `platform_*` calls with your SDK's.

```cpp title="my_spi.hpp"
--8<-- "tests/docs/my_spi.hpp:port"
```

[Porting](porting.md) explains the contract and points to the ESP-IDF port the examples use.

## 4. Read the angle

```cpp title="main.cpp"
--8<-- "tests/docs/quickstart.cpp:quickstart"
```

What each step does:

1. The constructor only stores the port and the frame format; it does not talk to the chip. `SPI_24` puts a CRC
   on every frame.
2. `GetDiagnostics()` reads the DIA register. `LoopsFinished` is set once the chip's internal loops have settled
   after power-on (at most 10 ms); `MagHalf_flag` is set when the field is too weak.
3. `GetAngleDegrees(2)` reads ANGLECOM and repeats the read up to two times if the chip reports a CRC or framing
   error. `GetVelocityRPM()` and `GetAGC()` read VEL and AGC.
4. Every getter returns an `as5047u::Result`: the reading in `.value` and, when the read failed, the cause in
   `.error`. Check it before using the value; a failed read is never reported as 0 or as the previous value.

This example is built by the host CI, so it compiles against the current headers.

## If it does not work

| Symptom | First thing to check |
| --- | --- |
| `print_angle()` returns `-1`, every register reads 0 | MISO wiring, supply, CSn toggling per frame |
| Every register reads `0x3FFF` | MISO floating or stuck high; SPI mode (it must be mode 1) |
| `MagHalf_flag` set, AGC at 255 | Magnet too far or too weak ([Troubleshooting](troubleshooting.md#agc-at-0-or-255)) |
| Angle jumps or does not follow the shaft | Magnet off-centre or not diametrically magnetized |

## Next

- [API guide](api-guide.md): velocity, diagnostics, error flags, outputs, raw registers.
- [Configuration](configuration.md): settings for commutation and for low-noise positioning.
- [Examples](examples.md): complete ESP32 applications.
