cmake_minimum_required(VERSION 3.28)

project(
  bernstein
  VERSION 0.1.0
  DESCRIPTION "Architecture-aware coding agent"
  LANGUAGES CXX
)

include(CTest)

option(BERNSTEIN_ENABLE_CLANG_TIDY "Run clang-tidy during CMake builds" OFF)
option(BERNSTEIN_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(BERNSTEIN_BUILD_TUI "Build the FTXUI terminal user interface" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BernsteinCompilerWarnings)
include(BernsteinStaticAnalysis)

add_subdirectory(libs)
add_subdirectory(apps)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
