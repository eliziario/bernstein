#include "bernstein/core/session.hpp"
#include "headless.hpp"
#include "input_intent.hpp"
#include "projection_view.hpp"

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

TEST_CASE("composer events translate deterministically", "[tui][intent]") {
    const std::vector<OperationRecord> operations;
    const auto typed =
        translate_event(ftxui::Event::Character("hello"), ComposerState{}, operations);
    REQUIRE(typed.handled);
    REQUIRE(typed.composer.draft == "hello");
    REQUIRE_FALSE(typed.intent.has_value());

    const auto submitted = translate_event(ftxui::Event::Return, typed.composer, operations);
    REQUIRE(submitted.handled);
    REQUIRE(submitted.composer.draft.empty());
    REQUIRE(submitted.intent.has_value());
    REQUIRE(std::get<SubmitInput>(*submitted.intent).text == "hello");

    const auto quit = translate_event(ftxui::Event::CtrlC, ComposerState{}, operations);
    REQUIRE(quit.handled);
    REQUIRE(std::holds_alternative<Quit>(*quit.intent));
}

TEST_CASE("composer editing never produces an intent", "[tui][intent]") {
    const std::vector<OperationRecord> operations;

    const auto empty_submit = translate_event(ftxui::Event::Return, ComposerState{}, operations);
    REQUIRE(empty_submit.handled);
    REQUIRE_FALSE(empty_submit.intent.has_value());

    for (const auto& clear : {ftxui::Event::Escape, ftxui::Event::CtrlU}) {
        const auto cleared = translate_event(clear, ComposerState{"draft"}, operations);
        REQUIRE(cleared.handled);
        REQUIRE(cleared.composer.draft.empty());
        REQUIRE_FALSE(cleared.intent.has_value());
    }

    const auto ascii = translate_event(ftxui::Event::Backspace, ComposerState{"ab"}, operations);
    REQUIRE(ascii.composer.draft == "a");
    REQUIRE_FALSE(ascii.intent.has_value());

    const auto empty = translate_event(ftxui::Event::Backspace, ComposerState{}, operations);
    REQUIRE(empty.handled);
    REQUIRE(empty.composer.draft.empty());

    const auto unhandled = translate_event(ftxui::Event::F5, ComposerState{"keep"}, operations);
    REQUIRE_FALSE(unhandled.handled);
    REQUIRE(unhandled.composer.draft == "keep");
}

TEST_CASE("backspace removes a whole UTF-8 code point", "[tui][intent]") {
    const std::vector<OperationRecord> operations;
    const auto accented =
        translate_event(ftxui::Event::Backspace, ComposerState{"caf\xC3\xA9"}, operations);
    REQUIRE(accented.composer.draft == "caf");

    const auto emoji =
        translate_event(ftxui::Event::Backspace, ComposerState{"a\xF0\x9F\x99\x82"}, operations);
    REQUIRE(emoji.composer.draft == "a");
}

TEST_CASE("cancel and reconcile keys target the lifecycle state", "[tui][intent]") {
    FakeSession session;
    REQUIRE_FALSE(
        translate_event(ftxui::Event::CtrlX, ComposerState{}, session.projection()->operations)
            .intent.has_value());

    const auto receipt = session.admit(SubmitInput{"work"});
    const auto cancel =
        translate_event(ftxui::Event::CtrlX, ComposerState{}, session.projection()->operations);
    REQUIRE(std::get<CancelOperation>(*cancel.intent).id == *receipt.operation);

    REQUIRE_FALSE(
        translate_event(ftxui::Event::CtrlR, ComposerState{}, session.projection()->operations)
            .intent.has_value());

    REQUIRE(session.simulate_next().has_value());
    REQUIRE(session.simulate_next().has_value());
    session.dispatch_control(std::get<CancelOperation>(*cancel.intent));
    const auto reconcile =
        translate_event(ftxui::Event::CtrlR, ComposerState{}, session.projection()->operations);
    REQUIRE(std::get<ReconcileOperation>(*reconcile.intent).id == *receipt.operation);
}

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
