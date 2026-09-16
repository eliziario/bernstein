#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
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
    EffectRecorded,
    EffectUnknown,
    OperationReconciled,
};

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
    std::string detail;
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
    std::vector<DurableEvent> durable_events;
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
    [[nodiscard]] virtual UiProjection projection() const = 0;
    [[nodiscard]] virtual std::vector<DurableEvent> replay(DurableEventCursor after) const = 0;
    virtual void dispatch_control(UserIntent intent) = 0;
};

class FakeSession final : public SessionFacade {
  public:
    FakeSession();

    InputReceipt admit(SubmitInput input) override;
    bool promote(InputId input) override;
    [[nodiscard]] UiProjection projection() const override;
    [[nodiscard]] std::vector<DurableEvent> replay(DurableEventCursor after) const override;
    void dispatch_control(UserIntent intent) override;

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
    State state_;
};

} // namespace bernstein::core
