/**
 * @file test_otp.cpp
 * @brief ProgramOTP() against the datasheet's step-by-step OTP procedure (DS000637 v1-01, page
 *        41-42) and its flowchart (Figure 60), over the chip model's OTP.
 */
#include "test_harness.hpp"

#include "mock_as5047u.hpp"

#include <utility>
#include <vector>

using Encoder = as5047u::AS5047U<MockAs5047u>;
using Write = std::pair<uint16_t, uint16_t>;

namespace {
constexpr uint16_t kProg = AS5047U_REG::PROG::ADDRESS;

// A chip after step 2: settings written, magnet at 0x1234 (step 3).
void setUp(MockAs5047u& chip) {
  chip.regs[AS5047U_REG::DISABLE::ADDRESS] = 0x0041;
  chip.regs[AS5047U_REG::SETTINGS1::ADDRESS] = 0x0013;
  chip.regs[AS5047U_REG::SETTINGS2::ADDRESS] = 0x0004;
  chip.regs[AS5047U_REG::SETTINGS3::ADDRESS] = 0x0083;
  chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 0x1234;
}

std::vector<uint16_t> progWrites(const MockAs5047u& chip) {
  std::vector<uint16_t> v;
  for (const auto& w : chip.writes) {
    if (w.first == kProg) {
      v.push_back(w.second);
    }
  }
  return v;
}
} // namespace

TEST_CASE("ProgramOTP runs steps 4 to 19 with the register writes of Figure 60") {
  MockAs5047u chip;
  setUp(chip);
  Encoder enc(chip, FrameFormat::SPI_24);
  const uint16_t key = [&] {
    MockAs5047u probe; // the key the model computes once ZPOS and ECC_en are written
    setUp(probe);
    probe.regs[AS5047U_REG::ZPOSM::ADDRESS] = 0x1234 >> 6;
    probe.regs[AS5047U_REG::ZPOSL::ADDRESS] = 0x1234 & 0x3F;
    probe.regs[AS5047U_REG::ECC::ADDRESS] = 0x80;
    return probe.eccKey();
  }();

  as5047u::OtpResult r{};
  REQUIRE(enc.ProgramOTP(&r));
  CHECK_EQ(r.failed_step, 0U);
  CHECK(r.burn_started);
  CHECK_EQ(chip.burns, 1);

  const std::vector<Write> expected = {
      {AS5047U_REG::ZPOSL::ADDRESS, 0x1234 & 0x3F}, // 5
      {AS5047U_REG::ZPOSM::ADDRESS, 0x1234 >> 6},   // 5
      {AS5047U_REG::ECC::ADDRESS, 0x0080},          // 7: ECC_en
      {AS5047U_REG::ECC::ADDRESS, 0x0080 | key},    // 9: ECC_s, ECC_en kept
      {kProg, 0x0001},                              // 12: PROGEN
      {kProg, 0x0008},                              // 13: PROGOTP
      {0x0015, 0},
      {0x0016, 0},
      {0x0017, 0},
      {0x0018, 0},
      {0x0019, 0},
      {0x001A, 0},
      {0x001B, 0},     // 15
      {kProg, 0x0040}, // 16: PROGVER
      {kProg, 0x0004}, // 17: OTPREF
  };
  CHECK(chip.writes == expected);

  const std::array<uint16_t, 7> image = {0x0041,
                                         0x1234 >> 6,
                                         0x1234 & 0x3F,
                                         0x0013,
                                         0x0004,
                                         0x0083,
                                         static_cast<uint16_t>(0x80 | key)};
  CHECK(r.written == image);
  CHECK(chip.otp == image);

  chip.powerCycle();               // step 20
  CHECK(enc.VerifyOTP(r.written)); // steps 21, 22
  chip.regs[AS5047U_REG::SETTINGS3::ADDRESS] ^= 1U;
  CHECK(!enc.VerifyOTP(r.written));
}

TEST_CASE("ProgramOTP clears the volatile copy before the guard-band test (step 15)") {
  // A weakly burned fuse reads as 0 under the guard band. Without step 15 the refresh could leave
  // the old volatile content in place and hide it; with it, step 19 sees the difference.
  MockAs5047u chip;
  setUp(chip);
  chip.weak_fuses[AS5047U_REG::SETTINGS1::ADDRESS - 0x0015] = 0x0010;
  Encoder enc(chip, FrameFormat::SPI_24);

  as5047u::OtpResult r{};
  CHECK(!enc.ProgramOTP(&r));
  CHECK_EQ(r.failed_step, 19U);
  CHECK(r.burn_started);
  const auto prog = progWrites(chip);
  REQUIRE(prog.size() == 4U);
  CHECK_EQ(prog[2], 0x0040);
  CHECK_EQ(prog[3], 0x0004);
  for (uint16_t a = 0x0015; a <= 0x001B; ++a) { // each cleared between the burn and PROGVER
    bool cleared = false;
    bool after_burn = false;
    for (const auto& w : chip.writes) {
      after_burn = after_burn || (w.first == kProg && w.second == 0x0008);
      if (w.first == kProg && w.second == 0x0040) {
        break;
      }
      cleared = cleared || (after_burn && w.first == a && w.second == 0);
    }
    CHECK(cleared);
  }
}

TEST_CASE("ProgramOTP stops before the burn when a step before it fails") {
  { // step 7: ECC_en does not take
    MockAs5047u chip;
    setUp(chip);
    chip.read_only_addr = AS5047U_REG::ECC::ADDRESS;
    Encoder enc(chip, FrameFormat::SPI_24);
    as5047u::OtpResult r{};
    CHECK(!enc.ProgramOTP(&r));
    CHECK_EQ(r.failed_step, 7U);
    CHECK(!r.burn_started);
    CHECK(progWrites(chip).empty());
    CHECK_EQ(chip.burns, 0);
  }
  { // step 5: the zero position does not take
    MockAs5047u chip;
    setUp(chip);
    chip.read_only_addr = AS5047U_REG::ZPOSM::ADDRESS;
    Encoder enc(chip, FrameFormat::SPI_24);
    as5047u::OtpResult r{};
    CHECK(!enc.ProgramOTP(&r));
    CHECK_EQ(r.failed_step, 5U);
    CHECK_EQ(chip.burns, 0);
  }
  // A bus error at any frame before step 13: the burn never starts.
  for (long at = 0; at < 60; ++at) {
    MockAs5047u chip;
    setUp(chip);
    chip.fail_transfer_at = at;
    Encoder enc(chip, FrameFormat::SPI_24);
    as5047u::OtpResult r{};
    CHECK(!enc.ProgramOTP(&r));
    CHECK(r.failed_step >= 4U && r.failed_step <= 12U);
    CHECK(!r.burn_started);
    CHECK_EQ(chip.burns, 0);
  }
}

TEST_CASE("ProgramOTP reports a burn that never completes (step 14)") {
  MockAs5047u chip;
  setUp(chip);
  chip.burn_polls = 1L << 30;
  Encoder enc(chip, FrameFormat::SPI_24);
  as5047u::OtpResult r{};
  CHECK(!enc.ProgramOTP(&r));
  CHECK_EQ(r.failed_step, 14U);
  CHECK(r.burn_started);
  CHECK_EQ(progWrites(chip).size(), 2U); // nothing after the poll
}

TEST_CASE("ProgramOTP uses 24-bit frames in 16-bit mode and restores the format") {
  MockAs5047u chip;
  setUp(chip);
  Encoder enc(chip, FrameFormat::SPI_16);
  CHECK(enc.ProgramOTP());
  CHECK_EQ(chip.framesOfLength(2), 0U);
  chip.mosi.clear();
  (void)enc.GetAngle();
  CHECK_EQ(chip.framesOfLength(2), 4U);
}
