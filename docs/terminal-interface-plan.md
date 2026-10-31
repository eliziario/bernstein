# The plan the first interface was built from

| | |
|---|---|
| Status | Completed and retained as a record |
| Outcome | [tui-results.md](tui-results.md) |
| Live plan | [ROADMAP.md](../ROADMAP.md) |

This was a sequencing plan for the first terminal interface. It has been carried out, so what remains useful about it is the order it chose and the two things it got wrong. The detailed per-change history is not reproduced here; the earlier revisions of this file in version control hold the original plan in full.

## The order, and why it held

The plan put the contracts first and the interface last: lifecycle types and a deterministic session, then tests for the failure boundaries, then the terminal dependency, then a read-only renderer, then event translation, then simulated lifecycle updates, then the interrupt and resize paths, then deterministic tests, then headless parity, then the manual scenarios, then a results document and a decision.

The reason for that order was that the contracts are expensive to change once UI details depend on them, and the ordering did its job. The lifecycle distinctions that matter — admission against promotion, operation state against effect state, live observation against durable event, unknown against failure — were all settled before anything was drawn, and none of them had to be revisited when the interface arrived. Had the screen come first, the states would have been invented to suit a layout.

It also required that failure boundaries be tested before FTXUI appeared at all, which is why cancellation, reconciliation, and unknown outcomes have the semantics they do rather than whatever would have been convenient to render.

## What it produced

Twelve stacked changes, merged through pull requests #2 to #16, delivering the session lifecycle and facade, the immutable projection contract, the FTXUI renderer, key translation, a simulated lifecycle, the fullscreen loop, deterministic rendering tests, and a headless path. The outcome, the evidence behind it, and the gaps it left are recorded in [tui-results.md](tui-results.md).

## What the plan got wrong

It asked for the session to be terminal-independent, and the first implementation was not. Intent translation took an FTXUI event, and the headless driver imported the same library to synthesise events for text read from a stream, so the boundary the plan called for existed in the core and not in the input path. This was corrected afterwards by giving Bernstein its own key event type and confining the library to the renderer, the loop, and a single adapter. The lesson is that a boundary stated in a document is not a boundary; the version that holds is the one a build can check, which is why the no-interface build now asserts it.

It also treated rendering performance as a question for later, and the renderer accordingly used the scrolling idiom the library invites. Measurement afterwards showed that idiom costs a full tree rebuild per frame, which is harmless for a conversation and disqualifying for a diff. Nothing in the plan would have caught this, because the plan's acceptance criteria were about correctness and said nothing about cost.

## Where the work goes now

Forward-looking work for the interface lives in [ROADMAP.md](../ROADMAP.md), chiefly M1, which covers navigation, inspection, a diff view that renders only the visible window, approval controls, and the states the current implementation declares but never produces.
