/**
 * @file mock_as5047u.hpp
 * @brief A model of the AS5047U SPI slave for the host unit tests.
 *
 * It follows the datasheet (DS000637 v1-01, "SPI Transaction"):
 * - The frame length (2, 3 or 4 bytes) selects the 16-, 24- or 32-bit format.
 * - The reply to a read command is clocked out during the *next* frame.
 * - A write is a command frame followed by a data frame; MISO carries the old content during the
 *   data frame and the new content during the frame after it.
 * - 24- and 32-bit MOSI frames carry a CRC-8 (polynomial 0x1D, initial value 0xC4, final XOR 0xFF)
 *   over bits 23:8; a wrong CRC sets ERRFL.CRC_error and the command is not executed.
 * - Reading ERRFL clears it.
 *
 * Every MOSI frame is logged so tests can check the exact bytes the driver sends. On demand a
 * transfer fails (the port's bus-error return), a reply is corrupted on MISO (a data bit flips after
 * the CRC was computed) or the replies carry the warning / error bits.
 */
#pragma once

#include "as5047u.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

class MockAs5047u : public as5047u::SpiInterface<MockAs5047u> {
public:
  /// Register file, indexed by 14-bit address.
  std::array<uint16_t, 0x4000> regs{};

  /// Every MOSI frame the driver sent, in order.
  std::vector<std::vector<uint8_t>> mosi;

  /// Addresses at or above this one ignore writes (the read-only output registers).
  static constexpr uint16_t kFirstReadOnly = 0x3FD0;

  static uint8_t crc8(uint16_t word) noexcept {
    return as5047u::AS5047U<MockAs5047u>::ComputeCRC8(word);
  }

  // --- bus fault injection ------------------------------------------------
  /// Number of transfers that still succeed; once it reaches 0 every transfer fails (the frame is
  /// not clocked: the chip does not see it and rx is left untouched). Negative: never fail.
  int transfers_before_failure = -1;
  /// Index (0-based, counting every call) of one transfer that fails; the bus works again after it.
  /// Negative: none.
  long fail_transfer_at = -1;
  std::size_t transfers = 0;        ///< transfer() calls so far
  std::size_t failed_transfers = 0; ///< transfers that reported a bus error

  // --- MISO fault injection -------------------------------------------------
  /// Index of one transfer whose MISO data bit 0 flips on the wire after the chip computed the CRC
  /// (a bit error on MISO). Negative: none.
  long corrupt_reply_at = -1;
  /// ORed into every MISO word: bit 15 = warning, bit 14 = error (DS Figures 22, 25, 28).
  uint16_t reply_flags = 0;

  // --- OTP model (datasheet page 41-43, Figure 60) ----------------------------
  static constexpr uint16_t kOtpFirst = 0x0015; ///< DISABLE; the OTP holds 0x0015..0x001B
  static constexpr std::size_t kOtpCount = 7;
  std::array<uint16_t, kOtpCount> otp{}; ///< the fuses, as burned
  int burns = 0;                         ///< burn cycles started (PROGOTP with PROGEN set before)
  long burn_polls = 3;                   ///< reads of PROG until it shows 0x0001 after a burn
  /// Bits that burn weakly: they read back correctly after a power-on cycle but as 0 when the OTP
  /// is refreshed with the guard band (PROGVER) set, which is what the guard-band test catches.
  std::array<uint16_t, kOtpCount> weak_fuses{};
  uint16_t read_only_addr = 0xFFFF; ///< one more register that ignores writes (to fail a step)
  std::vector<std::pair<uint16_t, uint16_t>> writes; ///< every executed write (address, value)

  /// Power-on cycle: volatile state is lost, the non-volatile registers load from the OTP.
  void powerCycle() {
    regs.fill(0);
    for (std::size_t i = 0; i < kOtpCount; ++i) {
      regs[kOtpFirst + i] = otp[i];
    }
    busy_ = 0;
    guard_band_ = false;
    expect_data_ = false;
    pending_ = 0;
  }

  /// Value the model returns for ECC_s (0x3FD0): any function of the settings will do.
  [[nodiscard]] uint16_t eccKey() const {
    uint16_t k = 0x2B;
    for (std::size_t i = 0; i + 1 < kOtpCount; ++i) {
      k = static_cast<uint16_t>((k * 31U + regs[kOtpFirst + i]) & 0x7FU);
    }
    return k;
  }

  /// CRTP contract: one full-duplex frame; false on a bus error.
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    const auto index = static_cast<long>(transfers++);
    if (transfers_before_failure == 0 || index == fail_transfer_at) {
      ++failed_transfers;
      return false;
    }
    if (transfers_before_failure > 0) {
      --transfers_before_failure;
    }
    frame(tx, rx, len);
    if (index == corrupt_reply_at) {
      rx[1] ^= 0x01U;
    }
    return true;
  }

  /// Number of frames of a given length the driver sent.
  [[nodiscard]] std::size_t framesOfLength(std::size_t len) const {
    std::size_t n = 0;
    for (const auto& f : mosi) {
      n += (f.size() == len) ? 1U : 0U;
    }
    return n;
  }

private:
  void frame(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    mosi.emplace_back(tx, tx + len);

    // MISO: the word prepared at the end of the previous frame.
    const auto out = static_cast<uint16_t>(pending_ | reply_flags);
    if (len == 2) {
      rx[0] = static_cast<uint8_t>(out >> 8);
      rx[1] = static_cast<uint8_t>(out & 0xFF);
    } else if (len == 3 || len == 4) {
      rx[0] = static_cast<uint8_t>(out >> 8);
      rx[1] = static_cast<uint8_t>(out & 0xFF);
      rx[2] = crc8(out);
      if (len == 4) {
        rx[3] = tx[0];  // PAD
      }
    }

    // MOSI: decode and execute at the end of the frame (rising edge of CSn).
    uint16_t word = 0;
    if (len == 2) {
      word = static_cast<uint16_t>((tx[0] << 8) | tx[1]);
    } else if (len == 3 || len == 4) {
      const std::size_t o = len - 3;  // skip the PAD byte of a 32-bit frame
      word = static_cast<uint16_t>((tx[o] << 8) | tx[o + 1]);
      if (tx[o + 2] != crc8(word)) {
        regs[AS5047U_REG::ERRFL::ADDRESS] |= 1U << 6;  // CRC_error
        expect_data_ = false;
        pending_ = 0x4000;  // error bit on the next MISO frame
        return;
      }
    } else {
      regs[AS5047U_REG::ERRFL::ADDRESS] |= 1U << 4;  // Framing_error
      return;
    }

    if (expect_data_) {
      expect_data_ = false;
      const auto value = static_cast<uint16_t>(word & 0x3FFF);
      writes.emplace_back(write_addr_, value);
      if (write_addr_ == AS5047U_REG::PROG::ADDRESS) {
        progWrite(value);
      } else if (write_addr_ < kFirstReadOnly && write_addr_ != read_only_addr) {
        regs[write_addr_] = value;
      }
      pending_ = regs[write_addr_];  // new content, shown during the next frame
      return;
    }

    const auto addr = static_cast<uint16_t>(word & 0x3FFF);
    const bool read = (word & 0x4000) != 0;
    if (read) {
      pending_ = readRegister(addr);
    } else {
      write_addr_ = addr;
      expect_data_ = true;
      pending_ = regs[addr];  // old content, shown during the data frame
    }
  }

  void progWrite(uint16_t value) {
    const uint16_t before = regs[AS5047U_REG::PROG::ADDRESS];
    regs[AS5047U_REG::PROG::ADDRESS] = value;
    if ((value & 0x0008U) != 0U && (before & 0x0001U) != 0U) { // PROGOTP after PROGEN: burn
      ++burns;
      for (std::size_t i = 0; i < kOtpCount; ++i) {
        otp[i] = regs[kOtpFirst + i];
      }
      busy_ = burn_polls;
    }
    if ((value & 0x0040U) != 0U) { // PROGVER: guard band
      guard_band_ = true;
    }
    if ((value & 0x0004U) != 0U) { // OTPREF: refresh from the OTP; the bit clears itself
      for (std::size_t i = 0; i < kOtpCount; ++i) {
        regs[kOtpFirst + i] = guard_band_ ? static_cast<uint16_t>(otp[i] & ~weak_fuses[i]) : otp[i];
      }
      regs[AS5047U_REG::PROG::ADDRESS] = static_cast<uint16_t>(value & ~0x0004U);
    }
  }

  uint16_t readRegister(uint16_t addr) {
    if (addr == AS5047U_REG::PROG::ADDRESS && busy_ > 0) {
      if (--busy_ == 0) {
        regs[addr] = 0x0001; // programming procedure complete
      }
      return regs[addr];
    }
    if (addr == AS5047U_REG::ECC_Checksum::ADDRESS) {
      return eccKey();
    }
    if (addr == AS5047U_REG::ERRFL::ADDRESS) {
      const uint16_t v = regs[addr];
      regs[addr] = 0;  // reading ERRFL clears it
      return v;
    }
    if (addr == AS5047U_REG::ANGLECOM::ADDRESS) {
      AS5047U_REG::SETTINGS2 s2{};
      s2.value = regs[AS5047U_REG::SETTINGS2::ADDRESS];
      return s2.bits.Data_select ? regs[AS5047U_REG::ANGLEUNC::ADDRESS] : regs[addr];
    }
    return regs[addr];
  }

  uint16_t pending_ = 0;
  long busy_ = 0;
  bool guard_band_ = false;
  bool expect_data_ = false;
  uint16_t write_addr_ = 0;
};
