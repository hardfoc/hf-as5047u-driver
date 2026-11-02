# Hardware Setup

![AS5047U interfaces — SPI / ABI / UVW / PWM](assets/as5047u-interfaces.svg)

This guide covers the physical connections and hardware requirements for the AS5047U magnetic encoder chip.

## Pin Connections

### Basic SPI Connections

```
MCU              AS5047U
─────────────────────────
3.3V      ────── VDD
GND       ────── GND
SCK       ────── CLK
MOSI      ────── MISO (note: AS5047U uses MISO for both directions)
MISO      ────── MOSI
CS        ────── CSn
```

### Pin Descriptions

| Pin | Name | Description | Required |
|-----|------|-------------|----------|
| VDD | Power | 3.3V power supply (2.7V - 3.6V) | Yes |
| GND | Ground | Ground reference | Yes |
| CLK | Clock | SPI clock line | Yes |
| MISO | Master In | SPI data input (MCU receives) | Yes |
| MOSI | Master Out | SPI data output (MCU transmits) | Yes |
| CSn | Chip Select | SPI chip select (active low) | Yes |
| PROG | Program | OTP programming enable (optional) | No |

**Note**: The AS5047U uses a unique SPI configuration where MISO and MOSI are swapped compared to standard SPI. The MCU's MOSI connects to the AS5047U's MISO pin, and the MCU's MISO connects to the AS5047U's MOSI pin.

## Power Requirements

- **Supply Voltage**: 2.7V - 3.6V (3.3V typical)
- **Current Consumption**: 6.5 mA typical, 10 mA maximum
- **Power Supply**: Clean 3.3V supply with decoupling capacitor (100nF ceramic + 10µF tantalum recommended)

## SPI Configuration

- **Mode**: SPI Mode 1 (CPOL=0, CPHA=1) or Mode 3 (CPOL=1, CPHA=1)
- **Speed**: Up to 10 MHz (4 MHz recommended for reliability)
- **Bit Order**: MSB first
- **CS Polarity**: Active low (CSn)
- **Frame Formats**: 16-bit, 24-bit (with CRC), or 32-bit (with CRC)

## Magnetic Setup

The AS5047U requires a diametrically magnetized magnet positioned above the sensor:

- **Magnet Type**: Diametrically magnetized (2-pole) magnet
- **Magnet Diameter**: 6-12 mm recommended
- **Air Gap**: 0.5 - 3.0 mm (1.0 mm typical)
- **Alignment**: Centered above the sensor die
- **Field Strength**: 30-100 mT at the sensor surface

## Physical Layout Recommendations

- Keep SPI traces short (< 10cm recommended)
- Use ground plane for noise reduction
- Place decoupling capacitors (100nF ceramic + 10µF tantalum) close to VDD pin
- Route clock and data lines away from noise sources
- Keep magnet away from ferromagnetic materials
- Avoid stray magnetic fields from motors or other sources

## Example Wiring Diagram

```
                    AS5047U
                    ┌─────────┐
        3.3V ───────┤ VDD     │
        GND  ───────┤ GND     │
        SCK  ───────┤ CLK     │
        MOSI ───────┤ MISO    │
        MISO ───────┤ MOSI    │
        CS   ───────┤ CSn     │
                    └─────────┘
                      │
                      │ (air gap 0.5-3mm)
                      ▼
                  ┌───────┐
                  │ Magnet│
                  │ (6-12mm)│
                  └───────┘
```

## Incremental Output Pins (ABI)

If using ABI incremental outputs:

| Pin | Name | Description |
|-----|------|-------------|
| A | A | Incremental encoder A output |
| B | B | Incremental encoder B output |
| I | I | Index pulse output |

## Commutation Output Pins (UVW)

If using UVW commutation outputs:

| Pin | Name | Description |
|-----|------|-------------|
| U | U | Commutation U output |
| V | V | Commutation V output |
| W | W | Commutation W output |

## PWM Output

If using PWM output:

| Pin | Name | Description |
|-----|------|-------------|
| PWM | PWM | PWM-encoded angle output (on W or I pin depending on configuration) |

## Next Steps

- Verify connections with a multimeter
- Proceed to [Quick Start](quickstart.md) to test the connection
- Review [Platform Integration](platform_integration.md) for software setup

---

**Navigation**
⬅️ [Quick Start](quickstart.md) | [Next: Platform Integration ➡️](platform_integration.md) | [Back to Index](index.md)

