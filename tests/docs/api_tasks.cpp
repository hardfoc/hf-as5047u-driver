/**
 * @file api_tasks.cpp
 * @brief The API guide's task snippets, compiled by the host build (never run).
 */
#include "my_spi.hpp"

#include "as5047u.hpp"

#include <cstdio>

using Encoder = as5047u::AS5047U<MySpi>;

void construct(MySpi& spi) {
  // --8<-- [start:construct]
  Encoder encoder(spi, FrameFormat::SPI_24);  // SPI_16 (no CRC), SPI_24 (CRC) or SPI_32 (CRC + PAD)

  encoder.SetFrameFormat(FrameFormat::SPI_16);  // change it later, e.g. for the fastest angle reads
  // --8<-- [end:construct]
}

void read(Encoder& encoder) {
  // --8<-- [start:read]
  const uint16_t lsb = encoder.GetAngle();              // ANGLECOM, 0..16383 (14 bit)
  const float deg = encoder.GetAngleDegrees();          // [0, 360)
  const float rad = encoder.GetAngle(as5047u::AngleUnit::Radians);
  const uint16_t raw = encoder.GetRawAngle();           // ANGLEUNC: without dynamic angle error compensation

  const int16_t vel = encoder.GetVelocity();            // signed 14 bit, 24.141 deg/s per LSB
  const float rpm = encoder.GetVelocity(as5047u::VelocityUnit::Rpm);

  std::printf("%u (%.2f deg, %.4f rad) raw %u, vel %d (%.1f rpm)\n", lsb, deg, rad, raw, vel, rpm);
  // --8<-- [end:read]
}

void diagnostics(Encoder& encoder) {
  // --8<-- [start:diagnostics]
  const uint8_t agc = encoder.GetAGC();          // 0..255; at 0 or 255 the field is out of range
  const uint16_t mag = encoder.GetMagnitude();   // CORDIC magnitude, held constant by the AGC
  const auto dia = encoder.GetDiagnostics();     // DIA (0x3FF5)

  std::printf("AGC %u  MAG %u  5V mode %u  AGC low %u high %u\n", agc, mag,
              static_cast<unsigned>(dia.bits.VDD_mode), static_cast<unsigned>(dia.bits.Comp_l),
              static_cast<unsigned>(dia.bits.Comp_h));

  encoder.DumpStatus();                          // every register, formatted, through printf
  // --8<-- [end:diagnostics]
}

bool errors(Encoder& encoder) {
  // --8<-- [start:errors]
  const auto angle = encoder.ReadReg<AS5047U_REG::ANGLECOM>();  // reads ANGLECOM, then ERRFL
  const auto flags = static_cast<uint16_t>(encoder.GetStickyErrorFlags());
  if (flags != 0U) {
    if (flags & static_cast<uint16_t>(AS5047U_Error::CrcError)) {
      std::printf("CRC error: check wiring, SPI mode 1, clock\n");
    }
    if (flags & (static_cast<uint16_t>(AS5047U_Error::AgcWarning) |
                 static_cast<uint16_t>(AS5047U_Error::MagHalf))) {
      std::printf("magnet out of range\n");
    }
    return false;  // do not use this sample
  }
  std::printf("angle %u\n", static_cast<unsigned>(angle.bits.ANGLECOM_value));
  return true;
  // --8<-- [end:errors]
}

bool configure(Encoder& encoder) {
  // --8<-- [start:configure]
  bool ok = true;
  ok = ok && encoder.SetZeroPosition(encoder.GetAngle());     // here is 0 degrees
  ok = ok && encoder.SetDirection(true);                      // clockwise (seen from the top) increases
  ok = ok && encoder.SetDynamicAngleCompensation(true);       // DAEC on (default)
  ok = ok && encoder.SetFilterPreset(FilterPreset::Balanced); // adaptive filter, K = 2..3
  ok = ok && encoder.SetHysteresis(AS5047U_REG::SETTINGS3::Hysteresis::LSB_1);
  ok = ok && encoder.Set150CTemperatureMode(false);           // 3.3 V supply: low noise, up to 125 C
  return ok;  // false: a write did not read back as written
  // --8<-- [end:configure]
}

bool outputs(Encoder& encoder) {
  // --8<-- [start:outputs]
  bool ok = true;
  ok = ok && encoder.ConfigureInterface(true, false, false);  // ABI on, UVW off, no PWM
  ok = ok && encoder.SetABIResolution(12);                    // 4096 steps, 1024 pulses per turn
  ok = ok && encoder.SetIndexPulseLength(3);                  // index pulse 3 LSB wide (or 1)

  ok = ok && encoder.SetUVWPolePairs(4);                      // for UVW commutation of a 4 pole-pair motor
  return ok;
  // --8<-- [end:outputs]
}

bool raw(Encoder& encoder) {
  // --8<-- [start:raw]
  auto s2 = encoder.ReadReg<AS5047U_REG::SETTINGS2>();  // typed register, fields in .bits
  s2.bits.ABI_DEC = 1;                                  // decimal ABI counts (1000, 500, ... pulses)
  s2.bits.IWIDTH = 1;                                   // 1 LSB index pulse
  if (!encoder.WriteReg(s2, 2)) {                       // verified write, 2 retries
    return false;
  }
  auto s3 = encoder.ReadReg<AS5047U_REG::SETTINGS3>();
  s3.bits.ABIRES = static_cast<uint8_t>(AS5047U_REG::SETTINGS3::ABIResolution::Code0);  // 1000 pulses
  return encoder.WriteReg(s3);
  // --8<-- [end:raw]
}

bool otp(Encoder& encoder) {
  // --8<-- [start:otp]
  // Settings are in the volatile registers now (configure() above). The magnet is at the zero
  // position, VDD is 4.5 to 5.5 V (5 V mode) or 3.3 to 3.5 V (3.3 V mode), 5 to 45 C.
  if (!encoder.ProgramOTP()) {   // sets the current angle as zero, then burns. Once per chip.
    return false;                // read-back did not match: do not use the part
  }
  return true;                   // power-cycle and compare the settings again (OTP programming)
  // --8<-- [end:otp]
}
