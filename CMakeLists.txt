cmake_minimum_required(VERSION 3.16)

project(pdf_qr_extractor
    VERSION 1.0
    LANGUAGES CXX
)

# --------------------------------------------------
# Version generation (Git-based)
# --------------------------------------------------

# Get latest tag (API version)
execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE API_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT API_VERSION)
    set(API_VERSION "v0.0.0")
endif()

# Get detailed build version
execute_process(
    COMMAND git describe --tags --dirty --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE BUILD_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT BUILD_VERSION)
    set(BUILD_VERSION "${API_VERSION}")
endif()

# Generate version.h from template
configure_file(
    ${CMAKE_SOURCE_DIR}/include/version.h.in
    ${CMAKE_SOURCE_DIR}/include/version.h
    @ONLY
)

# Optional: print version info
message(STATUS "API_VERSION: ${API_VERSION}")
message(STATUS "BUILD_VERSION: ${BUILD_VERSION}")

# --------------------------------------------------
# Global settings
# --------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

#--------------------------------------------------
# RPATH configuration (critical for bundled libs)
#--------------------------------------------------

set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_INSTALL_RPATH FALSE)

# --------------------------------------------------
# Compiler warnings
# --------------------------------------------------

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wno-unused-parameter
    )
endif()

# --------------------------------------------------
# Dependencies
# --------------------------------------------------

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(POPPLER REQUIRED poppler-cpp)
pkg_check_modules(ZXING REQUIRED zxing)

# --------------------------------------------------
# Source files (Auto-discovery for src)
# --------------------------------------------------

file(GLOB_RECURSE EXTRACTOR_SOURCES
    CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

# Add third-party implementation files explicitly
list(APPEND EXTRACTOR_SOURCES
    ${PROJECT_SOURCE_DIR}/third_party/stb/stb_impl.cpp
)


# --------------------------------------------------
# Executable
# --------------------------------------------------

add_executable(pdf_qr_extractor
    ${EXTRACTOR_SOURCES}
)

# --------------------------------------------------
# Include paths
# --------------------------------------------------

target_include_directories(pdf_qr_extractor
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/third_party/stb
        ${POPPLER_INCLUDE_DIRS}
        ${ZXING_INCLUDE_DIRS}
)

# --------------------------------------------------
# Linking libraries
# --------------------------------------------------

target_link_libraries(pdf_qr_extractor
    PRIVATE
        Threads::Threads
        ${POPPLER_LIBRARIES}
        ${ZXING_LIBRARIES}
)

# --------------------------------------------------
# Compiler flags from pkg-config
# --------------------------------------------------

target_compile_options(pdf_qr_extractor PRIVATE
    ${POPPLER_CFLAGS_OTHER}
    ${ZXING_CFLAGS_OTHER}
)

# --------------------------------------------------
# RPATH configuration
# Ensures extractor loads bundled libraries from ../lib
# --------------------------------------------------
set_target_properties(pdf_qr_extractor PROPERTIES
    BUILD_RPATH "$ORIGIN/../lib"
    INSTALL_RPATH "$ORIGIN/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)
# --------------------------------------------------
# Compile definitions
# --------------------------------------------------

target_compile_definitions(pdf_qr_extractor
    PRIVATE
        PROJECT_VERSION="${PROJECT_VERSION}"
        API_VERSION="${API_VERSION}"
        BUILD_VERSION="${BUILD_VERSION}"
)

# --------------------------------------------------
# Install targets
# --------------------------------------------------

install(TARGETS pdf_qr_extractor
    RUNTIME DESTINATION bin
)

install(FILES pdf_qr_extractor.conf
    DESTINATION etc/pdf_qr_extractor
)

# --------------------------------------------------
# Build summary
# --------------------------------------------------

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Poppler include dirs: ${POPPLER_INCLUDE_DIRS}")
message(STATUS "Poppler libraries: ${POPPLER_LIBRARIES}")
message(STATUS "ZXing include dirs: ${ZXING_INCLUDE_DIRS}")
message(STATUS "ZXing libraries: ${ZXING_LIBRARIES}")
