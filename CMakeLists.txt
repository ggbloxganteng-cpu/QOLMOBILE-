cmake_minimum_required(VERSION 3.21)
project(MobileQOL VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if ("${GEODE_TARGET_PLATFORM}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    add_definitions(-DGLES_SILENCE_DEPRECATION)
    add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

file(GLOB SOURCES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK. Set GEODE_SDK to your local Geode SDK path.")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
target_include_directories(${PROJECT_NAME} PRIVATE include src)
setup_geode_mod(${PROJECT_NAME})
