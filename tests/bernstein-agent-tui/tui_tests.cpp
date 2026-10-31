#include "bernstein/core/session.hpp"
#include "bernstein/input/key_event.hpp"
#include "headless.hpp"
#include "projection_view.hpp"
#include "terminal_keys.hpp"

#include <catch2/catch_test_macros.hpp>
#include <ftxui/component/event.hpp>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

using namespace bernstein::core;
using namespace bernstein::tui;

namespace {

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace

TEST_CASE("rendering shows input and operation lifecycle state", "[tui][projection]") {
    FakeSession session;
    const auto receipt = session.admit(SubmitInput{"review the boundary"});

    const auto accepted = render_plain(*session.projection(), "next draft");
    REQUIRE(contains(accepted, "review the boundary"));
    REQUIRE(contains(accepted, "input accepted, not yet model-visible"));
    REQUIRE(contains(accepted, "operation pending"));
    REQUIRE(contains(accepted, "next draft"));
    REQUIRE(contains(accepted, "durable #2"));

    REQUIRE(session.simulate_next().has_value());
    REQUIRE(session.simulate_next().has_value());
    session.dispatch_control(CancelOperation{*receipt.operation});

    const auto unknown = render_plain(*session.projection());
    REQUIRE(contains(unknown, "input promoted"));
    REQUIRE(contains(unknown, "operation outcome unknown"));
    REQUIRE_FALSE(contains(unknown, "settled"));
    REQUIRE(contains(unknown, "live cancel"));
}

TEST_CASE("rendering keeps the newest conversation entry visible", "[tui][projection]") {
    FakeSession session;
    for (int index = 1; index <= 12; ++index) {
        (void)session.admit(SubmitInput{"request " + std::to_string(index)});
    }

    const auto output = render_plain(*session.projection());
    REQUIRE(contains(output, "request 12"));
    REQUIRE_FALSE(contains(output, "> request 1 "));
}

TEST_CASE("small terminals render an explicit diagnostic", "[tui][projection]") {
    FakeSession session;
    REQUIRE(fits(minimum_view_size));
    REQUIRE_FALSE(fits({minimum_view_size.width - 1, minimum_view_size.height}));

    const auto output = render_plain(*session.projection(), {}, {40, 10});
    REQUIRE(contains(output, "Terminal too small"));
    REQUIRE(contains(output, "Current size: 40x10"));
}

TEST_CASE("headless mode drives the session through composer events", "[tui][headless]") {
    FakeSession session;
    std::istringstream input("first request\nsecond request\n");

    const auto output = run_headless(session, input);

    const auto view = session.projection();
    REQUIRE(view->inputs.size() == 2);
    REQUIRE(view->operations[0].status == OperationStatus::Settled);
    REQUIRE(view->operations[1].status == OperationStatus::Settled);
    REQUIRE(contains(output, "second request"));
    REQUIRE(contains(output, "operation settled"));
    REQUIRE_FALSE(contains(output, "\x1b"));
}

TEST_CASE("terminal events map onto Bernstein keys", "[tui][keys]") {
    using bernstein::input::Key;

    REQUIRE(from_terminal(ftxui::Event::CtrlC).key == Key::Quit);
    REQUIRE(from_terminal(ftxui::Event::Return).key == Key::Submit);
    REQUIRE(from_terminal(ftxui::Event::Escape).key == Key::ClearDraft);
    REQUIRE(from_terminal(ftxui::Event::CtrlU).key == Key::ClearDraft);
    REQUIRE(from_terminal(ftxui::Event::Backspace).key == Key::DeleteBefore);
    REQUIRE(from_terminal(ftxui::Event::CtrlX).key == Key::CancelOperation);
    REQUIRE(from_terminal(ftxui::Event::CtrlR).key == Key::ReconcileOperation);
    REQUIRE(from_terminal(ftxui::Event::F5).key == Key::Unknown);

    const auto typed = from_terminal(ftxui::Event::Character("\xC3\xA9"));
    REQUIRE(typed.key == Key::Character);
    REQUIRE(typed.text == "\xC3\xA9");
}
