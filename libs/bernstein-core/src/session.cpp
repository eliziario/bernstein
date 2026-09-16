#include "bernstein/core/session.hpp"

#include <algorithm>
#include <iterator>
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
                  std::string detail) {
    const auto sequence = state.durable_events.size() + 1;
    state.durable_events.push_back(
        DurableEvent{make_id<EventId>("event-" + std::to_string(state.next_event++)),
                     DurableEventCursor{sequence}, kind, std::move(operation), std::move(detail)});
}

void append_live(FakeSession::State& state, std::optional<OperationId> operation, std::string kind,
                 std::string detail) {
    state.live_observations.push_back(LiveObservation{
        make_id<ObservationId>("observation-" + std::to_string(state.live_observations.size() + 1)),
        std::move(operation), std::move(kind), std::move(detail), false});
}

} // namespace

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
}

InputReceipt FakeSession::admit(SubmitInput input) {
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

bool FakeSession::promote(InputId input) {
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

UiProjection FakeSession::projection() const {
    return UiProjection{state_.session,        state_.workspace_name,    state_.branch,
                        state_.selection,      state_.projects,          state_.modules,
                        state_.evidence,       state_.proposals,         state_.candidates,
                        state_.inputs,         state_.history,           state_.operations,
                        state_.durable_events, state_.live_observations, state_.quit_requested};
}

std::vector<DurableEvent> FakeSession::replay(DurableEventCursor after) const {
    std::vector<DurableEvent> result;
    std::copy_if(state_.durable_events.begin(), state_.durable_events.end(),
                 std::back_inserter(result),
                 [&after](const auto& event) { return event.cursor.sequence > after.sequence; });
    return result;
}

std::optional<SimulationReceipt> FakeSession::simulate_next() {
    const auto pending =
        std::find_if(state_.operations.begin(), state_.operations.end(), [](const auto& operation) {
            return operation.status == OperationStatus::Pending;
        });
    if (pending != state_.operations.end()) {
        const auto attempt_id =
            make_id<ExecutionAttemptId>("attempt-" + std::to_string(state_.attempts.size() + 1));
        state_.attempts.push_back(
            ExecutionAttempt{attempt_id, pending->id, ExecutionAttemptStatus::Running});
        pending->status = OperationStatus::Running;
        pending->active_attempt = attempt_id;
        pending->detail = "Simulated execution started.";
        append_event(state_, EventKind::ExecutionStarted, pending->id, pending->detail);
        return SimulationReceipt{pending->id, pending->status, state_.durable_events.back().cursor};
    }

    const auto running =
        std::find_if(state_.operations.begin(), state_.operations.end(), [](const auto& operation) {
            return operation.status == OperationStatus::Running;
        });
    if (running != state_.operations.end()) {
        running->status = OperationStatus::Settled;
        running->effect = EffectStatus::Recorded;
        running->detail = "Simulated execution settled.";
        if (running->active_attempt.has_value()) {
            const auto attempt = find_by_id(state_.attempts, *running->active_attempt);
            if (attempt != state_.attempts.end()) {
                attempt->status = ExecutionAttemptStatus::Settled;
            }
        }
        append_event(state_, EventKind::EffectRecorded, running->id,
                     "Simulated effect was recorded.");
        append_event(state_, EventKind::OperationSettled, running->id, running->detail);
        return SimulationReceipt{running->id, running->status, state_.durable_events.back().cursor};
    }

    return std::nullopt;
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
                                 "Proposal is awaiting explicit human approval.");
                } else {
                    append_live(state_, std::nullopt, "approval",
                                "Approval request could not be matched to a proposed item.");
                }
            } else if constexpr (std::is_same_v<Action, CancelOperation>) {
                const auto operation = find_by_id(state_.operations, action.id);
                if (operation == state_.operations.end()) {
                    append_live(state_, action.id, "cancel", "Operation identity is unknown.");
                } else {
                    operation->status = OperationStatus::Unknown;
                    operation->detail = "Cancellation was requested; external outcome is unknown.";
                    append_event(state_, EventKind::EffectUnknown, action.id,
                                 "Cancellation does not prove that an external effect stopped.");
                    append_live(state_, action.id, "cancel", operation->detail);
                }
            } else if constexpr (std::is_same_v<Action, ReconcileOperation>) {
                const auto operation = find_by_id(state_.operations, action.id);
                if (operation == state_.operations.end()) {
                    append_live(state_, action.id, "reconcile", "Operation identity is unknown.");
                } else {
                    append_live(state_, action.id, "reconcile",
                                "No external outcome is available in the deterministic session.");
                    append_event(state_, EventKind::OperationReconciled, action.id,
                                 "Reconciliation remains unresolved.");
                }
            } else if constexpr (std::is_same_v<Action, Quit>) {
                state_.quit_requested = true;
            } else if constexpr (std::is_same_v<Action, SubmitInput>) {
                (void)admit(action);
            }
        },
        std::move(intent));
}

} // namespace bernstein::core
