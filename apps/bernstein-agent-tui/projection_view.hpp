#pragma once

#include "bernstein/core/session.hpp"

#include <ftxui/dom/elements.hpp>

namespace bernstein::tui {

[[nodiscard]] ftxui::Element render_projection(const core::UiProjection& projection);

} // namespace bernstein::tui
