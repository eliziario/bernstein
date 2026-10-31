#pragma once

#include "bernstein/input/key_event.hpp"

#include <ftxui/component/event.hpp>

namespace bernstein::tui {

// The only place that knows what an FTXUI event is. Everything downstream works in terms of
// bernstein::input::KeyEvent, which is why intent translation links no terminal library.
[[nodiscard]] input::KeyEvent from_terminal(const ftxui::Event& event);

} // namespace bernstein::tui
