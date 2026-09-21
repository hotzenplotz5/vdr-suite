# VDR-Suite Golden User Journeys

## Purpose

These journeys complement unit, architecture, CI and real-system safety gates with end-to-end product acceptance. They define what a user must accomplish through VDR-Suite contracts without knowing private provider details.

A journey is not automatically a requirement for the current slice. The Strict Roadmap decides when a journey becomes a numbered-phase or product-milestone exit gate.

## Acceptance principles

- Start from a real user-visible entry point and finish at an observable outcome.
- Exercise Suite-owned contracts rather than private RESTfulAPI, SVDRP, Streamdev, SuiteBridge, plugin-cache or browser-control endpoints.
- Preserve backend identity, authorization, provider ownership and failure semantics.
- Do not mark a journey PASS from CI alone when it changes installed runtime, media or broadcast behavior.
- Record exact source head, relevant CI, runtime candidate and redacted observed result for real-system acceptance.

## Journey 1 — Live TV playback

```text
channel / EPG selection
  -> authorized MediaSession
  -> selected compatible delivery profile
  -> playback starts
  -> real picture + sound
  -> channel change
  -> old media route/provider resources close cleanly
```

Phase-65 acceptance proves provider privacy, explicit route/provider ownership, bounded resource lifetime, deterministic replacement/cleanup and classified failure.

## Journey 2 — Recording playback

```text
Recordings
  -> Recording detail
  -> authorized MediaSession
  -> play
  -> real picture + sound
  -> seek where supported
  -> stop / resume
  -> deterministic cleanup
```

Phase-65 acceptance requires least transformation, truthful seek/range capability and stable Suite Recording identity.

## Journey 3 — Record one programme

```text
EPG programme
  -> create TimerIntent
  -> TimerAssignment
  -> managed NativeTimerBinding
  -> authoritative native VDR Timer readback
  -> recording result
```

The engine portion is completed by Phase 64. Broad polished Timer UI remains a cross-cutting product milestone.

## Journey 4 — Multi-backend scheduling without provider knowledge

```text
one recording intent
  -> current backend evidence and policy
  -> deterministic eligible-backend decision
  -> exactly intended assignment ownership
  -> native fulfillment on selected backend
```

Phase 64 completed the engine semantics including controlled failover and stale/read-only rejection.

## Journey 5 — Failure without hidden unsafe recovery

```text
backend / provider / transport failure
  -> classified Suite-visible state
  -> no blind duplicate mutation or silent provider switch
  -> reconciliation / retry only when evidence permits
  -> understandable client/operator result
```

This remains cross-cutting across later phases.

## Journey 6 — Browse Media Home on desktop

```text
open VDR-Suite
  -> Home useful before preview
  -> browse Live hero rapidly
  -> Now/Next and artwork follow focus immediately
  -> focus settles
  -> optional delayed Live preview
  -> explicit Watch Live
  -> return Home
  -> Continue Watching / Recording discovery
```

Phase-66 Golden desktop acceptance is complete. Later post-phase Home hardening/rebuild preserved the same ownership model while strengthening performance, Series/Genre/Movie presentation and metadata/artwork correctness.

## Journey 7 — Browse Media Home on a phone

```text
open VDR-Suite on phone
  -> dominant Live hero / responsive composition
  -> swipe/browse channels
  -> Now/Next follows focus immediately
  -> optional settled preview
  -> Watch Live / EPG actions
  -> Recording/Continue Watching rails
  -> primary mobile navigation
```

Phase-66 Golden mobile acceptance is complete.

## Journey 8 — Teletext while watching Live TV

```text
Live TV
  -> Teletext available
  -> open Teletext
  -> page/subpage navigation
  -> close
  -> Live remains usable
```

Acceptance must use Suite Teletext service/page contracts with truthful freshness/backend/channel identity and no raw plugin command channel.

**Accepted.** Real yaVDR/browser acceptance proved the normalized service/page path, page/subpage navigation, 25 x 40 / 1000-cell rendering, desktop Live-TV companion behavior, close-with-Live-still-usable semantics, and direct Media Home Teletext entry. Durable evidence is in [Phase 67 Teletext Closeout](../development/phase-67-teletext-closeout.md).

## Journey 9 — Launch one HbbTV broadcast application

```text
Live Channel
  -> discovered app
  -> authorized BroadcastApplicationSession
  -> isolated runtime
  -> normalized input
  -> close/channel change
  -> cleanup
```

Acceptance proves bounded discovery, isolation, stale-context fencing and reuse of Phase-65 MediaSession semantics for Suite-owned media.

**Accepted.** Real yaVDR/browser acceptance proved discovered HbbTV application launch through a Suite-owned session, normalized input, presentation/media use, same-channel continuity, stale-context close behavior, close/relaunch and stable continued use on the supported deployment profile. Durable evidence is in [Phase 67 Closeout](../development/phase-67-closeout.md).

## Journey 10 — Use one legacy native OSD workflow safely

```text
Explicit Legacy OSD
  -> authorized session
  -> authoritative frame
  -> optional fenced controller lease
  -> allowlisted input
  -> resulting frame
  -> close
```

This is a Phase-68 journey.

## Journey 11 — Manage a Timer safely through the broad Timer UI

This remains a cross-cutting product milestone: EPG/Timer -> permission -> revision-safe TimerIntent mutation -> visible assignment/fulfillment -> authoritative readback/reconciliation -> final state.

## Relationship to phase completion

```text
Phase 64 [completed] -> engine portions of Journeys 3, 4 and Timer-related Journey 5
Phase 65 [completed] -> Journeys 1 and 2 + media Journey 5
Phase 66 [completed] -> Journeys 6 and 7
Phase 67 [completed] -> Journeys 8 and 9 accepted
Phase 68 [active: 68.A read-only observation] -> Journey 10
Broad Timer Product UI -> Journey 11 + user-facing Journey 3
Phase 69 -> public/client compatibility hardening
```

Phase 70 recommendation work must add its own user-visible journey before runtime acceptance.

## Change rule

New primary product surfaces should add or extend a Golden User Journey when technical component tests alone would not prove the user-visible outcome. Do not create a separate journey for every internal slice.
