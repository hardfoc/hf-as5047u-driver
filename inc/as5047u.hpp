/**
 * @file as5047u.hpp
 * @brief Driver for AMS AS5047U Magnetic Rotary Position Sensor (C++21)
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#pragma once
#include "as5047u_registers.hpp"
#include "as5047u_spi_interface.hpp"
#include "as5047u_version.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cstdint>
#include <cstdio>     // for printf in DumpStatus()
#include <functional> // for std::function
#include <utility>    // for std::pair

// Error flags: bits 0-10 are the ERRFL register (datasheet Figure 33); bits 11-15, which ERRFL does
// not use, are errors the driver detects itself.
enum class AS5047U_Error : uint16_t {
  None = 0,
  AgcWarning = 1 << 0,      ///< AGC reached minimum (0) or maximum (255) value
  MagHalf = 1 << 1,         ///< Magnetic field is half of regulated value (AGC=255)
  P2ramWarning = 1 << 2,    ///< ECC corrected 1 bit in P2RAM customer area
  P2ramError = 1 << 3,      ///< ECC detected 2+ uncorrectable errors in P2RAM
  FramingError = 1 << 4,    ///< SPI framing error
  CommandError = 1 << 5,    ///< Invalid SPI command received
  CrcError = 1 << 6,        ///< CRC error during SPI communication
  WatchdogError = 1 << 7,   ///< Internal oscillator or watchdog not working proper
  OffCompError = 1 << 9,    ///< Internal offset compensation not finished
  CordicOverflow = 1 << 10, ///< CORDIC algorithm overflow
  BusError = 1 << 11,       ///< Driver: the port's transfer() reported a bus error
  ReplyCrcError = 1 << 12,  ///< Driver: the CRC of a 24/32-bit MISO frame was wrong
  ReplyWarning = 1 << 13,   ///< Chip: warning bit set in a MISO frame (DS Figures 22, 25, 28)
  ReplyError = 1 << 14      ///< Chip: error bit set in a MISO frame (DS Figures 22, 25, 28)
};

// -----------------------------------------------------------------------------
// Type definitions
// -----------------------------------------------------------------------------
#include "as5047u_types.hpp" // For FrameFormat enum

// -----------------------------------------------------------------------------
// Default configuration values
// -----------------------------------------------------------------------------
#include "as5047u_config.hpp" // defines AS5047U_CFG namespace with defaults

// -----------------------------------------------------------------------------
// as5047u Namespace
// -----------------------------------------------------------------------------
namespace as5047u {

/// Number of non-volatile registers burned into OTP: DISABLE (0x0015) to ECC (0x001B).
inline constexpr std::size_t kOtpRegisterCount = 7;
/// First non-volatile register address (DISABLE).
inline constexpr uint16_t kOtpFirstAddress = 0x0015;

/**
 * @brief Outcome of AS5047U::ProgramOTP().
 *
 * The step numbers are those of the datasheet's step-by-step OTP procedure
 * (DS000637 v1-01, page 41-42).
 */
struct OtpResult {
  /// Datasheet step that failed (4 to 19), or 0 when every step succeeded.
  uint8_t failed_step = 0;
  /// True once step 13 (PROGOTP) was sent: the fuses may be burned, even if a
  /// later step failed. A part that failed after this must not be used.
  bool burn_started = false;
  /// DISABLE..ECC (0x0015..0x001B) as verified in step 11, the content that
  /// was burned. Compare after the power-on cycle with VerifyOTP().
  std::array<uint16_t, kOtpRegisterCount> written{};
};

/// Pi as a float constant. M_PI is not standard C++ (absent with MSVC unless _USE_MATH_DEFINES,
/// and with strict-ANSI newlib builds); C++20's std::numbers::pi_v<float> is not on every
/// toolchain.
inline constexpr float kPi = 3.14159265358979323846F;

/** @brief Units for angle-returning APIs. */
enum class AngleUnit : uint8_t {
  Lsb,
  Degrees,
  Radians
};

/** @brief Units for velocity-returning APIs. */
enum class VelocityUnit : uint8_t {
  Lsb,
  DegPerSec,
  RadPerSec,
  Rpm
};

/**
 * @brief AS5047U magnetic rotary sensor driver class.
 *
 * Provides high-level access to sensor features: reading angle (with or without
 * DAEC compensation), reading rotation velocity, retrieving AGC and magnitude
 * diagnostics, configuring outputs (ABI, UVW, PWM), handling error flags, and
 * performing OTP programming for permanent configuration storage.
 *
 * @tparam SpiType The SPI bus type (must inherit from as5047u::SpiInterface<SpiType>)
 *
 * @note The driver uses CRTP-based SPI interface for zero virtual call
 * overhead. SPI implementations should inherit from
 * as5047u::SpiInterface<DerivedType>
 *
 * @note C++17 CTAD allows automatic type deduction:
 *       AS5047U encoder(bus, format); // Type deduced automatically
 */
template <typename SpiType>
class AS5047U {
public:
  //------------------------------------------------------------------
  // Constructor and destructor
  //------------------------------------------------------------------

  /**
   * @brief Construct a new AS5047U encoder driver.
   * @param bus Reference to an SPI interface implementation.
   * @param format SPI frame format to use (16-bit, 24-bit, or 32-bit). Default
   * is 16-bit frames.
   */
  explicit AS5047U(SpiType& bus, FrameFormat format = AS5047U_CFG::DEFAULT_FRAME_FORMAT) noexcept;

  ~AS5047U() = default;

  /** Compute the CRC8 value used by AS5047U SPI frames. */
  static constexpr uint8_t ComputeCRC8(uint16_t data16) {
    uint8_t crc = 0xC4;
    for (int i = 0; i < 16; ++i) {
      bool bit = ((data16 >> (15 - i)) & 1) ^ ((crc >> 7) & 1);
      crc = (crc << 1) ^ (bit ? 0x1D : 0x00);
    }
    return crc ^ 0xFF;
  }

  //------------------------------------------------------------------
  // High-level API
  //------------------------------------------------------------------
  /**
   * @brief Set the SPI frame format (16, 24, or 32-bit).
   * @param format The desired SPI frame format.
   */
  void SetFrameFormat(FrameFormat format) noexcept;

  /**
   * @brief Read the 14-bit absolute angle with dynamic compensation (DAEC
   * active).
   *  @param retries Number of retries on CRC/framing error (default 0 = no
   * retry).
   *  @return The current angle in LSB (0-16383).
   */
  [[nodiscard]] uint16_t GetAngle(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /** @brief Read absolute angle in caller-selected unit.
   *  @param unit Output unit (LSB / degrees / radians).
   *  @param retries Number of retries on CRC/framing error (default 0 = no retry).
   *  @return Angle value in the requested unit.
   */
  [[nodiscard]] float GetAngle(AngleUnit unit,
                               uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /** @brief Read absolute angle in degrees.
   *  @param retries Number of retries on CRC/framing error (default 0 = no retry).
   *  @return Angle in degrees in the range [0, 360).
   */
  [[nodiscard]] float GetAngleDegrees(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /** @brief Read absolute angle in radians.
   *  @param retries Number of retries on CRC/framing error (default 0 = no retry).
   *  @return Angle in radians in the range [0, 2π).
   */
  [[nodiscard]] float GetAngleRadians(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /**
   * @brief Read the 14-bit absolute angle without dynamic compensation (raw
   * angle).
   *  @param retries Number of retries on CRC/framing error (default 0 = no
   * retry).
   *  @return The current raw angle in LSB (0-16383).
   */
  [[nodiscard]] uint16_t GetRawAngle(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /**
   * @brief Read the current rotational velocity (signed 14-bit).
   *  @param retries Number of retries on CRC/framing error (default 0 = no
   * retry).
   *  @return The current velocity in LSB (signed 14-bit).
   */
  [[nodiscard]] int16_t GetVelocity(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /** @brief Read rotational velocity in caller-selected unit.
   *  @param unit Output unit (LSB / deg/s / rad/s / RPM).
   *  @param retries Number of retries on CRC/framing error (default 0 = no retry).
   *  @return Velocity in the requested unit.
   */
  [[nodiscard]] float GetVelocity(VelocityUnit unit,
                                  uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /** @brief Angle conversion helpers. */
  struct Angle {
    static constexpr float DEG_PER_LSB = 360.0F / 16384.0F;
    static constexpr float RAD_PER_LSB = (2.0F * kPi) / 16384.0F;

    static constexpr uint16_t DegreesToLsb(float degrees) noexcept {
      return static_cast<uint16_t>((degrees * 16384.0F) / 360.0F) & 0x3FFF;
    }

    static constexpr uint16_t RadiansToLsb(float radians) noexcept {
      return static_cast<uint16_t>((radians * 16384.0F) / (2.0F * kPi)) & 0x3FFF;
    }
  };

  /** @brief Helper constants and methods for velocity unit conversions. */
  struct Velocity {
    static constexpr float DEG_PER_LSB = 24.141F;
    static constexpr float RAD_PER_LSB = DEG_PER_LSB * kPi / 180.0F;
    static constexpr float RPM_PER_LSB = DEG_PER_LSB * (60.0F / 360.0F);
  };

  /** @brief Get rotational velocity in degrees per second.
   *  @param retries Number of retries on CRC/framing error (default 0 = no
   * retry).
   */
  [[nodiscard]] float GetVelocityDegPerSec(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;
  /** @brief Get rotational velocity in radians per second.
   *  @param retries Number of retries on CRC/framing error (default 0 = no
   * retry).
   */
  [[nodiscard]] float GetVelocityRadPerSec(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;
  /** @brief Get rotational velocity in revolutions per minute.
   *  @param retries Number of retries on CRC/framing error (default 0 = no
   * retry).
   */
  [[nodiscard]] float GetVelocityRPM(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /**
   * @brief Read the current Automatic Gain Control (AGC) value (0-255).
   * @param retries Number of retries on CRC/framing error (default 0 = no
   * retry).
   * @return The current AGC value.
   */
  [[nodiscard]] uint8_t GetAGC(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /**
   * @brief Read the current magnetic field magnitude (14-bit value).
   * @param retries Number of retries on CRC/framing error (default 0 = no
   * retry).
   * @return The current magnetic field magnitude in LSB (0-16383).
   */
  [[nodiscard]] uint16_t GetMagnitude(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /**
   * @brief Read the ERRFL register (0x0001) from the chip.
   *
   * The chip clears ERRFL when it is read (datasheet Figure 33), so the value
   * covers only what happened since the driver's previous read of ERRFL. Every
   * ERRFL value the driver reads, including this one, is also ORed into the
   * sticky flags (GetStickyErrorFlags()), so nothing read here is lost.
   * @param retries Number of retries on a failed read (default 0 = no retry).
   * @return 16-bit error flag register (ERRFL).
   */
  [[nodiscard]] uint16_t GetErrorFlags(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /**
   * @brief Dump formatted status and diagnostics using printf
   */
  void DumpStatus() const;

  /**
   * @brief Get the currently configured soft zero position offset (14-bit).
   * @param retries Number of retries on CRC/framing error (default 0 = no
   * retry).
   * @return The current zero position in LSB (0-16383).
   */
  [[nodiscard]] uint16_t GetZeroPosition(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /**
   * @brief Set a new zero reference position (soft offset).
   * @param angle_lsb 14-bit angle value that should be treated as 0°.
   */
  bool SetZeroPosition(uint16_t angle_lsb, uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /**
   * @brief Define the rotation direction for increasing angle.
   *
   * Writes SETTINGS2 (0x0019) bit 2 (DIR) per datasheet Fig.47:
   * 0 = CW default, 1 = invert direction.
   * @param clockwise If true, clockwise yields increasing angle (DIR=0).
   * If false, invert direction (DIR=1).
   * @param retries Number of retries on CRC/framing error (default 0 = no
   * retry).
   * @return true if register write succeeded.
   */
  bool SetDirection(bool clockwise, uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /**
   * @brief Set the ABI (incremental encoder) resolution.
   * @param resolution_bits Resolution in bits (10 to 14 bits).
   */
  bool SetABIResolution(uint8_t resolution_bits, uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /**
   * @brief Set the number of pole pairs for UVW commutation outputs.
   * @param pairs Number of pole pairs (1-7).
   */
  bool SetUVWPolePairs(uint8_t pairs, uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /**
   * @brief Set the index pulse width for ABI output.
   * @param lsb_len Index pulse length in LSB periods (3 or 1).
   */
  bool SetIndexPulseLength(uint8_t lsb_len, uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /**
   * @brief Configure interface outputs (ABI, UVW) and PWM output.
   *
   * Enables/disables ABI and UVW outputs and configures PWM on the available
   * pin:
   * - If `abi` is true and `uvw` false, ABI outputs are enabled and PWM (if
   * enabled) is on W pin.
   * - If `uvw` is true and `abi` false, UVW outputs are enabled and PWM (if
   * enabled) is on I pin.
   * - If both `abi` and `uvw` are true, both interfaces are active (PWM not
   * available in this mode).
   * - If both are false, all interfaces are disabled (PWM can still be enabled
   * on W by default).
   *
   * @param abi Enable ABI (A, B, I) outputs.
   * @param uvw Enable UVW commutation outputs.
   * @param pwm Enable PWM output on the appropriate pin (W if UVW disabled, I
   * if ABI disabled).
   */
  bool ConfigureInterface(bool abi, bool uvw, bool pwm, uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /** @brief Enable/disable Dynamic Angle Error Compensation (DAEC). */
  bool SetDynamicAngleCompensation(bool enable, uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /** @brief Enable/disable the adaptive filter (Dynamic Filter System). */
  bool SetAdaptiveFilter(bool enable, uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /**
   * @brief Set adaptive filter from a preset (easiest way to configure filter).
   *
   * Enables the filter and sets K_min/K_max to match the preset. See FilterPreset
   * and datasheet Figure 17 (velocity RMS noise vs K).
   */
  bool SetFilterPreset(FilterPreset preset,
                      uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /**
   * @brief Set adaptive filter parameters (K_min and K_max as 3-bit register codes).
   *
   * Both arguments are 0–7 (register codes). See SETTINGS1::AdaptiveFilterKmin
   * and AdaptiveFilterKmax for mapping to effective K (datasheet Figure 17).
   * For a single effective K, use SetFilterPreset() instead.
   */
  bool SetFilterParameters(uint8_t k_min, uint8_t k_max,
                           uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /** @brief Read whether the adaptive filter is enabled. */
  [[nodiscard]] bool GetAdaptiveFilterEnabled(uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /** @brief Read current K_min and K_max register codes (0–7 each). */
  [[nodiscard]] std::pair<uint8_t, uint8_t> GetFilterParameters(
      uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /** @brief Set temperature mode for 150°C operation (NOISESET bit).
   *
   * When enabled, the sensor supports full 150°C range (NOISESET=1),
   * but with increased noise. When disabled, lower noise but max 125°C.
   *
   * @param enable True for 150°C mode (NOISESET=1), false for low-noise
   * (NOISESET=0).
   */
  bool Set150CTemperatureMode(bool enable, uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /**
   * @brief Permanently program the current settings into OTP memory.
   *
   * Runs steps 4 to 19 of the datasheet's "Step-by-Step Procedure to
   * Permanently Program the Non-Volatile Memory (OTP)" (DS000637 v1-01,
   * page 41-42, flowchart Figure 60), checking the result of every step and
   * stopping at the first that fails:
   *  4. read ANGLECOM;  5. write it into ZPOSL[5:0] and ZPOSM[7:0];
   *  6. read DISABLE..ECC (0x0015..0x001B);  7. ECC = 0x80 (ECC_en);
   *  8. read ECC_s (0x3FD0);  9. ECC = 0x80 | ECC_s;
   * 10. read 0x0015..0x001B;  11. compare with what was written;
   * 12. PROG = 0x01 (PROGEN);  13. PROG = 0x08 (PROGOTP, starts the burn);
   * 14. poll PROG until it reads 0x0001;
   * 15. write 0x0000 into 0x0015..0x001B (clear the volatile copy);
   * 16. PROG = 0x40 (PROGVER, guard band);  17. PROG = 0x04 (OTPREF, reload
   *     from OTP);  18. read 0x0015..0x001B;  19. compare with step 10.
   *
   * Steps 1 to 3 (power-on cycle, write the settings, magnet at the zero
   * position) come before the call; steps 20 to 22 (power-on cycle, read
   * back, compare) after it, with VerifyOTP() and the image in @p result.
   * 16-bit frames are replaced by 24-bit frames for the duration.
   *
   * @param result Optional: the step that failed, whether the burn was
   * started, and the register image that was burned.
   * @return true if every step succeeded and the guard-band comparison
   * (step 19) matched.
   * @warning OTP can be programmed only once, and a part that fails step 19
   * must not be used (datasheet: "Reprogramming is not allowed!"). VDD 4.5 to
   * 5.5 V (5 V mode) or 3.3 to 3.5 V (3.3 V mode), 5 to 45 degC (Figure 59).
   */
  bool ProgramOTP(OtpResult* result = nullptr);

  /**
   * @brief Steps 21 and 22 of the OTP procedure: after the power-on cycle
   * (step 20), read DISABLE..ECC (0x0015..0x001B) and compare them with the
   * image ProgramOTP() burned.
   * @param burned OtpResult::written from ProgramOTP().
   * @param retries Number of retries of each read (default 0 = no retry).
   * @return true if every register reads as burned; false on a difference
   * (the part must not be used) or a failed read.
   */
  [[nodiscard]] bool VerifyOTP(const std::array<uint16_t, kOtpRegisterCount>& burned,
                               uint8_t retries = AS5047U_CFG::CRC_RETRIES) const;

  /**
   * @brief Set the daisy-chain pad byte for 32-bit SPI frames.
   *
   * In 32-bit frame mode, the first byte is used as a pad to allow multiple
   * devices on the same bus.
   * @param pad Pad byte value (0x00–0xFF) to send as the MSB of each 32-bit
   * transfer.
   */
  void SetPad(uint8_t pad) noexcept;

  /**
   * @brief Set incremental output hysteresis level.
   *
   * Controls the HYS field in SETTINGS3 to add output deadband and reduce
   * chatter.
   * @param hysteresis Hysteresis level:
   *   - SETTINGS3::Hysteresis::LSB_1 (0b00): 1 LSB deadband (~0.17°, default)
   *   - SETTINGS3::Hysteresis::LSB_2 (0b01): 2 LSB deadband (~0.35°)
   *   - SETTINGS3::Hysteresis::LSB_3 (0b10): 3 LSB deadband (~0.52°)
   *   - SETTINGS3::Hysteresis::NONE  (0b11): No hysteresis
   */
  bool SetHysteresis(AS5047U_REG::SETTINGS3::Hysteresis hysteresis,
                     uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /**
   * @brief Get current incremental output hysteresis setting.
   * @return Current Hysteresis enum value (0b00–0b11).
   */
  AS5047U_REG::SETTINGS3::Hysteresis GetHysteresis() const;

  /**
   * @brief Select which angle register (0x3FFF) is returned on reads.
   *
   * Controls the Data_select bit in SETTINGS2.
   * @param source Angle output source:
   *   - SETTINGS2::AngleOutputSource::UseANGLECOM: read compensated angle
   * (ANGLECOM)
   *   - SETTINGS2::AngleOutputSource::UseANGLEUNC: read raw angle (ANGLEUNC)
   */
  bool SetAngleOutputSource(AS5047U_REG::SETTINGS2::AngleOutputSource source,
                            uint8_t retries = AS5047U_CFG::CRC_RETRIES);

  /**
   * @brief Get currently selected angle output source for 0x3FFF reads.
   * @return AngleOutputSource enum indicating ANGLECOM or ANGLEUNC.
   */
  AS5047U_REG::SETTINGS2::AngleOutputSource GetAngleOutputSource() const;

  /**
   * @brief Read the full diagnostic register (DIA).
   * @return DIA register struct containing per-bit calibration and status
   * flags.
   */
  AS5047U_REG::DIA GetDiagnostics() const;

  /**
   * @brief Reads data from a specified register in the AS5047U sensor
   *
   * @tparam RegT The register type which must have an ADDRESS static member and
   * be decodable
   * @return RegT The register object populated with the data read from the
   * sensor
   *
   * This method reads the raw value from the specified register address and
   * decodes it into a strongly-typed register object.
   */
  template <typename RegT>
  RegT ReadReg() const {
    return decode<RegT>(readRegister(RegT::ADDRESS));
  }

  /**
   * @brief Read a register and report whether the read succeeded.
   *
   * @tparam RegT The register type (has a static ADDRESS)
   * @param reg Receives the register content (value 0 when the read failed)
   * @param retries Number of retries on a failed read (default 0 = no retry)
   * @return true if the read went through and the chip reported no CRC or
   * framing error for it; false on a bus error or such a chip error. The
   * reason is in the sticky flags (GetStickyErrorFlags()).
   */
  template <typename RegT>
  [[nodiscard]] bool ReadReg(RegT& reg, uint8_t retries = AS5047U_CFG::CRC_RETRIES) const {
    return readWithRetries(RegT::ADDRESS, retries, reg.value);
  }

  /**
   * @brief Writes data to a specified register in the AS5047U sensor
   *
   * @tparam RegT The register type which must have an ADDRESS static member and
   * be encodable
   * @param reg The register object containing the data to be written
   * @param retries Number of retries on CRC/framing error (default 0 = no
   * retry)
   * @return true if the value read back as written; false if it did not, or
   * if the port's transfer() reported a bus error
   */
  template <typename RegT>
  bool WriteReg(const RegT& reg, uint8_t retries = AS5047U_CFG::CRC_RETRIES) {
    return writeRegister(RegT::ADDRESS, encode(reg), retries);
  }

  /**
   * @brief Error flags accumulated since the last ClearStickyErrorFlags().
   *
   * ERRFL (0x0001) clears on the chip when it is read (datasheet Figure 33),
   * and the driver reads it after every register access. Every ERRFL value the
   * driver reads is ORed into this cache, which only ClearStickyErrorFlags()
   * clears: reading it, and the driver's own retry decisions, leave it intact.
   * @return Bitwise OR of AS5047U_Error flags.
   */
  [[nodiscard]] AS5047U_Error GetStickyErrorFlags() const noexcept;

  /**
   * @brief Clear the sticky error flags.
   * @return The flags that were set, read and cleared in one atomic step.
   */
  AS5047U_Error ClearStickyErrorFlags() noexcept;

  // ===========================================================================
  // Driver Version
  // ===========================================================================

  /**
   * @brief Get the compiled driver version string.
   * @return Null-terminated version string, e.g. "1.0.0".
   */
  static constexpr const char* GetDriverVersion() noexcept {
    return HF_AS5047U_VERSION_STRING;
  }

  /** @brief Get the compiled driver major version number. */
  static constexpr uint8_t GetDriverVersionMajor() noexcept {
    return HF_AS5047U_VERSION_MAJOR;
  }

  /** @brief Get the compiled driver minor version number. */
  static constexpr uint8_t GetDriverVersionMinor() noexcept {
    return HF_AS5047U_VERSION_MINOR;
  }

  /** @brief Get the compiled driver patch version number. */
  static constexpr uint8_t GetDriverVersionPatch() noexcept {
    return HF_AS5047U_VERSION_PATCH;
  }

private:
  //------------------------------------------------------------------
  // Low-level helpers
  //------------------------------------------------------------------
  // Low level register access helpers
  static constexpr uint16_t kReadBit = 0x4000; ///< RW / R bit of a command frame: 1 = read
  static constexpr uint16_t kReplyWarningBit = 0x8000; ///< MISO warning bit (DS Figure 22, 25, 28)
  static constexpr uint16_t kReplyErrorBit = 0x4000;   ///< MISO error bit (DS Figure 22, 25, 28)

  /// One SPI frame in @p format; @p reply is the 16-bit reply word. False on a bus error, or, with
  /// @p inspect (the reply carries data the caller uses), when the reply's CRC is wrong. The
  /// warning and error bits of every reply with a valid CRC go to the sticky flags.
  bool transferFrame(FrameFormat format, uint16_t word, uint16_t& reply,
                     bool inspect = false) const;
  /// Read a register (command frame, then a NOP for the reply) without reading ERRFL.
  bool rawReadRegister(uint16_t addr, uint16_t& value) const;
  /// Read a register, then ERRFL (accumulated into the sticky flags). False on a bus error or when
  /// the chip reports a CRC or framing error for the access.
  bool readRegister(uint16_t addr, uint16_t& value) const;
  uint16_t readRegister(uint16_t addr) const; ///< value only; errors go to the sticky flags
  /// readRegister(), repeated up to @p retries times while it fails. Never consumes the sticky
  /// flags.
  bool readWithRetries(uint16_t addr, uint8_t retries, uint16_t& value) const;
  uint16_t readWithRetries(uint16_t addr, uint8_t retries) const;
  /// Write a register and check the read-back (datasheet Figure 30). With @p verify false (a PROG
  /// command, whose content changes on its own), the write succeeds when the frames went through
  /// and the following ERRFL read shows no CRC, framing or command error.
  bool writeRegister(uint16_t addr, uint16_t val, uint8_t retries, bool verify = true) const;
  /// ProgramOTP() steps 4..19, in the current frame format.
  bool programOtpSteps(OtpResult& r);
  /// Read DISABLE..ECC (0x0015..0x001B).
  bool readOtpRegisters(std::array<uint16_t, kOtpRegisterCount>& regs, uint8_t retries) const;
  static constexpr uint32_t kOtpPollLimit = 15000; ///< step 14: reads of PROG before giving up

  /// ERRFL bits that make a getter retry its read.
  static constexpr uint16_t kRetryMask = static_cast<uint16_t>(AS5047U_Error::CrcError) |
                                         static_cast<uint16_t>(AS5047U_Error::FramingError);
  /// ERRFL bits that mean the chip rejected a frame (CRC, framing or command error).
  static constexpr uint16_t kCommandErrorMask =
      kRetryMask | static_cast<uint16_t>(AS5047U_Error::CommandError);

  SpiType& spi_;             ///< SPI bus reference
  FrameFormat frame_format_; ///< current SPI frame format
  uint8_t pad_byte_{0};      ///< pad byte for SPI_32 daisy-chain indexing

  mutable std::atomic<uint16_t> sticky_errors_{0}; ///< sticky error bits since last clear
  void updateStickyErrors(uint16_t err_fl) const;

  // Helper functions implemented inline for templates
  template <typename RegT>
  static constexpr uint16_t encode(const RegT& r) {
    return r.value;
  }

  template <typename RegT>
  static constexpr RegT decode(uint16_t raw) {
    RegT r{};
    r.value = raw;
    return r;
  }
};

// Template member function definitions must be in header
template <typename SpiType>
AS5047U<SpiType>::AS5047U(SpiType& bus, FrameFormat format) noexcept
    : spi_(bus), frame_format_(format) {
  // No further initialization (use sensor defaults unless configured).
}

template <typename SpiType>
inline bool AS5047U<SpiType>::SetDirection(bool clockwise, uint8_t retries) {
  AS5047U_REG::SETTINGS2 s2{};
  if (!ReadReg(s2, retries)) {
    return false;
  }
  s2.bits.DIR = clockwise ? 0 : 1;
  return WriteReg(s2, retries);
}

// Public API: Get driver version string
inline const char* GetDriverVersion() noexcept {
  return HF_AS5047U_VERSION_STRING;
}

} // namespace as5047u

// Include template implementation
#define AS5047U_HEADER_INCLUDED
// NOLINTNEXTLINE(bugprone-suspicious-include) - Intentional: template implementation file
#include "../src/as5047u.ipp"
#undef AS5047U_HEADER_INCLUDED
