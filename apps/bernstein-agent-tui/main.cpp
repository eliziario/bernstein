#include "bernstein/core/session.hpp"
#include "input_intent.hpp"
#include "projection_view.hpp"

#include <cstdlib>
#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <utility>

int main() {
    using namespace ftxui;

    bernstein::core::FakeSession session;
    bernstein::tui::ComposerState composer;
    auto app = App::Fullscreen();
    app.ForceHandleCtrlC(false);

    auto renderer = Renderer(
        [&] { return bernstein::tui::render_projection(session.projection(), composer.draft); });
    auto component = CatchEvent(renderer, [&](const Event& event) {
        const auto translation = bernstein::tui::translate_event(event, std::move(composer));
        composer = translation.composer;
        if (translation.intent.has_value()) {
            if (std::holds_alternative<bernstein::core::Quit>(*translation.intent)) {
                app.Exit();
            } else {
                session.dispatch_control(*translation.intent);
            }
        }
        return translation.handled;
    });

    app.Loop(component);

    return EXIT_SUCCESS;
}
