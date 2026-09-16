#include "bernstein/core/session.hpp"
#include "headless.hpp"
#include "input_intent.hpp"
#include "projection_view.hpp"

#include <catch2/catch_test_macros.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

using namespace bernstein::core;
using namespace bernstein::tui;

TEST_CASE("composer events translate deterministically", "[tui][intent]") {
    const auto typed = translate_event(ftxui::Event::Character("hello"), ComposerState{});
    REQUIRE(typed.handled);
    REQUIRE(typed.composer.draft == "hello");
    REQUIRE_FALSE(typed.intent.has_value());

    const auto submitted = translate_event(ftxui::Event::Return, typed.composer);
    REQUIRE(submitted.handled);
    REQUIRE(submitted.composer.draft.empty());
    REQUIRE(submitted.intent.has_value());
    REQUIRE(std::get<SubmitInput>(*submitted.intent).text == "hello");

    const auto quit = translate_event(ftxui::Event::CtrlC, ComposerState{});
    REQUIRE(quit.handled);
    REQUIRE(std::holds_alternative<Quit>(*quit.intent));
}

TEST_CASE("projection rendering is stable and read-only", "[tui][projection]") {
    FakeSession session;
    const auto before = session.projection();
    auto document = render_projection(before, "review the boundary");
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80), ftxui::Dimension::Fixed(20));

    ftxui::Render(screen, document);
    const auto output = screen.ToString();

    REQUIRE(output.find("Bernstein") != std::string::npos);
    REQUIRE(output.find("Bernstein workspace") != std::string::npos);
    REQUIRE(output.find("review the boundary") != std::string::npos);
    REQUIRE(output.find("No model-visible history yet.") != std::string::npos);
    REQUIRE(session.projection().durable_events.empty());
    REQUIRE(session.projection().inputs.empty());
}

TEST_CASE("headless rendering preserves projection output", "[tui][headless]") {
    FakeSession session;
    const auto projection = session.projection();
    const auto headless = render_headless(projection, "review", 80, 20);

    auto document = render_projection(projection, "review");
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80), ftxui::Dimension::Fixed(20));
    ftxui::Render(screen, document);

    REQUIRE(headless == screen.ToString());
}
