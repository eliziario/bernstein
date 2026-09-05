add_library(bernstein_core INTERFACE)
add_library(bernstein::core ALIAS bernstein_core)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

target_compile_features(bernstein_core INTERFACE cxx_std_23)
target_link_libraries(bernstein_core INTERFACE fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)

bernstein_target_warnings(bernstein_core)
