find_package(ftxui CONFIG REQUIRED)

add_library(bernstein_tui STATIC headless.cpp input_intent.cpp projection_view.cpp)
add_library(bernstein::tui ALIAS bernstein_tui)
target_include_directories(bernstein_tui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bernstein_tui PUBLIC bernstein::core ftxui::ftxui)

bernstein_target_warnings(bernstein_tui)

add_executable(bernstein-agent-tui main.cpp)
target_link_libraries(bernstein-agent-tui PRIVATE bernstein::tui)

bernstein_target_warnings(bernstein-agent-tui)
bernstein_enable_static_analysis(bernstein-agent-tui)
