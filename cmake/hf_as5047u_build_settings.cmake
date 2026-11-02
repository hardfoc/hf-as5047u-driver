#===============================================================================
# AS5047U Driver - Build Settings
# Shared variables for target name, includes, sources, and dependencies.
# This file is the SINGLE SOURCE OF TRUTH for the driver version.
#===============================================================================

include_guard(GLOBAL)

# Target name
set(HF_AS5047U_TARGET_NAME "hf_as5047u")

#===============================================================================
# Versioning (single source of truth)
#===============================================================================
set(HF_AS5047U_VERSION_MAJOR 1)
set(HF_AS5047U_VERSION_MINOR 0)
set(HF_AS5047U_VERSION_PATCH 0)
set(HF_AS5047U_VERSION "${HF_AS5047U_VERSION_MAJOR}.${HF_AS5047U_VERSION_MINOR}.${HF_AS5047U_VERSION_PATCH}")

#===============================================================================
# Generate version header from template (into build directory)
#===============================================================================
# The generated header is placed in CMAKE_CURRENT_BINARY_DIR to keep the
# source tree clean.  This eliminates the need for a .gitignore entry and
# allows parallel builds with different version stamps.
set(HF_AS5047U_VERSION_TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/../inc/as5047u_version.h.in")
set(HF_AS5047U_VERSION_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/hf_as5047u_generated")
set(HF_AS5047U_VERSION_HEADER     "${HF_AS5047U_VERSION_HEADER_DIR}/as5047u_version.h")

# Ensure the output directory exists before configure_file and before any
# consumer (e.g. ESP-IDF idf_component_register) validates include paths.
file(MAKE_DIRECTORY "${HF_AS5047U_VERSION_HEADER_DIR}")

if(EXISTS "${HF_AS5047U_VERSION_TEMPLATE}")
    configure_file(
        "${HF_AS5047U_VERSION_TEMPLATE}"
        "${HF_AS5047U_VERSION_HEADER}"
        @ONLY
    )
    message(STATUS "AS5047U driver v${HF_AS5047U_VERSION} — generated as5047u_version.h in ${HF_AS5047U_VERSION_HEADER_DIR}")
else()
    message(WARNING "as5047u_version.h.in not found at ${HF_AS5047U_VERSION_TEMPLATE}")
endif()

#===============================================================================
# Public include directories
#===============================================================================
# Three include directories (an installed copy has all three files in one include directory):
#   1. Source tree inc/ — hand-written headers (as5047u.hpp, etc.)
#   2. Source tree src/ — template implementation (as5047u.ipp, included by as5047u.hpp)
#   3. Build tree generated dir — configure_file outputs (as5047u_version.h)
set(HF_AS5047U_PUBLIC_INCLUDE_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../inc"
    "${CMAKE_CURRENT_LIST_DIR}/../src"
    "${HF_AS5047U_VERSION_HEADER_DIR}"
)

#===============================================================================
# Source files (empty for header-only)
#===============================================================================
set(HF_AS5047U_SOURCE_FILES)

#===============================================================================
# ESP-IDF component dependencies
#===============================================================================
set(HF_AS5047U_IDF_REQUIRES driver freertos)
