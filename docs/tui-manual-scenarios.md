# Bernstein TUI manual scenarios

These scenarios cover behavior that depends on a real terminal and is not established by the headless or fixed-screen tests. They are intentionally short and repeatable so a reviewer can run them against the exact built executable.

## Preparation

```text
cmake --build --preset dev
build/dev/apps/bernstein-agent-tui/bernstein-agent-tui
```

Run the executable from a terminal that can report its dimensions and restore the terminal after an interrupt.

## Scenarios

### 1. Fullscreen launch

1. Launch the executable in a normal terminal.
2. Confirm that the alternate screen is used and the layout occupies the available terminal area.
3. Confirm the header, conversation area, composer, and footer are visible.

Expected: the TUI takes the terminal dimensions, displays the empty conversation state, and does not leave application output in the primary terminal buffer after exit.

### 2. Composer input and submission

1. Type `review the session boundary`.
2. Confirm the text appears in the composer.
3. Press Enter.
4. Confirm the composer is cleared and the session summary changes to show an admitted operation.

Expected: typing changes local composer state; Enter creates a `SubmitInput` intent and admits the input through the session boundary. No external effect is implied.

### 3. Editing and cancellation

1. Type a short draft.
2. Use Backspace to remove the last character.
3. Press Escape or Ctrl-U.

Expected: the draft is cleared without creating an input receipt or operation.

### 4. Terminal resize

1. Launch the TUI.
2. Enlarge the terminal vertically and horizontally.
3. Reduce it to a narrow and then short size.
4. Restore a comfortable size.

Expected: the application redraws without crashing, the conversation region absorbs available height, and the composer/footer remain understandable. Any clipping or unreadable state is a finding for the next UI refinement.

### 5. Interrupt and terminal restoration

1. Launch the TUI.
2. Press Ctrl-C while the composer is empty and while it contains draft text.
3. Confirm the process exits.

Expected: the event is translated to `Quit`, the alternate screen is exited, and the shell prompt is usable without manual terminal repair.

### 6. Headless parity smoke check

```text
build/dev/apps/bernstein-agent-tui/bernstein-agent-tui --headless
```

Expected: a fixed-size snapshot is printed and the command exits without requiring an interactive terminal.

## Evidence boundary

The automated suite establishes deterministic translation, projection rendering, headless parity, lifecycle simulation, and fixed-size output. These scenarios are the human evidence for terminal ownership, actual resize signals, terminal restoration, and the qualitative readability of constrained layouts.
