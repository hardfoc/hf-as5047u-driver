/**
 * @file test_frames.cpp
 * @brief CRC-8 and the SPI frames the driver puts on the bus in 16-, 24- and 32-bit mode.
 */
#include "test_harness.hpp"

#include "mock_as5047u.hpp"

using Encoder = as5047u::AS5047U<MockAs5047u>;

namespace {
// Independent reference: CRC-8, polynomial 0x1D, init 0xC4, no reflection, final XOR 0xFF,
// computed a byte at a time (datasheet Figure 31).
uint8_t referenceCrc8(uint16_t word) {
  uint8_t crc = 0xC4;
  const uint8_t bytes[2] = {static_cast<uint8_t>(word >> 8), static_cast<uint8_t>(word & 0xFF)};
  for (uint8_t b : bytes) {
    crc ^= b;
    for (int i = 0; i < 8; ++i) {
      crc = (crc & 0x80U) ? static_cast<uint8_t>((crc << 1) ^ 0x1DU) : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc ^ 0xFF;
}
} // namespace

TEST_CASE("ComputeCRC8 matches the datasheet CRC for every 16-bit payload") {
  int mismatches = 0;
  for (uint32_t w = 0; w <= 0xFFFF; ++w) {
    mismatches += (Encoder::ComputeCRC8(static_cast<uint16_t>(w)) != referenceCrc8(static_cast<uint16_t>(w))) ? 1 : 0;
  }
  CHECK_EQ(mismatches, 0);
  static_assert(Encoder::ComputeCRC8(0x0000) != 0x00, "an all-zero frame must not carry a valid CRC");
}

TEST_CASE("24-bit read: command, NOP, then ERRFL and NOP, each with its CRC") {
  MockAs5047u chip;
  chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 0x1234;
  Encoder enc(chip, FrameFormat::SPI_24);

  CHECK_EQ(enc.GetAngle().value, 0x1234);
  REQUIRE(chip.mosi.size() == 4U);
  CHECK_EQ(chip.framesOfLength(3), 4U);
  // Read ANGLECOM: bit 22 = 1 (read), address 0x3FFF, CRC over bits 23:8.
  CHECK_EQ(chip.mosi[0][0], 0x7F);
  CHECK_EQ(chip.mosi[0][1], 0xFF);
  CHECK_EQ(chip.mosi[0][2], MockAs5047u::crc8(0x7FFF));
  // The second frame is a read of NOP (0x0000); its reply carries ANGLECOM.
  CHECK_EQ(chip.mosi[1][0], 0x40);
  CHECK_EQ(chip.mosi[1][1], 0x00);
  // Then ERRFL (0x0001) to refresh the sticky error flags.
  CHECK_EQ(chip.mosi[2][0], 0x40);
  CHECK_EQ(chip.mosi[2][1], 0x01);
}

TEST_CASE("16-bit read: two-byte frames, no CRC") {
  MockAs5047u chip;
  chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 8192;
  Encoder enc(chip, FrameFormat::SPI_16);

  CHECK_EQ(enc.GetAngle().value, 8192);
  CHECK_EQ(chip.framesOfLength(2), 4U);
  CHECK_EQ(chip.mosi[0][0], 0x7F);
  CHECK_EQ(chip.mosi[0][1], 0xFF);
  const float deg = enc.GetAngleDegrees().value;
  CHECK(deg > 179.99F && deg < 180.01F);
}

TEST_CASE("32-bit read: the PAD byte leads every MOSI frame") {
  MockAs5047u chip;
  chip.regs[AS5047U_REG::MAG::ADDRESS] = 4800;
  Encoder enc(chip, FrameFormat::SPI_32);
  enc.SetPad(0xA5);

  CHECK_EQ(enc.GetMagnitude().value, 4800);
  CHECK_EQ(chip.framesOfLength(4), 4U);
  for (const auto& f : chip.mosi) {
    CHECK_EQ(f[0], 0xA5);
  }
  CHECK_EQ(chip.mosi[0][1], 0x7F);  // MAG = 0x3FFD, read
  CHECK_EQ(chip.mosi[0][2], 0xFD);
}

TEST_CASE("Writes in 16-bit mode are sent as 24-bit frames with CRC") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_16);
  AS5047U_REG::ZPOSM m{};
  m.bits.ZPOSM_bits = 0x48;

  chip.mosi.clear();
  CHECK(enc.WriteReg(m));
  REQUIRE(chip.mosi.size() >= 3U);
  CHECK_EQ(chip.mosi[0].size(), 3U);
  CHECK_EQ(chip.mosi[0][0], 0x00);  // bit 22 = 0: write, address 0x0016
  CHECK_EQ(chip.mosi[0][1], 0x16);
  CHECK_EQ(chip.mosi[1][1], 0x48);  // data frame
  CHECK_EQ(chip.regs[AS5047U_REG::ZPOSM::ADDRESS], 0x48);
}

TEST_CASE("24-bit read: a reply with a wrong CRC is rejected and retried") {
  // Figure 25 and "CRC Checksum": the MISO frame carries a CRC over bits 23:8.
  MockAs5047u chip;
  chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 0x1234;
  Encoder enc(chip, FrameFormat::SPI_24);

  chip.corrupt_reply_at = 1; // the NOP reply that carries ANGLECOM
  CHECK_EQ(enc.GetAngle(1).value, 0x1234);
  CHECK_EQ(chip.transfers, 6U); // 2 frames rejected at the bad reply, then a full read of 4
  CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()),
           static_cast<uint16_t>(AS5047U_Error::ReplyCrcError));

  chip.transfers = 0;
  chip.corrupt_reply_at = 1;
  AS5047U_REG::ANGLECOM a{};
  CHECK(!enc.ReadReg(a)); // no retry: the corrupted value is not returned as good
  CHECK_EQ(a.value, 0U);
}

TEST_CASE("32-bit read: the reply CRC covers bits 31:16, not the PAD") {
  MockAs5047u chip;
  chip.regs[AS5047U_REG::MAG::ADDRESS] = 4800;
  Encoder enc(chip, FrameFormat::SPI_32);
  enc.SetPad(0x5A);

  CHECK_EQ(enc.GetMagnitude().value, 4800);
  CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()), 0U); // PAD does not upset the CRC

  chip.corrupt_reply_at = static_cast<long>(chip.transfers) + 1;
  const auto corrupted = enc.GetMagnitude();
  CHECK(corrupted.error == AS5047U_Error::ReplyCrcError);
  CHECK_EQ(corrupted.value, 0U);
  CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()),
           static_cast<uint16_t>(AS5047U_Error::ReplyCrcError));
}

TEST_CASE("A write whose old-content reply is corrupted is retried") {
  MockAs5047u chip;
  Encoder enc(chip, FrameFormat::SPI_24);
  AS5047U_REG::ZPOSM m{};
  m.bits.ZPOSM_bits = 0x48;

  chip.corrupt_reply_at = 1; // the data frame's reply
  CHECK(!enc.WriteReg(m));
  chip.transfers = 0;
  chip.corrupt_reply_at = 1;
  CHECK(enc.WriteReg(m, 1));
  CHECK_EQ(chip.regs[AS5047U_REG::ZPOSM::ADDRESS], 0x48);
  CHECK(static_cast<uint16_t>(enc.GetStickyErrorFlags()) &
        static_cast<uint16_t>(AS5047U_Error::ReplyCrcError));
}

TEST_CASE("The warning and error bits of the reply reach the sticky flags") {
  // Figures 22, 25 and 28: MISO bit 15 / 23 / 31 is the warning bit, bit 14 / 22 / 30 the error
  // bit.
  const auto warn = static_cast<uint16_t>(AS5047U_Error::ReplyWarning);
  const auto err = static_cast<uint16_t>(AS5047U_Error::ReplyError);
  for (const auto format : {FrameFormat::SPI_16, FrameFormat::SPI_24, FrameFormat::SPI_32}) {
    MockAs5047u chip;
    chip.regs[AS5047U_REG::ANGLECOM::ADDRESS] = 0x2AAA;
    Encoder enc(chip, format);

    chip.reply_flags = 0x8000;
    CHECK_EQ(enc.GetAngle().value, 0x2AAA); // the data is still good; the bits are not data
    CHECK_EQ(static_cast<uint16_t>(enc.ClearStickyErrorFlags()), warn);

    chip.reply_flags = 0x4000;
    CHECK_EQ(enc.GetAngle().value, 0x2AAA);
    CHECK_EQ(static_cast<uint16_t>(enc.ClearStickyErrorFlags()), err);

    chip.reply_flags = 0;
    CHECK(enc.SetDirection(false)); // a write with clean replies sets neither
    CHECK_EQ(static_cast<uint16_t>(enc.GetStickyErrorFlags()), 0U);
  }

  MockAs5047u chip; // a MOSI frame with a wrong CRC: the chip sets the error bit in the next reply
  Encoder enc(chip, FrameFormat::SPI_24);
  const uint8_t bad[3] = {0x7F, 0xFF, 0x00};
  uint8_t rx[3] = {};
  (void)chip.transfer(bad, rx, 3);
  (void)enc.GetAngle();
  CHECK(static_cast<uint16_t>(enc.GetStickyErrorFlags()) & err);
  CHECK(static_cast<uint16_t>(enc.GetStickyErrorFlags()) &
        static_cast<uint16_t>(AS5047U_Error::CrcError));
}
