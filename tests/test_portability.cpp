/**
 * @file test_portability.cpp
 * @brief The driver builds without the non-standard M_PI and prints nothing from its write path.
 *
 * This translation unit includes the driver with M_PI undefined (as with MSVC without
 * _USE_MATH_DEFINES, or newlib in strict ISO mode) and with printf replaced by a counter.
 */
#include "test_harness.hpp"

#include <cmath>
#include <cstdio>

#undef M_PI

namespace {
int g_printf_calls = 0;
template <typename... Args>
int countPrintf(Args&&... /*args*/) {
  return ++g_printf_calls;
}
} // namespace

#define printf(...) countPrintf(__VA_ARGS__)
#include "mock_as5047u.hpp"
#undef printf

namespace {
// A port type of its own, so that every driver member this file uses is instantiated here, with the
// printf counter, and not shared with the other test files.
class CountingPort : public as5047u::SpiInterface<CountingPort> {
public:
  MockAs5047u chip;
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    return chip.transfer(tx, rx, len);
  }
};
} // namespace

using Encoder = as5047u::AS5047U<CountingPort>;

TEST_CASE("Unit constants do not need M_PI") {
  static_assert(as5047u::kPi == static_cast<float>(3.14159265358979323846L));
  static_assert(Encoder::Angle::RadiansToLsb(as5047u::kPi) == 8192);
  CHECK(Encoder::Angle::RAD_PER_LSB > 3.8349e-4F && Encoder::Angle::RAD_PER_LSB < 3.8350e-4F);
  CHECK(Encoder::Velocity::RAD_PER_LSB > 0.42133F && Encoder::Velocity::RAD_PER_LSB < 0.42134F);
}

TEST_CASE("A failed write prints nothing") {
  CountingPort port;
  Encoder enc(port, FrameFormat::SPI_24);
  AS5047U_REG::AGC agc{}; // read-only: the read-back never matches
  agc.value = 9;
  g_printf_calls = 0;
  CHECK(!enc.WriteReg(agc, 2));
  CHECK_EQ(g_printf_calls, 0);
}
