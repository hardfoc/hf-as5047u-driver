# Troubleshooting

This guide helps you diagnose and resolve common issues when using the AS5047U driver.

## Common Error Messages

### Error: CRC Errors

**Symptoms:**
- Frequent CRC errors when reading angle
- `GetErrorFlags()` returns `CrcError` flag
- Unreliable angle readings

**Causes:**
- SPI signal integrity issues (noise, reflections)
- Incorrect SPI mode or timing
- Loose connections
- SPI speed too high
- Power supply noise

**Solutions:**
1. **Reduce SPI Speed**: Try lowering SPI frequency (e.g., 1-2 MHz instead of 10 MHz)
2. **Check SPI Mode**: Ensure SPI Mode 1 (CPOL=0, CPHA=1) or Mode 3 (CPOL=1, CPHA=1)
3. **Enable Retries**: Use retry parameter: `encoder.GetAngle(3)` for automatic retry
4. **Verify Connections**: Check all SPI connections are secure
5. **Add Decoupling**: Ensure proper decoupling capacitors (100nF + 10µF) near VDD
6. **Check Ground**: Verify good ground connection between MCU and sensor
7. **Use 24-bit Frames**: Switch to `FrameFormat::SPI_24` for CRC protection

---

### Error: Framing Errors

**Symptoms:**
- `GetErrorFlags()` returns `FramingError` flag
- Communication failures

**Causes:**
- Incorrect SPI frame format
- CS timing issues
- SPI transaction length mismatch

**Solutions:**
1. **Check Frame Format**: Ensure frame format matches sensor configuration
2. **Verify CS Timing**: Check chip select assertion/deassertion timing
3. **Review SPI Implementation**: Ensure `transfer()` method handles CS correctly

---

### Velocity range and scale (why the numbers look the way they do)

**Datasheet Figure 16 – Angular Velocity Measurement Parameter:**

| Symbol   | Parameter                    | Min/Typ/Max | Unit      | Notes                          |
|---------|------------------------------|------------|-----------|--------------------------------|
| **VRes**  | Velocity signal resolution   | 14         | bit       | Two's complement value         |
| **VRange**| Measurement range (default)  | -28000 … 28000 | rpm   | Default range                  |
| **VSens** | Velocity sensitivity (default) | 24.141   | °/s/bit   | 14-bit resolution; ±5% (actual rotation speed) |
| **VError**| Velocity total error         | 16.9 … 68.4 | —       | Based on actual rotation speed |
| **FCutoff** | Cut-off frequency          | 231        | Hz        | Depends on K (adaptive filter) |

- The velocity register is **14-bit two’s complement**: **-8192 to +8191 LSB**. Scale is **VSens = 24.141 °/s per LSB** (default, ±5%).
- The **measurement range** (VRange) is **±28,000 rpm** (default), i.e. about ±168,000 °/s. So 1 LSB = 24.141 °/s over that range; the ECU can use the value without further averaging.

**Why “1 LSB” is ~24 °/s:** The sensor covers a wide velocity range (up to ±28,000 rpm) in 14 bits, so each LSB is a fixed **24.141 °/s**. Example: **33 LSB** = 33 × 24.141 ≈ **797 °/s** (~133 rpm). In LSB terms that’s 33/8192 ≈ **0.4% of full scale**; in °/s it looks large only because of the fixed scale.

**Datasheet Figure 17 – Angular Velocity Measurement Filter Parameters (RMS noise at standstill):**

| Filter K | Typ RMS noise (°/s) |
|----------|----------------------|
| K=0      | 5.8                  |
| K=1      | 6                    |
| K=2      | 8.4                  |
| K=3      | 19.8                 |
| K=4      | 51.8                 |
| K=5      | 121.9                |
| K=6      | 244.9                |

**At standstill:** For typical K (e.g. K=0), RMS noise is **5.8 °/s** ≈ **0.24 LSB**. So 0 or ±1 LSB at standstill is normal. A reading of tens of LSB (e.g. 33 LSB ≈ 797 °/s) with the magnet stationary is **not** normal and may indicate noise, angle jitter, or a wrong register (e.g. AGC instead of VEL).

---

### Velocity Not Zero When Magnet Is Stationary

**Symptoms:**
- `GetVelocity()` returns small non-zero values (e.g. -1 or +1 LSB) when the magnet is not moving
- Converted values around ±24 °/s at standstill

**Datasheet:** See **Velocity range and scale** above for Figure 16 (VRes, VRange, VSens) and Figure 17 (RMS noise vs filter K). The AS5047U specifies **RMS velocity noise at standstill** depending on the adaptive filter K (OTP); e.g. **5.8 °/s** (K=0), **6 °/s** (K=1), **8.4 °/s** (K=2), up to **244.9 °/s** (K=6). With **VSens = 24.141 °/s/bit**, ±1 LSB (~24 °/s) at standstill is within the expected noise band.

**Conclusion:** Non-zero velocity while the magnet is not moving is **expected behavior**, not a fault. For “zero speed” detection, compare against a threshold (e.g. ±2 LSB or ±50 °/s) rather than exact zero. If velocity at standstill is **large** (e.g. > 10 LSB, ~240 °/s), the integration test logs a warning: check wiring, AGC, and that the VEL register (0x3FFC) is read correctly (not confused with AGC 0x3FF9).

---

### Error: AGC Warning

**Symptoms:**
- `GetErrorFlags()` returns `AgcWarning` flag
- AGC value at 0 or 255
- Unreliable readings

**Causes:**
- Magnet too close or too far from sensor
- Weak magnetic field
- Stray magnetic fields interfering

**Solutions:**
1. **Check Air Gap**: Adjust magnet distance (0.5-3.0 mm recommended, 1.0 mm typical)
2. **Verify Magnet**: Ensure diametrically magnetized magnet (6-12 mm diameter)
3. **Check Field Strength**: Magnetic field should be 30-100 mT at sensor surface
4. **Remove Interference**: Keep magnet away from ferromagnetic materials and motors
5. **Check Alignment**: Ensure magnet is centered above sensor die

---

### Error: Magnitude Half Warning

**Symptoms:**
- `GetErrorFlags()` returns `MagHalf` flag
- Magnetic field magnitude is half of regulated value

**Causes:**
- Magnet too far from sensor
- Weak magnetic field
- Incorrect magnet type

**Solutions:**
1. **Reduce Air Gap**: Move magnet closer to sensor (within 0.5-3.0 mm range)
2. **Check Magnet Strength**: Verify magnet provides adequate field (30-100 mT)
3. **Verify Magnet Type**: Ensure diametrically magnetized (2-pole) magnet

---

## Hardware Issues

### Device Not Responding

**Symptoms:**
- No response from sensor
- All readings return 0 or invalid values
- SPI communication fails

**Checklist:**
- [ ] Verify power supply voltage is 2.7V - 3.6V (3.3V typical)
- [ ] Check all connections are secure (VDD, GND, CLK, MISO, MOSI, CS)
- [ ] Verify SPI pin mapping (AS5047U uses reversed MISO/MOSI naming)
- [ ] Check CS line is properly controlled (active low)
- [ ] Verify SPI mode is correct (Mode 1 or Mode 3)
- [ ] Use oscilloscope/logic analyzer to verify bus activity
- [ ] Check for short circuits or incorrect wiring

---

### Communication Errors

**Symptoms:**
- Timeout errors
- CRC/checksum errors
- Inconsistent readings

**Solutions:**
- **Check Bus Speed**: Ensure SPI speed is within specifications (up to 10 MHz, 4 MHz recommended)
- **Verify Signal Integrity**: Check for noise, reflections, or signal degradation
- **Ensure Proper Termination**: Verify proper bus termination
- **Check for Loose Connections**: Inspect all connections
- **Reduce SPI Speed**: Try lower frequency if issues persist
- **Add Pull-ups**: Ensure proper pull-up resistors if needed

---

### Incorrect Angle Readings

**Symptoms:**
- Angle values don't change when rotating
- Angle values jump erratically
- Angle values are always zero

**Causes:**
- Magnet not properly positioned
- Incorrect magnet type
- Stray magnetic fields
- Sensor not initialized

**Solutions:**
1. **Check Magnet Placement**: Verify magnet is centered and at correct distance
2. **Verify Magnet Type**: Ensure diametrically magnetized (2-pole) magnet
3. **Check for Interference**: Remove sources of stray magnetic fields
4. **Verify Initialization**: Ensure SPI bus is properly initialized
5. **Check Error Flags**: Read error flags to diagnose specific issues

---

### Zero Position or Direction (SETTINGS2) Not Persisting

**Symptoms:**
- `SetZeroPosition()` or `SetDirection()` returns OK but read-back is wrong or unchanged
- Zero position read-back is 0 after write; DIR bit does not change

**Datasheet (AS5047U DS000637):**
- Non-volatile registers (ZPOSM 0x0016, ZPOSL 0x0017, SETTINGS2 0x0019, etc.) support **soft write**: SPI write can be done multiple times; content persists until **power cycle** (no OTP burn needed for runtime changes).
- If soft write is accepted, values should persist until reset. If read-back still shows 0 or old value, the write may not be reaching the device.

**Causes:**
- CRC or framing errors during write (check ERRFL after write; driver may still report success if read-back matches)
- SPI timing (e.g. tCSn ≥ 350 ns between frames; Mode 1, CPOL=0, CPHA=1)
- Wiring or noise causing failed transactions
- On some parts or boards, the device may not accept **DIR=0** (clockwise); DIR remains 1. The integration test passes with a warning in this case.

**Solutions:**
1. **Check ERRFL**: After a write, read `GetErrorFlags()`. If CRC_error or Framing_error is set, fix SPI signal/timing first.
2. **Use 24-bit frames**: Writes use 24- or 32-bit frames with CRC; ensure frame format and CRC (polynomial 0x1D, init 0xC4, final XOR 0xFF) match the datasheet.
3. **Retry**: Use `SetZeroPosition(angle, retries)` or `SetDirection(clockwise, retries)` with retries > 0.
4. **Permanent programming**: For values that must survive power loss, use the OTP programming procedure (PROG register, PROGOTP, etc.) as described in the datasheet.

**Write verification and diagnostic output:**  
The driver verifies each register write by sending a NOP after the write data; the AS5047U returns the new register content on that NOP response (datasheet: “at the next command” MISO = new content). If the returned value does not match what was written, the write is reported as failed and the driver prints a diagnostic line to stdout, for example:

`AS5047U write verify failed: addr=0x0016 expected=0x0040 read_back=0x0010 ERRFL=0x0000 (CRC_error=0 Framing_error=0 Command_error=0)`

Use this to see whether CRC_error, Framing_error, or Command_error is set (ERRFL bits 6, 4, 5). If all are 0 and read_back still differs from expected, the NOP response may be delayed or from a different transaction; the register may still have been updated—read the register again to confirm. If the API returns false but a subsequent read shows the correct value, the write likely succeeded and the verify failed due to timing or pipeline behaviour.

---

## Software Issues

### Compilation Errors

**Error: "No matching function"**

**Solution:**
- Ensure you've implemented the `transfer()` method in your SPI interface
- Check method signature matches: `void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len)`
- Verify you're inheriting from `as5047u::SpiInterface<YourClass>`

**Error: "Undefined reference"**

**Solution:**
- Verify you're linking the driver source file (`as5047u.cpp`)
- Check include paths are correct
- Ensure C++20 standard is enabled: `-std=c++20`

**Error: "Template instantiation failed"**

**Solution:**
- Verify your SPI interface class is properly templated: `class MySPI : public as5047u::SpiInterface<MySPI>`
- Check that all required methods are implemented

---

### Runtime Errors

**Initialization Fails**

**Checklist:**
- [ ] SPI bus interface is properly initialized
- [ ] Hardware connections are correct
- [ ] SPI configuration matches sensor requirements
- [ ] Device is powered and ready
- [ ] CS pin is correctly configured

**Unexpected Behavior**

**Checklist:**
- [ ] Verify configuration matches your use case
- [ ] Check for timing issues (add delays if needed)
- [ ] Review error handling code
- [ ] Verify frame format is correct
- [ ] Check error flags for diagnostic information

---

## Debugging Tips

### Enable Debug Output

Add debug prints to your SPI implementation:

```cpp
void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    printf("SPI Transfer: len=%zu\n", len);
    // ... your SPI code
}
```

### Use DumpStatus()

Print comprehensive sensor status:

```cpp
encoder.DumpStatus();
```

This prints angle, velocity, AGC, magnitude, and error flags.

### Check Error Flags

Always check error flags after operations:

```cpp
uint16_t angle = encoder.GetAngle();
AS5047U_Error errors = encoder.GetStickyErrorFlags();
if (errors != AS5047U_Error::None) {
    printf("Errors: 0x%04X\n", static_cast<uint16_t>(errors));
}
```

### Use a Logic Analyzer

For bus communication issues, a logic analyzer can help:
- Verify correct protocol timing
- Check for signal integrity issues
- Validate data being sent/received
- Debug CS timing issues

### Verify SPI Pin Mapping

**Important**: AS5047U uses reversed SPI pin naming:
- MCU MOSI → AS5047U MISO (data from MCU to sensor)
- MCU MISO → AS5047U MOSI (data from sensor to MCU)

Double-check your wiring matches this!

---

## FAQ

### Q: What SPI frame format should I use?

**A:** For most applications, use `FrameFormat::SPI_24` - it provides CRC protection with good throughput. Use `SPI_16` for maximum speed (no CRC), or `SPI_32` for daisy-chain applications.

### Q: How do I know if my magnet is positioned correctly?

**A:** Check the AGC value - it should be between 10-245 (not at limits 0 or 255). Also check magnitude - it should be in a reasonable range (typically 5000-15000). Use `DumpStatus()` to see all diagnostics.

### Q: Can I use multiple AS5047U sensors on the same SPI bus?

**A:** Yes, use `FrameFormat::SPI_32` with different pad bytes for each sensor. Each sensor needs its own CS line.

### Q: Why are my angle readings noisy?

**A:** Enable adaptive filtering: `encoder.SetAdaptiveFilter(true)` and adjust filter parameters. Also check magnet placement and ensure proper decoupling capacitors.

### Q: How do I calibrate the zero position?

**A:** Read the current angle when at your desired zero position, then call `encoder.SetZeroPosition(current_angle)`. See [Example 7](examples.md#example-7-zero-position-calibration) for details.

### Q: What happens if I program OTP incorrectly?

**A:** OTP programming is **irreversible**. If you program incorrect settings, you cannot change them. Always verify settings before programming OTP.

---

## Getting More Help

If you're still experiencing issues:

1. Check the [API Reference](api_reference.md) for method details
2. Review [Examples](examples.md) for working code
3. Search existing issues on GitHub
4. Open a new issue with:
   - Description of the problem
   - Steps to reproduce
   - Hardware setup details (MCU, wiring, magnet type)
   - Error messages/logs
   - Output from `DumpStatus()`

---

**Navigation**
⬅️ [Examples](examples.md) | [Back to Index](index.md)

