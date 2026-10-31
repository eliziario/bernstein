# RFC 0001: the boundary between proposing an action and taking one

| | |
|---|---|
| Status | Draft |
| Updated | 2026-10-31 |
| Owners | Agent runtime, language intelligence, and semantic reasoning |

A warning about what this document is. It describes an intended design, and at the time of writing almost none of it exists: the session lifecycle and the terminal interface are built, and the action boundary, the evidence layer, the reasoner, and the audit record are not. Read it as a statement of what we mean to build and of the distinctions we refuse to give up while building it, and not as a description of the code.

## The decision

In what follows we shall have to keep apart two notions that ordinary usage happily confuses: that an action has been proposed, and that it has been taken. The distinction may look pedantic, and it is not, for a model emits text, and text that happens to describe a patch, a shell command, or a deployment is, considered as text, in no way different from text that describes nothing whatsoever. We therefore represent model output as an `ActionProposal`, by which we mean a request that may well be structurally impeccable and is otherwise entirely without authority.

The authority to convert such a request into an effect rests with the C++ host and with nothing else, and the host provides a single path along which it may be exercised. Along that path it validates the shape of the request against the schema belonging to its kind, resolves the evidence that bears on it, asks the reasoner which decision the current policy yields, discharges the obligations that accompany that decision, derives a capability whose limits name this action and no other, performs the work in a worker holding only what that capability names, and records what it did. Each of these steps may refuse, and when one does, nothing happens and the refusal is written down with its reason.

Because there is exactly one such path, an editor, a command-line session, a continuous-integration worker, and the server we have not yet written are all clients of it, and on the same policy and the same normalized input they reach the same decision. They cannot be promised the same outcome. Builds fail, disks fill, and remote services become unreachable at inconvenient moments; deciding to act and succeeding in acting are different events, and we record them as such.

## What we are after, and what we are not

We want authority to be explicit, narrow, observable, and revocable, and we want the four things that are easiest to confuse — an architectural decision somebody made, an observation some tool produced, a hypothesis the model offered, and a result the policy yielded — to remain four things. We want every language we support to supply evidence through a common interface without being flattened to whatever all of them happen to share. We want semantic reasoning to be deterministic and free of effects. And we want the surface between C++ and OCaml to stay small enough that one person can hold it in mind.

What we are not after deserves equal prominence, because the failure modes of this kind of system come from quietly assuming the opposite. We are not proving programs correct or secure. We are not giving a model raw shell access, a production database, a cloud console, or a credential. We are not treating a well-formed result from constrained decoding as an authorized action, since a schema can tell us that a request is shaped correctly and can tell us nothing at all about whether it should happen. And we are not embedding a language server or a solver by default; both are heavy, and we would rather add one where it earns its weight than carry it everywhere.

## What crosses the boundary

The declarations below are the ones that actually travel between the host, the reasoner, and the executor. Rather than present them as a taxonomy, let us follow one action through them and see what each type is holding at the time.

Suppose the agent intends to change `session.cpp`. Before anything may be said about that file, something must have looked at it, and in Bernstein the looking is done by the C++ language module. What the module returns is not an opinion but an `EvidenceRef`, whose fields are chosen so that each answers a question we shall want to ask later, during an audit, when the circumstances have been forgotten.

```cpp
enum class ProvenanceKind {
    Observed,
    Derived,
    Inferred,
    Proposed,
    Approved,
};

struct EvidenceRef {
    std::string id;
    ProvenanceKind provenance;
    std::string producer;        // compiler, test, language module, human, model
    std::string subject_version; // commit, content hash, or workspace snapshot
    std::string digest;
};
```

`producer` records who looked: a compiler, a test run, a language module, a human, or the model. `provenance` records what sort of knowledge resulted — `Observed` when a tool saw it, `Derived` when we computed it from something observed, `Inferred` when the model guessed, `Proposed` when a person suggested it, `Approved` when a person accepted it. `subject_version` records which snapshot was looked at, and `digest` fixes the content, so that two references either describe the same thing or demonstrably do not.

A correction belongs here, because the enumeration above is tidier than the thing it describes. `ProvenanceKind` answers several questions with a single value, and they come apart under pressure. That a tool observed something and that a person approved it are not two points on one scale: approval establishes authority rather than fact, and a person may perfectly well approve a statement that is false. A derived fact likewise inherits every limitation of the premises it came from, which one label cannot carry. The position we now hold is that provenance is several independent axes — origin, normative approval status, validation status, subject and configuration scope, and freshness — each answered separately for each fact. The declaration above is kept as the original sketch and should be read as one rather than as a settled type.

That third field is doing more work than its size suggests. Because every piece of evidence names the snapshot it came from, a finding obtained on one branch cannot quietly be used to justify an action on another: the two carry different versions, and the mismatch is settled by comparing strings rather than by anybody's judgement.

A `ProjectFact` is then such a reference with a claim attached, and a `DecisionRecord` is a statement about the architecture that someone may have accepted.

```cpp
struct ProjectFact {
    std::string predicate;
    Json value;
    EvidenceRef evidence;
};

struct DecisionRecord {
    std::string id;
    std::string statement;
    std::vector<EvidenceRef> basis;
    ProvenanceKind status; // only Approved carries architectural authority
};
```

Of the five values `status` may take, only `Approved` confers architectural authority, which is why the field is a `ProvenanceKind` rather than a boolean. One might object that a flag would be smaller and simpler. It would, and we would then be unable to distinguish a decision a human signed from one the model proposed and nobody ever read, which is precisely the distinction the whole design exists to preserve.

## Actions, and the absence of a general-purpose one

An action expresses an intent that a person would recognise as such. It is not a command line wearing a struct.

```cpp
enum class ActionKind {
    ReadFile,
    SearchCode,
    InspectSyntax,
    QuerySymbols,
    ApplyPatch,
    RunBuild,
    RunTest,
    FormatFiles,
    InspectTerraformPlan,
    RequestDeployment,
};

struct ActionProposal {
    std::string proposal_id;
    ActionKind kind;
    Json arguments;                   // validates against kind-specific schema
    std::vector<EvidenceRef> basis;
    std::string declared_purpose;     // supplied by user or invoking system, never model-authored
    std::string workspace_snapshot;
};
```

The list contains no `RunShell`, no `RunSql`, and nothing resembling a general cloud-administration action, and the omission is deliberate rather than an oversight to be corrected later. A kind earns its place when we can write four things for it: a parser that understands its arguments, a validator that can reject a malformed or dangerous one, an executor that runs it under a profile, and policy semantics that let a rule say something useful about it. `RunShell` admits no such validator, because its argument is an arbitrary program; a rule about it could say nothing beyond permitting or forbidding everything.

`RunBuild` and `RunTest` are the interesting cases, since both plainly do run arbitrary code. They are admissible because their arguments do not come from the model: the command is resolved from configuration the project owns, and the proposal selects among what that configuration already defines. This is a smaller claim than it may appear. We are not asserting that running the project's own test suite is safe — it executes whatever the repository contains — only that the choice of what to run is not the model's to make, and that execution still happens inside a worker with the limits described below.

`declared_purpose` is supplied by the person or the system that invoked the agent and never by the model. A purpose the model wrote for itself would be a justification rather than evidence, and we would be building the machinery for an action to approve itself.

## What the reasoner is asked, and what it may reply

The reasoner is handed a closed snapshot and nothing else. It cannot consult the filesystem, the clock, an identity provider, or the network, which is what makes its answers reproducible.

```cpp
struct ReasoningRequest {
    std::string policy_digest;
    std::string workspace_snapshot;
    CallerIdentity caller;            // authenticated attributes, not model claims
    ActionProposal action;
    std::vector<ProjectFact> facts;
    std::vector<DecisionRecord> approved_decisions;
};

enum class Decision { Allow, Deny, RequireApproval, Unknown };

struct Obligation {
    std::string kind;                 // e.g. "run_test", "human_review", "network_disabled"
    Json parameters;
};

struct ReasoningResult {
    Decision decision;
    std::string explanation;
    std::vector<std::string> proof_refs;
    std::vector<Obligation> obligations;
    std::vector<EvidenceRef> relied_on;
};
```

Four decisions rather than two, and the fourth is the one that matters. `Unknown` authorizes nothing, exactly as `Deny` authorizes nothing, and yet we insist on recording them differently, because they call for different responses. `Deny` means the policy considered the action and refused it, and the remedy is to propose something else or to change the policy deliberately. `Unknown` means the policy could not be brought to bear at all: evidence was missing, stale, or of a shape the evaluator could not use, or the policy package itself was malformed. Collapsing the two would let a broken evidence pipeline present itself as a well-reasoned refusal, and a system that cannot tell those apart will eventually be trusted for the wrong reason.

The reasoner may evaluate relations in the manner of Datalog for questions of authorization and architectural structure, use ordinary typed predicates where a predicate suffices, and lower a bounded claim to a solver where that is worth doing. Whatever it uses, its explanation must be expressed in terms of policy rules and evidence references. A fluent paragraph from a model is not a proof, and must never be returned in the place where one is expected.

## The single path from proposal to effect

```text
ActionProposal from model or user
  -> structural and schema validation
  -> resolve language-specific evidence
  -> ReasoningRequest / policy decision
  -> satisfy required obligations
  -> create scoped capability
  -> tool-specific validator
  -> isolated execution
  -> collect result evidence and emit audit event
```

The executor owns this sequence. A tool cannot step around it, and the agent loop cannot reach a tool implementation directly; there is one entry point.

```cpp
class ActionExecutor {
public:
    ExecutionResult execute(const ActionProposal& proposal,
                            const RunContext& context);
};
```

`ExecutionResult` distinguishes `Succeeded`, `Denied`, `NeedsApproval`, `ValidationFailed`, `Cancelled`, `TimedOut`, and `ExecutionFailed`. Seven outcomes where many systems have two, and each of the seven asks something different of whoever reads it: a policy denial is a question for the policy, a compiler failure is a question for the code, and a timeout is a question about the world. `TimedOut` is the awkward one, since it tells us that we stopped waiting and tells us nothing about whether the effect occurred. We shall therefore treat it as an unresolved outcome to be reconciled rather than as a failure, and a retry must not assume that the earlier attempt did nothing.

## What an `Allow` actually hands over

An `Allow` does not transfer authority; it permits the executor to mint some. What the executor mints is deliberately narrow:

```text
subject identity
action kind and arguments digest
workspace snapshot and permitted paths
credential reference
network profile
CPU, memory, output, and wall-time limits
not-before and expiry time
policy digest and decision reference
```

The worker receives only what the capability names, and a capability describes one action on one snapshot within one window of time. Note in particular that it holds a credential *reference* and not a credential: the agent having proposed an action is never a reason for a production secret to appear anywhere near it. An action that affects production requires an approval authenticated independently of the agent, and a capability minted for that action and no other.

It is worth being clear about what this does not accomplish. A capability limits what the worker is handed; it does not make the code inside the worker trustworthy, and it does not help if the machine minting capabilities is itself compromised.

## What a language module must be able to say about itself

Every language module implements the same interface, and the interesting method is the one that reports its own limits.

```cpp
class LanguageModule {
public:
    virtual LanguageId language() const = 0;
    virtual CapabilityProfile capabilities() const = 0;
    virtual EvidenceBundle inspect(const WorkspaceSnapshot&, const Query&) = 0;
    virtual ValidationResult validate_patch(const Patch&, const WorkspaceSnapshot&) = 0;
};
```

`CapabilityProfile` states which conclusions the module can actually support — syntax, symbols, diagnostics, refactorings, the build graph, extracted contracts, runtime evidence — or declares itself reduced. The profile is shown to the user and supplied to policy evaluation, so that a rule may decline to rely on an analysis the module cannot perform.

The first four modules are C++, Python, TypeScript, and Go. C++ draws on compile commands, `clangd`, compiler output, build targets, and sanitizer results where available; the others use their own native tooling. The rule that matters here is one of abstention: a module must never report evidence of a kind it cannot produce. An honest gap is useful, and a confident guess dressed as an observation corrupts everything downstream of it.

## Where OCaml stops

OCaml is confined to the semantic reasoner and the compiler for the architecture and policy language. It parses and type-checks that language, compiles declarations into typed rules, evaluates them deterministically, constructs explanations, and where useful lowers a bounded claim to a solver.

Everything effectful belongs to the C++ host. The reasoner runs as an isolated component behind a versioned protocol with generated bindings, and it cannot load a model, start a language server, read a workspace, reach a control plane, obtain a credential, or spawn a process. The purpose of drawing the line here is to keep the formal part of the system small enough to test thoroughly, and a feature should not be allowed to cross it unless it genuinely changes what we mean by architecture, policy, proof, or obligation.

## What must be written down, and what that record does not prove

Every execution attempt is recorded under one stable operation identity, and that identity collects several durable lifecycle events rather than a single summarising one. An earlier version of this document asked for exactly one event per attempt, which was a convenient shorthand and wrong in the case that matters most: a process that dies between dispatch and acknowledgement would produce no record at all. Intent is therefore persisted before anything irreversible is dispatched, so that a crash leaves behind the knowledge that an attempt was begun. The events carry, between them:

```text
event id, request id, actor identity, workspace snapshot, action digest,
policy digest, decision, obligations, evidence references, capability digest,
executor profile, result kind, output digest, timestamps, and parent events
```

Attempts that were denied, that failed validation, or that timed out produce an event on the same terms as attempts that succeeded. A log that records only what worked is a log that cannot answer the question one actually asks after an incident.

Recovery has to cope with what distributed delivery actually does, which means acknowledging gaps, tolerating duplicate events, and reconstructing a lifecycle from what survived. We cannot invent atomicity with an external system by writing a record about it, and a reconciliation step is the price of that.

Ingestion should be authenticated and the events chained or otherwise independently verifiable, so that tampering leaves a trace. That is the whole of the claim, and it is weaker than it sounds: a chained log tells us that records were not altered after the fact, and says nothing about a compromised endpoint, a stolen identity, or an event that was never collected. Those are threat-model questions, and they are not answered by hashing.

## Invariants

1. No model output can directly cause an effect.
2. No effect runs without a structurally valid action and a matching scoped capability.
3. `Deny` and `Unknown` never authorize execution.
4. The reasoner is pure with respect to a request snapshot.
5. Every decision cites a policy digest and the evidence it relied on.
6. Every action attempt is recorded under one operation identity, with intent persisted before irreversible dispatch; denials and failures are recorded on the same terms as successes.
7. A language module reports its capability level rather than simulating semantics it does not have.

## Questions still open

1. Which versioned schema system serves C++ and OCaml without our having to maintain the domain types twice?
2. Which invariant should we enforce first, in order to find out whether any of this is worth the trouble: protected routes, dependency boundaries, migration rules, or infrastructure plans?
3. Which capability deserves attention first on macOS, where development currently happens?
4. Should the reasoner begin life as a local child process or be embedded through an FFI boundary? The answer should come from evidence about packaging, restarts, and debugging, and not from an argument about call overhead.
