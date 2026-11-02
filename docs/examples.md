# Examples

This guide provides complete, working examples demonstrating various use cases for the AS5047U driver.

## Example 1: Basic Angle Reading

This example shows the minimal setup required to read angle values.

```cpp
#include "as5047u.hpp"
#include "esp32_as5047u_bus.hpp"  // Your SPI implementation

void app_main() {
    // 1. Configure SPI bus
    Esp32As5047uBus::SPIConfig config;
    config.miso_pin = GPIO_NUM_2;
    config.mosi_pin = GPIO_NUM_7;
    config.sclk_pin = GPIO_NUM_6;
    config.cs_pin = GPIO_NUM_10;
    config.frequency = 4000000;
    config.mode = 1;
    
    auto bus = std::make_unique<Esp32As5047uBus>(config);
    bus->initialize();
    
    // 2. Create encoder instance
    as5047u::AS5047U encoder(*bus, FrameFormat::SPI_24);
    
    // 3. Read angle in a loop
    while (true) {
        float angle_lsb = encoder.GetAngle(as5047u::AngleUnit::Lsb);
        float angle_deg = encoder.GetAngle(as5047u::AngleUnit::Degrees);
        printf("Angle: %.0f (%.2f°)\n", angle_lsb, angle_deg);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### Explanation

1. **SPI Configuration**: Configure the SPI bus with appropriate pins and settings
2. **Encoder Creation**: Create the encoder instance with SPI bus and frame format
3. **Angle Reading**: Request angle directly in the unit you need

### Expected Output

```
Angle: 8192 (180.00°)
Angle: 8200 (180.18°)
Angle: 8208 (180.35°)
```

---

## Example 2: Velocity Measurement

This example demonstrates reading velocity in different units.

```cpp
#include "as5047u.hpp"

void app_main() {
    // ... SPI setup (same as Example 1)
    
    as5047u::AS5047U encoder(*bus, FrameFormat::SPI_24);
    
    while (true) {
        // Read velocity in different units
        float vel_lsb = encoder.GetVelocity(as5047u::VelocityUnit::Lsb);
        float vel_dps = encoder.GetVelocity(as5047u::VelocityUnit::DegPerSec);
        float vel_rpm = encoder.GetVelocity(as5047u::VelocityUnit::Rpm);
        
        printf("Velocity: %.0f LSB, %.2f deg/s, %.2f RPM\n", 
               vel_lsb, vel_dps, vel_rpm);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### Explanation

Use `GetVelocity(VelocityUnit)` to request the target unit in one call.
Compatibility methods (`GetVelocityDegPerSec()`, `GetVelocityRPM()`, etc.) remain available.

---

## Example 3: Diagnostics and Error Handling

This example shows proper error handling and diagnostic reading.

```cpp
#include "as5047u.hpp"

void app_main() {
    // ... SPI setup
    
    as5047u::AS5047U encoder(*bus, FrameFormat::SPI_24);
    
    while (true) {
        // Read angle with retry on CRC errors
        float angle_deg = encoder.GetAngle(as5047u::AngleUnit::Degrees, 3);  // 3 retries
        
        // Read diagnostics
        uint8_t agc = encoder.GetAGC();
        uint16_t mag = encoder.GetMagnitude();
        
        // Check for errors
        AS5047U_Error errors = encoder.GetStickyErrorFlags();
        if (errors != AS5047U_Error::None) {
            printf("Error detected: 0x%04X\n", static_cast<uint16_t>(errors));
            
            // Check specific errors
            if (errors & static_cast<uint16_t>(AS5047U_Error::AgcWarning)) {
                printf("Warning: AGC at limit - check magnet placement\n");
            }
            if (errors & static_cast<uint16_t>(AS5047U_Error::CrcError)) {
                printf("Warning: CRC error detected\n");
            }
        }
        
        printf("Angle: %.2f°, AGC: %u, Mag: %u\n", angle_deg, agc, mag);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### Explanation

1. **CRC Retry**: Pass retry count to `GetAngle()` for automatic retry on CRC errors
2. **Diagnostics**: Read AGC and magnitude for health monitoring
3. **Error Checking**: Check error flags and handle specific error conditions

---

## Example 4: ABI Incremental Output Configuration

This example configures the AS5047U to output incremental encoder signals (A, B, I).

```cpp
#include "as5047u.hpp"

void app_main() {
    // ... SPI setup
    
    as5047u::AS5047U encoder(*bus, FrameFormat::SPI_24);
    
    // Configure ABI output
    encoder.SetABIResolution(12);  // 12-bit = 4096 PPR
    encoder.SetIndexPulseLength(1); // Index pulse length
    encoder.ConfigureInterface(true, false, false);  // Enable ABI, disable UVW and PWM
    
    printf("ABI output configured: 12-bit (4096 PPR)\n");
    printf("Connect A, B, I pins to your motor controller\n");
    
    // Continue reading angle via SPI
    while (true) {
        uint16_t angle = encoder.GetAngle();
        printf("Angle: %u\n", angle);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### Explanation

1. **Resolution**: Set ABI resolution (10-14 bits)
2. **Index Pulse**: Configure index pulse width
3. **Interface**: Enable ABI output via `ConfigureInterface()`

---

## Example 5: UVW Commutation Output Configuration

This example configures the AS5047U to output 3-phase commutation signals (U, V, W).

```cpp
#include "as5047u.hpp"

void app_main() {
    // ... SPI setup
    
    as5047u::AS5047U encoder(*bus, FrameFormat::SPI_24);
    
    // Configure UVW output
    encoder.SetUVWPolePairs(5);  // 5 pole pairs
    encoder.ConfigureInterface(false, true, false);  // Disable ABI, enable UVW, disable PWM
    
    printf("UVW output configured: 5 pole pairs\n");
    printf("Connect U, V, W pins to your motor controller\n");
    
    // Continue reading angle via SPI
    while (true) {
        uint16_t angle = encoder.GetAngle();
        printf("Angle: %u\n", angle);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### Explanation

1. **Pole Pairs**: Set number of pole pairs (1-7) for your motor
2. **Interface**: Enable UVW output via `ConfigureInterface()`

---

## Example 6: Advanced Features (DAEC and Filtering)

This example demonstrates enabling DAEC and adaptive filtering for high-performance applications.

```cpp
#include "as5047u.hpp"

void app_main() {
    // ... SPI setup
    
    as5047u::AS5047U encoder(*bus, FrameFormat::SPI_24);
    
    // Enable advanced features
    encoder.SetDynamicAngleCompensation(true);  // Enable DAEC
    encoder.SetAdaptiveFilter(true);            // Enable DFS
    encoder.SetFilterParameters(2, 3);         // Set filter parameters
    
    printf("DAEC and adaptive filter enabled\n");
    
    while (true) {
        // Read compensated angle (with DAEC)
        uint16_t angle = encoder.GetAngle();
        
        // Read raw angle (without DAEC) for comparison
        uint16_t raw_angle = encoder.GetRawAngle();
        
        printf("Compensated: %u, Raw: %u\n", angle, raw_angle);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### Explanation

1. **DAEC**: Dynamic Angle Error Compensation for low-latency angle correction
2. **Adaptive Filter**: Dynamic Filter System for noise reduction
3. **Filter Parameters**: Set K_min and K_max for filter strength

---

## Example 7: Zero Position Calibration

This example shows how to set a zero reference position.

```cpp
#include "as5047u.hpp"

void app_main() {
    // ... SPI setup
    
    as5047u::AS5047U encoder(*bus, FrameFormat::SPI_24);
    
    // Wait for sensor to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Read current angle
    uint16_t current_angle = encoder.GetAngle();
    
    // Set it as zero reference
    if (encoder.SetZeroPosition(current_angle)) {
        printf("Zero position set to: %u\n", current_angle);
    } else {
        printf("Failed to set zero position\n");
    }
    
    // Now angle readings will be relative to this position
    while (true) {
        uint16_t angle = encoder.GetAngle();
        printf("Angle: %u\n", angle);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### Explanation

1. **Stabilize**: Wait for sensor to stabilize after power-on
2. **Read Current**: Read the current angle position
3. **Set Zero**: Set the current position as zero reference

---

## Example 8: Full Feature Configuration

This example demonstrates configuring all features together.

```cpp
#include "as5047u.hpp"

void app_main() {
    // ... SPI setup
    
    as5047u::AS5047U encoder(*bus, FrameFormat::SPI_24);
    
    // Configure all features
    encoder.SetZeroPosition(0);
    encoder.SetDirection(true);  // Clockwise = positive
    encoder.SetDynamicAngleCompensation(true);
    encoder.SetAdaptiveFilter(true);
    encoder.SetFilterParameters(2, 3);
    encoder.SetABIResolution(12);
    encoder.SetUVWPolePairs(5);
    encoder.ConfigureInterface(true, true, false);  // ABI + UVW
    
    printf("Full configuration applied\n");
    
    // Dump status for verification
    encoder.DumpStatus();
    
    // Main loop
    while (true) {
        float angle_deg = encoder.GetAngle(as5047u::AngleUnit::Degrees);
        float vel_dps = encoder.GetVelocity(as5047u::VelocityUnit::DegPerSec);
        uint8_t agc = encoder.GetAGC();
        
        printf("Angle: %.2f°, Vel: %.2f deg/s, AGC: %u\n", angle_deg, vel_dps, agc);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### Explanation

This example shows a complete configuration with all major features enabled.

---

## Running the Examples

### ESP32

```bash
cd examples/esp32
idf.py build flash monitor
```

### Other Platforms

For other platforms, implement the SPI interface and compile with C++20 support:

```bash
g++ -std=c++20 -I inc/ your_code.cpp src/as5047u.cpp
```

## Next Steps

- Review the [API Reference](api_reference.md) for method details
- Check [Troubleshooting](troubleshooting.md) if you encounter issues
- Explore the [examples directory](../examples/) for more examples

---

**Navigation**
⬅️ [API Reference](api_reference.md) | [Next: Troubleshooting ➡️](troubleshooting.md) | [Back to Index](index.md)

