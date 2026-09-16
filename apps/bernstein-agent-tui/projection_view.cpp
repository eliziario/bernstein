#include "projection_view.hpp"

#include <string>

namespace bernstein::tui {
namespace {

std::string input_status(core::InputStatus status) {
    switch (status) {
    case core::InputStatus::Accepted:
        return "accepted";
    case core::InputStatus::Promoted:
        return "promoted";
    case core::InputStatus::Rejected:
        return "rejected";
    case core::InputStatus::Conflict:
        return "conflict";
    }
    return "unknown";
}

std::string operation_status(core::OperationStatus status) {
    switch (status) {
    case core::OperationStatus::Pending:
        return "pending";
    case core::OperationStatus::Running:
        return "running";
    case core::OperationStatus::Interrupted:
        return "interrupted";
    case core::OperationStatus::Unknown:
        return "unknown";
    case core::OperationStatus::Settled:
        return "settled";
    }
    return "unknown";
}

ftxui::Element line(std::string label, std::string value) {
    using namespace ftxui;
    return hbox({text(std::move(label) + ": "), filler(), text(std::move(value))});
}

} // namespace

ftxui::Element render_projection(const core::UiProjection& projection) {
    using namespace ftxui;

    Elements inputs;
    for (const auto& input : projection.inputs) {
        inputs.push_back(
            text(input.id.value + "  " + input_status(input.status) + "  " + input.text));
    }
    if (inputs.empty()) {
        inputs.push_back(text("No inputs admitted."));
    }

    Elements operations;
    for (const auto& operation : projection.operations) {
        operations.push_back(text(operation.id.value + "  " + operation_status(operation.status) +
                                  "  " + operation.detail));
    }
    if (operations.empty()) {
        operations.push_back(text("No operations."));
    }

    Elements observations;
    for (const auto& observation : projection.live_observations) {
        observations.push_back(text(observation.kind + ": " + observation.detail));
    }
    if (observations.empty()) {
        observations.push_back(text("No live observations."));
    }

    return vbox({text("Bernstein TUI") | bold, separator(),
                 line("Workspace", projection.workspace_name), line("Branch", projection.branch),
                 separator(), text("Inputs") | bold, vbox(std::move(inputs)), separator(),
                 text("Operations") | bold, vbox(std::move(operations)), separator(),
                 text("Live observations") | bold, vbox(std::move(observations))}) |
           border;
}

} // namespace bernstein::tui
