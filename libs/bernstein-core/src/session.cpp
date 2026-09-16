#include "bernstein/core/session.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace bernstein::core {
namespace {

template <typename Id> Id make_id(std::string value) {
    return Id{std::move(value)};
}

template <typename Collection, typename Id>
auto find_by_id(Collection& collection, const Id& expected) {
    return std::find_if(collection.begin(), collection.end(),
                        [&expected](const auto& item) { return item.id == expected; });
}

} // namespace

namespace {

void append_event(FakeSession::State& state, EventKind kind, std::optional<OperationId> operation,
                  std::string detail, std::optional<ProposalId> proposal = std::nullopt) {
    const auto sequence = state.durable_events.size() + 1;
    state.durable_events.push_back(
        DurableEvent{make_id<EventId>("event-" + std::to_string(state.next_event++)),
                     DurableEventCursor{sequence}, kind, std::move(operation), std::move(proposal),
                     std::move(detail)});
}

void append_live(FakeSession::State& state, std::optional<OperationId> operation, std::string kind,
                 std::string detail) {
    state.live_observations.push_back(LiveObservation{
        make_id<ObservationId>("observation-" + std::to_string(state.live_observations.size() + 1)),
        std::move(operation), std::move(kind), std::move(detail), false});
}

void set_attempt_status(FakeSession::State& state, const OperationRecord& operation,
                        ExecutionAttemptStatus status) {
    if (!operation.active_attempt.has_value()) {
        return;
    }
    const auto attempt = find_by_id(state.attempts, *operation.active_attempt);
    if (attempt != state.attempts.end()) {
        attempt->status = status;
    }
}

} // namespace

std::string_view to_string(InputStatus status) {
    switch (status) {
    case InputStatus::Accepted:
        return "accepted";
    case InputStatus::Promoted:
        return "promoted";
    case InputStatus::Rejected:
        return "rejected";
    case InputStatus::Conflict:
        return "conflict";
    }
    return "invalid";
}

std::string_view to_string(OperationStatus status) {
    switch (status) {
    case OperationStatus::Pending:
        return "pending";
    case OperationStatus::Running:
        return "running";
    case OperationStatus::Interrupted:
        return "interrupted";
    case OperationStatus::Unknown:
        return "unknown";
    case OperationStatus::Settled:
        return "settled";
    }
    return "invalid";
}

std::string_view to_string(EffectStatus status) {
    switch (status) {
    case EffectStatus::Intended:
        return "intended";
    case EffectStatus::Recorded:
        return "recorded";
    case EffectStatus::Observed:
        return "observed";
    case EffectStatus::Unknown:
        return "unknown";
    }
    return "invalid";
}

std::string_view to_string(ProposalStatus status) {
    switch (status) {
    case ProposalStatus::Proposed:
        return "proposed";
    case ProposalStatus::RequiresApproval:
        return "requires approval";
    case ProposalStatus::Admitted:
        return "admitted";
    }
    return "invalid";
}

std::string_view to_string(VerificationStatus status) {
    switch (status) {
    case VerificationStatus::Unverified:
        return "unverified";
    case VerificationStatus::Verified:
        return "verified";
    case VerificationStatus::CoverageGap:
        return "coverage gap";
    }
    return "invalid";
}

std::string_view to_string(EventKind kind) {
    switch (kind) {
    case EventKind::InputAdmitted:
        return "input admitted";
    case EventKind::InputPromoted:
        return "input promoted";
    case EventKind::ApprovalRequested:
        return "approval requested";
    case EventKind::OperationCreated:
        return "operation created";
    case EventKind::OperationCancelled:
        return "operation cancelled";
    case EventKind::ExecutionStarted:
        return "execution started";
    case EventKind::EffectRecorded:
        return "effect recorded";
    case EventKind::EffectUnknown:
        return "effect unknown";
    case EventKind::OperationSettled:
        return "operation settled";
    case EventKind::OperationReconciled:
        return "operation reconciled";
    }
    return "invalid";
}

bool is_active(OperationStatus status) {
    return status == OperationStatus::Pending || status == OperationStatus::Running;
}

FakeSession::FakeSession() : state_{} {
    state_.session = make_id<SessionId>("session-1");
    state_.workspace_name = "Bernstein workspace";
    state_.branch = "main";

    state_.projects.push_back(
        Project{make_id<ProjectId>("project-1"), "Agent host", {make_id<ModuleId>("module-1")}});
    state_.modules.push_back(Module{make_id<ModuleId>("module-1"),
                                    make_id<ProjectId>("project-1"),
                                    "Session boundary",
                                    {make_id<EvidenceId>("evidence-1")}});
    state_.evidence.push_back(Evidence{make_id<EvidenceId>("evidence-1"), "TUI lifecycle contract",
                                       "workspace@initial", true});
    state_.proposals.push_back(
        Proposal{make_id<ProposalId>("proposal-1"),
                 "Review the session boundary",
                 "Keep UI projection separate from authority and effects.",
                 {make_id<EvidenceId>("evidence-1")},
                 {"The displayed workspace snapshot identifies the review scope."},
                 {"Admission, execution, observation, and settlement remain distinct."},
                 ProposalStatus::Proposed});
    state_.candidates.push_back(Candidate{make_id<CandidateId>("candidate-1"),
                                          make_id<ProposalId>("proposal-1"),
                                          "candidate/session-boundary"});
    publish_projection();
}

InputReceipt FakeSession::admit(SubmitInput input) {
    auto receipt = admit_input(std::move(input));
    publish_projection();
    return receipt;
}

bool FakeSession::promote(InputId input) {
    if (!promote_input(input)) {
        return false;
    }
    publish_projection();
    return true;
}

InputReceipt FakeSession::admit_input(SubmitInput input) {
    const auto input_id = make_id<InputId>("input-" + std::to_string(state_.next_input++));
    const auto operation_id = make_id<OperationId>("operation-" + input_id.value.substr(6));
    state_.inputs.push_back(InputRecord{input_id, state_.session, input.text, input.delivery,
                                        InputStatus::Accepted, operation_id});
    state_.operations.push_back(OperationRecord{operation_id, input_id, OperationStatus::Pending,
                                                EffectStatus::Intended, std::nullopt,
                                                "Accepted; not promoted or executed."});
    append_event(state_, EventKind::InputAdmitted, operation_id,
                 "Input admitted; assistant response is not implied.");
    append_event(state_, EventKind::OperationCreated, operation_id,
                 "Operation record created without a live execution pass.");
    return InputReceipt{input_id, state_.session, input.delivery, InputStatus::Accepted,
                        operation_id};
}

bool FakeSession::promote_input(const InputId& input) {
    const auto record = find_by_id(state_.inputs, input);
    if (record == state_.inputs.end() || record->status != InputStatus::Accepted) {
        return false;
    }

    record->status = InputStatus::Promoted;
    state_.history.push_back(ConversationEntry{state_.next_history++, input, record->text});
    append_event(state_, EventKind::InputPromoted, record->operation,
                 "Input promoted to model-visible history.");
    return true;
}

void FakeSession::publish_projection() {
    const auto& events = state_.durable_events;
    const auto tail_begin = events.end() - static_cast<std::ptrdiff_t>(
                                               std::min(events.size(), durable_event_tail_size));
    const auto cursor = events.empty() ? DurableEventCursor{} : events.back().cursor;

    projection_ = std::make_shared<const UiProjection>(UiProjection{
        state_.session, state_.workspace_name, state_.branch, state_.selection, state_.projects,
        state_.modules, state_.evidence, state_.proposals, state_.candidates, state_.inputs,
        state_.history, state_.operations, std::vector<DurableEvent>(tail_begin, events.end()),
        cursor, state_.live_observations, state_.quit_requested});
}

std::shared_ptr<const UiProjection> FakeSession::projection() const {
    return projection_;
}

std::vector<DurableEvent> FakeSession::replay(DurableEventCursor after) const {
    std::vector<DurableEvent> result;
    std::copy_if(state_.durable_events.begin(), state_.durable_events.end(),
                 std::back_inserter(result),
                 [&after](const auto& event) { return event.cursor.sequence > after.sequence; });
    return result;
}

std::optional<SimulationReceipt> FakeSession::simulate_next() {
    auto receipt = advance_simulation();
    if (receipt.has_value()) {
        publish_projection();
    }
    return receipt;
}

std::optional<SimulationReceipt> FakeSession::advance_simulation() {
    const auto operation =
        std::find_if(state_.operations.begin(), state_.operations.end(),
                     [](const auto& candidate) { return is_active(candidate.status); });
    if (operation == state_.operations.end()) {
        return std::nullopt;
    }

    if (operation->status == OperationStatus::Pending) {
        const auto input = find_by_id(state_.inputs, operation->input);
        if (input != state_.inputs.end() && input->status == InputStatus::Accepted) {
            (void)promote_input(input->id);
            operation->detail = "Input promoted; execution has not started.";
            return SimulationReceipt{operation->id, operation->status,
                                     state_.durable_events.back().cursor};
        }

        const auto attempt_id =
            make_id<ExecutionAttemptId>("attempt-" + std::to_string(state_.attempts.size() + 1));
        state_.attempts.push_back(
            ExecutionAttempt{attempt_id, operation->id, ExecutionAttemptStatus::Running});
        operation->status = OperationStatus::Running;
        operation->active_attempt = attempt_id;
        operation->detail = "Simulated execution started.";
        append_event(state_, EventKind::ExecutionStarted, operation->id, operation->detail);
        return SimulationReceipt{operation->id, operation->status,
                                 state_.durable_events.back().cursor};
    }

    operation->status = OperationStatus::Settled;
    operation->effect = EffectStatus::Recorded;
    operation->detail = "Simulated execution settled.";
    set_attempt_status(state_, *operation, ExecutionAttemptStatus::Settled);
    append_event(state_, EventKind::EffectRecorded, operation->id,
                 "Simulated effect was recorded.");
    append_event(state_, EventKind::OperationSettled, operation->id, operation->detail);
    return SimulationReceipt{operation->id, operation->status, state_.durable_events.back().cursor};
}

bool FakeSession::simulate_timeout(const OperationId& id) {
    const auto operation = find_by_id(state_.operations, id);
    if (operation == state_.operations.end() || operation->status != OperationStatus::Running) {
        return false;
    }

    operation->status = OperationStatus::Unknown;
    operation->effect = EffectStatus::Unknown;
    operation->detail = "Timed out after dispatch; external outcome is unknown.";
    set_attempt_status(state_, *operation, ExecutionAttemptStatus::Unknown);
    append_event(state_, EventKind::EffectUnknown, operation->id, operation->detail);
    append_live(state_, operation->id, "timeout", operation->detail);
    publish_projection();
    return true;
}

void FakeSession::dispatch_control(UserIntent intent) {
    std::visit(
        [this](const auto& action) {
            using Action = std::decay_t<decltype(action)>;
            if constexpr (std::is_same_v<Action, SelectProject> ||
                          std::is_same_v<Action, SelectModule> ||
                          std::is_same_v<Action, SelectEvidence> ||
                          std::is_same_v<Action, InspectProposal> ||
                          std::is_same_v<Action, InspectCandidate> ||
                          std::is_same_v<Action, OpenDiff>) {
                state_.selection = action.id;
            } else if constexpr (std::is_same_v<Action, RequestApproval>) {
                const auto proposal = find_by_id(state_.proposals, action.id);
                if (proposal != state_.proposals.end() &&
                    proposal->status == ProposalStatus::Proposed) {
                    proposal->status = ProposalStatus::RequiresApproval;
                    append_event(state_, EventKind::ApprovalRequested, std::nullopt,
                                 "Proposal " + proposal->id.value +
                                     " is awaiting explicit human approval.",
                                 proposal->id);
                } else {
                    append_live(state_, std::nullopt, "approval",
                                "Approval request could not be matched to a proposed item.");
                }
            } else if constexpr (std::is_same_v<Action, CancelOperation>) {
                const auto operation = find_by_id(state_.operations, action.id);
                if (operation == state_.operations.end()) {
                    append_live(state_, action.id, "cancel", "Operation identity is unknown.");
                } else if (operation->status == OperationStatus::Pending) {
                    operation->status = OperationStatus::Interrupted;
                    operation->detail = "Cancelled before dispatch; no effect was attempted.";
                    append_event(state_, EventKind::OperationCancelled, action.id,
                                 operation->detail);
                } else if (operation->status == OperationStatus::Running) {
                    operation->status = OperationStatus::Unknown;
                    operation->effect = EffectStatus::Unknown;
                    operation->detail = "Cancellation was requested; external outcome is unknown.";
                    set_attempt_status(state_, *operation, ExecutionAttemptStatus::Unknown);
                    append_event(state_, EventKind::EffectUnknown, action.id,
                                 "Cancellation does not prove that an external effect stopped.");
                    append_live(state_, action.id, "cancel", operation->detail);
                } else {
                    append_live(state_, action.id, "cancel",
                                "Operation is " + std::string(to_string(operation->status)) +
                                    "; cancellation has no effect.");
                }
            } else if constexpr (std::is_same_v<Action, ReconcileOperation>) {
                const auto operation = find_by_id(state_.operations, action.id);
                if (operation == state_.operations.end()) {
                    append_live(state_, action.id, "reconcile", "Operation identity is unknown.");
                } else if (operation->status == OperationStatus::Unknown) {
                    append_live(state_, action.id, "reconcile",
                                "No external outcome is available in the deterministic session.");
                    append_event(state_, EventKind::OperationReconciled, action.id,
                                 "Reconciliation remains unresolved.");
                } else {
                    append_live(state_, action.id, "reconcile",
                                "Operation is " + std::string(to_string(operation->status)) +
                                    "; there is no unknown outcome to reconcile.");
                }
            } else if constexpr (std::is_same_v<Action, Quit>) {
                state_.quit_requested = true;
            } else if constexpr (std::is_same_v<Action, SubmitInput>) {
                (void)admit_input(action);
            }
        },
        std::move(intent));
    // Every intent above writes session state, including diagnostics for unmatched identities.
    publish_projection();
}

} // namespace bernstein::core
