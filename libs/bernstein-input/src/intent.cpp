#include "bernstein/input/intent.hpp"

#include <algorithm>
#include <utility>

namespace bernstein::input {

std::optional<core::OperationId>
cancellation_target(const std::vector<core::OperationRecord>& operations) {
    const auto operation =
        std::find_if(operations.begin(), operations.end(),
                     [](const auto& candidate) { return core::is_active(candidate.status); });
    if (operation == operations.end()) {
        return std::nullopt;
    }
    return operation->id;
}

std::optional<core::OperationId>
reconciliation_target(const std::vector<core::OperationRecord>& operations) {
    const auto operation =
        std::find_if(operations.rbegin(), operations.rend(), [](const auto& candidate) {
            return candidate.status == core::OperationStatus::Unknown;
        });
    if (operation == operations.rend()) {
        return std::nullopt;
    }
    return operation->id;
}

EventTranslation translate_event(const KeyEvent& event, ComposerState state,
                                 const std::vector<core::OperationRecord>& operations) {
    switch (event.key) {
    case Key::Quit:
        return {std::move(state), core::Quit{}, true};

    case Key::Submit: {
        if (state.draft.empty()) {
            return {std::move(state), std::nullopt, true};
        }
        auto text = std::move(state.draft);
        state.draft.clear();
        return {std::move(state), core::SubmitInput{std::move(text)}, true};
    }

    case Key::ClearDraft:
        state.draft.clear();
        return {std::move(state), std::nullopt, true};

    case Key::DeleteBefore:
        erase_last_code_point(state.draft);
        return {std::move(state), std::nullopt, true};

    case Key::CancelOperation: {
        auto target = cancellation_target(operations);
        if (!target.has_value()) {
            return {std::move(state), std::nullopt, true};
        }
        return {std::move(state), core::CancelOperation{std::move(*target)}, true};
    }

    case Key::ReconcileOperation: {
        auto target = reconciliation_target(operations);
        if (!target.has_value()) {
            return {std::move(state), std::nullopt, true};
        }
        return {std::move(state), core::ReconcileOperation{std::move(*target)}, true};
    }

    case Key::Character:
        state.draft += event.text;
        return {std::move(state), std::nullopt, true};

    case Key::Unknown:
        break;
    }
    return {std::move(state), std::nullopt, false};
}

} // namespace bernstein::input
