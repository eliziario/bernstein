#pragma once

#include "bernstein/core/session.hpp"
#include "projection_view.hpp"

#include <istream>
#include <string>
#include <string_view>

namespace bernstein::tui {

// Renders the projection to plain text without terminal styling or escape sequences.
[[nodiscard]] std::string render_plain(const core::UiProjection& projection,
                                       std::string_view draft = {}, ViewSize size = {});

// Feeds each input line through the same event translation as the terminal composer, dispatches
// the resulting intents, runs the simulation until no operation is active, and renders plain text.
[[nodiscard]] std::string run_headless(core::FakeSession& session, std::istream& stream,
                                       ViewSize size = {});

} // namespace bernstein::tui
