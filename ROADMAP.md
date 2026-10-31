# Roadmap

This is the plan as it currently stands, together with the reasoning that puts the items in this order. It is not a commitment and it is not an approved specification. Items will move when evidence says they should, and the order matters more than the contents, for reasons that become clear below.

The milestones are ordered by dependency rather than by date, and no dates appear anywhere in this document. Each one says what it delivers, how we would know it was finished, and what it deliberately leaves alone. A milestone is done when there is evidence for its claims, not when the code has been written.

## Where things stand

| Area | State |
| --- | --- |
| Build, dependencies, quality tooling | Working: CMake presets, a Conan lockfile, clang-format, clang-tidy, cppcheck, pre-commit. |
| Session lifecycle | Working: admission, promotion, operations, execution attempts, live observations, durable events, settlement. |
| Projection contract | Working: immutable shared snapshots, one per change, with a bounded event tail and a replay cursor. |
| Terminal interface | Working: renders lifecycle state, turns keys into typed intents, performs no effects. |
| Headless mode | Working: the same translation and renderer, printing plain text. |
| Tests | Twenty-five: fifteen over the session model, five over key events and intent translation, five over rendering, the headless path, and the terminal adapter. |
| Deterministic session | In memory only. It is the reference for what the lifecycle means, not a feature. |
| Persistence | Nothing. |
| Model provider | Nothing. |
| Workspace effects and containment | Nothing. |
| Project model and evidence | Nothing. |
| OCaml reasoner and protocol | Described in [RFC 0001](docs/rfc-0001-core-interfaces.md), not built. |
| Verification | Nothing. |

## What constrains all of it

The following are commitments rather than aspirations, and work that breaks one of them is wrong even when its tests pass. They are repeated here because the order of the milestones follows from them, and because a plan that lists features without listing its constraints tends to acquire the features and lose the constraints.

People own the goals, the architectural decisions, and the invariants. The agent proposes; it does not quietly redefine something that was decided. An approval is not permission to execute: admission has to bind the particular action, the artifact, the authority, and the state that was required at the time. Evidence that is missing, stale, or of the wrong shape never becomes permission or a successful verification, and a timeout after dispatch never becomes proof that nothing happened. Effects travel through typed, bounded, audited actions carrying scoped capabilities, and builds and tests resolve from configuration the project owns rather than from a command string somebody generated. Finally, a claim about verification states its own strength, because compiling, running one test, running the suite, and having a person look are not the same claim and cannot be substituted for one another.

## Gaps we are carrying

Of the eight entries below, seven are recorded in [docs/tui-results.md](docs/tui-results.md), where the evidence behind each one sits. The eighth concerns continuous integration and has since been closed; it stays here because earlier versions of this document listed it as outstanding. Each gap that remains open names the milestone that answers it.

Replay is a read-only query today, and nothing rebuilds a projection from durable events, which means no test can yet demonstrate the property we care about — that replaying history does not re-run its effects. [M2]

Several declared states are never produced: an input is never rejected or found in conflict, an effect is never observed, a proposal is never admitted, and the difference between steering and queueing delivery does nothing. A state that cannot occur is a state nobody has thought through. [M1, M3]

The interface has no way to select, inspect, approve, or diff anything, which makes it a demonstration of the boundary rather than something a person can decide with. [M1]

`simulate_timeout` exists and no key reaches it, so the unresolved outcome that matters most can only be produced from a test. [M1]

`bernstein_target_warnings` attaches its flags as `INTERFACE`, so a library's own sources compile without the warnings it is supposed to enforce. [M0]

The terminal scenarios have now been run through a `pty` harness with real resize signals, and under GNU `screen`: 39 checks, all passing. What remains unestablished is legibility, other emulators, a non-UTF-8 locale, and restoration as a person experiences it rather than as an escape sequence in a byte stream. [M1]

Scrolling is implemented in the way FTXUI invites and does not scale. The conversation region builds an element per entry and wraps the whole tree in `frame`, which costs a rebuild every frame: measured at a 200x50 viewport, 1.1 ms at 1,000 lines, 5.8 ms at 5,000, 25.6 ms at 20,000, 137 ms at 100,000, against a flat 0.15 ms when only the visible window is built. Harmless at today's handful of entries, disqualifying for a diff view. [M1]

Continuous integration now builds and tests the project, which closes the earlier gap; formatting and clang-tidy remain a pre-commit concern because their output depends on a locally installed LLVM version. [M0]

## M0 — Finish what is already started

Corrections, not new subsystems. They come first because each one is small and each one currently undermines a claim made elsewhere.

- [ ] Fix `bernstein_target_warnings` so a target's own sources are compiled with the warnings, and demonstrate the fix by introducing a deliberate warning in a library source and watching the build fail.
- [x] Run the terminal scenarios and record the environment. Done on 2026-10-31 through a `pty` harness and under GNU `screen`; the remaining emulator and legibility checks are noted in [docs/tui-manual-scenarios.md](docs/tui-manual-scenarios.md).
- [x] Pull the terminal library back behind the renderer, so that turning a keystroke into an intent no longer depends on it. The input library and its tests now link no FTXUI, which the no-interface build checks.
- [ ] Rename the terminal-interface documents, which still read as a spike although the technology question was settled.
- [ ] Decide whether `apps/bernstein-sdlc` and `libs/bernstein-protocol` remain as placeholders or are removed until something needs them. An empty directory that has sat untouched for a while is a statement of intent nobody has reviewed.

Done when the warning fix has a demonstrated failure case and no claim in the documents rests on evidence that was never gathered.

## M1 — Make the interface something a person can decide with

The interface proves the boundary holds. It does not let anyone look at a proposal and form a view about it, which is the activity the whole product is supposed to support.

- [ ] Navigation across projects, modules, evidence, proposals, and candidates, with selection remaining local to the interface.
- [ ] A proposal view showing its goal, the evidence behind it, the assumptions it rests on, the obligations attached, and what remains uncertain.
- [ ] A diff view for a candidate change, rendering only the visible window rather than building an element per line. The measurement above is the reason: the idiomatic approach costs 25 ms per frame at 20,000 lines and 137 ms at 100,000, which is a redraw on every keystroke.
- [ ] An approval interaction that looks nothing like ordinary navigation, emits a typed intent, and cannot acquire a capability from a rendering callback.
- [ ] Produce the states that are currently unreachable: rejected and conflicting inputs, observed effects, admitted proposals.
- [ ] A key that triggers a simulated timeout, so an unresolved outcome can be produced by hand.
- [ ] A help view listing the keys that exist, since the footer currently advertises one that does not.
- [ ] A minimum-size policy, decided rather than inherited, and a defined way for the layout to degrade above it.
- [ ] Scrollback through history that does not lose the newest entry.
- [ ] Simulated updates that keep out of the way of typing as the event volume grows.

Done when each view has a deterministic rendering test, an approval intent is shown by test to change no effect state, the manual scenarios have been run again, and no view can render an unknown outcome as a success or as a blank.

Not in this milestone: real model output, real diffs, or any real approval authority.

## M2 — Persistence, and replay that proves something

The lifecycle speaks of durable events while nothing is durable, which is the largest gap between what the model claims and what the code does.

- [ ] An append-only event store with stable sequencing.
- [ ] Rebuilding a projection from replayed events alone, with no live session involved.
- [ ] Reconnection from a cursor that restores durable state without re-running a single effect.
- [ ] A decision about which observations are deliberately live-only and unreplayable, carried in the projection so the interface can say so.
- [ ] Recovery of a session whose process died mid-operation, leaving anything dispatched and unsettled explicitly unknown.
- [ ] Recording intent before irreversible dispatch, under one operation identity.

Done when a projection rebuilt from replay equals the live projection for the same events, a test shows replay invoking no effect a second time, and a killed and restarted session reports unknown rather than guessing either way.

## M3 — A model provider

The first point at which the system stops being deterministic, which is why it comes after the machinery for recording what happened.

- [ ] A provider boundary that produces lifecycle records rather than text: attempts, recorded tool calls, live deltas, turn settlement.
- [ ] Streaming output shown as live observation, with the live and durable distinction preserved on screen.
- [ ] Provider completion, tool settlement, turn settlement, and operation settlement kept apart, since they are four events and conflating them is how a system comes to report success early.
- [ ] Cancellation of a call in flight that reports an unknown outcome honestly.
- [ ] Credentials from the environment, never the repository, with a `.env.example` carrying placeholders.
- [ ] A decision on the first boundary: a completed turn in batch, or concurrent streaming with tool work, with the recovery cost of the second stated plainly.

Done when the tests run against recorded fixtures and make no network calls, a cancelled call never reports a settled effect, and no credential appears in a projection, a log, or an event.

Not in this milestone: letting provider output change files or run commands.

## M4 — Effects, capabilities, and containment

The first milestone that can damage something, which is the reason it sits behind persistence and honest unknown states rather than in front of them. [RFC 0001](docs/rfc-0001-core-interfaces.md) describes the intended shape in detail.

- [ ] Typed, bounded action proposals for reading and changing files.
- [ ] A capability minted per admitted action, enforced by the executor, carrying its obligations.
- [ ] `RunBuild` and `RunTest` resolved from configuration the project owns.
- [ ] Execution in a restricted worker, since a configured build still runs whatever the repository contains.
- [ ] Retry and recovery that neither broaden authority nor assume the earlier attempt did nothing.
- [ ] An audit record binding each effect to its action, artifact, authority, and the state required when it was admitted.

Done when a rendering callback demonstrably cannot obtain a capability, an action attempted outside its scope is refused by the executor rather than only by the policy layer, and a dispatched command that times out produces an unknown outcome with a path to reconcile it.

## M5 — The project model and the evidence layer

What would make the agent architecture-aware rather than merely file-aware.

- [ ] A shared, versioned model holding goals and scope, architectural decisions and their rationale, invariants, boundaries, ownership, modules, dependencies, artifacts, symbols, tasks, branches, and changes.
- [ ] Evidence carrying provenance, scope, coverage, and freshness.
- [ ] Coverage gaps and unknowns as values in their own right, distinct from a negative result.
- [ ] Competing proposals and open questions held without forcing a resolution, since premature resolution is how a design loses the objection that was right.
- [ ] Queries usable by the interface and the reasoner alike.

Done when stale or missing evidence cannot be rendered or recorded as a successful verification, and a design change can be represented as a proposal naming its alternatives, its consequences, the objects it affects, and the approval it requires.

## M6 — The reasoner

Builds what `ocaml/` and `protocol/` currently only describe.

- [ ] A language for architectural and policy constraints, parsed and type-checked.
- [ ] Evaluation over a closed, versioned snapshot of facts and approved decisions.
- [ ] Decisions returned with obligations, rule references, and the evidence relied on.
- [ ] A versioned protocol carrying only closed inputs: no path the reasoner may read, no credential, no means of causing an effect.
- [ ] Validation on the host of every response before it is used to mint a capability.
- [ ] A transport and a schema format chosen, with the reasoning recorded.

Done when the reasoner's build gives it no filesystem, network, or process authority, an `Allow` is demonstrably insufficient to execute anything, and identical closed inputs produce identical decisions.

## M7 — Verification that means something

The project's central claim, and the one most easily faked.

- [ ] Test selection determined by fully declared inputs: the workspace snapshot, the normalized change, project-owned configuration, the affected modules and their dependency graph, the language capability profile, applicable policy, and tool versions.
- [ ] Evidence bound to the exact artifact, command definition, selected tests, tool versions, captured output, and declared coverage.
- [ ] A test change treated as a change to the verification contract, so that a test modified after a failure requires review of its intent and scope before it counts as evidence for the original behaviour.
- [ ] An unestablished test universe represented as a coverage gap, or escalated to a broader configured check.
- [ ] First-class support for C++, Python, TypeScript, and Go, with unsupported analysis exposed as a gap rather than passed off as equivalent assurance.

Done when identical inputs reproduce the same selection, no path exists by which the model decides what counts as sufficient verification, and mutating a fix is caught by the test that claims to cover it.

## M8 — Getting it to other people

- [ ] Linux as a supported development and build platform, not only macOS with Homebrew.
- [ ] Release artifacts with build provenance.
- [ ] Terminal portability evidence across emulators and multiplexers, recorded as the sample it is rather than generalized into a claim.
- [ ] Installation that does not require the whole development toolchain.

Done when a release can be reproduced from a recorded configuration and every portability claim names the environments actually tried.

## Questions still open

These want a decision from a person and will not be settled by writing code.

1. Which state deserves to be permanently visible in the interface, and which belongs behind a detail view?
2. Is a conversation with a composer the right primary surface for an agent meant to be argued with, or does it encourage accepting output instead of reviewing a proposal?
3. What is the minimum terminal size worth supporting, and what should happen below it?
4. Is a completed turn the right first boundary for tool execution, or is concurrent streaming worth what it costs in recovery?
5. Which lifecycle records must survive a restart for the first local promise to hold?
6. JSON or a binary schema system between host and reasoner?
7. How much of the project model has to exist before the reasoner earns its place rather than merely occupying one?

## How the work is organized

Small self-contained changes, each stating what was verified and what was not. Documents keep apart what was decided, what was synthesized, what was observed, and what remains open. A results document records a decision with an owner and a date; anything the agent suggests stays labelled as a suggestion.
