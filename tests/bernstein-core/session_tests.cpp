#include "bernstein/core/session.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace bernstein::core;

TEST_CASE("admission creates a receipt and pending operation", "[session][lifecycle]") {
    FakeSession session;

    const auto receipt = session.admit(SubmitInput{"Review the session boundary"});
    const auto view = session.projection();

    REQUIRE(receipt.status == InputStatus::Accepted);
    REQUIRE(receipt.id.value == "input-1");
    REQUIRE(receipt.operation.has_value());
    REQUIRE(view.inputs.size() == 1);
    REQUIRE(view.inputs.front().status == InputStatus::Accepted);
    REQUIRE(view.history.empty());
    REQUIRE(view.operations.size() == 1);
    REQUIRE(view.operations.front().status == OperationStatus::Pending);
    REQUIRE(view.durable_events.size() == 2);
    REQUIRE(view.durable_events[0].kind == EventKind::InputAdmitted);
    REQUIRE(view.durable_events[1].kind == EventKind::OperationCreated);
}

TEST_CASE("promotion is distinct and idempotent for an input", "[session][lifecycle]") {
    FakeSession session;
    const auto receipt = session.admit(SubmitInput{"Promote me"});

    REQUIRE(session.promote(receipt.id));
    REQUIRE_FALSE(session.promote(receipt.id));

    const auto view = session.projection();
    REQUIRE(view.inputs.front().status == InputStatus::Promoted);
    REQUIRE(view.history.size() == 1);
    REQUIRE(view.history.front().input == receipt.id);
    REQUIRE(view.durable_events.back().kind == EventKind::InputPromoted);
}

TEST_CASE("durable events can be replayed without changing projection", "[session][replay]") {
    FakeSession session;
    const auto receipt = session.admit(SubmitInput{"Replay me"});
    REQUIRE(session.promote(receipt.id));

    const auto before = session.projection();
    const auto replayed = session.replay(DurableEventCursor{0});
    const auto after = session.projection();

    REQUIRE(replayed.size() == before.durable_events.size());
    REQUIRE(replayed.back().kind == EventKind::InputPromoted);
    REQUIRE(after.inputs.size() == before.inputs.size());
    REQUIRE(after.inputs.front().status == before.inputs.front().status);
    REQUIRE(after.history.size() == before.history.size());
    REQUIRE(after.history.front().input == before.history.front().input);
    REQUIRE(after.operations.size() == before.operations.size());
    REQUIRE(after.operations.front().status == before.operations.front().status);

    const auto tail = session.replay(before.durable_events.back().cursor);
    REQUIRE(tail.empty());
}

TEST_CASE("approval request changes proposal state without admitting an effect",
          "[session][approval]") {
    FakeSession session;

    session.dispatch_control(RequestApproval{ProposalId{"proposal-1"}});

    const auto view = session.projection();
    REQUIRE(view.proposals.front().status == ProposalStatus::RequiresApproval);
    REQUIRE(view.operations.empty());
    REQUIRE(view.durable_events.back().kind == EventKind::ApprovalRequested);
}

TEST_CASE("cancellation preserves unknown external outcome", "[session][recovery]") {
    FakeSession session;
    const auto receipt = session.admit(SubmitInput{"Cancel me"});
    REQUIRE(receipt.operation.has_value());

    session.dispatch_control(CancelOperation{*receipt.operation});

    const auto view = session.projection();
    REQUIRE(view.operations.front().status == OperationStatus::Unknown);
    REQUIRE(view.operations.front().effect == EffectStatus::Intended);
    REQUIRE(view.durable_events.back().kind == EventKind::EffectUnknown);
    REQUIRE_FALSE(view.live_observations.empty());
    REQUIRE_FALSE(view.live_observations.back().replayable);
}

TEST_CASE("reconciliation does not invent an external outcome", "[session][recovery]") {
    FakeSession session;
    const auto receipt = session.admit(SubmitInput{"Reconcile me"});
    REQUIRE(receipt.operation.has_value());

    session.dispatch_control(ReconcileOperation{*receipt.operation});

    const auto view = session.projection();
    REQUIRE(view.operations.front().status == OperationStatus::Pending);
    REQUIRE(view.operations.front().effect == EffectStatus::Intended);
    REQUIRE(view.durable_events.back().kind == EventKind::OperationReconciled);
    REQUIRE(view.live_observations.back().kind == "reconcile");
    REQUIRE(view.live_observations.back().detail.find("No external outcome") != std::string::npos);
}

TEST_CASE("projection is a value view and cannot mutate session state", "[session][projection]") {
    FakeSession session;
    auto copy = session.projection();
    copy.workspace_name = "not the session";
    copy.proposals.clear();
    copy.durable_events.clear();

    const auto original = session.projection();
    REQUIRE(original.workspace_name == "Bernstein workspace");
    REQUIRE(original.proposals.size() == 1);
    REQUIRE(original.durable_events.empty());
}
