/**
 * @file test_harness.hpp
 * @brief Minimal dependency-free test registry and check macros for the host unit tests.
 */
#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace as5047u_test {

struct TestCase {
  const char* name;
  std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
  static std::vector<TestCase> tests;
  return tests;
}

inline int& failureCount() {
  static int failures = 0;
  return failures;
}

struct Registrar {
  Registrar(const char* name, std::function<void()> fn) {
    registry().push_back({name, std::move(fn)});
  }
};

inline void reportFailure(const char* file, int line, const std::string& what) {
  ++failureCount();
  std::printf("    FAIL %s:%d: %s\n", file, line, what.c_str());
}

} // namespace as5047u_test

#define AS_TEST_CONCAT2(a, b) a##b
#define AS_TEST_CONCAT(a, b) AS_TEST_CONCAT2(a, b)

/// Define a test case.
#define TEST_CASE(name)                                                                            \
  static void AS_TEST_CONCAT(test_fn_, __LINE__)();                                               \
  static const ::as5047u_test::Registrar AS_TEST_CONCAT(test_reg_, __LINE__)(                     \
      name, &AS_TEST_CONCAT(test_fn_, __LINE__));                                                 \
  static void AS_TEST_CONCAT(test_fn_, __LINE__)()

/// Non-fatal boolean check.
#define CHECK(cond)                                                                                \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      ::as5047u_test::reportFailure(__FILE__, __LINE__, #cond);                                    \
    }                                                                                              \
  } while (0)

/// Non-fatal equality check with values printed as unsigned long long.
#define CHECK_EQ(a, b)                                                                             \
  do {                                                                                             \
    const auto as_test_a = (a);                                                                   \
    const auto as_test_b = (b);                                                                   \
    if (!(as_test_a == as_test_b)) {                                                             \
      ::as5047u_test::reportFailure(                                                               \
          __FILE__, __LINE__,                                                                      \
          std::string(#a " == " #b " (") +                                                         \
              std::to_string(static_cast<unsigned long long>(as_test_a)) + " vs " +               \
              std::to_string(static_cast<unsigned long long>(as_test_b)) + ")");                  \
    }                                                                                              \
  } while (0)

/// Fatal check: stop the current test case.
#define REQUIRE(cond)                                                                              \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      ::as5047u_test::reportFailure(__FILE__, __LINE__, #cond);                                    \
      return;                                                                                      \
    }                                                                                              \
  } while (0)
