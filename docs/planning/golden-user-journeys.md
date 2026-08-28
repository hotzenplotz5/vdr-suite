# VDR-Suite Golden User Journeys

## Purpose

These journeys complement unit, architecture, CI and real-system safety gates with end-to-end product acceptance. They define what a user must ultimately accomplish through VDR-Suite contracts without knowing private provider details.

A journey is not automatically a requirement for the current slice. The Strict Roadmap decides when a journey becomes a numbered-phase or product-milestone exit gate.

## Acceptance principles

- Start from a real user-visible entry point and finish at an observable outcome.
- Exercise Suite-owned contracts rather than private RESTfulAPI, SVDRP, Streamdev, SuiteBridge, plugin-cache or browser-control endpoints.
- Preserve backend identity, authorization, provider ownership and failure semantics.
- Do not mark a journey PASS from CI alone when it changes installed runtime, media or broadcast behavior.
- Record exact source head, relevant CI, runtime candidate and redacted observed result for real-system acceptance.
- When a provider/browser/network dependency is external, distinguish Suite correctness from external service availability.

---

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

Acceptance proves:

- client never constructs a private provider URL;
- selected provider/route is explicit and fenced;
- slow/disconnected client does not retain unbounded VDR resources;
- channel replacement/stop cleanup is deterministic;
- real playback is observed, not only an HTTP 200 or manifest fetch;
- failure is classified instead of silently switching provider.

This is a Phase-65 product journey.

---

## Journey 2 — Recording playback

```text
Recordings
  -> Recording detail
  -> authorized MediaSession
  -> play
  -> real picture + sound
  -> seek where supported
  -> stop
  -> deterministic cleanup
  -> later resume from durable Suite progress when that capability is enabled
```

Acceptance rules:

- pass-through is used when valid;
- remux/repackage is introduced only from demonstrated packaging/protocol need;
- transcode is not selected when a lower-transformation profile is valid;
- seek/range capability is truthful;
- a growing Recording is not represented as a complete immutable file;
- persistent resume/progress uses stable Suite media identity and actor scope, not provider URL/player-private identity.

This is a Phase-65 product journey.

---

## Journey 3 — Record one programme

```text
EPG programme
  -> create TimerIntent
  -> TimerAssignment
  -> managed NativeTimerBinding
  -> authoritative native VDR Timer readback
  -> recording result
```

The user request remains backend-neutral. The Suite may explain why a backend was selected, but the client does not select a private execution provider.

Creation is not complete merely because transport accepted a write. Required readback and reconciliation remain part of the journey.

The **engine portion** of this journey is already a Phase-64 completion concern and is complete.

The **broad user-facing Timer Product UI portion** remains a cross-cutting milestone. It must later prove the same journey from real EPG UI interaction while preserving TimerIntent, revision, assignment, reconciliation and permission semantics.

---

## Journey 4 — Multi-backend scheduling without provider knowledge

```text
one recording intent
  -> current backend evidence and policy
  -> deterministic eligible-backend decision
  -> exactly intended assignment ownership
  -> native fulfillment on selected backend
```

Acceptance proves:

- read-only, stale, generation-mismatched or otherwise ineligible backend is not selected;
- provider reachability does not grant authority;
- active assignment does not silently move to another provider/backend;
- deliberate replicas are explicit policy, not accidental duplicates reclassified after the fact;
- controlled failover uses durable evidence and does not overlap exclusive owners.

The engine portion is completed by Phase 64. A later Timer Product UI may expose understandable policy/assignment state without changing these semantics.

---

## Journey 5 — Failure without hidden unsafe recovery

```text
backend / provider / transport failure
  -> classified Suite-visible state
  -> no blind duplicate mutation or silent provider switch
  -> reconciliation / retry only when evidence permits
  -> understandable client/operator result
```

Examples include:

- unavailable backend before dispatch;
- ambiguous native mutation outcome;
- provider epoch drift;
- disconnected media route;
- expired media grant;
- stale HbbTV application context;
- Agent disconnect during Legacy OSD control.

The journey preserves the distinction between definitive no-effect failure, unknown outcome and verified success.

This is cross-cutting and reused by Phases 64–69.

---

## Journey 6 — Browse Media Home on desktop

```text
open VDR-Suite
  -> Home useful before preview
  -> browse Live hero rapidly
  -> Now/Next and artwork follow focus immediately
  -> focus settles
  -> one delayed Live preview attaches
  -> explicit Watch Live
  -> return Home
  -> Continue Watching / Recording discovery rail
```

Acceptance proves browse focus is independent of playback/session state, rapid movement creates no preview sessions, stale preview cannot attach after focus changes, preview relinquishes through the canonical Phase-65 owner, existing domain identities are reused, and keyboard/reduced-motion behavior remains usable.

This is a Phase-66 product journey under ADR-0058. Runtime remains not started until separately authorized.

---

## Journey 7 — Browse Media Home on a phone

```text
open VDR-Suite on phone
  -> one dominant Live hero with neighbor peeks
  -> swipe channels
  -> Now/Next follows focus immediately
  -> settled focus may preview inside hero
  -> Watch Live / EPG touch actions
  -> Continue Watching rail
  -> bottom navigation Home / Live / Recordings / Search / More
```

Acceptance proves mobile is semantic recomposition rather than scaled desktop, swipe/touch never waits on MediaSession startup, obsolete preview work is canceled/relinquished, no persistent floating mini-player steals the viewport, and canonical content/playback identities are shared with desktop.

This is a Phase-66 product journey under ADR-0058.

---

## Journey 8 — Teletext while watching Live TV

Live TV -> Teletext indication -> open -> page/subpage navigation -> close -> Live remains usable.

Acceptance uses Suite Teletext service/page contracts, truthful freshness, correct backend/channel identity and no raw VDR/plugin command channel. This is a Phase-67 journey under ADR-0054.

---

## Journey 9 — Launch one HbbTV broadcast application

Live Channel -> discovered app -> authorized BroadcastApplicationSession -> isolated runtime -> normalized input -> close/channel change -> cleanup.

Acceptance proves bounded discovery, no unrestricted browser/plugin control endpoint, isolation from Suite secrets, stale-context fencing and reuse of Phase-65 MediaSession semantics for Suite-owned media. This is a Phase-67 journey under ADR-0054.

---

## Journey 10 — Use one legacy native OSD workflow safely

Explicit Legacy OSD -> authorized session -> authoritative frame -> optional fenced controller lease -> allowlisted input -> resulting frame -> close.

Acceptance proves domain-first features are not routed through OSD when normal contracts exist, view/control remain separate and no arbitrary command tunnel exists. This is a Phase-68 journey.

---

## Journey 11 — Manage a Timer safely through the broad Timer UI

This remains a cross-cutting product milestone. EPG/Timer -> permission -> revision-safe TimerIntent mutation -> visible assignment/fulfillment -> authoritative readback/reconciliation -> final state.

Acceptance preserves intent-first ownership, read-only enforcement, truthful `outcome_unknown`, no unsafe blind retry and no browser use of private SuiteBridge/SVDRP Timer writes.

---

## Relationship to phase completion

```text
Phase 64 [completed] -> engine portions of Journeys 3, 4 and Timer-related Journey 5
Phase 65 [completed] -> Journeys 1 and 2 + media Journey 5
Phase 66 [next; not started] -> Journeys 6 and 7
Phase 67 -> Journeys 8 and 9
Phase 68 -> Journey 10
Broad Timer Product UI -> Journey 11 + user-facing Journey 3
Phase 69 -> public/client compatibility hardening
```

Phase 70 recommendation work must add its own user-visible journey before runtime acceptance.

## Change rule

New primary product surfaces should add or extend a Golden User Journey when technical component tests alone would not prove the user-visible outcome.

Do not create a separate journey for every internal slice. Journeys intentionally remain vertical, stable and product-oriented.