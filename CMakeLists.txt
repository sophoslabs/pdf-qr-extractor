cmake_minimum_required(VERSION 3.16)

project(pdf_qr_extractor
    VERSION 1.0.0
    LANGUAGES CXX
)

set(API_VERSION "1.0.0")
set(BUILD_VERSION "1.0.0")

# --------------------------------------------------
# Global settings
# --------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# --------------------------------------------------
# RPATH (portable / relocatable)
# --------------------------------------------------

set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

# 🔥 ONLY relative path — no absolute paths
set(CMAKE_BUILD_RPATH "$ORIGIN/../lib")
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

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

# --------------------------------------------------
# Poppler (vendored)
# --------------------------------------------------

set(POPPLER_ROOT "${PROJECT_SOURCE_DIR}/third_party/poppler")

set(POPPLER_INCLUDE_DIRS
    "${POPPLER_ROOT}/include/poppler"
    "${POPPLER_ROOT}/include/poppler/cpp"
)

set(POPPLER_CPP_LIB "${POPPLER_ROOT}/lib/libpoppler-cpp.so")
set(POPPLER_LIB     "${POPPLER_ROOT}/lib/libpoppler.so")

if(NOT EXISTS "${POPPLER_CPP_LIB}")
    message(FATAL_ERROR "Poppler not found in third_party/poppler")
endif()

# --------------------------------------------------
# ZXing (vendored)
# --------------------------------------------------

set(ZXING_ROOT "${PROJECT_SOURCE_DIR}/third_party/zxing")

set(ZXING_INCLUDE_DIRS
    "${ZXING_ROOT}/include"
)

set(ZXING_LIB "${ZXING_ROOT}/lib/libZXing.a")

if(NOT EXISTS "${ZXING_LIB}")
    message(FATAL_ERROR "ZXing not found in third_party/zxing")
endif()

# --------------------------------------------------
# Source files
# --------------------------------------------------

file(GLOB_RECURSE EXTRACTOR_SOURCES
    CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

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
# Include directories
# --------------------------------------------------

target_include_directories(pdf_qr_extractor
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/third_party/stb
        ${PROJECT_SOURCE_DIR}/third_party/asio/include
        ${POPPLER_INCLUDE_DIRS}
        ${ZXING_INCLUDE_DIRS}
)

# --------------------------------------------------
# Linking libraries
# --------------------------------------------------

target_link_libraries(pdf_qr_extractor
    PRIVATE
        Threads::Threads

        # Poppler (vendored)
        ${POPPLER_CPP_LIB}
        ${POPPLER_LIB}

        # ZXing (vendored, static)
        ${ZXING_LIB}

        # Required for static libs
        pthread
)

# --------------------------------------------------
# IMPORTANT: NO absolute rpath / rpath-link here
# --------------------------------------------------
# ❌ DO NOT add:
# -Wl,-rpath,<absolute path>
# -Wl,-rpath-link,<absolute path>

# --------------------------------------------------
# Install
# --------------------------------------------------

install(TARGETS pdf_qr_extractor
    RUNTIME DESTINATION bin
)

# --------------------------------------------------
# Debug output
# --------------------------------------------------

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Poppler libs: ${POPPLER_CPP_LIB} ${POPPLER_LIB}")
message(STATUS "ZXing lib: ${ZXING_LIB}")
