#include "input_intent.hpp"

#include <algorithm>
#include <utility>

namespace bernstein::tui {
namespace {

bool is_utf8_continuation(char byte) {
    return (static_cast<unsigned char>(byte) & 0xC0U) == 0x80U;
}

void erase_last_code_point(std::string& text) {
    while (!text.empty() && is_utf8_continuation(text.back())) {
        text.pop_back();
    }
    if (!text.empty()) {
        text.pop_back();
    }
}

} // namespace

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

EventTranslation translate_event(const ftxui::Event& event, ComposerState state,
                                 const std::vector<core::OperationRecord>& operations) {
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

    if (event == ftxui::Event::Escape || event == ftxui::Event::CtrlU) {
        state.draft.clear();
        return {std::move(state), std::nullopt, true};
    }

    if (event == ftxui::Event::Backspace) {
        erase_last_code_point(state.draft);
        return {std::move(state), std::nullopt, true};
    }

    if (event == ftxui::Event::CtrlX) {
        auto target = cancellation_target(operations);
        if (!target.has_value()) {
            return {std::move(state), std::nullopt, true};
        }
        return {std::move(state), core::CancelOperation{std::move(*target)}, true};
    }

    if (event == ftxui::Event::CtrlR) {
        auto target = reconciliation_target(operations);
        if (!target.has_value()) {
            return {std::move(state), std::nullopt, true};
        }
        return {std::move(state), core::ReconcileOperation{std::move(*target)}, true};
    }

    if (event.is_character()) {
        state.draft += event.character();
        return {std::move(state), std::nullopt, true};
    }

    return {std::move(state), std::nullopt, false};
}

} // namespace bernstein::tui
