find_package(ftxui CONFIG REQUIRED)

add_executable(bernstein-agent-tui main.cpp)
target_link_libraries(bernstein-agent-tui PRIVATE ftxui::ftxui)

bernstein_enable_static_analysis(bernstein-agent-tui)
