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

  const auto dia = encoder.GetDiagnostics();         // 1. chip up, magnet in range?
  if (dia.bits.LoopsFinished == 0U || dia.bits.MagHalf_flag != 0U) {
    return -1;                                       // no supply, no SPI or no magnet
  }

  const float degrees = encoder.GetAngleDegrees(2);  // 2. angle, 2 retries on CRC
  const float rpm = encoder.GetVelocityRPM();        // 3. velocity
  std::printf("%.2f deg  %.1f rpm  AGC %u\n", degrees, rpm, encoder.GetAGC());
  return 0;
}
// --8<-- [end:quickstart]
