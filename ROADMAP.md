# Bernstein Roadmap

This is the current plan and the reasoning behind its order. It is not a commitment, and it is not an approved specification. Items move when evidence says they should.

Milestones are ordered by dependency, not by date. Each one states what it delivers, how it will be checked, and what it deliberately leaves out. A milestone is done when its acceptance criteria have evidence, not when the code exists.

## Where the project stands

| Area | State |
| --- | --- |
| Build, dependencies, quality tooling | Working. CMake presets, Conan lockfile, clang-format/clang-tidy/cppcheck, pre-commit. |
| Session lifecycle model | Working. Input admission, promotion, operations, execution attempts, live observations, durable events, settlement. |
| UI projection contract | Working. Immutable shared snapshots, one per state change, with a bounded durable-event tail and a replay cursor. |
| Terminal interface | Working. Renders lifecycle state, translates keys to typed intents, performs no effects. |
| Headless mode | Working. Same translation and renderer, plain-text output. |
| Automated tests | 23 passing, covering the model, the snapshot contract, key translation, rendering, and headless behavior. |
| Deterministic session | In-memory only. It is the reference model for lifecycle semantics, not a product feature. |
| Persistence | Not started. |
| Model provider | Not started. |
| Workspace effects and containment | Not started. |
| Project model and evidence layer | Not started. |
| OCaml reasoner and host protocol | Documented, not implemented. |
| Verification engine | Not started. |

## Constraints that shape every milestone

These are design commitments, not aspirations. Work that violates one is wrong even if it passes tests.

- People own goals, architectural decisions, and invariants. The agent proposes; it does not silently redefine approved decisions.
- An approval is not an execution grant. Admission must bind the actual action, artifact, authority, and required state.
- Unknown, missing, or stale evidence never becomes permission or a successful verification result.
- A timeout after dispatch does not prove that no effect occurred.
- Effects pass through typed, bounded, audited actions with scoped capabilities. Builds and tests resolve from project-owned configuration, never from an arbitrary command string.
- Verification claims name their own strength. Compilation, one test, the configured suite, and human review are different claims.

## Known gaps carried from the current implementation

The first six are recorded in [docs/tui-results.md](docs/tui-results.md). Each is addressed by the milestone noted.

- Replay is a read-only query. Nothing rebuilds a projection from durable events, so no test can yet show that replay avoids re-running effects. (M2)
- `Rejected`, `Conflict`, `Observed`, and `Admitted` states, and the steer/queue delivery distinction, are declared but never produced. (M1, M3)
- The interface has no selection, inspection, approval, or diff views. (M1)
- `simulate_timeout` exists but no key triggers it. (M1)
- `bernstein_target_warnings` attaches flags as `INTERFACE`, so a library's own sources compile without them. (M0)
- The manual terminal scenarios have never been run in a human-operated terminal. (M1)
- There is no continuous integration that builds and tests the project. (M0)

## M0 — Close the gaps in what already exists

Small, immediate corrections. No new subsystems.

- [ ] Fix `bernstein_target_warnings` so a target's own sources get the warning flags, not only its consumers. Verify by introducing a deliberate warning in a library source and confirming the build fails.
- [ ] Add a build-and-test workflow for pushes and pull requests, using the project's own presets.
- [ ] Run the manual terminal scenarios and record the environment, as the scenarios document requires.
- [ ] Rename the terminal-interface documents so they no longer read as a spike, since the technology decision is made.
- [ ] Remove the reference to a non-public RFC from `protocol/README.md`, or restate the boundary in the public document.
- [ ] Decide whether `apps/bernstein-sdlc` and `libs/bernstein-protocol` stay as placeholders or are removed until needed.

**Acceptance:** continuous integration is green on a pull request, the warning-flag fix has a demonstrated failure case, and no public document points at material that is not published.

## M1 — Make the terminal interface a usable product surface

The interface currently proves the boundary. It does not yet let a person inspect or decide anything.

- [ ] Navigation: move between projects, modules, evidence items, proposals, and candidates. Selection stays local to the interface.
- [ ] Detail views: a proposal with its goal, evidence, assumptions, obligations, and remaining uncertainty.
- [ ] A candidate diff view.
- [ ] An approval interaction that is visually distinct from ordinary navigation, emits a typed intent, and never acquires an effect capability from a display callback.
- [ ] Produce the declared-but-unused states: rejected and conflicting input, observed effects, admitted proposals.
- [ ] A key for simulated timeout, so unresolved outcomes can be exercised by hand.
- [ ] A help overlay listing the keys that exist.
- [ ] Decide and enforce a minimum terminal size policy, and define how the layout degrades above it.
- [ ] Scrollback through conversation history without losing the newest entry.
- [ ] Keep simulated updates from blocking input as the event volume grows.

**Acceptance:** deterministic rendering tests for each new view; a test that an approval intent alone changes no effect state; the manual scenarios re-run and recorded; no view that can render an unknown outcome as success or as blank.

**Not in this milestone:** real model output, real diffs, real approval authority.

## M2 — Persistence and replay

The lifecycle model claims durable events. Nothing is durable yet.

- [ ] An append-only durable event store with stable sequencing.
- [ ] Rebuild a `UiProjection` from replayed events alone, independent of the live session.
- [ ] Reconnect from a cursor: replay durable semantic state without re-running any effect.
- [ ] Define which observations are deliberately live-only and never replayable, and carry that in the projection.
- [ ] Recover a session whose process died mid-operation, leaving dispatched-but-unsettled operations explicitly unknown.
- [ ] Record intent before irreversible dispatch, under one operation identity.

**Acceptance:** a projection rebuilt from replay equals the live projection for the same event sequence; a test proves a replay invokes no effect a second time; a killed-and-restarted session reports unknown rather than settled or failed.

## M3 — Model provider adapter

First point where the agent stops being deterministic.

- [ ] A provider boundary that produces lifecycle records: provider attempts, recorded tool calls, live deltas, turn settlement.
- [ ] Streaming output rendered as live observation, with the durable/live distinction preserved.
- [ ] Separate provider completion, tool settlement, turn settlement, and operation settlement.
- [ ] Cancellation of an in-flight provider call that reports an unknown outcome honestly.
- [ ] Credentials from the environment, never from the repository. Ship a `.env.example` with placeholders.
- [ ] Decide the first boundary: a completed-turn batch, or concurrent streaming with tool work, with the recovery cost stated.

**Acceptance:** tests run against recorded fixtures and make no network calls; a cancelled call never reports a settled effect; no credential appears in a projection, a log, or an event.

**Not in this milestone:** workspace edits or command execution driven by provider output.

## M4 — Workspace effects, capabilities, and containment

The first milestone that can damage something. It comes after persistence and honest unknown states on purpose.

- [ ] Typed, bounded action proposals for reading and changing files.
- [ ] Scoped capabilities minted per admitted action, enforced by the executor, with obligations attached.
- [ ] `RunBuild` and `RunTest` resolved from project-owned configuration.
- [ ] Execution in a restricted worker, since configured builds and tests still run repository code.
- [ ] Retry and recovery that never broaden authority and never assume exactly-once external execution.
- [ ] An audit record binding each effect to its action, artifact, authority, and the state required when it was admitted.

**Acceptance:** a display callback cannot obtain a capability; an action executed outside its scope is refused by the executor, not only by the policy layer; a dispatched-then-timed-out command yields an unknown outcome and a reconciliation path.

## M5 — Project model and evidence layer

What makes the agent architecture-aware rather than file-aware.

- [ ] A shared, versioned project model: goals, scope, architectural decisions and rationale, invariants, boundaries, ownership, modules, dependencies, artifacts, symbols, tasks, branches, changes.
- [ ] Evidence records with provenance, scope, coverage, and freshness.
- [ ] Coverage gaps and `Unknown` as first-class values, distinct from a negative result.
- [ ] Competing proposals and unresolved questions held without forcing a resolution.
- [ ] Queries the interface and the reasoner can both use.

**Acceptance:** stale or missing evidence cannot be rendered or recorded as successful verification; a design change is representable as a proposal naming alternatives, consequences, affected objects, and the approval required.

## M6 — Architecture and policy reasoner

Implements what `ocaml/` and `protocol/` currently only describe.

- [ ] An intent language for architectural and policy constraints, parsed and type-checked.
- [ ] Evaluation over a closed, versioned snapshot of facts and approved decisions.
- [ ] Decisions returned with obligations, rule references, and the evidence relied on.
- [ ] A versioned request/response protocol carrying only closed inputs: no paths the reasoner can read, no credentials, no capability to cause effects.
- [ ] Host-side validation of every response before it is used to mint a capability.
- [ ] Choose the transport and the schema format, and record why.

**Acceptance:** the reasoner has no filesystem, network, or process authority in its build; an `Allow` is demonstrably insufficient to execute anything; identical closed inputs produce identical decisions.

## M7 — Deterministic verification

The project's central claim is that verification means something specific.

- [ ] Deterministic test selection from fully declared semantic inputs: workspace snapshot, normalized change, project-owned configuration, affected modules and dependency graph, language capability profile, applicable policy, tool versions.
- [ ] Verification evidence bound to the exact artifact, command definition, selected tests, tool versions, captured output, and declared coverage.
- [ ] Treat a test change as a change to the verification contract, with a test modified after a failure requiring explicit review.
- [ ] Represent an unestablished test universe as a coverage gap, or escalate to a broader configured check.
- [ ] First-class language support for C++, Python, TypeScript, and Go, exposing unsupported analysis as a gap rather than equivalent assurance.

**Acceptance:** identical semantic inputs reproduce the same selection; no path exists by which the model chooses what counts as sufficient verification; mutation of a fix is caught by the test that claims to cover it.

## M8 — Distribution

- [ ] Linux as a supported development and build platform, not only macOS with Homebrew.
- [ ] Release artifacts with build provenance.
- [ ] Terminal portability evidence across emulators and multiplexers, with the sample recorded rather than generalized.
- [ ] Installation that does not require the full development toolchain.

**Acceptance:** a release is reproducible from a recorded configuration, and portability claims name the environments actually tested.

## Open questions

These need a human decision and are not settled by implementation.

1. Which state deserves permanent visibility in the interface, and which belongs behind a detail view?
2. Is the conversation-plus-composer metaphor the right primary surface for a design-first agent, or does it bias users toward accepting output instead of reviewing proposals?
3. What is the minimum supported terminal size, and what degradation is acceptable below it?
4. Is a completed-turn batch the right first tool-execution boundary, or is concurrent streaming worth the recovery cost?
5. Which lifecycle records must survive a restart for the first local promise to hold?
6. JSON or a binary schema system for the host/reasoner protocol?
7. How much of the project model must exist before the reasoner is useful rather than ceremonial?

## How work is organized

- Small, self-contained changes, each stating what was verified and what was not.
- Documents separate explicit decisions from synthesis, observations, and open questions.
- A results document records decisions with an owner and a date; recommendations stay labelled as recommendations.
