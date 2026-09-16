add_subdirectory(bernstein-agent)

if(BERNSTEIN_BUILD_TUI)
  add_subdirectory(bernstein-agent-tui)
endif()
