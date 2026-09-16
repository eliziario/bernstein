find_package(Catch2 3 CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)

include(Catch)

add_executable(bernstein_core_tests bernstein-core/session_tests.cpp)
target_link_libraries(bernstein_core_tests PRIVATE bernstein::core Catch2::Catch2WithMain)

bernstein_target_warnings(bernstein_core_tests)

catch_discover_tests(bernstein_core_tests)

add_executable(
  bernstein_tui_tests
  bernstein-agent-tui/tui_tests.cpp
  ${CMAKE_SOURCE_DIR}/apps/bernstein-agent-tui/input_intent.cpp
  ${CMAKE_SOURCE_DIR}/apps/bernstein-agent-tui/projection_view.cpp
)
target_include_directories(bernstein_tui_tests PRIVATE ${CMAKE_SOURCE_DIR}/apps/bernstein-agent-tui)
target_link_libraries(bernstein_tui_tests PRIVATE bernstein::core ftxui::ftxui Catch2::Catch2WithMain)

bernstein_target_warnings(bernstein_tui_tests)

catch_discover_tests(bernstein_tui_tests)
