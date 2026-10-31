add_library(bernstein_input STATIC src/intent.cpp src/key_event.cpp)
add_library(bernstein::input ALIAS bernstein_input)

target_include_directories(
  bernstein_input PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(bernstein_input PUBLIC cxx_std_23)
target_link_libraries(bernstein_input PUBLIC bernstein::core)

bernstein_target_warnings(bernstein_input)
