/**
 * @file configuration.cpp
 * @brief The Configuration page's setups, compiled by the host build (never run).
 */
#include "my_spi.hpp"

#include "as5047u.hpp"

using Encoder = as5047u::AS5047U<MySpi>;

bool commutation_setup(Encoder& encoder) {
  // --8<-- [start:commutation]
  // BLDC commutation and speed control: fast response, UVW for the gate driver, ABI for the MCU.
  bool ok = true;
  ok = ok && encoder.SetDynamicAngleCompensation(true);             // latency compensated at speed
  ok = ok && encoder.SetFilterPreset(FilterPreset::HighBandwidth);  // K = 6, fastest
  ok = ok && encoder.ConfigureInterface(true, true, false);         // ABI and UVW, no PWM
  ok = ok && encoder.SetUVWPolePairs(7);
  ok = ok && encoder.SetABIResolution(14);                          // 4096 pulses per turn
  return ok;
  // --8<-- [end:commutation]
}

bool positioning_setup(Encoder& encoder) {
  // --8<-- [start:positioning]
  // Static or slow positioning (under about 100 rpm): lowest noise.
  bool ok = true;
  ok = ok && encoder.SetDynamicAngleCompensation(false);       // saves 0.016 deg rms of noise
  ok = ok && encoder.SetFilterPreset(FilterPreset::LowNoise);  // K = 0
  ok = ok && encoder.SetHysteresis(AS5047U_REG::SETTINGS3::Hysteresis::LSB_2);
  ok = ok && encoder.ConfigureInterface(false, false, false);  // SPI only, ABI and UVW tristated
  return ok;
  // --8<-- [end:positioning]
}
