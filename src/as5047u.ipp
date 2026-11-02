/**
 * @file as5047u.ipp
 * @brief Template implementation of AS5047U driver methods
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#ifndef AS5047U_IMPL
#define AS5047U_IMPL

#ifdef AS5047U_HEADER_INCLUDED
#include "../inc/as5047u.hpp"
#else
#include "../inc/as5047u.hpp"
#endif

#include <iomanip> // for std::hex and formatting

namespace as5047u {

// Member function definitions
template <typename SpiType>
void AS5047U<SpiType>::SetFrameFormat(FrameFormat format) noexcept {
  this->frame_format_ = format;
}

// ══════════════════════════════════════════════════════════════════════════════════════════
//                                PRIVATE HELPERS
// ══════════════════════════════════════════════════════════════════════════════════════════

// Helper: encode/decode register value

// Encoding/decoding helpers are defined inline in the header

// ══════════════════════════════════════════════════════════════════════════════════════════
//                                 PUBLIC HIGH-LEVEL API
// ══════════════════════════════════════════════════════════════════════════════════════════

template <typename SpiType>
uint16_t AS5047U<SpiType>::GetAngle(uint8_t retries) const {
  return decode<AS5047U_REG::ANGLECOM>(readWithRetries(AS5047U_REG::ANGLECOM::ADDRESS, retries))
      .bits.ANGLECOM_value;
}

template <typename SpiType>
float AS5047U<SpiType>::GetAngle(AngleUnit unit, uint8_t retries) const {
  switch (unit) {
    case AngleUnit::Lsb:
      return static_cast<float>(GetAngle(retries));
    case AngleUnit::Degrees:
      return GetAngleDegrees(retries);
    case AngleUnit::Radians:
      return GetAngleRadians(retries);
    default:
      return static_cast<float>(GetAngle(retries));
  }
}

template <typename SpiType>
float AS5047U<SpiType>::GetAngleDegrees(uint8_t retries) const {
  return static_cast<float>(GetAngle(retries)) * Angle::DEG_PER_LSB;
}

template <typename SpiType>
float AS5047U<SpiType>::GetAngleRadians(uint8_t retries) const {
  return static_cast<float>(GetAngle(retries)) * Angle::RAD_PER_LSB;
}

template <typename SpiType>
uint16_t AS5047U<SpiType>::GetRawAngle(uint8_t retries) const {
  return decode<AS5047U_REG::ANGLEUNC>(readWithRetries(AS5047U_REG::ANGLEUNC::ADDRESS, retries))
      .bits.ANGLEUNC_value;
}

template <typename SpiType>
int16_t AS5047U<SpiType>::GetVelocity(uint8_t retries) const {
  const auto v =
      decode<AS5047U_REG::VEL>(readWithRetries(AS5047U_REG::VEL::ADDRESS, retries)).bits.VEL_value;
  return static_cast<int16_t>((static_cast<int16_t>(v << 2)) >> 2);
}

template <typename SpiType>
float AS5047U<SpiType>::GetVelocity(VelocityUnit unit, uint8_t retries) const {
  switch (unit) {
    case VelocityUnit::Lsb:
      return static_cast<float>(GetVelocity(retries));
    case VelocityUnit::DegPerSec:
      return GetVelocityDegPerSec(retries);
    case VelocityUnit::RadPerSec:
      return GetVelocityRadPerSec(retries);
    case VelocityUnit::Rpm:
      return GetVelocityRPM(retries);
    default:
      return static_cast<float>(GetVelocity(retries));
  }
}

template <typename SpiType>
float AS5047U<SpiType>::GetVelocityDegPerSec(uint8_t retries) const {
  return GetVelocity(retries) * Velocity::DEG_PER_LSB;
}

template <typename SpiType>
float AS5047U<SpiType>::GetVelocityRadPerSec(uint8_t retries) const {
  return GetVelocity(retries) * Velocity::RAD_PER_LSB;
}

template <typename SpiType>
float AS5047U<SpiType>::GetVelocityRPM(uint8_t retries) const {
  return GetVelocity(retries) * Velocity::RPM_PER_LSB;
}

template <typename SpiType>
uint8_t AS5047U<SpiType>::GetAGC(uint8_t retries) const {
  return decode<AS5047U_REG::AGC>(readWithRetries(AS5047U_REG::AGC::ADDRESS, retries))
      .bits.AGC_value;
}

template <typename SpiType>
uint16_t AS5047U<SpiType>::GetMagnitude(uint8_t retries) const {
  return decode<AS5047U_REG::MAG>(readWithRetries(AS5047U_REG::MAG::ADDRESS, retries))
      .bits.MAG_value;
}

template <typename SpiType>
uint16_t AS5047U<SpiType>::GetErrorFlags(uint8_t retries) const {
  // ERRFL clears when read, so a retry is only made when the read itself failed (bus error); every
  // value read is ORed into the result and, by readRegister(), into the sticky flags.
  uint16_t flags = 0;
  for (uint8_t i = 0; i <= retries; ++i) {
    uint16_t val = 0;
    const bool ok = readRegister(AS5047U_REG::ERRFL::ADDRESS, val);
    flags |= val;
    if (ok) {
      break;
    }
  }
  return flags;
}

template <typename SpiType>
uint16_t AS5047U<SpiType>::GetZeroPosition(uint8_t retries) const {
  const uint8_t m =
      decode<AS5047U_REG::ZPOSM>(readWithRetries(AS5047U_REG::ZPOSM::ADDRESS, retries))
          .bits.ZPOSM_bits;
  const uint8_t l =
      decode<AS5047U_REG::ZPOSL>(readWithRetries(AS5047U_REG::ZPOSL::ADDRESS, retries))
          .bits.ZPOSL_bits;
  return static_cast<uint16_t>((m << 6) | l);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetZeroPosition(uint16_t angle_lsb, uint8_t retries) {
  AS5047U_REG::ZPOSM m{};
  m.bits.ZPOSM_bits = (angle_lsb >> 6) & 0xFF;
  AS5047U_REG::ZPOSL l{};
  l.bits.ZPOSL_bits = angle_lsb & 0x3F;
  return this->WriteReg(m, retries) && this->WriteReg(l, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetABIResolution(uint8_t resolution_bits, uint8_t retries) {
  resolution_bits = std::clamp(resolution_bits, uint8_t(10), uint8_t(14));
  // Datasheet SETTINGS3 ABIRES (binary mode): 12-bit=0, 11=1, 10=2, 13=3, 14=4 (non-linear)
  static constexpr uint8_t kBitsToAbires[] = {2, 1, 0, 3, 4};  // index (bits-10) -> ABIRES code
  AS5047U_REG::SETTINGS3 s3{};
  if (!this->ReadReg(s3, retries)) {
    return false;
  }
  s3.bits.ABIRES = kBitsToAbires[resolution_bits - 10];
  return this->WriteReg(s3, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetUVWPolePairs(uint8_t pairs, uint8_t retries) {
  pairs = std::clamp(pairs, uint8_t(1), uint8_t(7));
  AS5047U_REG::SETTINGS3 s3{};
  if (!this->ReadReg(s3, retries)) {
    return false;
  }
  s3.bits.UVWPP = static_cast<uint8_t>(pairs - 1);
  return this->WriteReg(s3, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetIndexPulseLength(uint8_t lsb_len, uint8_t retries) {
  AS5047U_REG::SETTINGS2 s2{};
  if (!this->ReadReg(s2, retries)) {
    return false;
  }
  s2.bits.IWIDTH = (lsb_len == 1) ? 1 : 0;
  return this->WriteReg(s2, retries);
}

// Truth table for configureInterface():
// | ABI | UVW | PWM | Pin 8 (I) | Pin 14 (W) |
// |-----|-----|-----|-----------|------------|
// |  1  |  0  |  0  |   I       |   A/B      |
// |  1  |  0  |  1  |   I       |   PWM      |
// |  0  |  1  |  0  |   UVW     |   W        |
// |  0  |  1  |  1  |   PWM     |   W        |
// |  1  |  1  |  x  |   I       |   W        |
// |  0  |  0  |  1  |   -       |   PWM      |
// |  0  |  0  |  0  |   -       |   -        |
//
template <typename SpiType>
bool AS5047U<SpiType>::ConfigureInterface(bool abi, bool uvw, bool pwm, uint8_t retries) {
  AS5047U_REG::DISABLE dis{};
  if (!this->ReadReg(dis, retries)) {
    return false;
  }
  AS5047U_REG::SETTINGS2 s2{};
  if (!this->ReadReg(s2, retries)) {
    return false;
  }
  dis.bits.ABI_off = abi ? 0 : 1;
  dis.bits.UVW_off = uvw ? 0 : 1;
  if (abi && !uvw) {
    s2.bits.UVW_ABI = 0;
    s2.bits.PWMon = pwm;
  } else if (!abi && uvw) {
    s2.bits.UVW_ABI = 1;
    s2.bits.PWMon = pwm;
  } else {
    s2.bits.UVW_ABI = 0;
    s2.bits.PWMon = pwm;
  }
  return this->WriteReg(dis, retries) && this->WriteReg(s2, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetDynamicAngleCompensation(bool enable, uint8_t retries) {
  AS5047U_REG::SETTINGS2 s2{};
  if (!this->ReadReg(s2, retries)) {
    return false;
  }
  s2.bits.DAECDIS = enable ? 0 : 1;
  return this->WriteReg(s2, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetAdaptiveFilter(bool enable, uint8_t retries) {
  AS5047U_REG::DISABLE dis{};
  if (!this->ReadReg(dis, retries)) {
    return false;
  }
  dis.bits.FILTER_disable = enable ? 0 : 1;
  return this->WriteReg(dis, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetFilterParameters(uint8_t k_min, uint8_t k_max, uint8_t retries) {
  k_min = std::min(k_min, uint8_t(7));
  k_max = std::min(k_max, uint8_t(7));
  AS5047U_REG::SETTINGS1 s1{};
  if (!this->ReadReg(s1, retries)) {
    return false;
  }
  s1.bits.K_min = k_min;
  s1.bits.K_max = k_max;
  return this->WriteReg(s1, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetFilterPreset(FilterPreset preset, uint8_t retries) {
  if (!SetAdaptiveFilter(true, retries)) {
    return false;
  }
  // Register codes for SETTINGS1 K_min/K_max. See SETTINGS1 enums; presets use
  // (K_min_code, K_max_code) to get effective K per datasheet Figure 17.
  uint8_t k_min_code = 0;
  uint8_t k_max_code = 0;
  switch (preset) {
    case FilterPreset::LowNoise:
      k_min_code = 5;  // actual K = 0
      k_max_code = 6;  // actual K = 0
      break;
    case FilterPreset::Balanced:
      k_min_code = 0;  // actual K = 2
      k_max_code = 3;  // actual K = 3
      break;
    case FilterPreset::HighBandwidth:
      k_min_code = 4;  // actual K = 6
      k_max_code = 0;  // actual K = 6
      break;
  }
  return SetFilterParameters(k_min_code, k_max_code, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::GetAdaptiveFilterEnabled(uint8_t retries) const {
  const auto dis =
      decode<AS5047U_REG::DISABLE>(readWithRetries(AS5047U_REG::DISABLE::ADDRESS, retries));
  return (dis.bits.FILTER_disable == 0);
}

template <typename SpiType>
std::pair<uint8_t, uint8_t> AS5047U<SpiType>::GetFilterParameters(uint8_t retries) const {
  const auto s1 =
      decode<AS5047U_REG::SETTINGS1>(readWithRetries(AS5047U_REG::SETTINGS1::ADDRESS, retries));
  return {static_cast<uint8_t>(s1.bits.K_min), static_cast<uint8_t>(s1.bits.K_max)};
}

template <typename SpiType>
bool AS5047U<SpiType>::Set150CTemperatureMode(bool enable, uint8_t retries) {
  AS5047U_REG::SETTINGS2 s2{};
  if (!this->ReadReg(s2, retries)) {
    return false;
  }
  s2.bits.NOISESET = enable ? 1 : 0;
  return this->WriteReg(s2, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::ProgramOTP() {
  // Save current frame format and ensure we use CRC for OTP programming
  FrameFormat backup = this->frame_format_;
  if (this->frame_format_ == FrameFormat::SPI_16) {
    this->frame_format_ = FrameFormat::SPI_24;
  }

  // Set current angle as zero position
  SetZeroPosition(GetAngle());

  // Backup the volatile shadow registers that will be committed to OTP
  uint16_t volatile_shadow[5];
  for (uint16_t a = 0x0016; a <= 0x001A; ++a) {
    volatile_shadow[a - 0x0016] = readRegister(a);
  }

  // Enable ECC and compute needed checksum
  auto ecc = this->template ReadReg<AS5047U_REG::ECC>();
  ecc.bits.ECC_en = 1;
  this->WriteReg(ecc);
  auto key = this->template ReadReg<AS5047U_REG::ECC_Checksum>().bits.ECC_s;
  ecc.bits.ECC_chsum = key;
  this->WriteReg(ecc);

  // Verify shadow registers are still correct
  for (uint16_t a = 0x0016; a <= 0x001A; ++a) {
    if (readRegister(a) != volatile_shadow[a - 0x0016]) {
      this->frame_format_ = backup;
      return false;
    }
  }

  // Begin OTP programming sequence
  AS5047U_REG::PROG p{};
  p.bits.PROGEN = 1;
  this->WriteReg(p);
  p.bits.PROGOTP = 1;
  this->WriteReg(p);

  // Wait for programming to complete (timeout after ~15000 cycles)
  for (uint16_t i = 0; i < 15000; ++i) {
    if (readRegister(AS5047U_REG::PROG::ADDRESS) == 0x0001) {
      this->frame_format_ = backup;

      // Guard-band verification: enable PROGVER and refresh OTPREF
      p.bits.PROGVER = 1;
      this->WriteReg(p);

      // Toggle OTPREF to reload from OTP
      p.bits.OTPREF = 1;
      this->WriteReg(p);
      p.bits.OTPREF = 0;
      this->WriteReg(p);

      // Verify shadow registers match OTP
      for (uint16_t a = 0x0016; a <= 0x001A; ++a) {
        if (readRegister(a) != volatile_shadow[a - 0x0016]) {
          this->frame_format_ = backup;
          return false;
        }
      }
      return true;
    }
  }

  // Restore original frame format and return failure if timeout
  this->frame_format_ = backup;
  return false;
}

template <typename SpiType>
void AS5047U<SpiType>::updateStickyErrors(uint16_t err_fl) const {
  // Map ERRFL bits (0-10) to sticky error enum
  if (err_fl & (1u << 0))
    sticky_errors_ |= static_cast<uint16_t>(AS5047U_Error::AgcWarning);
  if (err_fl & (1u << 1))
    sticky_errors_ |= static_cast<uint16_t>(AS5047U_Error::MagHalf);
  if (err_fl & (1u << 2))
    sticky_errors_ |= static_cast<uint16_t>(AS5047U_Error::P2ramWarning);
  if (err_fl & (1u << 3))
    sticky_errors_ |= static_cast<uint16_t>(AS5047U_Error::P2ramError);
  if (err_fl & (1u << 4))
    sticky_errors_ |= static_cast<uint16_t>(AS5047U_Error::FramingError);
  if (err_fl & (1u << 5))
    sticky_errors_ |= static_cast<uint16_t>(AS5047U_Error::CommandError);
  if (err_fl & (1u << 6))
    sticky_errors_ |= static_cast<uint16_t>(AS5047U_Error::CrcError);
  if (err_fl & (1u << 7))
    sticky_errors_ |= static_cast<uint16_t>(AS5047U_Error::WatchdogError);
  if (err_fl & (1u << 9))
    sticky_errors_ |= static_cast<uint16_t>(AS5047U_Error::OffCompError);
  if (err_fl & (1u << 10))
    sticky_errors_ |= static_cast<uint16_t>(AS5047U_Error::CordicOverflow);
}

template <typename SpiType>
AS5047U_Error AS5047U<SpiType>::GetStickyErrorFlags() const noexcept {
  return static_cast<AS5047U_Error>(sticky_errors_.load());
}

template <typename SpiType>
AS5047U_Error AS5047U<SpiType>::ClearStickyErrorFlags() noexcept {
  return static_cast<AS5047U_Error>(sticky_errors_.exchange(0));
}

// Public API implementations
template <typename SpiType>
void AS5047U<SpiType>::SetPad(uint8_t pad) noexcept {
  this->pad_byte_ = pad;
}

template <typename SpiType>
bool AS5047U<SpiType>::SetHysteresis(AS5047U_REG::SETTINGS3::Hysteresis hysteresis,
                                     uint8_t retries) {
  AS5047U_REG::SETTINGS3 s3{};
  if (!this->ReadReg(s3, retries)) {
    return false;
  }
  s3.bits.HYS = static_cast<uint8_t>(hysteresis);
  return this->WriteReg(s3, retries);
}

template <typename SpiType>
AS5047U_REG::SETTINGS3::Hysteresis AS5047U<SpiType>::GetHysteresis() const {
  auto s3 = this->template ReadReg<AS5047U_REG::SETTINGS3>();
  return static_cast<AS5047U_REG::SETTINGS3::Hysteresis>(s3.bits.HYS);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetAngleOutputSource(AS5047U_REG::SETTINGS2::AngleOutputSource source,
                                            uint8_t retries) {
  AS5047U_REG::SETTINGS2 s2{};
  if (!this->ReadReg(s2, retries)) {
    return false;
  }
  s2.bits.Data_select = static_cast<uint8_t>(source);
  return this->WriteReg(s2, retries);
}

template <typename SpiType>
AS5047U_REG::SETTINGS2::AngleOutputSource AS5047U<SpiType>::GetAngleOutputSource() const {
  auto s2 = this->template ReadReg<AS5047U_REG::SETTINGS2>();
  return static_cast<AS5047U_REG::SETTINGS2::AngleOutputSource>(s2.bits.Data_select);
}

template <typename SpiType>
AS5047U_REG::DIA AS5047U<SpiType>::GetDiagnostics() const {
  return this->template ReadReg<AS5047U_REG::DIA>();
}

// ══════════════════════════════════════════════════════════════════════════════════════════
//                           PRIVATE - COMMUNICATION API
// ══════════════════════════════════════════════════════════════════════════════════════════
//
// SPI frame layout per AS5047U datasheet (DS000637 v1-01, "SPI Transaction"):
// - 16-bit (Figures 20-22): MOSI [15] don't care, [14] R, [13:0] ADDR;
//                           MISO [15] warning, [14] error, [13:0] DATA.
// - 24-bit (Figures 23-25): MOSI [23] don't care, [22] RW, [21:8] ADDR or DATA, [7:0] CRC;
//                           MISO [23] warning, [22] error, [21:8] DATA, [7:0] CRC.
// - 32-bit (Figures 26-28): MOSI [31:24] PAD, then the 24-bit frame;
//                           MISO [31] warning, [30] error, [29:16] DATA, [15:8] CRC, [7:0] PAD.
// In every format the 16-bit word (RW and address, or the data) leads the frame after the PAD, and
// the CRC covers that word, MOSI and MISO alike (Figure 31 and "CRC Checksum"). The warning and
// error bits of a MISO frame report a problem in the previous command frame (Figure 28 text).

// One frame: send `word` in `format`, return the 16-bit reply word (warning, error, 14 data bits).
// False when the port's transfer() reports a bus error; that also sets the BusError sticky flag.
// The warning / error bits of every reply with a valid CRC (any 16-bit reply) are recorded as the
// ReplyWarning / ReplyError sticky flags. They do not fail the frame: the data of the reply is
// still valid, and ERRFL, read after every access, has the cause. `inspect` marks a reply whose
// content the caller uses: a wrong CRC on it sets ReplyCrcError and fails the frame. The CRC of a
// reply the driver discards is not enforced (its bits are then not recorded either).
template <typename SpiType>
bool AS5047U<SpiType>::transferFrame(FrameFormat format, uint16_t word, uint16_t& reply,
                                     bool inspect) const {
  uint8_t tx[4] = {};
  uint8_t rx[4] = {};
  std::size_t len = 2;
  std::size_t o = 0; // offset of the word: 1 in 32-bit frames, after the PAD
  if (format == FrameFormat::SPI_32) {
    tx[0] = pad_byte_;
    o = 1;
    len = 4;
  } else if (format == FrameFormat::SPI_24) {
    len = 3;
  }
  tx[o] = static_cast<uint8_t>(word >> 8);
  tx[o + 1] = static_cast<uint8_t>(word & 0xFF);
  if (format != FrameFormat::SPI_16) {
    tx[o + 2] = ComputeCRC8(word);
  }
  if (!detail::Transfer(spi_, tx, rx, len)) {
    sticky_errors_.fetch_or(static_cast<uint16_t>(AS5047U_Error::BusError));
    reply = 0;
    return false;
  }
  reply = static_cast<uint16_t>((static_cast<uint16_t>(rx[0]) << 8) | rx[1]);
  const bool crc_ok = (format == FrameFormat::SPI_16) || (rx[2] == ComputeCRC8(reply));
  if (crc_ok) {
    if ((reply & kReplyWarningBit) != 0U) {
      sticky_errors_.fetch_or(static_cast<uint16_t>(AS5047U_Error::ReplyWarning));
    }
    if ((reply & kReplyErrorBit) != 0U) {
      sticky_errors_.fetch_or(static_cast<uint16_t>(AS5047U_Error::ReplyError));
    }
  }
  if (inspect && !crc_ok) {
    sticky_errors_.fetch_or(static_cast<uint16_t>(AS5047U_Error::ReplyCrcError));
    return false;
  }
  return true;
}

// Register read without the ERRFL read. DS: "the data is transmitted on MISO with the next read
// command" (Figure 29), so the read command is followed by a NOP whose reply carries the data.
template <typename SpiType>
bool AS5047U<SpiType>::rawReadRegister(uint16_t address, uint16_t& value) const {
  uint16_t reply = 0;
  const bool ok =
      transferFrame(frame_format_, static_cast<uint16_t>(kReadBit | (address & 0x3FFF)), reply) &&
      transferFrame(frame_format_, kReadBit | AS5047U_REG::NOP::ADDRESS, reply, true);
  value = ok ? static_cast<uint16_t>(reply & 0x3FFF) : 0U;
  return ok;
}

// Register read that also fetches ERRFL (cleared on the chip by that read, datasheet Figure 33) and
// ORs it into the sticky flags. A read of ERRFL itself is accumulated directly: reading it a second
// time would return 0 and lose the flags. True when every frame went through and, for a register
// other than ERRFL, the chip reported no CRC or framing error for the access.
template <typename SpiType>
bool AS5047U<SpiType>::readRegister(uint16_t address, uint16_t& value) const {
  if (!rawReadRegister(address, value)) {
    return false;
  }
  if (address == AS5047U_REG::ERRFL::ADDRESS) {
    updateStickyErrors(value);
    return true;
  }
  uint16_t err = 0;
  if (!rawReadRegister(AS5047U_REG::ERRFL::ADDRESS, err)) {
    return false;
  }
  updateStickyErrors(err);
  return (err & kRetryMask) == 0U;
}

template <typename SpiType>
uint16_t AS5047U<SpiType>::readRegister(uint16_t address) const {
  uint16_t value = 0;
  (void)readRegister(address, value);
  return value;
}

template <typename SpiType>
bool AS5047U<SpiType>::readWithRetries(uint16_t address, uint8_t retries, uint16_t& value) const {
  for (uint8_t i = 0; i <= retries; ++i) {
    if (readRegister(address, value)) {
      return true;
    }
  }
  return false;
}

template <typename SpiType>
uint16_t AS5047U<SpiType>::readWithRetries(uint16_t address, uint8_t retries) const {
  uint16_t value = 0;
  (void)readWithRetries(address, retries, value);
  return value;
}

template <typename SpiType>
bool AS5047U<SpiType>::writeRegister(uint16_t address, uint16_t value, uint8_t retries) const {
  // The AS5047U datasheet specifies 16-bit frames for read operations only.
  // Writes require 24-bit or 32-bit frames (which include CRC). If the current
  // frame format is SPI_16, temporarily promote to SPI_24 for the write, then
  // restore. This ensures writes always reach the IC correctly.
  const FrameFormat format =
      (frame_format_ == FrameFormat::SPI_16) ? FrameFormat::SPI_24 : frame_format_;

  // DS Fig.30: Write = command frame then data frame. MISO during data = old content.
  // "At the next command" MISO = new content — so we send NOP after data and use that MISO to verify.
  // CS may toggle between command and data frames.
  const uint16_t expected = value & 0x3FFF;
  for (uint8_t attempt = 0; attempt <= retries; ++attempt) {
    uint16_t reply = 0;
    const bool sent = transferFrame(format, address & 0x3FFF, reply) && // RW = 0: write
                      transferFrame(format, expected, reply, true) &&   // MISO: old content
                      transferFrame(format, kReadBit | AS5047U_REG::NOP::ADDRESS, reply, true);
    const uint16_t read_back = reply & 0x3FFF; // MISO on the NOP: new content
    if (sent && read_back == expected) {
      return true;
    }
    uint16_t errfl = 0; // why it failed; accumulated into the sticky flags
    (void)readRegister(AS5047U_REG::ERRFL::ADDRESS, errfl);
    if (sent) {
      AS5047U_REG::ERRFL e{};
      e.value = errfl;
      printf("AS5047U write verify failed: addr=0x%04X expected=0x%04X read_back=0x%04X "
             "ERRFL=0x%04X (CRC_error=%u Framing_error=%u Command_error=%u)\n",
             static_cast<unsigned>(address), static_cast<unsigned>(expected),
             static_cast<unsigned>(read_back), static_cast<unsigned>(e.value),
             static_cast<unsigned>(e.bits.CRC_error), static_cast<unsigned>(e.bits.Framing_error),
             static_cast<unsigned>(e.bits.Command_error));
    }
  }
  return false;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//                       Public API: retry-enabled getters and status dump
// ════════════════════════════════════════════════════════════════════════════════════════════

// Complete dumpStatus with full register dump
template <typename SpiType>
void AS5047U<SpiType>::DumpStatus() const {
  printf("\n=== AS5047U Comprehensive Status ===\n");
  // Core measurements
  printf("Angle (COM) : %u\n", GetAngle());
  printf("Angle (UNC) : %u\n", GetRawAngle());
  printf("Velocity    : %d counts (%.3f deg/s, %.3f rad/s, %.3f RPM)\n", GetVelocity(),
         GetVelocityDegPerSec(), GetVelocityRadPerSec(), GetVelocityRPM());
  printf("AGC         : %u\n", GetAGC());
  printf("Magnitude   : %u\n", GetMagnitude());
  // Error flags
  uint16_t err = GetErrorFlags();
  printf("ERRFL       : 0x%04X\n", err);
  // Diagnostic register bits
  auto dia = this->template ReadReg<AS5047U_REG::DIA>();
  printf("DIA (0x3FF5): 0x%04X\n", dia.value);
  printf("  VDD_mode         : %u\n", dia.bits.VDD_mode);
  printf("  LoopsFinished    : %u\n", dia.bits.LoopsFinished);
  printf("  CORDIC_overflow  : %u\n", dia.bits.CORDIC_overflow_flag);
  printf("  Comp_l           : %u\n", dia.bits.Comp_l);
  printf("  Comp_h           : %u\n", dia.bits.Comp_h);
  printf("  MagHalf_flag     : %u\n", dia.bits.MagHalf_flag);
  printf("  CosOff_fin       : %u\n", dia.bits.CosOff_fin);
  printf("  SinOff_fin       : %u\n", dia.bits.SinOff_fin);
  printf("  OffComp_finished : %u\n", dia.bits.OffComp_finished);
  printf("  AGC_finished     : %u\n", dia.bits.AGC_finished);
  printf("  SPI_cnt          : %u\n", dia.bits.SPI_cnt);
  // Configuration registers
  auto dis = this->template ReadReg<AS5047U_REG::DISABLE>();
  printf("DISABLE (0x0015): 0x%04X UVW_off=%u ABI_off=%u FILTER_disable=%u\n", dis.value,
         dis.bits.UVW_off, dis.bits.ABI_off, dis.bits.FILTER_disable);
  auto s1 = this->template ReadReg<AS5047U_REG::SETTINGS1>();
  printf("SETTINGS1(0x0016): K_max=%u K_min=%u Dia3_en=%u Dia4_en=%u\n", s1.bits.K_max,
         s1.bits.K_min, s1.bits.Dia3_en, s1.bits.Dia4_en);
  auto s2 = this->template ReadReg<AS5047U_REG::SETTINGS2>();
  printf("SETTINGS2(0x0019): IWIDTH=%u NOISESET=%u DIR=%u UVW_ABI=%u "
         "DAECDIS=%u ABI_DEC=%u "
         "Data_select=%u PWMon=%u\n",
         s2.bits.IWIDTH, s2.bits.NOISESET, s2.bits.DIR, s2.bits.UVW_ABI, s2.bits.DAECDIS,
         s2.bits.ABI_DEC, s2.bits.Data_select, s2.bits.PWMon);
  auto s3 = this->template ReadReg<AS5047U_REG::SETTINGS3>();
  printf("SETTINGS3(0x001A): UVWPP=%u HYS=%u ABIRES=%u\n", s3.bits.UVWPP, s3.bits.HYS,
         s3.bits.ABIRES);
  // Other registers
  auto sind = this->template ReadReg<AS5047U_REG::SINDATA>();
  printf("SINDATA(0x3FFA): %d\n", sind.bits.SINDATA);
  auto eccsum = this->template ReadReg<AS5047U_REG::ECC_Checksum>();
  printf("ECC_Checksum(0x3FD0): %u\n", eccsum.bits.ECC_s);
  auto prog = this->template ReadReg<AS5047U_REG::PROG>();
  printf("PROG(0x0003): PROGEN=%u PROGOTP=%u OTPREF=%u PROGVER=%u\n", prog.bits.PROGEN,
         prog.bits.PROGOTP, prog.bits.OTPREF, prog.bits.PROGVER);
  // Interface settings
  printf("FrameFormat      : %u  PadByte=0x%02X\n", static_cast<uint8_t>(this->frame_format_),
         this->pad_byte_);
  printf("========================================\n\n");
}

} // namespace as5047u

#endif // AS5047U_IMPL
