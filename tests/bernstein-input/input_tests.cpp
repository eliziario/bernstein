#include "bernstein/core/session.hpp"
#include "bernstein/input/intent.hpp"
#include "bernstein/input/key_event.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <variant>
#include <vector>

using namespace bernstein::core;
using namespace bernstein::input;

TEST_CASE("keystrokes translate deterministically", "[input][intent]") {
    const std::vector<OperationRecord> operations;

    const auto typed = translate_event(KeyEvent::character("h"), ComposerState{}, operations);
    REQUIRE(typed.handled);
    REQUIRE(typed.composer.draft == "h");
    REQUIRE_FALSE(typed.intent.has_value());

    const auto submitted =
        translate_event(KeyEvent::of(Key::Submit), ComposerState{"hello"}, operations);
    REQUIRE(submitted.handled);
    REQUIRE(submitted.composer.draft.empty());
    REQUIRE(std::get<SubmitInput>(*submitted.intent).text == "hello");

    const auto quit = translate_event(KeyEvent::of(Key::Quit), ComposerState{}, operations);
    REQUIRE(quit.handled);
    REQUIRE(std::holds_alternative<Quit>(*quit.intent));
}

TEST_CASE("editing a draft never produces an intent", "[input][intent]") {
    const std::vector<OperationRecord> operations;

    const auto empty_submit =
        translate_event(KeyEvent::of(Key::Submit), ComposerState{}, operations);
    REQUIRE(empty_submit.handled);
    REQUIRE_FALSE(empty_submit.intent.has_value());

    const auto cleared =
        translate_event(KeyEvent::of(Key::ClearDraft), ComposerState{"draft"}, operations);
    REQUIRE(cleared.composer.draft.empty());
    REQUIRE_FALSE(cleared.intent.has_value());

    const auto deleted =
        translate_event(KeyEvent::of(Key::DeleteBefore), ComposerState{"ab"}, operations);
    REQUIRE(deleted.composer.draft == "a");
    REQUIRE_FALSE(deleted.intent.has_value());

    const auto at_start =
        translate_event(KeyEvent::of(Key::DeleteBefore), ComposerState{}, operations);
    REQUIRE(at_start.handled);
    REQUIRE(at_start.composer.draft.empty());

    const auto unknown =
        translate_event(KeyEvent::of(Key::Unknown), ComposerState{"keep"}, operations);
    REQUIRE_FALSE(unknown.handled);
    REQUIRE(unknown.composer.draft == "keep");
}

TEST_CASE("deletion removes a whole code point", "[input][utf8]") {
    const std::vector<OperationRecord> operations;

    const auto accented =
        translate_event(KeyEvent::of(Key::DeleteBefore), ComposerState{"caf\xC3\xA9"}, operations);
    REQUIRE(accented.composer.draft == "caf");

    const auto emoji = translate_event(KeyEvent::of(Key::DeleteBefore),
                                       ComposerState{"a\xF0\x9F\x99\x82"}, operations);
    REQUIRE(emoji.composer.draft == "a");
}

TEST_CASE("text splits into whole code points", "[input][utf8]") {
    const auto points = split_code_points("a\xC3\xA9\xF0\x9F\x99\x82");
    REQUIRE(points.size() == 3);
    REQUIRE(points[0] == "a");
    REQUIRE(points[1] == "\xC3\xA9");
    REQUIRE(points[2] == "\xF0\x9F\x99\x82");
    REQUIRE(split_code_points("").empty());
}

TEST_CASE("cancel and reconcile keys resolve against lifecycle state", "[input][intent]") {
    FakeSession session;
    REQUIRE_FALSE(translate_event(KeyEvent::of(Key::CancelOperation), ComposerState{},
                                  session.projection()->operations)
                      .intent.has_value());

    const auto receipt = session.admit(SubmitInput{"work"});
    const auto cancel = translate_event(KeyEvent::of(Key::CancelOperation), ComposerState{},
                                        session.projection()->operations);
    REQUIRE(std::get<CancelOperation>(*cancel.intent).id == *receipt.operation);

    REQUIRE_FALSE(translate_event(KeyEvent::of(Key::ReconcileOperation), ComposerState{},
                                  session.projection()->operations)
                      .intent.has_value());

    REQUIRE(session.simulate_next().has_value());
    REQUIRE(session.simulate_next().has_value());
    session.dispatch_control(std::get<CancelOperation>(*cancel.intent));

    const auto reconcile = translate_event(KeyEvent::of(Key::ReconcileOperation), ComposerState{},
                                           session.projection()->operations);
    REQUIRE(std::get<ReconcileOperation>(*reconcile.intent).id == *receipt.operation);
}
