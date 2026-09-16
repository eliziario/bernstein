#include "projection_view.hpp"

#include <string>

namespace bernstein::tui {
namespace {

ftxui::Element render_header(const core::UiProjection& projection) {
    using namespace ftxui;
    return hbox({text("●") | color(Color::Green), text(" Bernstein") | bold, filler(),
                 text(projection.workspace_name + " · " + projection.branch) | dim});
}

ftxui::Element render_history(const core::UiProjection& projection) {
    using namespace ftxui;

    Elements entries;
    for (const auto& entry : projection.history) {
        entries.push_back(vbox({text("> " + entry.text), text("  model response pending") | dim}));
    }
    if (entries.empty()) {
        entries.push_back(text("No model-visible history yet.") | dim);
    }
    return vbox(std::move(entries));
}

ftxui::Element render_summary(const core::UiProjection& projection) {
    using namespace ftxui;

    const auto operation_count = std::to_string(projection.operations.size());
    const auto observation_count = std::to_string(projection.live_observations.size());
    return text("Session: " + projection.session.value + " · " + operation_count +
                " operations · " + observation_count + " live observations") |
           dim;
}

ftxui::Element render_composer() {
    using namespace ftxui;
    return hbox({text("> ") | bold, text("Describe what you want Bernstein to do...") | dim}) |
           border;
}

ftxui::Element render_footer() {
    using namespace ftxui;
    return hbox({text("Enter send") | dim, filler(), text("? shortcuts") | dim, text("  ") | dim,
                 text("Ctrl+C quit") | dim});
}

} // namespace

ftxui::Element render_projection(const core::UiProjection& projection) {
    using namespace ftxui;
    return vbox({render_header(projection), separator(), render_history(projection),
                 render_summary(projection), separator(), render_composer(), render_footer()}) |
           border;
}

} // namespace bernstein::tui
