/**
 * @file ports.cpp
 * @brief Instantiates the driver over the documented port, so every member compiles in full.
 */
#include "my_spi.hpp"

#include "as5047u.hpp"

template class as5047u::AS5047U<MySpi>;
