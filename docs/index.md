---
hide:
  - navigation
  - toc
---

<div class="hf-hero" markdown>
<span class="hf-kicker">HardFOC driver</span>

# AS5047U Driver

<p>A C++20 driver for the ams OSRAM AS5047U 14-bit magnetic rotary position sensor. It reads angle, velocity and
field diagnostics over CRC-protected SPI, configures the ABI, UVW and PWM outputs and programs the OTP, on any
MCU with an SPI master.</p>

[Quickstart](quickstart.md){ .md-button .md-button--primary }
[API guide](api-guide.md){ .md-button }
[Source on GitHub](https://github.com/hardfoc/hf-as5047u-driver){ .md-button }
</div>

The driver has no platform code. You give it a class with one method that moves a frame over SPI; it does the
rest: frame building, CRC-8, the one-frame-late read protocol, read-back checks of every write, error flags and
unit conversion.

<div class="hf-chip" markdown>
<div class="hf-chip__head">
<span class="hf-chip__kicker">14-bit on-axis magnetic rotary position sensor</span>
<span class="hf-chip__name">AS5047U</span>
<span class="hf-chip__vendor">ams OSRAM</span>
</div>

Measures
:   Absolute angle, 14 bit (16384 positions per turn), velocity 24.141 °/s per LSB, AGC and field magnitude

Host interface
:   SPI mode 1 (CPOL = 0, CPHA = 1) up to 10 MHz; 16-bit frames, or 24- and 32-bit frames with CRC-8

Outputs
:   ABI incremental up to 4096 pulses per turn, UVW commutation for 1 to 7 pole pairs, 12-bit PWM

Supply
:   4.5 V to 5.5 V on VDD (internal LDO), or 3.0 V to 3.6 V on VDD and VDD3V3 tied

Limits
:   Up to 28000 rpm, −40 °C to 150 °C, field 35 mT to 70 mT at the die

Package and memory
:   TSSOP-14; settings and zero position in volatile registers, burnable once to OTP

[Datasheet](datasheet/AS5047U_DS000637_1-01.pdf)
</div>

## What you get

- **One object per sensor.** `AS5047U<YourSpi>` reads the angle (`GetAngle()`, `GetAngleDegrees()`), the
  uncompensated angle, velocity in LSB, °/s, rad/s or rpm, AGC and magnitude. See [API guide](api-guide.md).
- **All three frame formats.** 16-bit frames for the fastest reads, 24-bit frames with CRC, 32-bit frames with a
  PAD byte for daisy chains. Writes always use a CRC frame. See [SPI protocol](protocol.md).
- **Checked writes.** Every register write is read back on the next frame and retried on request. See
  [API guide](api-guide.md#handle-errors).
- **The outputs configured in one call.** ABI resolution, index width, UVW pole pairs, PWM pin, hysteresis,
  direction, zero position, DAEC and the adaptive filter. See [Configuration](configuration.md).
- **Typed registers.** Every register is a struct with named bit fields, for `ReadReg<>()` and `WriteReg()`.
- **OTP programming.** The datasheet's burn sequence in `ProgramOTP()`. See [OTP programming](otp.md).
- **Tested without hardware.** Host unit tests run the driver against a model of the chip's SPI slave. See
  [Testing and CI](testing.md).

## Where to go next

<div class="grid cards" markdown>

-   :material-rocket-launch-outline: **[Quickstart](quickstart.md)**

    ---

    Wire the sensor, write the port, read an angle in about 15 lines.

-   :material-chip: **[Hardware](hardware.md)**

    ---

    Pins, bus settings, timing, power modes and the magnet.

-   :material-layers-outline: **[Architecture](architecture.md)**

    ---

    The three layers and what each one owns.

-   :material-code-braces: **[API guide](api-guide.md)**

    ---

    Read, configure, set up the outputs and handle errors, task by task.

-   :material-swap-horizontal: **[Porting](porting.md)**

    ---

    Implement the SPI interface for your MCU.

-   :material-book-open-variant: **[API reference](api-reference.md)**

    ---

    Every class, member and register, generated from the headers.

</div>
