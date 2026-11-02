/**
 * @file configuration.cpp
 * @brief The Configuration page's setups, compiled by the host build (never run).
 */
#include "my_spi.hpp"

#include "as5047u.hpp"

using Encoder = as5047u::AS5047U<MySpi>;

bool commutation_setup(Encoder& encoder) {
  // --8<-- [start:commutation]
  // BLDC commutation and speed: fast response, UVW to the gate driver, ABI to the MCU.
  bool ok = true;
  ok = ok && encoder.SetDynamicAngleCompensation(true);       // latency compensated
  ok = ok && encoder.SetFilterPreset(FilterPreset::HighBandwidth);  // K = 6
  ok = ok && encoder.ConfigureInterface(true, true, false);   // ABI and UVW, no PWM
  ok = ok && encoder.SetUVWPolePairs(7);
  ok = ok && encoder.SetABIResolution(14);                    // 4096 pulses per turn
  return ok;
  // --8<-- [end:commutation]
}

bool positioning_setup(Encoder& encoder) {
  // --8<-- [start:positioning]
  // Static or slow positioning (under about 100 rpm): lowest noise.
  using Hys = AS5047U_REG::SETTINGS3::Hysteresis;
  bool ok = true;
  ok = ok && encoder.SetDynamicAngleCompensation(false);      // 0.016 deg rms less
  ok = ok && encoder.SetFilterPreset(FilterPreset::LowNoise); // K = 0
  ok = ok && encoder.SetHysteresis(Hys::LSB_2);
  ok = ok && encoder.ConfigureInterface(false, false, false); // SPI only, rest off
  return ok;
  // --8<-- [end:positioning]
}
