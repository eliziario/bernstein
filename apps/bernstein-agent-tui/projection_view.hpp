#pragma once

#include "bernstein/core/session.hpp"

#include <ftxui/dom/elements.hpp>
#include <string_view>

namespace bernstein::tui {

struct ViewSize {
    int width = 80;
    int height = 24;
};

// Below this size the layout would hide lifecycle state, so a diagnostic is rendered instead.
inline constexpr ViewSize minimum_view_size{60, 20};

[[nodiscard]] bool fits(ViewSize size);

// Renders a read-only view of the projection. The renderer never dispatches intents.
[[nodiscard]] ftxui::Element render_projection(const core::UiProjection& projection,
                                               std::string_view draft, ViewSize size);

} // namespace bernstein::tui
