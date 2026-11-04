/**
 * @file quickstart.cpp
 * @brief The Quickstart page's example, compiled by the host build (never run).
 */
#include "my_spi.hpp"

// --8<-- [start:quickstart]
#include "as5047u.hpp"

#include <cstdio>

int print_angle(MySpi& spi) {
  as5047u::AS5047U<MySpi> encoder(spi, FrameFormat::SPI_24);  // CRC on every frame

  // Every getter returns a Result: .value and .error (None when the read worked).
  const auto dia = encoder.GetDiagnostics();         // 1. chip up, magnet in range?
  if (!dia || dia.value.bits.LoopsFinished == 0U || dia.value.bits.MagHalf_flag != 0U) {
    return -1;                                       // no supply, no SPI or no magnet
  }

  const auto degrees = encoder.GetAngleDegrees(2);   // 2. angle, 2 retries on CRC
  const auto rpm = encoder.GetVelocityRPM();         // 3. velocity
  const auto agc = encoder.GetAGC();
  if (!degrees || !rpm || !agc) {
    return -2;                                       // a read failed: .error says why
  }
  std::printf("%.2f deg  %.1f rpm  AGC %u\n", static_cast<double>(degrees.value),
              static_cast<double>(rpm.value), agc.value);
  return 0;
}
// --8<-- [end:quickstart]
