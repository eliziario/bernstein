# Bernstein

Bernstein is a coding agent built on the assumption that the interesting unit of work is not a diff. It is a decision, the parts of the design that decision touches, the implementation that follows from it, the evidence that the implementation does what was intended, and whatever uncertainty is left over. Those five things tend to arrive in a chat transcript flattened into one undifferentiated stream, where a plausible suggestion and a verified result look alike. Keeping them apart is the whole point of the project.

It is early. The session lifecycle and the terminal interface exist and are tested; nothing is connected to a model provider, and nothing touches your files. What you can run today is an architectural skeleton with an interface attached, which is more useful to read than to use. [ROADMAP.md](ROADMAP.md) says what is planned, in what order, and why that order.

## Who might want this

Someone who has watched an assistant report success for work it never ran, and who would rather have a tool that distinguishes a proposal from an approval, and an approval from something that actually happened. Someone who needs to answer, later, why a change was made and what evidence supported it. It is not yet a tool you would use to get work done, and it will say so rather than pretend otherwise.

## What exists

The session model keeps apart the states that are easy to conflate: an input that has been admitted, an input that has become visible to the model, an operation that is pending, running, interrupted, of unknown outcome, or settled, and an effect that is intended, recorded, observed, or unknown. The interface renders those states and never performs an effect itself; a keystroke becomes a typed intent, and only the session changes anything.

Snapshots given to the interface are immutable and shared. The session publishes a new one after each change, carrying the most recent durable events and a cursor; the full history is read back through that cursor rather than copied into every frame.

Cancellation respects what is actually known. Cancelling before anything was dispatched is a known outcome, since no effect was attempted. Cancelling after dispatch is not, and the operation says so instead of claiming it stopped. Reconciliation applies only to operations in that unknown state, and it never invents a result it did not observe.

A keystroke becomes a Bernstein key event before it becomes anything else, so the path from input to intent knows nothing about the terminal library: the library that owns it links no FTXUI, and a build with the interface switched off contains no reference to FTXUI at all. There is also a headless mode that drives the same session through the same translation and prints plain text, so the behaviour can be checked without a terminal. Twenty-five tests cover the lifecycle transitions, the snapshot contract, key translation, rendering, and the headless path.

## What does not exist

No model provider; the session is deterministic and in memory, and serves as the reference for what the lifecycle means. No changes to your workspace, no commands run, no sandboxing, no capability enforcement. No persistence, so everything is lost when the process exits. No navigation, inspection, approval, or diff views in the interface. The OCaml reasoner and the protocol between it and the host are described in [docs/rfc-0001-core-interfaces.md](docs/rfc-0001-core-interfaces.md) and not built.

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
- [Terminal interface design](docs/ftxui-spike.md) and the [implementation plan](docs/ftxui-spike-implementation-plan.md).
- [Results and decision record](docs/tui-results.md), which carries the evidence behind the claims above along with its own limits.
- [Manual terminal scenarios](docs/tui-manual-scenarios.md), for the behaviour a test cannot establish.
- [ROADMAP.md](ROADMAP.md).

## On the claims in this repository

Compiling, passing one test, passing the configured suite, a scripted run in a pseudo-terminal, and a person actually looking at the screen are five different claims, and the results document says which of them was made. Where something was not checked it is recorded as a gap rather than described as working. This is tiresome to write and it is the only way the evidence is worth anything.

## Contributing

The roadmap is the current plan and its open questions are genuinely open. Small self-contained changes that state what was verified are the easiest to review.

## License

Apache License 2.0. See [LICENSE](LICENSE).
