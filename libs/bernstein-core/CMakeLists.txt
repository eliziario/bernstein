add_library(bernstein_core STATIC src/session.cpp)
add_library(bernstein::core ALIAS bernstein_core)

target_include_directories(
  bernstein_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

target_compile_features(bernstein_core PUBLIC cxx_std_23)
target_link_libraries(bernstein_core PUBLIC fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)

bernstein_target_warnings(bernstein_core)
