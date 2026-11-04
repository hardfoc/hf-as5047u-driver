/**
 * @file as5047u.ipp
 * @brief Template implementation of AS5047U driver methods
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#ifndef AS5047U_IMPL
#define AS5047U_IMPL

// Normally included at the end of as5047u.hpp. Found through the include path (src/ in the source
// tree, the include directory once installed), never by a path relative to the header.
#ifndef AS5047U_HEADER_INCLUDED
#include "as5047u.hpp"
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

namespace detail {
/// Result<T> with @p fn applied to the value; the error, and the value-initialised T of a failed
/// read, are kept.
template <typename U, typename T, typename Fn>
constexpr Result<U> MapResult(const Result<T>& r, Fn&& fn) {
  if (!r.ok()) {
    return {U{}, r.error};
  }
  return {static_cast<U>(fn(r.value)), AS5047U_Error::None};
}
} // namespace detail

template <typename SpiType>
Result<uint16_t> AS5047U<SpiType>::GetAngle(uint8_t retries) const {
  return detail::MapResult<uint16_t>(ReadReg<AS5047U_REG::ANGLECOM>(retries),
                                     [](const auto& r) { return r.bits.ANGLECOM_value; });
}

template <typename SpiType>
Result<float> AS5047U<SpiType>::GetAngle(AngleUnit unit, uint8_t retries) const {
  switch (unit) {
    case AngleUnit::Degrees:
      return GetAngleDegrees(retries);
    case AngleUnit::Radians:
      return GetAngleRadians(retries);
    case AngleUnit::Lsb:
    default:
      return detail::MapResult<float>(GetAngle(retries),
                                      [](uint16_t v) { return static_cast<float>(v); });
  }
}

template <typename SpiType>
Result<float> AS5047U<SpiType>::GetAngleDegrees(uint8_t retries) const {
  return detail::MapResult<float>(
      GetAngle(retries), [](uint16_t v) { return static_cast<float>(v) * Angle::DEG_PER_LSB; });
}

template <typename SpiType>
Result<float> AS5047U<SpiType>::GetAngleRadians(uint8_t retries) const {
  return detail::MapResult<float>(
      GetAngle(retries), [](uint16_t v) { return static_cast<float>(v) * Angle::RAD_PER_LSB; });
}

template <typename SpiType>
Result<uint16_t> AS5047U<SpiType>::GetRawAngle(uint8_t retries) const {
  return detail::MapResult<uint16_t>(ReadReg<AS5047U_REG::ANGLEUNC>(retries),
                                     [](const auto& r) { return r.bits.ANGLEUNC_value; });
}

template <typename SpiType>
Result<int16_t> AS5047U<SpiType>::GetVelocity(uint8_t retries) const {
  return detail::MapResult<int16_t>(ReadReg<AS5047U_REG::VEL>(retries), [](const auto& r) {
    const auto v = r.bits.VEL_value;
    return static_cast<int16_t>((static_cast<int16_t>(v << 2)) >> 2);
  });
}

template <typename SpiType>
Result<float> AS5047U<SpiType>::GetVelocity(VelocityUnit unit, uint8_t retries) const {
  switch (unit) {
    case VelocityUnit::DegPerSec:
      return GetVelocityDegPerSec(retries);
    case VelocityUnit::RadPerSec:
      return GetVelocityRadPerSec(retries);
    case VelocityUnit::Rpm:
      return GetVelocityRPM(retries);
    case VelocityUnit::Lsb:
    default:
      return detail::MapResult<float>(GetVelocity(retries),
                                      [](int16_t v) { return static_cast<float>(v); });
  }
}

template <typename SpiType>
Result<float> AS5047U<SpiType>::GetVelocityDegPerSec(uint8_t retries) const {
  return detail::MapResult<float>(GetVelocity(retries),
                                  [](int16_t v) { return v * Velocity::DEG_PER_LSB; });
}

template <typename SpiType>
Result<float> AS5047U<SpiType>::GetVelocityRadPerSec(uint8_t retries) const {
  return detail::MapResult<float>(GetVelocity(retries),
                                  [](int16_t v) { return v * Velocity::RAD_PER_LSB; });
}

template <typename SpiType>
Result<float> AS5047U<SpiType>::GetVelocityRPM(uint8_t retries) const {
  return detail::MapResult<float>(GetVelocity(retries),
                                  [](int16_t v) { return v * Velocity::RPM_PER_LSB; });
}

template <typename SpiType>
Result<uint8_t> AS5047U<SpiType>::GetAGC(uint8_t retries) const {
  return detail::MapResult<uint8_t>(ReadReg<AS5047U_REG::AGC>(retries),
                                    [](const auto& r) { return r.bits.AGC_value; });
}

template <typename SpiType>
Result<uint16_t> AS5047U<SpiType>::GetMagnitude(uint8_t retries) const {
  return detail::MapResult<uint16_t>(ReadReg<AS5047U_REG::MAG>(retries),
                                     [](const auto& r) { return r.bits.MAG_value; });
}

template <typename SpiType>
Result<uint16_t> AS5047U<SpiType>::GetErrorFlags(uint8_t retries) const {
  // ERRFL clears when read, so a retry is only made when the read itself failed (bus error); every
  // value read is ORed into the result and, by readRegister(), into the sticky flags.
  Result<uint16_t> flags{0U, AS5047U_Error::None};
  for (uint8_t i = 0; i <= retries; ++i) {
    uint16_t val = 0;
    AS5047U_Error cause = AS5047U_Error::None;
    const bool ok = readRegister(AS5047U_REG::ERRFL::ADDRESS, val, &cause);
    flags.value = static_cast<uint16_t>(flags.value | val);
    flags.error = cause;
    if (ok) {
      flags.error = AS5047U_Error::None;
      break;
    }
  }
  return flags;
}

template <typename SpiType>
Result<uint16_t> AS5047U<SpiType>::GetZeroPosition(uint8_t retries) const {
  const auto m = ReadReg<AS5047U_REG::ZPOSM>(retries);
  if (!m.ok()) {
    return {0U, m.error};
  }
  const auto l = ReadReg<AS5047U_REG::ZPOSL>(retries);
  if (!l.ok()) {
    return {0U, l.error};
  }
  return {static_cast<uint16_t>((m.value.bits.ZPOSM_bits << 6) | l.value.bits.ZPOSL_bits),
          AS5047U_Error::None};
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
Result<bool> AS5047U<SpiType>::GetAdaptiveFilterEnabled(uint8_t retries) const {
  return detail::MapResult<bool>(ReadReg<AS5047U_REG::DISABLE>(retries),
                                 [](const auto& r) { return r.bits.FILTER_disable == 0; });
}

template <typename SpiType>
Result<std::pair<uint8_t, uint8_t>> AS5047U<SpiType>::GetFilterParameters(uint8_t retries) const {
  const auto s1 = ReadReg<AS5047U_REG::SETTINGS1>(retries);
  if (!s1.ok()) {
    return {{0U, 0U}, s1.error};
  }
  return {{static_cast<uint8_t>(s1.value.bits.K_min), static_cast<uint8_t>(s1.value.bits.K_max)},
          AS5047U_Error::None};
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
bool AS5047U<SpiType>::ProgramOTP(OtpResult* result) {
  OtpResult local{};
  OtpResult& r = (result != nullptr) ? *result : local;
  r = OtpResult{};
  // The procedure writes registers: use frames with CRC (16-bit frames are for reads only).
  const FrameFormat saved = frame_format_;
  if (frame_format_ == FrameFormat::SPI_16) {
    frame_format_ = FrameFormat::SPI_24;
  }
  const bool ok = programOtpSteps(r);
  frame_format_ = saved;
  return ok;
}

template <typename SpiType>
bool AS5047U<SpiType>::readOtpRegisters(std::array<uint16_t, kOtpRegisterCount>& regs,
                                        uint8_t retries) const {
  for (std::size_t i = 0; i < kOtpRegisterCount; ++i) {
    if (!readWithRetries(static_cast<uint16_t>(kOtpFirstAddress + i), retries, regs[i])) {
      return false;
    }
  }
  return true;
}

// Datasheet DS000637 v1-01, "Step-by-Step Procedure to Permanently Program the Non-Volatile Memory
// (OTP)" (page 41-42) and Figure 60 (OTP Memory Burn and Verification Flowchart, page 43). The step
// text reads 0x0016..0x001A / 0x001B; Figure 60 reads and clears 0x0015 (DISABLE) as well, and
// DISABLE is one of the non-volatile registers (Figure 42), so 0x0015..0x001B are read and
// compared. The PROG values are those of Figure 60. Steps 1-3 and 20-22 are the caller's (see
// ProgramOTP()).
template <typename SpiType>
bool AS5047U<SpiType>::programOtpSteps(OtpResult& r) {
  constexpr uint8_t kRetries = AS5047U_CFG::CRC_RETRIES;
  constexpr std::size_t kZposm = AS5047U_REG::ZPOSM::ADDRESS - kOtpFirstAddress;
  constexpr std::size_t kZposl = AS5047U_REG::ZPOSL::ADDRESS - kOtpFirstAddress;
  constexpr std::size_t kEcc = AS5047U_REG::ECC::ADDRESS - kOtpFirstAddress;
  constexpr uint16_t kProg = AS5047U_REG::PROG::ADDRESS;
  const auto fail = [&r](uint8_t step) {
    r.failed_step = step;
    return false;
  };

  // 4. Read the measured angle from ANGLECOM.
  uint16_t angle = 0;
  if (!readWithRetries(AS5047U_REG::ANGLECOM::ADDRESS, kRetries, angle)) {
    return fail(4);
  }
  // 5. ANGLECOM[5:0] into ZPOSL[5:0] (bits 7:6 kept), ANGLECOM[13:6] into ZPOSM[7:0].
  AS5047U_REG::ZPOSL zposl{};
  if (!ReadReg(zposl, kRetries)) {
    return fail(5);
  }
  zposl.bits.ZPOSL_bits = angle & 0x3FU;
  AS5047U_REG::ZPOSM zposm{};
  zposm.bits.ZPOSM_bits = (angle >> 6) & 0xFFU;
  if (!WriteReg(zposl, kRetries) || !WriteReg(zposm, kRetries)) {
    return fail(5);
  }
  // 6. Read the settings (0x0015..0x001B): what the caller wrote in step 2.
  std::array<uint16_t, kOtpRegisterCount> expected{};
  if (!readOtpRegisters(expected, kRetries)) {
    return fail(6);
  }
  // 7. ECC_en = 1 (Figure 60: Reg(0x001B) = 0x80).
  AS5047U_REG::ECC ecc{};
  ecc.bits.ECC_en = 1;
  if (!WriteReg(ecc, kRetries)) {
    return fail(7);
  }
  // 8. Read the ECC key ECC_s (0x3FD0).
  AS5047U_REG::ECC_Checksum key{};
  if (!ReadReg(key, kRetries)) {
    return fail(8);
  }
  // 9. Write the key into ECC without clearing ECC_en.
  ecc.bits.ECC_chsum = key.bits.ECC_s;
  if (!WriteReg(ecc, kRetries)) {
    return fail(9);
  }
  expected[kZposl] = zposl.value;
  expected[kZposm] = zposm.value;
  expected[kEcc] = ecc.value;
  // 10. Read 0x0015..0x001B: read register step 1.
  std::array<uint16_t, kOtpRegisterCount> step1{};
  if (!readOtpRegisters(step1, kRetries)) {
    return fail(10);
  }
  // 11. Compare with the written content (settings and angle).
  if (step1 != expected) {
    return fail(11);
  }
  r.written = step1;
  // 12. PROGEN = 1 (Reg(0x0003) = 0x01): verified, it reads back as written.
  if (!writeRegister(kProg, 0x0001, kRetries)) {
    return fail(12);
  }
  // 13. PROGOTP = 1 (Reg(0x0003) = 0x08) starts the burn. Sent once, never retried, and from here
  // on the part may be programmed whatever happens next.
  r.burn_started = true;
  if (!writeRegister(kProg, 0x0008, 0, false)) {
    return fail(13);
  }
  // 14. Read PROG until it reads 0x0001 (programming procedure complete).
  bool complete = false;
  for (uint32_t i = 0; i < kOtpPollLimit && !complete; ++i) {
    uint16_t prog = 0;
    complete = readRegister(kProg, prog) && prog == 0x0001;
  }
  if (!complete) {
    return fail(14);
  }
  // 15. Clear the volatile copy: 0x0000 into every non-volatile register (Figure 60).
  for (std::size_t i = 0; i < kOtpRegisterCount; ++i) {
    if (!writeRegister(static_cast<uint16_t>(kOtpFirstAddress + i), 0x0000, kRetries)) {
      return fail(15);
    }
  }
  // 16. PROGVER = 1: the guard band for the guard-band test (Reg(0x0003) = 0x40).
  if (!writeRegister(kProg, 0x0040, 0, false)) {
    return fail(16);
  }
  // 17. OTPREF = 1: refresh the registers from the OTP content (Reg(0x0003) = 0x04).
  if (!writeRegister(kProg, 0x0004, 0, false)) {
    return fail(17);
  }
  // 18. Read 0x0015..0x001B: read register step 2.
  std::array<uint16_t, kOtpRegisterCount> step2{};
  if (!readOtpRegisters(step2, kRetries)) {
    return fail(18);
  }
  // 19. Compare with the written content: a deviation fails the guard-band test.
  if (step2 != r.written) {
    return fail(19);
  }
  return true;
}

template <typename SpiType>
bool AS5047U<SpiType>::VerifyOTP(const std::array<uint16_t, kOtpRegisterCount>& burned,
                                 uint8_t retries) const {
  std::array<uint16_t, kOtpRegisterCount> now{};
  return readOtpRegisters(now, retries) && now == burned;
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
Result<AS5047U_REG::SETTINGS3::Hysteresis> AS5047U<SpiType>::GetHysteresis(uint8_t retries) const {
  using Hysteresis = AS5047U_REG::SETTINGS3::Hysteresis;
  return detail::MapResult<Hysteresis>(ReadReg<AS5047U_REG::SETTINGS3>(retries),
                                       [](const auto& r) { return static_cast<Hysteresis>(r.bits.HYS); });
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
Result<AS5047U_REG::SETTINGS2::AngleOutputSource>
AS5047U<SpiType>::GetAngleOutputSource(uint8_t retries) const {
  using Source = AS5047U_REG::SETTINGS2::AngleOutputSource;
  return detail::MapResult<Source>(ReadReg<AS5047U_REG::SETTINGS2>(retries), [](const auto& r) {
    return static_cast<Source>(r.bits.Data_select);
  });
}

template <typename SpiType>
Result<AS5047U_REG::DIA> AS5047U<SpiType>::GetDiagnostics(uint8_t retries) const {
  return ReadReg<AS5047U_REG::DIA>(retries);
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
                                     bool inspect, AS5047U_Error* cause) const {
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
    if (cause != nullptr) {
      *cause = AS5047U_Error::BusError;
    }
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
    if (cause != nullptr) {
      *cause = AS5047U_Error::ReplyCrcError;
    }
    return false;
  }
  return true;
}

// Register read without the ERRFL read. DS: "the data is transmitted on MISO with the next read
// command" (Figure 29), so the read command is followed by a NOP whose reply carries the data.
template <typename SpiType>
bool AS5047U<SpiType>::rawReadRegister(uint16_t address, uint16_t& value,
                                       AS5047U_Error* cause) const {
  uint16_t reply = 0;
  const bool ok = transferFrame(frame_format_, static_cast<uint16_t>(kReadBit | (address & 0x3FFF)),
                                reply, false, cause) &&
                  transferFrame(frame_format_, kReadBit | AS5047U_REG::NOP::ADDRESS, reply, true,
                                cause);
  value = ok ? static_cast<uint16_t>(reply & 0x3FFF) : 0U;
  return ok;
}

// Register read that also fetches ERRFL (cleared on the chip by that read, datasheet Figure 33) and
// ORs it into the sticky flags. A read of ERRFL itself is accumulated directly: reading it a second
// time would return 0 and lose the flags. True when every frame went through and, for a register
// other than ERRFL, the chip reported no CRC or framing error for the access.
template <typename SpiType>
bool AS5047U<SpiType>::readRegister(uint16_t address, uint16_t& value,
                                    AS5047U_Error* cause) const {
  if (!rawReadRegister(address, value, cause)) {
    return false;
  }
  if (address == AS5047U_REG::ERRFL::ADDRESS) {
    updateStickyErrors(value);
    return true;
  }
  uint16_t err = 0;
  if (!rawReadRegister(AS5047U_REG::ERRFL::ADDRESS, err, cause)) {
    value = 0;
    return false;
  }
  updateStickyErrors(err);
  if ((err & kRetryMask) != 0U) {
    // The chip rejected the access (ERRFL bits 4 and 6 are the FramingError and CrcError bits):
    // the value is not a reading.
    value = 0;
    if (cause != nullptr) {
      *cause = static_cast<AS5047U_Error>(err & kRetryMask);
    }
    return false;
  }
  return true;
}

template <typename SpiType>
bool AS5047U<SpiType>::readWithRetries(uint16_t address, uint8_t retries, uint16_t& value,
                                       AS5047U_Error* cause) const {
  for (uint8_t i = 0; i <= retries; ++i) {
    if (readRegister(address, value, cause)) {
      if (cause != nullptr) {
        *cause = AS5047U_Error::None;
      }
      return true;
    }
  }
  return false;
}

template <typename SpiType>
Result<uint16_t> AS5047U<SpiType>::readResult(uint16_t address, uint8_t retries) const {
  Result<uint16_t> r{};
  (void)readWithRetries(address, retries, r.value, &r.error);
  return r;
}

template <typename SpiType>
bool AS5047U<SpiType>::writeRegister(uint16_t address, uint16_t value, uint8_t retries,
                                     bool verify) const {
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
    if (sent && verify && read_back == expected) {
      return true;
    }
    // Why it failed (or, unverified, whether the chip took it): ERRFL goes to the sticky flags,
    // where the caller finds it. The driver itself prints nothing.
    uint16_t errfl = 0;
    const bool errfl_ok = readRegister(AS5047U_REG::ERRFL::ADDRESS, errfl);
    if (!verify && sent && errfl_ok && (errfl & kCommandErrorMask) == 0U) {
      return true;
    }
  }
  return false;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//                       Public API: retry-enabled getters and status dump
// ════════════════════════════════════════════════════════════════════════════════════════════

// Complete dumpStatus with full register dump. A read that failed is printed as "read failed"
// with its cause, never as a value.
template <typename SpiType>
void AS5047U<SpiType>::DumpStatus() const {
  const auto failed = [](AS5047U_Error e) {
    printf("read failed (error 0x%04X)\n", static_cast<unsigned>(e));
  };
  printf("\n=== AS5047U Comprehensive Status ===\n");
  // Core measurements
  const auto angle = GetAngle();
  printf("Angle (COM) : ");
  angle.ok() ? (void)printf("%u\n", angle.value) : failed(angle.error);
  const auto raw = GetRawAngle();
  printf("Angle (UNC) : ");
  raw.ok() ? (void)printf("%u\n", raw.value) : failed(raw.error);
  const auto vel = GetVelocity();
  printf("Velocity    : ");
  if (vel.ok()) {
    printf("%d counts (%.3f deg/s, %.3f rad/s, %.3f RPM)\n", vel.value,
           static_cast<double>(vel.value * Velocity::DEG_PER_LSB),
           static_cast<double>(vel.value * Velocity::RAD_PER_LSB),
           static_cast<double>(vel.value * Velocity::RPM_PER_LSB));
  } else {
    failed(vel.error);
  }
  const auto agc = GetAGC();
  printf("AGC         : ");
  agc.ok() ? (void)printf("%u\n", agc.value) : failed(agc.error);
  const auto mag = GetMagnitude();
  printf("Magnitude   : ");
  mag.ok() ? (void)printf("%u\n", mag.value) : failed(mag.error);
  // Error flags
  const auto err = GetErrorFlags();
  printf("ERRFL       : ");
  err.ok() ? (void)printf("0x%04X\n", err.value) : failed(err.error);
  // Diagnostic register bits
  const auto dia_r = this->template ReadReg<AS5047U_REG::DIA>();
  printf("DIA (0x3FF5): ");
  if (dia_r.ok()) {
    const auto& dia = dia_r.value;
    printf("0x%04X\n", dia.value);
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
  } else {
    failed(dia_r.error);
  }
  // Configuration registers
  const auto dis = this->template ReadReg<AS5047U_REG::DISABLE>();
  printf("DISABLE (0x0015): ");
  if (dis.ok()) {
    printf("0x%04X UVW_off=%u ABI_off=%u FILTER_disable=%u\n", dis.value.value,
           dis.value.bits.UVW_off, dis.value.bits.ABI_off, dis.value.bits.FILTER_disable);
  } else {
    failed(dis.error);
  }
  const auto s1 = this->template ReadReg<AS5047U_REG::SETTINGS1>();
  printf("SETTINGS1(0x0016): ");
  if (s1.ok()) {
    printf("K_max=%u K_min=%u Dia3_en=%u Dia4_en=%u\n", s1.value.bits.K_max, s1.value.bits.K_min,
           s1.value.bits.Dia3_en, s1.value.bits.Dia4_en);
  } else {
    failed(s1.error);
  }
  const auto s2 = this->template ReadReg<AS5047U_REG::SETTINGS2>();
  printf("SETTINGS2(0x0019): ");
  if (s2.ok()) {
    const auto& b = s2.value.bits;
    printf("IWIDTH=%u NOISESET=%u DIR=%u UVW_ABI=%u DAECDIS=%u ABI_DEC=%u Data_select=%u "
           "PWMon=%u\n",
           b.IWIDTH, b.NOISESET, b.DIR, b.UVW_ABI, b.DAECDIS, b.ABI_DEC, b.Data_select, b.PWMon);
  } else {
    failed(s2.error);
  }
  const auto s3 = this->template ReadReg<AS5047U_REG::SETTINGS3>();
  printf("SETTINGS3(0x001A): ");
  if (s3.ok()) {
    printf("UVWPP=%u HYS=%u ABIRES=%u\n", s3.value.bits.UVWPP, s3.value.bits.HYS,
           s3.value.bits.ABIRES);
  } else {
    failed(s3.error);
  }
  // Other registers
  const auto sind = this->template ReadReg<AS5047U_REG::SINDATA>();
  printf("SINDATA(0x3FFA): ");
  sind.ok() ? (void)printf("%d\n", sind.value.bits.SINDATA) : failed(sind.error);
  const auto eccsum = this->template ReadReg<AS5047U_REG::ECC_Checksum>();
  printf("ECC_Checksum(0x3FD0): ");
  eccsum.ok() ? (void)printf("%u\n", eccsum.value.bits.ECC_s) : failed(eccsum.error);
  const auto prog = this->template ReadReg<AS5047U_REG::PROG>();
  printf("PROG(0x0003): ");
  if (prog.ok()) {
    printf("PROGEN=%u PROGOTP=%u OTPREF=%u PROGVER=%u\n", prog.value.bits.PROGEN,
           prog.value.bits.PROGOTP, prog.value.bits.OTPREF, prog.value.bits.PROGVER);
  } else {
    failed(prog.error);
  }
  // Interface settings
  printf("FrameFormat      : %u  PadByte=0x%02X\n", static_cast<uint8_t>(this->frame_format_),
         this->pad_byte_);
  printf("========================================\n\n");
}

} // namespace as5047u

#endif // AS5047U_IMPL
