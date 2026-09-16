#include "bernstein/core/session.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>
#include <utility>
#include <variant>

using namespace bernstein::core;

TEST_CASE("admission creates a receipt and pending operation", "[session][lifecycle]") {
    FakeSession session;

    const auto receipt = session.admit(SubmitInput{"Review the session boundary"});
    const auto view = session.projection();

    REQUIRE(receipt.status == InputStatus::Accepted);
    REQUIRE(receipt.id.value == "input-1");
    REQUIRE(receipt.operation.has_value());
    REQUIRE(view->inputs.size() == 1);
    REQUIRE(view->inputs.front().status == InputStatus::Accepted);
    REQUIRE(view->history.empty());
    REQUIRE(view->operations.size() == 1);
    REQUIRE(view->operations.front().status == OperationStatus::Pending);
    REQUIRE(view->durable_tail.size() == 2);
    REQUIRE(view->durable_tail[0].kind == EventKind::InputAdmitted);
    REQUIRE(view->durable_tail[1].kind == EventKind::OperationCreated);
}

TEST_CASE("promotion is distinct and idempotent for an input", "[session][lifecycle]") {
    FakeSession session;
    const auto receipt = session.admit(SubmitInput{"Promote me"});

    REQUIRE(session.promote(receipt.id));
    REQUIRE_FALSE(session.promote(receipt.id));

    const auto view = session.projection();
    REQUIRE(view->inputs.front().status == InputStatus::Promoted);
    REQUIRE(view->history.size() == 1);
    REQUIRE(view->history.front().input == receipt.id);
    REQUIRE(view->durable_tail.back().kind == EventKind::InputPromoted);
}

TEST_CASE("durable events can be replayed without changing projection", "[session][replay]") {
    FakeSession session;
    const auto receipt = session.admit(SubmitInput{"Replay me"});
    REQUIRE(session.promote(receipt.id));

    const auto before = session.projection();
    const auto replayed = session.replay(DurableEventCursor{0});
    const auto after = session.projection();

    REQUIRE(replayed.size() == before->durable_cursor.sequence);
    REQUIRE(replayed.back().kind == EventKind::InputPromoted);
    REQUIRE(after->inputs.size() == before->inputs.size());
    REQUIRE(after->inputs.front().status == before->inputs.front().status);
    REQUIRE(after->history.size() == before->history.size());
    REQUIRE(after->history.front().input == before->history.front().input);
    REQUIRE(after->operations.size() == before->operations.size());
    REQUIRE(after->operations.front().status == before->operations.front().status);

    const auto tail = session.replay(before->durable_cursor);
    REQUIRE(tail.empty());
}

TEST_CASE("approval request changes proposal state without admitting an effect",
          "[session][approval]") {
    FakeSession session;

    session.dispatch_control(RequestApproval{ProposalId{"proposal-1"}});

    const auto view = session.projection();
    REQUIRE(view->proposals.front().status == ProposalStatus::RequiresApproval);
    REQUIRE(view->operations.empty());
    REQUIRE(view->durable_tail.back().kind == EventKind::ApprovalRequested);
    REQUIRE(view->durable_tail.back().proposal == ProposalId{"proposal-1"});
}

TEST_CASE("selection changes only selection state", "[session][selection]") {
    FakeSession session;
    const auto before = session.projection();

    session.dispatch_control(SelectModule{ModuleId{"module-1"}});

    const auto after = session.projection();
    REQUIRE(std::holds_alternative<std::monostate>(before->selection));
    REQUIRE(std::get<ModuleId>(after->selection) == ModuleId{"module-1"});
    REQUIRE(after->inputs.empty());
    REQUIRE(after->operations.empty());
    REQUIRE(after->durable_tail.empty());
    REQUIRE(after->live_observations.empty());
    REQUIRE(after->proposals.front().status == before->proposals.front().status);
}

namespace {

OperationId start_operation(FakeSession& session, std::string text) {
    const auto receipt = session.admit(SubmitInput{std::move(text)});
    REQUIRE(session.simulate_next()->status == OperationStatus::Pending);
    REQUIRE(session.simulate_next()->status == OperationStatus::Running);
    return *receipt.operation;
}

} // namespace

TEST_CASE("cancellation before dispatch interrupts without an unknown outcome",
          "[session][recovery]") {
    FakeSession session;
    const auto receipt = session.admit(SubmitInput{"Cancel me early"});
    REQUIRE(receipt.operation.has_value());

    session.dispatch_control(CancelOperation{*receipt.operation});

    const auto view = session.projection();
    REQUIRE(view->operations.front().status == OperationStatus::Interrupted);
    REQUIRE(view->operations.front().effect == EffectStatus::Intended);
    REQUIRE(view->durable_tail.back().kind == EventKind::OperationCancelled);
    REQUIRE_FALSE(session.simulate_next().has_value());
}

TEST_CASE("cancellation after dispatch preserves unknown external outcome", "[session][recovery]") {
    FakeSession session;
    const auto operation = start_operation(session, "Cancel me");

    session.dispatch_control(CancelOperation{operation});

    const auto view = session.projection();
    REQUIRE(view->operations.front().status == OperationStatus::Unknown);
    REQUIRE(view->operations.front().effect == EffectStatus::Unknown);
    REQUIRE(view->durable_tail.back().kind == EventKind::EffectUnknown);
    REQUIRE_FALSE(view->live_observations.empty());
    REQUIRE_FALSE(view->live_observations.back().replayable);
}

TEST_CASE("cancellation of a settled operation changes nothing durable", "[session][recovery]") {
    FakeSession session;
    const auto operation = start_operation(session, "Settle me");
    REQUIRE(session.simulate_next()->status == OperationStatus::Settled);
    const auto before = session.projection();

    session.dispatch_control(CancelOperation{operation});

    const auto after = session.projection();
    REQUIRE(after->operations.front().status == OperationStatus::Settled);
    REQUIRE(after->operations.front().effect == EffectStatus::Recorded);
    REQUIRE(after->durable_cursor.sequence == before->durable_cursor.sequence);
    REQUIRE(after->live_observations.back().kind == "cancel");
}

TEST_CASE("timeout after dispatch produces an unknown outcome", "[session][recovery]") {
    FakeSession session;
    const auto receipt = session.admit(SubmitInput{"Time me out"});
    REQUIRE_FALSE(session.simulate_timeout(*receipt.operation));

    REQUIRE(session.simulate_next()->status == OperationStatus::Pending);
    REQUIRE(session.simulate_next()->status == OperationStatus::Running);
    REQUIRE(session.simulate_timeout(*receipt.operation));

    const auto view = session.projection();
    REQUIRE(view->operations.front().status == OperationStatus::Unknown);
    REQUIRE(view->operations.front().effect == EffectStatus::Unknown);
    REQUIRE(view->durable_tail.back().kind == EventKind::EffectUnknown);
    REQUIRE_FALSE(session.simulate_next().has_value());
}

TEST_CASE("reconciliation does not invent an external outcome", "[session][recovery]") {
    FakeSession session;
    const auto operation = start_operation(session, "Reconcile me");
    session.dispatch_control(CancelOperation{operation});

    session.dispatch_control(ReconcileOperation{operation});

    const auto view = session.projection();
    REQUIRE(view->operations.front().status == OperationStatus::Unknown);
    REQUIRE(view->operations.front().effect == EffectStatus::Unknown);
    REQUIRE(view->durable_tail.back().kind == EventKind::OperationReconciled);
    REQUIRE(view->live_observations.back().kind == "reconcile");
    REQUIRE(view->live_observations.back().detail.find("No external outcome") != std::string::npos);
}

TEST_CASE("reconciliation without an unknown outcome records no durable event",
          "[session][recovery]") {
    FakeSession session;
    const auto receipt = session.admit(SubmitInput{"Nothing to reconcile"});
    const auto before = session.projection();

    session.dispatch_control(ReconcileOperation{*receipt.operation});

    const auto after = session.projection();
    REQUIRE(after->operations.front().status == OperationStatus::Pending);
    REQUIRE(after->durable_cursor.sequence == before->durable_cursor.sequence);
    REQUIRE(after->live_observations.back().kind == "reconcile");
}

TEST_CASE("simulation promotes, starts, and settles with replay cursors", "[session][simulation]") {
    FakeSession session;
    const auto receipt = session.admit(SubmitInput{"Simulate me"});
    REQUIRE(receipt.operation.has_value());

    const auto promoted = session.simulate_next();
    REQUIRE(promoted.has_value());
    REQUIRE(promoted->operation == *receipt.operation);
    REQUIRE(promoted->status == OperationStatus::Pending);
    REQUIRE(session.projection()->inputs.front().status == InputStatus::Promoted);
    REQUIRE(session.projection()->history.size() == 1);

    const auto started = session.simulate_next();
    REQUIRE(started.has_value());
    REQUIRE(started->operation == *receipt.operation);
    REQUIRE(started->status == OperationStatus::Running);
    REQUIRE(started->cursor.sequence > promoted->cursor.sequence);
    REQUIRE_FALSE(session.replay(DurableEventCursor{0}).empty());
    REQUIRE(session.replay(started->cursor).empty());

    const auto settled = session.simulate_next();
    REQUIRE(settled.has_value());
    REQUIRE(settled->operation == *receipt.operation);
    REQUIRE(settled->status == OperationStatus::Settled);
    REQUIRE(settled->cursor.sequence > started->cursor.sequence);

    const auto view = session.projection();
    REQUIRE(view->operations.front().effect == EffectStatus::Recorded);
    REQUIRE_FALSE(session.simulate_next().has_value());
}

TEST_CASE("simulation runs operations serially", "[session][simulation]") {
    FakeSession session;
    const auto first = start_operation(session, "First");
    const auto second = session.admit(SubmitInput{"Second"});

    REQUIRE(session.simulate_next()->operation == first);
    const auto view = session.projection();
    REQUIRE(view->operations[0].status == OperationStatus::Settled);
    REQUIRE(view->operations[1].status == OperationStatus::Pending);
    REQUIRE(view->inputs[1].status == InputStatus::Accepted);

    REQUIRE(session.simulate_next()->operation == *second.operation);
    REQUIRE(session.projection()->inputs[1].status == InputStatus::Promoted);
}

TEST_CASE("projection snapshots are shared until state changes", "[session][projection]") {
    FakeSession session;
    const auto first = session.projection();
    REQUIRE(first != nullptr);
    REQUIRE(session.projection() == first);

    const auto receipt = session.admit(SubmitInput{"Change state"});
    const auto second = session.projection();
    REQUIRE(second != first);
    REQUIRE(first->inputs.empty());
    REQUIRE(second->inputs.size() == 1);

    REQUIRE(session.promote(receipt.id));
    const auto third = session.projection();
    REQUIRE(third != second);
    REQUIRE_FALSE(session.promote(receipt.id));
    REQUIRE(session.projection() == third);

    REQUIRE(session.simulate_next().has_value());
    const auto fourth = session.projection();
    REQUIRE(fourth != third);
    REQUIRE(session.simulate_next().has_value());
    const auto fifth = session.projection();
    REQUIRE(fifth != fourth);
    REQUIRE_FALSE(session.simulate_next().has_value());
    REQUIRE(session.projection() == fifth);
    REQUIRE_FALSE(session.simulate_timeout(*receipt.operation));
    REQUIRE(session.projection() == fifth);

    session.dispatch_control(SelectModule{ModuleId{"module-1"}});
    REQUIRE(session.projection() != fifth);
}

TEST_CASE("projection carries a bounded durable tail while replay keeps full history",
          "[session][replay]") {
    FakeSession session;
    const auto inputs = durable_event_tail_size;
    for (std::size_t index = 0; index < inputs; ++index) {
        (void)session.admit(SubmitInput{"Input " + std::to_string(index)});
    }

    // Each admission records two durable events.
    const auto total = inputs * 2;
    const auto view = session.projection();
    REQUIRE(view->durable_tail.size() == durable_event_tail_size);
    REQUIRE(view->durable_cursor.sequence == total);
    REQUIRE(view->durable_tail.back().cursor.sequence == total);
    REQUIRE(view->durable_tail.front().cursor.sequence == total - durable_event_tail_size + 1);

    const auto replayed = session.replay(DurableEventCursor{0});
    REQUIRE(replayed.size() == total);
    REQUIRE(replayed.front().cursor.sequence == 1);
    REQUIRE(replayed.back().cursor.sequence == total);
}
