# AS5047U — CMake Integration Guide

How to consume the AS5047U driver in your CMake or ESP-IDF project.


---

## Quick Start (Generic CMake)

```cmake
add_subdirectory(external/hf-as5047u-driver)
target_link_libraries(my_app PRIVATE hf::as5047u)
```

---

## ESP-IDF Integration

The driver ships with an ESP-IDF component wrapper in
`examples/esp32/components/hf_as5047u/`.

In your `main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES hf_as5047u driver freertos
)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
```

---

## AS5047U-Specific Build Variables

| Variable | Value |
|----------|-------|
| `HF_AS5047U_TARGET_NAME` | `hf_as5047u` |
| `HF_AS5047U_VERSION` | Current `MAJOR.MINOR.PATCH` |
| `HF_AS5047U_PUBLIC_INCLUDE_DIRS` | `inc/` + generated header dir |
| `HF_AS5047U_SOURCE_FILES` | `""` (header-only) |
| `HF_AS5047U_IDF_REQUIRES` | `driver freertos` |

---

## Version Header

The build system generates `as5047u_version.h` at configure time:

```c
#define HF_AS5047U_VERSION_MAJOR  1
#define HF_AS5047U_VERSION_MINOR  0
#define HF_AS5047U_VERSION_PATCH  0
#define HF_AS5047U_VERSION_STRING "1.0.0"
```

---

**Navigation**
⬅️ [Back to Documentation Index](index.md)
