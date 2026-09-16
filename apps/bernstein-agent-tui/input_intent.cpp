#include "input_intent.hpp"

namespace bernstein::tui {

EventTranslation translate_event(const ftxui::Event& event, ComposerState state) {
    if (event == ftxui::Event::CtrlC) {
        return {std::move(state), core::Quit{}, true};
    }

    if (event == ftxui::Event::Return) {
        if (state.draft.empty()) {
            return {std::move(state), std::nullopt, true};
        }
        auto text = std::move(state.draft);
        state.draft.clear();
        return {std::move(state), core::SubmitInput{std::move(text)}, true};
    }

    if (event == ftxui::Event::Escape) {
        state.draft.clear();
        return {std::move(state), std::nullopt, true};
    }

    if (event == ftxui::Event::Backspace) {
        if (!state.draft.empty()) {
            state.draft.pop_back();
        }
        return {std::move(state), std::nullopt, true};
    }

    if (event == ftxui::Event::CtrlU) {
        state.draft.clear();
        return {std::move(state), std::nullopt, true};
    }

    if (event.is_character()) {
        state.draft += event.character();
        return {std::move(state), std::nullopt, true};
    }

    return {std::move(state), std::nullopt, false};
}

} // namespace bernstein::tui
