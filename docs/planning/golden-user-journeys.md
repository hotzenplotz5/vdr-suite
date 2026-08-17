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

This is cross-cutting and reused by Phases 64–68.

---

## Journey 6 — Teletext while watching Live TV

```text
Live TV
  -> Teletext available indication
  -> open Teletext
  -> page 100
  -> numeric page selection
  -> page/subpage navigation
  -> supported color/link navigation
  -> close Teletext
  -> Live TV remains usable
```

Acceptance proves:

- client consumes Suite Teletext service/page contracts, not OSD screenshots or provider cache paths;
- page identity is tied to the correct backend/channel/service;
- stale/incomplete cached data is marked truthfully;
- channel change invalidates or replaces the Teletext service context correctly;
- page/subpage navigation is deterministic;
- no raw VDR remote/plugin command channel is required for normal Teletext browsing.

This becomes a Phase-66 product journey after proposed ADR-0054 is accepted.

---

## Journey 7 — Launch one HbbTV broadcast application

```text
Live Channel
  -> HbbTV application available
  -> user launches application
  -> authorized BroadcastApplicationSession
  -> isolated HbbTV-capable runtime
  -> application becomes usable
  -> normalized remote/color-key interaction
  -> close or channel change
  -> deterministic application cleanup
```

Acceptance proves:

- application discovery comes from bounded backend-local broadcast evidence;
- browser/client never receives an unrestricted local plugin control endpoint;
- no general public arbitrary URL/JavaScript/raw-key API exists;
- application runtime is isolated from Suite administrative/session secrets;
- stale application context is fenced after channel/backend-generation changes;
- Suite-owned Live/Recording media continues to use Phase-65 MediaSession semantics;
- application close/channel change releases local runtime/provider resources.

External broadcaster/network failure is reported distinctly from Suite discovery/session failure.

This becomes a Phase-66 product journey after proposed ADR-0054 is accepted.

---

## Journey 8 — Use one legacy native OSD workflow safely

```text
open explicitly labeled Legacy OSD compatibility surface
  -> authorized LegacyOsdSession
  -> full authoritative OSD frame
  -> view-only navigation state
  -> optional controller lease for an authorized user
  -> allowlisted input
  -> observed resulting frame
  -> release/close
```

Acceptance proves:

- normal EPG/Timer/Recording/Streaming/Teletext/HbbTV functions are not routed through this journey when domain APIs exist;
- `osd.view` and `osd.control` are separate;
- read-only backend cannot obtain controller authority;
- one native surface has at most one active Suite controller lease;
- stale lease/generation/OSD epoch commands fail closed;
- sequence gaps force full resync rather than guessed display state;
- no shell, raw SVDRP, unrestricted plugin service or arbitrary key-code tunnel exists;
- OSD frame contents do not enter normal audit/log storage.

This is a Phase-67 product journey.

---

## Journey 9 — Manage a Timer safely through the broad Timer UI

This is a cross-cutting product milestone rather than a numbered phase.

```text
EPG or Timer screen
  -> authenticated actor/backend permission
  -> create/update/disable/cancel TimerIntent
  -> revision-safe request
  -> assignment/fulfillment state visible
  -> native readback/reconciliation visible
  -> final user-visible state
```

Acceptance proves:

- UI is intent-first rather than native-Timer-first;
- stale revision produces conflict instead of overwrite;
- read-only/permission denial is clear and server-enforced;
- `outcome_unknown` is not shown as verified failure or success;
- unsafe blind retry is not offered;
- primary/replica/failover state reflects durable engine truth;
- browser never calls private SuiteBridge/SVDRP Timer commands.

Prerequisites are completed Phase 62, completed Phase 64 and required account/backend access administration.

---

## Relationship to phase completion

```text
Phase 64 [completed]
  -> engine portions of Journeys 3, 4 and Timer-related Journey 5

Phase 65
  -> Journeys 1 and 2
  -> media portion of Journey 5

Phase 66
  -> Journeys 6 and 7

Phase 67
  -> Journey 8

Broad Timer Product UI milestone
  -> Journey 9
  -> user-facing completion of Journey 3

Phase 68
  -> hardens the public/client compatibility contracts underlying the implemented domains
```

Phase 69 recommendation work must add its own user-visible journey before runtime acceptance.

## Change rule

New primary product surfaces should add or extend a Golden User Journey when technical component tests alone would not prove the user-visible outcome.

Do not create a separate journey for every internal slice. Journeys intentionally remain vertical, stable and product-oriented.
