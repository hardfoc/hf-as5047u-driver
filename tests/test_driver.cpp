/**
 * @file test_driver.cpp
 * @brief The driver's high-level calls end to end over the chip model: units, register fields,
 *        write verification and error flags.
 */
#include "test_harness.hpp"

#include "mock_as5047u.hpp"

using Encoder = as5047u::AS5047U<MockAs5047u>;

TEST_CASE("Velocity is a sign-extended 14-bit value in 24.141 deg/s per LSB") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);

  chip.regs[AS5047U_REG::VEL::ADDRESS] = 0x3FFF;  // -1
  CHECK_EQ(static_cast<int>(enc.GetVelocity()), -1);
  chip.regs[AS5047U_REG::VEL::ADDRESS] = 0x2000;  // most negative
  CHECK_EQ(static_cast<int>(enc.GetVelocity()), -8192);
  chip.regs[AS5047U_REG::VEL::ADDRESS] = 100;
  const float dps = enc.GetVelocityDegPerSec();
  CHECK(dps > 2414.0F && dps < 2414.2F);
  const float rpm = enc.GetVelocity(as5047u::VelocityUnit::Rpm);
  CHECK(rpm > 402.3F && rpm < 402.4F);
}

TEST_CASE("SetZeroPosition splits 14 bits into ZPOSM[7:0] and ZPOSL[5:0]") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);

  CHECK(enc.SetZeroPosition(0x1234));
  CHECK_EQ(chip.regs[AS5047U_REG::ZPOSM::ADDRESS], 0x1234 >> 6);
  CHECK_EQ(chip.regs[AS5047U_REG::ZPOSL::ADDRESS], 0x1234 & 0x3F);
  CHECK_EQ(enc.GetZeroPosition(), 0x1234);
}

TEST_CASE("SetABIResolution maps 10..14 bits to the datasheet ABIRES codes") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  const auto abires = [&] {
    AS5047U_REG::SETTINGS3 s3{};
    s3.value = chip.regs[AS5047U_REG::SETTINGS3::ADDRESS];
    return static_cast<unsigned>(s3.bits.ABIRES);
  };
  // Figure 51: 14 bit = 100, 13 = 011, 12 = 000, 11 = 001, 10 = 010.
  CHECK(enc.SetABIResolution(14));
  CHECK_EQ(abires(), 4U);
  CHECK(enc.SetABIResolution(13));
  CHECK_EQ(abires(), 3U);
  CHECK(enc.SetABIResolution(12));
  CHECK_EQ(abires(), 0U);
  CHECK(enc.SetABIResolution(11));
  CHECK_EQ(abires(), 1U);
  CHECK(enc.SetABIResolution(9));  // clamped to 10
  CHECK_EQ(abires(), 2U);
}

TEST_CASE("Outputs, direction and filter land in the right register bits") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);

  CHECK(enc.SetUVWPolePairs(7));
  AS5047U_REG::SETTINGS3 s3{};
  s3.value = chip.regs[AS5047U_REG::SETTINGS3::ADDRESS];
  CHECK_EQ(static_cast<unsigned>(s3.bits.UVWPP), 6U);

  CHECK(enc.SetDirection(false));
  AS5047U_REG::SETTINGS2 s2{};
  s2.value = chip.regs[AS5047U_REG::SETTINGS2::ADDRESS];
  CHECK_EQ(static_cast<unsigned>(s2.bits.DIR), 1U);

  CHECK(enc.ConfigureInterface(true, false, true));  // ABI on, UVW off, PWM on W
  AS5047U_REG::DISABLE dis{};
  dis.value = chip.regs[AS5047U_REG::DISABLE::ADDRESS];
  s2.value = chip.regs[AS5047U_REG::SETTINGS2::ADDRESS];
  CHECK_EQ(static_cast<unsigned>(dis.bits.ABI_off), 0U);
  CHECK_EQ(static_cast<unsigned>(dis.bits.UVW_off), 1U);
  CHECK_EQ(static_cast<unsigned>(s2.bits.UVW_ABI), 0U);
  CHECK_EQ(static_cast<unsigned>(s2.bits.PWMon), 1U);

  CHECK(enc.SetFilterPreset(FilterPreset::LowNoise));  // K_min code 5, K_max code 6: K = 0
  AS5047U_REG::SETTINGS1 s1{};
  s1.value = chip.regs[AS5047U_REG::SETTINGS1::ADDRESS];
  CHECK_EQ(static_cast<unsigned>(s1.bits.K_min), 5U);
  CHECK_EQ(static_cast<unsigned>(s1.bits.K_max), 6U);
  CHECK(enc.GetAdaptiveFilterEnabled());
}

TEST_CASE("A write the chip does not take is reported as false") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  chip.regs[AS5047U_REG::AGC::ADDRESS] = 5;  // read-only register
  AS5047U_REG::AGC agc{};
  agc.value = 9;
  CHECK(!enc.WriteReg(agc));
}

TEST_CASE("ERRFL flags seen during ReadReg are kept until ClearStickyErrorFlags") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  chip.regs[AS5047U_REG::ERRFL::ADDRESS] = 1U << 0;  // AGC warning

  (void)enc.ReadReg<AS5047U_REG::MAG>();
  const auto agc = static_cast<uint16_t>(AS5047U_Error::AgcWarning);
  CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()), agc);
  CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()), agc);   // reading does not clear
  CHECK_EQ(chip.regs[AS5047U_REG::ERRFL::ADDRESS], 0U);              // the chip's copy is gone
  CHECK_EQ(static_cast<uint16_t>(enc.ClearStickyErrorFlags()), agc); // returns what it cleared
  CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()), 0U);
}

TEST_CASE("The measurement getters leave the sticky flags to the caller") {
  // Datasheet Figure 33: reading ERRFL clears it, so the driver's cache is the only copy left.
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  const auto agc = static_cast<uint16_t>(AS5047U_Error::AgcWarning);
  const auto half = static_cast<uint16_t>(AS5047U_Error::MagHalf);

  chip.regs[AS5047U_REG::ERRFL::ADDRESS] = 1U << 0;
  (void)enc.GetAngle();
  CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()), agc);

  chip.regs[AS5047U_REG::ERRFL::ADDRESS] = 1U << 1;
  (void)enc.GetVelocity();
  (void)enc.GetAGC();
  (void)enc.GetMagnitude();
  (void)enc.GetZeroPosition();
  (void)enc.GetAdaptiveFilterEnabled();
  (void)enc.GetFilterParameters();
  CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()), agc | half); // accumulated
}

TEST_CASE("Every ERRFL value the driver reads reaches the sticky flags") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  const auto crc = static_cast<uint16_t>(AS5047U_Error::CrcError);

  chip.regs[AS5047U_REG::ERRFL::ADDRESS] = 1U << 6;
  CHECK_EQ(enc.GetErrorFlags(), crc);
  CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()), crc);
  CHECK_EQ(enc.GetErrorFlags(), 0U); // cleared on the chip by the first read
  (void)enc.ClearStickyErrorFlags();

  chip.regs[AS5047U_REG::ERRFL::ADDRESS] = 1U << 9; // offset compensation not finished
  CHECK_EQ(enc.ReadReg<AS5047U_REG::ERRFL>().value, 1U << 9);
  CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()),
           static_cast<uint16_t>(AS5047U_Error::OffCompError));
}

TEST_CASE("A getter retries on a CRC error of its own read and keeps the flag") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 77;
  chip.regs[AS5047U_REG::ERRFL::ADDRESS] = 1U << 6; // CRC error reported for the first read

  chip.mosi.clear();
  CHECK_EQ(enc.GetAngle(1), 77);
  CHECK_EQ(chip.mosi.size(), 8U); // two reads of four frames
  CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()),
           static_cast<uint16_t>(AS5047U_Error::CrcError));

  chip.mosi.clear();
  CHECK_EQ(enc.GetAngle(1), 77); // no error this time: one read
  CHECK_EQ(chip.mosi.size(), 4U);
}

TEST_CASE("Angle output source selects what address 0x3FFF returns") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 100;
  chip.regs[AS5047U_REG::ANGLEUNC::ADDRESS] = 200;

  CHECK_EQ(enc.GetAngle(), 100);
  CHECK(enc.SetAngleOutputSource(AS5047U_REG::SETTINGS2::AngleOutputSource::UseANGLEUNC));
  CHECK_EQ(enc.GetAngle(), 200);
  CHECK(enc.GetAngleOutputSource() == AS5047U_REG::SETTINGS2::AngleOutputSource::UseANGLEUNC);
}
