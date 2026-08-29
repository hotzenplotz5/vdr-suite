# Phase 66.4 — Continue Watching Closeout

Status: **ACTIVE implementation candidate. Exact final-head hosted CI and real-system acceptance are still mandatory before Slice 66.4 is accepted.**

Draft PR: **#235 — Phase 66.4: Continue Watching**

This document records the bounded Slice-66.4 architecture and acceptance contract. It does not authorize Slice 66.5 or later Phase-66 work.

## Scope

Slice 66.4 adds a prominent `Continue Watching` / `Weiterschauen` rail to Media Home and the smallest durable server-side truth required to make unfinished Recording playback portable across first-party clients.

The slice deliberately does **not** add viewing history, recommendation ranking, analytics, personalized rail ordering, a second Recording catalog, a second capability engine, or a second media/player owner.

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

The Recording cache does **not** manufacture or project a `playbackCapable` / resume-capability bit for Continue Watching. Resume capability belongs to the canonical active Recording playback owner and its normalized Phase-65 playback contract.

The browser does not use `localStorage` as cross-client truth. The authenticated actor is supplied by the trusted HTTP/session boundary and is never accepted from the Continue Watching request payload.

## Resume-capability truth

Continue Watching state is created only after the existing canonical Recording playback owner confirms real resume support through its established `canResume()` contract. That owner derives the capability from the normalized server playback contract; Slice 66.4 does not add a second capability resolver.

The first-party progress mutation carries `resumeSupported: true` only after this canonical owner check. A false or missing value fails closed and clears/prevents current resume state.

When persisted state is read later, the server revalidates only truths the Recording projection actually owns:

- backend/Recording identity still resolves;
- the Recording still exists;
- current duration is used when it is truthfully known;
- exact completion is rejected when known `position >= duration`.

The read path does not pretend that the Recording cache can independently rediscover current playback/resume capability.

## Update semantics

A progress update is retained only when all of the following remain true:

- the canonical Recording identity resolves for the requested backend;
- the active canonical Recording owner has already confirmed resume support;
- the canonical absolute position is greater than zero;
- when duration is known, the absolute position is strictly before the duration.

Position zero, missing/non-resumable owner evidence and exact completion (`position >= duration`) clear the current-state row.

Unknown duration does not fabricate completion or percentage evidence. The row may remain resumable with an absolute position while the UI omits percentage progress.

`operationId` makes a repeated identical write idempotent. Replaying the same operation does not rewrite position or activity ordering. A later distinct operation may truthfully move the current position backward, for example after an explicit restart and subsequent playback.

A same-client mutation queue serializes progress/clear writes so an older in-flight progress update cannot arrive after a newer clear and recreate stale state.

## API and security boundary

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

Responses are `no-store`. Backend and Recording identity stay explicit.

For authorization, the Continue Watching request is normalized to the existing Recording media-session boundary `/api/media/sessions`, reusing the established `media.recording.play` permission instead of inventing a new privilege surface. Browser POSTs require the existing CSRF contract.

Actor identity comes exclusively from the trusted security context (`gate.context.actor.actorId`); client JSON cannot select or spoof another actor. The original Continue Watching request path remains intact for API dispatch after authorization.

This endpoint is a first-party product/runtime contract for current resume state; it is not a public history or recommendation API.

## Playback ownership and runtime composition

Both Home actions enter the existing `VdrSuiteRecordings2` Recording flow:

```text
Continue
  -> canonical existing Recording owner
  -> saved absolute Recording position

From beginning
  -> same canonical existing Recording owner
  -> absolute position 0
```

A fresh Media Home does not eagerly instantiate a second Recording runtime. `openItem()` uses the established deferred frontend loader `VdrSuiteDeferredFrontendRuntimes.loadRecordings2()` when necessary and then delegates only to `VdrSuiteRecordings2.openRecording(...)`.

No Home-specific `MediaSession`, `<video>` element, transport adapter or cleanup engine is created.

For HLS compatibility playback, the already-existing internal absolute `startAt(position)` path is exposed on the canonical owner as `startAtAbsolute`. It creates the ordinary Recording MediaSession with `startPositionSeconds` and therefore preserves the accepted server-side restart/seek semantics.

For fast completed-Recording playback, Continue Watching stays on the same canonical owner and uses that owner's established start plus absolute seek/replacement contract. It does not create a concurrent playback owner.

## Live-preview relinquish

The existing Home Live Preview remains the canonical preview owner from Slice 66.3. Slice 66.4 adds only a narrow public release surface:

```text
VdrSuiteHomeLivePreview.cancel(...)
```

Continue Watching calls that public API before handing control to the Recording flow. Production code does not reach into preview `__test` hooks and does not build a second preview ownership engine.

This guarantees that pending/active Home preview intent is relinquished before Recording detail/playback ownership starts.

## Playback lifecycle and progress sampling

Session-bound Continue Watching behavior follows the canonical Phase-65 owner lifecycle publication:

```text
snapshot()
subscribe()
```

It does not use DOM observation, polling or method interception as primary session lifecycle truth. In particular, `start()`, `stop()`, `destroy()` and `relinquishForReplacement()` are not decorated as lifecycle authority.

A five-second timer is transport-local sampling only. It samples canonical absolute Recording position while the published owner state proves active playback ownership. When ownership is not active, the timer does not create progress truth.

Stop/relinquish transitions flush the last truthful resumable absolute position through the serialized mutation queue. Destroyed lifecycle publication releases observation. The media `ended` signal clears Continue Watching state, including cases where duration is unknown to the projection.

Absolute Recording position is canonical throughout; Slice 66.4 does not introduce a second heuristic timeline or a near-end threshold.

## Completion and cleanup

Completion clears server truth when either:

- known duration proves `position >= duration`; or
- the existing media element emits `ended`, including when total duration is not known by the Continue Watching projection.

Unknown duration is never interpreted as zero/completed and never produces a fabricated progress percentage.

Deleted/stale Recording identity is removed from durable current state when list projection can no longer resolve the Recording.

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
- missing/non-resumable canonical owner evidence is excluded;
- no fabricated server/cache `playbackCapable` truth exists;
- actor and backend isolation;
- unknown duration remains truthful and percentage-free;
- exact, not heuristic, completion semantics;
- duplicate operation replay idempotency;
- later distinct operations may move position backward;
- same-client mutation ordering;
- activity ordering;
- stale/deleted Recording cleanup;
- scoped explicit clear;
- no browser-local cross-client truth;
- CSRF and existing Recording-play authorization boundary reuse;
- actor identity remains trusted-server-context only;
- owner lifecycle uses `snapshot()/subscribe()` rather than lifecycle method interception;
- progress sampling happens only under published active ownership;
- no Home-created MediaSession/video owner;
- Continue and From beginning use `VdrSuiteRecordings2`;
- fresh Home loads the canonical deferred Recordings2 runtime before opening;
- production Continue Watching uses the public Live Preview `cancel()` API, never preview `__test` hooks;
- HLS uses its canonical absolute server-start path;
- fast playback keeps the canonical owner and established absolute-seek path;
- install staging contains the Continue Watching frontend/runtime assets.

## Candidate evidence

The final candidate evidence is intentionally not frozen until the exact final branch head has completed hosted CI.

```text
final_candidate_sha=PENDING
source_ci_workflow=VDR-Suite CI
source_ci_run_number=PENDING
source_ci_run_id=PENDING
source_ci_result=PENDING
real_system_acceptance=PENDING
```

A green CI run for any earlier intermediate SHA is not Slice-66.4 completion evidence.

## Acceptance gate

Slice 66.4 is not accepted merely because the code is committed or a PR exists. Acceptance requires:

1. exact-head hosted CI green for the final candidate;
2. install/staging and repository-derived installation path verified for that same candidate;
3. real yaVDR browser validation of persistence, cross-browser/device resume truth, Resume, From-beginning, preview-to-Recording ownership transfer, stale Recording behavior and completion cleanup;
4. no regression to the existing Phase-65 Recording/Live playback ownership contracts;
5. no work from Slice 66.5 or later Phase-66 slices.

Only after those gates may Slice 66.4 be recorded as completed. Until real yaVDR acceptance passes, this document and `docs/CURRENT.md` must continue to report Slice 66.4 as active/PENDING.
