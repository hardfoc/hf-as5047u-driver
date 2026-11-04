/**
 * @file configuration_example.cpp
 * @brief Configuration example with ABI, UVW, zero position and direction setup
 *
 * This example demonstrates:
 * - ABI output configuration
 * - UVW commutation setup
 * - Zero position and direction setting
 * - Checking every setter's result (false: the write did not read back, or the bus failed)
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

static const char* TAG = "AS5047U_Config";

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "as5047u::AS5047U Configuration Example");
  ESP_LOGI(TAG, "==============================");
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

  // Each setter returns false when the value did not read back as written or a frame failed;
  // the sticky error flags say why.
  const auto report = [&encoder](bool ok, const char* what) {
    if (ok) {
      ESP_LOGI(TAG, "%s", what);
    } else {
      ESP_LOGE(TAG, "%s: FAILED (sticky flags 0x%04X)", what,
               static_cast<unsigned>(encoder.ClearStickyErrorFlags()));
    }
    return ok;
  };

  bool ok = true;
  ok &= report(encoder.SetABIResolution(12), "ABI output configured: 12-bit resolution");
  ok &= report(encoder.SetUVWPolePairs(5), "UVW commutation configured: 5 pole pairs");
  ok &= report(encoder.ConfigureInterface(true, false, false), "Interface configured: ABI enabled");
  ok &= report(encoder.SetZeroPosition(0), "Zero position set: 0");
  ok &= report(encoder.SetDirection(true), "Direction set: clockwise");

  ESP_LOGI(TAG, "Configuration %s. Starting angle reading...", ok ? "complete" : "incomplete");

  // Main reading loop
  while (true) {
    const auto angle = encoder.GetAngle();
    if (angle.ok()) {
      float angle_deg = angle.value * 360.0f / 16384.0f;
      ESP_LOGI(TAG, "Angle: %u (%.2f°)", angle.value, angle_deg);
    } else {
      ESP_LOGW(TAG, "Angle read failed (error 0x%04X)", static_cast<uint16_t>(angle.error));
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
