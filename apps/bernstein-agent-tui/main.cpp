#include "bernstein/core/session.hpp"
#include "headless.hpp"
#include "input_intent.hpp"
#include "projection_view.hpp"

#include <cstdlib>
#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <iostream>
#include <string_view>
#include <utility>

int main(int argc, char** argv) {
    using namespace ftxui;

    bernstein::core::FakeSession session;
    if (argc == 2 && std::string_view(argv[1]) == "--headless") {
        std::cout << bernstein::tui::render_headless(session.projection());
        return EXIT_SUCCESS;
    }

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
