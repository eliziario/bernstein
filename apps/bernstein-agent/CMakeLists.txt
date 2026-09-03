add_executable(bernstein-agent main.cpp)
target_link_libraries(bernstein-agent PRIVATE bernstein::core bernstein::protocol)

bernstein_enable_static_analysis(bernstein-agent)
