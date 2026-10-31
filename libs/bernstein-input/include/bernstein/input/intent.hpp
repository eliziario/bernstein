#pragma once

#include "bernstein/core/session.hpp"
#include "bernstein/input/key_event.hpp"

#include <optional>
#include <string>
#include <vector>

namespace bernstein::input {

struct ComposerState {
    std::string draft;
};

struct EventTranslation {
    ComposerState composer;
    std::optional<core::UserIntent> intent;
    bool handled = false;
};

// The operation that a cancel key refers to: the oldest one still pending or running.
[[nodiscard]] std::optional<core::OperationId>
cancellation_target(const std::vector<core::OperationRecord>& operations);

// The operation that a reconcile key refers to: the most recent one of unknown outcome.
[[nodiscard]] std::optional<core::OperationId>
reconciliation_target(const std::vector<core::OperationRecord>& operations);

// Turns a keystroke into composer state and at most one typed intent. The operations are those
// of the snapshot the person was looking at, and are read only to resolve a target.
[[nodiscard]] EventTranslation
translate_event(const KeyEvent& event, ComposerState state,
                const std::vector<core::OperationRecord>& operations);

} // namespace bernstein::input
