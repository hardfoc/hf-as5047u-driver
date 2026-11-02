# Consumer of an installed hf-as5047u-driver: find_package() + the documented target name. Built by the
# hf_as5047u_install_consumer test (tests/install_consumer.cmake) against a fresh `cmake --install`.
cmake_minimum_required(VERSION 3.16)
project(hf_as5047u_install_consumer LANGUAGES CXX)

find_package(hf_as5047u CONFIG REQUIRED)

add_library(consumer OBJECT main.cpp)
target_link_libraries(consumer PRIVATE hf::as5047u)
