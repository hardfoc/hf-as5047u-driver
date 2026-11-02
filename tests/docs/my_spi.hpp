/**
 * @file my_spi.hpp
 * @brief The SPI port shown on the Porting page, compiled by the host build.
 *
 * Every platform call goes through the `platform_*` functions declared below; a
 * real port replaces them with its SDK (ESP-IDF, STM32 HAL, Zephyr, spidev).
 */
#pragma once

#include "as5047u_spi_interface.hpp"

#include <cstddef>
#include <cstdint>

// Platform hooks: your SDK's calls. Declared only, so the port compiles.
void platform_cs_low() noexcept;
void platform_cs_high() noexcept;
// Full duplex, SPI mode 1, MSB first.
void platform_spi_exchange(const uint8_t* tx, uint8_t* rx, std::size_t len) noexcept;

// --8<-- [start:port]
class MySpi : public as5047u::SpiInterface<MySpi> {
public:
  // One frame of 2, 3 or 4 bytes, full duplex, CSn low for the whole frame.
  // SPI mode 1 (CPOL = 0, CPHA = 1), MSB first, up to 10 MHz.
  void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    platform_cs_low();
    platform_spi_exchange(tx, rx, len);
    platform_cs_high();  // the chip executes the frame here; >= 350 ns high
  }
};
// --8<-- [end:port]
