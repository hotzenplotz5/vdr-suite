# Phase 66.4 — Continue Watching Closeout

Status: **Implementation candidate under hosted-CI validation. Real-system acceptance remains mandatory before Slice 66.4 is accepted.**

This document records the bounded Slice-66.4 architecture and acceptance contract. It does not authorize Slice 66.5 or later Phase-66 work.

## Scope

Slice 66.4 adds a prominent `Continue Watching` / `Weiterschauen` rail to Media Home and the smallest durable server-side truth required to make unfinished Recording playback portable across first-party clients.

The slice deliberately does **not** add viewing history, recommendation ranking, analytics, personalized rail ordering, a second Recording catalog, or a second media/player owner.

## Durable truth

Continue Watching is actor-scoped current state, not an event log.

Its durable key is:

```text
(actor_id, backend_id, recording_id)
```

The stored mutable state is limited to:

```text
absolute Recording position
last relevant activity time
last operation id
```

Recording title, duration and existence remain current Recording-domain truth and are resolved from the existing VDR Recording cache when the rail is read. A persisted row whose Recording no longer exists is removed instead of being presented as stale content.

The browser does not use `localStorage` as cross-client truth. The authenticated actor is supplied by the trusted HTTP/session boundary and is never accepted from the Continue Watching request payload.

## Update semantics

A progress update is retained only when all of the following remain true:

- the canonical Recording identity resolves for the requested backend;
- playback is capable/resumable;
- the canonical absolute position is greater than zero;
- when duration is known, the absolute position is strictly before the duration.

Position zero, non-resumable state and exact completion (`position >= duration`) clear the current-state row.

Unknown duration does not fabricate completion or percentage evidence. The row may remain resumable with an absolute position while the UI omits percentage progress.

`operationId` makes a repeated identical write idempotent. Replaying the same operation does not rewrite position or activity ordering. A later distinct operation may truthfully move the current position backward, for example after an explicit restart and subsequent playback.

## API boundary

The bounded first-party endpoint is:

```text
POST /api/media/continue-watching
```

Supported operations are:

```text
list
progress
clear
```

Responses are `no-store`. Actor scope comes from the authenticated server context. Backend and Recording identity stay explicit.

This endpoint is a first-party product/runtime contract for current resume state; it is not a public history or recommendation API.

## Playback ownership

Both Home actions enter the existing `VdrSuiteRecordings2` Recording flow:

```text
Continue
  -> canonical existing Recording owner
  -> saved absolute Recording position

From beginning
  -> same canonical existing Recording owner
  -> absolute position 0
```

No Home-specific `MediaSession`, `<video>` element, transport adapter or cleanup engine is created.

For HLS compatibility playback, the already-existing internal absolute `startAt(position)` path is exposed on the canonical owner as `startAtAbsolute`. It creates the ordinary Recording MediaSession with `startPositionSeconds` and therefore preserves the accepted server-side restart/seek semantics.

For fast completed-Recording playback, Continue Watching stays on the same canonical owner and uses that owner's established start plus absolute seek/replacement contract. It does not create a concurrent playback owner.

An active delayed Live preview is relinquished before the Recording detail/playback flow is opened, so preview ownership cannot coexist as a second active Home media owner.

## Completion and cleanup

While Recording playback is resumable, the frontend periodically publishes changed canonical absolute position.

Completion clears server truth when either:

- known duration proves `position >= duration`; or
- the existing media element emits `ended`, including when total duration is not known by the Continue Watching projection.

Stop, owner destruction and owner relinquish flush the last resumable position where truthful. Polling/observer state is torn down with the owner.

## Home presentation

The rail is attached below the Live-TV hero in the existing Home `additional-sections` zone.

Each valid item exposes:

- stable Recording identity/backend;
- title and optional subtitle;
- absolute resume position;
- truthful duration/progress only when duration is known;
- `Fortsetzen`;
- `Von vorn`.

Unknown duration is shown as an absolute resume time without a fabricated percentage meter.

## Regression contract

The Slice-66.4 regression covers at least:

- position zero is excluded;
- non-resumable state is excluded;
- actor and backend isolation;
- unknown duration remains truthful and percentage-free;
- exact, not heuristic, completion semantics;
- duplicate operation replay idempotency;
- later distinct operations may move position backward;
- activity ordering;
- stale/deleted Recording cleanup;
- scoped explicit clear;
- no browser-local cross-client truth;
- no Home-created MediaSession/video owner;
- Continue and From beginning use `VdrSuiteRecordings2`;
- HLS uses its canonical absolute server-start path;
- fast playback keeps the canonical owner and established absolute-seek path;
- install staging contains both new frontend runtime assets.

## Acceptance gate

Slice 66.4 is not accepted merely because the code is committed or a PR exists. Acceptance requires:

1. exact-head hosted CI green for the candidate;
2. install/staging regression green for that same candidate;
3. real yaVDR browser validation of persistence, resume position, From-beginning, preview-to-Recording ownership transfer and completion cleanup;
4. no regression to the existing Phase-65 Recording/Live playback ownership contracts.

Only after those gates may Slice 66.4 be recorded as completed. Slice 66.5 remains separately gated.
