#pragma once

#include "bernstein/core/session.hpp"

#include <ftxui/component/event.hpp>
#include <optional>
#include <string>

namespace bernstein::tui {

struct ComposerState {
    std::string draft;
};

struct EventTranslation {
    ComposerState composer;
    std::optional<core::UserIntent> intent;
    bool handled = false;
};

[[nodiscard]] EventTranslation translate_event(const ftxui::Event& event, ComposerState state);

} // namespace bernstein::tui
