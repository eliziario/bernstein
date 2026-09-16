#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace bernstein::core {

template <typename Tag> struct Identifier {
    std::string value;

    auto operator<=>(const Identifier&) const = default;

    [[nodiscard]] bool empty() const {
        return value.empty();
    }
};

struct SessionTag;
struct InputTag;
struct ProjectTag;
struct ModuleTag;
struct EvidenceTag;
struct ProposalTag;
struct CandidateTag;
struct OperationTag;
struct ExecutionAttemptTag;
struct ObservationTag;
struct EventTag;

using SessionId = Identifier<SessionTag>;
using InputId = Identifier<InputTag>;
using ProjectId = Identifier<ProjectTag>;
using ModuleId = Identifier<ModuleTag>;
using EvidenceId = Identifier<EvidenceTag>;
using ProposalId = Identifier<ProposalTag>;
using CandidateId = Identifier<CandidateTag>;
using OperationId = Identifier<OperationTag>;
using ExecutionAttemptId = Identifier<ExecutionAttemptTag>;
using ObservationId = Identifier<ObservationTag>;
using EventId = Identifier<EventTag>;

enum class DeliveryMode {
    Steer,
    Queue,
};

enum class InputStatus {
    Accepted,
    Promoted,
    Rejected,
    Conflict,
};

enum class OperationStatus {
    Pending,
    Running,
    Interrupted,
    Unknown,
    Settled,
};

enum class ExecutionAttemptStatus {
    Ready,
    Running,
    Interrupted,
    Unknown,
    Settled,
};

enum class EffectStatus {
    Intended,
    Recorded,
    Observed,
    Unknown,
};

enum class ProposalStatus {
    Proposed,
    RequiresApproval,
    Admitted,
};

enum class VerificationStatus {
    Unverified,
    Verified,
    CoverageGap,
};

enum class EventKind {
    InputAdmitted,
    InputPromoted,
    ApprovalRequested,
    OperationCreated,
    OperationCancelled,
    ExecutionStarted,
    EffectRecorded,
    EffectUnknown,
    OperationSettled,
    OperationReconciled,
};

[[nodiscard]] std::string_view to_string(InputStatus status);
[[nodiscard]] std::string_view to_string(OperationStatus status);
[[nodiscard]] std::string_view to_string(EffectStatus status);
[[nodiscard]] std::string_view to_string(ProposalStatus status);
[[nodiscard]] std::string_view to_string(VerificationStatus status);
[[nodiscard]] std::string_view to_string(EventKind kind);

// Pending and Running operations can still change without reconciliation.
[[nodiscard]] bool is_active(OperationStatus status);

struct Project {
    ProjectId id;
    std::string name;
    std::vector<ModuleId> modules;
};

struct Module {
    ModuleId id;
    ProjectId project;
    std::string name;
    std::vector<EvidenceId> evidence;
};

struct Evidence {
    EvidenceId id;
    std::string subject;
    std::string version;
    bool fresh = false;
};

struct Proposal {
    ProposalId id;
    std::string title;
    std::string goal;
    std::vector<EvidenceId> evidence;
    std::vector<std::string> assumptions;
    std::vector<std::string> obligations;
    ProposalStatus status = ProposalStatus::Proposed;
};

struct Candidate {
    CandidateId id;
    ProposalId proposal;
    std::string artifact;
    VerificationStatus verification = VerificationStatus::Unverified;
};

struct InputRecord {
    InputId id;
    SessionId session;
    std::string text;
    DeliveryMode delivery = DeliveryMode::Steer;
    InputStatus status = InputStatus::Accepted;
    std::optional<OperationId> operation;
};

struct InputReceipt {
    InputId id;
    SessionId session;
    DeliveryMode delivery = DeliveryMode::Steer;
    InputStatus status = InputStatus::Accepted;
    std::optional<OperationId> operation;
};

struct ConversationEntry {
    std::uint64_t sequence = 0;
    InputId input;
    std::string text;
};

struct ExecutionAttempt {
    ExecutionAttemptId id;
    OperationId operation;
    ExecutionAttemptStatus status = ExecutionAttemptStatus::Ready;
};

struct OperationRecord {
    OperationId id;
    InputId input;
    OperationStatus status = OperationStatus::Pending;
    EffectStatus effect = EffectStatus::Intended;
    std::optional<ExecutionAttemptId> active_attempt;
    std::string detail;
};

struct DurableEventCursor {
    std::uint64_t sequence = 0;
};

struct DurableEvent {
    EventId id;
    DurableEventCursor cursor;
    EventKind kind = EventKind::InputAdmitted;
    std::optional<OperationId> operation;
    std::optional<ProposalId> proposal;
    std::string detail;
};

struct SimulationReceipt {
    OperationId operation;
    OperationStatus status = OperationStatus::Pending;
    DurableEventCursor cursor;
};

struct LiveObservation {
    ObservationId id;
    std::optional<OperationId> operation;
    std::string kind;
    std::string detail;
    bool replayable = false;
};

using Selection = std::variant<std::monostate, ProjectId, ModuleId, EvidenceId, ProposalId,
                               CandidateId, OperationId>;

// Number of most recent durable events carried by a projection. Full history is read through
// SessionFacade::replay.
inline constexpr std::size_t durable_event_tail_size = 32;

// An immutable snapshot for display. A session publishes a new snapshot after each state change
// and shares it with readers; readers never observe later changes through an existing snapshot.
struct UiProjection {
    SessionId session;
    std::string workspace_name;
    std::string branch;
    Selection selection;
    std::vector<Project> projects;
    std::vector<Module> modules;
    std::vector<Evidence> evidence;
    std::vector<Proposal> proposals;
    std::vector<Candidate> candidates;
    std::vector<InputRecord> inputs;
    std::vector<ConversationEntry> history;
    std::vector<OperationRecord> operations;
    // The last durable_event_tail_size events, oldest first.
    std::vector<DurableEvent> durable_tail;
    // Cursor of the latest durable event; sequence 0 when no event exists.
    DurableEventCursor durable_cursor;
    std::vector<LiveObservation> live_observations;
    bool quit_requested = false;
};

struct SubmitInput {
    std::string text;
    DeliveryMode delivery = DeliveryMode::Steer;
};

struct SelectProject {
    ProjectId id;
};

struct SelectModule {
    ModuleId id;
};

struct SelectEvidence {
    EvidenceId id;
};

struct InspectProposal {
    ProposalId id;
};

struct InspectCandidate {
    CandidateId id;
};

struct RequestApproval {
    ProposalId id;
};

struct CancelOperation {
    OperationId id;
};

struct ReconcileOperation {
    OperationId id;
};

struct OpenDiff {
    CandidateId id;
};

struct Quit {};

using UserIntent = std::variant<SubmitInput, SelectProject, SelectModule, SelectEvidence,
                                InspectProposal, InspectCandidate, RequestApproval, CancelOperation,
                                ReconcileOperation, OpenDiff, Quit>;

class SessionFacade {
  public:
    virtual ~SessionFacade() = default;

    virtual InputReceipt admit(SubmitInput input) = 0;
    virtual bool promote(InputId input) = 0;
    // Returns the snapshot published after the most recent state change. Repeated calls without
    // an intervening change return the same pointer; the call never copies session state.
    [[nodiscard]] virtual std::shared_ptr<const UiProjection> projection() const = 0;
    [[nodiscard]] virtual std::vector<DurableEvent> replay(DurableEventCursor after) const = 0;
    virtual void dispatch_control(UserIntent intent) = 0;
};

class FakeSession final : public SessionFacade {
  public:
    FakeSession();

    InputReceipt admit(SubmitInput input) override;
    bool promote(InputId input) override;
    [[nodiscard]] std::shared_ptr<const UiProjection> projection() const override;
    [[nodiscard]] std::vector<DurableEvent> replay(DurableEventCursor after) const override;
    void dispatch_control(UserIntent intent) override;

    // Advances the oldest active operation by one lifecycle step: promotion, execution start,
    // then settlement. Operations run serially; a later input waits until earlier ones leave
    // the active states.
    [[nodiscard]] std::optional<SimulationReceipt> simulate_next();

    // Simulates a timeout after dispatch. Returns false unless the operation is Running.
    bool simulate_timeout(const OperationId& operation);

  public:
    struct State {
        SessionId session;
        std::string workspace_name;
        std::string branch;
        Selection selection;
        std::vector<Project> projects;
        std::vector<Module> modules;
        std::vector<Evidence> evidence;
        std::vector<Proposal> proposals;
        std::vector<Candidate> candidates;
        std::vector<InputRecord> inputs;
        std::vector<ConversationEntry> history;
        std::vector<ExecutionAttempt> attempts;
        std::vector<OperationRecord> operations;
        std::vector<DurableEvent> durable_events;
        std::vector<LiveObservation> live_observations;
        std::uint64_t next_input = 1;
        std::uint64_t next_event = 1;
        std::uint64_t next_history = 1;
        bool quit_requested = false;
    };

  private:
    InputReceipt admit_input(SubmitInput input);
    bool promote_input(const InputId& input);
    std::optional<SimulationReceipt> advance_simulation();
    void publish_projection();

    State state_;
    std::shared_ptr<const UiProjection> projection_;
};

} // namespace bernstein::core
