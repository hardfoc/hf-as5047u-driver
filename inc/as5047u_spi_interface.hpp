/**
 * @file as5047u_spi_interface.hpp
 * @brief CRTP-based SPI bus interface for AS5047U driver
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace as5047u {

/**
 * @brief CRTP-based template interface for SPI bus operations
 *
 * This template class provides a hardware-agnostic interface for SPI
 * communication using the CRTP pattern. Platform-specific implementations
 * should inherit from this template with themselves as the template parameter.
 *
 * Benefits of CRTP:
 * - Compile-time polymorphism (no virtual function overhead)
 * - Static dispatch instead of dynamic dispatch
 * - Better optimization opportunities for the compiler
 *
 * Example usage:
 * @code
 * class MySPI : public as5047u::SpiInterface<MySPI> {
 * public:
 *   bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
 *     // Platform-specific SPI implementation; false on a bus error
 *     return my_sdk_spi_transfer(tx, rx, len) == MY_SDK_OK;
 *   }
 * };
 * @endcode
 *
 * The port's `transfer()` returns `bool`: `true` when the frame was clocked,
 * `false` on a bus error. The driver turns `false` into a failed call (`false`
 * from the setters, `WriteReg()` and `ProgramOTP()`, the `BusError` sticky flag
 * for the reads). A port written for the earlier contract, with a `void`
 * transfer, still compiles and is treated as never failing. Any other return
 * type is rejected at compile time, so an SDK status code (for example an
 * `esp_err_t`, where 0 means success) cannot be mistaken for a bool.
 *
 * @tparam Derived The derived class type (CRTP pattern)
 */
namespace detail {

/** @brief Return type of `Port::transfer(const uint8_t*, uint8_t*, std::size_t)`. */
template <typename Port>
using TransferResult = decltype(std::declval<Port&>().transfer(
    std::declval<const uint8_t*>(), std::declval<uint8_t*>(), std::declval<std::size_t>()));

/** @brief True when the port's transfer() reports a status (returns bool). */
template <typename Port>
inline constexpr bool kTransferReportsStatus =
    std::is_same_v<std::remove_cv_t<std::remove_reference_t<TransferResult<Port>>>, bool>;

/**
 * @brief Call the port's transfer() and return its status.
 *
 * A `bool` result is returned as-is; a `void` transfer (the earlier contract)
 * is treated as always successful.
 */
template <typename Port>
inline bool Transfer(Port& port, const uint8_t* tx, uint8_t* rx, std::size_t len) {
  using Result = TransferResult<Port>;
  static_assert(std::is_void_v<Result> || kTransferReportsStatus<Port>,
                "as5047u: the port's transfer(tx, rx, len) must return bool (true = frame "
                "clocked, false = bus error), or void for a port that never reports errors");
  if constexpr (std::is_void_v<Result>) {
    port.transfer(tx, rx, len);
    return true;
  } else {
    return static_cast<bool>(port.transfer(tx, rx, len));
  }
}

} // namespace detail

template <typename Derived> class SpiInterface {
public:
  /**
   * @brief Perform a full-duplex SPI data transfer.
   *
   * Sends `len` bytes from `tx` and simultaneously receives `len` bytes into
   * `rx`. Implementations should assert the device's chip select for the
   * duration of the transfer.
   *
   * @param tx Pointer to data to transmit (len bytes). If nullptr, zeros can be
   * sent.
   * @param rx Pointer to buffer for received data (len bytes). If nullptr,
   * received data can be ignored.
   * @param len Number of bytes to transfer.
   * @return true when the frame was transferred, false on a bus error.
   */
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    return detail::Transfer(*static_cast<Derived*>(this), tx, rx, len);
  }

  // Prevent copying
  SpiInterface(const SpiInterface &) = delete;
  SpiInterface &operator=(const SpiInterface &) = delete;

protected:
  /**
   * @brief Protected constructor to prevent direct instantiation
   */
  SpiInterface() = default;

  // Allow moving
  SpiInterface(SpiInterface &&) = default;
  SpiInterface &operator=(SpiInterface &&) = default;

  /**
   * @brief Protected destructor
   * @note Derived classes can have public destructors
   */
  ~SpiInterface() = default;
};

} // namespace as5047u
