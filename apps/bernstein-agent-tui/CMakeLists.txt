find_package(ftxui CONFIG REQUIRED)

add_executable(
  bernstein-agent-tui
  headless.cpp
  input_intent.cpp
  main.cpp
  projection_view.cpp
)
target_link_libraries(bernstein-agent-tui PRIVATE bernstein::core ftxui::ftxui)

bernstein_enable_static_analysis(bernstein-agent-tui)
