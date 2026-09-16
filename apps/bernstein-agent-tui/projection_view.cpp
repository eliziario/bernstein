#include "projection_view.hpp"

#include <algorithm>
#include <cstddef>
#include <string>

namespace bernstein::tui {
namespace {

using core::OperationStatus;

constexpr std::size_t durable_activity_lines = 2;
constexpr std::size_t live_activity_lines = 1;

std::string label(std::string_view value) {
    return std::string(value);
}

ftxui::Decorator operation_style(OperationStatus status) {
    using namespace ftxui;
    switch (status) {
    case OperationStatus::Pending:
        return dim;
    case OperationStatus::Running:
        return color(Color::Yellow);
    case OperationStatus::Interrupted:
        return color(Color::Yellow);
    case OperationStatus::Unknown:
        return color(Color::Red) | bold;
    case OperationStatus::Settled:
        return color(Color::Cyan);
    }
    return nothing;
}

std::string operation_label(const core::OperationRecord& operation) {
    if (operation.status == OperationStatus::Unknown) {
        return "operation outcome unknown · effect " + label(core::to_string(operation.effect));
    }
    return "operation " + label(core::to_string(operation.status)) + " · effect " +
           label(core::to_string(operation.effect));
}

ftxui::Element render_header(const core::UiProjection& projection) {
    using namespace ftxui;
    return hbox({text("●") | color(Color::Green), text(" Bernstein") | bold, filler(),
                 text(projection.session.value + " · " + projection.workspace_name + " · " +
                      projection.branch) |
                     dim});
}

ftxui::Element render_input(const core::UiProjection& projection, const core::InputRecord& input) {
    using namespace ftxui;

    const auto visibility = input.status == core::InputStatus::Accepted
                                ? std::string("accepted, not yet model-visible")
                                : label(core::to_string(input.status));
    Elements status{text("  input " + visibility) | dim};

    Elements lines{hbox({text("> ") | bold, paragraph(input.text) | flex})};
    const auto operation =
        std::find_if(projection.operations.begin(), projection.operations.end(),
                     [&input](const auto& candidate) { return candidate.id == input.operation; });
    if (operation != projection.operations.end()) {
        status.push_back(text(" · ") | dim);
        status.push_back(text(operation_label(*operation)) | operation_style(operation->status));
        lines.push_back(hbox(std::move(status)));
        lines.push_back(text("  " + operation->detail) | dim);
    } else {
        lines.push_back(hbox(std::move(status)));
    }
    return vbox(std::move(lines));
}

ftxui::Element render_conversation(const core::UiProjection& projection) {
    using namespace ftxui;

    Elements entries;
    for (const auto& input : projection.inputs) {
        entries.push_back(render_input(projection, input));
    }
    if (entries.empty()) {
        entries.push_back(text("No input yet. Describe what you want Bernstein to do.") | dim);
    }
    // Keep the newest entry visible when the history is taller than the region.
    return vbox(std::move(entries)) | focusPositionRelative(0.0F, 1.0F) | frame |
           vscroll_indicator | flex;
}

ftxui::Element render_activity(const core::UiProjection& projection) {
    using namespace ftxui;

    Elements lines{text("Activity") | bold | dim};
    const auto& events = projection.durable_tail;
    const auto first_event = events.size() - std::min(events.size(), durable_activity_lines);
    for (auto index = first_event; index < events.size(); ++index) {
        const auto& event = events[index];
        lines.push_back(hbox({text("durable #" + std::to_string(event.cursor.sequence) + " ") | dim,
                              text(label(core::to_string(event.kind)) + ": " + event.detail)}));
    }

    const auto& live = projection.live_observations;
    const auto first_live = live.size() - std::min(live.size(), live_activity_lines);
    for (auto index = first_live; index < live.size(); ++index) {
        const auto& observation = live[index];
        lines.push_back(hbox({text("live ") | color(Color::Magenta),
                              text(observation.kind + ": " + observation.detail) | dim}));
    }

    while (lines.size() < 1 + durable_activity_lines + live_activity_lines) {
        lines.push_back(text(""));
    }
    return vbox(std::move(lines));
}

ftxui::Element render_context(const core::UiProjection& projection) {
    using namespace ftxui;

    // Status precedes the title so that clipping on narrow terminals removes the title first.
    std::string context;
    if (!projection.proposals.empty()) {
        const auto& proposal = projection.proposals.front();
        context += "Proposal " + label(core::to_string(proposal.status)) + ": " + proposal.title;
    }
    if (!projection.candidates.empty()) {
        const auto& candidate = projection.candidates.front();
        if (!context.empty()) {
            context += " · ";
        }
        context += "Candidate " + label(core::to_string(candidate.verification)) + ": " +
                   candidate.artifact;
    }
    return text(context) | dim;
}

ftxui::Element render_composer(std::string_view draft) {
    using namespace ftxui;
    const auto content =
        draft.empty() ? text("Describe what you want Bernstein to do...") | dim : paragraph(draft);
    return hbox({text("> ") | bold, content | flex}) | border;
}

ftxui::Element render_footer() {
    using namespace ftxui;
    return hbox({text("Enter send · Esc clear") | dim, filler(),
                 text("Ctrl+X cancel · Ctrl+R reconcile · Ctrl+C quit") | dim});
}

ftxui::Element render_too_small(ViewSize size) {
    using namespace ftxui;
    return vbox(
        {text("Terminal too small") | bold,
         text("Current size: " + std::to_string(size.width) + "x" + std::to_string(size.height)),
         text("Required: " + std::to_string(minimum_view_size.width) + "x" +
              std::to_string(minimum_view_size.height)),
         text("Enlarge the terminal, or press Ctrl+C to quit.") | dim});
}

} // namespace

bool fits(ViewSize size) {
    return size.width >= minimum_view_size.width && size.height >= minimum_view_size.height;
}

ftxui::Element render_projection(const core::UiProjection& projection, std::string_view draft,
                                 ViewSize size) {
    using namespace ftxui;
    if (!fits(size)) {
        return render_too_small(size);
    }
    return vbox({render_header(projection), separator(), render_conversation(projection),
                 separator(), render_activity(projection), render_context(projection), separator(),
                 render_composer(draft), render_footer()}) |
           border | flex;
}

} // namespace bernstein::tui
