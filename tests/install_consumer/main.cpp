/**
 * @file main.cpp
 * @brief Compiles every public header from an installed copy of the driver.
 */
#include "as5047u.hpp"
#include "as5047u_config.hpp"
#include "as5047u_registers.hpp"
#include "as5047u_spi_interface.hpp"
#include "as5047u_types.hpp"
#include "as5047u_version.h"

class InstalledPort : public as5047u::SpiInterface<InstalledPort> {
public:
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    (void)tx;
    (void)rx;
    (void)len;
    return true;
  }
};

template class as5047u::AS5047U<InstalledPort>;
