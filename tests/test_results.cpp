/**
 * @file test_results.cpp
 * @brief Every value getter carries the error of its read: a bus error, a corrupted reply or a
 *        CRC/framing error reported by the chip is never returned as a plain 0 or a stale value.
 */
#include "test_harness.hpp"

#include "mock_as5047u.hpp"

#include <type_traits>

using Encoder = as5047u::AS5047U<MockAs5047u>;

namespace {
constexpr auto kNone = AS5047U_Error::None;
constexpr auto kBus = AS5047U_Error::BusError;

// The getters return as5047u::Result<T>; there is no overload that returns the bare value.
static_assert(
    std::is_same_v<decltype(std::declval<const Encoder&>().GetAngle()), as5047u::Result<uint16_t>>);
static_assert(std::is_same_v<decltype(std::declval<const Encoder&>().GetVelocity()),
                             as5047u::Result<int16_t>>);
static_assert(std::is_same_v<decltype(std::declval<const Encoder&>().GetAngleDegrees()),
                             as5047u::Result<float>>);
static_assert(std::is_same_v<decltype(std::declval<const Encoder&>().ReadReg<AS5047U_REG::DIA>()),
                             as5047u::Result<AS5047U_REG::DIA>>);
} // namespace

TEST_CASE("A good read returns the value with no error") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 1234;

  const auto angle = enc.GetAngle();
  CHECK(angle.ok());
  CHECK(static_cast<bool>(angle));
  CHECK(angle.error == kNone);
  CHECK_EQ(angle.value, 1234);
}

TEST_CASE("Every value getter reports a bus error instead of returning 0") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 1234;
  chip.regs[AS5047U_REG::VEL::ADDRESS] = 100;
  chip.transfers_before_failure = 0; // the bus is down

  CHECK(enc.GetAngle().error == kBus);
  CHECK(enc.GetAngle(as5047u::AngleUnit::Lsb).error == kBus);
  CHECK(enc.GetAngle(as5047u::AngleUnit::Degrees).error == kBus);
  CHECK(enc.GetAngle(as5047u::AngleUnit::Radians).error == kBus);
  CHECK(enc.GetAngleDegrees().error == kBus);
  CHECK(enc.GetAngleRadians().error == kBus);
  CHECK(enc.GetRawAngle().error == kBus);
  CHECK(enc.GetVelocity().error == kBus);
  CHECK(enc.GetVelocity(as5047u::VelocityUnit::Lsb).error == kBus);
  CHECK(enc.GetVelocity(as5047u::VelocityUnit::DegPerSec).error == kBus);
  CHECK(enc.GetVelocity(as5047u::VelocityUnit::RadPerSec).error == kBus);
  CHECK(enc.GetVelocity(as5047u::VelocityUnit::Rpm).error == kBus);
  CHECK(enc.GetVelocityDegPerSec().error == kBus);
  CHECK(enc.GetVelocityRadPerSec().error == kBus);
  CHECK(enc.GetVelocityRPM().error == kBus);
  CHECK(enc.GetAGC().error == kBus);
  CHECK(enc.GetMagnitude().error == kBus);
  CHECK(enc.GetErrorFlags().error == kBus);
  CHECK(enc.GetZeroPosition().error == kBus);
  CHECK(enc.GetAdaptiveFilterEnabled().error == kBus);
  CHECK(enc.GetFilterParameters().error == kBus);
  CHECK(enc.GetHysteresis().error == kBus);
  CHECK(enc.GetAngleOutputSource().error == kBus);
  CHECK(enc.GetDiagnostics().error == kBus);
  CHECK(enc.ReadReg<AS5047U_REG::SETTINGS2>().error == kBus);
  CHECK(!enc.GetAngle().ok());
  CHECK(!static_cast<bool>(enc.GetAngle()));
}

TEST_CASE("A failed read after a good one is not reported as the old value") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 1234;
  CHECK_EQ(enc.GetAngle().value, 1234);

  chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 4321;
  chip.transfers_before_failure = 1; // the NOP that carries the data fails
  const auto angle = enc.GetAngle();
  CHECK(!angle.ok());
  CHECK(angle.error == kBus);
}

TEST_CASE("A chip CRC error on the last attempt is the getter's error") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 77;
  chip.regs[AS5047U_REG::ERRFL::ADDRESS] = 1U << 6; // CRC error reported for this read

  const auto angle = enc.GetAngle(0); // no retry
  CHECK(angle.error == AS5047U_Error::CrcError);
  CHECK(!angle.ok());

  chip.regs[AS5047U_REG::ERRFL::ADDRESS] = 1U << 6;
  const auto retried = enc.GetAngle(1); // the retry goes through
  CHECK(retried.ok());
  CHECK_EQ(retried.value, 77);
}

TEST_CASE("A corrupted reply is the getter's error") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  chip.regs[AS5047U_REG::MAG::ADDRESS] = 4800;
  chip.corrupt_reply_at = 1; // the NOP that carries the data
  const auto mag = enc.GetMagnitude();
  CHECK(mag.error == AS5047U_Error::ReplyCrcError);
}

TEST_CASE("GetZeroPosition fails when either half fails") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  CHECK(enc.SetZeroPosition(0x1234));
  chip.transfers = 0;
  chip.fail_transfer_at = 4; // the first transfer of the ZPOSL read
  const auto zero = enc.GetZeroPosition();
  CHECK(zero.error == kBus);
}

TEST_CASE("GetErrorFlags keeps what it read and reports a read that never succeeded") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  chip.regs[AS5047U_REG::ERRFL::ADDRESS] = 1U << 0;
  const auto flags = enc.GetErrorFlags();
  CHECK(flags.ok());
  CHECK_EQ(flags.value, static_cast<uint16_t>(AS5047U_Error::AgcWarning));

  chip.transfers_before_failure = 0;
  CHECK(enc.GetErrorFlags(2).error == kBus);
}
