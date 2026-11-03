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

  // Configure advanced features. Each setter returns false when the value did not read back as
  // written or a frame failed; the sticky error flags say why.
  const auto report = [&encoder](bool ok, const char* what) {
    if (ok) {
      ESP_LOGI(TAG, "%s", what);
    } else {
      ESP_LOGE(TAG, "%s: FAILED (sticky flags 0x%04X)", what,
               static_cast<unsigned>(encoder.ClearStickyErrorFlags()));
    }
  };

  report(encoder.SetDynamicAngleCompensation(true), "DAEC enabled");
  report(encoder.SetAdaptiveFilter(true), "Adaptive filter enabled");
  report(encoder.SetFilterParameters(2, 3), "Filter parameters set: K_min code 2, K_max code 3");

  // Configure outputs
  report(encoder.SetABIResolution(12) && encoder.SetUVWPolePairs(5) &&
             encoder.ConfigureInterface(true, true, false),
         "Outputs configured: ABI (12-bit), UVW (5 pole pairs)");

  ESP_LOGI(TAG, "Starting comprehensive reading loop...");

  // Main reading loop
  while (true) {
    // Read angle
    // Every getter returns a Result: .value, and .error (None when the read worked).
    const auto angle = encoder.GetAngle();
    const auto raw_angle = encoder.GetRawAngle();
    if (angle.ok() && raw_angle.ok()) {
      float angle_deg = angle.value * 360.0f / 16384.0f;
      ESP_LOGI(TAG, "Angle: %u (%.2f°), Raw: %u", angle.value, angle_deg, raw_angle.value);
    } else {
      ESP_LOGW(TAG, "Angle read failed (error 0x%04X / 0x%04X)",
               static_cast<uint16_t>(angle.error), static_cast<uint16_t>(raw_angle.error));
    }

    // Read velocity
    const auto vel_deg = encoder.GetVelocityDegPerSec();
    const auto vel_rpm = encoder.GetVelocityRPM();
    if (vel_deg.ok() && vel_rpm.ok()) {
      ESP_LOGI(TAG, "Velocity: %.2f deg/s, %.2f RPM", vel_deg.value, vel_rpm.value);
    } else {
      ESP_LOGW(TAG, "Velocity read failed (error 0x%04X / 0x%04X)",
               static_cast<uint16_t>(vel_deg.error), static_cast<uint16_t>(vel_rpm.error));
    }

    // Read diagnostics
    const auto agc = encoder.GetAGC();
    const auto mag = encoder.GetMagnitude();
    if (agc.ok() && mag.ok()) {
      ESP_LOGI(TAG, "AGC: %u, Magnitude: %u", agc.value, mag.value);
    } else {
      ESP_LOGW(TAG, "AGC/magnitude read failed (error 0x%04X / 0x%04X)",
               static_cast<uint16_t>(agc.error), static_cast<uint16_t>(mag.error));
    }

    // Check for errors
    AS5047U_Error errors = encoder.ClearStickyErrorFlags(); // report each flag once
    if (errors != AS5047U_Error::None) {
      ESP_LOGW(TAG, "Errors: 0x%04X", static_cast<uint16_t>(errors));
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
