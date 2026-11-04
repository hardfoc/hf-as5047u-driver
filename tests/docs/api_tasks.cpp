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
  // SPI_16 (no CRC), SPI_24 (CRC) or SPI_32 (CRC and a PAD byte)
  Encoder encoder(spi, FrameFormat::SPI_24);

  encoder.SetFrameFormat(FrameFormat::SPI_16);  // later, e.g. for the fastest reads
  // --8<-- [end:construct]
}

bool read(Encoder& encoder) {
  // --8<-- [start:read]
  // Every getter returns as5047u::Result<T>: .value, and .error (None when the read worked).
  const auto lsb = encoder.GetAngle();      // ANGLECOM, 0..16383 (14 bit)
  if (!lsb.ok()) {                          // BusError, ReplyCrcError, CrcError, FramingError
    std::printf("angle read failed: 0x%04X\n", static_cast<unsigned>(lsb.error));
    return false;                           // lsb.value is not a reading
  }
  const auto deg = encoder.GetAngleDegrees();  // [0, 360)
  const auto rad = encoder.GetAngle(as5047u::AngleUnit::Radians);
  const auto raw = encoder.GetRawAngle();   // ANGLEUNC: without DAEC

  const auto vel = encoder.GetVelocity();   // signed 14 bit, 24.141 deg/s per LSB
  const auto rpm = encoder.GetVelocity(as5047u::VelocityUnit::Rpm);
  if (!deg || !rad || !raw || !vel || !rpm) {
    return false;
  }

  std::printf("%u (%.2f deg, %.4f rad) raw %u, vel %d (%.1f rpm)\n", lsb.value,
              static_cast<double>(deg.value), static_cast<double>(rad.value), raw.value,
              vel.value, static_cast<double>(rpm.value));
  return true;
  // --8<-- [end:read]
}

bool diagnostics(Encoder& encoder) {
  // --8<-- [start:diagnostics]
  const auto agc = encoder.GetAGC();         // 0..255; 0 or 255: field out of range
  const auto mag = encoder.GetMagnitude();   // CORDIC magnitude, kept steady by AGC
  const auto dia = encoder.GetDiagnostics(); // DIA (0x3FF5)
  if (!agc || !mag || !dia) {
    return false;                            // a read failed: no diagnostics to show
  }

  std::printf("AGC %u  MAG %u  5V mode %u  AGC low %u high %u\n", agc.value, mag.value,
              static_cast<unsigned>(dia.value.bits.VDD_mode),
              static_cast<unsigned>(dia.value.bits.Comp_l),
              static_cast<unsigned>(dia.value.bits.Comp_h));

  encoder.DumpStatus();                      // every register, through printf
  return true;
  // --8<-- [end:diagnostics]
}

bool errors(Encoder& encoder) {
  // --8<-- [start:errors]
  (void)encoder.ClearStickyErrorFlags();      // start a clean window
  const auto angle = encoder.GetAngle(2);     // then ERRFL; 2 retries on CRC
  if (!angle.ok()) {
    return false;  // the read failed; angle.error says why
  }
  const auto flags = static_cast<uint16_t>(encoder.GetStickyErrorFlags()); // kept until cleared
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
  std::printf("angle %u\n", static_cast<unsigned>(angle.value));
  return true;
  // --8<-- [end:errors]
}

bool configure(Encoder& encoder) {
  // --8<-- [start:configure]
  using Hys = AS5047U_REG::SETTINGS3::Hysteresis;
  const auto here = encoder.GetAngle();
  bool ok = here.ok();
  ok = ok && encoder.SetZeroPosition(here.value);             // here is 0 degrees
  ok = ok && encoder.SetDirection(true);          // clockwise (from the top) counts up
  ok = ok && encoder.SetDynamicAngleCompensation(true);       // DAEC on (default)
  ok = ok && encoder.SetFilterPreset(FilterPreset::Balanced); // K = 2..3
  ok = ok && encoder.SetHysteresis(Hys::LSB_1);
  ok = ok && encoder.Set150CTemperatureMode(false);  // 3.3 V: low noise, up to 125 C
  return ok;  // false: a write did not read back as written
  // --8<-- [end:configure]
}

bool outputs(Encoder& encoder) {
  // --8<-- [start:outputs]
  bool ok = true;
  ok = ok && encoder.ConfigureInterface(true, false, false);  // ABI on, UVW off, no PWM
  ok = ok && encoder.SetABIResolution(12);       // 4096 steps, 1024 pulses per turn
  ok = ok && encoder.SetIndexPulseLength(3);     // index pulse 3 LSB wide (or 1)

  ok = ok && encoder.SetUVWPolePairs(4);         // UVW for a 4 pole-pair motor
  return ok;
  // --8<-- [end:outputs]
}

bool raw(Encoder& encoder) {
  // --8<-- [start:raw]
  AS5047U_REG::SETTINGS2 s2{};
  if (!encoder.ReadReg(s2, 2)) {                        // fields in .bits, 2 retries
    return false;
  }
  s2.bits.ABI_DEC = 1;        // decimal ABI counts (1000, 500, ... pulses)
  s2.bits.IWIDTH = 1;         // 1 LSB index pulse
  if (!encoder.WriteReg(s2, 2)) {                       // verified, 2 retries
    return false;
  }
  using Res = AS5047U_REG::SETTINGS3::ABIResolution;
  auto s3 = encoder.ReadReg<AS5047U_REG::SETTINGS3>();  // or as a Result<SETTINGS3>
  if (!s3) {
    return false;
  }
  s3.value.bits.ABIRES = static_cast<uint8_t>(Res::Code0);    // with ABI_DEC: 1000 pulses
  return encoder.WriteReg(s3.value);
  // --8<-- [end:raw]
}

bool otp(Encoder& encoder) {
  // --8<-- [start:otp]
  // The settings are in the volatile registers (configure() above), the magnet is
  // at the zero position, VDD is 4.5 to 5.5 V (5 V mode) or 3.3 to 3.5 V (3.3 V
  // mode), and the chip is at 5 to 45 C.
  as5047u::OtpResult result{};
  if (!encoder.ProgramOTP(&result)) { // current angle becomes zero, then burns. Once.
    std::printf("OTP step %u failed%s\n", static_cast<unsigned>(result.failed_step),
                result.burn_started ? ": do not use the part" : ", nothing burned");
    return false;
  }
  // Step 20: cycle the supply, then steps 21 and 22:
  return encoder.VerifyOTP(result.written);
  // --8<-- [end:otp]
}
