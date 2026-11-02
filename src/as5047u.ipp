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
  // ERRFL clears when read: a retry would return the flags of the retry, not of the failed read.
  // readRegister() ORs every ERRFL value into the sticky flags, so nothing is lost either way.
  (void)retries;
  uint16_t val = 0;
  (void)readRegister(AS5047U_REG::ERRFL::ADDRESS, val);
  return val;
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
  auto s3 = this->template ReadReg<AS5047U_REG::SETTINGS3>();
  s3.bits.ABIRES = kBitsToAbires[resolution_bits - 10];
  return this->WriteReg(s3, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetUVWPolePairs(uint8_t pairs, uint8_t retries) {
  pairs = std::clamp(pairs, uint8_t(1), uint8_t(7));
  auto s3 = this->template ReadReg<AS5047U_REG::SETTINGS3>();
  s3.bits.UVWPP = static_cast<uint8_t>(pairs - 1);
  return this->WriteReg(s3, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetIndexPulseLength(uint8_t lsb_len, uint8_t retries) {
  auto s2 = this->template ReadReg<AS5047U_REG::SETTINGS2>();
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
  auto dis = this->template ReadReg<AS5047U_REG::DISABLE>();
  auto s2 = this->template ReadReg<AS5047U_REG::SETTINGS2>();
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
  auto s2 = this->template ReadReg<AS5047U_REG::SETTINGS2>();
  s2.bits.DAECDIS = enable ? 0 : 1;
  return this->WriteReg(s2, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetAdaptiveFilter(bool enable, uint8_t retries) {
  auto dis = this->template ReadReg<AS5047U_REG::DISABLE>();
  dis.bits.FILTER_disable = enable ? 0 : 1;
  return this->WriteReg(dis, retries);
}

template <typename SpiType>
bool AS5047U<SpiType>::SetFilterParameters(uint8_t k_min, uint8_t k_max, uint8_t retries) {
  k_min = std::min(k_min, uint8_t(7));
  k_max = std::min(k_max, uint8_t(7));
  auto s1 = this->template ReadReg<AS5047U_REG::SETTINGS1>();
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
  auto s2 = this->template ReadReg<AS5047U_REG::SETTINGS2>();
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
  auto s3 = this->template ReadReg<AS5047U_REG::SETTINGS3>();
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
  auto s2 = this->template ReadReg<AS5047U_REG::SETTINGS2>();
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
// SPI frame layout per AS5047U datasheet (DS000637):
// - 16-bit: Fig.20-22  MOSI bit14=R(1:Read), 13:0=ADDR; MISO bit15=ER, 14=0, 13:0=RDATA (Fig.21-22).
// - 24-bit: Fig.23-25  MOSI 22=RW, 21:8=ADDR, 7:0=CRC; MISO 23=ER, 22=Error, 21:8=DATA[13:0], 7:0=CRC (Fig.25). CRC Fig.31.
// - 32-bit: Fig.26-28  PAD in B0 (MOSI) / B3 (MISO); MISO B0=[ER,Err,Data13:8], B1=Data7:0, B2=CRC, B3=PAD (Fig.28).
//
// Low level register read without sticky error update.
// DS: "The data is transmitted on MISO with the *next* read command." So we always send
// (1) read command for address, (2) NOP; the NOP response carries the data for that address.
template <typename SpiType>
uint16_t AS5047U<SpiType>::rawReadRegister(uint16_t address) const {
  uint16_t result = 0;
  if (this->frame_format_ == FrameFormat::SPI_16) {
    // ---- 16-bit frame without CRC ----
    // (1) Read command (bit14=1 for read)
    uint16_t cmd = static_cast<uint16_t>(0x4000 | (address & 0x3FFF));
    const uint8_t tx[2] = {static_cast<uint8_t>(cmd >> 8), static_cast<uint8_t>(cmd & 0xFF)};
    uint8_t rx[2];
    spi_.transfer(tx, rx, 2);

    // (2) NOP — MISO from this transfer = data for the address we just requested
    const uint8_t tx_nop[2] = {0x40, 0x00};
    uint8_t rx_data[2];
    spi_.transfer(tx_nop, rx_data, 2);

    // Process response
    uint16_t raw = (static_cast<uint16_t>(rx_data[0]) << 8) | rx_data[1];
    result = raw & 0x3FFF; // Mask out status flags
  } else if (this->frame_format_ == FrameFormat::SPI_24) {
    // ---- 24-bit frame with CRC ----
    // (1) Read command with CRC
    uint16_t crc_input = static_cast<uint16_t>((1 << 14) | (address & 0x3FFF));
    uint8_t crc = ComputeCRC8(crc_input);
    const uint8_t tx_cmd[3] = {
        static_cast<uint8_t>(((address >> 8) & 0x3F) | 0x40), // bit6=1 for read
        static_cast<uint8_t>(address & 0xFF), crc};
    uint8_t rx_cmd[3];
    spi_.transfer(tx_cmd, rx_cmd, 3);

    // (2) NOP — MISO from this transfer = data for the requested address
    uint16_t nop_addr = AS5047U_REG::NOP::ADDRESS;
    uint16_t nop_crc_input = static_cast<uint16_t>((1 << 14) | (nop_addr & 0x3FFF));
    uint8_t crc_nop = ComputeCRC8(nop_crc_input);
    const uint8_t tx_nop[3] = {static_cast<uint8_t>(((nop_addr >> 8) & 0x3F) | 0x40),
                               static_cast<uint8_t>(nop_addr & 0xFF), crc_nop};
    uint8_t rx_data_frame[3];
    spi_.transfer(tx_nop, rx_data_frame, 3);

    // Process response with CRC verification
    uint16_t raw = (static_cast<uint16_t>(rx_data_frame[0]) << 8) | rx_data_frame[1];
    uint8_t crc_device = rx_data_frame[2];
    uint8_t crc_calc = ComputeCRC8(raw);
    if (crc_device != crc_calc) {
      // crc error, caller will read ERRFL
    }
    result = raw & 0x3FFF;
  } else if (this->frame_format_ == FrameFormat::SPI_32) {
    // ---- 32-bit frame with CRC and pad byte (DS Fig.26-28) ----
    // MISO data frame (Fig.28): Bit31=ER, 30=Error, 29:16=Data(14b), 15:8=CRC, 7:0=PAD.
    // So Byte0=[ER,Err,Data13:8], Byte1=Data7:0, Byte2=CRC, Byte3=PAD. CRC over bits 31:16 (DS p.23).
    uint16_t crc_input = static_cast<uint16_t>((1 << 14) | (address & 0x3FFF));
    uint8_t crc = ComputeCRC8(crc_input);
    const uint8_t tx_cmd[4] = {
        this->pad_byte_,
        static_cast<uint8_t>(((address >> 8) & 0x3F) | 0x40), // bit6=1 for read
        static_cast<uint8_t>(address & 0xFF), crc};
    uint8_t rx_cmd[4];
    spi_.transfer(tx_cmd, rx_cmd, 4);

    // (2) NOP — MISO from this transfer = data for the requested address. Fig.28: Byte0/1=Data, Byte2=CRC, Byte3=PAD.
    uint16_t nop_addr = AS5047U_REG::NOP::ADDRESS;
    uint16_t nop_crc_input = static_cast<uint16_t>((1 << 14) | (nop_addr & 0x3FFF));
    uint8_t crc_nop = ComputeCRC8(nop_crc_input);
    const uint8_t tx_nop[4] = {this->pad_byte_,
                               static_cast<uint8_t>(((nop_addr >> 8) & 0x3F) | 0x40),
                               static_cast<uint8_t>(nop_addr & 0xFF), crc_nop};
    uint8_t rx_data_frame[4];
    spi_.transfer(tx_nop, rx_data_frame, 4);

    // Data = bits 29:16 = (Byte0 & 0x3F)<<8 | Byte1; CRC = Byte2 (bits 15:8); Byte3 = PAD
    uint16_t raw = (static_cast<uint16_t>(rx_data_frame[0] & 0x3Fu) << 8) | rx_data_frame[1];
    uint16_t crc_payload_32 = (static_cast<uint16_t>(rx_data_frame[0]) << 8) | rx_data_frame[1];
    uint8_t crc_device = rx_data_frame[2];
    uint8_t crc_calc = ComputeCRC8(crc_payload_32);
    if (crc_device != crc_calc) {
      // crc error, caller will read ERRFL
    }
    result = raw & 0x3FFF;
  }
  return result;
}

// Register read that also fetches ERRFL (cleared on the chip by that read, datasheet Figure 33) and
// ORs it into the sticky flags. A read of ERRFL itself is accumulated directly: reading it a second
// time would return 0 and lose the flags.
template <typename SpiType>
uint16_t AS5047U<SpiType>::readRegister(uint16_t address, uint16_t& value) const {
  value = rawReadRegister(address);
  const uint16_t err = (address == AS5047U_REG::ERRFL::ADDRESS)
                           ? value
                           : rawReadRegister(AS5047U_REG::ERRFL::ADDRESS);
  updateStickyErrors(err);
  return err;
}

template <typename SpiType>
uint16_t AS5047U<SpiType>::readRegister(uint16_t address) const {
  uint16_t value = 0;
  (void)readRegister(address, value);
  return value;
}

template <typename SpiType>
uint16_t AS5047U<SpiType>::readWithRetries(uint16_t address, uint8_t retries) const {
  uint16_t value = 0;
  for (uint8_t i = 0; i <= retries; ++i) {
    if ((readRegister(address, value) & kRetryMask) == 0U) {
      break;
    }
  }
  return value;
}

template <typename SpiType>
bool AS5047U<SpiType>::writeRegister(uint16_t address, uint16_t value, uint8_t retries) const {
  bool success = false;

  // The AS5047U datasheet specifies 16-bit frames for read operations only.
  // Writes require 24-bit or 32-bit frames (which include CRC). If the current
  // frame format is SPI_16, temporarily promote to SPI_24 for the write, then
  // restore. This ensures writes always reach the IC correctly.
  FrameFormat active_format = this->frame_format_;
  if (active_format == FrameFormat::SPI_16) {
    active_format = FrameFormat::SPI_24;
  }

  // DS Fig.30: Write = command frame then data frame. MISO during data = old content.
  // "At the next command" MISO = new content — so we send NOP after data and use that MISO to verify.
  // CS may toggle between command and data frames.
  const uint16_t expected = value & 0x3FFF;
  const uint16_t nop_addr = AS5047U_REG::NOP::ADDRESS;
  const uint16_t nop_crc_input = static_cast<uint16_t>((1 << 14) | (nop_addr & 0x3FFF));
  const uint8_t crc_nop = ComputeCRC8(nop_crc_input);

  for (uint8_t attempt = 0; attempt <= retries; ++attempt) {
    if (active_format == FrameFormat::SPI_24) {
      // ---- 24-bit write: command, data, then NOP (MISO on NOP = new content) ----
      uint16_t cmd_payload = static_cast<uint16_t>(address & 0x3FFF);
      uint8_t cmd_crc = ComputeCRC8(cmd_payload);
      const uint8_t tx_cmd[3] = {static_cast<uint8_t>((address >> 8) & 0x3F), // bit6=0 for write
                                 static_cast<uint8_t>(address & 0xFF), cmd_crc};
      uint8_t rx_cmd[3];
      spi_.transfer(tx_cmd, rx_cmd, 3);

      uint16_t data_payload = value & 0x3FFF;
      uint8_t data_crc = ComputeCRC8(data_payload);
      const uint8_t tx_data[3] = {static_cast<uint8_t>((data_payload >> 8) & 0xFF),
                                  static_cast<uint8_t>(data_payload & 0xFF), data_crc};
      uint8_t rx_data[3];
      spi_.transfer(tx_data, rx_data, 3);  // MISO here = old content (DS Fig.30)

      // NOP — MISO = new content of the written register (third TX = response to write)
      // 24-bit MISO same layout as 32-bit: Byte0=[ER,Err,Data13:8], Byte1=Data7:0 → 14-bit = (B0&0x3F)<<8|B1
      const uint8_t tx_nop[3] = {static_cast<uint8_t>(((nop_addr >> 8) & 0x3F) | 0x40),
                                 static_cast<uint8_t>(nop_addr & 0xFF), crc_nop};
      uint8_t rx_nop[3];
      spi_.transfer(tx_nop, rx_nop, 3);
      uint16_t read_back = (static_cast<uint16_t>(rx_nop[0] & 0x3Fu) << 8) | rx_nop[1];
      if (read_back == expected) {
        success = true;
        break;
      }
      auto errfl = this->template ReadReg<AS5047U_REG::ERRFL>(); // accumulated into sticky flags
      printf("AS5047U write verify failed: addr=0x%04X expected=0x%04X read_back=0x%04X "
             "ERRFL=0x%04X (CRC_error=%u Framing_error=%u Command_error=%u)\n",
             static_cast<unsigned>(address), static_cast<unsigned>(expected),
             static_cast<unsigned>(read_back), static_cast<unsigned>(errfl.value),
             static_cast<unsigned>(errfl.bits.CRC_error),
             static_cast<unsigned>(errfl.bits.Framing_error),
             static_cast<unsigned>(errfl.bits.Command_error));
    } else if (active_format == FrameFormat::SPI_32) {
      // ---- 32-bit write: command, data, then NOP (MISO on NOP = new content) ----
      uint16_t cmd_payload = static_cast<uint16_t>(address & 0x3FFF);
      uint8_t cmd_crc = ComputeCRC8(cmd_payload);
      const uint8_t tx_cmd[4] = {this->pad_byte_,
                                 static_cast<uint8_t>((address >> 8) & 0x3F), // bit6=0 for write
                                 static_cast<uint8_t>(address & 0xFF), cmd_crc};
      uint8_t rx_cmd[4];
      spi_.transfer(tx_cmd, rx_cmd, 4);

      uint16_t data_payload = value & 0x3FFF;
      uint8_t data_crc = ComputeCRC8(data_payload);
      const uint8_t tx_data[4] = {this->pad_byte_, static_cast<uint8_t>((data_payload >> 8) & 0xFF),
                                  static_cast<uint8_t>(data_payload & 0xFF), data_crc};
      uint8_t rx_data[4];
      spi_.transfer(tx_data, rx_data, 4);  // MISO here = old content (DS Fig.30)

      // NOP — MISO = new content (Fig.28: Byte0/1 = Data, Byte2 = CRC, Byte3 = PAD)
      const uint8_t tx_nop[4] = {this->pad_byte_,
                                static_cast<uint8_t>(((nop_addr >> 8) & 0x3F) | 0x40),
                                static_cast<uint8_t>(nop_addr & 0xFF), crc_nop};
      uint8_t rx_nop[4];
      spi_.transfer(tx_nop, rx_nop, 4);
      uint16_t read_back = (static_cast<uint16_t>(rx_nop[0] & 0x3Fu) << 8) | rx_nop[1];
      if (read_back == expected) {
        success = true;
        break;
      }
      auto errfl = this->template ReadReg<AS5047U_REG::ERRFL>(); // accumulated into sticky flags
      printf("AS5047U write verify failed: addr=0x%04X expected=0x%04X read_back=0x%04X "
             "ERRFL=0x%04X (CRC_error=%u Framing_error=%u Command_error=%u)\n",
             static_cast<unsigned>(address), static_cast<unsigned>(expected),
             static_cast<unsigned>(read_back), static_cast<unsigned>(errfl.value),
             static_cast<unsigned>(errfl.bits.CRC_error),
             static_cast<unsigned>(errfl.bits.Framing_error),
             static_cast<unsigned>(errfl.bits.Command_error));
    }
  }
  return success;
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
