#pragma once

#include "bernstein/core/session.hpp"

#include <ftxui/dom/elements.hpp>
#include <string_view>

namespace bernstein::tui {

[[nodiscard]] ftxui::Element render_projection(const core::UiProjection& projection,
                                               std::string_view draft = {});

} // namespace bernstein::tui
