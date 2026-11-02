/**
 * @file my_spi.hpp
 * @brief The SPI port shown on the documentation's Porting page, compiled by the host build.
 *
 * Every platform call goes through the `platform_*` functions declared below; a real port
 * replaces them with its SDK (ESP-IDF, STM32 HAL, Zephyr, Linux spidev). The class is what the
 * driver needs, nothing more.
 */
#pragma once

#include "as5047u_spi_interface.hpp"

#include <cstddef>
#include <cstdint>

// Platform hooks: your SDK's calls. Declared only, so the docs build checks the port compiles.
void platform_cs_low() noexcept;
void platform_cs_high() noexcept;
void platform_spi_exchange(const uint8_t* tx, uint8_t* rx, std::size_t len) noexcept;  // mode 1, MSB first

// --8<-- [start:port]
class MySpi : public as5047u::SpiInterface<MySpi> {
public:
  // One AS5047U frame: 2, 3 or 4 bytes, full duplex, CSn low for the whole frame.
  // SPI mode 1 (CPOL = 0, CPHA = 1), MSB first, up to 10 MHz.
  void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    platform_cs_low();
    platform_spi_exchange(tx, rx, len);
    platform_cs_high();  // the chip executes the frame on this edge; keep CSn high >= 350 ns
  }
};
// --8<-- [end:port]
