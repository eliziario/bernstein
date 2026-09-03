add_library(bernstein_protocol INTERFACE)
add_library(bernstein::protocol ALIAS bernstein_protocol)

find_package(CLI11 CONFIG REQUIRED)

target_compile_features(bernstein_protocol INTERFACE cxx_std_23)
target_link_libraries(bernstein_protocol INTERFACE CLI11::CLI11)

bernstein_target_warnings(bernstein_protocol)
