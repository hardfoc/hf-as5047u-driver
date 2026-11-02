# Platform Integration Guide

This guide explains how to implement the hardware abstraction interface for the AS5047U driver on your platform.

## Understanding CRTP (Curiously Recurring Template Pattern)

The AS5047U driver uses **CRTP** (Curiously Recurring Template Pattern) for hardware abstraction. This design choice provides several critical benefits for embedded systems:

### Why CRTP Instead of Virtual Functions?

#### 1. **Zero Runtime Overhead**
- **Virtual functions**: Require a vtable lookup (indirect call) = ~5-10 CPU cycles overhead per call
- **CRTP**: Direct function calls = 0 overhead, compiler can inline
- **Impact**: In time-critical embedded code reading angles at high rates, this matters significantly

#### 2. **Compile-Time Polymorphism**
- **Virtual functions**: Runtime dispatch - the compiler cannot optimize across the abstraction boundary
- **CRTP**: Compile-time dispatch - full optimization, dead code elimination, constant propagation
- **Impact**: Smaller code size, faster execution

#### 3. **Memory Efficiency**
- **Virtual functions**: Each object needs a vtable pointer (4-8 bytes)
- **CRTP**: No vtable pointer needed
- **Impact**: Critical in memory-constrained systems (many MCUs have <64KB RAM)

#### 4. **Type Safety**
- **Virtual functions**: Runtime errors if method not implemented
- **CRTP**: Compile-time errors if method not implemented
- **Impact**: Catch bugs at compile time, not in the field

### How CRTP Works

```cpp
// Base template class (from as5047u_spi_interface.hpp)
template <typename Derived>
class SpiInterface {
public:
    void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
        // Cast 'this' to Derived* and call the derived implementation
        static_cast<Derived*>(this)->transfer(tx, rx, len);
    }
};

// Your implementation
class MySPI : public as5047u::SpiInterface<MySPI> {
public:
    // This method is called directly (no virtual overhead)
    void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
        // Your platform-specific SPI code
    }
};
```

The key insight: `static_cast<Derived*>(this)` allows the base class to call methods on the derived class **at compile time**, not runtime.

### Performance Comparison

| Aspect | Virtual Functions | CRTP |
|--------|------------------|------|
| Function call overhead | ~5-10 cycles | 0 cycles (inlined) |
| Code size | Larger (vtables) | Smaller (optimized) |
| Memory per object | +4-8 bytes (vptr) | 0 bytes |
| Compile-time checks | No | Yes |
| Optimization | Limited | Full |

## Interface Definition

The AS5047U driver requires you to implement the following interface:

```cpp
template <typename Derived>
class SpiInterface {
public:
    // Required method
    void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len);
};
```

**Key Requirements:**
- `tx`: Pointer to data to transmit (can be `nullptr` to send zeros)
- `rx`: Pointer to buffer for received data (can be `nullptr` to ignore received data)
- `len`: Number of bytes to transfer
- **CS Management**: Your implementation must assert chip select before transfer and deassert after
- **SPI Mode**: AS5047U uses SPI Mode 1 (CPOL=0, CPHA=1) or Mode 3 (CPOL=1, CPHA=1)
- **Bit Order**: MSB first

## Implementation Steps

### Step 1: Create Your Implementation Class

```cpp
#include "as5047u_spi_interface.hpp"

class MyPlatformSPI : public as5047u::SpiInterface<MyPlatformSPI> {
private:
    // Your platform-specific members
    spi_device_handle_t spi_device_;  // Example for ESP32
    
public:
    // Constructor
    MyPlatformSPI(spi_device_handle_t device) : spi_device_(device) {}
    
    // Implement required method
    void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
        // Assert CS
        // Perform SPI transfer
        // Deassert CS
    }
};
```

### Step 2: Platform-Specific Examples

#### ESP32 (ESP-IDF)

```cpp
#include "driver/spi_master.h"
#include "as5047u_spi_interface.hpp"

class Esp32SPIBus : public as5047u::SpiInterface<Esp32SPIBus> {
private:
    spi_device_handle_t spi_device_;
    
public:
    Esp32SPIBus(spi_device_handle_t device) : spi_device_(device) {}
    
    void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
        spi_transaction_t trans = {};
        trans.length = len * 8;  // Length in bits
        trans.tx_buffer = tx;
        trans.rx_buffer = rx;
        
        esp_err_t ret = spi_device_transmit(spi_device_, &trans);
        if (ret != ESP_OK) {
            // Handle error
        }
    }
};
```

**Full ESP32 Example:**

```cpp
// From examples/esp32/main/esp32_as5047u_bus.hpp
class Esp32As5047uBus : public as5047u::SpiInterface<Esp32As5047uBus> {
public:
    struct SPIConfig {
        spi_host_device_t host = SPI2_HOST;
        gpio_num_t miso_pin = GPIO_NUM_2;
        gpio_num_t mosi_pin = GPIO_NUM_7;
        gpio_num_t sclk_pin = GPIO_NUM_6;
        gpio_num_t cs_pin = GPIO_NUM_10;
        uint32_t frequency = 4000000;
        uint8_t mode = 1;  // SPI Mode 1
    };
    
    explicit Esp32As5047uBus(const SPIConfig& config);
    ~Esp32As5047uBus();
    
    bool initialize();
    void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len);
    
private:
    SPIConfig config_;
    spi_device_handle_t spi_device_;
    bool initialized_;
};
```

#### STM32 (HAL)

```cpp
#include "stm32f4xx_hal.h"
#include "as5047u_spi_interface.hpp"

extern SPI_HandleTypeDef hspi1;

class STM32SPIBus : public as5047u::SpiInterface<STM32SPIBus> {
private:
    GPIO_TypeDef* cs_port_;
    uint16_t cs_pin_;
    
public:
    STM32SPIBus(GPIO_TypeDef* cs_port, uint16_t cs_pin) 
        : cs_port_(cs_port), cs_pin_(cs_pin) {}
    
    void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
        // Assert CS
        HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
        
        // Perform transfer
        HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi1, 
                                                          (uint8_t*)tx, 
                                                          (uint8_t*)rx, 
                                                          len, 
                                                          HAL_MAX_DELAY);
        
        // Deassert CS
        HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
        
        if (status != HAL_OK) {
            // Handle error
        }
    }
};
```

#### Arduino

```cpp
#include <SPI.h>
#include "as5047u_spi_interface.hpp"

class ArduinoSPIBus : public as5047u::SpiInterface<ArduinoSPIBus> {
private:
    uint8_t cs_pin_;
    
public:
    ArduinoSPIBus(uint8_t cs_pin) : cs_pin_(cs_pin) {
        pinMode(cs_pin_, OUTPUT);
        digitalWrite(cs_pin_, HIGH);
    }
    
    void transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) {
        // Begin transaction with AS5047U settings
        SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE1));
        
        // Assert CS
        digitalWrite(cs_pin_, LOW);
        
        // Transfer data
        for (size_t i = 0; i < len; i++) {
            uint8_t byte = SPI.transfer(tx ? tx[i] : 0);
            if (rx) {
                rx[i] = byte;
            }
        }
        
        // Deassert CS
        digitalWrite(cs_pin_, HIGH);
        
        // End transaction
        SPI.endTransaction();
    }
};
```

## Common Pitfalls

### Don't Use Virtual Functions

```cpp
// WRONG - defeats the purpose of CRTP
class MyBus : public as5047u::SpiInterface<MyBus> {
public:
    virtual void transfer(...) override {  // ❌ Virtual keyword not needed
        // ...
    }
};
```

### Correct CRTP Implementation

```cpp
// CORRECT - no virtual keyword
class MyBus : public as5047u::SpiInterface<MyBus> {
public:
    void transfer(...) {  // ✅ Direct implementation
        // ...
    }
};
```

### Don't Forget the Template Parameter

```cpp
// WRONG - missing template parameter
class MyBus : public as5047u::SpiInterface {  // ❌ Compiler error
    // ...
};
```

### Correct Template Parameter

```cpp
// CORRECT - pass your class as template parameter
class MyBus : public as5047u::SpiInterface<MyBus> {  // ✅
    // ...
};
```

### Important: AS5047U SPI Pin Mapping

The AS5047U uses a **unique SPI pin mapping**:
- MCU MOSI → AS5047U MISO (data from MCU to sensor)
- MCU MISO → AS5047U MOSI (data from sensor to MCU)

This is opposite to standard SPI naming. Make sure your wiring matches this!

## Testing Your Implementation

After implementing the interface, test it:

```cpp
MyPlatformSPI spi(/* your config */);
as5047u::AS5047U encoder(spi, FrameFormat::SPI_24);

// Test basic operations
uint16_t angle = encoder.GetAngle();
uint8_t agc = encoder.GetAGC();
```

## Next Steps

- See [Configuration](configuration.md) for driver configuration options
- Check [Examples](examples.md) for complete usage examples
- Review [API Reference](api_reference.md) for all available methods

---

**Navigation**
⬅️ [Hardware Setup](hardware_setup.md) | [Next: Configuration ➡️](configuration.md)

