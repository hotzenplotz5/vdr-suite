# VDR-Suite Golden User Journeys

## Purpose

These journeys complement unit, architecture, CI and real-system safety gates with end-to-end product acceptance. They define what a user must ultimately be able to accomplish through VDR-Suite contracts without knowing private provider details.

A journey is not automatically a requirement for the current slice. The Strict Roadmap decides when a journey becomes a phase or milestone exit gate.

## Acceptance principles

- Start from a real user-visible entry point and finish at an observable outcome.
- Exercise Suite-owned contracts rather than private RESTfulAPI, SVDRP, Streamdev or SuiteBridge endpoints.
- Preserve backend identity, authorization, provider ownership and failure semantics.
- Do not mark a journey PASS from CI alone when the journey changes installed runtime or media behavior.
- Record exact source head, relevant CI, runtime candidate and redacted observed result for real-system acceptance.

## Journey 1 — Live TV playback

```text
channel / EPG selection
  -> authorized MediaSession
  -> selected compatible delivery profile
  -> playback starts
  -> picture and sound are present
  -> channel change closes/replaces the old media resources cleanly
```

Acceptance must prove that the client never constructs a private provider URL, a slow/disconnected client does not retain unbounded VDR resources and route/provider cleanup is deterministic.

This becomes a Phase-65 product journey. The preferred media transformation order is pass-through, then remux/repackage, then transcode only when materially required.

## Journey 2 — Recording playback

```text
Recordings
  -> recording detail
  -> authorized playback
  -> play
  -> seek where supported
  -> stop
  -> later resume from durable Suite progress when that feature is enabled
```

Growing recordings must not be represented as complete immutable files when the underlying media is still being written. Persistent resume/progress belongs to stable Suite media identity and actor scope, not a provider URL or player-private identifier.

This becomes a Phase-65 product journey as the corresponding MediaSession capabilities are implemented.

## Journey 3 — Record one programme

```text
EPG programme
  -> TimerIntent
  -> TimerAssignment
  -> managed NativeTimerBinding
  -> authoritative native VDR Timer readback
  -> recording result
```

The user request remains backend-neutral. The Suite may expose why a backend was selected, but the client does not select a private execution provider. Creation is not complete merely because a transport accepted a write; required readback and reconciliation remain part of the journey.

The engine portion of this journey is a Phase-64 completion concern. A broad polished Timer UI is not required to close Phase 64 and remains separately gated on account/backend access management.

## Journey 4 — Multi-backend scheduling without provider knowledge

```text
one recording intent
  -> current backend evidence and policy
  -> deterministic eligible-backend decision
  -> exactly intended assignment ownership
  -> native fulfillment on the selected backend
```

The journey must prove that a read-only, stale, generation-mismatched or otherwise ineligible backend is not selected; provider reachability does not grant authority; and an active assignment or media route does not silently move to another provider.

Deliberate replicas are explicit policy, not accidental duplicates reclassified after the fact.

## Journey 5 — Failure without hidden unsafe recovery

```text
backend / provider / transport failure
  -> classified Suite-visible state
  -> no blind duplicate mutation or silent provider switch
  -> reconciliation / retry only when evidence permits it
  -> understandable client or operator result
```

Examples include an unavailable backend before dispatch, an ambiguous native mutation outcome, provider epoch drift, or a disconnected media route. The outcome must preserve the distinction between definitive no-effect failure, unknown outcome and verified success.

## Relationship to phase completion

Phase 64 uses the Timer portions of Journeys 3–5 to prove the reliable orchestration engine. Phase 65 then makes Journeys 1 and 2 first-class media acceptance paths and reuses the failure principles from Journey 5.

Broad Timer UI completion remains separate from the Phase-64 engine gate. Stable public third-party client contracts remain aligned with Phase 67.

## Change rule

New primary product surfaces should add or extend a golden journey when technical component tests alone would not prove the user-visible outcome. Do not create a separate journey for every internal slice; journeys intentionally remain vertical and product-oriented.
