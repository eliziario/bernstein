# Bernstein

Bernstein is a coding agent built for advanced developers for architecturally relevant work. It is intended to act as a pair programmer to the developer, instead of a task-executing subordinate. By design the model's text is not authority: an effect happens only where policy written as code permits that exact action, and what was permitted, on what evidence, is recorded.

The code is in its early stage. The session lifecycle and the terminal interface exist and are tested; nothing is connected to a model provider, and nothing touches your files. What you can run today is an architectural skeleton with an interface attached, which is more useful to read than to use. [ROADMAP.md](ROADMAP.md) says what is planned, in what order, and why that order.

## What it is trying to do

Two concerns carry equal weight. One is the working lifecycle: the review of an agent's work should happen while the work is happening, the way it does when two people share a keyboard, rather than afterwards as a diff somebody has to read. The other is that no tool call should run merely because a model asked for it; an effect should pass a gate made of rules that are code, type-checked, and evaluated the same way every time. Two further goals follow from those. The agent builds the checks for its own change, and something other than the agent decides whether they were enough. And the decisions reached along the way stay on the record, with their rationale and the alternatives that were rejected.

### Pairing instead of reviewing

Working with a coding agent usually means describing a task, waiting, and reading a diff. The reading is where the cost is. Whoever wrote the change knew which alternatives were considered, which files were opened and discarded, which function was left alone deliberately; a patch records none of this, so the reader reconstructs it from the result. This reconstruction is what people mean by the review tax. It is slow, it is approximate, and it grows with the size of the change.

Extreme programming dealt with the problem by not letting the work accumulate. The second person in a pair watches the decision being made, raises the objection while it still costs a sentence, and reconstructs nothing, having been present.

Bernstein tries to arrange an agent session the same way. A decision is stated before the edit that follows from it. Intent and evidence are attached to the change instead of being left in a transcript. Approvals happen where the question is still small, which is also what keeps the admission step described below from becoming a prompt that appears every few seconds.

This is why the session model separates an admitted input from a promoted one, a pending operation from a settled one, and an intended effect from an observed one. Those are the distinctions the second person would otherwise have to recover by reading, and they are the part that already exists.

### Tool calls that pass a policy gate

In the usual arrangement a model emits text describing a tool call and a loop runs it, which makes the text the authority. A shell tool accepts any program, so a rule about it says nothing finer than yes or no. The arguments need not come from the person's request: a file the model read, an issue, a dependency's README, or a web page can supply them, and a transcript does not distinguish those origins. When it goes wrong, the transcript is also all that remains.

Bernstein treats model output as an `ActionProposal` — a request that may be structurally impeccable and carries no authority whatever. The host owns the only path from a proposal to an effect. Along it, the host validates the request against the schema for its kind, resolves the evidence bearing on it, asks the reasoner what current policy yields, discharges the obligations attached to that decision, mints a capability whose limits name this action and no other, runs the work in a worker holding only what the capability names, and records what happened. Any step may refuse, and a refusal is written down with its reason. The capability carries a credential reference and not a credential.

The set of action kinds is short and contains no `RunShell`, no `RunSql`, and nothing resembling cloud administration. A kind becomes admissible when four things can be written for it: a parser for its arguments, a validator that rejects a malformed or dangerous one, an executor profile it runs under, and policy semantics that let a rule say something useful about it. `RunShell` has no possible validator, its argument being an arbitrary program.

`RunBuild` and `RunTest` do execute arbitrary code, and they are admissible for a narrower reason: their arguments are not the model's. The command is resolved from configuration the project owns, and the proposal selects among what that configuration already defines. Running a repository's own test suite is not safe, since it runs whatever the repository contains and the build configuration itself may be hostile. The claim is only that the selection is outside the model's control; containment is a separate obligation belonging to the worker profile. A list of permitted tool names is not a sandbox.

### Policy written as code

The rules live in a language that is parsed and type-checked, not in a prompt. The reasoner evaluates them over a closed, versioned snapshot — the workspace, the project's build and test configuration, the declared constraints and invariants, the decisions a person has approved — and returns `Allow`, `Deny`, `RequireApproval`, or `Unknown`, with the obligations that come with it and references to the rules and evidence that produced it. The explanation is in those terms; a fluent paragraph from a model is not a derivation and is not accepted where one is expected.

Relations in the manner of Datalog carry the questions that are naturally recursive: whether a module transitively depends on one it is forbidden to reach, whether a permission arrives through a chain of delegations. Typed predicates serve where a predicate suffices, and a bounded claim can be lowered to an SMT solver where that earns its weight. Datalog rather than Prolog, because the termination properties differ: pure positive Datalog over a finite universe terminates, and Prolog's search strategy offers no such guarantee. Those properties are also easy to lose, since arithmetic extensions can permit nontermination, as [Souffle documents](https://souffle-lang.github.io/tutorial), so the permitted fragment and the resource budget have to be stated rather than assumed. Negation and aggregation need stated semantics for a related reason: positive Datalog is monotone, and once negation or changing facts are admitted, adding a fact can withdraw an earlier `Allow`, which makes invalidation and re-evaluation part of the design rather than an afterthought.

Determinism here means that the same complete semantic inputs yield the same semantic result, so a decision can be re-derived next month and argued about on its inputs. It does not mean the rules are sound, and it does not mean they capture the requirement anyone had in mind. Those are three claims and the system keeps them apart.

`Allow` is a policy result and not a capability. `Unknown` authorizes nothing, and is recorded separately from `Deny` because the two call for different responses: `Deny` means policy was brought to bear and refused, while `Unknown` means it could not be brought to bear at all, the evidence being missing, stale, or of an unusable shape. A system unable to tell those apart will eventually trust a broken evidence pipeline that presents itself as a well-reasoned refusal.

### Verifiers the agent writes

An agent proposes checks readily. Give it a function and a claim about the function and it produces properties, cases, assertions, and invariants to state in the architecture language, including for code nobody had covered. The intention is to use that: let the agent build verifiers for the change in front of it, new ones included, and run them.

Deciding that those verifiers were sufficient is a different job and not the agent's. Which checks are required follows from declared inputs — the snapshot, the normalized change, project-owned configuration, the affected modules and their dependency graph, the language module's capability profile, applicable policy — so the selection is reproducible and auditable rather than chosen by the model partway through a task. A gap in coverage is recorded as a gap. A test edited after it failed does not resume standing as evidence for the behaviour it checked before the edit; editing a test changes the verification contract and is reviewed on those terms.

### Decisions that stay on the record

The project model is versioned and shared between people and agent sessions: the goals and current scope, the architectural decisions and why they were made, the invariants and boundaries and who owns them, the modules and dependencies and tasks and changes, the evidence gathered, and the questions still open. An agent may maintain it and query it. An agent is not its authority.

The practical effect is that an agent cannot quietly redefine something that was decided. A design change is a proposal, and a proposal names the alternatives, the consequences, the objects it affects, and the approval it needs. This is the lightest of the four goals in terms of mechanism and the easiest to lose, since nothing in a transcript objects when a rationale goes missing.

## Where the shape comes from

A generative part that proposes and a separate, non-generative part that constrains is not our idea, and the arguments for it are better repeated as the people who made them put them.

LeCun's objection to autoregressive models is that a model emitting one token at a time, each conditioned on its own previous output, has nothing in it that compares the result against an objective, and that errors therefore accumulate along the sequence. Scale does not supply the missing piece. His remedy is structural: the part that proposes and the part that optimises are separate parts, and the system searches for an output satisfying stated objectives. His own version of that optimisation is learned and differentiable — he has no interest in a logic engine.

Marcus argues the hybrid case directly. Pattern completion and symbol manipulation do different work, and a dependable system needs both.

Bernstein takes the division of labour from the first argument and builds it from the materials of the second: logic programming for the relations, types for the rules, and a host that mediates effects. The model proposes. A symbolic reasoner over versioned, closed inputs says what the architecture and the policy allow. The C++ host carries out effects and mediates them. The constraint layer is a type-checked policy language rather than an energy function, which gives up generality in exchange for something a person can read and contradict.

## Who might want this

Someone who has watched an assistant report success for work it never ran, and who would rather have a tool that distinguishes a proposal from an approval, and an approval from something that actually happened. Someone unwilling to hand an agent a shell on a machine that matters, and unsatisfied by an allowlist of tool names. Someone who needs to answer, later, why a change was made, what permitted it, and what evidence supported it. It is not yet a tool you would use to get work done, and it will say so rather than pretend otherwise.

## What exists

The session model keeps apart the states that are easy to conflate: an input that has been admitted, an input that has become visible to the model, an operation that is pending, running, interrupted, of unknown outcome, or settled, and an effect that is intended, recorded, observed, or unknown. The interface renders those states and never performs an effect itself; a keystroke becomes a typed intent, and only the session changes anything.

Snapshots given to the interface are immutable and shared. The session publishes a new one after each change, carrying the most recent durable events and a cursor; the full history is read back through that cursor rather than copied into every frame.

Cancellation respects what is actually known. Cancelling before anything was dispatched is a known outcome, since no effect was attempted. Cancelling after dispatch is not, and the operation says so instead of claiming it stopped. Reconciliation applies only to operations in that unknown state, and it never invents a result it did not observe.

A keystroke becomes a Bernstein key event before it becomes anything else, so the path from input to intent knows nothing about the terminal library: the library that owns it links no FTXUI, and a build with the interface switched off contains no reference to FTXUI at all. There is also a headless mode that drives the same session through the same translation and prints plain text, so the behaviour can be checked without a terminal. Twenty-five tests cover the lifecycle transitions, the snapshot contract, key translation, rendering, and the headless path.


## Built with

| Area | Choice |
| --- | --- |
| Language | C++23, which hosts anything effectful |
| Build | CMake 3.28 or newer with presets, Ninja |
| Dependencies | Conan 2, pinned by `conan.lock` |
| Terminal interface | FTXUI 7.0.3 |
| Tests | Catch2 3 under CTest |
| Libraries | fmt, spdlog, nlohmann_json, CLI11 |
| Reasoning layer | OCaml with Dune, currently a package skeleton |
| Tasks | just |
| Quality | clang-format, clang-tidy, cppcheck, cmake-format, pre-commit |

C++ hosts the parts that cause effects. OCaml is confined to architecture and policy semantics behind a versioned contract, which keeps the formal part of the system small enough to test properly.

## Building it

You need a C++23 compiler, CMake 3.28 or newer, Ninja, Conan 2, and Python 3 for the formatting tools. clang-tidy is needed only for the `ci` preset.

On macOS one script installs the toolchain through Homebrew:

```bash
./scripts/bootstrap-dev.sh
```

On Linux, install the equivalents of what [Brewfile](Brewfile) lists, then `pip install conan cmakelang`.

```bash
just setup
just configure
just build
just test
```

`just configure` resolves the pinned dependencies and generates a Ninja build under `build/dev`. `just quality` formats, lints, and tests. `just reasoner-build` builds the OCaml package once its opam dependencies are present.

The same thing without `just`:

```bash
conan install . --lockfile=conan.lock --output-folder build/conan/dev --build missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset dev -DCMAKE_TOOLCHAIN_FILE="$PWD/build/conan/dev/build/Debug/generators/conan_toolchain.cmake"
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

## Running it

```bash
build/dev/apps/bernstein-agent-tui/bernstein-agent-tui
```

Type something and press Enter. The entry appears marked as admitted but not yet visible to the model, with an operation beside it in the pending state. A moment later the input is promoted, the operation starts running, and then settles, and each of those transitions adds a durable event to the activity region with its sequence number. Submit a second request while the first is still working and it waits: operations run one at a time, which is a simplification the roadmap intends to revisit rather than a property worth keeping.

Press Ctrl+X while an operation is running and the result is instructive. The operation does not report that it was cancelled, because the host cannot know that; it reports an unknown outcome, in red, and Ctrl+R then records that reconciliation found nothing to resolve it.

| Key | Effect |
| --- | --- |
| Enter | Submit the draft as an input |
| Backspace | Delete one character, counted in code points rather than bytes |
| Esc or Ctrl+U | Clear the draft |
| Ctrl+X | Cancel the oldest active operation |
| Ctrl+R | Reconcile the newest operation of unknown outcome |
| Ctrl+C | Quit |

Below 60 by 20 the interface stops drawing the layout and says the terminal is too small, on the grounds that a clipped view of lifecycle state is worse than no view.

For a snapshot without a terminal:

```bash
printf 'first request\nsecond request\n' | build/dev/apps/bernstein-agent-tui/bernstein-agent-tui --headless
```

It reads lines, runs them through the same translation the keyboard uses, advances the simulation until nothing is active, and prints plain text with no escape sequences.

To build without the interface and its dependency:

```bash
cmake --preset dev -DBERNSTEIN_BUILD_TUI=OFF
```

## Layout

```text
apps/bernstein-agent/      Command-line entry point, currently a placeholder.
apps/bernstein-agent-tui/  The interface: rendering, the loop, the terminal-event adapter.
apps/bernstein-sdlc/       Reserved for a later lifecycle-coordination executable.
libs/bernstein-core/       Session lifecycle, facade, and projection.
libs/bernstein-input/      Key events and the translation from a keystroke to a typed intent.
libs/bernstein-protocol/   Reserved for the host and reasoner boundary types.
ocaml/                     Architecture and policy reasoner, a skeleton.
protocol/                  The boundary between host and reasoner, described.
docs/                      Design, plan, results, scenarios, and RFC 0001.
tests/                     Catch2 tests for the model and the interface.
```

## Documents

- [RFC 0001](docs/rfc-0001-core-interfaces.md) is the one to read first. It describes the boundary between proposing an action and taking one, which is the reason the rest of the code is shaped as it is. Almost none of it is built yet, and it says so.
- [The terminal interface](docs/terminal-interface.md): the boundary it enforces, the rules its rendering obeys, and the measured limit on scrolling. The [plan it was built from](docs/terminal-interface-plan.md) is kept as a record.
- [Results and decision record](docs/tui-results.md), which carries the evidence behind the claims above along with its own limits.
- [Manual terminal scenarios](docs/tui-manual-scenarios.md), for the behaviour a test cannot establish.
- [ROADMAP.md](ROADMAP.md).

## On the claims in this repository

Compiling, passing one test, passing the configured suite, a scripted run in a pseudo-terminal, and a person actually looking at the screen are five different claims, and the results document says which of them was made. Where something was not checked it is recorded as a gap rather than described as working. This is tiresome to write and it is the only way the evidence is worth anything.

## Contributing

The roadmap is the current plan and its open questions are genuinely open. Small self-contained changes that state what was verified are the easiest to review.

## License

Apache License 2.0. See [LICENSE](LICENSE).
