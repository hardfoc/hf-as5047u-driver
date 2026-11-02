/**
 * @file full_features_example.cpp
 * @brief Comprehensive example demonstrating all as5047u::AS5047U features
 *
 * This example demonstrates:
 * - All sensor features enabled
 * - Angle, velocity, diagnostics
 * - ABI, UVW configuration
 * - DAEC and filter settings
 *
 * @author N3b3x
 * @date 2025
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <memory>
#include <stdio.h>

#include "../../../inc/as5047u.hpp"
#include "esp32_as5047u_bus.hpp"
#include "esp32_as5047u_test_config.hpp"

static const char* TAG = "AS5047U_Full";

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "as5047u::AS5047U Full Features Example");
  ESP_LOGI(TAG, "=============================");
  ESP_LOGI(TAG, "Driver version: %s", as5047u::GetDriverVersion());

  // Create SPI bus using centralized test config
  auto bus = CreateEsp32As5047uSpiBus();

  if (!bus) {
    ESP_LOGE(TAG, "Failed to create SPI bus");
    return;
  }

  if (!bus->initialize()) {
    ESP_LOGE(TAG, "Failed to initialize SPI bus");
    return;
  }

  // Create encoder instance
  as5047u::AS5047U encoder(*bus, FrameFormat::SPI_24);

  ESP_LOGI(TAG, "as5047u::AS5047U encoder initialized");

  // Configure advanced features
  encoder.SetDynamicAngleCompensation(true);
  ESP_LOGI(TAG, "DAEC enabled");

  encoder.SetAdaptiveFilter(true);
  ESP_LOGI(TAG, "Adaptive filter enabled");

  encoder.SetFilterParameters(2, 3);
  ESP_LOGI(TAG, "Filter parameters set: K_min=2, K_max=3");

  // Configure outputs
  encoder.SetABIResolution(12);
  encoder.SetUVWPolePairs(5);
  encoder.ConfigureInterface(true, true, false);
  ESP_LOGI(TAG, "Outputs configured: ABI (12-bit), UVW (5 pole pairs)");

  ESP_LOGI(TAG, "Starting comprehensive reading loop...");

  // Main reading loop
  while (true) {
    // Read angle
    uint16_t angle = encoder.GetAngle();
    uint16_t raw_angle = encoder.GetRawAngle();
    float angle_deg = angle * 360.0f / 16384.0f;
    ESP_LOGI(TAG, "Angle: %u (%.2f°), Raw: %u", angle, angle_deg, raw_angle);

    // Read velocity
    float vel_deg = encoder.GetVelocityDegPerSec();
    float vel_rpm = encoder.GetVelocityRPM();
    ESP_LOGI(TAG, "Velocity: %.2f deg/s, %.2f RPM", vel_deg, vel_rpm);

    // Read diagnostics
    uint8_t agc = encoder.GetAGC();
    uint16_t mag = encoder.GetMagnitude();
    ESP_LOGI(TAG, "AGC: %u, Magnitude: %u", agc, mag);

    // Check for errors
    AS5047U_Error errors = encoder.ClearStickyErrorFlags(); // report each flag once
    if (errors != AS5047U_Error::None) {
      ESP_LOGW(TAG, "Errors: 0x%04X", static_cast<uint16_t>(errors));
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
