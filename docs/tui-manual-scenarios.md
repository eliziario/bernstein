# Bernstein TUI manual scenarios

These scenarios cover behavior that needs a real terminal and is not established by the automated tests. They are short and repeatable, so a reviewer can run them against the exact built executable.

## Preparation

```bash
just configure
just build
```

Launch:

```bash
build/dev/apps/bernstein-agent-tui/bernstein-agent-tui
```

For each run, record the operating system, terminal emulator and version, `$TERM`, locale, terminal dimensions, and any multiplexer in use.

## Scenarios

### 1. Fullscreen launch

1. Launch the executable.
2. Confirm that it uses the alternate screen and fills the terminal.
3. Confirm that the header, conversation, activity, context, composer, and footer are visible.

Expected: the empty conversation state is shown. After exit, no application output remains in the primary terminal buffer.

### 2. Submission and simulated lifecycle

1. Type `review the session boundary` and press Enter.
2. Watch the entry for a few seconds.

Expected:

- The input first shows `accepted, not yet model-visible`, then `promoted`.
- The operation moves from `pending` to `running` to `settled`.
- The activity region shows durable events with increasing sequence numbers.
- Typing stays responsive while the simulation advances.

### 3. Serial operations and cancellation

1. Submit two inputs in quick succession.
2. While the second one is still pending, press Ctrl+X until the first one settles.

Expected:

- The second operation waits until the first leaves the active states.
- Ctrl+X on a pending operation shows `interrupted`.
- Ctrl+X on a running operation shows `operation outcome unknown`, in red, never `settled`.

### 4. Reconciliation

1. Produce an unknown outcome (scenario 3) and press Ctrl+R.

Expected: the operation stays unknown, and a live `reconcile` observation says that no external outcome is available.

### 5. Editing

1. Type a draft that contains `café`.
2. Press Backspace once.
3. Press Esc or Ctrl+U.

Expected: Backspace removes `é` as one character, Esc and Ctrl+U clear the draft, and no input or operation is created.

### 6. Terminal resize

1. Enlarge the terminal.
2. Shrink it below 60 columns or 20 rows.
3. Restore a comfortable size.

Expected: the application redraws without crashing. Below the minimum it shows `Terminal too small` with the current and required sizes. When the size is restored, the full layout returns and the draft is preserved.

### 7. Interrupt and terminal restoration

1. Press Ctrl+C with an empty composer.
2. Relaunch, and press Ctrl+C with a draft and an operation running.

Expected: the process exits with status 0, the alternate screen is exited, and the shell prompt works without manual terminal repair.

### 8. Headless mode

```bash
printf 'first request\nsecond request\n' | build/dev/apps/bernstein-agent-tui/bernstein-agent-tui --headless
```

Expected: an 80x24 plain-text snapshot with both requests settled, containing no escape sequences. With stdin attached to a terminal, `--headless` prints the initial state.

## Evidence boundary

The automated suite establishes lifecycle transitions, key translation, fixed-size rendering, and headless parity. These scenarios provide the human evidence for real terminal ownership, resize signals, terminal restoration, responsiveness, and the readability of constrained layouts.
