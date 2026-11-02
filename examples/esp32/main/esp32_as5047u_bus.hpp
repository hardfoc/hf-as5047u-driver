/**
 * @file esp32_as5047u_bus.hpp
 * @brief ESP32 SPI transport implementation for AS5047U driver (header-only)
 *
 * This file provides the ESP32-specific implementation of the as5047u::SpiInterface
 * interface for communicating with AS5047U magnetic encoder over SPI.
 *
 * Header-only implementation — no separate .cpp file needed.
 *
 * @author N3b3x
 * @date 2025
 * @version 2.0.0
 */

#pragma once

#include "../../../inc/as5047u_spi_interface.hpp"
#include "esp32_as5047u_test_config.hpp"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

/**
 * @class Esp32As5047uSpiBus
 * @brief ESP32 SPI transport implementation for AS5047U driver
 *
 * This class implements the as5047u::SpiInterface interface using ESP-IDF's SPI
 * driver with CRTP pattern. It supports configurable SPI pins, frequency, and
 * chip select.
 */
class Esp32As5047uSpiBus : public as5047u::SpiInterface<Esp32As5047uSpiBus> {
public:
  /**
   * @brief SPI configuration structure
   */
  /**
   * @brief SPI configuration structure
   * 
   * @note For standard usage, use CreateEsp32As5047uSpiBus() factory function
   *       which pulls configuration from esp32_as5047u_test_config.hpp.
   *       Only create SPIConfig manually if you need custom pin assignments.
   */
  struct SPIConfig {
    spi_host_device_t host;      ///< SPI host (e.g., SPI2_HOST for ESP32-S3)
    gpio_num_t miso_pin;         ///< MISO pin (Master In Slave Out)
    gpio_num_t mosi_pin;         ///< MOSI pin (Master Out Slave In)
    gpio_num_t sclk_pin;         ///< SCLK pin (SPI Clock)
    gpio_num_t cs_pin;           ///< CS pin (Chip Select, active low)
    uint32_t frequency;          ///< SPI frequency in Hz (max 10MHz for AS5047U)
    uint8_t mode;                ///< SPI mode (must be 1: CPOL=0, CPHA=1 for AS5047U)
    uint8_t queue_size;         ///< Transaction queue size
    uint8_t cs_ena_pretrans;     ///< CS asserted N clock cycles before transaction
    uint8_t cs_ena_posttrans;    ///< CS held N clock cycles after transaction
  };

  /**
   * @brief Constructor with SPI configuration
   * 
   * @param config SPI configuration parameters (must be fully specified)
   * 
   * @note For standard usage, prefer CreateEsp32As5047uSpiBus() factory function
   *       which uses configuration from esp32_as5047u_test_config.hpp
   */
  explicit Esp32As5047uSpiBus(const SPIConfig &config) : config_(config) {}

  /**
   * @brief Destructor - cleans up SPI resources
   */
  ~Esp32As5047uSpiBus() { deinitialize(); }

  /**
   * @brief Perform a full-duplex SPI data transfer
   * @param tx Pointer to data to transmit (len bytes). If nullptr, zeros are
   * sent.
   * @param rx Pointer to buffer for received data (len bytes). If nullptr,
   * received data is ignored.
   * @param len Number of bytes to transfer.
   * @return true when the frame was transferred, false on an ESP-IDF error.
   */
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
    if (!initialized_ || spi_device_ == nullptr) {
      ESP_LOGE(TAG, "SPI bus not initialized");
      return false;
    }

    spi_transaction_t trans = {};
    trans.length = len * 8; // Length in bits
    trans.tx_buffer = tx;
    trans.rx_buffer = rx;

    esp_err_t ret = spi_device_transmit(spi_device_, &trans);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "SPI transfer failed: %s", esp_err_to_name(ret));
      return false;
    }

#if ESP32_AS5047U_ENABLE_DETAILED_SPI_LOGGING
    // Log TX/RX frame per transfer for debugging (direction, zero position, etc.)
    if (len > 0 && len <= 8) {
      char buf[80];
      int n = 0;
      n += snprintf(buf + n, sizeof(buf) - n, "SPI[%zu] TX:", len);
      for (std::size_t i = 0; i < len && n < (int)(sizeof(buf) - 4); ++i) {
        n += snprintf(buf + n, sizeof(buf) - n, " %02X", tx ? tx[i] : 0u);
      }
      if (rx) {
        n += snprintf(buf + n, sizeof(buf) - n, "  RX:");
        for (std::size_t i = 0; i < len && n < (int)(sizeof(buf) - 4); ++i) {
          n += snprintf(buf + n, sizeof(buf) - n, " %02X", rx[i]);
        }
      } else {
        n += snprintf(buf + n, sizeof(buf) - n, "  RX: (none)");
      }
      ESP_LOGI(TAG, "%s", buf);
    }
#endif
    return true;
  }

  /**
   * @brief Initialize the SPI bus (must be called before use)
   * @return true if successful, false otherwise
   */
  bool initialize() {
    if (initialized_) {
      ESP_LOGW(TAG, "SPI bus already initialized");
      return true;
    }

    if (!initializeSPI()) {
      ESP_LOGE(TAG, "Failed to initialize SPI bus");
      return false;
    }

    if (!addSPIDevice()) {
      ESP_LOGE(TAG, "Failed to add SPI device");
      deinitialize();
      return false;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "SPI bus initialized successfully");
    return true;
  }

  /**
   * @brief Deinitialize the SPI bus
   */
  void deinitialize() {
    if (!initialized_) {
      return;
    }

    if (spi_device_ != nullptr) {
      spi_bus_remove_device(spi_device_);
      spi_device_ = nullptr;
    }

    spi_bus_free(config_.host);

    initialized_ = false;
    ESP_LOGI(TAG, "SPI bus deinitialized");
  }

  /**
   * @brief Get the current SPI configuration
   * @return Current SPI configuration
   */
  const SPIConfig &getConfig() const noexcept { return config_; }

  /**
   * @brief Check if SPI bus is initialized
   * @return true if initialized, false otherwise
   */
  bool isInitialized() const noexcept { return initialized_; }

private:
  SPIConfig config_;                         ///< SPI configuration
  spi_device_handle_t spi_device_ = nullptr; ///< SPI device handle
  bool initialized_ = false;                 ///< Initialization state
  static constexpr const char *TAG = "Esp32As5047uSpiBus"; ///< Logging tag

  /**
   * @brief Initialize SPI bus
   * @return true if successful, false otherwise
   */
  bool initializeSPI() {
    // Verify pin numbers are valid before configuring
    if (config_.sclk_pin < 0 || config_.sclk_pin > 48) {
      ESP_LOGE(TAG, "Invalid SCLK pin: %d", config_.sclk_pin);
      return false;
    }
    if (config_.mosi_pin < 0 || config_.mosi_pin > 48) {
      ESP_LOGE(TAG, "Invalid MOSI pin: %d", config_.mosi_pin);
      return false;
    }
    if (config_.miso_pin < 0 || config_.miso_pin > 48) {
      ESP_LOGE(TAG, "Invalid MISO pin: %d", config_.miso_pin);
      return false;
    }
    
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = config_.mosi_pin;
    buscfg.miso_io_num = config_.miso_pin;
    buscfg.sclk_io_num = config_.sclk_pin;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 64;
    buscfg.flags = SPICOMMON_BUSFLAG_MASTER;

    // ESP-IDF: SPI1_HOST=0, SPI2_HOST=1, SPI3_HOST=2. Log as SPI1/SPI2/SPI3 for clarity.
    ESP_LOGI(TAG, "Initializing SPI bus: MISO=GPIO%d, MOSI=GPIO%d, SCLK=GPIO%d, Host=SPI%d",
             config_.miso_pin, config_.mosi_pin, config_.sclk_pin, config_.host + 1);

    esp_err_t ret = spi_bus_initialize(config_.host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
      ESP_LOGE(TAG, "Check: SCLK pin GPIO%d is not used by another peripheral", config_.sclk_pin);
      return false;
    }

    ESP_LOGI(TAG, "SPI bus initialized successfully: MISO=GPIO%d, MOSI=GPIO%d, SCLK=GPIO%d",
             config_.miso_pin, config_.mosi_pin, config_.sclk_pin);
    
    // If you see CS/MOSI/MISO but no SCLK on GPIO%d:
    // - Ensure SPI frequency > 0 (addSPIDevice checks this).
    // - On ESP32-C6, GPIO12 is USB-JTAG; use another pin or disable USB-JTAG.
    // - If SPI2 is used elsewhere, try SPI3_HOST in CreateEsp32As5047uSpiBus().
    // - Confirm no other driver (e.g. PSRAM, display) has claimed this GPIO.
    ESP_LOGI(TAG, "SCLK pin GPIO%d is driven by SPI%d; freq=%lu Hz",
             config_.sclk_pin, config_.host + 1, (unsigned long)config_.frequency);

    return true;
  }

  /**
   * @brief Add SPI device to the bus
   * @return true if successful, false otherwise
   */
  bool addSPIDevice() {
    if (config_.frequency == 0) {
      ESP_LOGE(TAG, "SPI frequency is 0 - SCLK will not run. Set SPIParams::FREQUENCY > 0 (e.g. 4000000)");
      return false;
    }
    spi_device_interface_config_t devcfg = {};
    devcfg.command_bits = 0;
    devcfg.address_bits = 0;
    devcfg.dummy_bits = 0;
    devcfg.clock_speed_hz = config_.frequency;
    devcfg.mode = config_.mode;
    devcfg.duty_cycle_pos = 128;  // 50% duty cycle
    devcfg.spics_io_num = config_.cs_pin;
    devcfg.queue_size = config_.queue_size;
    devcfg.cs_ena_pretrans = config_.cs_ena_pretrans;
    devcfg.cs_ena_posttrans = config_.cs_ena_posttrans;
    devcfg.flags = 0;  // No special flags
    devcfg.input_delay_ns = 0;
    devcfg.pre_cb = nullptr;
    devcfg.post_cb = nullptr;

    ESP_LOGI(TAG, "Adding SPI device: CS=GPIO%d, Freq=%lu Hz, Mode=%d, SCLK=GPIO%d",
             config_.cs_pin, config_.frequency, config_.mode, config_.sclk_pin);

    esp_err_t ret = spi_bus_add_device(config_.host, &devcfg, &spi_device_);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
      ESP_LOGE(TAG, "Check: CS pin GPIO%d and SCLK pin GPIO%d are not used elsewhere",
               config_.cs_pin, config_.sclk_pin);
      return false;
    }

    ESP_LOGI(TAG, "SPI device added successfully: CS=GPIO%d, Freq=%lu Hz, Mode=%d",
             config_.cs_pin, config_.frequency, config_.mode);

    return true;
  }
};

/**
 * @brief Factory function to create a configured Esp32As5047uSpiBus instance
 *
 * Creates an Esp32As5047uSpiBus using pin and parameter values from
 * AS5047U_TestConfig namespace (esp32_as5047u_test_config.hpp).
 *
 * @return std::unique_ptr<Esp32As5047uSpiBus> Configured SPI bus interface
 */
inline auto CreateEsp32As5047uSpiBus() noexcept -> std::unique_ptr<Esp32As5047uSpiBus> {
  using namespace AS5047U_TestConfig;

  Esp32As5047uSpiBus::SPIConfig config;

  // SPI pins and host from esp32_as5047u_test_config.hpp
  config.host = (SPIParams::SPI_HOST_ID == 3) ? SPI3_HOST : SPI2_HOST;
  config.miso_pin = static_cast<gpio_num_t>(SPIPins::MISO);
  config.mosi_pin = static_cast<gpio_num_t>(SPIPins::MOSI);
  config.sclk_pin = static_cast<gpio_num_t>(SPIPins::SCLK);
  config.cs_pin = static_cast<gpio_num_t>(SPIPins::CS);

  // SPI parameters from esp32_as5047u_test_config.hpp
  config.frequency = SPIParams::FREQUENCY;
  config.mode = SPIParams::MODE;
  config.queue_size = SPIParams::QUEUE_SIZE;
  config.cs_ena_pretrans = SPIParams::CS_ENA_PRETRANS;
  config.cs_ena_posttrans = SPIParams::CS_ENA_POSTTRANS;

  return std::make_unique<Esp32As5047uSpiBus>(config);
}
