find_package(Catch2 3 CONFIG REQUIRED)

include(Catch)

add_executable(bernstein_core_tests bernstein-core/session_tests.cpp)
target_link_libraries(bernstein_core_tests PRIVATE bernstein::core Catch2::Catch2WithMain)

bernstein_target_warnings(bernstein_core_tests)

catch_discover_tests(bernstein_core_tests)

# Links no terminal library: the input path must stay independent of one.
add_executable(bernstein_input_tests bernstein-input/input_tests.cpp)
target_link_libraries(bernstein_input_tests PRIVATE bernstein::input Catch2::Catch2WithMain)

bernstein_target_warnings(bernstein_input_tests)

catch_discover_tests(bernstein_input_tests)

if(BERNSTEIN_BUILD_TUI)
  add_executable(bernstein_tui_tests bernstein-agent-tui/tui_tests.cpp)
  target_link_libraries(bernstein_tui_tests PRIVATE bernstein::tui Catch2::Catch2WithMain)

  bernstein_target_warnings(bernstein_tui_tests)

  catch_discover_tests(bernstein_tui_tests)
endif()
