#include "bernstein/core/session.hpp"
#include "bernstein/input/intent.hpp"
#include "headless.hpp"
#include "projection_view.hpp"
#include "terminal_keys.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/screen/terminal.hpp>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string_view>
#include <thread>
#include <unistd.h>
#include <utility>

namespace {

constexpr auto simulation_step = std::chrono::milliseconds(700);

int run_headless_mode() {
    bernstein::core::FakeSession session;
    if (isatty(STDIN_FILENO) != 0) {
        std::istringstream empty;
        std::cout << bernstein::tui::run_headless(session, empty);
    } else {
        std::cout << bernstein::tui::run_headless(session, std::cin);
    }
    return EXIT_SUCCESS;
}

int run_terminal() {
    using namespace ftxui;

    bernstein::core::FakeSession session;
    bernstein::input::ComposerState composer;
    auto app = App::Fullscreen();
    app.ForceHandleCtrlC(false);

    // One snapshot per frame. The renderer takes it after the previous event was handled, and the
    // next event translates against the same pointer, so keys act on the state that was shown.
    auto displayed = session.projection();
    auto renderer = Renderer([&] {
        displayed = session.projection();
        const auto terminal = Terminal::Size();
        return bernstein::tui::render_projection(*displayed, composer.draft,
                                                 {terminal.dimx, terminal.dimy});
    });
    auto component = CatchEvent(renderer, [&](const Event& event) {
        if (event == Event::Custom) {
            (void)session.simulate_next();
            return true;
        }
        auto translation = bernstein::input::translate_event(
            bernstein::tui::from_terminal(event), std::move(composer), displayed->operations);
        composer = std::move(translation.composer);
        if (translation.intent.has_value()) {
            if (std::holds_alternative<bernstein::core::Quit>(*translation.intent)) {
                app.Exit();
            } else {
                session.dispatch_control(std::move(*translation.intent));
            }
        }
        return translation.handled;
    });

    // The session is only touched on the UI loop; the ticker thread only posts events to it.
    std::mutex ticker_mutex;
    std::condition_variable ticker_wake;
    bool ticker_stop = false;
    std::thread ticker([&] {
        std::unique_lock lock(ticker_mutex);
        while (!ticker_wake.wait_for(lock, simulation_step, [&] { return ticker_stop; })) {
            app.PostEvent(Event::Custom);
        }
    });

    app.Loop(component);

    {
        const std::scoped_lock lock(ticker_mutex);
        ticker_stop = true;
    }
    ticker_wake.notify_one();
    ticker.join();
    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 1) {
        return run_terminal();
    }
    if (argc == 2 && std::string_view(argv[1]) == "--headless") {
        return run_headless_mode();
    }
    std::cerr << "usage: bernstein-agent-tui [--headless]\n"
                 "  --headless  read input lines from stdin and print a plain-text snapshot\n";
    return EXIT_FAILURE;
}
