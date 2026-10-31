# Terminal scenarios

These cover behaviour that depends on a terminal and that the unit tests cannot reach. They are written so that a person can run them against the exact built executable, and most of them have now also been run under a scripted harness, whose results and limits are recorded at the end.

## Preparation

```bash
just configure
just build
build/dev/apps/bernstein-agent-tui/bernstein-agent-tui
```

For each run, record the operating system, the terminal emulator and its version, `$TERM`, the locale, the dimensions, and any multiplexer.

## The scenarios

### 1. Launch and restoration

Launch it, confirm the alternate screen is used and the layout fills the terminal, and that the header, conversation, activity, context, composer, and footer are all visible. On exit no application output should remain in the primary buffer.

### 2. Submission and the simulated lifecycle

Type `review the session boundary` and press Enter, then watch for a few seconds. The input should appear first as accepted but not yet model-visible and then as promoted; the operation should move from pending through running to settled; the activity region should show durable events with increasing sequence numbers; and typing should stay responsive while all this happens.

### 3. Serial operations and cancellation

Submit two inputs in quick succession. The second should wait while the first is active. Press Ctrl+X while an operation is still pending and it should report itself interrupted. Press Ctrl+X while one is running and it should report an unknown outcome, in red, and never report settled.

### 4. Reconciliation

With an unknown outcome on screen, press Ctrl+R. The operation should stay unknown and a live observation should say that no external outcome is available.

### 5. Editing

Type a draft containing `café`, press Backspace once, then press Esc or Ctrl+U. One backspace should remove the whole accented character, the clear keys should empty the draft, and none of it should create an input or an operation.

### 6. Resize

Enlarge the terminal, then shrink it below 60 columns or 20 rows, then restore it. It should redraw without crashing; below the minimum it should say the terminal is too small and give the current and required sizes; and on restoring, the full layout and the unsent draft should both come back.

### 7. Interrupt

Press Ctrl+C with an empty composer, then again with a draft and an operation in flight. The process should exit with status 0, leave the alternate screen, and return a usable shell prompt without manual repair.

### 8. Headless

```bash
printf 'first request\nsecond request\n' | build/dev/apps/bernstein-agent-tui/bernstein-agent-tui --headless
```

An 80x24 plain-text snapshot with both requests settled and no escape sequences. With stdin attached to a terminal it prints the initial state instead.

## What has been executed

Run on 2026-10-31: macOS 15.6.1, arm64, Apple clang 16.0.0, `TERM=xterm-256color`, `LANG=en_US.UTF-8`, through a Python `pty` harness at 100x30, with resizes issued as real `TIOCSWINSZ` calls so the process receives genuine `SIGWINCH`.

Thirty-nine checks across scenarios 1 to 7, all passing. Among the ones worth naming: the alternate screen is entered and left and the exit status is 0; durable event sequence numbers were observed increasing (1 through 6); typing remained responsive while the simulation advanced; cancelling a pending operation reported interrupted while cancelling a running one reported `operation outcome unknown · effect unknown` carrying a red SGR (`\x1b[31m`); Ctrl+R left the outcome unknown and recorded that no external outcome was available; one backspace removed the whole of `é`; below the minimum the diagnostic reported `Current size: 50x10` and `Required: 60x20`; and after restoring the size both the layout and the unsent draft returned.

Scenario 8 was verified separately, both piped and with stdin attached to a terminal.

A multiplexer was also exercised: under GNU `screen` with `TERM=screen`, all regions rendered, the composer accepted input, and the lifecycle reached `input promoted` and `operation settled`.

Two notes on the harness, since they bound the claim. First, an early version of it reported failures in scenarios 3 and 4 that turned out to be its own: it cancelled an operation that had already settled, which correctly produces only an observation, and its escape-stripping inserted a space inside a styled label. Both were harness defects and neither was a defect in the program. Second, a `pty` is not a terminal emulator. The harness reads the bytes the program writes; it does not rasterise them.

## What still needs a person

Whether the layout is legible, whether the colours are distinguishable, and whether a constrained terminal remains understandable rather than merely uncrashed. The harness can confirm that the text is present and cannot judge any of that.

Emulators other than a bare `pty`: Terminal.app, iTerm2, Alacritty, Ghostty, and a remote session over SSH. A different `TERM`, a non-UTF-8 locale, and a light background are all untested.

Restoration as experienced by a human after an interrupt, as distinct from the escape sequence being present in the output stream.
