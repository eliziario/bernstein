find_package(Catch2 3 CONFIG REQUIRED)

include(Catch)

add_executable(bernstein_core_tests bernstein-core/session_tests.cpp)
target_link_libraries(bernstein_core_tests PRIVATE bernstein::core Catch2::Catch2WithMain)

bernstein_target_warnings(bernstein_core_tests)

catch_discover_tests(bernstein_core_tests)
