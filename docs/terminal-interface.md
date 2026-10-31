# The terminal interface

| | |
|---|---|
| Status | Built and in use; FTXUI adopted on 2026-09-16 |
| Decision record | [tui-results.md](tui-results.md) |

This describes the interface as it is, the boundary it is there to enforce, and the measurements that constrain what it may become. It replaces an earlier document that posed the choice of terminal library as an open question; that question was settled and the reasoning is in the decision record.

A word on ambition. The interface is a surface for seeing what the agent is doing, and it is not the product. It earns its place by making lifecycle state visible and hard to misread, which is a narrow job and a useful one.

## What the interface may and may not do

Input arrives as a keystroke and leaves as a typed intent. Along the way it passes through exactly one adapter that knows what a terminal event is, and after that point nothing downstream has any notion of one:

```text
terminal event -> from_terminal -> KeyEvent -> translate_event -> UserIntent
  -> SessionFacade -> immutable snapshot -> render_projection -> screen
```

The renderer is handed a value. It cannot write back through it, it cannot reach the session, and it cannot acquire anything that causes an effect. This is not a convention: `bernstein::input`, which owns the key event and the translation, links no terminal library at all, its tests link none, and a build configured with `BERNSTEIN_BUILD_TUI=OFF` contains no reference to FTXUI anywhere. The arrangement is checkable, and the no-interface job in continuous integration checks it.

Three targets, with the dependency pointing one way:

| Target | Holds | Knows about FTXUI |
|---|---|---|
| `bernstein::core` | Session lifecycle, facade, projection | No |
| `bernstein::input` | Key events, intent translation | No |
| `bernstein::tui` | Rendering, the headless driver, the terminal adapter | Yes |

## Rules the rendering obeys

These exist because the failure they prevent is quiet. A screen that misrepresents lifecycle state is worse than no screen, since it invites a person to act on a belief the system never held.

An unknown outcome is never drawn as a success, as a blank, or as anything green. It says it is unknown, in red, and it carries its effect status beside it.

An entry says which stage it has reached: admitted, promoted into model-visible history, running, settled, or unresolved. These are different, and collapsing any two of them is the thing the lifecycle model exists to prevent.

A live observation is marked as live and distinguished from a durable event, which carries a sequence number. One of these survives a restart and the other does not, and a reader cannot be expected to guess which.

When a view shows a proposal or a candidate, it shows its approval and verification status rather than its title alone, since the status is the part that governs what may happen next.

An approval control, when one exists, will not look like ordinary navigation.

## Scrolling, and the limit we measured

The conversation region builds an element for every entry and wraps the result in `frame` with `focusPositionRelative`, which is how FTXUI invites you to scroll. It rebuilds the whole tree each frame, and a frame happens on every keystroke. Rendering a synthetic diff at a 200x50 viewport:

| Lines | Element per line, wrapped in `frame` | Visible window only |
|---|---|---|
| 1,000 | 1.1 ms | 0.15 ms |
| 5,000 | 5.8 ms | 0.15 ms |
| 20,000 | 25.6 ms | 0.15 ms |
| 100,000 | 137 ms | 0.15 ms |

So the idiom is fine for a conversation of a few dozen entries and unusable for a diff. A diff view must render only the visible window, and the same applies to conversation history if it ever grows large. The numbers are from one machine, one viewport, and synthetic content; they are the right order of magnitude and should be re-measured against real content before anyone relies on them closely.

Below 60 by 20 the interface stops drawing the layout and says the terminal is too small, giving both the current and the required size. A clipped view of lifecycle state is the failure this avoids.

## How it is tested, and what each test establishes

Rendering to a fixed off-screen screen gives deterministic assertions about content at a known size. It establishes what text is produced; it says nothing about how a terminal draws it.

The headless mode drives the same session through the same translation and prints plain text with no escape sequences, which makes the behaviour checkable without a terminal at all and is the path a script would use.

A pseudo-terminal harness runs the scenarios in [tui-manual-scenarios.md](tui-manual-scenarios.md), including real resize signals. It establishes that the program enters and leaves the alternate screen, survives resizing, reports the states it should, and exits cleanly.

What none of these establishes is whether a person can read the result. That remains a human judgement, and the scenarios document says which parts of it are still outstanding.

## Why FTXUI, and what would reopen the question

The library supplies four things we would otherwise have to write: a layout engine with flexible sizing and a scrolling viewport, terminal lifecycle including the alternate screen and restoration, off-screen rendering at a fixed size, which is what makes the tests deterministic, and UTF-8 width handling. The API surface we actually touch is about eighteen DOM functions, the component loop, and the screen, and the dependency is isolated to one target.

Three things would justify reconsidering it.

A diff viewer with large buffers, syntax highlighting, and mouse selection, if its requirements outgrow what the DOM can do per frame. The measurement above is the beginning of that argument rather than the end.

A decision that the primary surface should not be a full screen at all. If the useful first product is a loop whose output is a transcript, a diff, and check results, then the layout engine is the part that stops paying for itself. This is a product question, not a library one, and it is open in the roadmap.

Terminal portability, if a real emulator turns out to break something. There is no evidence of this and also little evidence against it: one pseudo-terminal and one multiplexer have been exercised.

## Alternatives retained

`ncursesw` remains the portability-oriented fallback, preferable if deployment on restricted or unusual terminals becomes a requirement, or if FTXUI's abstraction blocks control we need.

`CPP-Terminal` remains the option for owning our own layout and widgets, which reduces the dependency and increases what we must design and test ourselves.

`Notcurses` remains interesting for a later graphics-rich view of evidence or topology, and is not needed for a text interface.

A line-oriented renderer of our own is the fourth option, and the one that becomes attractive if the full-screen metaphor is abandoned.

## Related

- [Results and decision record](tui-results.md), with the evidence and its limits
- [Terminal scenarios](tui-manual-scenarios.md)
- [RFC 0001](rfc-0001-core-interfaces.md), for the boundary the interface sits in front of
- [The plan this interface was built from](terminal-interface-plan.md)
