#===============================================================================
# AS5047U Driver - Root CMake
# Defines the main build target for the driver. Minimal, platform-agnostic.
#===============================================================================

cmake_minimum_required(VERSION 3.16)

project(hf_as5047u_driver LANGUAGES CXX)

#===============================================================================
# Load shared build settings (target name, includes, sources, dependencies)
#===============================================================================
include(${CMAKE_CURRENT_LIST_DIR}/cmake/hf_as5047u_build_settings.cmake)

#===============================================================================
# Target type selection
# INTERFACE for header-only, STATIC if sources present
#===============================================================================
if(HF_AS5047U_SOURCE_FILES)
    add_library(${HF_AS5047U_TARGET_NAME} STATIC ${HF_AS5047U_SOURCE_FILES})
    target_include_directories(${HF_AS5047U_TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
            $<BUILD_INTERFACE:${HF_AS5047U_VERSION_HEADER_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    target_compile_features(${HF_AS5047U_TARGET_NAME} PUBLIC cxx_std_20)
else()
    add_library(${HF_AS5047U_TARGET_NAME} INTERFACE)
    target_include_directories(${HF_AS5047U_TARGET_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
            $<BUILD_INTERFACE:${HF_AS5047U_VERSION_HEADER_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    target_compile_features(${HF_AS5047U_TARGET_NAME} INTERFACE cxx_std_20)
endif()

# =============================================================================
# Namespaced Consumer Alias
# =============================================================================
add_library(hf::as5047u ALIAS ${HF_AS5047U_TARGET_NAME})

#===============================================================================
# Versioning
# Version variables and header generation are defined in:
#   cmake/hf_as5047u_build_settings.cmake (single source of truth)
#===============================================================================
set_target_properties(${HF_AS5047U_TARGET_NAME} PROPERTIES VERSION ${HF_AS5047U_VERSION} SOVERSION ${HF_AS5047U_VERSION_MAJOR})

#===============================================================================
# Optional: Enable extra warnings (can be toggled by user)
#===============================================================================
option(HF_AS5047U_ENABLE_WARNINGS "Enable extra warnings for AS5047U driver" OFF)
if(HF_AS5047U_ENABLE_WARNINGS)
    if(TARGET ${HF_AS5047U_TARGET_NAME})
        target_compile_options(${HF_AS5047U_TARGET_NAME} INTERFACE -Wall -Wextra -Wpedantic)
    endif()
endif()

#===============================================================================
# Host unit tests (model of the chip; no hardware)
#===============================================================================
option(HF_AS5047U_BUILD_TESTS "Build the host unit tests in tests/" OFF)
if(HF_AS5047U_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#===============================================================================
# Install and export support (for find_package usage)
#===============================================================================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ${HF_AS5047U_TARGET_NAME} EXPORT hf_as5047uTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY inc/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.h.in" EXCLUDE)
install(FILES "${HF_AS5047U_VERSION_HEADER}" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT hf_as5047uTargets
    FILE hf_as5047uTargets.cmake
    NAMESPACE hf::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_as5047u
)

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/hf_as5047uConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hf_as5047uConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_as5047u
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/hf_as5047uConfigVersion.cmake"
    VERSION ${HF_AS5047U_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/hf_as5047uConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hf_as5047uConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_as5047u
)
