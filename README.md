# Bernstein

Bernstein is an architecture-aware coding agent for teams that want people to keep ownership of product direction and system design while delegating bounded investigation, implementation, and verification.

The idea it is built around: the useful unit of collaboration is not a diff. It is a decision, the design objects it affects, the implementation, the evidence that it works, and the uncertainty that remains. Bernstein is meant to keep those things distinct instead of collapsing them into a chat transcript where a plausible suggestion and a verified result look the same.

**Status: early. The terminal interface and the session lifecycle model are real and tested. Nothing is connected to a model provider, and nothing touches your workspace yet.** See [ROADMAP.md](ROADMAP.md) for what is planned and in what order.

## Who it is for

- Developers who want an agent that distinguishes a proposal from an approval, and an approval from an executed effect.
- Teams who need an audit trail of decisions and evidence, not just a record of edits.
- Anyone who has had an assistant report success for work it never verified.

It is not yet useful as a daily driver. Today it is a working architectural skeleton you can run and read.

## What works today

- A terminal-independent session lifecycle model that keeps admission, promotion to model-visible history, operations, execution attempts, live observations, durable events, and settlement as separate states.
- Immutable shared UI snapshots, published once per state change, carrying a bounded tail of recent durable events. Full history is read back through a replay cursor.
- A terminal interface built on FTXUI that renders those states and never performs effects itself: keys become typed intents, and only the session changes state.
- Cancellation and reconciliation that respect what is actually known. Cancelling before dispatch is a known outcome; cancelling after dispatch is explicitly unknown, and reconciliation never invents a result.
- A headless mode that drives the same session through the same key translation and prints a plain-text snapshot, so the behavior can be checked without a terminal.
- 23 tests covering lifecycle transitions, the snapshot contract, key translation, rendering, and headless behavior.

## What does not work yet

- No model provider. There is a deterministic in-memory session used as the reference model for lifecycle semantics.
- No workspace changes, no command execution, no sandboxing, no capability enforcement.
- No persistence. Everything is lost when the process exits.
- No navigation, inspection, approval, or diff views in the interface.
- The OCaml reasoner and the C++/OCaml protocol are described but not implemented.

## Built with

| Area | Choice |
| --- | --- |
| Language | C++23 (the operational host) |
| Build | CMake 3.28+ with presets, Ninja |
| Dependencies | Conan 2, pinned by `conan.lock` |
| Terminal UI | FTXUI 7.0.3 |
| Tests | Catch2 3 with CTest |
| Libraries | fmt, spdlog, nlohmann_json, CLI11 |
| Reasoning layer | OCaml with Dune (package skeleton) |
| Tasks | just |
| Quality | clang-format, clang-tidy, cppcheck, cmake-format, pre-commit |

C++ is the host because it owns effects and mediation. OCaml is kept narrowly scoped to architecture and policy semantics behind a versioned contract.

## Getting started

Requirements: a C++23 compiler, CMake 3.28+, Ninja, Conan 2, and Python 3 for the formatting tools. Building the terminal interface also needs clang-tidy only when you use the `ci` preset.

On macOS, one script installs the whole toolchain through Homebrew:

```bash
./scripts/bootstrap-dev.sh
```

On Linux, install the equivalents of [Brewfile](Brewfile) with your package manager, then `pip install conan cmakelang`.

Build and test:

```bash
just setup
just configure
just build
just test
```

`just configure` resolves the pinned Conan dependencies and generates a Ninja build under `build/dev`. `just quality` formats, lints, and tests. `just reasoner-build` builds the OCaml package once opam dependencies are installed.

Without `just`, the same thing by hand:

```bash
conan install . --lockfile=conan.lock --output-folder build/conan/dev --build missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset dev -DCMAKE_TOOLCHAIN_FILE="$PWD/build/conan/dev/build/Debug/generators/conan_toolchain.cmake"
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

## Running the terminal interface

```bash
build/dev/apps/bernstein-agent-tui/bernstein-agent-tui
```

Type a request and press Enter. The entry appears with its lifecycle state, and a simulated operation advances from pending to running to settled. Operations run one at a time.

| Key | Effect |
| --- | --- |
| Enter | Submit the draft as an input |
| Backspace | Delete one character, UTF-8 aware |
| Esc or Ctrl+U | Clear the draft |
| Ctrl+X | Cancel the oldest active operation |
| Ctrl+R | Reconcile the newest operation with an unknown outcome |
| Ctrl+C | Quit |

A terminal smaller than 60x20 shows a diagnostic instead of a clipped layout.

Headless, for scripting or for checking behavior without a terminal:

```bash
printf 'first request\nsecond request\n' | build/dev/apps/bernstein-agent-tui/bernstein-agent-tui --headless
```

It reads input lines, runs them through the same key translation, advances the simulation until nothing is active, and prints a plain-text snapshot with no escape sequences.

Build without the terminal interface and its dependency:

```bash
cmake --preset dev -DBERNSTEIN_BUILD_TUI=OFF
```

## Layout

```text
apps/bernstein-agent/      Command-line entry point (placeholder).
apps/bernstein-agent-tui/  Terminal interface: rendering, key translation, headless driver.
apps/bernstein-sdlc/       Reserved for a later lifecycle-coordination executable.
libs/bernstein-core/       Session lifecycle model, facade, and UI projection.
libs/bernstein-protocol/   Reserved for the host/reasoner boundary types.
ocaml/                     Architecture and policy reasoner package (skeleton).
protocol/                  Host/reasoner protocol documentation.
docs/                      Design, implementation plan, results, and manual scenarios.
tests/                     Catch2 tests for the core model and the interface.
```

## Documentation

- [Terminal interface design](docs/ftxui-spike.md)
- [Implementation plan](docs/ftxui-spike-implementation-plan.md)
- [Results and decision record](docs/tui-results.md), including the evidence boundaries and known gaps
- [Manual terminal scenarios](docs/tui-manual-scenarios.md)
- [Roadmap](ROADMAP.md)

## How this project treats evidence

Claims in this repository are meant to name their own limits. Compilation, a single test, the configured suite, a scripted terminal run, and human review are different claims, and the results document says which was actually done. If something was not verified, it is listed as a gap rather than described as working.

## Contributing

The roadmap is the current plan, and the open questions in it are genuinely open. Small, self-contained changes with their verification stated are easiest to review.

## License

BSD 3-Clause. See [LICENSE](LICENSE).
