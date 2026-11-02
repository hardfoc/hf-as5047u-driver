/**
 * @file as5047u.hpp
 * @brief Driver for AMS AS5047U Magnetic Rotary Position Sensor (C++21)
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#pragma once
#include "as5047u_spi_interface.hpp"
#include "as5047u_registers.hpp"
#include "as5047u_version.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cmath> // for M_PI and math functions
#include <cstdint>
#include <cstdio>     // for printf
#include <functional> // for std::function
#include <utility>    // for std::pair

// Error flags from ERRFL register
enum class AS5047U_Error : uint16_t {
  None = 0,
  AgcWarning = 1 << 0,     ///< AGC reached minimum (0) or maximum (255) value
  MagHalf = 1 << 1,        ///< Magnetic field is half of regulated value (AGC=255)
  P2ramWarning = 1 << 2,   ///< ECC corrected 1 bit in P2RAM customer area
  P2ramError = 1 << 3,     ///< ECC detected 2+ uncorrectable errors in P2RAM
  FramingError = 1 << 4,   ///< SPI framing error
  CommandError = 1 << 5,   ///< Invalid SPI command received
  CrcError = 1 << 6,       ///< CRC error during SPI communication
  WatchdogError = 1 << 7,  ///< Internal oscillator or watchdog not working proper
  OffCompError = 1 << 9,   ///< Internal offset compensation not finished
  CordicOverflow = 1 << 10 ///< CORDIC algorithm overflow
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
    static constexpr float RAD_PER_LSB = (2.0F * static_cast<float>(M_PI)) / 16384.0F;

    static constexpr uint16_t DegreesToLsb(float degrees) noexcept {
      return static_cast<uint16_t>((degrees * 16384.0F) / 360.0F) & 0x3FFF;
    }

    static constexpr uint16_t RadiansToLsb(float radians) noexcept {
      return static_cast<uint16_t>((radians * 16384.0F) / (2.0F * static_cast<float>(M_PI))) & 0x3FFF;
    }
  };

  /** @brief Helper constants and methods for velocity unit conversions. */
  struct Velocity {
    static constexpr float DEG_PER_LSB = 24.141F;
    static constexpr float RAD_PER_LSB = DEG_PER_LSB * M_PI / 180.0F;
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
   * @brief Permanently program current settings into OTP memory.
   *
   * Performs the full OTP programming sequence:
   * - Writes current configuration (zero position, settings registers) to
   * volatile registers.
   * - Reads current angle and sets it as zero reference.
   * - Enables ECC, reads and writes ECC checksum.
   * - Triggers OTP burn and waits for completion.
   * - Performs guard-band verification (clears registers, refreshes from OTP,
   * verifies content).
   *
   * @return True if programming and verification succeeded, false otherwise.
   * @warning OTP can be programmed only once. Ensure proper supply voltage
   * (3.3-3.5V for 3V mode, ~5V for 5V mode) and desired configuration before
   * use.
   */
  bool ProgramOTP();

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
   * @brief Writes data to a specified register in the AS5047U sensor
   *
   * @tparam RegT The register type which must have an ADDRESS static member and
   * be encodable
   * @param reg The register object containing the data to be written
   * @param retries Number of retries on CRC/framing error (default 0 = no
   * retry)
   * @return true if write succeeded, false on CRC/framing error
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
  uint16_t rawReadRegister(uint16_t addr) const; ///< read register without updating sticky errors
  /// Read a register, then ERRFL (accumulated into the sticky flags). Returns the ERRFL value
  /// read for this access (the register's own value when @p addr is ERRFL).
  uint16_t readRegister(uint16_t addr, uint16_t& value) const;
  uint16_t readRegister(uint16_t addr) const; ///< value only; errors go to the sticky flags
  /// Read a register, repeating up to @p retries times while the chip reports a CRC or framing
  /// error for the access. Never consumes the sticky flags.
  uint16_t readWithRetries(uint16_t addr, uint8_t retries) const;
  bool writeRegister(uint16_t addr, uint16_t val, uint8_t retries) const;

  /// ERRFL bits that make a getter retry its read.
  static constexpr uint16_t kRetryMask = static_cast<uint16_t>(AS5047U_Error::CrcError) |
                                         static_cast<uint16_t>(AS5047U_Error::FramingError);

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
  auto s2 = ReadReg<AS5047U_REG::SETTINGS2>();
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
