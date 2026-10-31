#include "terminal_keys.hpp"

namespace bernstein::tui {

input::KeyEvent from_terminal(const ftxui::Event& event) {
    using input::Key;
    using input::KeyEvent;

    if (event == ftxui::Event::CtrlC) {
        return KeyEvent::of(Key::Quit);
    }
    if (event == ftxui::Event::Return) {
        return KeyEvent::of(Key::Submit);
    }
    if (event == ftxui::Event::Escape || event == ftxui::Event::CtrlU) {
        return KeyEvent::of(Key::ClearDraft);
    }
    if (event == ftxui::Event::Backspace) {
        return KeyEvent::of(Key::DeleteBefore);
    }
    if (event == ftxui::Event::CtrlX) {
        return KeyEvent::of(Key::CancelOperation);
    }
    if (event == ftxui::Event::CtrlR) {
        return KeyEvent::of(Key::ReconcileOperation);
    }
    if (event.is_character()) {
        return KeyEvent::character(event.character());
    }
    return KeyEvent::of(Key::Unknown);
}

} // namespace bernstein::tui
