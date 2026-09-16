#pragma once

#include "bernstein/core/session.hpp"

#include <string>
#include <string_view>

namespace bernstein::tui {

[[nodiscard]] std::string render_headless(const core::UiProjection& projection,
                                          std::string_view draft = {}, int width = 80,
                                          int height = 20);

} // namespace bernstein::tui
