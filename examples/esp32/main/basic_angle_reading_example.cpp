/**
 * @file basic_angle_reading_example.cpp
 * @brief Basic angle reading example with diagnostics
 *
 * This example demonstrates:
 * - Basic initialization
 * - Angle reading in LSB and degrees
 * - AGC and magnitude diagnostics
 * - Error flag monitoring
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

// Forward declaration of a detailed diagnostics routine that
// exercises the low-level register API and verifies that
// configuration writes can be read back correctly.
static void run_diagnostics(as5047u::AS5047U<Esp32As5047uSpiBus>& encoder);

static const char* TAG = "AS5047U_Basic";

extern "C" void app_main(void) {
  // Enable verbose logging for SPI transactions
  esp_log_level_set("Esp32As5047uSpiBus", ESP_LOG_DEBUG);
  
  ESP_LOGI(TAG, "as5047u::AS5047U Basic Angle Reading Example");
  ESP_LOGI(TAG, "===================================");
  ESP_LOGI(TAG, "Driver version: %s", as5047u::GetDriverVersion());

  // Create SPI bus using centralized configuration from esp32_as5047u_test_config.hpp
  // This ensures consistency across all examples and makes it easy to change
  // hardware configuration in one place. The factory function uses:
  // - SPIPins::MISO, MOSI, SCLK, CS from test config
  // - SPIParams::FREQUENCY, MODE, etc. from test config
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
  // Use 16-bit frame format (no CRC) for maximum compatibility.
  as5047u::AS5047U encoder(*bus, FrameFormat::SPI_16);

  ESP_LOGI(TAG, "as5047u::AS5047U encoder initialized");

  // Give the sensor some time after power-on (datasheet tpon ~10ms).
  vTaskDelay(pdMS_TO_TICKS(20));

  // Run a one-shot diagnostics and configuration verification pass.
  // This includes SPI communication health check
  run_diagnostics(encoder);

  ESP_LOGI(TAG, "Starting angle reading loop...");

  // Main reading loop
  while (true) {
    // Read angle
    // Every getter returns a Result: .value, and .error (None when the read worked).
    const auto angle = encoder.GetAngle();
    if (angle.ok()) {
      float angle_deg = angle.value * 360.0f / 16384.0f;
      ESP_LOGI(TAG, "Angle: %u (%.2f°)", angle.value, angle_deg);
    } else {
      ESP_LOGW(TAG, "Angle read failed (error 0x%04X)", static_cast<uint16_t>(angle.error));
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
      ESP_LOGW(TAG, "Errors detected: 0x%04X", static_cast<uint16_t>(errors));
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Check if SPI communication is actually working by verifying chip responses
static bool check_spi_communication_health(as5047u::AS5047U<Esp32As5047uSpiBus>& encoder) {
  ESP_LOGI(TAG, "Checking SPI communication health...");
  
  bool healthy = true;
  
  // Test 1: ERRFL should clear after read (self-clearing register)
  // If chip is responding, first read may have value, second should be 0x0000
  AS5047U_REG::ERRFL errfl1{};
  if (!encoder.ReadReg(errfl1)) {
    ESP_LOGW(TAG, "ERRFL read failed (value shown as 0)");
  }
  vTaskDelay(pdMS_TO_TICKS(1)); // Small delay
  AS5047U_REG::ERRFL errfl2{};
  if (!encoder.ReadReg(errfl2)) {
    ESP_LOGW(TAG, "ERRFL read failed (value shown as 0)");
  }
  ESP_LOGI(TAG, "ERRFL test: first=0x%04X, second=0x%04X (should clear)", errfl1.value, errfl2.value);
  
  // Test 2: DIA register should have VDD_mode bit set if chip is powered
  // Even if chip is uninitialized, VDD_mode should indicate power state
  AS5047U_REG::DIA dia{};
  if (!encoder.ReadReg(dia)) {
    ESP_LOGW(TAG, "DIA read failed (value shown as 0)");
  }
  ESP_LOGI(TAG, "DIA register: 0x%04X (VDD_mode=%u)", dia.value, dia.bits.VDD_mode);
  
  if (dia.value == 0x0000 && errfl1.value == 0x0000 && errfl2.value == 0x0000) {
    ESP_LOGE(TAG, "⚠️  CRITICAL: All registers read 0x0000 - likely hardware issue!");
    ESP_LOGE(TAG, "   Possible causes:");
    ESP_LOGE(TAG, "   1. MISO line not connected or stuck low");
    ESP_LOGE(TAG, "   2. CS (chip select) pin not connected");
    ESP_LOGE(TAG, "   3. Chip not powered (check VDD/VDD3V3)");
    ESP_LOGE(TAG, "   4. Wrong SPI pins configured");
    healthy = false;
  }
  
  // Test 3: Check for FramingError or CommandError in ERRFL
  // These would indicate SPI communication problems
  if (errfl1.value != 0x0000) {
    if (errfl1.bits.Framing_error) {
      ESP_LOGW(TAG, "⚠️  FramingError detected - SPI frame alignment issue");
      healthy = false;
    }
    if (errfl1.bits.Command_error) {
      ESP_LOGW(TAG, "⚠️  CommandError detected - invalid SPI command");
      healthy = false;
    }
  }
  
  // Test 4: DIA.VDD_mode should be set if chip is powered (0=3.3V mode, 1=5V mode)
  // If it's 0 and all other bits are 0, chip might not be responding
  if (dia.value == 0x0000) {
    ESP_LOGW(TAG, "⚠️  DIA register is 0x0000 - chip may not be powered or responding");
  }
  
  if (healthy) {
    ESP_LOGI(TAG, "✅ SPI communication appears healthy");
  } else {
    ESP_LOGE(TAG, "❌ SPI communication health check FAILED");
  }
  
  return healthy;
}

// Detailed diagnostics: read core registers, toggle a few configuration bits,
// and read them back so we can verify that SPI writes are actually sticking
// and that we are not just seeing a stuck-low MISO line.
static void run_diagnostics(as5047u::AS5047U<Esp32As5047uSpiBus>& encoder) {
  ESP_LOGI(TAG, "Running AS5047U diagnostics and configuration verification...");

  // 0) First check if SPI communication is working at all
  bool comm_ok = check_spi_communication_health(encoder);
  if (!comm_ok) {
    ESP_LOGE(TAG, "SPI communication health check failed - skipping further diagnostics");
    return;
  }

  // 1) Dump comprehensive status using the driver's helper.
  encoder.DumpStatus();

  // 2) Read core status/diagnostic registers directly.
  AS5047U_REG::ERRFL errfl{};
  if (!encoder.ReadReg(errfl)) {
    ESP_LOGW(TAG, "ERRFL read failed (value shown as 0)");
  }
  ESP_LOGI(TAG, "ERRFL raw: 0x%04X", errfl.value);

  AS5047U_REG::DIA dia{};
  if (!encoder.ReadReg(dia)) {
    ESP_LOGW(TAG, "DIA read failed (value shown as 0)");
  }
  ESP_LOGI(TAG,
           "DIA: 0x%04X (VDD_mode=%u LoopsFinished=%u OffComp_finished=%u AGC_finished=%u "
           "CORDIC_overflow=%u MagHalf_flag=%u)",
           dia.value, dia.bits.VDD_mode, dia.bits.LoopsFinished, dia.bits.OffComp_finished,
           dia.bits.AGC_finished, dia.bits.CORDIC_overflow_flag, dia.bits.MagHalf_flag);

  AS5047U_REG::AGC agc_reg{};
  if (!encoder.ReadReg(agc_reg)) {
    ESP_LOGW(TAG, "AGC read failed (value shown as 0)");
  }
  ESP_LOGI(TAG, "AGC register raw: 0x%04X (AGC_value=%u)", agc_reg.value, agc_reg.bits.AGC_value);

  AS5047U_REG::MAG mag_reg{};
  if (!encoder.ReadReg(mag_reg)) {
    ESP_LOGW(TAG, "MAG read failed (value shown as 0)");
  }
  ESP_LOGI(TAG, "MAG register raw: 0x%04X (MAG_value=%u)", mag_reg.value, mag_reg.bits.MAG_value);

  AS5047U_REG::ANGLEUNC angle_unc{};
  if (!encoder.ReadReg(angle_unc)) {
    ESP_LOGW(TAG, "ANGLEUNC read failed (value shown as 0)");
  }
  AS5047U_REG::ANGLECOM angle_com{};
  if (!encoder.ReadReg(angle_com)) {
    ESP_LOGW(TAG, "ANGLECOM read failed (value shown as 0)");
  }
  ESP_LOGI(TAG, "ANGLEUNC raw: 0x%04X (value=%u)", angle_unc.value, angle_unc.bits.ANGLEUNC_value);
  ESP_LOGI(TAG, "ANGLECOM raw: 0x%04X (value=%u)", angle_com.value, angle_com.bits.ANGLECOM_value);

  // 3) Verify that non-volatile configuration registers can be written and read back.
  //    We toggle a few bits and immediately restore the original value.

  // SETTINGS2: test NOISESET and PWMon bits.
  AS5047U_REG::SETTINGS2 s2_orig{};
  if (!encoder.ReadReg(s2_orig)) {
    // Without the original value there is nothing to toggle and restore: writing it back
    // would put 0 into the register.
    ESP_LOGE(TAG, "SETTINGS2 read failed - skipping the write tests");
    return;
  }
  ESP_LOGI(TAG, "SETTINGS2 original: 0x%04X", s2_orig.value);

  auto s2_test = s2_orig;
  s2_test.bits.NOISESET ^= 1u;
  s2_test.bits.PWMon ^= 1u;
  bool s2_write_ok = encoder.WriteReg(s2_test);
  vTaskDelay(pdMS_TO_TICKS(1)); // Small delay for write to complete
  AS5047U_REG::SETTINGS2 s2_after{};
  if (!encoder.ReadReg(s2_after)) {
    ESP_LOGW(TAG, "SETTINGS2 read failed (value shown as 0)");
  }
  ESP_LOGI(TAG,
           "SETTINGS2 test write ok=%d, written=0x%04X, read-back=0x%04X "
           "(NOISESET=%u PWMon=%u)",
           s2_write_ok ? 1 : 0, s2_test.value, s2_after.value, s2_after.bits.NOISESET,
           s2_after.bits.PWMon);
  
  // CRITICAL CHECK: If write reports success but read-back doesn't match, SPI isn't working
  if (s2_write_ok && s2_after.value != s2_test.value) {
    ESP_LOGE(TAG, "❌ CRITICAL: Write reported success but read-back doesn't match!");
    ESP_LOGE(TAG, "   This indicates SPI writes are not reaching the chip.");
    ESP_LOGE(TAG, "   Check: CS pin connection, chip power, SPI wiring");
  }

  // Restore original SETTINGS2.
  if (!encoder.WriteReg(s2_orig)) {
    ESP_LOGW(TAG, "Failed to restore original SETTINGS2");
  }

  // DISABLE: toggle ABI_off and UVW_off.
  AS5047U_REG::DISABLE dis_orig{};
  if (!encoder.ReadReg(dis_orig)) {
    // Without the original value there is nothing to toggle and restore: writing it back
    // would put 0 into the register.
    ESP_LOGE(TAG, "DISABLE read failed - skipping the write tests");
    return;
  }
  ESP_LOGI(TAG, "DISABLE original: 0x%04X", dis_orig.value);

  auto dis_test = dis_orig;
  dis_test.bits.ABI_off ^= 1u;
  dis_test.bits.UVW_off ^= 1u;
  bool dis_write_ok = encoder.WriteReg(dis_test);
  vTaskDelay(pdMS_TO_TICKS(1)); // Small delay for write to complete
  AS5047U_REG::DISABLE dis_after{};
  if (!encoder.ReadReg(dis_after)) {
    ESP_LOGW(TAG, "DISABLE read failed (value shown as 0)");
  }
  ESP_LOGI(TAG,
           "DISABLE test write ok=%d, written=0x%04X, read-back=0x%04X "
           "(ABI_off=%u UVW_off=%u)",
           dis_write_ok ? 1 : 0, dis_test.value, dis_after.value, dis_after.bits.ABI_off,
           dis_after.bits.UVW_off);
  
  // CRITICAL CHECK: If write reports success but read-back doesn't match, SPI isn't working
  if (dis_write_ok && dis_after.value != dis_test.value) {
    ESP_LOGE(TAG, "❌ CRITICAL: DISABLE write reported success but read-back doesn't match!");
    ESP_LOGE(TAG, "   This confirms SPI writes are not reaching the chip.");
  }

  // Restore original DISABLE.
  if (!encoder.WriteReg(dis_orig)) {
    ESP_LOGW(TAG, "Failed to restore original DISABLE");
  }

  // SETTINGS3: read back ABIRES and HYS to confirm they are not stuck at zero.
  AS5047U_REG::SETTINGS3 s3{};
  if (!encoder.ReadReg(s3)) {
    ESP_LOGW(TAG, "SETTINGS3 read failed (value shown as 0)");
  }
  ESP_LOGI(TAG, "SETTINGS3: 0x%04X (UVWPP=%u HYS=%u ABIRES=%u)", s3.value, s3.bits.UVWPP,
           s3.bits.HYS, s3.bits.ABIRES);

  // Finally, read sticky error flags after the above transactions.
  auto sticky = encoder.GetStickyErrorFlags();
  ESP_LOGI(TAG, "Sticky error flags after diagnostics: 0x%04X",
           static_cast<uint16_t>(sticky));

  ESP_LOGI(TAG, "Diagnostics completed.");
}
