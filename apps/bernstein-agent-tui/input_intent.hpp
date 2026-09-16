#pragma once

#include "bernstein/core/session.hpp"

#include <ftxui/component/event.hpp>
#include <optional>
#include <string>
#include <vector>

namespace bernstein::tui {

struct ComposerState {
    std::string draft;
};

struct EventTranslation {
    ComposerState composer;
    std::optional<core::UserIntent> intent;
    bool handled = false;
};

// The operation that Ctrl+X cancels: the oldest Pending or Running operation.
[[nodiscard]] std::optional<core::OperationId>
cancellation_target(const std::vector<core::OperationRecord>& operations);

// The operation that Ctrl+R reconciles: the most recent operation with an Unknown outcome.
[[nodiscard]] std::optional<core::OperationId>
reconciliation_target(const std::vector<core::OperationRecord>& operations);

// Translates a terminal event into composer state and at most one typed intent. The operations
// are those of the snapshot the user saw; they only resolve targets. Translation has no effects.
[[nodiscard]] EventTranslation
translate_event(const ftxui::Event& event, ComposerState state,
                const std::vector<core::OperationRecord>& operations);

} // namespace bernstein::tui
